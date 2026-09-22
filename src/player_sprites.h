#pragma once
#include <Arduino.h>
#include <math.h>
#include "LGFX_config.h"
#include "stadium_render.h"

// ============================================================================
// Kunio-kun 16-Bit Retro Arcade Character Models & Dynamic Animation (Phase 2)
// Super Famicom / Neo-Geo Multi-Tone Shading, Contours & Athletic Poses
// ============================================================================

#ifndef PLAYER_DEFINITIONS_H
#define PLAYER_DEFINITIONS_H

enum PlayerRole { ROLE_GK, ROLE_FWD, ROLE_DEF };
enum PlayerState {
  STATE_IDLE,
  STATE_RUNNING,
  STATE_SLIDE,
  STATE_KICKING,
  STATE_DIVING,
  STATE_DOWN,
  STATE_CHEER
};

struct Player {
  float x, y;
  float vx, vy;
  int team;          // 0 = RED (attacks right), 1 = BLUE (attacks left)
  PlayerRole role;
  PlayerState state;
  int state_timer;
  int facing;        // 1 = facing right, -1 = facing left
  int anim_frame;
  int anim_tick;
  uint16_t hair_color;
  float lane_y;
  float weave_phase;
};

#endif // PLAYER_DEFINITIONS_H

// External linkage to main game state
extern SportType current_sport;
extern void spawn_particle(float x, float y, float vx, float vy, uint16_t color, uint8_t life);

// ============================================================================
// 16-Bit Multi-Tone Color Palettes (RGB565)
// ============================================================================

// 1. Skin Multi-Tone Palette (Arcade Peach / Sunlit Athlete)
#define SPR_SKIN_HI         0xFF57  // Sunlit warm peach highlight
#define SPR_SKIN_MID        0xFD73  // Saturated arcade peach midtone
#define SPR_SKIN_SHADOW     0xBA49  // Terracotta contour shadow
#define SPR_SKIN_DEEP       0x7923  // Under-chin / ear crease shadow
#define SPR_SKIN_AA         0x48A2  // Soft silhouette sub-pixel anti-aliasing edge

// 2. Facial & Expression Accents
#define SPR_EYE_PUPIL       0x0000  // Jet black pupil
#define SPR_EYE_SPEC        0xFFFF  // 1-pixel Neo-Geo white eye glint
#define SPR_WHITE           0xFFFF  // Pure white highlight
#define SPR_BLACK           0x0000  // Pure black
#define SPR_BROW            0x18C3  // Furrowed determined athletic brow
#define SPR_MOUTH_DARK      0x4102  // Grimace mouth line
#define SPR_MOUTH_OPEN      0x9000  // Open cheering mouth interior
#define SPR_MOUTH_TEETH     0xFFFF  // Cheering white teeth glint

// 3. Red Team Uniform (3-Tone Crimson)
#define SPR_RED_HI          0xFA88  // Sunlit crimson-coral highlight along shoulder
#define SPR_RED_MID         0xD800  // Saturated arcade red midtone
#define SPR_RED_SHADOW      0x7800  // Deep burgundy contour fold shadow
#define SPR_RED_DARKEST     0x3800  // Crease / seam outline
#define SPR_RED_AA          0x5000  // Sub-pixel anti-aliasing edge

// 4. Blue Team Uniform (3-Tone Cobalt / Azure)
#define SPR_BLU_HI          0x65FF  // Electric sky cyan highlight along shoulder
#define SPR_BLU_MID         0x1BDF  // Vivid cobalt blue midtone
#define SPR_BLU_SHADOW      0x09F3  // Deep ocean navy fold shadow
#define SPR_BLU_DARKEST     0x0188  // Midnight crease outline
#define SPR_BLU_AA          0x0928  // Sub-pixel anti-aliasing edge

// 5. Shorts (3-Tone Silky Athletic White)
#define SPR_SHORTS_HI       0xFFFF  // Pure white crest highlight
#define SPR_SHORTS_MID      0xDEFB  // Silk light grey-white midtone
#define SPR_SHORTS_SHADOW   0x9CD3  // Cool slate fold shadow
#define SPR_SHORTS_DARK     0x5A8B  // Under-leg contour outline

// 6. Boots, Cleats, & Sneakers
#define SPR_BOOT_HI         0x52AA  // Polished leather gleam
#define SPR_BOOT_MID        0x2124  // Charcoal arcade boot
#define SPR_BOOT_SHADOW     0x1082  // Deep sole shadow
#define SPR_BOOT_LACE       0xFFFF  // White cleat lace highlight

// Basketball High-Tops
#define SPR_SNEAKER_SOLE    0xFFFF  // Crisp white rubber sole
#define SPR_SNEAKER_HI      0xEF7D  // High-top leather highlight
#define SPR_SNEAKER_SH      0x8C71  // High-top leather shadow
#define SPR_SNEAKER_TRIM    0xF800  // Red ankle collar accent

// Ice Hockey Skates
#define SPR_SKATE_HOLDER    0x2965  // Molded plastic blade holder
#define SPR_SKATE_BLADE_HI  0xFFFF  // Chrome blade reflective sheen
#define SPR_SKATE_BLADE_MID 0xD6BA  // Burnished steel silver
#define SPR_SKATE_BLADE_SH  0x73AE  // Underside blade shadow

// 7. Hair 3-Tone Palettes
// Black Hair (Kunio standard)
#define SPR_HAIR_BLK_HI     0x4228  // Graphite sheen
#define SPR_HAIR_BLK_MID    0x18C3  // Jet black hair
#define SPR_HAIR_BLK_SH     0x0000  // Deep contour shadow

// Brown Hair
#define SPR_HAIR_BRN_HI     0xBD25  // Warm caramel highlight
#define SPR_HAIR_BRN_MID    0x79A0  // Rich chestnut brown
#define SPR_HAIR_BRN_SH     0x3920  // Dark espresso shadow

// Blonde Hair
#define SPR_HAIR_BLD_HI     0xFFF0  // Platinum blonde highlight
#define SPR_HAIR_BLD_MID    0xFDE0  // Golden honey blonde
#define SPR_HAIR_BLD_SH     0xB440  // Amber gold shadow

// 8. Sports Gear & Props
#define SPR_WOOD_HI         0xF695  // Lacquered ash wood highlight
#define SPR_WOOD_MID        0xC443  // Turned maple/ash wood stick or bat
#define SPR_WOOD_SH         0x7A01  // Underside wood grain shadow
#define SPR_WOOD_TAPE       0xFFFF  // White friction grip tape

#define SPR_HELMET_VISOR    0x96DF  // Anti-glare tinted hockey visor
#define SPR_HELMET_SPEC     0xFFFF  // Visor reflection glint
#define SPR_HELMET_STRAP    0x2104  // Dark helmet chin strap

#define SPR_GLOVE_HI        0xFFE4  // Padded goalie glove gold highlight
#define SPR_GLOVE_MID       0xDC00  // Goalie glove gold leather
#define SPR_GLOVE_SH        0x8A00  // Glove crease shadow
#define SPR_GLOVE_PALM      0x18C3  // Black latex grip palm

// ============================================================================
// Palette Helpers
// ============================================================================

