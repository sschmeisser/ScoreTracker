#pragma once

#include <Arduino.h>
#include <LovyanGFX.hpp>

// =============================================================================
// GARFIELD 28x18 PALETTE-INDEXED SPRITES (504 bytes per frame)
// Palette:
// 0: Transparent (0x0001)
// 1: Garfield Orange (255, 145, 0)
// 2: Dark Orange Shadow (210, 95, 0)
// 3: Black Stripes & Outline (20, 20, 25)
// 4: White Eye Glint (255, 255, 255)
// 5: Pink Nose & Ear (255, 120, 140)
// 6: Cream Muzzle (255, 225, 150)
// 7: Sleepy Eye Slit (20, 20, 25)
// =============================================================================

static const uint8_t GARFIELD_MAP_SLEEP[504] PROGMEM = {
  0, 0, 0, 0, 0, 0, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 3, 5, 5, 3, 0, 0, 0, 0, 0, 0, 0, 0, 3, 5, 5, 3, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 3, 1, 5, 5, 1, 3, 3, 3, 3, 3, 3, 3, 3, 1, 5, 5, 1, 3, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 3, 1, 1, 1, 1, 1, 1, 3, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 3, 3, 0, 0, 0, 0,
  0, 0, 0, 3, 1, 1, 1, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 3, 2, 3, 0, 0, 0,
  0, 0, 3, 1, 1, 1, 1, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 3, 2, 3, 0, 0,
  0, 3, 1, 1, 1, 1, 6, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 6, 1, 1, 1, 1, 3, 2, 3, 0,
  3, 1, 1, 1, 1, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 1, 1, 1, 3, 2, 3, 0,
  3, 1, 1, 1, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 1, 3, 3, 3, 2, 3,
  3, 1, 1, 1, 6, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 1, 3, 3, 2, 3, 0,
  3, 1, 1, 1, 6, 6, 7, 7, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 1, 3, 3, 2, 3, 0,
  3, 1, 1, 1, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 1, 3, 3, 2, 3, 0,
  0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 2, 3, 0,
  0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 2, 3, 0, 0,
  0, 0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 3, 3, 2, 3, 0, 0,
  0, 0, 0, 0, 3, 1, 1, 3, 3, 3, 3, 3, 1, 1, 1, 1, 3, 3, 3, 3, 3, 1, 1, 3, 2, 3, 0, 0,
  0, 0, 0, 0, 0, 3, 3, 3, 0, 0, 0, 0, 0, 3, 3, 0, 0, 0, 0, 0, 0, 3, 3, 3, 3, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static const uint8_t GARFIELD_MAP_CRAWL0[504] PROGMEM = {
  0, 0, 0, 0, 0, 0, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 3, 5, 5, 3, 0, 0, 0, 0, 0, 0, 0, 0, 3, 5, 5, 3, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 3, 1, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 1, 1, 3, 0, 0, 0, 0, 0,
  0, 0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 3, 3, 0, 0, 0,
  0, 0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 3, 2, 3, 0, 0,
  0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 3, 2, 3, 0, 0,
  0, 3, 1, 1, 1, 1, 6, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 6, 1, 1, 1, 1, 3, 2, 3, 0,
  3, 1, 1, 1, 1, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 1, 1, 1, 3, 2, 3, 0,
  3, 1, 1, 1, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 1, 3, 3, 3, 2, 3,
  3, 1, 1, 1, 6, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 1, 3, 3, 2, 3, 0,
  3, 1, 1, 1, 6, 6, 4, 4, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 1, 3, 3, 2, 3, 0,
  0, 3, 1, 1, 6, 6, 7, 7, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 1, 3, 3, 2, 3, 0,
  0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 2, 3, 0,
  0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 2, 3, 0, 0,
  0, 3, 1, 1, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 1, 1, 3, 3, 0, 0, 0, 3, 2, 3, 0, 0,
  0, 3, 1, 1, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 1, 1, 3, 3, 0, 0, 0, 0, 3, 3, 0, 0,
  3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static const uint8_t GARFIELD_MAP_CRAWL1[504] PROGMEM = {
  0, 0, 0, 0, 0, 0, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 3, 5, 5, 3, 0, 0, 0, 0, 0, 0, 0, 0, 3, 5, 5, 3, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 3, 1, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 1, 1, 3, 0, 0, 0, 0, 0,
  0, 0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 3, 3, 0, 0, 0,
  0, 0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 3, 2, 3, 0, 0,
  0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 3, 2, 3, 0, 0,
  0, 3, 1, 1, 1, 1, 6, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 6, 1, 1, 1, 1, 3, 2, 3, 0,
  3, 1, 1, 1, 1, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 1, 1, 1, 3, 2, 3, 0,
  3, 1, 1, 1, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 1, 3, 3, 3, 2, 3,
  3, 1, 1, 1, 6, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 1, 3, 3, 2, 3, 0,
  3, 1, 1, 1, 6, 6, 4, 4, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 1, 3, 3, 2, 3, 0,
  0, 3, 1, 1, 6, 6, 7, 7, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 1, 3, 3, 2, 3, 0,
  0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 2, 3, 0,
  0, 0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 2, 3, 0, 0,
  0, 0, 0, 0, 3, 1, 1, 3, 3, 3, 3, 3, 1, 1, 1, 1, 3, 3, 3, 3, 3, 1, 1, 3, 2, 3, 0, 0,
  0, 0, 0, 0, 3, 1, 1, 3, 3, 3, 3, 3, 1, 1, 1, 1, 3, 3, 3, 3, 3, 1, 1, 3, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 3, 3, 3, 0, 0, 0, 0, 0, 3, 3, 3, 0, 0, 0, 3, 3, 3, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

// =============================================================================
// GARFIELD CONTROLLER & STATE MACHINE
// =============================================================================

enum GarfieldState {
  GARFIELD_SLEEP_ON_HOUR = 0,   // Sleeping lazily on the hour hand
  GARFIELD_WAKING,              // Yawning and waking up
  GARFIELD_CRAWL_IN_HOUR,       // Crawling along hour hand towards center
  GARFIELD_TURN_TO_MINUTE,      // Stepping across to minute hand at arbor
  GARFIELD_CRAWL_OUT_MINUTE,    // Crawling out along minute hand
  GARFIELD_CHILL_ON_MINUTE,     // Hanging out / catnap on minute hand
  GARFIELD_CRAWL_IN_MINUTE,     // Crawling along minute hand towards center
  GARFIELD_TURN_TO_HOUR,        // Stepping across to hour hand at arbor
  GARFIELD_CRAWL_OUT_HOUR       // Crawling out to favorite spot on hour hand
};

struct ZParticle {
  float x;
  float y;
  float vx;
  float vy;
  int life;
  char ch;
};

static inline uint16_t make_rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint16_t)(r >> 3) << 11) | ((uint16_t)(g >> 2) << 5) | (b >> 3);
}

