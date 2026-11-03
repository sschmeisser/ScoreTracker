#pragma once

#include <Arduino.h>
#include <LovyanGFX.hpp>
#include "all_team_logos.h"

// =============================================================================
// NFL SCORE TRACKER DATA & RENDERING FOR ESP32-2424S012 (240x240 CIRCULAR IPS)
// =============================================================================

struct NFLGame {
  const char* league;
  const char* day;
  const char* date;
  const char* status;
  const char* away_code;
  const char* away_name;
  const char* away_rec;
  uint16_t    away_score;
  uint8_t     away_q[4];
  uint8_t     away_rank;
  const char* home_code;
  const char* home_name;
  const char* home_rec;
  uint16_t    home_score;
  uint8_t     home_q[4];
  uint8_t     home_rank;
  const char* leader;
  const char* stadium;
};
typedef NFLGame SportGame;

// 15 NFL Games spanning the past 7 days (Thu, Fri, Sun, Mon)
static const NFLGame NFL_GAMES[] = {
  // 1. Thursday Night Football Kickoff
  {
    .day = "THU", .date = "SEP 17", .status = "FINAL",
    .away_code = "BAL", .away_name = "Ravens", .away_rec = "0-1", .away_score = 20, .away_q = {7, 3, 7, 3},
    .home_code = "KC",  .home_name = "Chiefs", .home_rec = "1-0", .home_score = 27, .home_q = {7, 10, 3, 7},
    .leader = "Mahomes: 291 YDS, 2 TD", .stadium = "GEHA Field at Arrowhead"
  },
  // 2. Friday International Special (São Paulo, Brazil)
  {
    .day = "FRI", .date = "SEP 18", .status = "FINAL",
    .away_code = "GB",  .away_name = "Packers", .away_rec = "0-1", .away_score = 29, .away_q = {6, 13, 7, 3},
    .home_code = "PHI", .home_name = "Eagles",  .home_rec = "1-0", .home_score = 34, .home_q = {0, 14, 14, 6},
    .leader = "Barkley: 109 YDS, 3 TD", .stadium = "Arena Corinthians (BR)"
  },
  // 3. Sunday Night Football Overtime Thriller
  {
    .day = "SUN", .date = "SEP 20", .status = "FINAL/OT",
    .away_code = "LAR", .away_name = "Rams",    .away_rec = "0-1", .away_score = 20, .away_q = {3, 0, 14, 3},
    .home_code = "DET", .home_name = "Lions",   .home_rec = "1-0", .home_score = 26, .home_q = {3, 14, 0, 3},
    .leader = "Montgomery: 91 YDS, 1 TD", .stadium = "Ford Field"
  },
  // 4. Monday Night Football
  {
    .day = "MON", .date = "SEP 21", .status = "FINAL",
    .away_code = "NYJ", .away_name = "Jets",    .away_rec = "0-1", .away_score = 19, .away_q = {7, 6, 0, 6},
    .home_code = "SF",  .home_name = "49ers",   .home_rec = "1-0", .home_score = 32, .home_q = {3, 13, 10, 6},
    .leader = "Mason: 147 YDS, 1 TD", .stadium = "Levi's Stadium"
  },
  // 5. Sunday Afternoon Rivalry
  {
    .day = "SUN", .date = "SEP 20", .status = "FINAL",
    .away_code = "DAL", .away_name = "Cowboys", .away_rec = "1-0", .away_score = 33, .away_q = {7, 13, 10, 3},
    .home_code = "CLE", .home_name = "Browns",  .home_rec = "0-1", .home_score = 17, .home_q = {3, 0, 7, 7},
    .leader = "Prescott: 179 YDS, 1 TD", .stadium = "Huntington Bank Field"
  },
  // 6. AFC South Showdown
  {
    .day = "SUN", .date = "SEP 20", .status = "FINAL",
    .away_code = "HOU", .away_name = "Texans",  .away_rec = "1-0", .away_score = 29, .away_q = {3, 9, 7, 10},
    .home_code = "IND", .home_name = "Colts",   .home_rec = "0-1", .home_score = 27, .home_q = {7, 0, 7, 13},
    .leader = "Mixon: 159 YDS, 1 TD", .stadium = "Lucas Oil Stadium"
  },
  // 7. High Scoring Shootout
  {
    .day = "SUN", .date = "SEP 20", .status = "FINAL",
    .away_code = "ARI", .away_name = "Cardinals", .away_rec = "0-1", .away_score = 28, .away_q = {7, 10, 0, 11},
    .home_code = "BUF", .home_name = "Bills",     .home_rec = "1-0", .home_score = 34, .home_q = {3, 7, 14, 10},
    .leader = "Allen: 232 YDS, 4 TOTAL TD", .stadium = "Highmark Stadium"
  },
  // 8. Florida Duel
  {
    .day = "SUN", .date = "SEP 20", .status = "FINAL",
    .away_code = "JAX", .away_name = "Jaguars", .away_rec = "0-1", .away_score = 17, .away_q = {7, 10, 0, 0},
    .home_code = "MIA", .home_name = "Dolphins",.home_rec = "1-0", .home_score = 20, .home_q = {0, 7, 0, 13},
    .leader = "Hill: 130 YDS, 1 TD", .stadium = "Hard Rock Stadium"
  },
  // 9. Soldier Field Comeback
  {
    .day = "SUN", .date = "SEP 20", .status = "FINAL",
    .away_code = "TEN", .away_name = "Titans",  .away_rec = "0-1", .away_score = 17, .away_q = {7, 10, 0, 0},
    .home_code = "CHI", .home_name = "Bears",   .home_rec = "1-0", .home_score = 24, .home_q = {0, 3, 7, 14},
    .leader = "Bears Defense: 2 TD Comeback", .stadium = "Soldier Field"
  },
  // 10. MetLife Blowout
  {
    .day = "SUN", .date = "SEP 20", .status = "FINAL",
    .away_code = "MIN", .away_name = "Vikings", .away_rec = "1-0", .away_score = 28, .away_q = {7, 7, 7, 7},
    .home_code = "NYG", .home_name = "Giants",  .home_rec = "0-1", .home_score = 6,  .home_q = {3, 0, 3, 0},
    .leader = "Darnold: 208 YDS, 2 TD", .stadium = "MetLife Stadium"
  },
  // 11. Defensive Battle
  {
    .day = "SUN", .date = "SEP 20", .status = "FINAL",
    .away_code = "PIT", .away_name = "Steelers",.away_rec = "1-0", .away_score = 18, .away_q = {3, 3, 3, 9},
    .home_code = "ATL", .home_name = "Falcons", .home_rec = "0-1", .home_score = 10, .home_q = {3, 7, 0, 0},
    .leader = "Boswell: 6/6 FG (3x 50+)", .stadium = "Mercedes-Benz Stadium"
  },
  // 12. Harbaugh Debut
  {
    .day = "SUN", .date = "SEP 20", .status = "FINAL",
    .away_code = "LV",  .away_name = "Raiders", .away_rec = "0-1", .away_score = 10, .away_q = {7, 0, 3, 0},
    .home_code = "LAC", .home_name = "Chargers",.home_rec = "1-0", .home_score = 22, .home_q = {3, 3, 3, 13},
    .leader = "Dobbins: 135 YDS, 1 TD", .stadium = "SoFi Stadium"
  },
  // 13. Pacific Northwest
  {
    .day = "SUN", .date = "SEP 20", .status = "FINAL",
    .away_code = "DEN", .away_name = "Broncos", .away_rec = "0-1", .away_score = 20, .away_q = {7, 6, 0, 7},
    .home_code = "SEA", .home_name = "Seahawks",.home_rec = "1-0", .home_score = 26, .home_q = {3, 6, 10, 7},
    .leader = "Walker III: 103 YDS, 1 TD", .stadium = "Lumen Field"
  },
  // 14. Tampa Bay Explosion
  {
    .day = "SUN", .date = "SEP 20", .status = "FINAL",
    .away_code = "WAS", .away_name = "Commanders",.away_rec = "0-1", .away_score = 20, .away_q = {0, 7, 7, 6},
    .home_code = "TB",  .home_name = "Buccaneers",.home_rec = "1-0", .home_score = 37, .home_q = {10, 6, 7, 14},
    .leader = "Mayfield: 289 YDS, 4 TD", .stadium = "Raymond James Stadium"
  },
  // 15. NFC South Clash
  {
    .day = "SUN", .date = "SEP 20", .status = "FINAL",
    .away_code = "CAR", .away_name = "Panthers", .away_rec = "0-1", .away_score = 10, .away_q = {0, 3, 7, 0},
    .home_code = "NO",  .home_name = "Saints",   .home_rec = "1-0", .home_score = 47, .home_q = {17, 13, 7, 10},
    .leader = "Carr: 200 YDS, 3 TD", .stadium = "Caesars Superdome"
  },
  // 16. AFC North Clash
  {
    .day = "SUN", .date = "SEP 20", .status = "FINAL",
    .away_code = "NE",  .away_name = "Patriots", .away_rec = "1-0", .away_score = 16, .away_q = {7, 3, 3, 3},
    .home_code = "CIN", .home_name = "Bengals",  .home_rec = "0-1", .home_score = 10, .home_q = {0, 0, 7, 3},
    .leader = "Stevenson: 120 YDS, 1 TD", .stadium = "Paycor Stadium"
  }
};

