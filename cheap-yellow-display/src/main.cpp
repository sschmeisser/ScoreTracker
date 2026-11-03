#include <Arduino.h>
#include <esp_heap_caps.h>
#include "LGFX_config.h"
#include "config.h"
#include "game_data.h"
#include "live_data.h"
#include "cyd_ui.h"
#include "cyd_boot_screen.h"

// =============================================================================
// CYD SCORE TRACKER — MAIN LOOP
// -----------------------------------------------------------------------------
// Mirrors the round tracker's ../../src/main.cpp structure: serial host protocol,
// Hermite-eased page slide, BOOT button advance. Adds XPT2046 resistive touch
// (page / pause / card-detail zones) plus TOUCHCAL and TOUCHDEBUG diagnostics.
// =============================================================================

#ifndef PIN_BOOT_BTN
#define PIN_BOOT_BTN 0
#endif

// Hardware instances
static LGFX lcd;
static LGFX_Sprite canvas(&lcd);

// Caller-owned framebuffer for the sprite. LovyanGFX's createSprite uses a
// DMA-capable allocation, which on the classic ESP32 is too fragmented for a
// 320x240@16bpp (153600-byte) block. We allocate from general 8-bit heap and
// hand the pointer to setBuffer(); Preallocated buffers are never freed by the
// sprite, so g_canvas_buf must outlive it (file scope).
static uint8_t* g_canvas_buf = nullptr;

// --- Paging / animation state (mirrors round tracker) ---
static int current_page_idx = 0;
static int next_page_idx = 0;
static bool is_transitioning = false;
static uint32_t last_page_switch_ms = 0;
static uint32_t transition_start_ms = 0;

// --- Button / auto-cycle state ---
static bool btn_is_down = false;
static bool auto_tick_enabled = true;
static bool is_syncing = false;

// --- Touch state machine ---
static uint32_t last_touch_ms = 0;       // cooldown timestamp for last accepted tap
static bool touch_was_down = false;      // require release between taps

// --- Detail overlay state ---
static bool detail_open = false;
static int detail_game_idx = 0;

// --- Startup Splash State ---
static bool splash_mode = false;
static float splash_progress = 0.75f;
static String splash_status = "SYNCING LIVE ESPN FEEDS...";

static void push_boot_screen(float progress, const char* status_text) {
  splash_progress = progress;
  splash_status = status_text;
  for (int band = 0; band < 2; ++band) {
    int camera_y = band * CYD_BAND_H;
    cyd_draw_boot_screen(canvas, camera_y, progress, status_text);
    canvas.pushSprite(0, camera_y);
  }
}

// --- Diagnostics ---
static bool touchdebug_enabled = false;
static bool touchcal_active = false;
static bool testpat_active = false;

// -----------------------------------------------------------------------------
// TESTPAT: 8 equal vertical RGB bars across the full 320x240, drawn through the
// same two-band sprite/push mechanism as the normal UI. Lets the user identify
// channel swaps and mirroring directly on the panel.
// -----------------------------------------------------------------------------
static void draw_and_push_testpat() {
  static const uint16_t bars[8] = {0xF800, 0x07E0, 0x001F, 0xFFE0,
                                   0x07FF, 0xF81F, 0xFFFF, 0x0000};
  const int bar_w = CYD_W / 8;
  for (int band = 0; band < 2; ++band) {
    int camera_y = band * CYD_BAND_H;
    for (int b = 0; b < 8; ++b) {
      int x = b * bar_w;
      int w = (b == 7) ? (CYD_W - x) : bar_w;
      canvas.fillRect(x, 0, w, CYD_BAND_H, bars[b]);
    }
    canvas.pushSprite(0, camera_y);
  }
}

// -----------------------------------------------------------------------------
static void draw_and_push_sync(int loaded, int total) {
  for (int band = 0; band < 2; ++band) {
    int camera_y = band * CYD_BAND_H;
    cyd_draw_sync_screen(canvas, camera_y, loaded, total);
    canvas.pushSprite(0, camera_y);
  }
}