static inline uint16_t make_sprite_color(uint8_t r, uint8_t g, uint8_t b) {
  // LovyanGFX sprite buffer uses swap565 (big-endian 16-bit)
  uint16_t col = make_rgb565(r, g, b);
  return (col >> 8) | (col << 8);
}

class GarfieldManager {
public:
  GarfieldState state = GARFIELD_CRAWL_OUT_HOUR;
  float cur_r = 15.0f;
  float cur_angle = 0.0f;
  uint32_t state_timer = 0;
  uint32_t crawl_frame_counter = 0;
  uint8_t crawl_anim_frame = 0;

  // Palette array (0..7) mapped to sprite memory format
  uint16_t pal[8];

  // Sleeping Z particles
  static const int MAX_Z = 3;
  ZParticle z_particles[MAX_Z];
  uint32_t last_z_spawn = 0;

  // LGFX Sprite for rotated rendering (28x18)
  LGFX_Sprite spr;

  void init(LovyanGFX* parent) {
    pal[0] = 0x0000;                              // Transparent (0x0000)
    pal[1] = make_sprite_color(255, 140, 0);      // Pure Garfield Orange (#FF8C00)
    pal[2] = make_sprite_color(205, 85, 0);       // Dark Orange Shadow
    pal[3] = make_sprite_color(20, 20, 25);       // Black stripes & outlines
    pal[4] = make_sprite_color(255, 255, 255);    // White eye glint
    pal[5] = make_sprite_color(255, 120, 140);    // Pink nose & ears
    pal[6] = make_sprite_color(255, 225, 150);    // Cream muzzle
    pal[7] = make_sprite_color(20, 20, 25);       // Eye slit

    spr.setColorDepth(16);
    spr.createSprite(28, 18);
    spr.setPivot(14, 9); // Center pivot

    for (int i = 0; i < MAX_Z; i++) {
      z_particles[i].life = 0;
    }

    // Start crawling out along the hour hand towards his sleeping spot
    state = GARFIELD_CRAWL_OUT_HOUR;
    cur_r = 12.0f;
  }

  // Shortest angular difference (-180 to +180)
  float angle_diff(float a, float b) {
    float d = fmodf(b - a + 540.0f, 360.0f) - 180.0f;
    return d;
  }

