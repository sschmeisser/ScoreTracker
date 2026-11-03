#pragma once

// =============================================================================
// CYD SCORE TRACKER — TOUCH-FIRST RENDERER (ESP32-2432S028R, 320x240 landscape)
// =============================================================================
// Single caller-owned LGFX_Sprite. Everything is drawn relative to (0,0); this
// file NEVER pushes to the LCD. Reuses the round tracker's cali-zen language:
// rounded cards, muted status pills, sage records, warm-sand scores, gold winner
// chevron, and native 32x26 chroma-key blitted team logos where available.
// =============================================================================

#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <stdio.h>
#include <string.h>
#include "game_data.h"
#include "nfl_logos.h"

#define CYD_W 320
#define CYD_H 240
#define CYD_BAND_H 120

enum CydTouchAction {
  CYD_TOUCH_NONE,
  CYD_TOUCH_PREV,
  CYD_TOUCH_NEXT,
  CYD_TOUCH_TOGGLE,
  CYD_TOUCH_CARD,
  CYD_TOUCH_DETAIL_BACK
};

// ---- Palette (calm zen, mirrors ARCHITECTURE.md) ---------------------------
static const uint16_t CYD_BG        = 0x1103; // deep forest moss
static const uint16_t CYD_CARD      = 0x18E3; // lifted card
static const uint16_t CYD_HAIR      = 0x21A5; // pine hairline
static const uint16_t CYD_TEXT      = 0xDEB8; // warm sand
static const uint16_t CYD_SAGE      = 0x84B0; // sage records
static const uint16_t CYD_CODE      = 0xBDD4; // pale sage team code
static const uint16_t CYD_SUB       = 0x744F; // earthy eucalyptus sub-text
static const uint16_t CYD_AMBER     = 0x39E4; // live accent
static const uint16_t CYD_GOLD      = 0xFE60; // winner gold
static const uint16_t CYD_FOG       = 0x5AE9; // upcoming moss fog
static const uint16_t CYD_DOT_OFF   = 0x29C6; // inactive pager dot

// Status pill fills/borders/texts
static const uint16_t CYD_PILL_FINAL_BG = 0x1964;
static const uint16_t CYD_PILL_FINAL_BD = 0x3A88;
static const uint16_t CYD_PILL_FINAL_TX = 0x9DD4;
static const uint16_t CYD_PILL_LIVE_BG  = 0x39E4;
static const uint16_t CYD_PILL_LIVE_BD  = 0x6AE7;
static const uint16_t CYD_PILL_LIVE_TX  = 0xDF78;
static const uint16_t CYD_PILL_OT_BG    = 0x21A5;
static const uint16_t CYD_PILL_OT_BD    = 0x4A89;
static const uint16_t CYD_PILL_OT_TX    = 0xBDD4;

// ---- Layout constants (shared by drawing + hit-testing) --------------------
static const int CYD_HEADER_H = 20;
static const int CYD_FOOTER_H = 16;
static const int CYD_NAV_TOP  = CYD_H - CYD_FOOTER_H;      // 224
static const int CYD_CARD_X   = 4;
static const int CYD_CARD_W   = CYD_W - 2 * CYD_CARD_X;    // 312
static const int CYD_CARD_H   = 66;
static const int CYD_CARD_GAP = 3;
static const int CYD_CARD_TOP = CYD_HEADER_H;              // 20
static const int CYD_CARDS_PER_PAGE = 3;
static const int CYD_ROW_AWAY_LOGO_DY = 16;                // from card top
static const int CYD_ROW_HOME_LOGO_DY = 49;
static const int CYD_SCORE_RIGHT      = CYD_CARD_X + CYD_CARD_W - 8; // 308

// Two-band framebuffer: a scene y is drawn at sprite y = y - camera_y.
static inline int cyd_sy(int y, int camera_y) { return y - camera_y; }

// Set true by the detail renderer so a stray tap is reported as DETAIL_BACK.
// main.cpp owns the overlay state; this flag is only a convenience fallback.
static bool g_cyd_detail_open = false;

// ---- Integer trig (Q10 sine table, 0..90 deg) ------------------------------
static const int16_t CYD_SIN_Q10[91] = {
     0,   18,   36,   54,   71,   89,  107,  125,  143,  160,
   178,  195,  213,  230,  248,  265,  282,  299,  316,  333,
   350,  367,  384,  400,  416,  433,  449,  465,  481,  496,
   512,  527,  543,  558,  573,  587,  602,  616,  630,  644,
   658,  672,  685,  698,  711,  724,  737,  749,  761,  773,
   784,  796,  807,  818,  828,  839,  849,  859,  868,  878,
   887,  896,  904,  912,  920,  928,  935,  943,  949,  956,
   962,  968,  974,  979,  984,  989,  994,  998, 1002, 1005,
  1008, 1011, 1014, 1016, 1018, 1020, 1022, 1023, 1023, 1024,
  1024
};

static inline int cyd_sin_q10(int deg) {
  deg %= 360;
  if (deg < 0) deg += 360;
  int s = 1;
  if (deg >= 180) { deg -= 180; s = -1; }
  if (deg > 90) deg = 180 - deg;
  return s * CYD_SIN_Q10[deg];
}

static inline int cyd_cos_q10(int deg) { return cyd_sin_q10(deg + 90); }

// ---- Status classification / pill helpers ----------------------------------
static inline bool cyd_status_is_ot(const char* st) {
  if (!st) return false;
  return strstr(st, "OT") || strstr(st, "SO") || strstr(st, "PK") || strstr(st, "ET");
}

static inline bool cyd_status_is_final(const char* st) {
  if (!st) return false;
  return strstr(st, "FINAL") || strstr(st, "FT");
}