struct JerseyPalette {
  uint16_t hi;
  uint16_t mid;
  uint16_t shadow;
  uint16_t darkest;
  uint16_t aa;
};

inline JerseyPalette get_jersey_palette(int team) {
  if (team == 0) {
    return { SPR_RED_HI, SPR_RED_MID, SPR_RED_SHADOW, SPR_RED_DARKEST, SPR_RED_AA };
  } else {
    return { SPR_BLU_HI, SPR_BLU_MID, SPR_BLU_SHADOW, SPR_BLU_DARKEST, SPR_BLU_AA };
  }
}

struct HairPalette {
  uint16_t hi;
  uint16_t mid;
  uint16_t shadow;
};

inline HairPalette get_hair_palette(uint16_t hair_col) {
  if (hair_col == 0xFDE0 || hair_col == 0xFFE0) {
    return { SPR_HAIR_BLD_HI, SPR_HAIR_BLD_MID, SPR_HAIR_BLD_SH };
  } else if (hair_col == 0x79A0 || hair_col == 0x8260) {
    return { SPR_HAIR_BRN_HI, SPR_HAIR_BRN_MID, SPR_HAIR_BRN_SH };
  } else {
    return { SPR_HAIR_BLK_HI, SPR_HAIR_BLK_MID, SPR_HAIR_BLK_SH };
  }
}

// ============================================================================
// Modular Sub-Sprite Renderers (Head, Torso, Limbs, Gear)
// ============================================================================

// 1. Head & Face Detail (3-Tone skin, expressive brow, eye pupil with specular glint)
inline void draw_player_head(LGFX_Sprite &canvas, int hx, int hy, int dir,
                            const HairPalette &hp, const JerseyPalette &jp,
                            SportType sport, bool cheering = false) {
  // Beveled facial base (sub-pixel anti-aliasing corners)
  canvas.fillRect(hx + 1, hy + 2, 7, 6, SPR_SKIN_MID);
  canvas.drawFastHLine(hx + 2, hy + 1, 5, SPR_SKIN_HI);       // Forehead highlight
  canvas.drawFastVLine(hx, hy + 3, 4, SPR_SKIN_SHADOW);        // Rear jaw shadow
  canvas.drawFastHLine(hx + 2, hy + 8, 5, SPR_SKIN_SHADOW);   // Chin shadow
  canvas.drawPixel(hx + 1, hy + 8, SPR_SKIN_AA);               // Sub-pixel AA corner
  canvas.drawPixel(hx + 7, hy + 8, SPR_SKIN_AA);

  // --- Headgear & Hair Styling ---
  if (sport == SPORT_HOCKEY) {
    // 16-Bit Hockey Helmet with team colors, curved shell, and tinted visor
    canvas.fillRect(hx, hy - 3, 9, 4, jp.mid);
    canvas.drawFastHLine(hx + 1, hy - 4, 7, jp.hi);             // Top shell specular highlight
    canvas.drawFastHLine(hx, hy + 1, 9, jp.shadow);             // Lower helmet rim
    canvas.drawPixel(hx, hy - 3, jp.darkest);                   // Anti-aliased shell bevels
    canvas.drawPixel(hx + 8, hy - 3, jp.darkest);

    // Protective Ear Flap
    int ear_x = (dir > 0) ? (hx + 1) : (hx + 7);
    canvas.drawFastVLine(ear_x, hy + 2, 3, jp.shadow);

    // Tinted Curved Polycarbonate Visor with glare sheen
    int visor_start = (dir > 0) ? (hx + 3) : (hx + 1);
    canvas.drawFastHLine(visor_start, hy + 1, 5, SPR_HELMET_VISOR);
    canvas.drawPixel(visor_start + (dir > 0 ? 1 : 2), hy + 1, SPR_HELMET_SPEC); // Glare glint

    // Chin Strap
    canvas.drawFastHLine(hx + 2, hy + 7, 5, SPR_HELMET_STRAP);
  } else if (sport == SPORT_BASEBALL) {
    // 16-Bit Baseball Cap with 3D drop-shadow bill
    canvas.fillRect(hx + 1, hy - 3, 7, 4, jp.mid);
    canvas.drawFastHLine(hx + 2, hy - 4, 5, jp.hi);             // Cap crown highlight
    canvas.drawPixel(hx + 4, hy - 4, SPR_WHITE);                // Cap button
    canvas.drawPixel(hx + 1, hy - 3, jp.darkest);               // Corner bevel
    canvas.drawPixel(hx + 7, hy - 3, jp.darkest);

    // 3D Bill projecting forward with drop shadow
    int bill_x = (dir > 0) ? (hx + 5) : (hx - 2);
    canvas.drawFastHLine(bill_x, hy + 1, 5, jp.mid);            // Cap bill top
    canvas.drawFastHLine(bill_x + (dir > 0 ? 1 : 0), hy + 2, 4, jp.shadow); // Bill underside
    // Drop shadow cast onto the forehead & eyes!
    canvas.drawFastHLine(hx + 2, hy + 2, 5, SPR_SKIN_SHADOW);
  } else {
    // Textured Kunio Spiky Hair (Soccer & Basketball)
    // Multi-tiered spiky hair crest with highlight ridge & shadow roots
    canvas.fillRect(hx, hy - 3, 9, 4, hp.mid);
    canvas.drawFastHLine(hx + 1, hy - 4, 6, hp.hi);             // Top spike highlight
    // Back spikes projecting behind head
    int spike_back = (dir > 0) ? (hx - 1) : (hx + 8);
    canvas.drawFastVLine(spike_back, hy - 2, 3, hp.hi);
    canvas.drawPixel(spike_back, hy - 3, hp.shadow);
    // Front fringe
    int spike_front = (dir > 0) ? (hx + 7) : (hx + 1);
    canvas.drawPixel(spike_front, hy - 1, hp.hi);
    // Hair roots depth shadow across upper forehead
    canvas.drawFastHLine(hx + 1, hy + 1, 7, hp.shadow);
    // Sideburns
    int sideburn_x = (dir > 0) ? (hx + 1) : (hx + 7);
    canvas.drawFastVLine(sideburn_x, hy + 2, 3, hp.shadow);
  }

  // --- Expressive Facial Features ---
  int eye_x = (dir > 0) ? (hx + 5) : (hx + 2);
  int brow_x = (dir > 0) ? (hx + 4) : (hx + 2);

  // Furrowed athletic brow (slanted for intense competitive look)
  canvas.drawFastHLine(brow_x, hy + 2, 3, SPR_BROW);
  if (dir > 0) canvas.drawPixel(brow_x + 2, hy + 1, SPR_BROW);
  else canvas.drawPixel(brow_x, hy + 1, SPR_BROW);

  // 2-Pixel Eye with Neo-Geo specular glint
  canvas.drawPixel(eye_x, hy + 3, SPR_EYE_PUPIL);
  canvas.drawPixel(eye_x + 1, hy + 3, SPR_EYE_PUPIL);
  canvas.drawPixel(eye_x, hy + 3, SPR_EYE_SPEC);               // White highlight glint!

  // Nose contour
  int nose_x = (dir > 0) ? (hx + 7) : (hx + 1);
  canvas.drawPixel(nose_x, hy + 4, SPR_SKIN_SHADOW);

  // Mouth
  int mouth_x = (dir > 0) ? (hx + 4) : (hx + 3);
  if (cheering) {
    // Open cheering smile
    canvas.fillRect(mouth_x, hy + 5, 3, 2, SPR_MOUTH_OPEN);
    canvas.drawFastHLine(mouth_x, hy + 5, 3, SPR_MOUTH_TEETH);
  } else {
    // Determined grit / grimace
    canvas.drawFastHLine(mouth_x, hy + 6, 2, SPR_MOUTH_DARK);
  }
}

