#pragma once

#include <Arduino.h>
#include <LovyanGFX.hpp>
#include "config.h"

// =============================================================================
// BIG PILOT GMT WATCH FACE THEME DEFINITIONS
// =============================================================================

enum WatchTheme {
  THEME_COKE = 0,     // Classic Pilot GMT: Matte Black Night / Crimson Red Day
  THEME_PEPSI,        // Iconic Pepsi GMT: Deep Navy Night / Crimson Red Day
  THEME_BATMAN,       // Batman GMT: Pure Black Night / Royal Blue Day
  THEME_STEALTH,      // Pilot Stealth: Pitch Black Night / Slate Gray Day
  THEME_NIGHT_LUME,   // Super-LumiNova Night Lume: Vivid Glowing Green
  THEME_COUNT
};

struct DialPalette {
  uint16_t col_bg;
  uint16_t col_bezel_border;
  uint16_t col_gmt_bezel_night;
  uint16_t col_gmt_bezel_day;
  uint16_t col_gmt_bezel_text;
  uint16_t col_major_tick;
  uint16_t col_minor_tick;
  uint16_t col_triangle;
  uint16_t col_numeral;
  uint16_t col_text_pilot;
  uint16_t col_text_gmt;
  uint16_t col_text_pst;
  uint16_t col_hand_border;
  uint16_t col_hand_fill;
  uint16_t col_gmt_hand;
  uint16_t col_gmt_fill;
  uint16_t col_sec_hand;
  uint16_t col_sec_pip;
  uint16_t col_cap_outer;
  uint16_t col_cap_inner;
  uint8_t  brightness;
};