static inline bool cyd_status_is_live(const char* st) {
  if (!st) return false;
  return strstr(st, "LIVE") || strstr(st, "Q1") || strstr(st, "Q2") ||
         strstr(st, "Q3") || strstr(st, "Q4") || strstr(st, "HALFTIME") ||
         strstr(st, "HT") || strstr(st, "1H") || strstr(st, "2H") ||
         strstr(st, "P1") || strstr(st, "P2") || strstr(st, "P3") ||
         strstr(st, "1st") || strstr(st, "2nd") || strstr(st, "3rd") ||
         strstr(st, "4th") || strstr(st, "Half") || strstr(st, "'") ||
         strstr(st, "MIN");
}

// align: 0 => ax is the pill centre, 1 => ax is the pill right edge.
// max_w > 0 caps the pill width (border+padding included); the text is
// ellipsized to fit so the pill can never spill into a reserved column.
static inline void cyd_draw_status_pill(LGFX_Sprite& c, int ax, int py,
                                        const char* league, const char* status,
                                        int align, int max_w = 0) {
  char pill[40];
  if (league && league[0]) {
    snprintf(pill, sizeof(pill), "%s | %s", league, status ? status : "");
  } else {
    snprintf(pill, sizeof(pill), "%s", status ? status : "");
  }

  bool is_ot = cyd_status_is_ot(status);
  bool is_final = cyd_status_is_final(status);
  bool is_live = !is_final && cyd_status_is_live(status);

  uint16_t bg, bd, tx;
  if (is_live)      { bg = CYD_PILL_LIVE_BG;  bd = CYD_PILL_LIVE_BD;  tx = CYD_PILL_LIVE_TX; }
  else if (is_ot)   { bg = CYD_PILL_OT_BG;    bd = CYD_PILL_OT_BD;    tx = CYD_PILL_OT_TX; }
  else              { bg = CYD_PILL_FINAL_BG; bd = CYD_PILL_FINAL_BD; tx = CYD_PILL_FINAL_TX; }

  c.setFont(&fonts::Font0);

  // Ellipsize against the width budget so the pill never overlaps its neighbours.
  if (max_w > 0) {
    int avail = max_w - 12;
    if (avail < 0) avail = 0;
    if ((int)c.textWidth(pill) > avail) {
      int n = (int)strlen(pill);
      while (n > 0) {
        char cand[40];
        pill[n] = 0;
        snprintf(cand, sizeof(cand), "%s..", pill);
        if ((int)c.textWidth(cand) <= avail) break;
        --n;
      }
      strncat(pill, "..", sizeof(pill) - strlen(pill) - 1);
    }
  }

  int pw = c.textWidth(pill) + 12;
  if (pw < 36) pw = 36;
  int px = (align == 1) ? (ax - pw) : (ax - pw / 2);

  c.fillRoundRect(px, py, pw, 13, 3, bg);
  c.drawRoundRect(px, py, pw, 13, 3, bd);
  c.setTextDatum(textdatum_t::middle_center);
  c.setTextColor(tx);
  c.drawString(pill, px + pw / 2, py + 7);
}

// ---- Small drawing utilities -----------------------------------------------
// Transliterate UTF-8 diacritics / special characters to clean ASCII.
static inline void cyd_sanitize_ascii(const char* src, char* dst, size_t dst_len) {
  if (!dst || dst_len == 0) return;
  dst[0] = '\0';
  if (!src) return;

  size_t di = 0;
  for (size_t si = 0; src[si] && di + 1 < dst_len; ) {
    unsigned char c = (unsigned char)src[si];
    if (c < 0x80) {
      dst[di++] = (char)c;
      si++;
    } else if (c == 0xC3 && src[si + 1]) {
      unsigned char c2 = (unsigned char)src[si + 1];
      si += 2;
      char rep = 0;
      if (c2 >= 0x80 && c2 <= 0x85) rep = 'A';
      else if (c2 == 0x86)          rep = 'A';
      else if (c2 == 0x87)          rep = 'C';
      else if (c2 >= 0x88 && c2 <= 0x8B) rep = 'E';
      else if (c2 >= 0x8C && c2 <= 0x8F) rep = 'I';
      else if (c2 == 0x90)          rep = 'D';
      else if (c2 == 0x91)          rep = 'N';
      else if (c2 >= 0x92 && c2 <= 0x96) rep = 'O';
      else if (c2 == 0x98)          rep = 'O';
      else if (c2 >= 0x99 && c2 <= 0x9C) rep = 'U';
      else if (c2 == 0x9D)          rep = 'Y';
      else if (c2 == 0x9F)          rep = 's';
      else if (c2 >= 0xA0 && c2 <= 0xA5) rep = 'a';
      else if (c2 == 0xA6)          rep = 'a';
      else if (c2 == 0xA7)          rep = 'c';
      else if (c2 >= 0xA8 && c2 <= 0xAB) rep = 'e'; // 'é' is 0xA9 -> 'e'
      else if (c2 >= 0xAC && c2 <= 0xAF) rep = 'i';
      else if (c2 == 0xB0)          rep = 'd';
      else if (c2 == 0xB1)          rep = 'n';
      else if (c2 >= 0xB2 && c2 <= 0xB6) rep = 'o';
      else if (c2 == 0xB8)          rep = 'o';
      else if (c2 >= 0xB9 && c2 <= 0xBC) rep = 'u';
      else if (c2 == 0xBD || c2 == 0xBF) rep = 'y';
      if (rep && di + 1 < dst_len) dst[di++] = rep;
    } else if (c == 0xC4 && src[si + 1]) {
      unsigned char c2 = (unsigned char)src[si + 1];
      si += 2;
      char rep = 0;
      if (c2 == 0x86 || c2 == 0x8C) rep = 'C';
      else if (c2 == 0x87 || c2 == 0x8D) rep = 'c';
      if (rep && di + 1 < dst_len) dst[di++] = rep;
    } else if (c == 0xC5 && src[si + 1]) {
      unsigned char c2 = (unsigned char)src[si + 1];
      si += 2;
      char rep = 0;
      if (c2 == 0xA0) rep = 'S';
      else if (c2 == 0xA1) rep = 's';
      else if (c2 == 0xBD) rep = 'Z';
      else if (c2 == 0xBE) rep = 'z';
      if (rep && di + 1 < dst_len) dst[di++] = rep;
    } else {
      if ((c & 0xE0) == 0xC0) {
        si += (src[si + 1] ? 2 : 1);
      } else if ((c & 0xF0) == 0xE0) {
        if (c == 0xE2 && (unsigned char)src[si+1] == 0x80) {
          unsigned char c3 = (unsigned char)src[si+2];
          if (c3 == 0x98 || c3 == 0x99) { if (di + 1 < dst_len) dst[di++] = '\''; }
          else if (c3 == 0x93 || c3 == 0x94) { if (di + 1 < dst_len) dst[di++] = '-'; }
        }
        si += (src[si + 1] && src[si + 2] ? 3 : 1);
      } else if ((c & 0xF8) == 0xF0) {
        si += (src[si + 1] && src[si + 2] && src[si + 3] ? 4 : 1);
      } else {
        si++;
      }
    }
  }
  dst[di] = '\0';
}

