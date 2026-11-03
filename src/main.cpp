#include <Arduino.h>
#include "LGFX_config.h"
#include "nfl_tracker.h"
#include "live_nfl.h"

#ifndef PIN_BOOT_BTN
#define PIN_BOOT_BTN 9
#endif

// Hardware instances
static LGFX lcd;
static LGFX_Sprite canvas(&lcd);

// Zen Score Tracker State (2 rows per screen)
static int current_page_idx = 0;
static int next_page_idx = 0;
static bool is_transitioning = false;
static uint32_t last_page_switch_ms = 0;
static uint32_t transition_start_ms = 0;

static const uint32_t HOLD_DURATION_MS = 6000;       // 6.0 seconds peaceful hold
static const uint32_t TRANSITION_DURATION_MS = 750;   // 750ms smooth liquid glide

// Button & auto-cycle state
static bool btn_is_down = false;
static bool auto_tick_enabled = true;
static bool is_syncing = false;

void setup() {
  Serial.setRxBufferSize(4096);
  Serial.begin(115200);
  Serial.setTimeout(50);
  delay(100);

  pinMode(PIN_BOOT_BTN, INPUT_PULLUP);

  lcd.init();
  lcd.setRotation(0);
  lcd.setBrightness(130); // Gentle, eye-soothing ambient brightness (no harsh glare)

  canvas.setColorDepth(16);
  canvas.createSprite(240, 240);

  last_page_switch_ms = millis();
  init_live_nfl();
  Serial.printf("[NFL] Zen NFL Score Tracker initialized. %d default games loaded.\n",
                TOTAL_NFL_GAMES);
}