// 16-bit RGB565 Palettes
static const DialPalette PALETTES[THEME_COUNT] = {
  // THEME_COKE: Classic Pilot GMT (Matte Black Night / Crimson Red Day, Aviation Red Hand)
  {
    .col_bg               = 0x0841, // #08090a matte black
    .col_bezel_border      = 0x2965, // #282c32 steel rim
    .col_gmt_bezel_night  = 0x0862, // #0e0f11 dark night
    .col_gmt_bezel_day    = 0x88A3, // #8e161c crimson red
    .col_gmt_bezel_text   = 0xFFFF, // pure white
    .col_major_tick       = 0xFFFF, // pure white
    .col_minor_tick       = 0xBDF7, // #b8bec4
    .col_triangle         = 0xFFFF,
    .col_numeral          = 0xFFFF,
    .col_text_pilot       = 0x8410, // titanium gray
    .col_text_gmt         = 0xE944, // aviation red
    .col_text_pst         = 0xAD55, // #a8adb3
    .col_hand_border      = 0x0000, // stark black contour
    .col_hand_fill        = 0xFFDF, // off-white lume fill
    .col_gmt_hand         = 0xE944, // bright aviation red
    .col_gmt_fill         = 0xFFFF, // white lume insert
    .col_sec_hand         = 0xFFFF,
    .col_sec_pip          = 0xFFFF,
    .col_cap_outer        = 0x18C3,
    .col_cap_inner        = 0xD6BA,
    .brightness           = BRIGHTNESS_HIGH
  },
  // THEME_PEPSI: Iconic Pepsi GMT (Deep Navy Night / Crimson Red Day, Aviation Red Hand)
  {
    .col_bg               = 0x0841,
    .col_bezel_border      = 0x2965,
    .col_gmt_bezel_night  = 0x08E8, // #0e1c40 deep navy blue
    .col_gmt_bezel_day    = 0x88A3, // #8e161c crimson red
    .col_gmt_bezel_text   = 0xFFFF,
    .col_major_tick       = 0xFFFF,
    .col_minor_tick       = 0xBDF7,
    .col_triangle         = 0xFFFF,
    .col_numeral          = 0xFFFF,
    .col_text_pilot       = 0x8410,
    .col_text_gmt         = 0xE944,
    .col_text_pst         = 0xAD55,
    .col_hand_border      = 0x0000,
    .col_hand_fill        = 0xFFDF,
    .col_gmt_hand         = 0xE944,
    .col_gmt_fill         = 0xFFFF,
    .col_sec_hand         = 0xFFFF,
    .col_sec_pip          = 0xFFFF,
    .col_cap_outer        = 0x18C3,
    .col_cap_inner        = 0xD6BA,
    .brightness           = BRIGHTNESS_HIGH
  },
  // THEME_BATMAN: Batman GMT (Pure Black Night / Royal Blue Day, Electric Blue Hand)
  {
    .col_bg               = 0x0841,
    .col_bezel_border      = 0x2965,
    .col_gmt_bezel_night  = 0x0862, // pure black
    .col_gmt_bezel_day    = 0x09AF, // #0f377d royal blue
    .col_gmt_bezel_text   = 0xFFFF,
    .col_major_tick       = 0xFFFF,
    .col_minor_tick       = 0xBDF7,
    .col_triangle         = 0xFFFF,
    .col_numeral          = 0xFFFF,
    .col_text_pilot       = 0x8410,
    .col_text_gmt         = 0x2C7F, // electric blue
    .col_text_pst         = 0xAD55,
    .col_hand_border      = 0x0000,
    .col_hand_fill        = 0xFFDF,
    .col_gmt_hand         = 0x2C7F,
    .col_gmt_fill         = 0xFFFF,
    .col_sec_hand         = 0xFFFF,
    .col_sec_pip          = 0xFFFF,
    .col_cap_outer        = 0x18C3,
    .col_cap_inner        = 0xD6BA,
    .brightness           = BRIGHTNESS_HIGH
  },
  // THEME_STEALTH: Pilot Monochrome (Pitch Black Night / Slate Gray Day, Aviation Orange Hand)
  {
    .col_bg               = 0x0841,
    .col_bezel_border      = 0x2965,
    .col_gmt_bezel_night  = 0x0862, // pitch black
    .col_gmt_bezel_day    = 0x2987, // #2a3038 slate gray
    .col_gmt_bezel_text   = 0xFFFF,
    .col_major_tick       = 0xFFFF,
    .col_minor_tick       = 0xBDF7,
    .col_triangle         = 0xFFFF,
    .col_numeral          = 0xFFFF,
    .col_text_pilot       = 0x8410,
    .col_text_gmt         = 0xFD40, // aviation orange
    .col_text_pst         = 0xAD55,
    .col_hand_border      = 0x0000,
    .col_hand_fill        = 0xFFDF,
    .col_gmt_hand         = 0xFD40,
    .col_gmt_fill         = 0xFFFF,
    .col_sec_hand         = 0xFFFF,
    .col_sec_pip          = 0xFFFF,
    .col_cap_outer        = 0x18C3,
    .col_cap_inner        = 0xD6BA,
    .brightness           = BRIGHTNESS_MED
  },
  // THEME_NIGHT_LUME: Super-LumiNova Night Lume (Glowing Green Dial & GMT Hand)
  {
    .col_bg               = 0x0020, // pitch night black
    .col_bezel_border      = 0x08E1,
    .col_gmt_bezel_night  = 0x0180, // deep dark lume green
    .col_gmt_bezel_day    = 0x1BA4, // muted luminous green
    .col_gmt_bezel_text   = 0x57F3, // vivid lume
    .col_major_tick       = 0x47E8, // luminous green
    .col_minor_tick       = 0x1BA4,
    .col_triangle         = 0x57F3,
    .col_numeral          = 0x47E8,
    .col_text_pilot       = 0x1382,
    .col_text_gmt         = 0x57F3,
    .col_text_pst         = 0x2426,
    .col_hand_border      = 0x0060,
    .col_hand_fill        = 0x57F3,
    .col_gmt_hand         = 0x57F3,
    .col_gmt_fill         = 0x57F3,
    .col_sec_hand         = 0x57F3,
    .col_sec_pip          = 0x57F3,
    .col_cap_outer        = 0x08C1,
    .col_cap_inner        = 0x3448,
    .brightness           = BRIGHTNESS_HIGH
  }
};

// =============================================================================
// GEOMETRY & RENDERING
// =============================================================================

static const int DIAL_CX = 120;
static const int DIAL_CY = 120;

struct HandPoint {
  float x;
  float y;
};