// Draw `s` clipped to max_w px (current font) and return the drawn width.
static inline int cyd_draw_text_fit(LGFX_Sprite& c, const char* s, int x, int y,
                                    int max_w, uint16_t color) {
  if (!s || !s[0] || max_w <= 0) return 0;
  char clean[64];
  cyd_sanitize_ascii(s, clean, sizeof(clean));
  char buf[64];
  size_t n = strlen(clean);
  if (n > sizeof(buf) - 1) n = sizeof(buf) - 1;
  memcpy(buf, clean, n);
  buf[n] = 0;
  while (n > 0 && c.textWidth(buf) > max_w) buf[--n] = 0;
  c.setTextColor(color);
  c.drawString(buf, x, y);
  return c.textWidth(buf);
}

// Right-pointing gold chevron marking the winner.
static inline void cyd_draw_chevron(LGFX_Sprite& c, int x, int y) {
  c.fillTriangle(x, y - 5, x, y + 5, x + 5, y, CYD_GOLD);
}

// Native 32x26 chroma-key logo blit; league-tinted monogram badge fallback.
static inline void cyd_draw_team_logo(LGFX_Sprite& c, int cx, int cy,
                                      const char* code, const char* league) {
  if (!code) return;

  const uint16_t* bmp = get_team_logo_bitmap(league, code);
  if (bmp != nullptr) {
    c.setSwapBytes(true);
    c.pushImage(cx - LOGO_WIDTH / 2, cy - LOGO_HEIGHT / 2,
                LOGO_WIDTH, LOGO_HEIGHT,
                bmp, (uint16_t)LOGO_CHROMA_KEY);
    c.setSwapBytes(false);
    return;
  }

  uint16_t bg = 0x1964, bd = 0x3A88, tx = 0xDEB8;
  if (league) {
    if      (strcmp(league, "NHL") == 0) { bg = 0x18C8; bd = 0x3A8E; tx = 0xCE79; }
    else if (strcmp(league, "NBA") == 0) { bg = 0x2964; bd = 0x5AE7; tx = 0xDF78; }
    else if (strcmp(league, "MLS") == 0) { bg = 0x11E5; bd = 0x2A67; tx = 0xBDD4; }
    else if (strcmp(league, "CFB") == 0) { bg = 0x2185; bd = 0x4A88; tx = 0xDEB8; }
    else if (strcmp(league, "CBB") == 0) { bg = 0x4208; bd = 0xD384; tx = 0xFBE0; }
  }
  c.fillCircle(cx, cy, 12, bg);
  c.drawCircle(cx, cy, 12, bd);
  c.setFont(&fonts::Font0);
  c.setTextDatum(textdatum_t::middle_center);
  c.setTextColor(tx);
  char abbr[4] = {0};
  strncpy(abbr, code, 3);
  c.drawString(abbr, cx, cy + 1);
}

// One team row inside a compact game card.
static inline void cyd_card_team_row(LGFX_Sprite& c, const SportGame& g,
                                     bool away, int card_x, int card_y,
                                     bool win, bool upcoming, int camera_y) {
  const char* code = away ? g.away_code : g.home_code;
  const char* name = away ? g.away_name : g.home_name;
  const char* rec  = away ? g.away_rec  : g.home_rec;
  uint8_t     rank = away ? g.away_rank : g.home_rank;
  uint16_t    score = away ? g.away_score : g.home_score;

  int logo_cy = cyd_sy(card_y + (away ? CYD_ROW_AWAY_LOGO_DY : CYD_ROW_HOME_LOGO_DY),
                       camera_y);
  int code_cy = logo_cy - 7;
  int name_cy = logo_cy + 8;
  int score_cy = logo_cy;

  // Logo
  cyd_draw_team_logo(c, card_x + 20, logo_cy, code, g.league);

  // Bold, larger team code (rank prefix when present)
  char codebuf[16];
  if (rank > 0 && rank <= 25) snprintf(codebuf, sizeof(codebuf), "#%d %s", rank, code ? code : "");
  else                        snprintf(codebuf, sizeof(codebuf), "%s", code ? code : "");
  c.setFont(&fonts::Font2);
  c.setTextDatum(textdatum_t::middle_left);
  c.setTextColor(win ? CYD_TEXT : CYD_CODE);
  c.drawString(codebuf, card_x + 48, code_cy);

  // Team name + record (record in sage)
  c.setFont(&fonts::Font0);
  c.setTextDatum(textdatum_t::middle_left);
  int nx = card_x + 48;
  int rec_w = (rec && rec[0]) ? c.textWidth(rec) : 0;
  int name_max = (card_x + CYD_CARD_W - 12) - nx - rec_w - 6;
  int nw = cyd_draw_text_fit(c, name ? name : "", nx, name_cy, name_max, CYD_SUB);
  if (rec && rec[0]) {
    c.setTextDatum(textdatum_t::middle_left);
    c.setTextColor(CYD_SAGE);
    c.drawString(rec, nx + nw + 6, name_cy);
  }

  // Right-aligned score + winner chevron
  char sb[8];
  if (upcoming) strcpy(sb, "-");
  else          itoa((int)score, sb, 10);
  c.setFont(&fonts::FreeSansBold12pt7b);
  c.setTextDatum(textdatum_t::middle_right);
  c.setTextColor(upcoming ? CYD_FOG : (win ? CYD_TEXT : CYD_SAGE));
  c.drawString(sb, CYD_SCORE_RIGHT, score_cy);
  if (win) {
    int sw = c.textWidth(sb);
    cyd_draw_chevron(c, CYD_SCORE_RIGHT - sw - 9, score_cy - 1);
  }
}