// 2. Torso & Uniform (3-Tone jersey, muscular contouring, team accents)
inline void draw_player_torso(LGFX_Sprite &canvas, int tx, int ty, int dir,
                             const JerseyPalette &jp, SportType sport) {
  if (sport == SPORT_HOCKEY) {
    // Bulky padded hockey jersey (wider shoulders)
    canvas.fillRect(tx - 1, ty, 10, 6, jp.mid);
    canvas.drawFastHLine(tx, ty, 8, jp.hi);                     // Shoulder pad highlight
    canvas.drawFastHLine(tx - 1, ty + 5, 10, jp.shadow);        // Waist fold shadow
    canvas.drawFastVLine(tx - 1, ty + 1, 4, jp.darkest);        // Outer seam bevel
    canvas.drawFastVLine(tx + 8, ty + 1, 4, jp.darkest);
    // Team chest stripe accent
    canvas.drawFastHLine(tx + 1, ty + 3, 6, SPR_WHITE);
  } else if (sport == SPORT_BASKETBALL) {
    // Sleeveless athletic tank top jersey (contoured muscular chest)
    canvas.fillRect(tx + 1, ty, 6, 6, jp.mid);
    canvas.drawFastHLine(tx + 2, ty, 4, jp.hi);                 // Collarbone highlight
    canvas.drawFastHLine(tx + 1, ty + 5, 6, jp.shadow);         // Waist shadow
    // Deep armholes showing muscular skin sides
    canvas.drawFastVLine(tx, ty + 1, 4, SPR_SKIN_SHADOW);
    canvas.drawFastVLine(tx + 7, ty + 1, 4, SPR_SKIN_SHADOW);
    // Jersey neckline
    canvas.drawPixel(tx + 3, ty, SPR_SKIN_SHADOW);
    canvas.drawPixel(tx + 4, ty, SPR_SKIN_SHADOW);
  } else {
    // Soccer & Baseball Standard Uniform
    canvas.fillRect(tx, ty, 8, 6, jp.mid);
    canvas.drawFastHLine(tx + 1, ty, 6, jp.hi);                 // Shoulder highlight
    canvas.drawFastHLine(tx, ty + 5, 8, jp.shadow);             // Hemline fold
    canvas.drawFastVLine(tx, ty + 1, 4, jp.darkest);            // Seam line
    canvas.drawFastVLine(tx + 7, ty + 1, 4, jp.darkest);
    // Athletic center seam fold
    canvas.drawFastVLine(tx + 4, ty + 1, 4, jp.shadow);
  }
}

// 3. Shorts (3-Tone athletic silk white with team side stripe)
inline void draw_player_shorts(LGFX_Sprite &canvas, int sx, int sy, int dir,
                              const JerseyPalette &jp) {
  canvas.fillRect(sx, sy, 8, 4, SPR_SHORTS_MID);
  canvas.drawFastHLine(sx + 1, sy, 6, SPR_SHORTS_HI);          // Waistband highlight
  canvas.drawFastHLine(sx, sy + 3, 8, SPR_SHORTS_SHADOW);      // Hem shadow
  // Team-colored side stripe piping
  int stripe_x = (dir > 0) ? (sx + 1) : (sx + 6);
  canvas.drawFastVLine(stripe_x, sy, 4, jp.mid);
  // Center groin crease shadow separating thighs
  canvas.drawFastVLine(sx + 4, sy + 2, 2, SPR_SHORTS_DARK);
}

// ============================================================================
// Sport-Specific Special Athletic Action Poses
// ============================================================================

// 1. SOCCER KICKING FOLLOW-THROUGH (Leg fully extended forward, arms wide, body lean)
inline void draw_player_soccer_kick(LGFX_Sprite &canvas, int px, int py, int dir,
                                   const Player &p, const JerseyPalette &jp,
                                   const HairPalette &hp) {
  // Shadow on grass beneath plant foot
  canvas.fillEllipse(px - 2 * dir, py + 1, 8, 3, COL_SHADOW);

  // Dynamic backward torso lean during strike
  int head_y = py - 17;
  int torso_y = head_y + 7;
  int hx = px - 4 * dir;
  int tx = px - 3 * dir;

  // Head focused on ball trajectory
  draw_player_head(canvas, hx, head_y, dir, hp, jp, SPORT_SOCCER, false);

  // Torso arched back
  draw_player_torso(canvas, tx, torso_y, dir, jp, SPORT_SOCCER);

  // Arms flung wide for athletic counter-balance
  // Trailing arm (back and low)
  canvas.fillRect(tx - 3 * dir, torso_y + 1, 2, 4, jp.mid);
  canvas.fillRect(tx - 4 * dir, torso_y + 4, 3, 2, SPR_SKIN_MID);
  // Leading arm (forward and high)
  canvas.fillRect(tx + 7 * dir, torso_y - 2, 2, 4, jp.hi);
  canvas.fillRect(tx + 8 * dir, torso_y - 4, 3, 3, SPR_SKIN_HI);

  // Planted support leg (bent deep to support kick)
  int plant_x = tx;
  int plant_y = torso_y + 6;
  canvas.fillRect(plant_x, plant_y, 4, 3, SPR_SHORTS_MID);
  canvas.fillRect(plant_x - 1 * dir, plant_y + 3, 3, 3, SPR_SKIN_MID);
  canvas.fillRect(plant_x - 2 * dir, plant_y + 5, 4, 3, SPR_BOOT_MID);
  canvas.drawFastHLine(plant_x - 2 * dir, plant_y + 7, 4, SPR_BOOT_SHADOW);

  // Powerful Kicking Leg (Violently extended forward & upward at 40 degrees!)
  int kick_thigh_x = tx + 4 * dir;
  int kick_thigh_y = torso_y + 5;
  canvas.fillRect(kick_thigh_x, kick_thigh_y, 4 * dir, 4, SPR_SHORTS_MID);
  // Shin stretching forward
  int shin_x = kick_thigh_x + 3 * dir;
  int shin_y = kick_thigh_y - 1;
  canvas.fillRect(shin_x, shin_y, 5 * dir, 3, SPR_SKIN_HI);
  // Cleat with white laces pointed forward
  int cleat_x = shin_x + 4 * dir;
  int cleat_y = shin_y - 1;
  canvas.fillRect(cleat_x, cleat_y, 4 * dir, 3, SPR_BOOT_MID);
  canvas.drawPixel(cleat_x + dir, cleat_y, SPR_BOOT_LACE);     // White shoelace glint
  canvas.drawFastHLine(cleat_x, cleat_y + 2, 4 * dir, SPR_BOOT_SHADOW);

  // Grass divot particles flying forward
  spawn_particle(cleat_x, cleat_y + 2, dir * 2.2f, -0.8f, 0x24C5, 8);
}

