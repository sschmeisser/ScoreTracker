#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <LovyanGFX.hpp>
#include "config.h"

// =============================================================================
// CYD ON-THE-FLY WI-FI MANAGER & TOUCH QWERTY KEYBOARD
// =============================================================================
// Features:
// 1. NVS credential persistence (remembers Wi-Fi across reboots).
// 2. Auto-discovery and connection to open Wi-Fi with verified internet access
//    (probes site.api.espn.com:80 to ensure it's not a dead captive portal).
// 3. On-screen 320x240 Touch UI Network Picker with signal bars & lock badges.
// 4. On-screen Touch QWERTY Keyboard with letters, numbers, symbols, shift,
//    backspace, clear, and password visibility toggle.
// =============================================================================

enum CydWifiUiState {
  WIFI_UI_INACTIVE = 0,
  WIFI_UI_SCANNING,
  WIFI_UI_PICKER,
  WIFI_UI_KEYBOARD,
  WIFI_UI_CONNECTING,
  WIFI_UI_CONNECTED,
  WIFI_UI_FAILED
};

struct ScannedNetwork {
  char ssid[33];
  int rssi;
  uint8_t auth_mode;
  bool is_open;
};

// Global Wi-Fi Manager State
static CydWifiUiState g_wifi_ui_state = WIFI_UI_INACTIVE;
static ScannedNetwork g_scanned_nets[20];
static int g_scanned_count = 0;
static int g_picker_page = 0;
static const int NETS_PER_PAGE = 4;

static char g_target_ssid[33] = "";
static bool g_target_is_open = false;
static char g_entered_pass[65] = "";
static int g_entered_pass_len = 0;
static bool g_pass_masked = false;
static bool g_shift_active = false;
static bool g_symbols_active = false;

static char g_wifi_status_msg[64] = "";
static uint32_t g_wifi_ui_status_time = 0;

// -----------------------------------------------------------------------------
// NVS Credential Storage (Preferences)
// -----------------------------------------------------------------------------
inline void cyd_wifi_load_credentials(char* ssid_out, size_t ssid_sz, char* pass_out, size_t pass_sz) {
  Preferences prefs;
  bool ok = prefs.begin("scoretracker", true); // read-only
  String s = "";
  String p = "";
  if (ok) {
    s = prefs.getString("ssid", "");
    p = prefs.getString("pass", "");
    prefs.end();
  }

  if (s.length() > 0) {
    strncpy(ssid_out, s.c_str(), ssid_sz - 1);
    ssid_out[ssid_sz - 1] = '\0';
    strncpy(pass_out, p.c_str(), pass_sz - 1);
    pass_out[pass_sz - 1] = '\0';
    Serial.printf("[WIFI_NVS] Loaded saved SSID: '%s'\n", ssid_out);
  } else {
    strncpy(ssid_out, WIFI_SSID, ssid_sz - 1);
    ssid_out[ssid_sz - 1] = '\0';
    strncpy(pass_out, WIFI_PASS, pass_sz - 1);
    pass_out[pass_sz - 1] = '\0';
    Serial.printf("[WIFI_NVS] No saved SSID in flash. Using default: '%s'\n", ssid_out);
  }
}

inline void cyd_wifi_save_credentials(const char* ssid, const char* pass) {
  if (!ssid || !ssid[0]) return;
  Preferences prefs;
  prefs.begin("scoretracker", false); // read-write
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass ? pass : "");
  prefs.end();
  Serial.printf("[WIFI_NVS] Saved credentials for SSID '%s' to flash.\n", ssid);
}

inline void cyd_wifi_clear_credentials() {
  Preferences prefs;
  prefs.begin("scoretracker", false);
  prefs.clear();
  prefs.end();
  Serial.println("[WIFI_NVS] Cleared stored Wi-Fi credentials.");
}

// -----------------------------------------------------------------------------
// Active Internet Verification (Probes site.api.espn.com:80)
// -----------------------------------------------------------------------------
inline bool cyd_wifi_test_internet(uint32_t timeout_ms = 4000) {
  if (WiFi.status() != WL_CONNECTED) return false;

  WiFiClient client;
  client.setTimeout(timeout_ms / 1000);

  Serial.println("[WIFI_TEST] Probing site.api.espn.com:80 for internet connectivity...");
  if (!client.connect("site.api.espn.com", 80)) {
    Serial.println("[WIFI_TEST] Connection to site.api.espn.com:80 failed.");
    return false;
  }

  // Send minimal HEAD request to verify WAN routing and non-captive portal
  client.print("HEAD /apis/site/v2/sports/football/nfl/scoreboard HTTP/1.1\r\n"
               "Host: site.api.espn.com\r\n"
               "User-Agent: curl/8.0\r\n"
               "Connection: close\r\n\r\n");

  uint32_t start = millis();
  while (!client.available() && (millis() - start < timeout_ms)) {
    delay(10);
  }

  if (!client.available()) {
    client.stop();
    Serial.println("[WIFI_TEST] Timed out waiting for HTTP response.");
    return false;
  }

  String status_line = client.readStringUntil('\n');
  status_line.trim();
  client.stop();

  Serial.printf("[WIFI_TEST] Server response: '%s'\n", status_line.c_str());
  bool ok = status_line.startsWith("HTTP/1.1 200") || status_line.startsWith("HTTP/1.0 200");
  return ok;
}