// -----------------------------------------------------------------------------
// Blocking serial scores upload (same handshake as the round tracker)
// -----------------------------------------------------------------------------
static void handle_scores_start(int count) {
  is_syncing = true;
  if (count <= 0) count = 32;
  draw_and_push_sync(0, count);
  Serial.printf("[SERIAL_SCORES] READY %d\n", count);

  int loaded = 0;
  uint32_t start_rx = millis();
  String line_buffer = "";

  while (loaded < count && (millis() - start_rx < 20000)) {
    while (Serial.available()) {
      char c = (char)Serial.read();
      if (c == '\n') {
        line_buffer.trim();
        if (line_buffer.length() == 0) continue;

        if (line_buffer.indexOf("SCORES_END") >= 0 || line_buffer.indexOf("ABORT") >= 0) {
          goto sync_done;
        }
        if (line_buffer.startsWith("SCORES_START")) {
          count = line_buffer.substring(12).toInt();
          if (count <= 0) count = 32;
          loaded = 0;
          draw_and_push_sync(loaded, count);
          Serial.printf("[SERIAL_SCORES] READY %d\n", count);
          line_buffer = "";
          start_rx = millis();
          continue;
        }
        if (parse_and_store_game_json(loaded, line_buffer.c_str())) {
          loaded++;
          draw_and_push_sync(loaded, count);
          Serial.printf("[SERIAL_SCORES] ACK %d/%d\n", loaded, count);
        } else {
          Serial.printf("[SERIAL_SCORES] NACK %d/%d\n", loaded + 1, count);
        }
        line_buffer = "";
        start_rx = millis();
      } else if (c != '\r') {
        line_buffer += c;
      }
    }
    delay(1);
  }

sync_done:
  finalize_live_games(loaded);
  is_syncing = false;
  detail_open = false;
  current_page_idx = 0;
  next_page_idx = 0;
  is_transitioning = false;
  last_page_switch_ms = millis();
  Serial.printf("[SERIAL_SCORES] SUCCESS %d\n", loaded);
}

// -----------------------------------------------------------------------------
// TOUCHCAL: stream raw + normalized touch coordinates until a line is received
// -----------------------------------------------------------------------------
static void run_touchcal_tick() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    (void)line;  // any line (or "TOUCHCAL END") exits calibration
    touchcal_active = false;
    Serial.println("[TOUCHCAL] Calibration mode ended.");
    return;
  }

  uint16_t raw_x = 0, raw_y = 0;
  if (lcd.getTouchRaw(&raw_x, &raw_y)) {
    uint16_t nx = 0, ny = 0;
    lcd.getTouch(&nx, &ny);
    Serial.printf("[TOUCHCAL] raw=(%d,%d) norm=(%d,%d)\n",
                  (int)raw_x, (int)raw_y, (int)nx, (int)ny);
  }
}

// -----------------------------------------------------------------------------
static int eased_offset_y(uint32_t now_ms) {
  if (!is_transitioning) return 0;
  float p = (float)(now_ms - transition_start_ms) / (float)TRANSITION_DURATION_MS;
  if (p > 1.0f) p = 1.0f;
  float ease = p * p * (3.0f - 2.0f * p);
  return (int)(ease * (float)CYD_H);
}

static void begin_transition(int target_page, uint32_t now_ms) {
  next_page_idx = target_page;
  is_transitioning = true;
  transition_start_ms = now_ms;
}