// 2. SOCCER DIVING GOALKEEPER (Airborne horizontal dive with outstretched padded gloves)
inline void draw_player_soccer_dive(LGFX_Sprite &canvas, int px, int py, int dir,
                                   const Player &p, const JerseyPalette &jp,
                                   const HairPalette &hp) {
  // Ellipse shadow on turf below airborne keeper
  canvas.fillEllipse(px, py + 3, 10, 3, COL_SHADOW);

  int dive_y = py - 7; // Levitating in flight

  // Head tucked between shoulders
  int hx = px + 2 * dir;
  draw_player_head(canvas, hx, dive_y - 3, dir, hp, jp, SPORT_SOCCER, false);

  // Torso stretched horizontally
  int tx = px - 4 * dir;
  canvas.fillRect(tx, dive_y - 2, 7 * dir, 7, jp.mid);
  canvas.drawFastHLine(tx, dive_y - 2, 7 * dir, jp.hi);
  canvas.drawFastHLine(tx, dive_y + 4, 7 * dir, jp.shadow);

  // Outstretched Arms with Oversized Padded Goalie Gloves!
  int arm_x = hx + 5 * dir;
  int arm_y = dive_y - 2;
  canvas.fillRect(arm_x, arm_y, 5 * dir, 3, jp.hi);
  // Dual Padded Gloves reaching out to block shot
  int glove_x = arm_x + 4 * dir;
  canvas.fillRect(glove_x, arm_y - 2, 4 * dir, 4, SPR_GLOVE_MID);
  canvas.fillRect(glove_x + dir, arm_y + 1, 4 * dir, 4, SPR_GLOVE_HI);
  canvas.drawFastVLine(glove_x + 3 * dir, arm_y - 1, 5, SPR_GLOVE_PALM); // Black grip palm

  // Shorts & Legs trailing horizontally behind in mid-air
  int sx = tx - 4 * dir;
  canvas.fillRect(sx, dive_y - 1, 5 * dir, 6, SPR_SHORTS_MID);
  // Streamlined trailing legs
  int leg_x = sx - 4 * dir;
  canvas.fillRect(leg_x, dive_y, 5 * dir, 3, SPR_SKIN_MID);
  canvas.fillRect(leg_x - 3 * dir, dive_y, 4 * dir, 4, SPR_BOOT_MID);
  canvas.drawPixel(leg_x - 2 * dir, dive_y, SPR_BOOT_LACE);
}

// 3. HOCKEY TWO-HANDED STICK SLASH / CROSS-CHECK (Deep skate lunge, thrusting stick)
inline void draw_player_hockey_slash(LGFX_Sprite &canvas, int px, int py, int dir,
                                    const Player &p, const JerseyPalette &jp,
                                    const HairPalette &hp) {
  int head_y = py - 16;
  int torso_y = head_y + 7;
  int shorts_y = torso_y + 5;
  int legs_y = shorts_y + 3;

  // Helmet with team visor
  int hx = px - 3;
  draw_player_head(canvas, hx, head_y, dir, hp, jp, SPORT_HOCKEY, false);

  // Torso lunging forward
  int tx = px - 4 + 2 * dir;
  draw_player_torso(canvas, tx, torso_y, dir, jp, SPORT_HOCKEY);

  // Two-handed Wooden Stick Slash extending forward!
  int stick_x1 = px + 1 * dir;
  int stick_y1 = torso_y + 2;
  int stick_x2 = px + 13 * dir;
  int stick_y2 = py;
  // 3-Tone Wood Stick Shaft
  canvas.drawLine(stick_x1, stick_y1 - 1, stick_x2, stick_y2 - 1, SPR_WOOD_HI);
  canvas.drawLine(stick_x1, stick_y1, stick_x2, stick_y2, SPR_WOOD_MID);
  canvas.drawLine(stick_x1, stick_y1 + 1, stick_x2, stick_y2 + 1, SPR_WOOD_SH);
  // Stick Blade with white friction tape
  canvas.drawFastHLine(stick_x2, stick_y2, 5 * dir, SPR_WOOD_MID);
  canvas.drawPixel(stick_x2 + 2 * dir, stick_y2, SPR_WOOD_TAPE);

  // Dual Hands gripping stick
  canvas.fillRect(stick_x1 - 1, stick_y1 - 1, 3, 3, SPR_SKIN_MID);
  canvas.fillRect(stick_x1 + 3 * dir, stick_y1, 3, 3, SPR_SKIN_HI);

  // Shorts & bent legs on skates
  canvas.fillRect(px - 3, shorts_y, 7, 3, SPR_SHORTS_MID);
  canvas.fillRect(px - 4, legs_y, 3, 3, SPR_SKIN_MID);
  canvas.fillRect(px - 4, legs_y + 2, 3, 2, SPR_BOOT_MID);
  canvas.fillRect(px + 1, legs_y, 3, 3, SPR_SKIN_MID);
  canvas.fillRect(px + 1, legs_y + 2, 3, 2, SPR_BOOT_MID);

  // Silver Skate Blades on ice
  canvas.drawFastHLine(px - 5, legs_y + 4, 4, SPR_SKATE_BLADE_HI);
  canvas.drawFastHLine(px + 1, legs_y + 4, 4, SPR_SKATE_BLADE_HI);
  canvas.drawFastHLine(px - 5, legs_y + 5, 4, SPR_SKATE_BLADE_MID);
  canvas.drawFastHLine(px + 1, legs_y + 5, 4, SPR_SKATE_BLADE_MID);

  // Ice shavings trailing off skates
  spawn_particle(px - 2 * dir, py + 1, -dir * 1.8f, -0.6f, COL_WHITE, 8);
}

// 4. HOCKEY WIND-UP SLAP SHOT STANCE (Towering stick backswing, loaded rear skate)
inline void draw_player_hockey_slapshot(LGFX_Sprite &canvas, int px, int py, int dir,
                                       const Player &p, const JerseyPalette &jp,
                                       const HairPalette &hp) {
  canvas.fillEllipse(px, py + 1, 7, 3, COL_SHADOW);

  int head_y = py - 17;
  int torso_y = head_y + 7;
  int hx = px - 2 * dir;
  int tx = px - 3 * dir;

  draw_player_head(canvas, hx, head_y, dir, hp, jp, SPORT_HOCKEY, false);
  draw_player_torso(canvas, tx, torso_y, dir, jp, SPORT_HOCKEY);

  // Stick raised high in towering wind-up behind back!
  int stick_top_x = tx - 8 * dir;
  int stick_top_y = head_y - 6;
  int stick_bot_x = tx + 2 * dir;
  int stick_bot_y = torso_y + 3;

  canvas.drawLine(stick_top_x, stick_top_y, stick_bot_x, stick_bot_y, SPR_WOOD_MID);
  canvas.drawLine(stick_top_x, stick_top_y - 1, stick_bot_x, stick_bot_y - 1, SPR_WOOD_HI);
  // Blade angled up at top of swing
  canvas.drawFastHLine(stick_top_x - 3 * dir, stick_top_y, 4 * dir, SPR_WOOD_MID);

  // Hands raised gripping the shaft
  canvas.fillRect(stick_bot_x - 1, stick_bot_y - 1, 3, 3, SPR_SKIN_MID);
  canvas.fillRect(tx - 3 * dir, head_y + 2, 3, 3, SPR_SKIN_HI);

  // Wide loaded skate stance
  int shorts_y = torso_y + 5;
  int legs_y = shorts_y + 3;
  canvas.fillRect(tx, shorts_y, 8, 3, SPR_SHORTS_MID);
  // Front skate
  canvas.fillRect(tx + 4 * dir, legs_y, 3, 3, SPR_BOOT_MID);
  canvas.drawFastHLine(tx + 3 * dir, legs_y + 3, 5, SPR_SKATE_BLADE_HI);
  // Rear skate (dug in to ice)
  canvas.fillRect(tx - 3 * dir, legs_y, 3, 3, SPR_BOOT_MID);
  canvas.drawFastHLine(tx - 4 * dir, legs_y + 3, 5, SPR_SKATE_BLADE_HI);
}