// -----------------------------------------------------------------------------
// Wi-Fi Scanning & Network Sorting
// -----------------------------------------------------------------------------
inline void cyd_wifi_scan_networks() {
  Serial.println("[WIFI_SCAN] Scanning 2.4GHz Wi-Fi networks...");
  g_scanned_count = 0;
  g_picker_page = 0;

  int n = WiFi.scanNetworks(false, true); // synchronous, show hidden
  Serial.printf("[WIFI_SCAN] Found %d APs.\n", n);

  if (n <= 0) return;

  for (int i = 0; i < n && g_scanned_count < 20; ++i) {
    String s = WiFi.SSID(i);
    s.trim();
    if (s.length() == 0) continue; // skip hidden / empty

    // Check duplicate SSID (keep strongest RSSI)
    bool duplicate = false;
    for (int j = 0; j < g_scanned_count; ++j) {
      if (strcmp(g_scanned_nets[j].ssid, s.c_str()) == 0) {
        duplicate = true;
        if (WiFi.RSSI(i) > g_scanned_nets[j].rssi) {
          g_scanned_nets[j].rssi = WiFi.RSSI(i);
          g_scanned_nets[j].auth_mode = WiFi.encryptionType(i);
          g_scanned_nets[j].is_open = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
        }
        break;
      }
    }

    if (!duplicate) {
      strncpy(g_scanned_nets[g_scanned_count].ssid, s.c_str(), sizeof(g_scanned_nets[0].ssid) - 1);
      g_scanned_nets[g_scanned_count].ssid[sizeof(g_scanned_nets[0].ssid) - 1] = '\0';
      g_scanned_nets[g_scanned_count].rssi = WiFi.RSSI(i);
      g_scanned_nets[g_scanned_count].auth_mode = WiFi.encryptionType(i);
      g_scanned_nets[g_scanned_count].is_open = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
      g_scanned_count++;
    }
  }

  // Sort: Open networks first, then by signal strength (RSSI descending)
  for (int i = 0; i < g_scanned_count - 1; ++i) {
    for (int j = i + 1; j < g_scanned_count; ++j) {
      bool swap = false;
      if (g_scanned_nets[j].is_open && !g_scanned_nets[i].is_open) {
        swap = true;
      } else if (g_scanned_nets[j].is_open == g_scanned_nets[i].is_open) {
        if (g_scanned_nets[j].rssi > g_scanned_nets[i].rssi) swap = true;
      }
      if (swap) {
        ScannedNetwork temp = g_scanned_nets[i];
        g_scanned_nets[i] = g_scanned_nets[j];
        g_scanned_nets[j] = temp;
      }
    }
  }

  Serial.printf("[WIFI_SCAN] Filtered to %d unique networks.\n", g_scanned_count);
}

// -----------------------------------------------------------------------------
// Two-Band Sprite Coordinate Helper
// -----------------------------------------------------------------------------
#ifndef CYD_SY_DEFINED
#define CYD_SY_DEFINED
static inline int cyd_wifi_sy(int y, int camera_y) { return y - camera_y; }
#endif