// Helper to draw a Flieger lozenge/sword hand decomposed into triangles
inline void draw_flieger_polygon(LGFX_Sprite& canvas,
                                 float angle_deg,
                                 float tip_len,
                                 float shoulder_dist,
                                 float shoulder_w,
                                 float waist_dist,
                                 float waist_w,
                                 float tail_len,
                                 uint16_t color) {
  float rad = (angle_deg - 90.0f) * (PI / 180.0f);
  float cos_a = cosf(rad);
  float sin_a = sinf(rad);
  float nx = -sin_a;
  float ny =  cos_a;

  HandPoint pt_tip  = { DIAL_CX + tip_len * cos_a, DIAL_CY + tip_len * sin_a };
  HandPoint pt_sh_r = { DIAL_CX + shoulder_dist * cos_a + shoulder_w * nx,
                        DIAL_CY + shoulder_dist * sin_a + shoulder_w * ny };
  HandPoint pt_sh_l = { DIAL_CX + shoulder_dist * cos_a - shoulder_w * nx,
                        DIAL_CY + shoulder_dist * sin_a - shoulder_w * ny };
  HandPoint pt_w_r  = { DIAL_CX + waist_dist * cos_a + waist_w * nx,
                        DIAL_CY + waist_dist * sin_a + waist_w * ny };
  HandPoint pt_w_l  = { DIAL_CX + waist_dist * cos_a - waist_w * nx,
                        DIAL_CY + waist_dist * sin_a - waist_w * ny };
  HandPoint pt_tail = { DIAL_CX - tail_len * cos_a, DIAL_CY - tail_len * sin_a };

  // 4 Triangles forming the 6-sided lozenge hand
  canvas.fillTriangle(pt_tip.x,  pt_tip.y,  pt_sh_r.x, pt_sh_r.y, pt_sh_l.x, pt_sh_l.y, color);
  canvas.fillTriangle(pt_sh_r.x, pt_sh_r.y, pt_w_r.x,  pt_w_r.y,  pt_sh_l.x, pt_sh_l.y, color);
  canvas.fillTriangle(pt_w_r.x,  pt_w_r.y,  pt_w_l.x,  pt_w_l.y,  pt_sh_l.x, pt_sh_l.y, color);
  canvas.fillTriangle(pt_w_r.x,  pt_w_r.y,  pt_tail.x, pt_tail.y, pt_w_l.x,  pt_w_l.y,  color);
}