static const int TOTAL_NFL_GAMES = sizeof(NFL_GAMES) / sizeof(NFL_GAMES[0]);

// =============================================================================
// OFFICIAL NFL TEAM LOGO RENDERER (32x26 Crisp Broadcast Bitmaps)
// =============================================================================

inline void draw_nfl_team_logo(LGFX_Sprite& canvas, int cx, int cy, const char* code) {
  const uint16_t* bitmap = get_team_logo_bitmap("NFL", code);
  if (bitmap != nullptr) {
    int lx = cx - LOGO_WIDTH / 2;
    int ly = cy - LOGO_HEIGHT / 2;
    canvas.setSwapBytes(true);
    canvas.pushImage(lx, ly, LOGO_WIDTH, LOGO_HEIGHT, bitmap, (uint16_t)LOGO_CHROMA_KEY);
    canvas.setSwapBytes(false);
    return;
  }

  // Fallback badge with team code if unknown
  canvas.fillSmoothCircle(cx, cy, 12, 0x18C4);
  canvas.drawCircle(cx, cy, 12, 0x2946);
  canvas.setFont(&fonts::Font0);
  canvas.setTextDatum(textdatum_t::middle_center);
  canvas.setTextColor(0xAD75);
  char s[4] = { code[0], code[1], code[2], 0 };
  canvas.drawString(s, cx, cy);
}