// -----------------------------------------------------------------------------
// UI Drawing Functions
// -----------------------------------------------------------------------------
inline void cyd_draw_wifi_ui(LGFX_Sprite& canvas, int camera_y) {
  // Deep Forest Moss Background
  const uint16_t COL_BG       = 0x0122; // #08140B
  const uint16_t COL_CARD_BG  = 0x09C5; // #122818
  const uint16_t COL_CARD_BD  = 0x1A47; // #213C27
  const uint16_t COL_ACCENT   = 0x5609; // #53C04B Electric Neon Green
  const uint16_t COL_AMBER    = 0xD52B; // #D4A359 Warm Amber
  const uint16_t COL_TEXT     = 0xFFFF; // Pure White
  const uint16_t COL_SUB      = 0x8410; // Muted Silver
  const uint16_t COL_HAIR     = 0x1244; // Subtle separator
  const uint16_t COL_KEY_BG   = 0x1206; // Key button dark olive
  const uint16_t COL_KEY_BD   = 0x2288; // Key button border
  const uint16_t COL_KEY_ACT  = 0x2329; // Key active pressed
  const uint16_t COL_RED      = 0xD1A4; // Error Crimson

  canvas.fillScreen(COL_BG);

  // Common Header Bar (y=0..22)
  int hy = cyd_wifi_sy(11, camera_y);
  canvas.fillRect(0, cyd_wifi_sy(0, camera_y), 320, 22, COL_CARD_BG);
  canvas.drawFastHLine(0, cyd_wifi_sy(21, camera_y), 320, COL_CARD_BD);

  if (g_wifi_ui_state == WIFI_UI_PICKER || g_wifi_ui_state == WIFI_UI_SCANNING) {
    // Header Title
    canvas.setFont(&fonts::FreeSansBold9pt7b);
    canvas.setTextDatum(textdatum_t::middle_left);
    canvas.setTextColor(COL_TEXT);
    canvas.drawString("WI-FI SETUP", 8, hy);

    // Top action buttons: [AUTO] [SCAN] [X]
    // [AUTO OPEN] button (x=120..190)
    canvas.fillRoundRect(120, cyd_wifi_sy(2, camera_y), 74, 18, 3, COL_KEY_BG);
    canvas.drawRoundRect(120, cyd_wifi_sy(2, camera_y), 74, 18, 3, COL_ACCENT);
    canvas.setFont(&fonts::DejaVu9);
    canvas.setTextDatum(textdatum_t::middle_center);
    canvas.setTextColor(COL_ACCENT);
    canvas.drawString("AUTO OPEN", 157, hy);

    // [SCAN] button (x=200..255)
    canvas.fillRoundRect(200, cyd_wifi_sy(2, camera_y), 56, 18, 3, COL_KEY_BG);
    canvas.drawRoundRect(200, cyd_wifi_sy(2, camera_y), 56, 18, 3, COL_AMBER);
    canvas.setTextColor(COL_AMBER);
    canvas.drawString("RESCAN", 228, hy);

    // [EXIT / CLOSE] button (x=265..314)
    canvas.fillRoundRect(265, cyd_wifi_sy(2, camera_y), 48, 18, 3, COL_KEY_BG);
    canvas.drawRoundRect(265, cyd_wifi_sy(2, camera_y), 48, 18, 3, COL_SUB);
    canvas.setTextColor(COL_TEXT);
    canvas.drawString("DESK", 289, hy);

    if (g_wifi_ui_state == WIFI_UI_SCANNING) {
      canvas.setFont(&fonts::FreeSans9pt7b);
      canvas.setTextDatum(textdatum_t::middle_center);
      canvas.setTextColor(COL_AMBER);
      canvas.drawString("Scanning 2.4GHz networks...", 160, cyd_wifi_sy(110, camera_y));
      canvas.setFont(&fonts::DejaVu9);
      canvas.setTextColor(COL_SUB);
      canvas.drawString("Please wait a moment", 160, cyd_wifi_sy(135, camera_y));
      return;
    }

    // Network list rows (4 per page)
    int start_idx = g_picker_page * NETS_PER_PAGE;
    int total_pages = (g_scanned_count + NETS_PER_PAGE - 1) / NETS_PER_PAGE;
    if (total_pages < 1) total_pages = 1;

    for (int i = 0; i < NETS_PER_PAGE; ++i) {
      int idx = start_idx + i;
      int ry = 26 + i * 44;
      int sy = cyd_wifi_sy(ry, camera_y);

      if (idx < g_scanned_count) {
        const ScannedNetwork& net = g_scanned_nets[idx];
        bool is_curr = (WiFi.status() == WL_CONNECTED && strcmp(WiFi.SSID().c_str(), net.ssid) == 0);

        // Card container
        canvas.fillRoundRect(6, sy, 308, 40, 4, COL_CARD_BG);
        canvas.drawRoundRect(6, sy, 308, 40, 4, is_curr ? COL_ACCENT : COL_CARD_BD);

        // Lock / Open Badge
        if (net.is_open) {
          canvas.fillRoundRect(12, sy + 10, 46, 20, 3, 0x1A47);
          canvas.drawRoundRect(12, sy + 10, 46, 20, 3, COL_ACCENT);
          canvas.setFont(&fonts::Font0);
          canvas.setTextDatum(textdatum_t::middle_center);
          canvas.setTextColor(COL_ACCENT);
          canvas.drawString("OPEN", 35, sy + 20);
        } else {
          canvas.fillRoundRect(12, sy + 10, 46, 20, 3, 0x2A05);
          canvas.drawRoundRect(12, sy + 10, 46, 20, 3, COL_AMBER);
          canvas.setFont(&fonts::Font0);
          canvas.setTextDatum(textdatum_t::middle_center);
          canvas.setTextColor(COL_AMBER);
          canvas.drawString("LOCK", 35, sy + 20);
        }

        // SSID Name
        canvas.setFont(&fonts::FreeSansBold9pt7b);
        canvas.setTextDatum(textdatum_t::middle_left);
        canvas.setTextColor(COL_TEXT);
        // Truncate long SSID for display
        char disp_ssid[24];
        strncpy(disp_ssid, net.ssid, sizeof(disp_ssid) - 1);
        disp_ssid[sizeof(disp_ssid) - 1] = '\0';
        canvas.drawString(disp_ssid, 66, sy + 14);

        // Status / RSSI line
        canvas.setFont(&fonts::DejaVu9);
        canvas.setTextColor(is_curr ? COL_ACCENT : COL_SUB);
        char info_buf[32];
        if (is_curr) {
          snprintf(info_buf, sizeof(info_buf), "CONNECTED (%d dBm)", net.rssi);
        } else if (net.is_open) {
          snprintf(info_buf, sizeof(info_buf), "Tap to auto-connect (%d dBm)", net.rssi);
        } else {
          snprintf(info_buf, sizeof(info_buf), "Tap to enter pass (%d dBm)", net.rssi);
        }
        canvas.drawString(info_buf, 66, sy + 28);

        // Signal bars indicator on right side (x=280..302)
        int bars = 1;
        if (net.rssi > -60) bars = 4;
        else if (net.rssi > -70) bars = 3;
        else if (net.rssi > -80) bars = 2;

        for (int b = 0; b < 4; ++b) {
          int bh = 4 + b * 4;
          int bx = 286 + b * 5;
          int by = sy + 28 - bh;
          uint16_t bar_col = (b < bars) ? (net.is_open ? COL_ACCENT : COL_AMBER) : COL_CARD_BD;
          canvas.fillRect(bx, by, 3, bh, bar_col);
        }
      } else {
        // Empty slot placeholder
        canvas.drawRoundRect(6, sy, 308, 40, 4, COL_CARD_BD);
      }
    }

    // Bottom Footer / Pager Bar (y=210..238)
    int f_sy = cyd_wifi_sy(210, camera_y);
    canvas.drawFastHLine(0, f_sy, 320, COL_CARD_BD);

    // [PREV] button (x=8..68)
    if (g_picker_page > 0) {
      canvas.fillRoundRect(8, f_sy + 4, 60, 22, 3, COL_KEY_BG);
      canvas.drawRoundRect(8, f_sy + 4, 60, 22, 3, COL_CARD_BD);
      canvas.setFont(&fonts::DejaVu9);
      canvas.setTextDatum(textdatum_t::middle_center);
      canvas.setTextColor(COL_TEXT);
      canvas.drawString("< PREV", 38, f_sy + 15);
    }

    // Page indicator (x=160)
    canvas.setFont(&fonts::DejaVu9);
    canvas.setTextDatum(textdatum_t::middle_center);
    canvas.setTextColor(COL_SUB);
    char pbuf[24];
    snprintf(pbuf, sizeof(pbuf), "Page %d / %d (%d APs)", g_picker_page + 1, total_pages, g_scanned_count);
    canvas.drawString(pbuf, 160, f_sy + 15);

    // [NEXT] button (x=252..312)
    if (g_picker_page < total_pages - 1) {
      canvas.fillRoundRect(252, f_sy + 4, 60, 22, 3, COL_KEY_BG);
      canvas.drawRoundRect(252, f_sy + 4, 60, 22, 3, COL_CARD_BD);
      canvas.setFont(&fonts::DejaVu9);
      canvas.setTextDatum(textdatum_t::middle_center);
      canvas.setTextColor(COL_TEXT);
      canvas.drawString("NEXT >", 282, f_sy + 15);
    }

  } else if (g_wifi_ui_state == WIFI_UI_KEYBOARD) {
    // Header: Join Network & Cancel Button
    canvas.setFont(&fonts::FreeSansBold9pt7b);
    canvas.setTextDatum(textdatum_t::middle_left);
    canvas.setTextColor(COL_AMBER);
    char title_buf[32];
    snprintf(title_buf, sizeof(title_buf), "JOIN: %.16s", g_target_ssid);
    canvas.drawString(title_buf, 8, hy);

    // [CANCEL] button (x=255..314)
    canvas.fillRoundRect(255, cyd_wifi_sy(2, camera_y), 58, 18, 3, COL_KEY_BG);
    canvas.drawRoundRect(255, cyd_wifi_sy(2, camera_y), 58, 18, 3, COL_SUB);
    canvas.setFont(&fonts::DejaVu9);
    canvas.setTextDatum(textdatum_t::middle_center);
    canvas.setTextColor(COL_TEXT);
    canvas.drawString("CANCEL", 284, hy);

    // Password input field (y=26..56)
    int py = cyd_wifi_sy(26, camera_y);
    canvas.fillRoundRect(6, py, 225, 30, 4, COL_CARD_BG);
    canvas.drawRoundRect(6, py, 225, 30, 4, COL_ACCENT);

    // Render password or mask
    canvas.setFont(&fonts::FreeSans9pt7b);
    canvas.setTextDatum(textdatum_t::middle_left);
    canvas.setTextColor(COL_TEXT);
    if (g_entered_pass_len == 0) {
      canvas.setTextColor(COL_SUB);
      canvas.drawString("Enter password...", 14, py + 15);
    } else {
      char masked_str[65];
      if (g_pass_masked) {
        for (int m = 0; m < g_entered_pass_len; ++m) masked_str[m] = '*';
        masked_str[g_entered_pass_len] = '\0';
      } else {
        strncpy(masked_str, g_entered_pass, sizeof(masked_str) - 1);
        masked_str[sizeof(masked_str) - 1] = '\0';
      }
      canvas.drawString(masked_str, 14, py + 15);
    }

    // Action buttons beside input box:
    // [SHOW / HIDE] (x=236..276)
    canvas.fillRoundRect(236, py, 38, 30, 3, COL_KEY_BG);
    canvas.drawRoundRect(236, py, 38, 30, 3, COL_CARD_BD);
    canvas.setFont(&fonts::Font0);
    canvas.setTextDatum(textdatum_t::middle_center);
    canvas.setTextColor(g_pass_masked ? COL_SUB : COL_ACCENT);
    canvas.drawString(g_pass_masked ? "SHOW" : "HIDE", 255, py + 15);

    // [CLEAR] (x=278..314)
    canvas.fillRoundRect(278, py, 36, 30, 3, COL_KEY_BG);
    canvas.drawRoundRect(278, py, 36, 30, 3, COL_CARD_BD);
    canvas.setTextColor(COL_AMBER);
    canvas.drawString("CLR", 296, py + 15);

    // Subtle helper text (y=59)
    canvas.setFont(&fonts::Font0);
    canvas.setTextDatum(textdatum_t::middle_left);
    canvas.setTextColor(COL_SUB);
    char len_buf[32];
    snprintf(len_buf, sizeof(len_buf), "Length: %d chars", g_entered_pass_len);
    canvas.drawString(len_buf, 10, cyd_wifi_sy(66, camera_y));

    // Keyboard Key Layout (y=76..236, 5 rows)
    // Row definitions
    static const char* R0_NORM = "1234567890";
    static const char* R0_SYMB = "!@#$%^&*()";

    static const char* R1_NORM = "qwertyuiop";
    static const char* R1_SHFT = "QWERTYUIOP";
    static const char* R1_SYMB = "~`_-+={[]}\\";

    static const char* R2_NORM = "asdfghjkl-";
    static const char* R2_SHFT = "ASDFGHJKL_";
    static const char* R2_SYMB = ":;\"'<>,./?";

    static const char* R3_NORM = "zxcvbnm.";
    static const char* R3_SHFT = "ZXCVBNM,";

    // Row 0: Numbers / Symbols (10 keys, w=28, gap=3)
    int y0 = cyd_wifi_sy(76, camera_y);
    for (int k = 0; k < 10; ++k) {
      int kx = 6 + k * 31;
      canvas.fillRoundRect(kx, y0, 28, 28, 3, COL_KEY_BG);
      canvas.drawRoundRect(kx, y0, 28, 28, 3, COL_KEY_BD);
      char ch = g_symbols_active ? R0_SYMB[k] : R0_NORM[k];
      char sbuf[2] = {ch, '\0'};
      canvas.setFont(&fonts::FreeSansBold9pt7b);
      canvas.setTextDatum(textdatum_t::middle_center);
      canvas.setTextColor(COL_TEXT);
      canvas.drawString(sbuf, kx + 14, y0 + 14);
    }

    // Row 1: Top letters / Symbols (10 keys)
    int y1 = cyd_wifi_sy(108, camera_y);
    const char* r1_chars = g_symbols_active ? R1_SYMB : (g_shift_active ? R1_SHFT : R1_NORM);
    for (int k = 0; k < 10; ++k) {
      int kx = 6 + k * 31;
      canvas.fillRoundRect(kx, y1, 28, 28, 3, COL_KEY_BG);
      canvas.drawRoundRect(kx, y1, 28, 28, 3, COL_KEY_BD);
      char sbuf[2] = {r1_chars[k], '\0'};
      canvas.setFont(&fonts::FreeSansBold9pt7b);
      canvas.setTextDatum(textdatum_t::middle_center);
      canvas.setTextColor(COL_TEXT);
      canvas.drawString(sbuf, kx + 14, y1 + 14);
    }

    // Row 2: Home row (10 keys)
    int y2 = cyd_wifi_sy(140, camera_y);
    const char* r2_chars = g_symbols_active ? R2_SYMB : (g_shift_active ? R2_SHFT : R2_NORM);
    for (int k = 0; k < 10; ++k) {
      int kx = 6 + k * 31;
      canvas.fillRoundRect(kx, y2, 28, 28, 3, COL_KEY_BG);
      canvas.drawRoundRect(kx, y2, 28, 28, 3, COL_KEY_BD);
      char sbuf[2] = {r2_chars[k], '\0'};
      canvas.setFont(&fonts::FreeSansBold9pt7b);
      canvas.setTextDatum(textdatum_t::middle_center);
      canvas.setTextColor(COL_TEXT);
      canvas.drawString(sbuf, kx + 14, y2 + 14);
    }

    // Row 3: [CAPS] + Bottom letters + [DEL]
    int y3 = cyd_wifi_sy(172, camera_y);
    // [CAPS / SHIFT] (x=6..48)
    canvas.fillRoundRect(6, y3, 42, 28, 3, g_shift_active ? COL_ACCENT : COL_KEY_BG);
    canvas.drawRoundRect(6, y3, 42, 28, 3, COL_KEY_BD);
    canvas.setFont(&fonts::Font0);
    canvas.setTextDatum(textdatum_t::middle_center);
    canvas.setTextColor(g_shift_active ? COL_BG : COL_TEXT);
    canvas.drawString("CAPS", 27, y3 + 14);

    // 8 letters (x=52..264)
    const char* r3_chars = g_shift_active ? R3_SHFT : R3_NORM;
    for (int k = 0; k < 8; ++k) {
      int kx = 52 + k * 27;
      canvas.fillRoundRect(kx, y3, 24, 28, 3, COL_KEY_BG);
      canvas.drawRoundRect(kx, y3, 24, 28, 3, COL_KEY_BD);
      char sbuf[2] = {r3_chars[k], '\0'};
      canvas.setFont(&fonts::FreeSansBold9pt7b);
      canvas.setTextDatum(textdatum_t::middle_center);
      canvas.setTextColor(COL_TEXT);
      canvas.drawString(sbuf, kx + 12, y3 + 14);
    }

    // [DEL / BACKSPACE] (x=270..314)
    canvas.fillRoundRect(270, y3, 44, 28, 3, COL_KEY_BG);
    canvas.drawRoundRect(270, y3, 44, 28, 3, COL_AMBER);
    canvas.setFont(&fonts::Font0);
    canvas.setTextDatum(textdatum_t::middle_center);
    canvas.setTextColor(COL_AMBER);
    canvas.drawString("DEL", 292, y3 + 14);

    // Row 4: [?123 / ABC] + [SPACE] + [CONNECT]
    int y4 = cyd_wifi_sy(204, camera_y);
    // [?123 / ABC] toggle (x=6..58)
    canvas.fillRoundRect(6, y4, 52, 30, 3, g_symbols_active ? COL_AMBER : COL_KEY_BG);
    canvas.drawRoundRect(6, y4, 52, 30, 3, COL_KEY_BD);
    canvas.setFont(&fonts::DejaVu9);
    canvas.setTextDatum(textdatum_t::middle_center);
    canvas.setTextColor(g_symbols_active ? COL_BG : COL_TEXT);
    canvas.drawString(g_symbols_active ? "ABC" : "?123", 32, y4 + 15);

    // [SPACE BAR] (x=64..194)
    canvas.fillRoundRect(64, y4, 130, 30, 3, COL_KEY_BG);
    canvas.drawRoundRect(64, y4, 130, 30, 3, COL_KEY_BD);
    canvas.setTextColor(COL_SUB);
    canvas.drawString("SPACE", 129, y4 + 15);

    // [CONNECT] button (x=200..314)
    canvas.fillRoundRect(200, y4, 114, 30, 4, COL_ACCENT);
    canvas.drawRoundRect(200, y4, 114, 30, 4, COL_TEXT);
    canvas.setFont(&fonts::FreeSansBold9pt7b);
    canvas.setTextDatum(textdatum_t::middle_center);
    canvas.setTextColor(COL_BG);
    canvas.drawString("CONNECT", 257, y4 + 15);

  } else if (g_wifi_ui_state == WIFI_UI_CONNECTING) {
    // Modal Connecting Card
    int my = cyd_wifi_sy(60, camera_y);
    canvas.fillRoundRect(24, my, 272, 120, 6, COL_CARD_BG);
    canvas.drawRoundRect(24, my, 272, 120, 6, COL_ACCENT);

    canvas.setFont(&fonts::FreeSansBold9pt7b);
    canvas.setTextDatum(textdatum_t::middle_center);
    canvas.setTextColor(COL_ACCENT);
    canvas.drawString("CONNECTING TO WI-FI", 160, my + 25);

    canvas.setFont(&fonts::FreeSans9pt7b);
    canvas.setTextColor(COL_TEXT);
    canvas.drawString(g_target_ssid, 160, my + 52);

    // Progress bar
    canvas.drawRoundRect(44, my + 72, 232, 12, 3, COL_CARD_BD);
    int pw = (millis() / 25) % 228;
    canvas.fillRoundRect(46, my + 74, pw, 8, 2, COL_ACCENT);

    canvas.setFont(&fonts::DejaVu9);
    canvas.setTextColor(COL_SUB);
    canvas.drawString("Verifying IP & ESPN Internet Reachability...", 160, my + 100);

  } else if (g_wifi_ui_state == WIFI_UI_CONNECTED) {
    // Modal Connected Success Card
    int my = cyd_wifi_sy(60, camera_y);
    canvas.fillRoundRect(24, my, 272, 120, 6, COL_CARD_BG);
    canvas.drawRoundRect(24, my, 272, 120, 6, COL_ACCENT);

    canvas.setFont(&fonts::FreeSansBold9pt7b);
    canvas.setTextDatum(textdatum_t::middle_center);
    canvas.setTextColor(COL_ACCENT);
    canvas.drawString("WI-FI CONNECTED!", 160, my + 25);

    canvas.setFont(&fonts::FreeSans9pt7b);
    canvas.setTextColor(COL_TEXT);
    canvas.drawString(g_target_ssid, 160, my + 50);

    canvas.setFont(&fonts::DejaVu9);
    canvas.setTextColor(COL_AMBER);
    char ip_buf[48];
    snprintf(ip_buf, sizeof(ip_buf), "IP: %s (INTERNET OK)", WiFi.localIP().toString().c_str());
    canvas.drawString(ip_buf, 160, my + 74);

    canvas.setTextColor(COL_SUB);
    canvas.drawString("Launching Score Desk...", 160, my + 98);

  } else if (g_wifi_ui_state == WIFI_UI_FAILED) {
    // Modal Failed Card
    int my = cyd_wifi_sy(50, camera_y);
    canvas.fillRoundRect(24, my, 272, 140, 6, COL_CARD_BG);
    canvas.drawRoundRect(24, my, 272, 140, 6, COL_RED);

    canvas.setFont(&fonts::FreeSansBold9pt7b);
    canvas.setTextDatum(textdatum_t::middle_center);
    canvas.setTextColor(COL_RED);
    canvas.drawString("CONNECTION FAILED", 160, my + 22);

    canvas.setFont(&fonts::DejaVu9);
    canvas.setTextColor(COL_TEXT);
    canvas.drawString(g_wifi_status_msg[0] ? g_wifi_status_msg : "Could not obtain IP or incorrect password.", 160, my + 48);

    // [RETRY / EDIT PASSWORD] button (x=36..150)
    canvas.fillRoundRect(36, my + 80, 116, 36, 4, COL_KEY_BG);
    canvas.drawRoundRect(36, my + 80, 116, 36, 4, COL_AMBER);
    canvas.setTextDatum(textdatum_t::middle_center);
    canvas.setTextColor(COL_AMBER);
    canvas.drawString("EDIT PASS", 94, my + 98);

    // [BACK TO LIST] button (x=168..282)
    canvas.fillRoundRect(168, my + 80, 114, 36, 4, COL_KEY_BG);
    canvas.drawRoundRect(168, my + 80, 114, 36, 4, COL_SUB);
    canvas.setTextColor(COL_TEXT);
    canvas.drawString("NETWORKS", 225, my + 98);
  }
}