// One compact game card (both teams).
static inline void cyd_draw_card(LGFX_Sprite& c, const SportGame& g, int x, int y,
                                 int camera_y) {
  const char* st = g.status ? g.status : "";
  bool is_final = cyd_status_is_final(st);
  bool is_live = !is_final && cyd_status_is_live(st);
  bool upcoming = (!is_final && !is_live && g.away_score == 0 && g.home_score == 0);
  bool away_win = (!upcoming && g.away_score > g.home_score);
  bool home_win = (!upcoming && g.home_score > g.away_score);

  int dy = cyd_sy(y, camera_y);
  c.fillRoundRect(x, dy, CYD_CARD_W, CYD_CARD_H, 7, CYD_CARD);
  c.drawRoundRect(x, dy, CYD_CARD_W, CYD_CARD_H, 7, CYD_HAIR);

  // Live / amber edge accent whenever the game is not FINAL.
  if (!is_final) {
    c.fillRoundRect(x, dy + 6, 3, CYD_CARD_H - 12, 2, CYD_AMBER);
  }

  // Status pill anchored to the card's top-right corner, kept clear of both the
  // left code/name column and the right score/chevron column.
  char cb1[16], cb2[16];
  if (g.away_rank > 0 && g.away_rank <= 25)
    snprintf(cb1, sizeof(cb1), "#%d %s", g.away_rank, g.away_code ? g.away_code : "");
  else
    snprintf(cb1, sizeof(cb1), "%s", g.away_code ? g.away_code : "");
  if (g.home_rank > 0 && g.home_rank <= 25)
    snprintf(cb2, sizeof(cb2), "#%d %s", g.home_rank, g.home_code ? g.home_code : "");
  else
    snprintf(cb2, sizeof(cb2), "%s", g.home_code ? g.home_code : "");
  c.setFont(&fonts::Font2);
  int cw1 = c.textWidth(cb1), cw2 = c.textWidth(cb2);
  int code_right = x + 48 + (cw1 > cw2 ? cw1 : cw2);

  char s1[8], s2[8];
  itoa((int)g.away_score, s1, 10);
  itoa((int)g.home_score, s2, 10);
  c.setFont(&fonts::FreeSansBold12pt7b);
  int sw1 = c.textWidth(s1), sw2 = c.textWidth(s2);
  int score_left = CYD_SCORE_RIGHT - (sw1 > sw2 ? sw1 : sw2);
  int pill_right = score_left - 9 - 5 - 4;      // score text + chevron + margin
  int pill_max_w = pill_right - (code_right + 4);
  if (pill_max_w < 0) pill_max_w = 0;
  cyd_draw_status_pill(c, pill_right, dy + 3, g.league, st, 1, pill_max_w);

  cyd_card_team_row(c, g, true,  x, y, away_win, upcoming, camera_y);
  cyd_card_team_row(c, g, false, x, y, home_win, upcoming, camera_y);
}

// Draw one page (3 cards) at a vertical scene offset.
static inline void cyd_draw_page(LGFX_Sprite& c, const SportGame* games, int total,
                                 int page, int y_offset, int camera_y) {
  int base = page * CYD_CARDS_PER_PAGE;
  for (int i = 0; i < CYD_CARDS_PER_PAGE; ++i) {
    int idx = base + i;
    if (idx < 0 || idx >= total) continue; // guard short/empty pages
    int cy = CYD_CARD_TOP + i * (CYD_CARD_H + CYD_CARD_GAP) + y_offset;
    cyd_draw_card(c, games[idx], CYD_CARD_X, cy, camera_y);
  }
}

