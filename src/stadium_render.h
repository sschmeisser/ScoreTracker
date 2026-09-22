#pragma once
#include <Arduino.h>
#include <math.h>
#include "LGFX_config.h"

// ============================================================================
// Stadium Surfaces & Environmental Lighting (Phase 1)
// Authentic 16-Bit Retro Arcade Textures & Lighting for ESP32-C3 & LovyanGFX
// ============================================================================

#ifndef STADIUM_RENDER_CONSTANTS
#define STADIUM_RENDER_CONSTANTS

enum SportType {
  SPORT_SOCCER = 0,
  SPORT_HOCKEY = 1,
  SPORT_BASKETBALL = 2,
  SPORT_BASEBALL = 3
};

#ifndef PITCH_MIN_X
const int PITCH_MIN_X = 14;
const int PITCH_MAX_X = 226;
const int PITCH_MIN_Y = 28;
const int PITCH_MAX_Y = 230;

const int GOAL_Y_TOP = 104;
const int GOAL_Y_BOT = 154;
const int GOAL_LEFT_X = 22;
const int GOAL_RIGHT_X = 218;
#endif

#endif // STADIUM_RENDER_CONSTANTS

// ============================================================================
// Multi-Tone 16-Bit RGB565 Color Palette Definitions
// ============================================================================

// General Palette
#ifndef COL_BLACK
#define COL_BLACK       0x0000
#define COL_WHITE       0xFFFF
#define COL_GOLD        0xFFE0
#define COL_SHADOW      0x0A22
#endif

// 1. Soccer Pitch Multi-Tone Palette
#define SOCCER_GRASS_A1   0x24C5  // Rich vibrant emerald green (mowed stripe A base)
#define SOCCER_GRASS_A2   0x2D47  // Emerald grass highlight blade
#define SOCCER_GRASS_A3   0x1CA3  // Emerald grass depth shadow
#define SOCCER_GRASS_B1   0x1C23  // Deep stadium forest green (mowed stripe B base)
#define SOCCER_GRASS_B2   0x2484  // Forest grass highlight blade
#define SOCCER_GRASS_B3   0x1402  // Forest grass depth shadow
#define SOCCER_SHEEN      0x3DE9  // Roller mower reflective grass sheen
#define SOCCER_DIRT_1     0x8A42  // Goal mouth worn clay dirt core
#define SOCCER_DIRT_2     0x69C1  // Goal mouth dark scuffed wet dirt
#define SOCCER_DIRT_3     0x9B64  // Goal mouth dry chalky dust patch
#define SOCCER_WEAR_GRS   0x3B03  // Scuffed, trampled yellow-olive grass fringe
#define SOCCER_CHALK      0xFFFF  // Core chalk line (pure white)
#define SOCCER_CHALK_AA   0x64C7  // Feathered anti-aliased chalk dust edge
#define SOCCER_NET        0xC618  // Goal net cords
#define SOCCER_NET_SHD    0x2965  // Goal net back shadow depth

// 2. Ice Hockey Rink Multi-Tone Palette
#define HOCKEY_ICE_A      0xFFFF  // Brilliant pure white glossy ice sheet
#define HOCKEY_ICE_B      0xFFFF  // Solid uniform ice (NO STRIPES)
#define HOCKEY_ICE_CUT    0xFFFF  // Fresh frosted skate blade groove
#define HOCKEY_ICE_SHD    0xCE59  // Subtle neutral silver skate scratch shadow
#define HOCKEY_CREASE     0x9E3D  // Regulation ice-blue goal crease paint
#define HOCKEY_RED_LINE   0xD904  // Regulation vivid hockey red line / crease border
#define HOCKEY_BLU_LINE   0x2B7D  // Regulation deep hockey blue line
#define HOCKEY_BOARD_W    0xF7BE  // High-impact polyethylene dasher board white
#define HOCKEY_BOARD_Y    0xFFE0  // Bottom safety kickplate yellow
#define HOCKEY_BOARD_R    0xD904  // Top dasher cap rail red
#define HOCKEY_GLASS_HI   0xFFFF  // Acrylic glass gloss specular highlight streak
#define HOCKEY_GLASS_CY   0x8E1E  // Acrylic reflection cyan ambient hue
#define HOCKEY_NET_MESH   0xE73C  // Goal netting silver-white mesh

// 3. Basketball Court Multi-Tone Palette (Smooth Low-Noise Matte Maple)
#define BASKET_MAPLE_A    0xB58B  // Warm golden maple plank (rich contrast with white lines)
#define BASKET_MAPLE_B    0xAD49  // Subdued secondary maple plank
#define BASKET_MAPLE_SEAM 0x9CE7  // Gentle 1px horizontal plank seam
#define BASKET_KEY_RED    0x9882  // Deep arcade crimson key paint lane
#define BASKET_KEY_SHD    0x6800  // Key paint lane subtle perimeter shadow
#define BASKET_LINE       0xFFFF  // Boundary & 3-point lines (pure crisp white)
#define BASKET_HOOP_RIM   0xF960  // Heavy-duty breakaway rim orange
#define BASKET_NET        0xFFFF  // White braided nylon hoop net
#define BASKET_GLOSS      0xD6B3  // Subtle soft arena wood sheen

// 4. Baseball Field Multi-Tone Palette
#define BASEBALL_TURF_A   0x24C5  // Outfield manicured emerald grass
#define BASEBALL_TURF_B   0x1C23  // Outfield deep green mowing stripe
#define BASEBALL_TURF_S   0x2D47  // Radial mowing stripe sheen
#define BASEBALL_CLAY_A   0xD3C6  // Warm terracotta infield clay dirt base
#define BASEBALL_CLAY_B   0xBA63  // Medium clay dirt & basepath loam
#define BASEBALL_CLAY_C   0x9A42  // Deep damp clay dirt shadow
#define BASEBALL_CLAY_D   0x7201  // Infield clay pebble & rake furrow
#define BASEBALL_TRACK    0xA262  // Outfield warning track crushed volcanic clay
#define BASEBALL_FENCE    0x11C5  // Padded outfield wall stadium green
#define BASEBALL_PAD_Y    0xFFE0  // Outfield fence top line safety yellow
#define BASEBALL_BASE_W   0xFFFF  // 1st/2nd/3rd base canvas top
#define BASEBALL_BASE_S   0xCE7D  // Base bag 3D shaded edge
#define BASEBALL_SHADOW   0x2945  // 3D base drop-shadow onto clay dirt
#define BASEBALL_CHALK    0xFFFF  // Foul line & batter's box chalk
#define BASEBALL_BANNER_R 0xD882  // Fence sponsorship banner red
#define BASEBALL_BANNER_B 0x1A4F  // Fence sponsorship banner blue