// -----------------------------------------------------------------------------
// Push Full Two-Band Frame
// -----------------------------------------------------------------------------
inline void cyd_push_wifi_frame(LGFX_Sprite& canvas) {
  for (int band = 0; band < 2; ++band) {
    int camera_y = band * 120;
    cyd_draw_wifi_ui(canvas, camera_y);
    canvas.pushSprite(0, camera_y);
  }
}

// -----------------------------------------------------------------------------
// Auto-Open Wi-Fi Probe with Internet Verification
// -----------------------------------------------------------------------------
inline bool cyd_wifi_try_open_networks(LGFX_Sprite& canvas) {
  g_wifi_ui_state = WIFI_UI_SCANNING;
  cyd_push_wifi_frame(canvas);

  cyd_wifi_scan_networks();

  // Find open networks
  int open_indices[10];
  int open_count = 0;
  for (int i = 0; i < g_scanned_count && open_count < 10; ++i) {
    if (g_scanned_nets[i].is_open) {
      open_indices[open_count++] = i;
    }
  }

  Serial.printf("[WIFI_AUTO] Found %d open networks.\n", open_count);
  if (open_count == 0) {
    snprintf(g_wifi_status_msg, sizeof(g_wifi_status_msg), "No open networks found.");
    g_wifi_ui_state = WIFI_UI_PICKER;
    cyd_push_wifi_frame(canvas);
    return false;
  }

  // Iterate open networks, highest RSSI first
  for (int o = 0; o < open_count; ++o) {
    const ScannedNetwork& net = g_scanned_nets[open_indices[o]];
    strncpy(g_target_ssid, net.ssid, sizeof(g_target_ssid) - 1);
    g_target_ssid[sizeof(g_target_ssid) - 1] = '\0';
    g_wifi_ui_state = WIFI_UI_CONNECTING;
    cyd_push_wifi_frame(canvas);

    Serial.printf("[WIFI_AUTO] Trying open network: '%s' (RSSI: %d)...\n", net.ssid, net.rssi);
    WiFi.disconnect(true);
    delay(100);
    WiFi.begin(net.ssid);

    uint32_t start_ms = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start_ms < 6000)) {
      cyd_push_wifi_frame(canvas);
      delay(150);
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("[WIFI_AUTO] Associated with '%s'! Testing internet...\n", net.ssid);
      cyd_push_wifi_frame(canvas);
      if (cyd_wifi_test_internet(4000)) {
        Serial.printf("[WIFI_AUTO] SUCCESS: Open network '%s' has verified internet!\n", net.ssid);
        cyd_wifi_save_credentials(net.ssid, "");
        g_wifi_ui_state = WIFI_UI_CONNECTED;
        cyd_push_wifi_frame(canvas);
        delay(1200);
        g_wifi_ui_state = WIFI_UI_INACTIVE;
        return true;
      } else {
        Serial.printf("[WIFI_AUTO] Disconnected: '%s' failed internet connectivity check (captive portal or no WAN).\n", net.ssid);
        WiFi.disconnect(true);
      }
    } else {
      Serial.printf("[WIFI_AUTO] Timed out associating with open network '%s'.\n", net.ssid);
    }
  }

  snprintf(g_wifi_status_msg, sizeof(g_wifi_status_msg), "Open networks had no internet.");
  g_wifi_ui_state = WIFI_UI_PICKER;
  cyd_push_wifi_frame(canvas);
  return false;
}