// Fixed opaque header (title | date of current card | page + paused + USB).
static inline void cyd_draw_header(LGFX_Sprite& c, const SportGame* games, int total,
                                   int page, int total_pages, bool paused, bool wifi_on,
                                   int camera_y) {
  c.fillRect(0, cyd_sy(0, camera_y), CYD_W, CYD_HEADER_H, CYD_BG);
  c.drawFastHLine(0, cyd_sy(CYD_HEADER_H - 1, camera_y), CYD_W, CYD_HAIR);
  const int hy = cyd_sy(CYD_HEADER_H / 2, camera_y);

  // Right cluster, laid out from the right edge inward. Record its left edge so
  // the centred date can be measured against it.
  char pg[16];
  snprintf(pg, sizeof(pg), "%d/%d", page + 1, total_pages);
  c.setFont(&fonts::DejaVu9);
  c.setTextDatum(textdatum_t::middle_right);
  c.setTextColor(CYD_TEXT);
  int rx = CYD_W - 8;
  c.drawString(pg, rx, hy);
  rx -= c.textWidth(pg) + 10;

  if (paused) {
    int by = hy - 4;
    c.fillRect(rx - 8, by, 3, 8, CYD_AMBER);
    c.fillRect(rx - 3, by, 3, 8, CYD_AMBER);
    rx -= 16;
  }
  if (!wifi_on) {
    c.setFont(&fonts::Font0);
    c.setTextDatum(textdatum_t::middle_right);
    c.setTextColor(CYD_SUB);
    c.drawString("USB", rx, hy);
    rx -= c.textWidth("USB") + 10;
  }
  const int cluster_left = rx;

  // Day/date and league determination for the current page.
  int cur = page * CYD_CARDS_PER_PAGE;

  // Determine league(s) currently displayed on this page
  char distinct_leagues[3][8];
  int distinct_count = 0;
  if (games && total > 0 && cur >= 0 && cur < total) {
    for (int i = cur; i < cur + CYD_CARDS_PER_PAGE && i < total; ++i) {
      if (games[i].league && games[i].league[0]) {
        bool found = false;
        for (int d = 0; d < distinct_count; ++d) {
          if (strcmp(distinct_leagues[d], games[i].league) == 0) {
            found = true;
            break;
          }
        }
        if (!found && distinct_count < 3) {
          strncpy(distinct_leagues[distinct_count], games[i].league, sizeof(distinct_leagues[0]) - 1);
          distinct_leagues[distinct_count][sizeof(distinct_leagues[0]) - 1] = '\0';
          distinct_count++;
        }
      }
    }
  }

  char title[32];
  if (distinct_count == 1) {
    snprintf(title, sizeof(title), "%s SCORES", distinct_leagues[0]);
  } else if (distinct_count == 2) {
    snprintf(title, sizeof(title), "%s/%s SCORES", distinct_leagues[0], distinct_leagues[1]);
  } else {
    snprintf(title, sizeof(title), "SCORES");
  }

  // Left title with current league
  c.setFont(&fonts::FreeSansBold9pt7b);
  c.setTextDatum(textdatum_t::middle_left);
  c.setTextColor(CYD_TEXT);
  c.drawString(title, 8, hy);
  const int title_right = 8 + c.textWidth(title);

  // Day/date, centred in the gap between title and cluster, only when it fits.
  if (games && cur >= 0 && cur < total) {
    char db[24];
    snprintf(db, sizeof(db), "%s %s", games[cur].day ? games[cur].day : "",
             games[cur].date ? games[cur].date : "");
    c.setFont(&fonts::DejaVu9);
    int dw = c.textWidth(db);
    int gap_l = title_right + 8;
    int gap_r = cluster_left - 4;
    if (gap_r - gap_l >= dw) {
      c.setTextDatum(textdatum_t::middle_center);
      c.setTextColor(CYD_SAGE);
      c.drawString(db, (gap_l + gap_r) / 2, hy);
    }
  }
}

// Fixed opaque footer (pager dots + faint touch hints).
static inline void cyd_draw_footer(LGFX_Sprite& c, int total_pages, int active_page,
                                   bool paused, int camera_y) {
  c.fillRect(0, cyd_sy(CYD_NAV_TOP, camera_y), CYD_W, CYD_FOOTER_H, CYD_BG);
  c.drawFastHLine(0, cyd_sy(CYD_NAV_TOP, camera_y), CYD_W, CYD_HAIR);

  // Pager dots (max 12 shown; active scaled when pages exceed the cap).
  int n = total_pages;
  if (n < 1) n = 1;
  if (n > 12) n = 12;
  int active = active_page;
  if (total_pages > 12) active = (active_page * 12) / total_pages;
  active = active % n;

  const int spacing = 10;
  const int dots_y = cyd_sy(CYD_NAV_TOP + 5, camera_y);
  int start = CYD_W / 2 - ((n - 1) * spacing) / 2;
  for (int i = 0; i < n; ++i) {
    int dx = start + i * spacing;
    if (i == active) c.fillCircle(dx, dots_y, 2, CYD_TEXT);
    else             c.fillCircle(dx, dots_y, 1, CYD_DOT_OFF);
  }

  c.setFont(&fonts::Font0);
  c.setTextColor(CYD_SUB);
  const int hy = cyd_sy(CYD_NAV_TOP + 11, camera_y);
  c.setTextDatum(textdatum_t::middle_left);
  c.drawString("< PREV", 8, hy);
  c.setTextDatum(textdatum_t::middle_center);
  c.drawString(paused ? "RESUME" : "PAUSE", CYD_W / 2, hy);
  c.setTextDatum(textdatum_t::middle_right);
  c.drawString("NEXT >", CYD_W - 8, hy);
}

// =============================================================================
// PUBLIC API
// =============================================================================