// =============================================================================
// CALM MINIMALIST 3-ROW NFL SCORE TRACKER RENDERER
// ZEN PALETTE: DEEP FOREST MOSS & WARM SAND (Serene Botanical Night)
// =============================================================================
static const uint16_t ZEN_BG              = 0x1103; // (22, 32, 27) Deep Forest Moss (Tranquil, zero backlight glare)
static const uint16_t ZEN_BEZEL           = 0x29C6; // (42, 58, 48) Deep Moss Bezel
static const uint16_t ZEN_HAIRLINE        = 0x21A5; // (38, 54, 45) Subtle Pine Hairline Divider

// Status Pills
static const uint16_t ZEN_PILL_FINAL_BG     = 0x1964; // (30, 44, 37) Deep Botanical Fill
static const uint16_t ZEN_PILL_FINAL_BORDER = 0x3A88; // (58, 80, 68) Muted Moss Border
static const uint16_t ZEN_PILL_FINAL_TEXT   = 0x9DD4; // (154, 184, 164) Pale Eucalyptus Text

static const uint16_t ZEN_PILL_LIVE_BG      = 0x39E4; // Deep Muted Amber Moss
static const uint16_t ZEN_PILL_LIVE_BORDER  = 0x6AE7; // Muted Ochre
static const uint16_t ZEN_PILL_LIVE_TEXT    = 0xDF78; // Soft Sand

static const uint16_t ZEN_PILL_OT_BG        = 0x21A5; // Deep Moss Slate
static const uint16_t ZEN_PILL_OT_BORDER    = 0x4A89; // Muted Lichen Border
static const uint16_t ZEN_PILL_OT_TEXT      = 0xBDD4; // Pale Sage