// ============================================================================
// 3. Fast Radial Stadium Floodlight Vignette & Ambient Occlusion
// ============================================================================
// Smoothly darkens the outer 18 pixels near the 240x240 circular bezel
// and keeps the center field brilliantly lit, framing the round display.

static uint16_t vignette_lut[4000];
static bool vignette_init = false;

static inline uint16_t scale_rgb565(uint16_t c, uint32_t factor) {
  uint32_t r = ((c >> 11) * factor) >> 8;
  uint32_t g = (((c >> 5) & 0x3F) * factor) >> 8;
  uint32_t b = ((c & 0x1F) * factor) >> 8;
  if (r > 31) r = 31;
  if (g > 63) g = 63;
  if (b > 31) b = 31;
  return (uint16_t)((r << 11) | (g << 5) | b);
}

static void init_vignette_lut() {
  if (vignette_init) return;
  // Radius 102^2 = 10404 (center boundary: 100% full light)
  // Radius 120^2 = 14400 (bezel boundary: 0% light / black rim)
  for (int r2 = 10404; r2 <= 14400; r2++) {
    float r = sqrtf((float)r2);
    float t = (120.0f - r) / 18.0f; // 1.0 at r=102, 0.0 at r=120
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    // Cubic Hermite smoothstep for natural stadium floodlight falloff
    float smooth = t * t * (3.0f - 2.0f * t);
    vignette_lut[r2 - 10404] = (uint16_t)(smooth * 256.0f);
  }
  vignette_init = true;
}

static inline void apply_circular_vignette(LGFX_Sprite &canvas) {
  if (!vignette_init) init_vignette_lut();
  uint16_t* buf = (uint16_t*)canvas.getBuffer();
  if (!buf) return;

  for (int y = 0; y < 240; y++) {
    int dy = y - 120;
    int dy2 = dy * dy;

    // Scanlines completely outside the 240 circular bezel
    if (dy2 >= 14400) {
      memset(&buf[y * 240], 0, 240 * sizeof(uint16_t));
      continue;
    }

    int x_outer = (int)sqrtf((float)(14400 - dy2));
    int left_outer = 120 - x_outer;
    int right_outer = 120 + x_outer;

    // Zero-out pixels outside circular bezel on left & right
    if (left_outer > 0) {
      memset(&buf[y * 240], 0, left_outer * sizeof(uint16_t));
    }
    if (right_outer < 239) {
      memset(&buf[y * 240 + right_outer + 1], 0, (239 - right_outer) * sizeof(uint16_t));
    }

    // Vignette ring between radius 102 and 120
    if (dy2 < 10404) {
      int x_inner = (int)sqrtf((float)(10404 - dy2));
      int left_inner = 120 - x_inner;
      int right_inner = 120 + x_inner;

      // Left arc: [left_outer, left_inner]
      for (int x = left_outer; x <= left_inner; x++) {
        int dx = x - 120;
        int r2 = dx * dx + dy2;
        if (r2 >= 10404 && r2 <= 14400) {
          uint32_t factor = vignette_lut[r2 - 10404];
          buf[y * 240 + x] = scale_rgb565(buf[y * 240 + x], factor);
        }
      }
      // Right arc: [right_inner, right_outer]
      for (int x = right_inner; x <= right_outer; x++) {
        int dx = x - 120;
        int r2 = dx * dx + dy2;
        if (r2 >= 10404 && r2 <= 14400) {
          uint32_t factor = vignette_lut[r2 - 10404];
          buf[y * 240 + x] = scale_rgb565(buf[y * 240 + x], factor);
        }
      }
      // Pixels between left_inner and right_inner remain 100% brilliantly lit!
    } else {
      // Top and bottom cap lines where entire visible chord is in the vignette zone
      for (int x = left_outer; x <= right_outer; x++) {
        int dx = x - 120;
        int r2 = dx * dx + dy2;
        if (r2 >= 10404 && r2 <= 14400) {
          uint32_t factor = vignette_lut[r2 - 10404];
          buf[y * 240 + x] = scale_rgb565(buf[y * 240 + x], factor);
        }
      }
    }
  }
}

// ============================================================================
// 1. SOCCER PITCH RENDERING
// ============================================================================
// Lush 2-tone green grass with alternating mowed lawn stripes, subtle cross-hatch
// stippling, goal mouth grass wear, anti-aliased chalk boundary lines & net mesh.