inline void cyd_draw_score_tracker(LGFX_Sprite& canvas, int camera_y,
                                   int current_page, int offset_y, int next_page,
                                   bool transitioning, const SportGame* games,
                                   int total, bool paused, bool wifi_on) {
  g_cyd_detail_open = false;
  canvas.fillScreen(CYD_BG);

  if (!games || total <= 0) {
    cyd_draw_header(canvas, nullptr, 0, 0, 1, paused, wifi_on, camera_y);
    cyd_draw_footer(canvas, 1, 0, paused, camera_y);
    canvas.setFont(&fonts::DejaVu9);
    canvas.setTextDatum(textdatum_t::middle_center);
    canvas.setTextColor(CYD_SAGE);
    canvas.drawString("NO GAMES", CYD_W / 2, cyd_sy(CYD_H / 2, camera_y));
    return;
  }

  int total_pages = (total + CYD_CARDS_PER_PAGE - 1) / CYD_CARDS_PER_PAGE;
  if (total_pages < 1) total_pages = 1;
  int cur = ((current_page % total_pages) + total_pages) % total_pages;
  int nxt = ((next_page % total_pages) + total_pages) % total_pages;

  if (offset_y < 0) offset_y = 0;
  if (offset_y > CYD_H) offset_y = CYD_H;

  // Current page slides up; next page slides in from below (scene space; sprite clips).
  cyd_draw_page(canvas, games, total, cur, -offset_y, camera_y);
  if (transitioning && offset_y != 0) {
    cyd_draw_page(canvas, games, total, nxt, CYD_H - offset_y, camera_y);
  }

  // Chrome painted last, opaque, so sliding cards clip beneath it.
  cyd_draw_header(canvas, games, total, cur, total_pages, paused, wifi_on, camera_y);
  cyd_draw_footer(canvas, total_pages, transitioning ? nxt : cur, paused, camera_y);
}

inline void cyd_draw_sync_screen(LGFX_Sprite& canvas, int camera_y, int loaded, int total) {
  g_cyd_detail_open = false;
  canvas.fillScreen(CYD_BG);

  const int cx = CYD_W / 2;
  const int cy = 126;
  const int R  = 64;

  canvas.drawCircle(cx, cyd_sy(cy, camera_y), R, CYD_HAIR); // faint full track

  // Progress arc via integer trig, thickened to 3px with dot runs.
  int sweep = (total > 0) ? (loaded * 360) / total : 0;
  if (sweep < 0) sweep = 0;
  if (sweep > 360) sweep = 360;
  for (int d = 0; d <= sweep; ++d) {
    int a = d - 90; // start at 12 o'clock
    int cs = cyd_cos_q10(a);
    int sn = cyd_sin_q10(a);
    for (int t = -1; t <= 1; ++t) {
      int rr = R + t;
      canvas.drawPixel(cx + (rr * cs) / 1024,
                       cyd_sy(cy + (rr * sn) / 1024, camera_y), CYD_GOLD);
    }
  }

  // Title pill
  const char* title = "SYNCING SCORES";
  canvas.setFont(&fonts::FreeSansBold9pt7b);
  int tw = canvas.textWidth(title);
  int pw = tw + 18;
  int px = cx - pw / 2;
  canvas.fillRoundRect(px, cyd_sy(30, camera_y), pw, 18, 5, CYD_PILL_LIVE_BG);
  canvas.drawRoundRect(px, cyd_sy(30, camera_y), pw, 18, 5, CYD_PILL_LIVE_BD);
  canvas.setTextDatum(textdatum_t::middle_center);
  canvas.setTextColor(CYD_PILL_LIVE_TX);
  canvas.drawString(title, cx, cyd_sy(39, camera_y));

  // loaded / total
  char cnt[24];
  if (total > 0) snprintf(cnt, sizeof(cnt), "%d / %d", loaded, total);
  else           strcpy(cnt, "...");
  canvas.setFont(&fonts::FreeSansBold18pt7b);
  canvas.setTextDatum(textdatum_t::middle_center);
  canvas.setTextColor(CYD_TEXT);
  canvas.drawString(cnt, cx, cyd_sy(cy - 6, camera_y));

  canvas.setFont(&fonts::Font0);
  canvas.setTextColor(CYD_SAGE);
  canvas.drawString(total > 0 ? "GAMES LOADED" : "CONNECTING", cx,
                    cyd_sy(cy + 24, camera_y));
}

// =============================================================================
// GAME DETAIL FULL-SCREEN OVERLAY
// =============================================================================

// Unified matchup card containing both Away and Home teams.
static inline void cyd_detail_matchup_card(LGFX_Sprite& c, const SportGame& g, int camera_y) {
  const int card_x = 16, card_y = 25, card_w = 288, card_h = 76;
  int dy = cyd_sy(card_y, camera_y);

  c.fillRoundRect(card_x, dy, card_w, card_h, 7, CYD_CARD);
  c.drawRoundRect(card_x, dy, card_w, card_h, 7, CYD_HAIR);

  const char* st = g.status ? g.status : "";
  bool is_final = cyd_status_is_final(st);
  bool is_live = !is_final && cyd_status_is_live(st);
  bool upcoming = (!is_final && !is_live && g.away_score == 0 && g.home_score == 0);

  // Live accent bar on left edge
  if (is_live) {
    c.fillRoundRect(card_x, dy + 6, 3, card_h - 12, 2, CYD_AMBER);
  }

  // Divider between Away and Home team
  c.drawFastHLine(card_x + 8, dy + 38, card_w - 16, CYD_HAIR);

  for (int row = 0; row < 2; ++row) {
    bool away = (row == 0);
    const char* code = away ? g.away_code : g.home_code;
    const char* name = away ? g.away_name : g.home_name;
    const char* rec  = away ? g.away_rec  : g.home_rec;
    uint8_t     rank = away ? g.away_rank : g.home_rank;
    uint16_t    score = away ? g.away_score : g.home_score;
    bool win = is_final && (!upcoming && ((away && g.away_score > g.home_score) ||
                                          (!away && g.home_score > g.away_score)));
    bool leading = is_live && (!upcoming && ((away && g.away_score > g.home_score) ||
                                            (!away && g.home_score > g.away_score)));

    int cy = dy + (away ? 19 : 57);

    // Team logo
    cyd_draw_team_logo(c, card_x + 22, cy, code, g.league);

    // Team code (bold)
    char codebuf[16];
    if (rank > 0 && rank <= 25) snprintf(codebuf, sizeof(codebuf), "#%d %s", rank, code ? code : "");
    else                        snprintf(codebuf, sizeof(codebuf), "%s", code ? code : "");
    c.setFont(&fonts::FreeSansBold12pt7b);
    c.setTextDatum(textdatum_t::middle_left);
    c.setTextColor((win || leading) ? CYD_TEXT : CYD_CODE);
    c.drawString(codebuf, card_x + 46, cy - 7);

    // Team name + record
    c.setFont(&fonts::Font0);
    c.setTextDatum(textdatum_t::middle_left);
    int rec_w = (rec && rec[0]) ? c.textWidth(rec) : 0;
    int nw = cyd_draw_text_fit(c, name ? name : "", card_x + 46, cy + 8,
                               card_w - 46 - 60 - rec_w - 8, CYD_SUB);
    if (rec && rec[0]) {
      c.setTextColor(CYD_SAGE);
      c.drawString(rec, card_x + 46 + nw + 6, cy + 8);
    }

    // Score
    char sb[8];
    if (upcoming) strcpy(sb, "-");
    else          itoa((int)score, sb, 10);
    c.setFont(&fonts::FreeSansBold18pt7b);
    c.setTextDatum(textdatum_t::middle_right);
    c.setTextColor(upcoming ? CYD_FOG : ((win || leading) ? CYD_TEXT : CYD_SAGE));
    const int score_right = card_x + card_w - 12;
    c.drawString(sb, score_right, cy);

    // Winner chevron ONLY if game is final
    if (win) {
      int sw = c.textWidth(sb);
      cyd_draw_chevron(c, score_right - sw - 11, cy);
    }
  }
}