// Typography & Scores
static const uint16_t ZEN_SCORE_WIN       = 0xDEB8; // (223, 213, 198) Warm Sand / Oat (Soft warm glow, zero glare)
static const uint16_t ZEN_SCORE_LOSE      = 0x84B0; // (128, 148, 134) Muted Calm Sage
static const uint16_t ZEN_SCORE_UPCOMING  = 0x5AE9; // Gentle Moss Fog
static const uint16_t ZEN_TEAM_CODE       = 0x9553; // (145, 168, 152) Soft Muted Sage
static const uint16_t ZEN_CENTER_DOT      = 0x4AEA; // (72, 94, 80) Muted Forest Dot

// Subline Details (Records, Dates, Separator Dots)
static const uint16_t ZEN_SUBLINE_TEXT    = 0x744F; // (118, 138, 124) Earthy Soft Eucalyptus
static const uint16_t ZEN_SUBLINE_DOT     = 0x3A88; // (58, 80, 68) Subtle Moss Dot

// Bottom Pager Dots
static const uint16_t ZEN_PAGER_ACTIVE    = 0xDEB8; // (223, 213, 198) Warm Sand Dot
static const uint16_t ZEN_PAGER_INACTIVE  = 0x29C6; // (42, 58, 48) Deep Moss Dot

inline void draw_team_badge_or_logo(LGFX_Sprite& canvas, int cx, int cy, const char* code, const char* league, int rank = 0) {
  const uint16_t* bmp = get_team_logo_bitmap(league, code);
  if (bmp != nullptr) {
    int lx = cx - LOGO_WIDTH / 2;
    int ly = cy - LOGO_HEIGHT / 2;
    canvas.setSwapBytes(true);
    canvas.pushImage(lx, ly, LOGO_WIDTH, LOGO_HEIGHT, bmp, (uint16_t)LOGO_CHROMA_KEY);
    canvas.setSwapBytes(false);
    return;
  }

  // Circular monogram badge for teams across NHL, NBA, MLS, CFB
  uint16_t badge_bg = 0x1964;     // Muted Botanical Slate
  uint16_t badge_border = 0x3A88; // Muted Moss Border
  uint16_t badge_text = 0xDEB8;   // Warm Sand
  char badge_str[8] = {0};

  if (league) {
    if (strcmp(league, "NHL") == 0) {
      badge_bg = 0x18C8;     // Deep Ice Slate
      badge_border = 0x3A8E; // Cool Slate Border
      badge_text = 0xCE79;   // Ice Sand
    } else if (strcmp(league, "NBA") == 0) {
      badge_bg = 0x2964;     // Deep Ochre Moss
      badge_border = 0x5AE7; // Amber Moss Border
      badge_text = 0xDF78;   // Soft Sand
    } else if (strcmp(league, "MLS") == 0) {
      badge_bg = 0x11E5;     // Pine Green
      badge_border = 0x2A67; // Lichen Green Border
      badge_text = 0xBDD4;   // Pale Sage
    } else if (strcmp(league, "CFB") == 0) {
      if (rank > 0 && rank <= 25) {
        badge_bg = 0x2185;     // Deep Navy/Moss bg
        badge_border = 0xDEB8; // Collegiate Gold border
        badge_text = 0xDEB8;   // Collegiate Gold text
        snprintf(badge_str, sizeof(badge_str), "#%d", rank);
      } else {
        badge_bg = 0x2185;     // Deep Earth Slate
        badge_border = 0x4A88; // Moss Earth Border
        badge_text = 0xDEB8;   // Warm Oat
      }
    }
  }

  if (badge_str[0] == '\0') {
    if (code) {
      strncpy(badge_str, code, 3);
    }
  }

  canvas.fillCircle(cx, cy, 10, badge_bg);
  canvas.drawCircle(cx, cy, 10, badge_border);
  canvas.setFont(&fonts::Font0);
  canvas.setTextDatum(textdatum_t::middle_center);
  canvas.setTextColor(badge_text);
  canvas.drawString(badge_str, cx, cy + 1);
}