void setup() {
  Serial.setRxBufferSize(4096);
  Serial.begin(115200);
  Serial.setTimeout(50);
  delay(100);

  pinMode(PIN_BOOT_BTN, INPUT_PULLUP);

  lcd.init();
  // CYD rotations 0..3 are mirrored; 4..7 give the correct landscape orientation.
  lcd.setRotation(6);
  lcd.setBrightness(BRIGHTNESS_MED);

  Serial.printf("[MEM] free=%u maxAlloc=%u largestDMA=%u largest8bit=%u\n",
                (unsigned)ESP.getFreeHeap(),
                (unsigned)ESP.getMaxAllocHeap(),
                (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_DMA),
                (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));

  Serial.printf("[NFL] Free heap before sprite: %u bytes\n", (unsigned)ESP.getFreeHeap());
  const size_t canvas_bytes = (size_t)CYD_W * CYD_BAND_H * 2;
  g_canvas_buf = (uint8_t*)heap_caps_malloc(canvas_bytes, MALLOC_CAP_8BIT);
  if (g_canvas_buf == nullptr) {
    g_canvas_buf = (uint8_t*)malloc(canvas_bytes);
  }

  canvas.setColorDepth(16);
  if (g_canvas_buf != nullptr) {
    canvas.setBuffer(g_canvas_buf, CYD_W, CYD_BAND_H, 16);
  }
  Serial.printf("[NFL] Free heap after sprite:  %u bytes\n", (unsigned)ESP.getFreeHeap());

  if (g_canvas_buf == nullptr || canvas.getBuffer() == nullptr) {
    Serial.printf("[NFL] FATAL: sprite allocation failed (%dx%d @16bpp)\n", CYD_W, CYD_H);
    while (true) {
      delay(1000);
    }
  }

  // 1. Initial boot screen: Hardware & Subsystems initialized
  push_boot_screen(0.18f, "INITIALIZING SYSTEM HARDWARE...");
  delay(150);

  // 2. Connect to Wi-Fi with animated progress updates
  push_boot_screen(0.35f, "CONNECTING TO WI-FI...");
  init_live_data();

  uint32_t wifi_start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - wifi_start < 7500)) {
    float p = 0.35f + 0.28f * ((millis() - wifi_start) / 7500.0f);
    push_boot_screen(p, "CONNECTING TO WI-FI...");
    delay(100);
  }

  if (WiFi.status() == WL_CONNECTED) {
    char ip_buf[48];
    snprintf(ip_buf, sizeof(ip_buf), "WI-FI CONNECTED: %s", WiFi.localIP().toString().c_str());
    push_boot_screen(0.68f, ip_buf);
    delay(200);
  } else {
    push_boot_screen(0.50f, "PROBING OPEN WI-FI ACCESS...");
    if (cyd_wifi_try_open_networks(canvas)) {
      char ip_buf[48];
      snprintf(ip_buf, sizeof(ip_buf), "OPEN WI-FI CONNECTED: %s", WiFi.localIP().toString().c_str());
      push_boot_screen(0.68f, ip_buf);
      delay(300);
    } else {
      g_wifi_ui_state = WIFI_UI_INACTIVE;
      push_boot_screen(0.68f, "WI-FI DISCONNECTED (TAP HEADER TO SETUP)");
      delay(300);
    }
  }

  // 3. Fetch ESPN Live feeds across leagues
  push_boot_screen(0.85f, "SYNCING LIVE ESPN FEEDS...");
  fetch_live_scores();

  // 4. Ready!
  push_boot_screen(1.00f, "SYSTEM READY! LAUNCHING DESK...");
  delay(450);

  last_page_switch_ms = millis();

  // Push an initial frame of the scoreboard
  const SportGame* games = get_active_games();
  int total_games = get_active_games_count();
  for (int band = 0; band < 2; ++band) {
    int camera_y = band * CYD_BAND_H;
    cyd_draw_score_tracker(canvas, camera_y, 0, 0, 0, false, games, total_games,
                           !auto_tick_enabled, is_wifi_enabled());
    canvas.pushSprite(0, camera_y);
  }

  Serial.printf("[NFL] CYD Zen Score Tracker initialized. %d active games loaded.\n",
                total_games);
}