// Draw complete dial background (24-hour GMT bezel, 60-min track, 12 o'clock triangle, numerals, branding)
inline void draw_dial_face(LGFX_Sprite& canvas, const DialPalette& pal, bool ntp_synced) {
  // 1. Bi-color 24-Hour GMT Outer Bezel (r=103 to 119)
  // Day half: 06:00 to 18:00 (3 o'clock clockwise to 9 o'clock = [0, 180 deg])
  // Night half: 18:00 to 06:00 (9 o'clock clockwise to 3 o'clock = [180, 360 deg])
  canvas.fillArc(DIAL_CX, DIAL_CY, 103, 119, 0.0f, 180.0f, pal.col_gmt_bezel_day);
  canvas.fillArc(DIAL_CX, DIAL_CY, 103, 119, 180.0f, 360.0f, pal.col_gmt_bezel_night);

  // Bezel steel rims & inner dial mask
  canvas.drawCircle(DIAL_CX, DIAL_CY, 119, pal.col_bezel_border);
  canvas.drawCircle(DIAL_CX, DIAL_CY, 103, pal.col_bezel_border);
  canvas.fillCircle(DIAL_CX, DIAL_CY, 102, pal.col_bg);

  // 2. Bezel 24-Hour Scale (2, 4, 6... 22, inverted triangle at 24/0, odd hour dots)
  canvas.setFont(&fonts::DejaVu9);
  canvas.setTextDatum(textdatum_t::middle_center);
  canvas.setTextColor(pal.col_gmt_bezel_text);

  // 24 / 0 Inverted Triangle at 12 o'clock (base outer, apex inner pointing to dial)
  canvas.fillTriangle(DIAL_CX, DIAL_CY - 105,
                      DIAL_CX - 4.5f, DIAL_CY - 117,
                      DIAL_CX + 4.5f, DIAL_CY - 117,
                      pal.col_gmt_bezel_text);

  const float r_bezel_mid = 111.0f;
  for (int h24 = 1; h24 < 24; ++h24) {
    float deg = h24 * 15.0f - 90.0f;
    float rad = deg * (PI / 180.0f);
    float cos_a = cosf(rad);
    float sin_a = sinf(rad);

    if (h24 % 2 == 0) {
      char buf[4];
      itoa(h24, buf, 10);
      canvas.drawString(buf, (int)(DIAL_CX + r_bezel_mid * cos_a + 0.5f),
                             (int)(DIAL_CY + r_bezel_mid * sin_a + 0.5f));
    } else {
      canvas.fillSmoothCircle(DIAL_CX + r_bezel_mid * cos_a,
                              DIAL_CY + r_bezel_mid * sin_a,
                              1.2f, pal.col_gmt_bezel_text);
    }
  }

  // 3. Big Pilot Minute/Second Chapter Ring inside the Bezel (r=93 to 101)
  for (int i = 0; i < 60; ++i) {
    if (i == 0) continue; // 12 o'clock has the Flieger triangle

    float rad = (i * 6.0f - 90.0f) * (PI / 180.0f);
    float cos_a = cosf(rad);
    float sin_a = sinf(rad);

    if (i % 5 == 0) {
      // 5-minute major tick (bold baton)
      canvas.drawWideLine(DIAL_CX + 93.0f * cos_a, DIAL_CY + 93.0f * sin_a,
                          DIAL_CX + 101.0f * cos_a, DIAL_CY + 101.0f * sin_a,
                          1.4f, pal.col_major_tick);
    } else {
      // 1-minute minor tick
      canvas.drawWideLine(DIAL_CX + 96.0f * cos_a, DIAL_CY + 96.0f * sin_a,
                          DIAL_CX + 101.0f * cos_a, DIAL_CY + 101.0f * sin_a,
                          0.6f, pal.col_minor_tick);
    }
  }

  // 4. 12 o'clock Flieger Triangle & Two Dots
  float tri_apex_y = DIAL_CY - 100.0f;
  float tri_base_y = DIAL_CY - 88.0f;
  float tri_w = 7.0f;
  canvas.fillTriangle(DIAL_CX, tri_apex_y,
                      DIAL_CX - tri_w, tri_base_y,
                      DIAL_CX + tri_w, tri_base_y,
                      pal.col_triangle);

  float dot_y = DIAL_CY - 94.0f;
  canvas.fillSmoothCircle(DIAL_CX - 11.0f, dot_y, 2.2f, pal.col_triangle);
  canvas.fillSmoothCircle(DIAL_CX + 11.0f, dot_y, 2.2f, pal.col_triangle);

  // 5. Arabic Numerals 1 through 11 (No 12, no date)
  canvas.setFont(&fonts::FreeSansBold9pt7b);
  canvas.setTextDatum(textdatum_t::middle_center);
  canvas.setTextColor(pal.col_numeral);

  const float num_r = 74.0f;
  for (int h = 1; h <= 11; ++h) {
    float rad = (h * 30.0f - 90.0f) * (PI / 180.0f);
    int nx = (int)(DIAL_CX + num_r * cosf(rad) + 0.5f);
    int ny = (int)(DIAL_CY + num_r * sinf(rad) + 0.5f);
    char buf[4];
    itoa(h, buf, 10);
    canvas.drawString(buf, nx, ny);
  }

  // 6. Inscriptions: "PILOT GMT" below 12, "PST" above 6
  canvas.setFont(&fonts::DejaVu9);
  canvas.setTextDatum(textdatum_t::middle_center);

  canvas.setTextColor(pal.col_text_pilot);
  canvas.drawString("PILOT", DIAL_CX - 15, DIAL_CY - 40);
  canvas.setTextColor(pal.col_text_gmt);
  canvas.drawString("GMT", DIAL_CX + 16, DIAL_CY - 40);

  canvas.setTextColor(pal.col_text_pst);
  canvas.drawString("PST", DIAL_CX, DIAL_CY + 34);

  // Tiny discreet NTP status indicator dot (below "PST" text)
  // Green when atomic synced, subtle orange/dim when syncing or offline
  uint16_t sync_dot_col = ntp_synced ? 0x0540 : 0x8A00;
  canvas.fillCircle(DIAL_CX, DIAL_CY + 44, 1, sync_dot_col);
}

// Draw the distinct 24-hour GMT hand pointing to the 24-hour bezel
inline void draw_gmt_hand(LGFX_Sprite& canvas, float gmt_angle, const DialPalette& pal) {
  float g_rad = (gmt_angle - 90.0f) * (PI / 180.0f);
  float cos_g = cosf(g_rad);
  float sin_g = sinf(g_rad);
  float nx = -sin_g;
  float ny =  cos_g;

  // Slender colored stem with counterbalance tail
  float tail_x = DIAL_CX - 14.0f * cos_g;
  float tail_y = DIAL_CY - 14.0f * sin_g;
  float stem_x = DIAL_CX + 86.0f * cos_g;
  float stem_y = DIAL_CY + 86.0f * sin_g;
  canvas.drawWideLine(tail_x, tail_y, stem_x, stem_y, 1.2f, pal.col_gmt_hand);

  // Large arrow head reaching to the 24-hr bezel (apex at r=102, base at r=84, width ±5.5)
  float tip_x = DIAL_CX + 102.0f * cos_g;
  float tip_y = DIAL_CY + 102.0f * sin_g;
  float bl_x  = DIAL_CX + 84.0f * cos_g + 5.5f * nx;
  float bl_y  = DIAL_CY + 84.0f * sin_g + 5.5f * ny;
  float br_x  = DIAL_CX + 84.0f * cos_g - 5.5f * nx;
  float br_y  = DIAL_CY + 84.0f * sin_g - 5.5f * ny;
  canvas.fillTriangle(tip_x, tip_y, bl_x, bl_y, br_x, br_y, pal.col_gmt_hand);

  // Luminous white inner triangle insert
  float in_tip_x = DIAL_CX + 98.0f * cos_g;
  float in_tip_y = DIAL_CY + 98.0f * sin_g;
  float in_bl_x  = DIAL_CX + 86.0f * cos_g + 3.2f * nx;
  float in_bl_y  = DIAL_CY + 86.0f * sin_g + 3.2f * ny;
  float in_br_x  = DIAL_CX + 86.0f * cos_g - 3.2f * nx;
  float in_br_y  = DIAL_CY + 86.0f * sin_g - 3.2f * ny;
  canvas.fillTriangle(in_tip_x, in_tip_y, in_bl_x, in_bl_y, in_br_x, in_br_y, pal.col_gmt_fill);
}