// 5. BASKETBALL AIRBORNE JUMP-BLOCK SWAT (Both arms stretched high, high vertical jump)
inline void draw_player_basketball_block(LGFX_Sprite &canvas, int px, int py, int dir,
                                        const Player &p, const JerseyPalette &jp,
                                        const HairPalette &hp) {
  int jump_h = 9; // High vertical leap
  int j_py = py - jump_h;

  // Court floor shadow below jumper
  canvas.fillEllipse(px, py, 7, 3, COL_SHADOW);

  int head_y = j_py - 16;
  int torso_y = head_y + 7;
  int shorts_y = torso_y + 5;
  int legs_y = shorts_y + 3;

  // Head looking upward
  int hx = px - 3;
  draw_player_head(canvas, hx, head_y, dir, hp, jp, SPORT_BASKETBALL, false);

  // Both Arms stretched high to block!
  // Left arm & hand swatting
  canvas.fillRect(px - 5, head_y - 8, 3, 9, SPR_SKIN_MID);
  canvas.drawFastVLine(px - 5, head_y - 8, 9, SPR_SKIN_HI);
  canvas.fillRect(px - 6, head_y - 10, 4, 3, SPR_SKIN_HI); // Hand with splayed fingers
  // Right arm & hand swatting
  canvas.fillRect(px + 3, head_y - 8, 3, 9, SPR_SKIN_MID);
  canvas.drawFastVLine(px + 5, head_y - 8, 9, SPR_SKIN_SHADOW);
  canvas.fillRect(px + 3, head_y - 10, 4, 3, SPR_SKIN_HI);

  // Sleeveless Torso
  draw_player_torso(canvas, px - 4, torso_y, dir, jp, SPORT_BASKETBALL);

  // Shorts & Legs dangling in air
  draw_player_shorts(canvas, px - 3, shorts_y, dir, jp);

  // High-Top Basketball Sneakers in air
  canvas.fillRect(px - 3, legs_y, 2, 3, SPR_SKIN_MID);
  canvas.fillRect(px - 4, legs_y + 2, 3, 3, SPR_SNEAKER_HI);
  canvas.drawFastHLine(px - 4, legs_y + 4, 3, SPR_SNEAKER_SOLE); // White sole

  canvas.fillRect(px + 2, legs_y, 2, 3, SPR_SKIN_MID);
  canvas.fillRect(px + 1, legs_y + 2, 3, 3, SPR_SNEAKER_HI);
  canvas.drawFastHLine(px + 1, legs_y + 4, 3, SPR_SNEAKER_SOLE);
}

// 6. BASKETBALL HIGH-ELEVATION SLAM DUNK (One-hand dunk reach over the rim, ball gripped)
inline void draw_player_basketball_dunk(LGFX_Sprite &canvas, int px, int py, int dir,
                                       const Player &p, const JerseyPalette &jp,
                                       const HairPalette &hp) {
  int jump_h = 12; // Massive elevation above rim!
  int j_py = py - jump_h;

  canvas.fillEllipse(px, py, 6, 2, COL_SHADOW);

  int head_y = j_py - 16;
  int torso_y = head_y + 7;
  int hx = px - 3 * dir;

  // Cheering / roaring face
  draw_player_head(canvas, hx, head_y, dir, hp, jp, SPORT_BASKETBALL, true);
  draw_player_torso(canvas, px - 4, torso_y, dir, jp, SPORT_BASKETBALL);

  // Main Slam Dunk Arm reached high forward clutching the basketball!
  int arm_x = px + 3 * dir;
  int arm_y = head_y - 9;
  canvas.fillRect(arm_x, arm_y, 3, 9, SPR_SKIN_HI);
  // Hand gripping top of ball
  canvas.fillRect(arm_x - 1, arm_y - 2, 5, 3, SPR_SKIN_HI);

  // Gripped Basketball in hand poised over rim
  int b_x = arm_x + dir;
  int b_y = arm_y - 4;
  canvas.fillCircle(b_x, b_y, 4, 0xF3C0); // Classic basketball orange
  canvas.drawPixel(b_x, b_y, 0x2945);     // Black seam
  canvas.drawPixel(b_x - 1, b_y, 0x2945);
  canvas.drawPixel(b_x + 1, b_y, 0x2945);

  // Free arm bent at chest for balance
  canvas.fillRect(px - 5 * dir, torso_y, 3, 4, SPR_SKIN_MID);

  // Mid-air Scissor-kicked legs with high-tops
  int shorts_y = torso_y + 5;
  int legs_y = shorts_y + 3;
  draw_player_shorts(canvas, px - 3, shorts_y, dir, jp);

  // Front trailing leg
  canvas.fillRect(px + 2 * dir, legs_y, 3, 3, SPR_SKIN_MID);
  canvas.fillRect(px + 3 * dir, legs_y + 2, 4, 3, SPR_SNEAKER_HI);
  canvas.drawFastHLine(px + 3 * dir, legs_y + 4, 4, SPR_SNEAKER_SOLE);

  // Rear leg kicked back
  canvas.fillRect(px - 4 * dir, legs_y + 1, 3, 3, SPR_SKIN_MID);
  canvas.fillRect(px - 5 * dir, legs_y + 3, 4, 3, SPR_SNEAKER_HI);
  canvas.drawFastHLine(px - 5 * dir, legs_y + 5, 4, SPR_SNEAKER_SOLE);
}