void loop() {
  uint32_t now_ms = millis();

  // 0. Calibration mode monopolizes the loop until dismissed.
  if (touchcal_active) {
    run_touchcal_tick();
    delay(16);
    return;
  }

  // Poll Wi-Fi & ESPN live score updates:
  // If user selected an active game full-screen, update every 15s; otherwise 90s.
  const SportGame* cur_games = get_active_games();
  int cur_total = get_active_games_count();
  bool is_active_fullscreen = false;
  if (detail_open && detail_game_idx >= 0 && detail_game_idx < cur_total) {
    const char* st = cur_games[detail_game_idx].status;
    if (st && !cyd_status_is_final(st) && cyd_status_is_live(st)) {
      is_active_fullscreen = true;
    }
  }
  poll_live_data(is_active_fullscreen ? 15000 : 90000);

  const SportGame* games = get_active_games();
  int total_games = get_active_games_count();
  int total_pages = (total_games + 2) / 3;
  if (total_pages < 1) total_pages = 1;

  // Keep paging / detail state coherent across data refreshes.
  if (current_page_idx >= total_pages) current_page_idx = total_pages - 1;
  if (next_page_idx >= total_pages) next_page_idx = total_pages - 1;
  if (current_page_idx < 0) current_page_idx = 0;
  if (next_page_idx < 0) next_page_idx = 0;
  if (detail_open && detail_game_idx >= total_games) {
    detail_game_idx = (total_games > 0) ? 0 : 0;
    if (total_games == 0) detail_open = false;
  }

  // 1. USB CDC Host Commands (screenshot capture & interactive control)
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "SCREENSHOT") {
      Serial.println("SCREENSHOT_START");
      const size_t band_bytes = (size_t)CYD_W * CYD_BAND_H * 2;
      int shot_offset_y = eased_offset_y(now_ms);
      for (int band = 0; band < 2; ++band) {
        int camera_y = band * CYD_BAND_H;
        if (g_wifi_ui_state != WIFI_UI_INACTIVE) {
          cyd_draw_wifi_ui(canvas, camera_y);
        } else if (splash_mode) {
          cyd_draw_boot_screen(canvas, camera_y, splash_progress, splash_status.c_str());
        } else if (detail_open && detail_game_idx >= 0 && detail_game_idx < total_games) {
          cyd_draw_detail_screen(canvas, camera_y, games[detail_game_idx]);
        } else if (is_syncing) {
          cyd_draw_sync_screen(canvas, camera_y, 0, 0);
        } else {
          cyd_draw_score_tracker(canvas, camera_y, current_page_idx, shot_offset_y,
                                 next_page_idx, is_transitioning, games, total_games,
                                 !auto_tick_enabled, is_wifi_enabled());
        }
        uint8_t* buf = (uint8_t*)canvas.getBuffer();
        size_t sent = 0;
        while (sent < band_bytes) {
          int avail = Serial.availableForWrite();
          if (avail <= 0) {
            delay(1);
            continue;
          }
          size_t chunk = (band_bytes - sent > (size_t)avail) ? (size_t)avail : (band_bytes - sent);
          size_t written = Serial.write(buf + sent, chunk);
          sent += written;
        }
      }
      Serial.flush();
      Serial.println("\nSCREENSHOT_END");
    } else if (cmd == "NEXT") {
      detail_open = false;
      begin_transition((current_page_idx + 1) % total_pages, now_ms);
      Serial.printf("[NFL] Next Page: %d/%d\n", next_page_idx + 1, total_pages);
    } else if (cmd == "PREV") {
      detail_open = false;
      begin_transition((current_page_idx - 1 + total_pages) % total_pages, now_ms);
      Serial.printf("[NFL] Prev Page: %d/%d\n", next_page_idx + 1, total_pages);
    } else if (cmd.startsWith("SET ")) {
      detail_open = false;
      int idx = cmd.substring(4).toInt();
      if (idx >= 0 && idx < total_pages) {
        current_page_idx = idx;
        next_page_idx = idx;
        is_transitioning = false;
        last_page_switch_ms = now_ms;
        Serial.printf("[NFL] Set to Page %d/%d\n", current_page_idx + 1, total_pages);
      }
    } else if (cmd == "BACK" || cmd == "CLOSE") {
      detail_open = false;
      Serial.println("[NFL] Detail closed");
    } else if (cmd.startsWith("DETAIL") || cmd.startsWith("SELECT")) {
      String arg = "";
      int space_idx = cmd.indexOf(' ');
      if (space_idx > 0) arg = cmd.substring(space_idx + 1);
      arg.trim();
      int found_idx = -1;
      if (arg.length() > 0 && isDigit(arg[0])) {
        found_idx = arg.toInt();
      } else if (arg.length() > 0) {
        for (int i = 0; i < total_games; ++i) {
          if (strcasecmp(games[i].away_code, arg.c_str()) == 0 ||
              strcasecmp(games[i].home_code, arg.c_str()) == 0) {
            found_idx = i;
            break;
          }
        }
      } else {
        found_idx = 0;
      }
      if (found_idx >= 0 && found_idx < total_games) {
        detail_open = true;
        detail_game_idx = found_idx;
        Serial.printf("[NFL] Detail opened for game %d (%s @ %s)\n",
                      found_idx, games[found_idx].away_code, games[found_idx].home_code);
      }
    } else if (cmd == "PAUSE") {
      auto_tick_enabled = false;
      Serial.println("[NFL] Auto-tick paused");
    } else if (cmd == "RESUME") {
      splash_mode = false;
      auto_tick_enabled = true;
      last_page_switch_ms = now_ms;
      Serial.println("[NFL] Auto-tick resumed");
    } else if (cmd == "SPLASH" || cmd == "BOOT") {
      splash_mode = true;
      auto_tick_enabled = false;
      push_boot_screen(0.85f, "SYNCING LIVE ESPN FEEDS...");
      Serial.println("[SYSTEM] Startup splash screen displayed on LCD.");
    } else if (cmd == "DESK") {
      splash_mode = false;
      detail_open = false;
      auto_tick_enabled = true;
      last_page_switch_ms = now_ms;
      Serial.println("[SYSTEM] Resumed live scoreboard.");
    } else if (cmd == "SYNC_START") {
      is_syncing = true;
      detail_open = false;
      draw_and_push_sync(0, 0);
      Serial.println("[SERIAL_SCORES] SYNC_READY");
    } else if (cmd.startsWith("SCORES_START")) {
      handle_scores_start(cmd.substring(12).toInt());
    } else if (cmd.startsWith("SCORES_JSON ")) {
      String json_str = cmd.substring(12);
      int loaded = parse_and_store_games_json_array(json_str.c_str());
      if (loaded > 0) {
        detail_open = false;
        current_page_idx = 0;
        next_page_idx = 0;
        is_transitioning = false;
        last_page_switch_ms = millis();
        Serial.printf("[SERIAL_SCORES] SUCCESS %d\n", loaded);
      } else {
        Serial.println("[SERIAL_SCORES] ERROR");
      }
    } else if (cmd == "WIFI OFF") {
      set_wifi_enabled(false);
    } else if (cmd == "WIFI ON") {
      set_wifi_enabled(true);
    } else if (cmd == "WIFI SETUP" || cmd == "WIFI MANAGER") {
      auto_tick_enabled = false;
      cyd_wifi_open_manager(canvas);
      Serial.println("[WIFI] Opened Wi-Fi Network Picker on LCD.");
    } else if (cmd == "WIFI AUTO") {
      auto_tick_enabled = false;
      Serial.println("[WIFI] Probing open networks with verified internet access...");
      if (cyd_wifi_try_open_networks(canvas)) {
        fetch_live_scores();
        auto_tick_enabled = true;
        last_page_switch_ms = millis();
        Serial.println("[WIFI] SUCCESS: Open Wi-Fi connected and verified!");
      } else {
        Serial.println("[WIFI] FAILED: No open Wi-Fi with internet access found.");
      }
    } else if (cmd == "WIFI SCAN") {
      cyd_wifi_scan_networks();
      Serial.printf("[WIFI] Scanned %d unique 2.4GHz networks:\n", g_scanned_count);
      for (int i = 0; i < g_scanned_count; ++i) {
        Serial.printf("  [%2d] SSID: '%-20s' | RSSI: %3d dBm | Security: %s\n",
                      i, g_scanned_nets[i].ssid, g_scanned_nets[i].rssi,
                      g_scanned_nets[i].is_open ? "OPEN (UNPROTECTED)" : "PROTECTED");
      }
    } else if (cmd == "WIFI CLEAR") {
      cyd_wifi_clear_credentials();
      Serial.println("[WIFI] Stored flash credentials cleared.");
    } else if (cmd.startsWith("WIFI CONNECT ") || cmd.startsWith("CONNECT ")) {
      String rest = cmd.startsWith("WIFI CONNECT ") ? cmd.substring(13) : cmd.substring(8);
      rest.trim();
      int sp = rest.indexOf(' ');
      String ssid = (sp > 0) ? rest.substring(0, sp) : rest;
      String pass = (sp > 0) ? rest.substring(sp + 1) : "";
      ssid.trim(); pass.trim();
      strncpy(g_target_ssid, ssid.c_str(), sizeof(g_target_ssid) - 1);
      g_target_ssid[sizeof(g_target_ssid) - 1] = '\0';
      strncpy(g_entered_pass, pass.c_str(), sizeof(g_entered_pass) - 1);
      g_entered_pass[sizeof(g_entered_pass) - 1] = '\0';
      g_entered_pass_len = strlen(g_entered_pass);
      Serial.printf("[WIFI] Attempting connection to '%s' on the fly...\n", g_target_ssid);
      if (cyd_wifi_connect_to_target(canvas)) {
        fetch_live_scores();
        auto_tick_enabled = true;
        last_page_switch_ms = millis();
        Serial.println("[WIFI] CONNECTED & VERIFIED! Resumed scoreboard.");
      } else {
        Serial.println("[WIFI] CONNECTION FAILED.");
      }
    } else if (cmd.startsWith("WIFI KEYBOARD ")) {
      String net_name = cmd.substring(14);
      net_name.trim();
      strncpy(g_target_ssid, net_name.c_str(), sizeof(g_target_ssid) - 1);
      g_target_ssid[sizeof(g_target_ssid) - 1] = '\0';
      g_entered_pass[0] = '\0';
      g_entered_pass_len = 0;
      g_wifi_ui_state = WIFI_UI_KEYBOARD;
      cyd_push_wifi_frame(canvas);
      Serial.printf("[WIFI] Opened Touch Keyboard for '%s'.\n", g_target_ssid);
    } else if (cmd.startsWith("TOUCH ")) {
      int sp = cmd.indexOf(' ', 6);
      if (sp > 0) {
        int tx = cmd.substring(6, sp).toInt();
        int ty = cmd.substring(sp + 1).toInt();
        Serial.printf("[TOUCH_SIM] Injected touch (%d, %d)\n", tx, ty);
        if (g_wifi_ui_state != WIFI_UI_INACTIVE) {
          bool scores_ready = false;
          cyd_wifi_handle_touch(tx, ty, canvas, scores_ready);
          if (scores_ready) {
            fetch_live_scores();
            auto_tick_enabled = true;
          }
        } else if (detail_open) {
          detail_open = false;
          last_page_switch_ms = now_ms;
        } else {
          int card_idx = -1;
          CydTouchAction act = cyd_touch_hit(tx, ty, &card_idx);
          if (act == CYD_TOUCH_WIFI) {
            cyd_wifi_open_manager(canvas);
          } else if (act == CYD_TOUCH_NEXT) {
            begin_transition((current_page_idx + 1) % total_pages, now_ms);
          } else if (act == CYD_TOUCH_PREV) {
            begin_transition((current_page_idx - 1 + total_pages) % total_pages, now_ms);
          } else if (act == CYD_TOUCH_CARD && card_idx >= 0) {
            detail_open = true;
            detail_game_idx = current_page_idx * 3 + card_idx;
          }
        }
      }
    } else if (cmd == "STATUS") {
      Serial.printf("[STATUS] WiFi: %s | Games: %d | Page: %d/%d | AutoTick: %s | Detail: %s | TouchDbg: %s\n",
                    is_wifi_enabled() ? "ON" : "OFF",
                    total_games,
                    current_page_idx + 1, total_pages,
                    auto_tick_enabled ? "ON" : "PAUSED",
                    detail_open ? "OPEN" : "CLOSED",
                    touchdebug_enabled ? "ON" : "OFF");
    } else if (cmd == "REFRESH" || cmd == "SYNC") {
      Serial.println("[NFL] Manually triggering ESPN live fetch...");
      fetch_live_scores();
    } else if (cmd.startsWith("LEAGUE ") || cmd == "LEAGUE") {
      if (cmd.startsWith("LEAGUE ")) {
        String lg = cmd.substring(7);
        lg.trim();
        lg.toUpperCase();
        set_active_league(lg.c_str());
        detail_open = false;
        current_page_idx = 0;
        next_page_idx = 0;
        is_transitioning = false;
        Serial.printf("[LEAGUE] Switching to %s...\n", lg.c_str());
        fetch_live_scores();
      }
      Serial.printf("[LEAGUE] Active league: %s\n", get_active_league());
    } else if (cmd == "TOUCHDEBUG") {
      touchdebug_enabled = !touchdebug_enabled;
      Serial.printf("[TOUCH] Debug %s\n", touchdebug_enabled ? "ON" : "OFF");
    } else if (cmd == "TOUCHCAL") {
      touchcal_active = true;
      Serial.println("[TOUCHCAL] Streaming raw + normalized touch coords. Send any line to end.");
    } else if (cmd.startsWith("ROT ")) {
      int rot = cmd.substring(4).toInt();
      if (rot >= 0 && rot <= 7) {
        lcd.setRotation(rot);
        Serial.printf("[DISP] Rotation set to %d (%dx%d)\n", rot,
                      (int)lcd.width(), (int)lcd.height());
      } else {
        Serial.println("[DISP] ROT expects 0..7");
      }
    } else if (cmd.startsWith("INVERT")) {
      bool value;
      if (cmd.length() > 7) {
        value = (cmd.substring(7).toInt() != 0);
      } else {
        value = !lcd.getInvert();
      }
      lcd.invertDisplay(value);
      Serial.printf("[DISP] Invert set to %d\n", lcd.getInvert() ? 1 : 0);
    } else if (cmd == "TESTPAT") {
      testpat_active = !testpat_active;
      Serial.printf("[DISP] Test pattern %s\n", testpat_active ? "ON" : "OFF");
    }
  }

  // 2. Touch acquisition (XPT2046, polled)
  uint16_t x = 0, y = 0;
  bool touched = lcd.getTouch(&x, &y);

  if (touchdebug_enabled) {
    uint16_t rx = 0, ry = 0;
    bool raw = lcd.getTouchRaw(&rx, &ry);
    Serial.printf("[TOUCH] down=%d norm=(%d,%d) raw=%d:(%d,%d)\n",
                  touched ? 1 : 0, (int)x, (int)y, raw ? 1 : 0, (int)rx, (int)ry);
  }

  // 2a. Wi-Fi UI Active: Wi-Fi manager owns display & touch
  if (g_wifi_ui_state != WIFI_UI_INACTIVE) {
    if (touched) {
      bool fresh_tap = !touch_was_down && (now_ms - last_touch_ms >= TOUCH_DEBOUNCE_MS);
      touch_was_down = true;
      if (fresh_tap) {
        last_touch_ms = now_ms;
        bool scores_ready = false;
        cyd_wifi_handle_touch((int)x, (int)y, canvas, scores_ready);
        if (scores_ready) {
          fetch_live_scores();
          last_page_switch_ms = now_ms;
          auto_tick_enabled = true;
        }
      }
    } else {
      touch_was_down = false;
    }

    for (int band = 0; band < 2; ++band) {
      int camera_y = band * CYD_BAND_H;
      cyd_draw_wifi_ui(canvas, camera_y);
      canvas.pushSprite(0, camera_y);
    }
    delay(16);
    return;
  }

  if (touched) {
    bool fresh_tap = !touch_was_down && (now_ms - last_touch_ms >= TOUCH_DEBOUNCE_MS);
    touch_was_down = true;

    if (fresh_tap && !is_syncing) {
      last_touch_ms = now_ms;

      if (splash_mode) {
        splash_mode = false;
        auto_tick_enabled = true;
        last_page_switch_ms = now_ms;
        Serial.println("[TOUCH] Dismissed splash screen.");
      } else if (detail_open) {
        // Any further tap dismisses the detail overlay (CYD_TOUCH_DETAIL_BACK).
        int card_idx = -1;
        CydTouchAction act = cyd_touch_hit((int)x, (int)y, &card_idx);
        (void)act;  // any tap returns, regardless of zone
        detail_open = false;
        is_transitioning = false;
        last_page_switch_ms = now_ms;
        Serial.println("[TOUCH] Detail closed");
      } else {
        int card_idx = -1;
        CydTouchAction act = cyd_touch_hit((int)x, (int)y, &card_idx);
        switch (act) {
          case CYD_TOUCH_PREV:
            begin_transition((current_page_idx - 1 + total_pages) % total_pages, now_ms);
            Serial.printf("[TOUCH] Prev Page: %d/%d\n", next_page_idx + 1, total_pages);
            break;
          case CYD_TOUCH_NEXT:
            begin_transition((current_page_idx + 1) % total_pages, now_ms);
            Serial.printf("[TOUCH] Next Page: %d/%d\n", next_page_idx + 1, total_pages);
            break;
          case CYD_TOUCH_TOGGLE:
            auto_tick_enabled = !auto_tick_enabled;
            last_page_switch_ms = now_ms;
            Serial.printf("[TOUCH] Auto-tick %s\n", auto_tick_enabled ? "RESUMED" : "PAUSED");
            break;
          case CYD_TOUCH_CARD: {
            // TODO(interface): cyd_touch_hit has no page argument, so card_index
            // is assumed page-relative (0..2) within the current page.
            int abs_idx = current_page_idx * 3 + card_idx;
            if (card_idx >= 0 && abs_idx >= 0 && abs_idx < total_games) {
              detail_open = true;
              detail_game_idx = abs_idx;
              is_transitioning = false;
              Serial.printf("[TOUCH] Detail game %d (%s @ %s)\n",
                            abs_idx, games[abs_idx].away_code, games[abs_idx].home_code);
            }
            break;
          }
          case CYD_TOUCH_WIFI:
            auto_tick_enabled = false;
            cyd_wifi_open_manager(canvas);
            Serial.println("[TOUCH] Opened Wi-Fi Manager from header tap.");
            break;
          case CYD_TOUCH_DETAIL_BACK:
            // Overlay already closed above; ignore here.
            break;
          default:
            break;
        }
      }
    }
  } else {
    touch_was_down = false;
  }

  // 3. Physical Button Handling (GPIO 0 BOOT Button, active LOW, edge-triggered + long press)
  static uint32_t btn_down_start_ms = 0;
  static bool btn_long_fired = false;
  int btn_reading = digitalRead(PIN_BOOT_BTN);
  if (btn_reading == LOW) {
    if (!btn_is_down) {
      btn_is_down = true;
      btn_down_start_ms = now_ms;
      btn_long_fired = false;
    } else if (!btn_long_fired && (now_ms - btn_down_start_ms >= 1500)) {
      btn_long_fired = true;
      Serial.println("[BTN] Long press detected (>1.5s): Opening Wi-Fi Setup...");
      cyd_wifi_open_manager(canvas);
      auto_tick_enabled = false;
    }
  } else {
    if (btn_is_down) {
      btn_is_down = false;
      if (!btn_long_fired) {
        if (!detail_open && !is_syncing && g_wifi_ui_state == WIFI_UI_INACTIVE) {
          begin_transition((current_page_idx + 1) % total_pages, now_ms);
          Serial.printf("[BTN] Advanced to Page %d/%d\n", next_page_idx + 1, total_pages);
        }
      }
    }
  }

  // 4. Auto-Cycle Timer (HOLD_DURATION_MS hold, then TRANSITION_DURATION_MS glide)
  if (!is_transitioning) {
    if (auto_tick_enabled && !detail_open && !is_syncing && !splash_mode &&
        (now_ms - last_page_switch_ms >= HOLD_DURATION_MS)) {
      begin_transition((current_page_idx + 1) % total_pages, now_ms);
    }
  } else {
    uint32_t trans_elapsed = now_ms - transition_start_ms;
    if (trans_elapsed >= TRANSITION_DURATION_MS) {
      is_transitioning = false;
      current_page_idx = next_page_idx;
      last_page_switch_ms = now_ms;
    }
  }

  // 5. Calculate smooth Hermite eased offset (mirror round tracker)
  int offset_y = eased_offset_y(now_ms);

  // 6. Render + push (skip while a blocking serial sync owns the panel)
  if (!is_syncing) {
    if (splash_mode) {
      for (int band = 0; band < 2; ++band) {
        int camera_y = band * CYD_BAND_H;
        cyd_draw_boot_screen(canvas, camera_y, splash_progress, splash_status.c_str());
        canvas.pushSprite(0, camera_y);
      }
    } else if (testpat_active) {
      draw_and_push_testpat();
    } else {
      for (int band = 0; band < 2; ++band) {
        int camera_y = band * CYD_BAND_H;
        if (detail_open && detail_game_idx >= 0 && detail_game_idx < total_games) {
          cyd_draw_detail_screen(canvas, camera_y, games[detail_game_idx]);
        } else {
          cyd_draw_score_tracker(canvas, camera_y, current_page_idx, offset_y, next_page_idx,
                                 is_transitioning, games, total_games,
                                 !auto_tick_enabled, is_wifi_enabled());
        }
        canvas.pushSprite(0, camera_y);
      }
    }
  }

  // Maintain smooth ~60 FPS
  delay(16);
}