  void update(float hour_angle, float min_angle) {
    crawl_frame_counter++;
    if (crawl_frame_counter % 12 == 0) {
      crawl_anim_frame = (crawl_anim_frame == 0) ? 1 : 0;
    }

    switch (state) {
      // -------------------------------------------------------------
      // 1. SLEEPING LAZILY ON HOUR HAND
      // -------------------------------------------------------------
      case GARFIELD_SLEEP_ON_HOUR: {
        cur_r = 36.0f;
        cur_angle = hour_angle;

        // Spawn sleepy "Z" particles
        if (millis() - last_z_spawn > 900) {
          last_z_spawn = millis();
          for (int i = 0; i < MAX_Z; i++) {
            if (z_particles[i].life <= 0) {
              float rad = (cur_angle - 90.0f) * (PI / 180.0f);
              float gx = 120.0f + cur_r * cosf(rad);
              float gy = 120.0f + cur_r * sinf(rad);
              z_particles[i].x = gx;
              z_particles[i].y = gy;
              z_particles[i].vx = ((rand() % 10) - 5) * 0.05f;
              z_particles[i].vy = -0.45f - ((rand() % 5) * 0.05f);
              z_particles[i].life = 45;
              z_particles[i].ch = (i % 2 == 0) ? 'Z' : 'z';
              break;
            }
          }
        }

        if (millis() >= state_timer) {
          state = GARFIELD_WAKING;
          state_timer = millis() + 2000;
          Serial.println("[GARFIELD] Yawning and stretching awake...");
        }
        break;
      }

      // -------------------------------------------------------------
      // 2. WAKING UP
      // -------------------------------------------------------------
      case GARFIELD_WAKING: {
        cur_r = 36.0f;
        cur_angle = hour_angle;
        if (millis() >= state_timer) {
          state = GARFIELD_CRAWL_IN_HOUR;
          Serial.println("[GARFIELD] Crawling from hour hand towards center arbor...");
        }
        break;
      }

      // -------------------------------------------------------------
      // 3. CRAWLING INWARD ON HOUR HAND TOWARDS ARBOR
      // -------------------------------------------------------------
      case GARFIELD_CRAWL_IN_HOUR: {
        cur_angle = hour_angle;
        cur_r -= 0.35f;
        if (cur_r <= 12.0f) {
          cur_r = 12.0f;
          state = GARFIELD_TURN_TO_MINUTE;
          state_timer = 40;
          Serial.println("[GARFIELD] Stepping across from hour hand to minute hand...");
        }
        break;
      }

      // -------------------------------------------------------------
      // 4. TURNING / STEPPING ACROSS TO MINUTE HAND AT ARBOR
      // -------------------------------------------------------------
      case GARFIELD_TURN_TO_MINUTE: {
        cur_r = 12.0f;
        float diff = angle_diff(cur_angle, min_angle);
        cur_angle += diff * 0.12f;
        if (state_timer > 0) state_timer--;
        if (fabsf(diff) < 2.0f || state_timer == 0) {
          cur_angle = min_angle;
          state = GARFIELD_CRAWL_OUT_MINUTE;
          Serial.println("[GARFIELD] Crawling outward along the minute hand...");
        }
        break;
      }

      // -------------------------------------------------------------
      // 5. CRAWLING OUTWARD ALONG MINUTE HAND
      // -------------------------------------------------------------
      case GARFIELD_CRAWL_OUT_MINUTE: {
        cur_angle = min_angle;
        cur_r += 0.38f;
        if (cur_r >= 52.0f) {
          cur_r = 52.0f;
          state = GARFIELD_CHILL_ON_MINUTE;
          state_timer = millis() + (8000 + (rand() % 14000));
          Serial.printf("[GARFIELD] Chilling on the minute hand for %lu seconds\n",
                        (state_timer - millis()) / 1000);
        }
        break;
      }

      // -------------------------------------------------------------
      // 6. CHILLING / CATNAP ON MINUTE HAND
      // -------------------------------------------------------------
      case GARFIELD_CHILL_ON_MINUTE: {
        cur_r = 52.0f;
        cur_angle = min_angle;
        if (millis() >= state_timer) {
          state = GARFIELD_CRAWL_IN_MINUTE;
          Serial.println("[GARFIELD] Crawling from minute hand back to hour hand to sleep...");
        }
        break;
      }

      // -------------------------------------------------------------
      // 7. CRAWLING INWARD ALONG MINUTE HAND TOWARDS ARBOR
      // -------------------------------------------------------------
      case GARFIELD_CRAWL_IN_MINUTE: {
        cur_angle = min_angle;
        cur_r -= 0.35f;
        if (cur_r <= 12.0f) {
          cur_r = 12.0f;
          state = GARFIELD_TURN_TO_HOUR;
          state_timer = 40;
          Serial.println("[GARFIELD] Stepping across arbor back to hour hand...");
        }
        break;
      }

      // -------------------------------------------------------------
      // 8. TURNING / STEPPING ACROSS TO HOUR HAND AT ARBOR
      // -------------------------------------------------------------
      case GARFIELD_TURN_TO_HOUR: {
        cur_r = 12.0f;
        float diff = angle_diff(cur_angle, hour_angle);
        cur_angle += diff * 0.12f;
        if (state_timer > 0) state_timer--;
        if (fabsf(diff) < 2.0f || state_timer == 0) {
          cur_angle = hour_angle;
          state = GARFIELD_CRAWL_OUT_HOUR;
          Serial.println("[GARFIELD] Crawling to sleeping spot on the hour hand...");
        }
        break;
      }

      // -------------------------------------------------------------
      // 9. CRAWLING OUTWARD ALONG HOUR HAND TO SLEEPING SPOT
      // -------------------------------------------------------------
      case GARFIELD_CRAWL_OUT_HOUR: {
        cur_angle = hour_angle;
        cur_r += 0.35f;
        if (cur_r >= 30.0f) {
          cur_r = 30.0f;
          state = GARFIELD_SLEEP_ON_HOUR;
          state_timer = millis() + (18000 + (rand() % 27000));
          Serial.printf("[GARFIELD] Curled up to sleep on hour hand for %lu seconds z Z Z\n",
                        (state_timer - millis()) / 1000);
        }
        break;
      }
    }

    // Update active Z particles
    for (int i = 0; i < MAX_Z; i++) {
      if (z_particles[i].life > 0) {
        z_particles[i].x += z_particles[i].vx;
        z_particles[i].y += z_particles[i].vy;
        z_particles[i].life--;
      }
    }
  }