// 7. BASEBALL COILED BATTING STANCE (Batter coiled, wooden bat raised over shoulder)
inline void draw_player_baseball_batting(LGFX_Sprite &canvas, int px, int py, int dir,
                                        const Player &p, const JerseyPalette &jp,
                                        const HairPalette &hp) {
  canvas.fillEllipse(px, py + 1, 7, 3, COL_SHADOW);

  int head_y = py - 17;
  int torso_y = head_y + 7;
  int hx = px - 3;
  int tx = px - 4;

  draw_player_head(canvas, hx, head_y, dir, hp, jp, SPORT_BASEBALL, false);
  draw_player_torso(canvas, tx, torso_y, dir, jp, SPORT_BASEBALL);

  // Wooden Bat held cocked over back shoulder!
  int bat_bot_x = tx - 2 * dir;
  int bat_bot_y = torso_y + 1;
  int bat_top_x = tx - 9 * dir;
  int bat_top_y = head_y - 7;

  // 3-Tone Turned Ash Wood Bat
  canvas.drawLine(bat_bot_x, bat_bot_y, bat_top_x, bat_top_y, SPR_WOOD_MID);
  canvas.drawLine(bat_bot_x, bat_bot_y - 1, bat_top_x, bat_top_y - 1, SPR_WOOD_HI);
  canvas.drawLine(bat_bot_x, bat_bot_y + 1, bat_top_x, bat_top_y + 1, SPR_WOOD_SH);
  // White grip tape at base of handle
  canvas.drawFastHLine(bat_bot_x - dir, bat_bot_y, 2, SPR_WOOD_TAPE);

  // Hands gripping bat handle
  canvas.fillRect(bat_bot_x - 1, bat_bot_y - 1, 3, 3, SPR_SKIN_MID);
  canvas.fillRect(bat_bot_x + dir, bat_bot_y, 3, 3, SPR_SKIN_HI);

  // Coiled batting stance with bent knees
  int shorts_y = torso_y + 5;
  int legs_y = shorts_y + 3;
  draw_player_shorts(canvas, px - 3, shorts_y, dir, jp);

  canvas.fillRect(px - 4, legs_y, 3, 3, SPR_SKIN_MID);
  canvas.fillRect(px - 4, legs_y + 2, 3, 2, SPR_BOOT_MID);
  canvas.fillRect(px + 2, legs_y, 3, 3, SPR_SKIN_MID);
  canvas.fillRect(px + 2, legs_y + 2, 3, 2, SPR_BOOT_MID);
}

// 8. BASEBALL HORIZONTAL SWING FOLLOW-THROUGH (180-degree bat blur follow-through)
inline void draw_player_baseball_swing(LGFX_Sprite &canvas, int px, int py, int dir,
                                      const Player &p, const JerseyPalette &jp,
                                      const HairPalette &hp) {
  canvas.fillEllipse(px, py + 1, 8, 3, COL_SHADOW);

  int head_y = py - 17;
  int torso_y = head_y + 7;
  int hx = px - 2 * dir;
  int tx = px - 3 * dir;

  draw_player_head(canvas, hx, head_y, dir, hp, jp, SPORT_BASEBALL, false);
  draw_player_torso(canvas, tx, torso_y, dir, jp, SPORT_BASEBALL);

  // Torso rotated forward, arms swept across chest
  int swing_hand_x = tx + 5 * dir;
  int swing_hand_y = torso_y + 3;
  canvas.fillRect(tx + 1 * dir, torso_y + 1, 4 * dir, 3, SPR_SKIN_HI);

  // Horizontal Bat Whipping Across Front of Body!
  int bat_end_x = swing_hand_x + 10 * dir;
  int bat_end_y = torso_y + 2;
  canvas.drawFastHLine(swing_hand_x, bat_end_y, 10 * dir, SPR_WOOD_MID);
  canvas.drawFastHLine(swing_hand_x, bat_end_y - 1, 10 * dir, SPR_WOOD_HI);
  canvas.drawFastHLine(swing_hand_x, bat_end_y + 1, 10 * dir, SPR_WOOD_SH);

  // Speed blur streak behind bat
  canvas.drawFastHLine(swing_hand_x - 3 * dir, bat_end_y - 2, 8 * dir, 0xFFE0); // Gold trail

  // Pivoted cleats in dirt
  int shorts_y = torso_y + 5;
  int legs_y = shorts_y + 3;
  draw_player_shorts(canvas, tx + 1 * dir, shorts_y, dir, jp);
  canvas.fillRect(tx - 1 * dir, legs_y, 3, 3, SPR_BOOT_MID);
  canvas.fillRect(tx + 4 * dir, legs_y, 3, 3, SPR_BOOT_MID);

  // Infield clay dust kick-up
  spawn_particle(px, py + 1, -dir * 1.5f, -0.6f, 0xD3C6, 10);
}

// 9. KNOCKED DOWN POSE (Kunio dazed KO with 3-tone orbiting dizzy stars)
inline void draw_player_knocked_down(LGFX_Sprite &canvas, int px, int py, int dir,
                                    const Player &p, const JerseyPalette &jp,
                                    const HairPalette &hp) {
  // Lying horizontal on turf/ice/court
  canvas.fillEllipse(px, py - 2, 10, 4, COL_SHADOW);

  int ly = py - 6;
  // Splayed jersey on ground
  canvas.fillRect(px - 7, ly, 14, 5, jp.mid);
  canvas.drawFastHLine(px - 7, ly, 14, jp.hi);
  canvas.drawFastHLine(px - 7, ly + 4, 14, jp.shadow);

  // Head flat on ground
  int head_x = px - 11 * dir;
  canvas.fillRect(head_x, ly - 1, 6, 6, SPR_SKIN_MID);
  canvas.drawFastHLine(head_x, ly - 2, 7, hp.mid);
  // Dazed X eye
  canvas.drawPixel(head_x + 3, ly + 2, SPR_EYE_PUPIL);
  canvas.drawPixel(head_x + 4, ly + 1, SPR_EYE_PUPIL);

  // Shorts & legs splayed
  canvas.fillRect(px + 4 * dir, ly + 1, 5, 4, SPR_SHORTS_MID);
  canvas.fillRect(px + 8 * dir, ly + 1, 4, 3, SPR_BOOT_MID);

  // 3 Orbiting Comical 16-Bit Dizzy Stars with multi-tone sparkle
  uint32_t t = millis() / 90;
  for (int s = 0; s < 3; s++) {
    int angle = (t * 40 + s * 85) % 256;
    float rad = angle * (PI / 128.0f);
    int sx = (px - 8 * dir) + (int)(cosf(rad) * 9.0f);
    int sy = (ly - 8) + (int)(sinf(rad) * 4.0f);

    uint16_t star_col = (s == 0) ? 0xFFE0 : ((s == 1) ? 0x65FF : 0xFA88);
    canvas.drawPixel(sx, sy, star_col);
    canvas.drawPixel(sx + 1, sy, SPR_WHITE);
    canvas.drawPixel(sx, sy + 1, SPR_WHITE);
  }
}