inline void draw_zen_game_row(LGFX_Sprite& canvas, const NFLGame& g, int cy_row, int cx_away = 40, int cx_home = 200) {
  if (cy_row < -45 || cy_row > 285) return;
  const int cx = 120;

  // 1. Subtle, Muted Status Pill (Centered at cx, py = cy_row - 22)
  bool is_ot = (strstr(g.status, "OT") != NULL || strstr(g.status, "SO") != NULL || strstr(g.status, "PK") != NULL || strstr(g.status, "ET") != NULL);
  bool is_final = (strstr(g.status, "FINAL") != NULL || strstr(g.status, "FT") != NULL);
  bool is_live = !is_final && (strstr(g.status, "LIVE") != NULL || strstr(g.status, "Q1") != NULL ||
                  strstr(g.status, "Q2") != NULL || strstr(g.status, "Q3") != NULL ||
                  strstr(g.status, "Q4") != NULL || strstr(g.status, "HALFTIME") != NULL ||
                  strstr(g.status, "HT") != NULL || strstr(g.status, "1H") != NULL ||
                  strstr(g.status, "2H") != NULL || strstr(g.status, "P1") != NULL ||
                  strstr(g.status, "P2") != NULL || strstr(g.status, "P3") != NULL ||
                  strstr(g.status, "1st") != NULL || strstr(g.status, "2nd") != NULL ||
                  strstr(g.status, "3rd") != NULL || strstr(g.status, "4th") != NULL ||
                  strstr(g.status, "Half") != NULL ||
                  strstr(g.status, "'") != NULL || strstr(g.status, "MIN") != NULL);
  bool is_upcoming = (!is_final && !is_live && g.away_score == 0 && g.home_score == 0);
  bool away_win = (!is_upcoming && g.away_score > g.home_score);
  bool home_win = (!is_upcoming && g.home_score > g.away_score);

  uint16_t badge_bg, badge_border, badge_text;
  if (is_live) {
    badge_bg     = ZEN_PILL_LIVE_BG;
    badge_border = ZEN_PILL_LIVE_BORDER;
    badge_text   = ZEN_PILL_LIVE_TEXT;
  } else if (is_ot) {
    badge_bg     = ZEN_PILL_OT_BG;
    badge_border = ZEN_PILL_OT_BORDER;
    badge_text   = ZEN_PILL_OT_TEXT;
  } else {
    badge_bg     = ZEN_PILL_FINAL_BG;
    badge_border = ZEN_PILL_FINAL_BORDER;
    badge_text   = ZEN_PILL_FINAL_TEXT;
  }

  // Format pill string with serene league tag (using ASCII pipe separator)
  char pill_str[36];
  if (g.league && strlen(g.league) > 0) {
    snprintf(pill_str, sizeof(pill_str), "%s | %s", g.league, g.status);
  } else {
    strncpy(pill_str, g.status, sizeof(pill_str) - 1);
  }

  canvas.setFont(&fonts::Font0);
  int text_w = canvas.textWidth(pill_str);
  int pill_w = text_w + 10;
  if (pill_w < 34) pill_w = 34;
  int py = cy_row - 22;

  canvas.fillRoundRect(cx - pill_w / 2, py, pill_w, 11, 3, badge_bg);
  canvas.drawRoundRect(cx - pill_w / 2, py, pill_w, 11, 3, badge_border);
  canvas.setTextDatum(textdatum_t::middle_center);
  canvas.setTextColor(badge_text);
  canvas.drawString(pill_str, cx, py + 6);

  // 2. Teams, Logos, and Scores (Centered at cy_row + 1)
  // Away Team (Left)
  draw_team_badge_or_logo(canvas, cx_away, cy_row + 1, g.away_code, g.league, g.away_rank);

  char sc_a[8];
  if (is_upcoming) {
    strcpy(sc_a, "-");
  } else {
    itoa(g.away_score, sc_a, 10);
  }

  char sc_h[8];
  if (is_upcoming) {
    strcpy(sc_h, "-");
  } else {
    itoa(g.home_score, sc_h, 10);
  }

  canvas.setFont(&fonts::FreeSansBold12pt7b);
  int sc_a_w = canvas.textWidth(sc_a);
  int sc_h_w = canvas.textWidth(sc_h);

  canvas.setFont(&fonts::DejaVu9);
  int a_w = canvas.textWidth(g.away_code);
  int h_w = canvas.textWidth(g.home_code);

  bool use_font0_away = false;
  bool use_font0_home = false;
  int sc_a_x = 113;
  int sc_h_x = 127;

  if ((cx_away + 20 + a_w + 2) > (sc_a_x - sc_a_w)) {
    sc_a_x = 116;
    if ((cx_away + 20 + a_w + 2) > (sc_a_x - sc_a_w)) {
      use_font0_away = true;
    }
  }

  if ((sc_h_x + sc_h_w + 2) > (cx_home - 20 - h_w)) {
    sc_h_x = 124;
    if ((sc_h_x + sc_h_w + 2) > (cx_home - 20 - h_w)) {
      use_font0_home = true;
    }
  }

  canvas.setFont(use_font0_away ? (const lgfx::IFont*)&fonts::Font0 : (const lgfx::IFont*)&fonts::DejaVu9);
  canvas.setTextDatum(textdatum_t::middle_left);
  canvas.setTextColor(ZEN_TEAM_CODE);
  canvas.drawString(g.away_code, cx_away + 20, cy_row + 1);

  canvas.setFont(&fonts::FreeSansBold12pt7b);
  canvas.setTextDatum(textdatum_t::middle_right);
  canvas.setTextColor(away_win ? ZEN_SCORE_WIN : (is_upcoming ? ZEN_SCORE_UPCOMING : ZEN_SCORE_LOSE));
  canvas.drawString(sc_a, sc_a_x, cy_row + 1);

  // Quiet Center Dot
  canvas.fillCircle(cx, cy_row + 1, 1, ZEN_CENTER_DOT);

  // Home Team (Right)
  canvas.setFont(&fonts::FreeSansBold12pt7b);
  canvas.setTextDatum(textdatum_t::middle_left);
  canvas.setTextColor(home_win ? ZEN_SCORE_WIN : (is_upcoming ? ZEN_SCORE_UPCOMING : ZEN_SCORE_LOSE));
  canvas.drawString(sc_h, sc_h_x, cy_row + 1);

  canvas.setFont(use_font0_home ? (const lgfx::IFont*)&fonts::Font0 : (const lgfx::IFont*)&fonts::DejaVu9);
  canvas.setTextDatum(textdatum_t::middle_right);
  canvas.setTextColor(ZEN_TEAM_CODE);
  canvas.drawString(g.home_code, cx_home - 20, cy_row + 1);

  draw_team_badge_or_logo(canvas, cx_home, cy_row + 1, g.home_code, g.league, g.home_rank);

  // 3. Sub-detail Line (Records & Date, Centered at cy_row + 18)
  int sub_y = cy_row + 18;
  canvas.setFont(&fonts::Font0);
  canvas.setTextColor(ZEN_SUBLINE_TEXT);

  // Format away record (incorporating collegiate rank cleanly if CFB)
  char away_rec_buf[24];
  if (g.league != nullptr && strcmp(g.league, "CFB") == 0 && g.away_rank > 0 && g.away_rank <= 25) {
    if (g.away_rec && strlen(g.away_rec) > 0) {
      snprintf(away_rec_buf, sizeof(away_rec_buf), "#%d %s", g.away_rank, g.away_rec);
    } else {
      snprintf(away_rec_buf, sizeof(away_rec_buf), "#%d", g.away_rank);
    }
  } else {
    strncpy(away_rec_buf, g.away_rec ? g.away_rec : "", sizeof(away_rec_buf) - 1);
    away_rec_buf[sizeof(away_rec_buf) - 1] = '\0';
  }

  // Format home record (incorporating collegiate rank cleanly if CFB)
  char home_rec_buf[24];
  if (g.league != nullptr && strcmp(g.league, "CFB") == 0 && g.home_rank > 0 && g.home_rank <= 25) {
    if (g.home_rec && strlen(g.home_rec) > 0) {
      snprintf(home_rec_buf, sizeof(home_rec_buf), "#%d %s", g.home_rank, g.home_rec);
    } else {
      snprintf(home_rec_buf, sizeof(home_rec_buf), "#%d", g.home_rank);
    }
  } else {
    strncpy(home_rec_buf, g.home_rec ? g.home_rec : "", sizeof(home_rec_buf) - 1);
    home_rec_buf[sizeof(home_rec_buf) - 1] = '\0';
  }

  canvas.setTextDatum(textdatum_t::middle_left);
  canvas.drawString(away_rec_buf, cx_away - 6, sub_y);

  canvas.setTextDatum(textdatum_t::middle_right);
  canvas.drawString(home_rec_buf, cx_home + 6, sub_y);

  char date_buf[24];
  if (g.day && strlen(g.day) > 0 && g.date && strlen(g.date) > 0) {
    snprintf(date_buf, sizeof(date_buf), "%s %s", g.day, g.date);
    if (canvas.textWidth(date_buf) > 44) {
      strncpy(date_buf, g.date, sizeof(date_buf) - 1);
      date_buf[sizeof(date_buf) - 1] = '\0';
    }
  } else if (g.date && strlen(g.date) > 0) {
    strncpy(date_buf, g.date, sizeof(date_buf) - 1);
    date_buf[sizeof(date_buf) - 1] = '\0';
  } else if (g.day && strlen(g.day) > 0) {
    strncpy(date_buf, g.day, sizeof(date_buf) - 1);
    date_buf[sizeof(date_buf) - 1] = '\0';
  } else {
    date_buf[0] = '\0';
  }

  canvas.setTextDatum(textdatum_t::middle_center);
  canvas.drawString(date_buf, cx, sub_y);

  int away_rec_w = canvas.textWidth(away_rec_buf);
  int home_rec_w = canvas.textWidth(home_rec_buf);
  if (away_rec_w < 44 && home_rec_w < 44 && strlen(date_buf) > 0) {
    canvas.fillCircle(94, sub_y, 1, ZEN_SUBLINE_DOT);
    canvas.fillCircle(146, sub_y, 1, ZEN_SUBLINE_DOT);
  }
}