static void render_soccer_pitch(LGFX_Sprite &canvas) {
  canvas.fillScreen(COL_BLACK);
  uint16_t* buf = (uint16_t*)canvas.getBuffer();
  int cx = 120, cy = 120;
  int r = 117;

  // A. Grass Pitch with Alternating Mowed Stripes & Cross-Hatch Turf Stippling
  for (int y = PITCH_MIN_Y; y <= PITCH_MAX_Y; y++) {
    int dy = y - cy;
    int max_w = (int)sqrtf((float)(r * r - dy * dy));
    int x1 = cx - max_w;
    int x2 = cx + max_w;
    if (x1 < PITCH_MIN_X) x1 = PITCH_MIN_X;
    if (x2 > PITCH_MAX_X) x2 = PITCH_MAX_X;
    if (x1 >= x2) continue;

    int stripe = ((y - PITCH_MIN_Y) / 18) % 2;
    uint16_t c_base = (stripe == 0) ? SOCCER_GRASS_A1 : SOCCER_GRASS_B1;
    uint16_t c_hi   = (stripe == 0) ? SOCCER_GRASS_A2 : SOCCER_GRASS_B2;
    uint16_t c_sh   = (stripe == 0) ? SOCCER_GRASS_A3 : SOCCER_GRASS_B3;

    // Roller mower sheen line at boundary of stripes
    bool is_sheen_line = ((y - PITCH_MIN_Y) % 18 == 0);

    for (int x = x1; x <= x2; x++) {
      if (is_sheen_line && (x & 1)) {
        buf[y * 240 + x] = SOCCER_SHEEN;
        continue;
      }
      // Cross-hatch turf dithering for realistic blade texture
      uint8_t h = ((x * 19 + y * 41) ^ (x >> 1) ^ (y >> 2)) & 7;
      if (h == 0) {
        buf[y * 240 + x] = c_hi;
      } else if (h == 7) {
        buf[y * 240 + x] = c_sh;
      } else {
        buf[y * 240 + x] = c_base;
      }
    }
  }

  // B. Goal Mouth Grass Wear (Realistic goal mouth worn clay dirt & trampled turf)
  // Left Goal Mouth (centered at x=28, y=129)
  for (int dy = -16; dy <= 16; dy++) {
    int py = 129 + dy;
    if (py < PITCH_MIN_Y || py > PITCH_MAX_Y) continue;
    int hw = (int)sqrtf((float)(16 * 16 - dy * dy)) * 14 / 16;
    for (int dx = -hw; dx <= hw; dx++) {
      int px = 28 + dx;
      if (px < PITCH_MIN_X || px > PITCH_MAX_X) continue;
      float d = sqrtf((float)(dx * dx * 1.4f + dy * dy));
      if (d < 6.0f) {
        buf[py * 240 + px] = ((px ^ py) & 1) ? SOCCER_DIRT_1 : SOCCER_DIRT_2;
      } else if (d < 11.0f) {
        uint8_t h = (px * 7 + py * 13) & 3;
        buf[py * 240 + px] = (h == 0) ? SOCCER_DIRT_3 : ((h == 1) ? SOCCER_DIRT_1 : SOCCER_WEAR_GRS);
      } else if (d < 16.0f) {
        if (((px * 11 + py * 23) & 3) == 0) {
          buf[py * 240 + px] = SOCCER_WEAR_GRS;
        }
      }
    }
  }

  // Right Goal Mouth (centered at x=212, y=129)
  for (int dy = -16; dy <= 16; dy++) {
    int py = 129 + dy;
    if (py < PITCH_MIN_Y || py > PITCH_MAX_Y) continue;
    int hw = (int)sqrtf((float)(16 * 16 - dy * dy)) * 14 / 16;
    for (int dx = -hw; dx <= hw; dx++) {
      int px = 212 + dx;
      if (px < PITCH_MIN_X || px > PITCH_MAX_X) continue;
      float d = sqrtf((float)(dx * dx * 1.4f + dy * dy));
      if (d < 6.0f) {
        buf[py * 240 + px] = ((px ^ py) & 1) ? SOCCER_DIRT_1 : SOCCER_DIRT_2;
      } else if (d < 11.0f) {
        uint8_t h = (px * 7 + py * 13) & 3;
        buf[py * 240 + px] = (h == 0) ? SOCCER_DIRT_3 : ((h == 1) ? SOCCER_DIRT_1 : SOCCER_WEAR_GRS);
      } else if (d < 16.0f) {
        if (((px * 11 + py * 23) & 3) == 0) {
          buf[py * 240 + px] = SOCCER_WEAR_GRS;
        }
      }
    }
  }

  // C. Anti-Aliased Chalk Boundary Lines
  // Soft feathered chalk edges
  canvas.drawRect(PITCH_MIN_X + 7, PITCH_MIN_Y + 3, (PITCH_MAX_X - PITCH_MIN_X) - 14, (PITCH_MAX_Y - PITCH_MIN_Y) - 6, SOCCER_CHALK_AA);
  canvas.drawRect(PITCH_MIN_X + 9, PITCH_MIN_Y + 5, (PITCH_MAX_X - PITCH_MIN_X) - 18, (PITCH_MAX_Y - PITCH_MIN_Y) - 10, SOCCER_CHALK_AA);
  // Pure white chalk core lines
  canvas.drawRect(PITCH_MIN_X + 8, PITCH_MIN_Y + 4, (PITCH_MAX_X - PITCH_MIN_X) - 16, (PITCH_MAX_Y - PITCH_MIN_Y) - 8, SOCCER_CHALK);

  // Center line with anti-aliasing
  canvas.drawFastVLine(119, PITCH_MIN_Y + 4, (PITCH_MAX_Y - PITCH_MIN_Y) - 8, SOCCER_CHALK_AA);
  canvas.drawFastVLine(121, PITCH_MIN_Y + 4, (PITCH_MAX_Y - PITCH_MIN_Y) - 8, SOCCER_CHALK_AA);
  canvas.drawFastVLine(120, PITCH_MIN_Y + 4, (PITCH_MAX_Y - PITCH_MIN_Y) - 8, SOCCER_CHALK);

  // Center Circle & Center Spot
  canvas.drawCircle(120, 129, 29, SOCCER_CHALK_AA);
  canvas.drawCircle(120, 129, 27, SOCCER_CHALK_AA);
  canvas.drawCircle(120, 129, 28, SOCCER_CHALK);
  canvas.fillCircle(120, 129, 2, SOCCER_CHALK);

  // Corner Arcs
  canvas.drawCircle(PITCH_MIN_X + 8, PITCH_MIN_Y + 4, 6, SOCCER_CHALK);
  canvas.drawCircle(PITCH_MAX_X - 8, PITCH_MIN_Y + 4, 6, SOCCER_CHALK);
  canvas.drawCircle(PITCH_MIN_X + 8, PITCH_MAX_Y - 4, 6, SOCCER_CHALK);
  canvas.drawCircle(PITCH_MAX_X - 8, PITCH_MAX_Y - 4, 6, SOCCER_CHALK);

  // Penalty Boxes (18-yard box)
  canvas.drawRect(PITCH_MIN_X + 8, 76, 36, 106, SOCCER_CHALK);
  canvas.drawRect(PITCH_MAX_X - 44, 76, 36, 106, SOCCER_CHALK);

  // Goal Area (6-yard box)
  canvas.drawRect(PITCH_MIN_X + 8, 96, 14, 66, SOCCER_CHALK);
  canvas.drawRect(PITCH_MAX_X - 22, 96, 14, 66, SOCCER_CHALK);

  // Penalty Spots
  canvas.fillCircle(46, 129, 2, SOCCER_CHALK);
  canvas.fillCircle(194, 129, 2, SOCCER_CHALK);

  // Penalty Arcs ("The D" outside the box)
  for (int a = -50; a <= 50; a++) {
    float rad = a * (PI / 180.0f);
    int ax1 = 46 + (int)(cosf(rad) * 20.0f);
    int ay1 = 129 + (int)(sinf(rad) * 20.0f);
    if (ax1 > PITCH_MIN_X + 44) canvas.drawPixel(ax1, ay1, SOCCER_CHALK);

    int ax2 = 194 - (int)(cosf(rad) * 20.0f);
    int ay2 = 129 + (int)(sinf(rad) * 20.0f);
    if (ax2 < PITCH_MAX_X - 44) canvas.drawPixel(ax2, ay2, SOCCER_CHALK);
  }

  // D. 3D Goal Posts & Net Grid
  // Left Goal
  canvas.fillRect(PITCH_MIN_X, GOAL_Y_TOP, 8, GOAL_Y_BOT - GOAL_Y_TOP, SOCCER_NET_SHD);
  for (int gy = GOAL_Y_TOP; gy <= GOAL_Y_BOT; gy += 4) {
    canvas.drawFastHLine(PITCH_MIN_X, gy, 8, SOCCER_NET);
  }
  for (int gx = PITCH_MIN_X; gx <= PITCH_MIN_X + 8; gx += 3) {
    canvas.drawFastVLine(gx, GOAL_Y_TOP, GOAL_Y_BOT - GOAL_Y_TOP, SOCCER_NET);
  }
  canvas.drawFastVLine(PITCH_MIN_X + 8, GOAL_Y_TOP, GOAL_Y_BOT - GOAL_Y_TOP, COL_WHITE);
  canvas.drawFastHLine(PITCH_MIN_X, GOAL_Y_TOP, 8, COL_WHITE);
  canvas.drawFastHLine(PITCH_MIN_X, GOAL_Y_BOT, 8, COL_WHITE);

  // Right Goal
  canvas.fillRect(PITCH_MAX_X - 8, GOAL_Y_TOP, 8, GOAL_Y_BOT - GOAL_Y_TOP, SOCCER_NET_SHD);
  for (int gy = GOAL_Y_TOP; gy <= GOAL_Y_BOT; gy += 4) {
    canvas.drawFastHLine(PITCH_MAX_X - 8, gy, 8, SOCCER_NET);
  }
  for (int gx = PITCH_MAX_X - 8; gx <= PITCH_MAX_X; gx += 3) {
    canvas.drawFastVLine(gx, GOAL_Y_TOP, GOAL_Y_BOT - GOAL_Y_TOP, SOCCER_NET);
  }
  canvas.drawFastVLine(PITCH_MAX_X - 8, GOAL_Y_TOP, GOAL_Y_BOT - GOAL_Y_TOP, COL_WHITE);
  canvas.drawFastHLine(PITCH_MAX_X - 8, GOAL_Y_TOP, 8, COL_WHITE);
  canvas.drawFastHLine(PITCH_MAX_X - 8, GOAL_Y_BOT, 8, COL_WHITE);

  // E. Pitch-Side Sponsor Signage (Easter Eggs)
  // Avan FC Sign
  canvas.fillRect(46, 36, 44, 9, 0x8800);
  canvas.drawRect(46, 36, 44, 9, COL_GOLD);
  canvas.setTextSize(1);
  canvas.setTextColor(COL_GOLD, 0x8800);
  canvas.setCursor(48, 37);
  canvas.print("AVAN FC");

  // Merdan Sign
  canvas.fillRect(100, 36, 40, 9, 0x0113);
  canvas.drawRect(100, 36, 40, 9, COL_WHITE);
  canvas.setTextColor(COL_WHITE, 0x0113);
  canvas.setCursor(102, 37);
  canvas.print("MERDAN");

  // Rojda Sign
  canvas.fillRect(148, 36, 48, 9, 0x0B63);
  canvas.drawRect(148, 36, 48, 9, COL_GOLD);
  canvas.setTextColor(COL_GOLD, 0x0B63);
  canvas.setCursor(150, 37);
  canvas.print("ROJDA90");
}