// 10. CHEER CELEBRATION POSE (Jumping victory fists high in the air, beaming smile)
inline void draw_player_cheer(LGFX_Sprite &canvas, int px, int py, int dir,
                             const Player &p, const JerseyPalette &jp,
                             const HairPalette &hp, SportType sport) {
  int cheer_bob = ((p.anim_tick / 5) % 2 == 0) ? -2 : 0;
  int j_py = py + cheer_bob;

  canvas.fillEllipse(px, py + 1, 6, 2, COL_SHADOW);

  int head_y = j_py - 17;
  int torso_y = head_y + 7;
  int shorts_y = torso_y + 5;
  int legs_y = shorts_y + 3;

  // Head with big beaming victory smile!
  int hx = px - 3;
  draw_player_head(canvas, hx, head_y, dir, hp, jp, sport, true);

  // Torso
  draw_player_torso(canvas, px - 4, torso_y, dir, jp, sport);

  // Both Arms Raised High in Victory! (V-shape with clenched fists)
  // Left Arm
  canvas.fillRect(px - 7, torso_y - 4, 3, 5, jp.mid);
  canvas.drawFastVLine(px - 7, torso_y - 4, 5, jp.hi);
  canvas.fillRect(px - 8, torso_y - 7, 4, 3, SPR_SKIN_HI); // Left fist
  // Right Arm
  canvas.fillRect(px + 4, torso_y - 4, 3, 5, jp.mid);
  canvas.drawFastVLine(px + 6, torso_y - 4, 5, jp.shadow);
  canvas.fillRect(px + 4, torso_y - 7, 4, 3, SPR_SKIN_HI); // Right fist

  // Shorts & Bouncing Legs
  draw_player_shorts(canvas, px - 3, shorts_y, dir, jp);

  canvas.fillRect(px - 3, legs_y, 2, 2, SPR_SKIN_MID);
  canvas.fillRect(px - 4, legs_y + 2, 3, 2, SPR_BOOT_MID);
  canvas.fillRect(px + 1, legs_y, 2, 2, SPR_SKIN_MID);
  canvas.fillRect(px + 1, legs_y + 2, 3, 2, SPR_BOOT_MID);
}

// ============================================================================
// Fluid 6-Frame Athletic Stride Cycle with Dynamic Body Lean
// ============================================================================

inline void draw_player_run_stride(LGFX_Sprite &canvas, int px, int py, int dir,
                                  const Player &p, const JerseyPalette &jp,
                                  const HairPalette &hp, SportType sport) {
  // --- 1. Dynamic Body Lean ---
  // Tilt head, torso, and limbs 1-2 pixels along the velocity vector
  int lean_x = 0;
  int lean_y = 0;

  if (p.state == STATE_RUNNING) {
    float abs_vx = fabsf(p.vx);
    if (abs_vx > 0.35f) {
      lean_x = (p.vx > 0 ? 1 : -1) * (abs_vx > 1.8f ? 2 : 1);
    }
    if (fabsf(p.vy) > 0.45f) {
      lean_y = (p.vy > 0 ? 1 : -1);
    }
  }

  // --- 2. 6-Frame Stride Rhythm & Vertical Bob ---
  // Frame 0: Right contact plant (bob = 0)
  // Frame 1: Right knee flex compression dip (bob = +1)
  // Frame 2: Right push-off, high knee lift drive (bob = -1)
  // Frame 3: Left contact plant (bob = 0)
  // Frame 4: Left knee flex compression dip (bob = +1)
  // Frame 5: Left push-off, high knee lift drive (bob = -1)
  int bob_y = 0;
  int f = p.anim_frame % 6;
  if (p.state == STATE_RUNNING) {
    if (f == 1 || f == 4) bob_y = 1;       // Compression dip
    else if (f == 2 || f == 5) bob_y = -1; // Knee lift rise
  }

  // Shadow under feet
  canvas.fillEllipse(px + lean_x, py + 1, 6, 2, COL_SHADOW);

  int head_y = py - 17 + bob_y + lean_y;
  int torso_y = head_y + 7;
  int shorts_y = torso_y + 5;
  int legs_y = shorts_y + 3;

  int hx = px - 3 + lean_x;
  int tx = px - 4 + (lean_x > 0 ? 1 : (lean_x < 0 ? -1 : 0));

  // --- 3. Head & Face ---
  draw_player_head(canvas, hx, head_y, dir, hp, jp, sport, false);

  // --- 4. Arm Pumping Cycle (6 Distinct Stages) ---
  int arm_swing = 0;
  if (p.state == STATE_RUNNING) {
    switch (f) {
      case 0: arm_swing = 2; break;   // Right forward pump
      case 1: arm_swing = 3; break;   // Right peak pump
      case 2: arm_swing = 1; break;   // Right returning
      case 3: arm_swing = -2; break;  // Left forward pump
      case 4: arm_swing = -3; break;  // Left peak pump
      case 5: arm_swing = -1; break;  // Left returning
    }
  }

  if (sport == SPORT_BASKETBALL) {
    // Sleeveless: bare athletic muscular skin arms with highlight
    canvas.fillRect(tx - 2, torso_y + arm_swing, 2, 5, SPR_SKIN_MID);
    canvas.drawPixel(tx - 2, torso_y + arm_swing, SPR_SKIN_HI);
    canvas.fillRect(tx + 8, torso_y - arm_swing, 2, 5, SPR_SKIN_MID);
    canvas.drawPixel(tx + 9, torso_y - arm_swing, SPR_SKIN_SHADOW);
  } else if (sport != SPORT_HOCKEY) {
    // Team-colored jersey sleeves + skin forearms/fists
    canvas.fillRect(tx - 2, torso_y + arm_swing, 2, 4, jp.mid);
    canvas.drawFastVLine(tx - 2, torso_y + arm_swing, 4, jp.hi);
    canvas.fillRect(tx + 8, torso_y - arm_swing, 2, 4, jp.mid);
    canvas.drawFastVLine(tx + 9, torso_y - arm_swing, 4, jp.shadow);

    // Clenched fists with highlight
    canvas.fillRect(tx - 2, torso_y + 4 + arm_swing, 2, 2, SPR_SKIN_HI);
    canvas.fillRect(tx + 8, torso_y + 4 - arm_swing, 2, 2, SPR_SKIN_MID);
  }

  // --- 5. Torso ---
  draw_player_torso(canvas, tx, torso_y, dir, jp, sport);

  // Hockey Stick in Hands (when skating)
  if (sport == SPORT_HOCKEY) {
    int stick_x = px + 4 * dir;
    canvas.drawLine(px, torso_y + 3, stick_x, py + 1, SPR_WOOD_MID);
    canvas.drawLine(px, torso_y + 2, stick_x, py, SPR_WOOD_HI);
    // Blade with tape
    canvas.drawFastHLine(stick_x, py + 1, 4 * dir, SPR_WOOD_MID);
    canvas.drawPixel(stick_x + 2 * dir, py + 1, SPR_WOOD_TAPE);
    // Hands holding stick
    canvas.fillRect(px - 1, torso_y + 2, 3, 3, SPR_SKIN_MID);
    canvas.fillRect(px + 2 * dir, torso_y + 3, 3, 3, SPR_SKIN_HI);
  }

  // Baseball Bat held over shoulder (when batter is running)
  if (sport == SPORT_BASEBALL && p.role == ROLE_FWD) {
    canvas.drawLine(tx - 1 * dir, torso_y - 1, tx - 6 * dir, head_y - 3, SPR_WOOD_MID);
    canvas.drawLine(tx - 2 * dir, torso_y - 1, tx - 7 * dir, head_y - 3, SPR_WOOD_HI);
  }

  // --- 6. Shorts ---
  draw_player_shorts(canvas, tx + 1, shorts_y, dir, jp);

  // --- 7. Legs & Footwear Stride (6-Frame Stride Stances) ---
  int leg_l_y = legs_y;
  int leg_r_y = legs_y;
  int leg_l_x = tx + 1;
  int leg_r_x = tx + 5;

  if (p.state == STATE_RUNNING) {
    if (f == 0) {
      // Right foot plant, Left leg trailing
      leg_r_x += 2 * dir;
      leg_l_x -= 2 * dir;
    } else if (f == 1) {
      // Right leg flexed, Left leg swinging forward
      leg_r_y += 1;
      leg_l_x -= 1 * dir;
    } else if (f == 2) {
      // Left high knee lift! Right leg push-off
      leg_l_y -= 1;
      leg_l_x += 1 * dir;
      leg_r_x -= 2 * dir;
    } else if (f == 3) {
      // Left foot plant, Right leg trailing
      leg_l_x += 2 * dir;
      leg_r_x -= 2 * dir;
    } else if (f == 4) {
      // Left leg flexed, Right leg swinging forward
      leg_l_y += 1;
      leg_r_x -= 1 * dir;
    } else { // f == 5
      // Right high knee lift! Left leg push-off
      leg_r_y -= 1;
      leg_r_x += 1 * dir;
      leg_l_x -= 2 * dir;
    }
  }

  // Render Left & Right Leg & Footwear
  // Left Leg
  canvas.fillRect(leg_l_x, leg_l_y, 2, 3, SPR_SKIN_MID);
  canvas.drawPixel(leg_l_x, leg_l_y, SPR_SKIN_HI);
  canvas.fillRect(leg_l_x - (dir > 0 ? 0 : 1), leg_l_y + 2, 3, 2, SPR_BOOT_MID);
  canvas.drawPixel(leg_l_x, leg_l_y + 2, SPR_BOOT_LACE);

  // Right Leg
  canvas.fillRect(leg_r_x, leg_r_y, 2, 3, SPR_SKIN_MID);
  canvas.drawPixel(leg_r_x, leg_r_y, SPR_SKIN_HI);
  canvas.fillRect(leg_r_x - (dir > 0 ? 0 : 1), leg_r_y + 2, 3, 2, SPR_BOOT_MID);
  canvas.drawPixel(leg_r_x, leg_r_y + 2, SPR_BOOT_LACE);

  // Skate Blades for Hockey with ice sheen glint
  if (sport == SPORT_HOCKEY) {
    canvas.drawFastHLine(leg_l_x - 1, leg_l_y + 4, 4, SPR_SKATE_BLADE_HI);
    canvas.drawFastHLine(leg_l_x - 1, leg_l_y + 5, 4, SPR_SKATE_BLADE_MID);
    canvas.drawFastHLine(leg_r_x - 1, leg_r_y + 4, 4, SPR_SKATE_BLADE_HI);
    canvas.drawFastHLine(leg_r_x - 1, leg_r_y + 5, 4, SPR_SKATE_BLADE_MID);

    // Ice glint sheen on push-off blade
    if (p.state == STATE_RUNNING && (f == 1 || f == 4)) {
      spawn_particle(px - 2 * dir, py + 1, -dir * 1.2f, -0.4f, COL_WHITE, 6);
    }
  } else if (sport == SPORT_BASKETBALL) {
    // White rubber high-top sneaker soles
    canvas.drawFastHLine(leg_l_x - (dir > 0 ? 0 : 1), leg_l_y + 4, 3, SPR_SNEAKER_SOLE);
    canvas.drawFastHLine(leg_r_x - (dir > 0 ? 0 : 1), leg_r_y + 4, 3, SPR_SNEAKER_SOLE);
  }
}