// Draw the Flieger hands with anti-aliasing and depth borders
inline void draw_flieger_hands(LGFX_Sprite& canvas,
                               float h_angle,
                               float m_angle,
                               float s_angle,
                               const DialPalette& pal,
                               bool sync_burst) {
  // 1. Hour Hand (Lozenge silhouette with border)
  // Outer black border
  draw_flieger_polygon(canvas, h_angle,
                       56.0f + 1.2f, 28.0f, 4.2f + 1.2f, 6.0f, 1.8f + 1.0f, 10.0f + 1.0f,
                       pal.col_hand_border);
  // Inner lume fill
  draw_flieger_polygon(canvas, h_angle,
                       56.0f, 28.0f, 4.2f, 6.0f, 1.8f, 10.0f,
                       pal.col_hand_fill);

  // 2. Minute Hand (Long sword silhouette with border)
  // Outer black border
  draw_flieger_polygon(canvas, m_angle,
                       90.0f + 1.2f, 50.0f, 3.5f + 1.2f, 6.0f, 1.6f + 1.0f, 12.0f + 1.0f,
                       pal.col_hand_border);
  // Inner lume fill
  draw_flieger_polygon(canvas, m_angle,
                       90.0f, 50.0f, 3.5f, 6.0f, 1.6f, 12.0f,
                       pal.col_hand_fill);

  // 3. Second Hand (Fine needle with counterbalance & teardrop pip)
  float s_rad = (s_angle - 90.0f) * (PI / 180.0f);
  float cos_s = cosf(s_rad);
  float sin_s = sinf(s_rad);

  float tip_x  = DIAL_CX + 101.0f * cos_s;
  float tip_y  = DIAL_CY + 101.0f * sin_s;
  float tail_x = DIAL_CX - 20.0f * cos_s;
  float tail_y = DIAL_CY - 20.0f * sin_s;

  float pip_x  = DIAL_CX - 13.0f * cos_s;
  float pip_y  = DIAL_CY - 13.0f * sin_s;

  // Counterbalance pip (border + fill)
  canvas.fillSmoothCircle(pip_x, pip_y, 3.2f, pal.col_hand_border);
  canvas.fillSmoothCircle(pip_x, pip_y, 2.2f, pal.col_sec_pip);

  // Needle second hand with aviation red tip
  float tip_mid_x = DIAL_CX + 88.0f * cos_s;
  float tip_mid_y = DIAL_CY + 88.0f * sin_s;
  canvas.drawWideLine(tail_x, tail_y, tip_mid_x, tip_mid_y, 0.6f, pal.col_sec_hand);
  uint16_t red_tip_col = canvas.color565(240, 50, 40);
  canvas.drawWideLine(tip_mid_x, tip_mid_y, tip_x, tip_y, 0.75f, red_tip_col);

  // Optional momentary sync notification badge
  if (sync_burst) {
    canvas.setFont(&fonts::DejaVu9);
    canvas.setTextDatum(textdatum_t::middle_center);
    canvas.fillRoundRect(DIAL_CX - 24, DIAL_CY - 18, 48, 14, 3, 0x0540);
    canvas.setTextColor(0xFFFF);
    canvas.drawString("SYNC", DIAL_CX, DIAL_CY - 11);
  }
}

// Standalone center pinion cap so it layers cleanly above Garfield
inline void draw_center_pinion(LGFX_Sprite& canvas, const DialPalette& pal) {
  canvas.fillSmoothCircle(DIAL_CX, DIAL_CY, 4.5f, pal.col_cap_outer);
  canvas.fillSmoothCircle(DIAL_CX, DIAL_CY, 2.0f, pal.col_cap_inner);
}