// -----------------------------------------------------------------------------
// Connect to Selected Protected Network
// -----------------------------------------------------------------------------
inline bool cyd_wifi_connect_to_target(LGFX_Sprite& canvas) {
  g_wifi_ui_state = WIFI_UI_CONNECTING;
  cyd_push_wifi_frame(canvas);

  Serial.printf("[WIFI_CONNECT] Connecting to '%s' with %d-char pass...\n", g_target_ssid, g_entered_pass_len);
  WiFi.disconnect(true);
  delay(100);
  WiFi.begin(g_target_ssid, g_entered_pass_len > 0 ? g_entered_pass : nullptr);

  uint32_t start_ms = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start_ms < 8500)) {
    cyd_push_wifi_frame(canvas);
    delay(150);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[WIFI_CONNECT] Associated! IP: %s. Verifying internet...\n", WiFi.localIP().toString().c_str());
    cyd_push_wifi_frame(canvas);
    if (cyd_wifi_test_internet(4000)) {
      Serial.printf("[WIFI_CONNECT] SUCCESS: Connected to '%s' with verified internet!\n", g_target_ssid);
      cyd_wifi_save_credentials(g_target_ssid, g_entered_pass);
      g_wifi_ui_state = WIFI_UI_CONNECTED;
      cyd_push_wifi_frame(canvas);
      delay(1200);
      g_wifi_ui_state = WIFI_UI_INACTIVE;
      return true;
    } else {
      snprintf(g_wifi_status_msg, sizeof(g_wifi_status_msg), "Connected to AP, but no internet.");
      g_wifi_ui_state = WIFI_UI_FAILED;
      cyd_push_wifi_frame(canvas);
      return false;
    }
  } else {
    Serial.println("[WIFI_CONNECT] Association failed or timed out.");
    snprintf(g_wifi_status_msg, sizeof(g_wifi_status_msg), "Failed to associate. Check password.");
    g_wifi_ui_state = WIFI_UI_FAILED;
    cyd_push_wifi_frame(canvas);
    return false;
  }
}