void loop() {
  uint32_t now_ms = millis();

  // Poll Wi-Fi & ESPN live score updates
  poll_live_nfl();
  const NFLGame* games = get_active_nfl_games();
  int total_games = get_active_nfl_games_count();
  int total_pages = (total_games + 2) / 3;
  if (total_pages < 1) total_pages = 1;

  // 1. USB CDC Host Commands (Screenshot capture & interactive control)
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "SCREENSHOT") {
      uint8_t* buf = (uint8_t*)canvas.getBuffer();
      Serial.println("SCREENSHOT_START");
      const size_t total = 240 * 240 * 2;
      size_t sent = 0;
      while (sent < total) {
        int avail = Serial.availableForWrite();
        if (avail <= 0) {
          delay(1);
          continue;
        }
        size_t chunk = (total - sent > (size_t)avail) ? (size_t)avail : (total - sent);
        size_t written = Serial.write(buf + sent, chunk);
        sent += written;
      }
      Serial.flush();
      Serial.println("\nSCREENSHOT_END");
    } else if (cmd == "NEXT") {
      is_transitioning = true;
      transition_start_ms = now_ms;
      next_page_idx = (current_page_idx + 1) % total_pages;
      Serial.printf("[NFL] Next Page: %d/%d\n", next_page_idx + 1, total_pages);
    } else if (cmd == "PREV") {
      is_transitioning = true;
      transition_start_ms = now_ms;
      next_page_idx = (current_page_idx - 1 + total_pages) % total_pages;
      Serial.printf("[NFL] Prev Page: %d/%d\n", next_page_idx + 1, total_pages);
    } else if (cmd.startsWith("SET ")) {
      int idx = cmd.substring(4).toInt();
      if (idx >= 0 && idx < total_pages) {
        current_page_idx = idx;
        is_transitioning = false;
        last_page_switch_ms = now_ms;
        Serial.printf("[NFL] Set to Page %d/%d\n", current_page_idx + 1, total_pages);
      }
    } else if (cmd == "PAUSE") {
      auto_tick_enabled = false;
      Serial.println("[NFL] Auto-tick paused");
    } else if (cmd == "RESUME") {
      auto_tick_enabled = true;
      last_page_switch_ms = now_ms;
      Serial.println("[NFL] Auto-tick resumed");
    } else if (cmd == "SYNC_START") {
      is_syncing = true;
      draw_sync_loading_screen(canvas, 0, 0);
      canvas.pushSprite(0, 0);
      Serial.println("[SERIAL_SCORES] SYNC_READY");
    } else if (cmd.startsWith("SCORES_START")) {
      is_syncing = true;
      int count = cmd.substring(12).toInt();
      if (count <= 0) count = 32;
      draw_sync_loading_screen(canvas, 0, count);
      canvas.pushSprite(0, 0);
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
              draw_sync_loading_screen(canvas, 0, count);
              canvas.pushSprite(0, 0);
              Serial.printf("[SERIAL_SCORES] READY %d\n", count);
              line_buffer = "";
              start_rx = millis();
              continue;
            }
            if (parse_and_store_game_json(loaded, line_buffer.c_str())) {
              loaded++;
              draw_sync_loading_screen(canvas, loaded, count);
              canvas.pushSprite(0, 0);
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
      finalize_live_nfl_games(loaded);
      is_syncing = false;
      current_page_idx = 0;
      next_page_idx = 0;
      is_transitioning = false;
      last_page_switch_ms = millis();
      Serial.printf("[SERIAL_SCORES] SUCCESS %d\n", loaded);
    } else if (cmd.startsWith("SCORES_JSON ")) {
      String json_str = cmd.substring(12);
      int loaded = parse_and_store_games_json_array(json_str.c_str());
      if (loaded > 0) {
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
    } else if (cmd == "STATUS") {
      Serial.printf("[STATUS] WiFi: %s | Games: %d | Page: %d/%d | AutoTick: %s\n",
                    is_wifi_enabled() ? "ON" : "OFF",
                    get_active_nfl_games_count(),
                    current_page_idx + 1, total_pages,
                    auto_tick_enabled ? "ON" : "PAUSED");
    } else if (cmd == "REFRESH" || cmd == "SYNC") {
      Serial.println("[NFL] Manually triggering ESPN live fetch...");
      fetch_live_nfl_scores();
    }
  }

  // 2. Physical Button Handling (GPIO 9 BOOT Button)
  int btn_reading = digitalRead(PIN_BOOT_BTN);
  if (btn_reading == LOW) {
    if (!btn_is_down) {
      btn_is_down = true;
      is_transitioning = true;
      transition_start_ms = now_ms;
      next_page_idx = (current_page_idx + 1) % total_pages;
      Serial.printf("[BTN] Advanced to Page %d/%d\n", next_page_idx + 1, total_pages);
    }
  } else {
    if (btn_is_down) {
      btn_is_down = false;
    }
  }

  // 3. Auto-Cycle Timer (6.0s hold, then 750ms glide)
  if (!is_transitioning) {
    if (auto_tick_enabled && (now_ms - last_page_switch_ms >= HOLD_DURATION_MS)) {
      is_transitioning = true;
      transition_start_ms = now_ms;
      next_page_idx = (current_page_idx + 1) % total_pages;
    }
  } else {
    uint32_t trans_elapsed = now_ms - transition_start_ms;
    if (trans_elapsed >= TRANSITION_DURATION_MS) {
      is_transitioning = false;
      current_page_idx = next_page_idx;
      last_page_switch_ms = now_ms;
    }
  }

  // 4. Calculate Smooth Hermite Eased Offset
  int offset_y = 0;
  if (is_transitioning) {
    float p = (float)(now_ms - transition_start_ms) / (float)TRANSITION_DURATION_MS;
    if (p > 1.0f) p = 1.0f;
    float ease = p * p * (3.0f - 2.0f * p);
    offset_y = (int)(ease * 240.0f);
  }

  // 5. Render Calm 3-Row NFL Score Tracker
  if (!is_syncing) {
    draw_zen_score_tracker(canvas, current_page_idx, offset_y, next_page_idx, is_transitioning, games, total_games);
    // 6. Push to LCD via SPI DMA
    canvas.pushSprite(0, 0);
  }

  // Maintain smooth ~60 FPS
  delay(16);
}

