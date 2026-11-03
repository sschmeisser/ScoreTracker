#pragma once

#include <Arduino.h>
#include <LovyanGFX.hpp>
#include "cyd_ui.h"
#include "boot_logo.h"

// =============================================================================
// CYD SCORE TRACKER — HIGH-TECH STARTUP LOADING SCREEN
// =============================================================================
// Renders the ScoreTracker 80x80 emblem, branding, supported leagues,
// and real-time boot status / animated progress bar into the two-band sprite.
// =============================================================================

inline void cyd_draw_boot_screen(LGFX_Sprite& canvas, int camera_y, float progress, const char* status_text) {
  g_cyd_detail_open = false;
  canvas.fillScreen(CYD_BG);

  // Stadium concentric canopy arches
  for (int i = 0; i < 4; ++i) {
    int arc_r = 160 + i * 22;
    canvas.drawCircle(CYD_W / 2, cyd_sy(45, camera_y), arc_r, CYD_HAIR);
  }

  // Side athletic hash marks
  for (int y = 40; y < 200; y += 20) {
    canvas.drawFastHLine(10, cyd_sy(y, camera_y), 12, CYD_CARD);
    canvas.drawFastHLine(CYD_W - 22, cyd_sy(y, camera_y), 12, CYD_CARD);
  }

  // Push 80x80 custom ScoreTracker emblem with byte swapping & chroma-key transparency
  int ex = (CYD_W - EMBLEM_SIZE) / 2;
  int ey = 12;
  canvas.setSwapBytes(true);
  canvas.pushImage(ex, cyd_sy(ey, camera_y), EMBLEM_SIZE, EMBLEM_SIZE, SCORETRACKER_EMBLEM_80x80, (uint16_t)0xF81F);
  canvas.setSwapBytes(false);

  // Title: SCORE TRACKER
  canvas.setFont(&fonts::FreeSansBold12pt7b);
  canvas.setTextDatum(textdatum_t::top_left);
  int total_w = canvas.textWidth("SCORE TRACKER");
  int tx = (CYD_W - total_w) / 2;
  int ty = 98;
  canvas.setTextColor(CYD_TEXT);
  canvas.drawString("SCORE ", tx, cyd_sy(ty, camera_y));
  int w_s = canvas.textWidth("SCORE ");
  canvas.setTextColor(CYD_GOLD);
  canvas.drawString("TRACKER", tx + w_s, cyd_sy(ty, camera_y));

  // Subtitle
  canvas.setFont(&fonts::DejaVu9);
  canvas.setTextColor(CYD_SUB);
  const char* sub = "LIVE MULTI-SPORT DESK";
  int w_sub = canvas.textWidth(sub);
  int sy = 120;
  canvas.drawString(sub, (CYD_W - w_sub) / 2, cyd_sy(sy, camera_y));
  canvas.drawFastHLine((CYD_W - w_sub) / 2 - 24, cyd_sy(sy + 5, camera_y), 18, CYD_HAIR);
  canvas.drawFastHLine((CYD_W + w_sub) / 2 + 6, cyd_sy(sy + 5, camera_y), 18, CYD_HAIR);

  // Supported Leagues Pill (Dual-tier compact badge)
  const char* lg_row1 = "NFL   NBA   NHL   CBB   CFB";
  const char* lg_row2 = "EPL   UCL   UEL   LAL   BUN";
  canvas.setFont(&fonts::Font0);
  int pw = 196;
  int ph = 26;
  int px = (CYD_W - pw) / 2;
  int py = 138;
  canvas.fillRoundRect(px, cyd_sy(py, camera_y), pw, ph, 6, CYD_CARD);
  canvas.drawRoundRect(px, cyd_sy(py, camera_y), pw, ph, 6, CYD_HAIR);

  canvas.setTextColor(CYD_GOLD);
  int w1 = canvas.textWidth(lg_row1);
  int w2 = canvas.textWidth(lg_row2);
  canvas.drawString(lg_row1, (CYD_W - w1) / 2, cyd_sy(py + 4, camera_y));
  canvas.drawString(lg_row2, (CYD_W - w2) / 2, cyd_sy(py + 15, camera_y));

  // Status message with glowing live beacon
  canvas.setFont(&fonts::DejaVu9);
  canvas.setTextColor(CYD_TEXT);
  int ws = canvas.textWidth(status_text);
  int sx = (CYD_W - ws) / 2 + 6;
  int stat_y = 172;
  canvas.fillCircle(sx - 11, cyd_sy(stat_y + 5, camera_y), 4, 0x1AE4);
  canvas.fillCircle(sx - 11, cyd_sy(stat_y + 5, camera_y), 2, 0x2EA7);
  canvas.drawString(status_text, sx, cyd_sy(stat_y, camera_y));

  // Progress bar
  int bar_w = 210;
  int bar_h = 6;
  int bx = (CYD_W - bar_w) / 2;
  int by = 192;
  canvas.fillRoundRect(bx, cyd_sy(by, camera_y), bar_w, bar_h, 3, CYD_CARD);
  canvas.drawRoundRect(bx, cyd_sy(by, camera_y), bar_w, bar_h, 3, CYD_HAIR);

  int fill_w = (int)(bar_w * max(0.0f, min(1.0f, progress)));
  if (fill_w > 0) {
    canvas.fillRoundRect(bx, cyd_sy(by, camera_y), fill_w, bar_h, 3, 0x2EA7);
    if (fill_w >= 4) {
      canvas.fillCircle(bx + fill_w - 2, cyd_sy(by + bar_h / 2, camera_y), 3, 0xFFFF);
    }
  }

  // Hardware footer
  const char* foot = "ESP32-2432S028R  •  591 EMBEDDED LOGOS  •  V2.4";
  canvas.setFont(&fonts::Font0);
  int wf = canvas.textWidth(foot);
  canvas.setTextColor(CYD_SAGE);
  canvas.drawString(foot, (CYD_W - wf) / 2, cyd_sy(222, camera_y));
}