// ============================================================================
// 2. ICE HOCKEY RINK RENDERING
// ============================================================================
// Icy cyan-white glossy rink surface with frosted skate texture, red goal crease
// semi-circles, red center line, blue zone lines, face-off dots & circles,
// curved rink boards with high-contrast acrylic gloss reflection.

static void render_hockey_rink(LGFX_Sprite &canvas) {
  canvas.fillScreen(COL_BLACK);
  uint16_t* buf = (uint16_t*)canvas.getBuffer();
  int cx = 120, cy = 120;
  int r = 117;

  // A. Ice Sheet with Alternating Zamboni Sweep Texture
  for (int y = PITCH_MIN_Y; y <= PITCH_MAX_Y; y++) {
    int dy = y - cy;
    int max_w = (int)sqrtf((float)(r * r - dy * dy));
    int x1 = cx - max_w;
    int x2 = cx + max_w;
    if (x1 < PITCH_MIN_X) x1 = PITCH_MIN_X;
    if (x2 > PITCH_MAX_X) x2 = PITCH_MAX_X;
    if (x1 >= x2) continue;

    uint16_t ice_col = HOCKEY_ICE_A; // Crisp uniform brilliant white ice
    for (int x = x1; x <= x2; x++) {
      buf[y * 240 + x] = ice_col;
    }
  }

  // B. Frosted Skate Texture (Crisp skate gouges & frost trails)
  // Procedural skate cuts across the ice sheet
  for (int y = PITCH_MIN_Y + 10; y <= PITCH_MAX_Y - 10; y += 7) {
    for (int x = PITCH_MIN_X + 16; x <= PITCH_MAX_X - 16; x += 19) {
      uint8_t h = ((x * 13 + y * 29) ^ (x >> 2)) & 7;
      if (h < 3) {
        int len = 3 + (h & 3);
        int dir = (h & 1) ? 1 : -1;
        for (int i = 0; i < len; i++) {
          int px = x + i;
          int py = y + i * dir;
          if (px >= PITCH_MIN_X + 10 && px <= PITCH_MAX_X - 10 &&
              py >= PITCH_MIN_Y + 10 && py <= PITCH_MAX_Y - 10) {
            buf[py * 240 + px] = HOCKEY_ICE_CUT;
            if (py + 1 <= PITCH_MAX_Y - 10) {
              buf[(py + 1) * 240 + px] = HOCKEY_ICE_SHD;
            }
          }
        }
      }
    }
  }

  // C. Regulation Ice Markings: Red Line, Blue Lines, Goal Lines
  // Center Red Line (2px red with alternating red/white dashed checker)
  for (int y = PITCH_MIN_Y + 6; y <= PITCH_MAX_Y - 6; y++) {
    uint16_t c = ((y / 4) % 2 == 0) ? HOCKEY_RED_LINE : COL_WHITE;
    canvas.drawPixel(119, y, c);
    canvas.drawPixel(120, y, c);
  }

  // Blue Zone Lines (2px thick regulation neutral zone lines)
  canvas.drawFastVLine(84, PITCH_MIN_Y + 6, (PITCH_MAX_Y - PITCH_MIN_Y) - 12, HOCKEY_BLU_LINE);
  canvas.drawFastVLine(85, PITCH_MIN_Y + 6, (PITCH_MAX_Y - PITCH_MIN_Y) - 12, HOCKEY_BLU_LINE);
  canvas.drawFastVLine(155, PITCH_MIN_Y + 6, (PITCH_MAX_Y - PITCH_MIN_Y) - 12, HOCKEY_BLU_LINE);
  canvas.drawFastVLine(156, PITCH_MIN_Y + 6, (PITCH_MAX_Y - PITCH_MIN_Y) - 12, HOCKEY_BLU_LINE);

  // Red Goal Lines (Thin red line across each zone)
  canvas.drawFastVLine(PITCH_MIN_X + 14, PITCH_MIN_Y + 16, (PITCH_MAX_Y - PITCH_MIN_Y) - 32, HOCKEY_RED_LINE);
  canvas.drawFastVLine(PITCH_MAX_X - 14, PITCH_MIN_Y + 16, (PITCH_MAX_Y - PITCH_MIN_Y) - 32, HOCKEY_RED_LINE);

  // D. Goal Creases (Painted ice-blue semi-circles with solid red borders)
  // Left Goal Crease
  canvas.fillCircle(PITCH_MIN_X + 14, 129, 13, HOCKEY_CREASE);
  canvas.drawCircle(PITCH_MIN_X + 14, 129, 13, HOCKEY_RED_LINE);
  canvas.drawFastVLine(PITCH_MIN_X + 14, 116, 27, HOCKEY_RED_LINE);

  // Right Goal Crease
  canvas.fillCircle(PITCH_MAX_X - 14, 129, 13, HOCKEY_CREASE);
  canvas.drawCircle(PITCH_MAX_X - 14, 129, 13, HOCKEY_RED_LINE);
  canvas.drawFastVLine(PITCH_MAX_X - 14, 116, 27, HOCKEY_RED_LINE);

  // E. Center Circle & Face-Off Dots
  canvas.drawCircle(120, 129, 26, HOCKEY_BLU_LINE);
  canvas.fillCircle(120, 129, 3, HOCKEY_RED_LINE);

  // 4 End-Zone Face-Off Circles & Dots with Alignment Hash Marks
  const int faceoff_pts[4][2] = { {60, 85}, {60, 173}, {180, 85}, {180, 173} };
  for (int i = 0; i < 4; i++) {
    int fx = faceoff_pts[i][0];
    int fy = faceoff_pts[i][1];
    canvas.drawCircle(fx, fy, 15, HOCKEY_RED_LINE);
    canvas.fillCircle(fx, fy, 3, HOCKEY_RED_LINE);
    // Face-off player hash marks
    canvas.drawFastHLine(fx - 18, fy - 4, 3, HOCKEY_RED_LINE);
    canvas.drawFastHLine(fx + 15, fy - 4, 3, HOCKEY_RED_LINE);
    canvas.drawFastHLine(fx - 18, fy + 4, 3, HOCKEY_RED_LINE);
    canvas.drawFastHLine(fx + 15, fy + 4, 3, HOCKEY_RED_LINE);
  }

  // 4 Neutral Zone Face-Off Dots
  canvas.fillCircle(102, 95, 2, HOCKEY_RED_LINE);
  canvas.fillCircle(102, 163, 2, HOCKEY_RED_LINE);
  canvas.fillCircle(138, 95, 2, HOCKEY_RED_LINE);
  canvas.fillCircle(138, 163, 2, HOCKEY_RED_LINE);

  // F. Curved Rink Dasher Boards with High-Contrast Acrylic Gloss Reflection
  // Corner rounded boards
  canvas.drawRoundRect(PITCH_MIN_X + 6, PITCH_MIN_Y + 2, (PITCH_MAX_X - PITCH_MIN_X) - 12, (PITCH_MAX_Y - PITCH_MIN_Y) - 4, 18, HOCKEY_BOARD_W);
  canvas.drawRoundRect(PITCH_MIN_X + 5, PITCH_MIN_Y + 1, (PITCH_MAX_X - PITCH_MIN_X) - 10, (PITCH_MAX_Y - PITCH_MIN_Y) - 2, 19, HOCKEY_BOARD_R);
  canvas.drawRoundRect(PITCH_MIN_X + 7, PITCH_MIN_Y + 3, (PITCH_MAX_X - PITCH_MIN_X) - 14, (PITCH_MAX_Y - PITCH_MIN_Y) - 6, 17, HOCKEY_BOARD_Y);

  // High-Contrast Acrylic Gloss Reflection along top boards
  for (int x = 60; x <= 180; x++) {
    int off = (x - 60) % 24;
    if (off >= 0 && off <= 10) {
      uint16_t refl = (off >= 4 && off <= 7) ? HOCKEY_GLASS_HI : HOCKEY_GLASS_CY;
      canvas.drawPixel(x, PITCH_MIN_Y + 2, refl);
      canvas.drawPixel(x + 1, PITCH_MIN_Y + 1, refl);
    }
  }
  // Bottom acrylic reflection
  for (int x = 70; x <= 170; x++) {
    int off = (x - 70) % 30;
    if (off >= 0 && off <= 8) {
      uint16_t refl = (off >= 3 && off <= 5) ? HOCKEY_GLASS_HI : HOCKEY_GLASS_CY;
      canvas.drawPixel(x, PITCH_MAX_Y - 3, refl);
    }
  }

  // G. Red Hockey Goals & Net
  // Left Goal
  canvas.fillRect(PITCH_MIN_X + 6, GOAL_Y_TOP + 6, 8, (GOAL_Y_BOT - GOAL_Y_TOP) - 12, 0x2124);
  for (int gy = GOAL_Y_TOP + 6; gy <= GOAL_Y_BOT - 6; gy += 3) {
    canvas.drawFastHLine(PITCH_MIN_X + 6, gy, 8, HOCKEY_NET_MESH);
  }
  canvas.drawRect(PITCH_MIN_X + 6, GOAL_Y_TOP + 6, 8, (GOAL_Y_BOT - GOAL_Y_TOP) - 12, HOCKEY_RED_LINE);

  // Right Goal
  canvas.fillRect(PITCH_MAX_X - 14, GOAL_Y_TOP + 6, 8, (GOAL_Y_BOT - GOAL_Y_TOP) - 12, 0x2124);
  for (int gy = GOAL_Y_TOP + 6; gy <= GOAL_Y_BOT - 6; gy += 3) {
    canvas.drawFastHLine(PITCH_MAX_X - 14, gy, 8, HOCKEY_NET_MESH);
  }
  canvas.drawRect(PITCH_MAX_X - 14, GOAL_Y_TOP + 6, 8, (GOAL_Y_BOT - GOAL_Y_TOP) - 12, HOCKEY_RED_LINE);

  // H. Dasher & Rink Sponsor Banners (Easter Eggs)
  canvas.setTextSize(1);
  // Neutral zone left: AVAN ICE
  canvas.fillRect(48, 34, 34, 9, 0x2B7D);
  canvas.drawRect(48, 34, 34, 9, COL_WHITE);
  canvas.setTextColor(COL_WHITE, 0x2B7D);
  canvas.setCursor(50, 35);
  canvas.print("AVAN");

  // Center ice: MERDAN CUP
  canvas.fillRect(102, 34, 36, 9, HOCKEY_RED_LINE);
  canvas.drawRect(102, 34, 36, 9, COL_WHITE);
  canvas.setTextColor(COL_WHITE, HOCKEY_RED_LINE);
  canvas.setCursor(104, 35);
  canvas.print("MERDAN");

  // Neutral zone right: ROJDA PRO
  canvas.fillRect(158, 34, 34, 9, 0x2B7D);
  canvas.drawRect(158, 34, 34, 9, COL_WHITE);
  canvas.setTextColor(COL_WHITE, 0x2B7D);
  canvas.setCursor(160, 35);
  canvas.print("ROJDA");
}