// Q1..Q4 line-score table card for the detail overlay (288px width, perfectly contained).
static inline void cyd_detail_table(LGFX_Sprite& c, const SportGame& g, int camera_y) {
  const int card_x = 16, ty = 107, card_w = 288, th = 48;
  int dy = cyd_sy(ty, camera_y);

  c.fillRoundRect(card_x, dy, card_w, th, 6, CYD_CARD);
  c.drawRoundRect(card_x, dy, card_w, th, 6, CYD_HAIR);

  // Vertical line separating Quarters from Total
  c.drawFastVLine(card_x + card_w - 38, dy + 4, th - 8, CYD_HAIR);

  const int lab_x = card_x + 12; // 28
  const int qx[4] = {card_x + 92, card_x + 128, card_x + 164, card_x + 200};
  const int tot_x = card_x + card_w - 19; // 285
  const int hdr_y = dy + 9;

  c.setFont(&fonts::Font0);
  c.setTextDatum(textdatum_t::middle_left);
  c.setTextColor(CYD_SUB);
  c.drawString("TEAM", lab_x, hdr_y);

  c.setTextDatum(textdatum_t::middle_center);
  const char* qh[4] = {"Q1", "Q2", "Q3", "Q4"};
  if (g.league) {
    if (strcmp(g.league, "NHL") == 0) {
      qh[0] = "P1"; qh[1] = "P2"; qh[2] = "P3"; qh[3] = "OT";
    } else if (strcmp(g.league, "CBB") == 0) {
      qh[0] = "1H"; qh[1] = "2H"; qh[2] = "OT"; qh[3] = "OT2";
    } else if (strcmp(g.league, "MLS") == 0) {
      qh[0] = "1H"; qh[1] = "2H"; qh[2] = "ET"; qh[3] = "PK";
    }
  }
  for (int i = 0; i < 4; ++i) c.drawString(qh[i], qx[i], hdr_y);
  c.drawString("T", tot_x, hdr_y);

  const char* st = g.status ? g.status : "";
  bool is_final = cyd_status_is_final(st);
  bool is_live = !is_final && cyd_status_is_live(st);
  bool upcoming = (!is_final && !is_live && g.away_score == 0 && g.home_score == 0);

  for (int row = 0; row < 2; ++row) {
    bool away = (row == 0);
    int ry = dy + 25 + row * 14;
    const uint8_t* q = away ? g.away_q : g.home_q;
    uint16_t score = away ? g.away_score : g.home_score;
    const char* code = away ? g.away_code : g.home_code;
    bool win = is_final && (!upcoming && ((away && g.away_score > g.home_score) ||
                                          (!away && g.home_score > g.away_score)));
    bool leading = is_live && (!upcoming && ((away && g.away_score > g.home_score) ||
                                            (!away && g.home_score > g.away_score)));
    uint16_t col = (win || leading) ? CYD_TEXT : CYD_SAGE;

    c.setFont(&fonts::DejaVu9);
    c.setTextDatum(textdatum_t::middle_left);
    c.setTextColor((win || leading) ? CYD_TEXT : CYD_CODE);
    c.drawString(code ? code : "", lab_x, ry);

    c.setTextDatum(textdatum_t::middle_center);
    for (int i = 0; i < 4; ++i) {
      char qb[8];
      if (upcoming) {
        strcpy(qb, "-");
      } else if (g.league && strcmp(g.league, "NHL") == 0 && i == 3 && q[3] == 0 && g.away_q[3] == 0 && g.home_q[3] == 0 && !strstr(st, "OT") && !strstr(st, "SO")) {
        strcpy(qb, "-");
      } else if (g.league && strcmp(g.league, "CBB") == 0 && i >= 2 && q[i] == 0 && g.away_q[i] == 0 && g.home_q[i] == 0 && !strstr(st, "OT")) {
        strcpy(qb, "-");
      } else {
        itoa((int)q[i], qb, 10);
      }
      c.setTextColor(col);
      c.drawString(qb, qx[i], ry);
    }
    char tb[8];
    if (upcoming) strcpy(tb, "-");
    else          itoa((int)score, tb, 10);
    c.setTextColor(col);
    c.drawString(tb, tot_x, ry);
  }
}