  // Wake up Garfield immediately if user touches button
  void poke() {
    if (state == GARFIELD_SLEEP_ON_HOUR) {
      state = GARFIELD_WAKING;
      state_timer = millis() + 1000;
      Serial.println("[GARFIELD] Poked awake by button tap!");
    }
  }

  void render(LGFX_Sprite& canvas) {
    const uint8_t* map_ptr = GARFIELD_MAP_SLEEP;
    float render_angle = cur_angle - 90.0f;

    if (state == GARFIELD_SLEEP_ON_HOUR || state == GARFIELD_WAKING) {
      map_ptr = GARFIELD_MAP_SLEEP;
      // Gentle breathing heave when asleep
      if (state == GARFIELD_SLEEP_ON_HOUR && ((crawl_frame_counter / 40) % 2 == 1)) {
        render_angle += 1.0f;
      }
    } else if (state == GARFIELD_CHILL_ON_MINUTE) {
      map_ptr = GARFIELD_MAP_SLEEP;
    } else {
      map_ptr = (crawl_anim_frame == 0) ? GARFIELD_MAP_CRAWL0 : GARFIELD_MAP_CRAWL1;
      if (state == GARFIELD_CRAWL_IN_HOUR || state == GARFIELD_CRAWL_IN_MINUTE) {
        render_angle += 180.0f;
      }
    }

    // Unpack indexed map into sprite buffer using native color palette
    uint16_t* dst = (uint16_t*)spr.getBuffer();
    for (int i = 0; i < 504; i++) {
      uint8_t c_idx = pgm_read_byte(&map_ptr[i]);
      dst[i] = pal[c_idx];
    }

    // Compute screen position
    float rad = (cur_angle - 90.0f) * (PI / 180.0f);
    float gx = 120.0f + cur_r * cosf(rad);
    float gy = 120.0f + cur_r * sinf(rad);

    // Render rotated sprite (nearest-neighbor preserves crisp pixel art edges without blur)
    spr.pushRotateZoom(&canvas, gx, gy, render_angle, 1.0f, 1.0f, pal[0]);

    // Render floating Z particles when sleeping
    if (state == GARFIELD_SLEEP_ON_HOUR) {
      canvas.setFont(&fonts::DejaVu9);
      canvas.setTextDatum(textdatum_t::middle_center);
      for (int i = 0; i < MAX_Z; i++) {
        if (z_particles[i].life > 0) {
          uint16_t col = (z_particles[i].life > 20) ? 0x9E7F : 0x4B3B;
          canvas.setTextColor(col);
          char str[2] = { z_particles[i].ch, '\0' };
          canvas.drawString(str, (int)z_particles[i].x, (int)z_particles[i].y);
        }
      }
    }
  }
};