// ============================================================================
// 3. BASKETBALL COURT RENDERING
// ============================================================================
// High-gloss varnished parquet hardwood floor with alternating honey/amber oak
// wood planks, center circle with basketball seam emblem, 3-point arcs,
// painted key lanes, subtle overhead arena spotlight specular highlight.

static void render_basketball_court(LGFX_Sprite &canvas) {
  canvas.fillScreen(COL_BLACK);
  uint16_t* buf = (uint16_t*)canvas.getBuffer();
  int cx = 120, cy = 120;
  int r = 117;

  // A. Smooth Matte Maple Hardwood Floor (Calm, low-noise, high player/ball contrast)
  for (int y = PITCH_MIN_Y; y <= PITCH_MAX_Y; y++) {
    int dy = y - cy;
    int max_w = (int)sqrtf((float)(r * r - dy * dy));
    int x1 = cx - max_w;
    int x2 = cx + max_w;
    if (x1 < PITCH_MIN_X) x1 = PITCH_MIN_X;
    if (x2 > PITCH_MAX_X) x2 = PITCH_MAX_X;
    if (x1 >= x2) continue;

    int plank_idx = (y - PITCH_MIN_Y) / 14;
    int plank_y   = (y - PITCH_MIN_Y) % 14;

    uint16_t row_col = (plank_idx % 2 == 0) ? BASKET_MAPLE_A : BASKET_MAPLE_B;
    if (plank_y == 0) row_col = BASKET_MAPLE_SEAM;

    for (int x = x1; x <= x2; x++) {
      buf[y * 240 + x] = row_col;
    }
  }

  // B. Court Boundary & Halfway Line
  canvas.drawRect(PITCH_MIN_X + 8, PITCH_MIN_Y + 4, (PITCH_MAX_X - PITCH_MIN_X) - 16, (PITCH_MAX_Y - PITCH_MIN_Y) - 8, BASKET_LINE);
  canvas.drawFastVLine(120, PITCH_MIN_Y + 4, (PITCH_MAX_Y - PITCH_MIN_Y) - 8, BASKET_LINE);

  // C. Painted Key Paint Lanes (Deep arcade crimson with perimeter drop-shadow)
  // Left Key Lane
  canvas.fillRect(PITCH_MIN_X + 9, 95, 39, 68, BASKET_KEY_RED);
  canvas.drawRect(PITCH_MIN_X + 8, 95, 40, 68, BASKET_LINE);
  canvas.drawFastVLine(PITCH_MIN_X + 48, 95, 68, BASKET_KEY_SHD);
  // Free throw circle
  canvas.drawCircle(PITCH_MIN_X + 48, 129, 16, BASKET_LINE);
  // Rebound lane hash marks
  for (int hy = 105; hy <= 155; hy += 12) {
    canvas.drawFastHLine(PITCH_MIN_X + 8, hy, 4, BASKET_LINE);
    canvas.drawFastHLine(PITCH_MIN_X + 44, hy, 4, BASKET_LINE);
  }

  // Right Key Lane
  canvas.fillRect(PITCH_MAX_X - 47, 95, 39, 68, BASKET_KEY_RED);
  canvas.drawRect(PITCH_MAX_X - 48, 95, 40, 68, BASKET_LINE);
  canvas.drawFastVLine(PITCH_MAX_X - 49, 95, 68, BASKET_KEY_SHD);
  // Free throw circle
  canvas.drawCircle(PITCH_MAX_X - 48, 129, 16, BASKET_LINE);
  // Rebound lane hash marks
  for (int hy = 105; hy <= 155; hy += 12) {
    canvas.drawFastHLine(PITCH_MAX_X - 12, hy, 4, BASKET_LINE);
    canvas.drawFastHLine(PITCH_MAX_X - 48, hy, 4, BASKET_LINE);
  }

  // D. 3-Point Arcs
  // Left 3-Point Arc
  canvas.drawFastHLine(PITCH_MIN_X + 8, 52, 16, BASKET_LINE);
  canvas.drawFastHLine(PITCH_MIN_X + 8, 206, 16, BASKET_LINE);
  for (int a = -68; a <= 68; a++) {
    float rad = a * (PI / 180.0f);
    int ax = (PITCH_MIN_X + 24) + (int)(cosf(rad) * 58.0f);
    int ay = 129 + (int)(sinf(rad) * 58.0f);
    if (ax >= PITCH_MIN_X + 24 && ay >= 52 && ay <= 206) {
      canvas.drawPixel(ax, ay, BASKET_LINE);
    }
  }

  // Right 3-Point Arc
  canvas.drawFastHLine(PITCH_MAX_X - 24, 52, 16, BASKET_LINE);
  canvas.drawFastHLine(PITCH_MAX_X - 24, 206, 16, BASKET_LINE);
  for (int a = -68; a <= 68; a++) {
    float rad = a * (PI / 180.0f);
    int ax = (PITCH_MAX_X - 24) - (int)(cosf(rad) * 58.0f);
    int ay = 129 + (int)(sinf(rad) * 58.0f);
    if (ax <= PITCH_MAX_X - 24 && ay >= 52 && ay <= 206) {
      canvas.drawPixel(ax, ay, BASKET_LINE);
    }
  }

  // E. Center Court Jump Circle (Clean regulation markings - NO distracting orange ball!)
  canvas.drawCircle(120, 129, 24, BASKET_LINE);
  canvas.drawCircle(120, 129, 8, BASKET_LINE);
  canvas.fillCircle(120, 129, 2, BASKET_LINE);

  // F. 3D Basketball Hoops (Backboard, Target Box, Orange Rim & Net)
  // Left Hoop
  canvas.fillRect(PITCH_MIN_X + 6, 114, 3, 30, COL_WHITE);
  canvas.drawRect(PITCH_MIN_X + 6, 123, 2, 12, 0xF800); // Red target square
  canvas.drawCircle(PITCH_MIN_X + 16, 129, 5, BASKET_HOOP_RIM);
  canvas.drawCircle(PITCH_MIN_X + 16, 129, 4, BASKET_HOOP_RIM);
  canvas.drawFastHLine(PITCH_MIN_X + 9, 129, 3, 0x8410); // Rim support bracket
  // Net mesh
  canvas.drawFastHLine(PITCH_MIN_X + 12, 134, 8, BASKET_NET);
  canvas.drawFastHLine(PITCH_MIN_X + 13, 136, 6, BASKET_NET);
  canvas.drawFastHLine(PITCH_MIN_X + 14, 138, 4, BASKET_NET);

  // Right Hoop
  canvas.fillRect(PITCH_MAX_X - 9, 114, 3, 30, COL_WHITE);
  canvas.drawRect(PITCH_MAX_X - 8, 123, 2, 12, 0xF800);
  canvas.drawCircle(PITCH_MAX_X - 16, 129, 5, BASKET_HOOP_RIM);
  canvas.drawCircle(PITCH_MAX_X - 16, 129, 4, BASKET_HOOP_RIM);
  canvas.drawFastHLine(PITCH_MAX_X - 12, 129, 3, 0x8410);
  // Net mesh
  canvas.drawFastHLine(PITCH_MAX_X - 20, 134, 8, BASKET_NET);
  canvas.drawFastHLine(PITCH_MAX_X - 19, 136, 6, BASKET_NET);
  canvas.drawFastHLine(PITCH_MAX_X - 18, 138, 4, BASKET_NET);

  // G. Sideline Court Sponsor Banners (Easter Eggs)
  canvas.setTextSize(1);
  // Avan Court (Deep crimson / gloss gold)
  canvas.fillRect(48, 35, 42, 9, 0x5800);
  canvas.drawRect(48, 35, 42, 9, BASKET_GLOSS);
  canvas.setTextColor(BASKET_GLOSS, 0x5800);
  canvas.setCursor(50, 36);
  canvas.print("AVAN CT");

  // Merdan Hoops (Deep navy / white)
  canvas.fillRect(100, 35, 40, 9, 0x0113);
  canvas.drawRect(100, 35, 40, 9, COL_WHITE);
  canvas.setTextColor(COL_WHITE, 0x0113);
  canvas.setCursor(102, 36);
  canvas.print("MERDAN");

  // Rojda All-Star (Deep crimson / gloss gold)
  canvas.fillRect(150, 35, 40, 9, 0x5800);
  canvas.drawRect(150, 35, 40, 9, BASKET_GLOSS);
  canvas.setTextColor(BASKET_GLOSS, 0x5800);
  canvas.setCursor(152, 36);
  canvas.print("ROJDA");
}