// Leader & Venue stat card (aligned columns, clean card framing).
static inline void cyd_detail_info_card(LGFX_Sprite& c, const SportGame& g, int camera_y) {
  const int card_x = 16, iy = 161, card_w = 288, ih = 48;
  int dy = cyd_sy(iy, camera_y);

  c.fillRoundRect(card_x, dy, card_w, ih, 6, CYD_CARD);
  c.drawRoundRect(card_x, dy, card_w, ih, 6, CYD_HAIR);

  // Divider between Leader and Venue
  c.drawFastHLine(card_x + 8, dy + 24, card_w - 16, CYD_HAIR);

  const int val_x = card_x + 64; // 80 - perfectly aligned column!
  const int max_val_w = card_w - 72; // 216

  // Row 1: LEADER
  c.setFont(&fonts::Font0);
  c.setTextDatum(textdatum_t::middle_left);
  c.setTextColor(CYD_SUB);
  c.drawString("LEADER", card_x + 10, dy + 12);
  cyd_draw_text_fit(c, g.leader ? g.leader : "--", val_x, dy + 12, max_val_w, CYD_TEXT);

  // Row 2: VENUE
  c.setFont(&fonts::Font0);
  c.setTextColor(CYD_SUB);
  c.drawString("VENUE", card_x + 10, dy + 36);
  cyd_draw_text_fit(c, g.stadium ? g.stadium : "--", val_x, dy + 36, max_val_w, CYD_TEXT);
}

inline void cyd_draw_detail_screen(LGFX_Sprite& canvas, int camera_y, const SportGame& g) {
  g_cyd_detail_open = true;
  canvas.fillScreen(CYD_BG);

  // Header
  const int hy = cyd_sy(CYD_HEADER_H / 2, camera_y);
  canvas.fillRect(0, cyd_sy(0, camera_y), CYD_W, CYD_HEADER_H, CYD_BG);
  canvas.drawFastHLine(0, cyd_sy(CYD_HEADER_H - 1, camera_y), CYD_W, CYD_HAIR);

  // Title: "<LEAGUE> DETAIL" or "GAME DETAIL"
  char title[24];
  if (g.league && g.league[0]) snprintf(title, sizeof(title), "%s DETAIL", g.league);
  else                         strcpy(title, "GAME DETAIL");

  canvas.setFont(&fonts::FreeSansBold9pt7b);
  canvas.setTextDatum(textdatum_t::middle_left);
  canvas.setTextColor(CYD_TEXT);
  canvas.drawString(title, 8, hy);
  const int title_right = 8 + canvas.textWidth(title);

  // Right status pill (status only, e.g. "3:58 - 4th" / "FINAL")
  cyd_draw_status_pill(canvas, CYD_W - 8, cyd_sy(3, camera_y), nullptr, g.status, 1);

  canvas.setFont(&fonts::Font0);
  int pw = canvas.textWidth(g.status ? g.status : "") + 12;
  if (pw < 36) pw = 36;
  const int pill_left = CYD_W - 8 - pw;

  // Day/Date in center gap (only when it fits cleanly without collision)
  char db[24];
  snprintf(db, sizeof(db), "%s %s", g.day ? g.day : "", g.date ? g.date : "");
  canvas.setFont(&fonts::DejaVu9);
  int dw = canvas.textWidth(db);
  int gap_l = title_right + 8;
  int gap_r = pill_left - 6;
  if (gap_r - gap_l >= dw) {
    canvas.setTextDatum(textdatum_t::middle_center);
    canvas.setTextColor(CYD_SAGE);
    canvas.drawString(db, (gap_l + gap_r) / 2, hy);
  }

  // 1. Teams Matchup Card
  cyd_detail_matchup_card(canvas, g, camera_y);

  // 2. Line Score Table Card
  cyd_detail_table(canvas, g, camera_y);

  // 3. Leader + Venue Card
  cyd_detail_info_card(canvas, g, camera_y);

  // 4. Footer hint
  canvas.drawFastHLine(0, cyd_sy(CYD_NAV_TOP, camera_y), CYD_W, CYD_HAIR);
  canvas.setFont(&fonts::Font0);
  canvas.setTextDatum(textdatum_t::middle_center);
  canvas.setTextColor(CYD_SUB);
  canvas.drawString("TAP TO RETURN", CYD_W / 2, cyd_sy(CYD_NAV_TOP + 11, camera_y));
}

inline CydTouchAction cyd_touch_hit(int x, int y, int* card_index) {
  if (card_index) *card_index = -1;
  if (g_cyd_detail_open) return CYD_TOUCH_DETAIL_BACK; // detail overlay: any tap returns
  if (x < 0 || x >= CYD_W || y < 0 || y >= CYD_H) return CYD_TOUCH_NONE;

  const int zone_w = (CYD_W * TOUCH_ZONE_FRACTION_PCT) / 100; // 80 px each side

  // Cards win over the side zones, except inside the bottom nav strip.
  if (y < CYD_NAV_TOP) {
    for (int i = 0; i < CYD_CARDS_PER_PAGE; ++i) {
      int cy = CYD_CARD_TOP + i * (CYD_CARD_H + CYD_CARD_GAP);
      if (y >= cy && y < cy + CYD_CARD_H &&
          x >= CYD_CARD_X && x < CYD_CARD_X + CYD_CARD_W) {
        if (card_index) *card_index = i;
        return CYD_TOUCH_CARD;
      }
    }
  }

  // Left / centre / right thirds (nav strip and header/gap fall-through).
  if (x < zone_w) return CYD_TOUCH_PREV;
  if (x >= CYD_W - zone_w) return CYD_TOUCH_NEXT;
  return CYD_TOUCH_TOGGLE;
}