// -----------------------------------------------------------------------------
// Touch Hit Handler for Wi-Fi UI
// -----------------------------------------------------------------------------
inline bool cyd_wifi_handle_touch(int x, int y, LGFX_Sprite& canvas, bool& out_scores_ready) {
  out_scores_ready = false;
  if (g_wifi_ui_state == WIFI_UI_INACTIVE) return false;

  // 1. PICKER SCREEN TOUCHES
  if (g_wifi_ui_state == WIFI_UI_PICKER) {
    // Header Buttons (y <= 24)
    if (y <= 24) {
      // [AUTO OPEN] button (x=120..194)
      if (x >= 120 && x <= 194) {
        Serial.println("[TOUCH_WIFI] AUTO OPEN pressed.");
        if (cyd_wifi_try_open_networks(canvas)) {
          out_scores_ready = true;
        }
        return true;
      }
      // [RESCAN] button (x=200..256)
      if (x >= 200 && x <= 256) {
        Serial.println("[TOUCH_WIFI] RESCAN pressed.");
        g_wifi_ui_state = WIFI_UI_SCANNING;
        cyd_push_wifi_frame(canvas);
        cyd_wifi_scan_networks();
        g_wifi_ui_state = WIFI_UI_PICKER;
        cyd_push_wifi_frame(canvas);
        return true;
      }
      // [DESK] button (x >= 265)
      if (x >= 265) {
        Serial.println("[TOUCH_WIFI] Return to desk pressed.");
        g_wifi_ui_state = WIFI_UI_INACTIVE;
        return true;
      }
    }

    // Network row items (y=26..202)
    if (y >= 26 && y <= 202) {
      int row = (y - 26) / 44;
      int idx = g_picker_page * NETS_PER_PAGE + row;
      if (row >= 0 && row < NETS_PER_PAGE && idx >= 0 && idx < g_scanned_count) {
        const ScannedNetwork& net = g_scanned_nets[idx];
        strncpy(g_target_ssid, net.ssid, sizeof(g_target_ssid) - 1);
        g_target_ssid[sizeof(g_target_ssid) - 1] = '\0';
        g_target_is_open = net.is_open;

        Serial.printf("[TOUCH_WIFI] Selected network %d: '%s' (Open=%d)\n", idx, net.ssid, net.is_open ? 1 : 0);

        if (net.is_open) {
          // Open network: connect directly
          g_entered_pass[0] = '\0';
          g_entered_pass_len = 0;
          if (cyd_wifi_connect_to_target(canvas)) {
            out_scores_ready = true;
          }
        } else {
          // Protected network: open on-screen touch keyboard!
          g_entered_pass[0] = '\0';
          g_entered_pass_len = 0;
          g_wifi_ui_state = WIFI_UI_KEYBOARD;
          cyd_push_wifi_frame(canvas);
        }
        return true;
      }
    }

    // Pager bar (y >= 208)
    if (y >= 208) {
      int total_pages = (g_scanned_count + NETS_PER_PAGE - 1) / NETS_PER_PAGE;
      if (x <= 75 && g_picker_page > 0) {
        g_picker_page--;
        cyd_push_wifi_frame(canvas);
        return true;
      }
      if (x >= 245 && g_picker_page < total_pages - 1) {
        g_picker_page++;
        cyd_push_wifi_frame(canvas);
        return true;
      }
    }
    return true;
  }

  // 2. KEYBOARD SCREEN TOUCHES
  if (g_wifi_ui_state == WIFI_UI_KEYBOARD) {
    // Header [CANCEL] (y <= 24, x >= 250)
    if (y <= 24 && x >= 250) {
      Serial.println("[TOUCH_KEYBOARD] Cancel pressed. Returning to picker.");
      g_wifi_ui_state = WIFI_UI_PICKER;
      cyd_push_wifi_frame(canvas);
      return true;
    }

    // Password bar actions: [SHOW/HIDE] (x=236..276, y=26..56)
    if (y >= 26 && y <= 56) {
      if (x >= 236 && x <= 276) {
        g_pass_masked = !g_pass_masked;
        cyd_push_wifi_frame(canvas);
        return true;
      }
      // [CLEAR] (x=278..314)
      if (x >= 278) {
        g_entered_pass[0] = '\0';
        g_entered_pass_len = 0;
        cyd_push_wifi_frame(canvas);
        return true;
      }
    }

    // Keyboard Key Hit Testing
    static const char* R0_NORM = "1234567890";
    static const char* R0_SYMB = "!@#$%^&*()";
    static const char* R1_NORM = "qwertyuiop";
    static const char* R1_SHFT = "QWERTYUIOP";
    static const char* R1_SYMB = "~`_-+={[]}\\";
    static const char* R2_NORM = "asdfghjkl-";
    static const char* R2_SHFT = "ASDFGHJKL_";
    static const char* R2_SYMB = ":;\"'<>,./?";
    static const char* R3_NORM = "zxcvbnm.";
    static const char* R3_SHFT = "ZXCVBNM,";

    // Row 0: y=76..104
    if (y >= 74 && y <= 104) {
      for (int k = 0; k < 10; ++k) {
        int kx = 6 + k * 31;
        if (x >= kx && x < kx + 30) {
          char ch = g_symbols_active ? R0_SYMB[k] : R0_NORM[k];
          if (g_entered_pass_len < (int)sizeof(g_entered_pass) - 1) {
            g_entered_pass[g_entered_pass_len++] = ch;
            g_entered_pass[g_entered_pass_len] = '\0';
            cyd_push_wifi_frame(canvas);
          }
          return true;
        }
      }
    }

    // Row 1: y=106..136
    if (y >= 106 && y <= 136) {
      const char* r1_chars = g_symbols_active ? R1_SYMB : (g_shift_active ? R1_SHFT : R1_NORM);
      for (int k = 0; k < 10; ++k) {
        int kx = 6 + k * 31;
        if (x >= kx && x < kx + 30) {
          char ch = r1_chars[k];
          if (g_entered_pass_len < (int)sizeof(g_entered_pass) - 1) {
            g_entered_pass[g_entered_pass_len++] = ch;
            g_entered_pass[g_entered_pass_len] = '\0';
            cyd_push_wifi_frame(canvas);
          }
          return true;
        }
      }
    }

    // Row 2: y=138..168
    if (y >= 138 && y <= 168) {
      const char* r2_chars = g_symbols_active ? R2_SYMB : (g_shift_active ? R2_SHFT : R2_NORM);
      for (int k = 0; k < 10; ++k) {
        int kx = 6 + k * 31;
        if (x >= kx && x < kx + 30) {
          char ch = r2_chars[k];
          if (g_entered_pass_len < (int)sizeof(g_entered_pass) - 1) {
            g_entered_pass[g_entered_pass_len++] = ch;
            g_entered_pass[g_entered_pass_len] = '\0';
            cyd_push_wifi_frame(canvas);
          }
          return true;
        }
      }
    }

    // Row 3: y=170..200 -> [CAPS] (x=6..48) | Letters (x=52..268) | [DEL] (x=270..314)
    if (y >= 170 && y <= 200) {
      if (x <= 48) {
        g_shift_active = !g_shift_active;
        cyd_push_wifi_frame(canvas);
        return true;
      }
      if (x >= 270) {
        if (g_entered_pass_len > 0) {
          g_entered_pass_len--;
          g_entered_pass[g_entered_pass_len] = '\0';
          cyd_push_wifi_frame(canvas);
        }
        return true;
      }
      // Letters
      const char* r3_chars = g_shift_active ? R3_SHFT : R3_NORM;
      for (int k = 0; k < 8; ++k) {
        int kx = 52 + k * 27;
        if (x >= kx && x < kx + 26) {
          char ch = r3_chars[k];
          if (g_entered_pass_len < (int)sizeof(g_entered_pass) - 1) {
            g_entered_pass[g_entered_pass_len++] = ch;
            g_entered_pass[g_entered_pass_len] = '\0';
            cyd_push_wifi_frame(canvas);
          }
          return true;
        }
      }
    }

    // Row 4: y=202..238 -> [?123] (x=6..58) | [SPACE] (x=64..194) | [CONNECT] (x=200..314)
    if (y >= 202) {
      if (x <= 58) {
        g_symbols_active = !g_symbols_active;
        cyd_push_wifi_frame(canvas);
        return true;
      }
      if (x >= 64 && x <= 194) {
        if (g_entered_pass_len < (int)sizeof(g_entered_pass) - 1) {
          g_entered_pass[g_entered_pass_len++] = ' ';
          g_entered_pass[g_entered_pass_len] = '\0';
          cyd_push_wifi_frame(canvas);
        }
        return true;
      }
      if (x >= 200) {
        Serial.printf("[TOUCH_KEYBOARD] CONNECT pressed for SSID '%s' (pass len=%d)\n", g_target_ssid, g_entered_pass_len);
        if (cyd_wifi_connect_to_target(canvas)) {
          out_scores_ready = true;
        }
        return true;
      }
    }
    return true;
  }

  // 3. FAILED SCREEN TOUCHES
  if (g_wifi_ui_state == WIFI_UI_FAILED) {
    if (y >= 120 && y <= 180) {
      // [EDIT PASS] (x=36..152)
      if (x <= 152) {
        g_wifi_ui_state = WIFI_UI_KEYBOARD;
        cyd_push_wifi_frame(canvas);
        return true;
      }
      // [NETWORKS] (x >= 168)
      if (x >= 168) {
        g_wifi_ui_state = WIFI_UI_PICKER;
        cyd_push_wifi_frame(canvas);
        return true;
      }
    }
    return true;
  }

  return true;
}

// -----------------------------------------------------------------------------
// Open Wi-Fi Manager UI (Triggered on demand or auto)
// -----------------------------------------------------------------------------
inline void cyd_wifi_open_manager(LGFX_Sprite& canvas) {
  g_wifi_ui_state = WIFI_UI_SCANNING;
  cyd_push_wifi_frame(canvas);
  cyd_wifi_scan_networks();
  g_wifi_ui_state = WIFI_UI_PICKER;
  cyd_push_wifi_frame(canvas);
}