// ============================================================================
// 4. BASEBALL FIELD RENDERING
// ============================================================================
// Rich reddish-brown infield dirt clay texture, crisp white foul lines and bases
// with 3D drop-shadows, lush outfield green turf, and outfield fence banner.

static void render_baseball_field(LGFX_Sprite &canvas) {
  canvas.fillScreen(COL_BLACK);
  uint16_t* buf = (uint16_t*)canvas.getBuffer();
  int cx = 120, cy = 120;
  int r = 117;

  // A. Ballpark Outfield Grass with Radial Mowed Lawn Pattern
  for (int y = PITCH_MIN_Y; y <= PITCH_MAX_Y; y++) {
    int dy = y - cy;
    int max_w = (int)sqrtf((float)(r * r - dy * dy));
    int x1 = cx - max_w;
    int x2 = cx + max_w;
    if (x1 < PITCH_MIN_X) x1 = PITCH_MIN_X;
    if (x2 > PITCH_MAX_X) x2 = PITCH_MAX_X;
    if (x1 >= x2) continue;

    for (int x = x1; x <= x2; x++) {
      // Radial mowing stripes emanating from home plate (120, 195)
      int f_dx = x - 120;
      int f_dy = y - 195;
      int dist = (int)sqrtf((float)(f_dx * f_dx + f_dy * f_dy));
      int stripe = (dist / 14) % 2;
      uint16_t grass_col = (stripe == 0) ? BASEBALL_TURF_A : BASEBALL_TURF_B;

      // Roller sheen on mowing transition
      if (dist % 14 == 0) grass_col = BASEBALL_TURF_S;

      // Stippling for authentic lawn texture
      if (((x * 13 + y * 29) & 7) == 0) {
        grass_col = (stripe == 0) ? 0x2D47 : 0x2484;
      }
      buf[y * 240 + x] = grass_col;
    }
  }

  // B. Outfield Warning Track (Volcanic crushed red clay dirt arc)
  for (int dy = -142; dy <= 0; dy++) {
    int py = 195 + dy;
    if (py < PITCH_MIN_Y || py > PITCH_MAX_Y) continue;
    int hw_out = (int)sqrtf((float)(142 * 142 - dy * dy));
    int hw_in  = (int)sqrtf((float)(133 * 133 - dy * dy));
    for (int x = 120 - hw_out; x <= 120 - hw_in; x++) {
      if (x >= PITCH_MIN_X && x <= PITCH_MAX_X) {
        buf[py * 240 + x] = ((x ^ py) & 1) ? BASEBALL_TRACK : BASEBALL_CLAY_C;
      }
    }
    for (int x = 120 + hw_in; x <= 120 + hw_out; x++) {
      if (x >= PITCH_MIN_X && x <= PITCH_MAX_X) {
        buf[py * 240 + x] = ((x ^ py) & 1) ? BASEBALL_TRACK : BASEBALL_CLAY_C;
      }
    }
  }

  // C. Outfield Fence Wall & Advertising Banner Pads
  for (int a = 210; a <= 330; a++) {
    float rad = a * (PI / 180.0f);
    int fx = 120 + (int)(cosf(rad) * 143.0f);
    int fy = 195 + (int)(sinf(rad) * 143.0f);
    if (fx >= PITCH_MIN_X && fx <= PITCH_MAX_X && fy >= PITCH_MIN_Y && fy <= PITCH_MAX_Y) {
      // Padded green fence wall
      canvas.drawPixel(fx, fy, BASEBALL_FENCE);
      canvas.drawPixel(fx, fy - 1, BASEBALL_PAD_Y); // Yellow safety top line
      // Sponsorship banner pads (red, blue, gold accents)
      int banner = (a / 8) % 4;
      if (banner == 1) canvas.drawPixel(fx, fy, BASEBALL_BANNER_R);
      else if (banner == 3) canvas.drawPixel(fx, fy, BASEBALL_BANNER_B);
    }
  }

  // Outfield Wall Sponsor Billboards (Easter Eggs)
  canvas.setTextSize(1);
  // Avan 24 Billboard (Left-center fence)
  canvas.fillRect(56, 62, 34, 9, BASEBALL_BANNER_R);
  canvas.drawRect(56, 62, 34, 9, COL_WHITE);
  canvas.setTextColor(COL_WHITE, BASEBALL_BANNER_R);
  canvas.setCursor(58, 63);
  canvas.print("AVAN");

  // Merdan Billboard (Center fence)
  canvas.fillRect(102, 53, 36, 9, BASEBALL_BANNER_B);
  canvas.drawRect(102, 53, 36, 9, COL_WHITE);
  canvas.setTextColor(COL_WHITE, BASEBALL_BANNER_B);
  canvas.setCursor(104, 54);
  canvas.print("MERDAN");

  // Rojda Billboard (Right-center fence)
  canvas.fillRect(150, 62, 34, 9, 0x0B63);
  canvas.drawRect(150, 62, 34, 9, COL_GOLD);
  canvas.setTextColor(COL_GOLD, 0x0B63);
  canvas.setCursor(152, 63);
  canvas.print("ROJDA");

  // D. Rich Reddish-Brown Infield Dirt Clay Texture (Diamond)
  for (int dy = -52; dy <= 52; dy++) {
    int hw = 54 - (abs(dy) * 54) / 52;
    int py = 145 + dy;
    if (py >= PITCH_MIN_Y && py <= PITCH_MAX_Y) {
      for (int x = 120 - hw; x <= 120 + hw; x++) {
        if (x >= PITCH_MIN_X && x <= PITCH_MAX_X) {
          uint8_t h = ((x * 17 + py * 31) ^ (x >> 1)) & 7;
          if (h == 0) buf[py * 240 + x] = BASEBALL_CLAY_C;
          else if (h == 7) buf[py * 240 + x] = BASEBALL_CLAY_D;
          else if (h == 1) buf[py * 240 + x] = BASEBALL_CLAY_B;
          else buf[py * 240 + x] = BASEBALL_CLAY_A;
        }
      }
    }
  }

  // E. Infield Grass Cutout (Manicured green island inside base paths)
  for (int dy = -26; dy <= 26; dy++) {
    int hw = 28 - (abs(dy) * 28) / 26;
    int py = 145 + dy;
    if (py >= PITCH_MIN_Y && py <= PITCH_MAX_Y) {
      for (int x = 120 - hw; x <= 120 + hw; x++) {
        if (x >= PITCH_MIN_X && x <= PITCH_MAX_X) {
          buf[py * 240 + x] = ((x ^ py) & 1) ? BASEBALL_TURF_A : 0x1C84;
        }
      }
    }
  }

  // F. Pitcher's Mound & Rubber (Concentric 3D clay elevation shading)
  canvas.fillCircle(120, 145, 10, BASEBALL_CLAY_C);
  canvas.fillCircle(120, 145, 8, BASEBALL_CLAY_A);
  canvas.fillCircle(120, 144, 5, 0xDE08); // Lighter clay summit
  // Pitcher's rubber slab (white rectangular rubber with drop shadow)
  canvas.drawFastHLine(117, 146, 6, BASEBALL_SHADOW);
  canvas.fillRect(117, 144, 6, 2, BASEBALL_BASE_W);

  // G. Crisp White Chalk Foul Lines & Batter's Boxes
  // 1st base foul line
  canvas.drawLine(120, 195, 218, 97, BASEBALL_CHALK);
  // 3rd base foul line
  canvas.drawLine(120, 195, 22, 97, BASEBALL_CHALK);

  // Batter's boxes (chalk rectangles on either side of home plate)
  canvas.drawRect(107, 189, 7, 13, BASEBALL_CHALK);
  canvas.drawRect(126, 189, 7, 13, BASEBALL_CHALK);

  // Home Plate (5-sided pentagon with 3D drop-shadow)
  canvas.fillRect(119, 194, 5, 5, BASEBALL_SHADOW);
  canvas.fillRect(118, 193, 5, 4, BASEBALL_BASE_W);
  canvas.drawPixel(120, 197, BASEBALL_BASE_W);
  canvas.drawPixel(119, 197, BASEBALL_BASE_S);

  // H. White Bases with 3D Drop-Shadows (1st, 2nd, 3rd)
  // 1st Base at (170, 143)
  canvas.fillRect(171, 144, 6, 6, BASEBALL_SHADOW); // 3D drop-shadow onto clay
  canvas.fillRect(170, 143, 5, 5, BASEBALL_BASE_W);   // White canvas bag top
  canvas.drawFastHLine(170, 147, 5, BASEBALL_BASE_S); // Bag shaded bottom edge
  canvas.drawFastVLine(174, 143, 5, BASEBALL_BASE_S); // Bag shaded right edge

  // 2nd Base at (118, 93)
  canvas.fillRect(119, 94, 6, 6, BASEBALL_SHADOW);
  canvas.fillRect(118, 93, 5, 5, BASEBALL_BASE_W);
  canvas.drawFastHLine(118, 97, 5, BASEBALL_BASE_S);
  canvas.drawFastVLine(122, 93, 5, BASEBALL_BASE_S);

  // 3rd Base at (66, 143)
  canvas.fillRect(67, 144, 6, 6, BASEBALL_SHADOW);
  canvas.fillRect(66, 143, 5, 5, BASEBALL_BASE_W);
  canvas.drawFastHLine(66, 147, 5, BASEBALL_BASE_S);
  canvas.drawFastVLine(70, 143, 5, BASEBALL_BASE_S);
}

// ============================================================================
// Main Stadium Surface Dispatcher
// ============================================================================

static inline void render_stadium_surface(LGFX_Sprite &canvas, SportType sport) {
  switch (sport) {
    case SPORT_SOCCER:
      render_soccer_pitch(canvas);
      break;
    case SPORT_HOCKEY:
      render_hockey_rink(canvas);
      break;
    case SPORT_BASKETBALL:
      render_basketball_court(canvas);
      break;
    case SPORT_BASEBALL:
      render_baseball_field(canvas);
      break;
    default:
      render_soccer_pitch(canvas);
      break;
  }
}