// ============================================================================
// Main 16-Bit Contoured Player Dispatcher (Replaces legacy draw_player)
// ============================================================================

inline void draw_player_16bit_sport(LGFX_Sprite &canvas, const Player &p, SportType sport) {
  int px = (int)p.x;
  int py = (int)p.y;
  int dir = p.facing;

  JerseyPalette jp = get_jersey_palette(p.team);
  HairPalette hp = get_hair_palette(p.hair_color);

  // 1. Knocked Down State
  if (p.state == STATE_DOWN) {
    draw_player_knocked_down(canvas, px, py, dir, p, jp, hp);
    return;
  }

  // 2. Victory Cheer Celebration State
  if (p.state == STATE_CHEER) {
    draw_player_cheer(canvas, px, py, dir, p, jp, hp, sport);
    return;
  }

  // 3. Goalkeeper Horizontal Airborne Dive
  if (p.state == STATE_DIVING) {
    draw_player_soccer_dive(canvas, px, py, dir, p, jp, hp);
    return;
  }

  // 4. Sport-Specific Striking / Shooting Action (STATE_KICKING)
  if (p.state == STATE_KICKING) {
    if (sport == SPORT_SOCCER) {
      draw_player_soccer_kick(canvas, px, py, dir, p, jp, hp);
      return;
    } else if (sport == SPORT_HOCKEY) {
      draw_player_hockey_slapshot(canvas, px, py, dir, p, jp, hp);
      return;
    } else if (sport == SPORT_BASKETBALL) {
      draw_player_basketball_dunk(canvas, px, py, dir, p, jp, hp);
      return;
    } else if (sport == SPORT_BASEBALL) {
      draw_player_baseball_swing(canvas, px, py, dir, p, jp, hp);
      return;
    }
  }

  // 5. Defensive Moves (Slide Tackle / Stick Slash / Jump Block)
  if (p.state == STATE_SLIDE) {
    if (sport == SPORT_BASKETBALL) {
      draw_player_basketball_block(canvas, px, py, dir, p, jp, hp);
      return;
    } else if (sport == SPORT_HOCKEY) {
      draw_player_hockey_slash(canvas, px, py, dir, p, jp, hp);
      return;
    } else {
      // Soccer Slide Tackle (Low grass slide)
      int slide_y = py - 6;
      canvas.fillEllipse(px, py + 1, 8, 3, COL_SHADOW);

      int hx = px - 6 * dir;
      draw_player_head(canvas, hx, slide_y - 2, dir, hp, jp, SPORT_SOCCER, false);

      canvas.fillRect(px, slide_y - 1, 8 * dir, 6, jp.mid);
      canvas.drawFastHLine(px, slide_y - 1, 8 * dir, jp.hi);
      canvas.drawFastHLine(px, slide_y + 4, 8 * dir, jp.shadow);

      canvas.fillRect(px + 7 * dir, slide_y, 4 * dir, 5, SPR_SHORTS_MID);
      canvas.fillRect(px + 10 * dir, slide_y + 1, 6 * dir, 3, SPR_SKIN_MID);
      canvas.fillRect(px + 15 * dir, slide_y, 4 * dir, 4, SPR_BOOT_MID);

      spawn_particle(px - 2 * dir, py - 1, -dir * 1.5f, -0.5f, 0x1462, 8); // Grass spray
      return;
    }
  }

  // 6. Baseball Coiled Batter Stance (Idle Batter at Home Plate)
  if (sport == SPORT_BASEBALL && p.role == ROLE_FWD && p.state == STATE_IDLE) {
    draw_player_baseball_batting(canvas, px, py, dir, p, jp, hp);
    return;
  }

  // 7. Normal 6-Frame Athletic Stride / Skating / Base Running
  draw_player_run_stride(canvas, px, py, dir, p, jp, hp, sport);
}

// Master API conforming directly to requirement: draw_player_16bit(canvas, players[pid])
inline void draw_player_16bit(LGFX_Sprite &canvas, const Player &p) {
  draw_player_16bit_sport(canvas, p, current_sport);
}