inline void draw_zen_score_tracker(LGFX_Sprite& canvas, 
                                   int page_idx, 
                                   int offset_y = 0, 
                                   int next_page_idx = 0, 
                                   bool is_transitioning = false, 
                                   const NFLGame* games = nullptr, 
                                   int total_games = 0) {
  const NFLGame* game_array = (games != nullptr && total_games > 0) ? games : NFL_GAMES;
  int games_count = (games != nullptr && total_games > 0) ? total_games : TOTAL_NFL_GAMES;
  if (games_count <= 0) return;

  int total_pages = (games_count + 2) / 3;
  if (total_pages < 1) total_pages = 1;
  page_idx = (page_idx % total_pages + total_pages) % total_pages;
  next_page_idx = (next_page_idx % total_pages + total_pages) % total_pages;

  const int cx = 120;
  const int cy = 120;

  // 1. Soft Warm Linen Canvas & Outer Bezel (Drawn BEFORE game rows)
  canvas.fillScreen(ZEN_BG);
  canvas.drawCircle(cx, cy, 119, ZEN_BEZEL);

  // 2. Current Triad Rendering (3 rows at cy = 54, 120, 186)
  int g1_idx = (page_idx * 3) % games_count;
  int g2_idx = (page_idx * 3 + 1) % games_count;
  int g3_idx = (page_idx * 3 + 2) % games_count;

  draw_zen_game_row(canvas, game_array[g1_idx], 54 - offset_y, 46, 194);
  draw_zen_game_row(canvas, game_array[g2_idx], 120 - offset_y, 40, 200);
  draw_zen_game_row(canvas, game_array[g3_idx], 186 - offset_y, 46, 194);

  // Serene hairline separators between the 3 rows
  int div1_y = 87 - offset_y;
  if (div1_y >= 0 && div1_y < 240) {
    canvas.drawFastHLine(50, div1_y, 140, ZEN_HAIRLINE);
  }
  int div2_y = 153 - offset_y;
  if (div2_y >= 0 && div2_y < 240) {
    canvas.drawFastHLine(50, div2_y, 140, ZEN_HAIRLINE);
  }

  // 3. Next Triad Rendering (during smooth liquid transition)
  if (is_transitioning && offset_y != 0) {
    int next_g1_idx = (next_page_idx * 3) % games_count;
    int next_g2_idx = (next_page_idx * 3 + 1) % games_count;
    int next_g3_idx = (next_page_idx * 3 + 2) % games_count;

    draw_zen_game_row(canvas, game_array[next_g1_idx], 54 + 240 - offset_y, 46, 194);
    draw_zen_game_row(canvas, game_array[next_g2_idx], 120 + 240 - offset_y, 40, 200);
    draw_zen_game_row(canvas, game_array[next_g3_idx], 186 + 240 - offset_y, 46, 194);

    int div1_next = 87 + 240 - offset_y;
    if (div1_next >= 0 && div1_next < 240) {
      canvas.drawFastHLine(50, div1_next, 140, ZEN_HAIRLINE);
    }
    int div2_next = 153 + 240 - offset_y;
    if (div2_next >= 0 && div2_next < 240) {
      canvas.drawFastHLine(50, div2_next, 140, ZEN_HAIRLINE);
    }
  }

  // 4. Fixed Serene Accents (Outer Bezel is drawn at start; Quiet Dot Pager here)

  // Dot Pager at Bottom (y = 227)
  int active_dot = is_transitioning ? next_page_idx : page_idx;
  int max_dots = total_pages > 8 ? 8 : total_pages;
  int dot_spacing = 8;
  int start_dot_x = cx - (max_dots * dot_spacing) / 2 + 4;
  for (int i = 0; i < max_dots; i++) {
    int dx = start_dot_x + i * dot_spacing;
    int dy = 227;
    if (i == (active_dot % max_dots)) {
      canvas.fillCircle(dx, dy, 2, ZEN_PAGER_ACTIVE); // Deep sage dot
    } else {
      canvas.fillCircle(dx, dy, 1, ZEN_PAGER_INACTIVE); // Soft warm stone dot
    }
  }
}

// Backward-compatible wrapper
inline void draw_nfl_score_card(LGFX_Sprite& canvas, int game_idx, uint32_t elapsed_ms, const NFLGame* games = nullptr, int total_games = 0) {
  int page_idx = game_idx / 3;
  draw_zen_score_tracker(canvas, page_idx, 0, page_idx, false, games, total_games);
}

inline void draw_sync_loading_screen(LGFX_Sprite& canvas, int loaded, int total) {
  const int cx = 120;
  const int cy = 120;
  
  canvas.fillScreen(ZEN_BG);
  canvas.drawCircle(cx, cy, 119, ZEN_BEZEL);

  // Outer circular progress track (r = 90)
  canvas.drawCircle(cx, cy, 90, ZEN_BEZEL);
  
  // Progress arc
  if (total > 0 && loaded > 0) {
    float frac = (float)loaded / (float)total;
    if (frac > 1.0f) frac = 1.0f;
    int deg = (int)(frac * 360.0f);
    for (int r = 88; r <= 92; ++r) {
      canvas.drawArc(cx, cy, r, r, -90, -90 + deg, ZEN_SCORE_WIN);
    }
  }

  // Header
  canvas.setFont(&fonts::DejaVu9);
  canvas.setTextColor(ZEN_TEAM_CODE);
  canvas.setTextDatum(textdatum_t::middle_center);
  canvas.drawString("NFL SCORE TRACKER", cx, 66);

  // Center Status Pill
  int py = 88;
  canvas.fillRoundRect(cx - 52, py, 104, 18, 4, ZEN_PILL_LIVE_BG);
  canvas.drawRoundRect(cx - 52, py, 104, 18, 4, ZEN_PILL_LIVE_BORDER);
  canvas.setFont(&fonts::Font0);
  canvas.setTextColor(ZEN_PILL_LIVE_TEXT);
  canvas.drawString("SYNCING SCORES", cx, py + 9);

  // Big percentage / count in warm sand
  char count_buf[32];
  if (total > 0) {
    int pct = (loaded * 100) / total;
    snprintf(count_buf, sizeof(count_buf), "%d%%", pct);
  } else {
    strcpy(count_buf, "...");
  }
  canvas.setFont(&fonts::FreeSansBold12pt7b);
  canvas.setTextColor(ZEN_SCORE_WIN);
  canvas.drawString(count_buf, cx, 134);

  // Subtext
  char sub_buf[32];
  if (total > 0) {
    snprintf(sub_buf, sizeof(sub_buf), "%d of %d games loaded", loaded, total);
  } else {
    strcpy(sub_buf, "Connected to laptop");
  }
  canvas.setFont(&fonts::Font0);
  canvas.setTextColor(ZEN_SUBLINE_TEXT);
  canvas.drawString(sub_buf, cx, 162);
}

