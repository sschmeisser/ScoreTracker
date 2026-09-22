#include <Arduino.h>
#include <Preferences.h>
#include "LGFX_config.h"
#include "stadium_render.h"
#include "player_sprites.h"
#include <esp_sleep.h>

LGFX lcd;
LGFX_Sprite canvas(&lcd);
Preferences prefs;

// ============================================================================
// Palette & Colors (RGB565)
// ============================================================================
// General
#define COL_BLACK       0x0000
#define COL_WHITE       0xFFFF
#define COL_GOLD        0xFFE0
#define COL_HUD_BG      0x0841
#define COL_SHADOW      0x0A22

// Skin & Hair
#define COL_SKIN        0xFDF0
#define COL_SKIN_SHADOW 0xDC48
#define COL_HAIR_BLK    0x1082
#define COL_HAIR_BRN    0x79A0
#define COL_HAIR_BLD    0xFDE0

// Teams
#define COL_RED_TEAM    0xF800
#define COL_RED_DARK    0x9800
#define COL_BLU_TEAM    0x1BDF
#define COL_BLU_DARK    0x0215
#define COL_SHORTS      0xFFFF
#define COL_BOOTS       0x18C3

// Soccer
#define COL_PITCH_A     0x24A5
#define COL_PITCH_B     0x1C23
#define COL_LINE        0xFFFF
#define COL_NET         0xC618
#define COL_BALL        0xFFFF
#define COL_BALL_PATCH  0x18C3

// Ice Hockey
#define COL_ICE_A       0xFFFF  // Brilliant pure white glossy ice sheet
#define COL_ICE_B       0xFFFF  // Solid uniform ice (NO STRIPES)
#define COL_ICE_LINE_R  0xD904  // Red line / goal crease
#define COL_ICE_LINE_B  0x2B7D  // Blue zone lines
#define COL_PUCK        0x1082  // Black hockey puck
#define COL_STICK_WOOD  0xC443  // Wooden hockey stick
#define COL_BLADE_SILV  0xD6BA  // Skate blade silver

// Basketball
#define COL_WOOD_A      0xDC88  // Polished hardwood floor light
#define COL_WOOD_B      0xCBE4  // Hardwood floor dark stripe
#define COL_COURT_LINE  0xFFFF  // Court lines
#define COL_BASKET_BALL 0xF3C0  // Classic basketball orange
#define COL_BASKET_SEAM 0x2945  // Ball black seams
#define COL_HOOP_RIM    0xF800  // Red hoop rim
#define COL_BACKBOARD   0xFFFF  // White backboard

// Baseball
#define COL_DIRT_A      0xD3C6  // Warm infield clay dirt light
#define COL_DIRT_B      0xBA63  // Infield clay dirt dark
#define COL_BASE_WHITE  0xFFFF  // Bases white
#define COL_CHALK_WHITE 0xFFFF  // Foul line chalk
#define COL_FENCE_DARK  0x2124  // Outfield fence

// Fast integer trigonometry
static int16_t sin_tab[256];
static void init_sine_table() {
  for (int i = 0; i < 256; i++) {
    sin_tab[i] = (int16_t)(sinf(i * (2.0f * PI / 256.0f)) * 256.0f);
  }
}
static inline int16_t isin(uint8_t a) { return sin_tab[a]; }
static inline int16_t icos(uint8_t a) { return sin_tab[(uint8_t)(a + 64)]; }

// ============================================================================
// Multi-Sport Architecture
// ============================================================================
SportType current_sport = SPORT_SOCCER;

#define NUM_PLAYERS 6
Player players[NUM_PLAYERS];

// Ball / Puck
struct Ball {
  float x, y, z;
  float vx, vy, vz;
  bool is_super;
  int owner; // -1 if loose, else player index
};
Ball ball;

// Match Stats & Scores
int score_red = 0;
int score_blu = 0;
int match_seconds = 0;
int match_ticks = 0;

// Goal / Score Event Banner
int goal_banner_timer = 0;
int goal_scoring_team = 0;
bool sport_transition_pending = false;

// Easter egg flash on time clock ("avan", "merdan", "rojda")
static const char* flash_names[] = { "avan", "merdan", "rojda" };
static int flash_text_timer = 0;       // Remaining frames for 1s flash (~32 frames)
static int next_flash_countdown = 90;  // Random countdown until next flash
static int active_flash_idx = 0;

// Particles (Grass/ice spray, super shot spark, stars)
struct Particle {
  float x, y, vx, vy;
  uint16_t color;
  uint8_t life;
};
#define MAX_PARTICLES 35
Particle particles[MAX_PARTICLES];

void spawn_particle(float x, float y, float vx, float vy, uint16_t color, uint8_t life) {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (particles[i].life == 0) {
      particles[i].x = x;
      particles[i].y = y;
      particles[i].vx = vx;
      particles[i].vy = vy;
      particles[i].color = color;
      particles[i].life = life;
      break;
    }
  }
}

// Input button: ESP32-C3 BOOT button is GPIO 9 (active LOW)
static const int BOOT_BTN_PIN = 9;

// Long-press evasion & button gesture state
static bool btn_was_down = false;
static uint32_t btn_down_start = 0;
static bool long_press_active = false;
static bool long_press_fired = false;
static int evade_direction = 0; // -1 = UP (towards top wall), 1 = DOWN (towards bottom wall)

// Baseball State Machine & Batting Mechanics
enum BaseballState {
  BB_READY,        // Pitcher on mound holding ball, windup timer
  BB_PITCHING,     // Fastball traveling down toward plate (vy = +3.8f)
  BB_CAUGHT,       // Catcher caught ball behind plate
  BB_THROW_BACK,   // Catcher tosses ball back to pitcher
  BB_HIT_IN_PLAY,  // Base hit / line drive into outfield
  BB_HOMERUN       // CRACK! Towering home run into outfield bleachers!
};

static BaseballState bb_state = BB_READY;
static int bb_timer = 45;
static bool batter_swung = false;
static int hit_feedback_type = 0; // 0=none, 1=PERFECT, 2=GOOD, 3=MISS
static int hit_feedback_timer = 0;
static int base_run_target = 0;
static int screen_flash_timer = 0;

// Vertical Batting Timing Gauge at x=208, y=155 to 205 (exact middle = 180)
static const int GAUGE_X = 208;
static const int GAUGE_Y = 155;
static const int GAUGE_W = 7;
static const int GAUGE_H = 50;
static const int GAUGE_MID_Y = 180;
static const int GAUGE_Y_TOP = GAUGE_Y;
static const int GAUGE_Y_BOT = GAUGE_Y + GAUGE_H;
static const int GAUGE_SWEET_Y = GAUGE_MID_Y;
static bool baseball_pitch_active = false;
static float baseball_floater_y = 155.0f;

// ============================================================================
// 2-Minute Inactivity Sleep / Shutdown System
// ============================================================================
static uint32_t last_button_activity_ms = 0;
static const uint32_t INACTIVITY_TIMEOUT_MS = 120000; // 2 minutes (120,000 ms)

void enter_device_sleep() {
  Serial.println("Inactivity timeout (2 min). Shutting down device to deep sleep...");

  // Visual shutdown confirmation on round display
  canvas.fillScreen(COL_BLACK);
  canvas.drawRect(24, 92, 192, 56, COL_GOLD);
  canvas.fillRect(26, 94, 188, 52, COL_HUD_BG);
  canvas.setTextSize(1);
  canvas.setTextColor(COL_GOLD, COL_HUD_BG);
  canvas.setCursor(85, 104);
  canvas.print("SLEEP MODE");
  canvas.setTextColor(COL_WHITE, COL_HUD_BG);
  canvas.setCursor(54, 122);
  canvas.print("PRESS BOOT TO WAKE");
  canvas.pushSprite(0, 0);

  delay(1200);

  // Fade out backlight
  for (int b = 255; b >= 0; b -= 25) {
    lcd.setBrightness(b);
    delay(15);
  }
  lcd.setBrightness(0);
  lcd.sleep();

  // Clear display
  canvas.fillScreen(COL_BLACK);
  canvas.pushSprite(0, 0);

  // Configure BOOT button (GPIO 9) as low-level deep sleep wakeup trigger
  esp_deep_sleep_enable_gpio_wakeup((1ULL << BOOT_BTN_PIN), ESP_GPIO_WAKEUP_GPIO_LOW);

  // Enter deep sleep
  esp_deep_sleep_start();
}

// ============================================================================
// Reset & Kickoff Setup
// ============================================================================
void reset_kickoff(int kickoff_team) {
  goal_banner_timer = 0;
  goal_scoring_team = 0;
  ball.x = 120;
  ball.y = 129;
  ball.z = 0;
  ball.vx = 0;
  ball.vy = 0;
  ball.vz = 0;
  ball.is_super = false;
  ball.owner = -1;

  // Red Team (attacks towards Right)
  players[0].x = 32;  players[0].y = 129; players[0].team = 0; players[0].role = ROLE_GK;
  players[0].facing = 1; players[0].state = STATE_IDLE; players[0].hair_color = COL_HAIR_BLK;
  players[0].lane_y = 129; players[0].weave_phase = 0;

  players[1].x = (kickoff_team == 0) ? 116 : 85; players[1].y = 95; players[1].team = 0; players[1].role = ROLE_FWD;
  players[1].facing = 1; players[1].state = STATE_IDLE; players[1].hair_color = COL_HAIR_BLK;
  players[1].lane_y = 85; players[1].weave_phase = 20;

  players[2].x = 65;  players[2].y = 170; players[2].team = 0; players[2].role = ROLE_DEF;
  players[2].facing = 1; players[2].state = STATE_IDLE; players[2].hair_color = COL_HAIR_BRN;
  players[2].lane_y = 175; players[2].weave_phase = 110;

  // Blue Team (attacks towards Left)
  players[3].x = 208; players[3].y = 129; players[3].team = 1; players[3].role = ROLE_GK;
  players[3].facing = -1; players[3].state = STATE_IDLE; players[3].hair_color = COL_HAIR_BRN;
  players[3].lane_y = 129; players[3].weave_phase = 128;

  players[4].x = (kickoff_team == 1) ? 124 : 155; players[4].y = 165; players[4].team = 1; players[4].role = ROLE_FWD;
  players[4].facing = -1; players[4].state = STATE_IDLE; players[4].hair_color = COL_HAIR_BLD;
  players[4].lane_y = 170; players[4].weave_phase = 150;

  players[5].x = 175; players[5].y = 85;  players[5].team = 1; players[5].role = ROLE_DEF;
  players[5].facing = -1; players[5].state = STATE_IDLE; players[5].hair_color = COL_HAIR_BLK;
  players[5].lane_y = 80; players[5].weave_phase = 220;

  for (int i = 0; i < NUM_PLAYERS; i++) {
    players[i].vx = 0;
    players[i].vy = 0;
    players[i].state_timer = 0;
    players[i].anim_frame = 0;
    players[i].anim_tick = 0;
  }

  // Baseball Diamond Positioning
  if (current_sport == SPORT_BASEBALL) {
    // Catcher (Player 0, Blue team) behind home plate at (120, 206)
    players[0].x = 120; players[0].y = 206; players[0].team = 1; players[0].role = ROLE_GK;
    players[0].lane_y = 206; players[0].facing = 1; players[0].state = STATE_IDLE;

    // Batter (Player 1, Red team) in batter's box next to home plate at (110, 193) facing right
    players[1].x = 110; players[1].y = 193; players[1].team = 0; players[1].role = ROLE_FWD;
    players[1].lane_y = 193; players[1].facing = 1; players[1].state = STATE_IDLE;

    // 3rd Baseman (Player 2, Blue team) stationed at (66, 143)
    players[2].x = 66;  players[2].y = 143; players[2].team = 1; players[2].role = ROLE_DEF;
    players[2].lane_y = 143; players[2].facing = 1; players[2].state = STATE_IDLE;

    // Pitcher (Player 3, Blue team) on mound at (120, 140)
    players[3].x = 120; players[3].y = 140; players[3].team = 1; players[3].role = ROLE_DEF;
    players[3].lane_y = 140; players[3].facing = 1; players[3].state = STATE_IDLE;

    // 2nd Baseman (Player 4, Blue team) stationed at (118, 93)
    players[4].x = 118; players[4].y = 93;  players[4].team = 1; players[4].role = ROLE_DEF;
    players[4].lane_y = 93;  players[4].facing = 1; players[4].state = STATE_IDLE;

    // 1st Baseman (Player 5, Blue team) stationed at (170, 143)
    players[5].x = 170; players[5].y = 143; players[5].team = 1; players[5].role = ROLE_DEF;
    players[5].lane_y = 143; players[5].facing = -1; players[5].state = STATE_IDLE;

    // Pitcher begins with the ball on the mound
    ball.owner = 3;
    ball.x = 120.0f;
    ball.y = 140.0f;
    ball.z = 0.0f;
    ball.vx = 0.0f;
    ball.vy = 0.0f;
    ball.vz = 0.0f;
    ball.is_super = false;

    bb_state = BB_READY;
    bb_timer = 45;
    batter_swung = false;
    hit_feedback_timer = 0;
    hit_feedback_type = 0;
    base_run_target = 0;
    screen_flash_timer = 0;
    baseball_floater_y = 155.0f;
  }
}

// Switch to next sport upon scoring a goal / home run
void switch_to_next_sport() {
  int next = ((int)current_sport + 1) % 4;
  current_sport = (SportType)next;
  prefs.putInt("last_sport", next);
  reset_kickoff(rand() % 2);
}

// ============================================================================
// Pitch / Rink / Court Drawing Functions
// ============================================================================

void draw_stadium_floor() {
  render_stadium_surface(canvas, current_sport);
  apply_circular_vignette(canvas);
}

// ============================================================================
// Kunio-kun Player Rendering (Phase 2: 16-Bit Contoured Models)
// ============================================================================
void draw_player(const Player &p) {
  draw_player_16bit(canvas, p);
}

// ============================================================================
// Ball / Puck Drawing
// ============================================================================
void draw_ball() {
  int bx = (int)ball.x;
  int by = (int)ball.y;
  int bz = (int)ball.z;
  int ball_draw_y = by - bz;

  // Ball shadow
  canvas.fillEllipse(bx, by, 3, 2, COL_SHADOW);

  if (current_sport == SPORT_SOCCER) {
    if (ball.is_super) {
      uint16_t s_col = ((millis() / 50) % 2 == 0) ? COL_GOLD : COL_RED_TEAM;
      canvas.fillCircle(bx - (int)(ball.vx * 1.5f), ball_draw_y - (int)(ball.vy * 1.5f), 3, s_col);
      canvas.fillCircle(bx, ball_draw_y, 4, COL_WHITE);
      canvas.drawCircle(bx, ball_draw_y, 4, s_col);
    } else {
      canvas.fillCircle(bx, ball_draw_y, 3, COL_BALL);
      canvas.drawPixel(bx, ball_draw_y, COL_BALL_PATCH);
      canvas.drawPixel(bx - 1, ball_draw_y - 1, COL_BALL_PATCH);
      canvas.drawPixel(bx + 1, ball_draw_y + 1, COL_BALL_PATCH);
    }
  } else if (current_sport == SPORT_HOCKEY) {
    // Puck (flat 4x2 black disc)
    if (ball.is_super) {
      canvas.fillRect(bx - 3, ball_draw_y - 1, 6, 3, COL_RED_TEAM);
      spawn_particle(bx, ball_draw_y, -ball.vx * 0.4f, 0, COL_GOLD, 8);
    } else {
      canvas.fillRect(bx - 2, ball_draw_y - 1, 5, 3, COL_PUCK);
    }
  } else if (current_sport == SPORT_BASKETBALL) {
    // Basketball (orange with black cross seams)
    if (ball.is_super) {
      uint16_t s_col = ((millis() / 50) % 2 == 0) ? COL_GOLD : 0xFD20;
      canvas.fillCircle(bx - (int)(ball.vx * 1.5f), ball_draw_y - (int)(ball.vy * 1.5f), 4, s_col);
      canvas.fillCircle(bx, ball_draw_y, 4, COL_BASKET_BALL);
    } else {
      canvas.fillCircle(bx, ball_draw_y, 4, COL_BASKET_BALL);
      canvas.drawFastHLine(bx - 3, ball_draw_y, 7, COL_BASKET_SEAM);
      canvas.drawFastVLine(bx, ball_draw_y - 3, 7, COL_BASKET_SEAM);
    }
  } else if (current_sport == SPORT_BASEBALL) {
    // White baseball with red seam
    if (ball.is_super) {
      uint16_t s_col = ((millis() / 50) % 2 == 0) ? COL_GOLD : COL_RED_TEAM;
      canvas.fillCircle(bx - (int)(ball.vx * 1.5f), ball_draw_y - (int)(ball.vy * 1.5f), 3, s_col);
      canvas.fillCircle(bx, ball_draw_y, 3, COL_WHITE);
      spawn_particle(bx, ball_draw_y, 0, 1.0f, COL_GOLD, 8);
    } else {
      canvas.fillCircle(bx, ball_draw_y, 3, COL_WHITE);
      canvas.drawPixel(bx - 1, ball_draw_y, COL_HOOP_RIM);
      canvas.drawPixel(bx + 1, ball_draw_y, COL_HOOP_RIM);
    }
  }
}

// ============================================================================
// Top Arcade Scoreboard HUD
// ============================================================================
void draw_hud() {
  canvas.fillRect(25, 6, 190, 18, COL_HUD_BG);
  canvas.drawRect(25, 6, 190, 18, COL_GOLD);

  canvas.setTextColor(COL_GOLD, COL_HUD_BG);
  canvas.setTextSize(1);

  // RED Score
  canvas.fillRect(30, 9, 8, 8, COL_RED_TEAM);
  canvas.setCursor(42, 11);
  canvas.printf("RED %d", score_red);

  // Sport Name Badge or Easter Egg Flash ("avan", "merdan", "rojda")
  if (flash_text_timer > 0) {
    const char* name = flash_names[active_flash_idx];
    int tlen = strlen(name);
    int tx = 120 - (tlen * 6) / 2;
    canvas.fillRect(92, 7, 56, 16, ((flash_text_timer / 4) % 2 == 0) ? 0x0A2B : COL_HUD_BG);
    uint16_t fcol = ((flash_text_timer / 3) % 2 == 0) ? COL_GOLD : COL_WHITE;
    canvas.setTextColor(fcol);
    canvas.setCursor(tx, 11);
    canvas.print(name);
  } else {
    // Current Sport Name
    const char* sname = (current_sport == SPORT_SOCCER) ? "SOCCER" :
                        (current_sport == SPORT_HOCKEY) ? "HOCKEY" :
                        (current_sport == SPORT_BASKETBALL) ? "HOOPS" : "HARDBALL";
    int tlen = strlen(sname);
    int tx = 120 - (tlen * 6) / 2;
    canvas.setTextColor(COL_GOLD, COL_HUD_BG);
    canvas.setCursor(tx, 11);
    canvas.print(sname);
  }

  // BLUE Score
  canvas.setCursor(158, 11);
  canvas.printf("%d BLU", score_blu);
  canvas.fillRect(198, 9, 8, 8, COL_BLU_TEAM);

  // Physical Button Action Prompt on bottom bezel
  // Physical Button Action Prompt on bottom bezel
  if (current_sport == SPORT_BASEBALL) {
    canvas.setTextColor(COL_GOLD, COL_BLACK);
    canvas.setCursor(40, 228);
    canvas.print("BOOT: TIME SWING TO HIT!");
  } else if (long_press_active) {
    canvas.fillRect(48, 226, 144, 12, COL_HUD_BG);
    canvas.drawRect(48, 226, 144, 12, COL_GOLD);
    canvas.setTextColor(COL_GOLD, COL_HUD_BG);
    canvas.setCursor(56, 228);
    canvas.printf(">>> EVADING %s! <<<", (evade_direction < 0 ? "UP" : "DOWN"));
  } else {
    canvas.setTextColor(0x5AEB, COL_BLACK);
    canvas.setCursor(44, 228);
    canvas.print("BOOT: TAP=SHOT HOLD=EVADE");
  }
}

// Draw Big Retro Score / Goal Banner
void draw_goal_banner() {
  if (goal_banner_timer <= 0) return;

  int flash = (goal_banner_timer / 6) % 2;
  uint16_t box_bg = flash ? COL_RED_TEAM : 0x001F;
  uint16_t text_col = flash ? COL_GOLD : COL_WHITE;

  canvas.fillRect(30, 94, 180, 42, box_bg);
  canvas.drawRect(30, 94, 180, 42, COL_GOLD);
  canvas.drawRect(32, 96, 176, 38, COL_WHITE);

  canvas.setTextColor(text_col, box_bg);
  canvas.setTextSize(2);

  if (current_sport == SPORT_BASKETBALL) {
    canvas.setCursor(44, 100);
    canvas.print("SLAM DUNK!");
  } else if (current_sport == SPORT_BASEBALL) {
    canvas.setCursor(38, 100);
    canvas.print("HOME RUN ! ! !");
  } else {
    canvas.setCursor(55, 100);
    canvas.print("GOAL ! ! !");
  }

  // Easter Egg MVP Callout ("AVAN", "MERDAN", "ROJDA")
  static const char* easter_heroes[] = { "AVAN", "MERDAN", "ROJDA" };
  int hero_idx = (goal_scoring_team == 0) ? ((score_red + match_seconds) % 3) : ((score_blu + match_seconds) % 3);
  canvas.setTextSize(1);
  canvas.setTextColor(COL_GOLD, box_bg);
  canvas.setCursor(68, 122);
  canvas.printf("* MVP: %s *", easter_heroes[hero_idx]);

  // Confetti particles
  if (rand() % 2 == 0) {
    spawn_particle(rand() % 160 + 40, rand() % 30 + 100, (rand() % 20 - 10) * 0.1f, -1.0f, (rand() % 2 == 0) ? COL_GOLD : COL_WHITE, 20);
  }
}

void update_particles() {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (particles[i].life > 0) {
      particles[i].x += particles[i].vx;
      particles[i].y += particles[i].vy;
      particles[i].life--;
      canvas.drawPixel((int)particles[i].x, (int)particles[i].y, particles[i].color);
    }
  }
}

// ============================================================================
// Baseball Batting Gauge & Single-Button Hitting
// ============================================================================
void draw_batting_gauge() {
  // 1. Gauge Background & Outer Border (at x=208, y=155 to 205)
  canvas.fillRect(GAUGE_X - 1, GAUGE_Y - 1, GAUGE_W + 2, GAUGE_H + 2, COL_BLACK);
  canvas.drawRect(GAUGE_X - 1, GAUGE_Y - 1, GAUGE_W + 2, GAUGE_H + 2, COL_WHITE);

  // 2. Track inner fill
  canvas.fillRect(GAUGE_X, GAUGE_Y, GAUGE_W, GAUGE_H, 0x18C3); // Dark slate track

  // 3. Distinct SWEET SPOT Target Box in exact middle (middle 8 pixels: 176 to 184)
  canvas.fillRect(GAUGE_X, GAUGE_MID_Y - 4, GAUGE_W, 8, 0x07E0); // Bright emerald green
  canvas.drawRect(GAUGE_X, GAUGE_MID_Y - 4, GAUGE_W, 8, COL_GOLD); // Gold border
  canvas.drawFastHLine(GAUGE_X - 2, GAUGE_MID_Y, GAUGE_W + 4, COL_GOLD); // Center sweet-spot notch

  // 4. Track Tick Marks
  canvas.drawFastHLine(GAUGE_X + 1, GAUGE_Y + 12, GAUGE_W - 2, 0x528A);
  canvas.drawFastHLine(GAUGE_X + 1, GAUGE_Y + 38, GAUGE_W - 2, 0x528A);

  // 5. Dynamic Floater Cursor
  bool show_floater = false;
  if (bb_state == BB_PITCHING || (batter_swung && hit_feedback_timer > 0)) {
    show_floater = true;
  } else if (bb_state == BB_READY) {
    show_floater = true;
    baseball_floater_y = (float)GAUGE_Y + (millis() / 120) % 3;
  }

  if (show_floater) {
    int fy = (int)baseball_floater_y;
    if (fy < GAUGE_Y) fy = GAUGE_Y;
    if (fy > GAUGE_Y + GAUGE_H) fy = GAUGE_Y + GAUGE_H;
    // Distinct floater cursor bar with gold indicator notches
    canvas.fillRect(GAUGE_X - 2, fy - 1, GAUGE_W + 4, 3, COL_WHITE);
    canvas.drawFastHLine(GAUGE_X - 3, fy, 2, COL_GOLD);
    canvas.drawFastHLine(GAUGE_X + GAUGE_W + 1, fy, 2, COL_GOLD);
  }

  // 6. Sweet Spot / Hit Feedback Label
  canvas.setTextSize(1);
  if (hit_feedback_timer > 0) {
    hit_feedback_timer--;
    if (hit_feedback_type == 1) {
      canvas.setTextColor(COL_GOLD);
      canvas.setCursor(GAUGE_X - 52, GAUGE_MID_Y - 4);
      static const char* bb_heroes[] = { "AVAN HR!", "MERDAN HR!", "ROJDA HR!" };
      canvas.print(bb_heroes[(score_red + score_blu) % 3]);
    } else if (hit_feedback_type == 2) {
      canvas.setTextColor(0x07E0);
      canvas.setCursor(GAUGE_X - 30, GAUGE_MID_Y - 4);
      canvas.print("GOOD!");
    } else if (hit_feedback_type == 3) {
      canvas.setTextColor(COL_RED_TEAM);
      canvas.setCursor(GAUGE_X - 30, GAUGE_MID_Y - 4);
      canvas.print("MISS!");
    }
  } else {
    canvas.setTextColor(COL_GOLD);
    canvas.setCursor(GAUGE_X - 6, GAUGE_Y - 9);
    canvas.print("HIT");
  }
}

void swing_bat() {
  if (current_sport != SPORT_BASEBALL) return;
  if (batter_swung) return; // Prevent multiple swings during same pitch

  batter_swung = true;
  players[1].state = STATE_KICKING; // Triggers full horizontal swing follow-through!
  players[1].state_timer = 18;

  if (bb_state == BB_PITCHING) {
    float dist = fabsf(baseball_floater_y - (float)GAUGE_MID_Y);

    if (dist <= 4.0f) {
      // *** PERFECT SWEET SPOT HIT: TOWERING HOME RUN! (within ±4px) ***
      Serial.printf("Baseball: CRACK! PERFECT HIT (dist=%.1f) -> TOWERING HOME RUN!\n", dist);
      Serial.flush();
      bb_state = BB_HOMERUN;
      hit_feedback_type = 1; // PERFECT
      hit_feedback_timer = 35;
      screen_flash_timer = 3;

      ball.owner = -1;
      ball.x = 120.0f;
      ball.y = 192.0f;
      ball.vx = (rand() % 2 == 0 ? 1.2f : -1.2f);
      ball.vy = -7.2f; // Rockets UP into the outfield bleachers!
      ball.vz = 5.0f;
      ball.is_super = true;

      // Spawn gold spark explosion at home plate
      for (int k = 0; k < 25; k++) {
        float ang = (float)(rand() % 360) * (PI / 180.0f);
        float spd = (float)(rand() % 28 + 12) * 0.1f;
        spawn_particle(120.0f, 193.0f, cosf(ang) * spd, sinf(ang) * spd,
                       (k % 2 == 0) ? COL_GOLD : COL_WHITE, 25);
      }
      base_run_target = 1;

    } else if (dist <= 10.0f) {
      // *** GOOD HIT: Solid line drive / base hit into outfield (within ±10px) ***
      Serial.printf("Baseball: SOLID BASE HIT (dist=%.1f)!\n", dist);
      Serial.flush();
      bb_state = BB_HIT_IN_PLAY;
      hit_feedback_type = 2; // GOOD
      hit_feedback_timer = 25;

      ball.owner = -1;
      ball.x = 120.0f;
      ball.y = 192.0f;
      ball.vx = (baseball_floater_y < (float)GAUGE_MID_Y) ? -3.4f : 3.4f;
      ball.vy = -4.2f;
      ball.vz = 2.0f;
      ball.is_super = false;

      for (int k = 0; k < 12; k++) {
        spawn_particle(120.0f, 193.0f, (rand() % 20 - 10) * 0.15f, -1.5f, COL_STICK_WOOD, 16);
      }
      base_run_target = 1;

    } else {
      // *** MISS / FOUL (far from middle) ***
      Serial.printf("Baseball: MISS / FOUL (dist=%.1f)!\n", dist);
      Serial.flush();
      hit_feedback_type = 3; // MISS
      hit_feedback_timer = 20;
    }
  } else {
    // Swung with no pitch in air
    Serial.println("Baseball: Practice swing (no pitch in flight)!");
    Serial.flush();
    hit_feedback_type = 3;
    hit_feedback_timer = 15;
    spawn_particle(players[1].x + 8, players[1].y, 2.0f, 0, COL_WHITE, 6);
  }
}

// ============================================================================
// Interactive Shot Trigger (Mapped to BOOT Button & Serial)
// ============================================================================
void trigger_super_action() {
  if (current_sport == SPORT_BASEBALL) {
    swing_bat();
  } else if (current_sport == SPORT_BASKETBALL) {
    // High Flying 3-Pointer / Monster Dunk!
    if (ball.owner != -1) {
      int oid = ball.owner;
      ball.owner = -1;
      ball.is_super = true;
      players[oid].state = STATE_KICKING; // High-elevation slam dunk pose!
      players[oid].state_timer = 22;
      float hoop_x = (players[oid].team == 0) ? (PITCH_MAX_X - 16) : (PITCH_MIN_X + 16);
      float dx = hoop_x - players[oid].x;
      ball.vx = (dx > 0 ? 1.0f : -1.0f) * 6.5f;
      ball.vy = (129.0f - ball.y) * 0.08f;
      ball.vz = 4.2f; // High rainbow arc into basket!
      spawn_particle(ball.x, ball.y, 0, -2.0f, COL_GOLD, 25);
    } else {
      ball.is_super = true;
      ball.vz = 4.0f;
    }
  } else if (current_sport == SPORT_HOCKEY) {
    // 100 MPH Slap Shot!
    if (ball.owner != -1) {
      int oid = ball.owner;
      ball.owner = -1;
      ball.is_super = true;
      players[oid].state = STATE_KICKING; // Slap shot wind-up stance!
      players[oid].state_timer = 20;
      ball.vx = players[oid].facing * 9.2f;
      ball.vy = (rand() % 10 - 5) * 0.05f;
      spawn_particle(ball.x, ball.y, 0, -1.5f, COL_GOLD, 25);
    } else {
      ball.is_super = true;
      ball.vx = (ball.x < 120 ? 1.0f : -1.0f) * 8.5f;
    }
  } else {
    // Soccer Super Shot
    if (ball.owner != -1) {
      int oid = ball.owner;
      ball.owner = -1;
      ball.is_super = true;
      players[oid].state = STATE_KICKING; // Full leg extension follow-through!
      players[oid].state_timer = 20;
      ball.vx = players[oid].facing * 8.2f;
      ball.vy = (rand() % 10 - 5) * 0.1f;
      ball.vz = 2.5f;
      spawn_particle(ball.x, ball.y, 0, -2.0f, COL_GOLD, 25);
    } else {
      ball.is_super = true;
      ball.vx = (ball.x < 120 ? 1.0f : -1.0f) * 7.5f;
      ball.vz = 2.0f;
    }
  }
}

// ============================================================================
// Dedicated Baseball Game Mechanics & Autonomous AI
// ============================================================================
void update_baseball_ai() {
  // 1. Home Run Celebration / Banner Running
  if (goal_banner_timer > 0) {
    goal_banner_timer--;
    Player &batter = players[1];
    batter.state = STATE_RUNNING;
    float b_tx = batter.x, b_ty = batter.y;

    if (base_run_target == 1) {
      b_tx = 170.0f; b_ty = 143.0f;
      if (hypotf(batter.x - 170.0f, batter.y - 143.0f) < 4.0f) base_run_target = 2;
    } else if (base_run_target == 2) {
      b_tx = 118.0f; b_ty = 93.0f;
      if (hypotf(batter.x - 118.0f, batter.y - 93.0f) < 4.0f) base_run_target = 3;
    } else if (base_run_target == 3) {
      b_tx = 66.0f;  b_ty = 143.0f;
      if (hypotf(batter.x - 66.0f, batter.y - 143.0f) < 4.0f) base_run_target = 4;
    } else {
      b_tx = 110.0f; b_ty = 193.0f;
      if (hypotf(batter.x - 110.0f, batter.y - 193.0f) < 4.0f) {
        batter.state = STATE_CHEER;
        batter.anim_tick++;
      }
    }

    float b_dx = b_tx - batter.x;
    float b_dy = b_ty - batter.y;
    float b_dist = hypotf(b_dx, b_dy);
    if (b_dist > 1.5f) {
      batter.x += (b_dx / b_dist) * 2.2f;
      batter.y += (b_dy / b_dist) * 2.2f;
      batter.facing = (b_dx > 0) ? 1 : -1;
      batter.anim_tick++;
      if (batter.anim_tick >= 5) {
        batter.anim_tick = 0;
        batter.anim_frame = (batter.anim_frame + 1) % 4;
      }
    }

    // Blue fielders fall to knees in defeat
    for (int i = 0; i < NUM_PLAYERS; i++) {
      if (i != 1) players[i].state = STATE_DOWN;
    }

    if (goal_banner_timer == 0) {
      // 1 GOAL / HOME RUN SCORED -> SWITCH TO NEXT SPORT!
      switch_to_next_sport();
    }
    return;
  }

  // 2. Easter egg flash on time clock ("avan", "merdan", "rojda")
  if (flash_text_timer > 0) {
    flash_text_timer--;
  } else {
    next_flash_countdown--;
    if (next_flash_countdown <= 0) {
      active_flash_idx = rand() % 3;
      flash_text_timer = 32;
      next_flash_countdown = rand() % 140 + 90;
    }
  }

  // 3. Batter swing state timer
  if (players[1].state == STATE_KICKING) {
    players[1].state_timer--;
    if (players[1].state_timer <= 0) {
      players[1].state = STATE_IDLE;
    }
  }

  // 4. Ball Physics & Baseball Pitch/Hit State Machine
  switch (bb_state) {
    case BB_READY: {
      ball.owner = 3;
      ball.x = 120.0f;
      ball.y = 140.0f;
      ball.z = 0.0f;
      ball.vx = 0.0f;
      ball.vy = 0.0f;
      ball.vz = 0.0f;
      ball.is_super = false;
      batter_swung = false;

      // Pitcher windup motion on mound
      players[3].facing = 1;
      players[3].anim_tick++;
      if (players[3].anim_tick >= 8) {
        players[3].anim_tick = 0;
        players[3].anim_frame = (players[3].anim_frame + 1) % 4;
      }

      bb_timer--;
      if (bb_timer <= 0) {
        // PITCH THROWN DOWNWARD TOWARDS HOME PLATE!
        bb_state = BB_PITCHING;
        ball.owner = -1;
        ball.x = 120.0f;
        ball.y = 140.0f;
        ball.vx = (float)(rand() % 5 - 2) * 0.04f;
        ball.vy = 3.8f; // Strictly DOWN toward home plate y=195!
        ball.vz = 0.4f;
        ball.is_super = false;
        Serial.println("Baseball: Pitch thrown DOWNWARD toward home plate (vy=+3.8)!");
        Serial.flush();
      }
      break;
    }

    case BB_PITCHING: {
      ball.x += ball.vx;
      ball.y += ball.vy;

      // Track pitch progress with dynamic vertical floater cursor
      float progress = (ball.y - 140.0f) / 55.0f; // 0.0 at y=140, 1.0 at y=195
      if (progress <= 1.0f) {
        baseball_floater_y = (float)GAUGE_Y + progress * ((float)GAUGE_MID_Y - (float)GAUGE_Y);
      } else {
        baseball_floater_y = (float)GAUGE_MID_Y + ((ball.y - 195.0f) / 11.0f) * ((float)GAUGE_H / 2.0f);
      }

      // If ball crosses home plate without hit: catcher catches it!
      if (ball.y >= 195.0f) {
        bb_state = BB_CAUGHT;
        bb_timer = 22; // Pause in catcher mitt
        ball.owner = 0;
        ball.x = 120.0f;
        ball.y = 202.0f;
        ball.vx = 0; ball.vy = 0; ball.vz = 0;
        spawn_particle(120.0f, 202.0f, 0, -0.6f, COL_WHITE, 8); // Mitt catch puff
        Serial.println("Baseball: Ball in catcher's mitt at home plate!");
        Serial.flush();
      }
      break;
    }

    case BB_CAUGHT: {
      ball.owner = 0;
      ball.x = 120.0f;
      ball.y = 202.0f;
      bb_timer--;
      if (bb_timer <= 0) {
        // Catcher tosses ball back to pitcher at (120, 140)
        bb_state = BB_THROW_BACK;
        ball.owner = -1;
        ball.x = 120.0f;
        ball.y = 202.0f;
        ball.vx = 0.0f;
        ball.vy = -3.4f; // Toss UP toward pitcher
        ball.vz = 1.2f;
        Serial.println("Baseball: Catcher tosses ball back to pitcher!");
        Serial.flush();
      }
      break;
    }

    case BB_THROW_BACK: {
      ball.x += ball.vx;
      ball.y += ball.vy;
      if (ball.z > 0 || ball.vz != 0) {
        ball.z += ball.vz;
        ball.vz -= 0.12f;
        if (ball.z <= 0) ball.z = 0;
      }
      // Reached pitcher mound
      if (ball.y <= 140.0f && ball.vy < 0) {
        bb_state = BB_READY;
        bb_timer = 40; // Windup timer before next pitch
        ball.owner = 3;
        ball.x = 120.0f;
        ball.y = 140.0f;
        ball.vx = 0; ball.vy = 0; ball.vz = 0;
        spawn_particle(120.0f, 140.0f, 0, 0.5f, COL_WHITE, 6);
        Serial.println("Baseball: Pitcher caught ball, preparing next pitch!");
        Serial.flush();
      }
      break;
    }

    case BB_HIT_IN_PLAY: {
      ball.x += ball.vx;
      ball.y += ball.vy;
      ball.vx *= 0.95f;
      ball.vy *= 0.95f;
      if (ball.z > 0 || ball.vz != 0) {
        ball.z += ball.vz;
        ball.vz -= 0.35f;
        if (ball.z <= 0) {
          ball.z = 0;
          ball.vz = -ball.vz * 0.35f;
        }
      }

      // Batter runs to 1st base (170, 143)
      Player &batter = players[1];
      batter.state = STATE_RUNNING;
      float b_dx = 170.0f - batter.x;
      float b_dy = 143.0f - batter.y;
      float b_dist = hypotf(b_dx, b_dy);
      if (b_dist > 2.0f) {
        batter.x += (b_dx / b_dist) * 2.0f;
        batter.y += (b_dy / b_dist) * 2.0f;
        batter.facing = 1;
        batter.anim_tick++;
        if (batter.anim_tick >= 5) {
          batter.anim_tick = 0;
          batter.anim_frame = (batter.anim_frame + 1) % 4;
        }
      } else {
        batter.state = STATE_IDLE;
      }

      // Fielder (Player 4 at 2nd base) runs to field the ball
      Player &fielder = players[4];
      float f_dx = ball.x - fielder.x;
      float f_dy = ball.y - fielder.y;
      float f_dist = hypotf(f_dx, f_dy);
      if (f_dist > 6.0f) {
        fielder.state = STATE_RUNNING;
        fielder.x += (f_dx / f_dist) * 1.8f;
        fielder.y += (f_dy / f_dist) * 1.8f;
        fielder.facing = (f_dx > 0) ? 1 : -1;
      } else {
        // Fielder throws ball back to pitcher at (120, 140)
        bb_state = BB_THROW_BACK;
        ball.owner = -1;
        ball.vx = (120.0f - ball.x) * 0.08f;
        ball.vy = (140.0f - ball.y) * 0.08f;
        ball.vz = 1.4f;
        fielder.x = 118.0f; fielder.y = 93.0f; // Return to 2nd base
        fielder.state = STATE_IDLE;
        batter.x = 110.0f;  batter.y = 193.0f; // Return to batter box
        batter.state = STATE_IDLE;
      }
      break;
    }

    case BB_HOMERUN: {
      ball.x += ball.vx;
      ball.y += ball.vy;
      if (ball.z > 0 || ball.vz != 0) {
        ball.z += ball.vz;
        ball.vz -= 0.08f;
      }

      // Batter running the bases!
      Player &batter = players[1];
      batter.state = STATE_RUNNING;
      float b_tx = batter.x, b_ty = batter.y;
      if (base_run_target == 1) {
        b_tx = 170.0f; b_ty = 143.0f;
        if (hypotf(batter.x - 170.0f, batter.y - 143.0f) < 4.0f) base_run_target = 2;
      } else if (base_run_target == 2) {
        b_tx = 118.0f; b_ty = 93.0f;
        if (hypotf(batter.x - 118.0f, batter.y - 93.0f) < 4.0f) base_run_target = 3;
      } else if (base_run_target == 3) {
        b_tx = 66.0f;  b_ty = 143.0f;
        if (hypotf(batter.x - 66.0f, batter.y - 143.0f) < 4.0f) base_run_target = 4;
      } else {
        b_tx = 110.0f; b_ty = 193.0f;
        if (hypotf(batter.x - 110.0f, batter.y - 193.0f) < 4.0f) {
          batter.state = STATE_CHEER;
        }
      }
      float b_dx = b_tx - batter.x;
      float b_dy = b_ty - batter.y;
      float b_dist = hypotf(b_dx, b_dy);
      if (b_dist > 1.5f) {
        batter.x += (b_dx / b_dist) * 2.2f;
        batter.y += (b_dy / b_dist) * 2.2f;
        batter.facing = (b_dx > 0) ? 1 : -1;
        batter.anim_tick++;
        if (batter.anim_tick >= 5) {
          batter.anim_tick = 0;
          batter.anim_frame = (batter.anim_frame + 1) % 4;
        }
      }

      // CRITICAL: HOME RUN ONLY TRIGGERS ON FAIR BALL HIT UPWARD (vy < 0, y < 52)
      if (ball.y < 52.0f && ball.vy < 0.0f && goal_banner_timer == 0) {
        Serial.printf("HOME RUN OVER THE FENCE! y=%.1f vy=%.2f\n", ball.y, ball.vy);
        Serial.flush();
        score_red++;
        goal_banner_timer = 120;
        goal_scoring_team = 0; // Red batter scores!
        ball.vx = 0; ball.vy = 0;
        return;
      }
      break;
    }
  }

  // 5. Stationary Fielder & Pitcher Positioning Maintenance
  // Player 0: Catcher (120, 206)
  if (ball.owner != 0) {
    players[0].x = 120; players[0].y = 206; players[0].facing = 1; players[0].state = STATE_IDLE;
  }
  // Player 3: Pitcher (120, 140)
  if (bb_state != BB_READY) {
    players[3].x = 120; players[3].y = 140; players[3].facing = 1; players[3].state = STATE_IDLE;
  }
  // Player 2: 3rd Baseman (66, 143)
  players[2].x = 66; players[2].y = 143; players[2].facing = 1; players[2].state = STATE_IDLE;
  // Player 5: 1st Baseman (170, 143)
  players[5].x = 170; players[5].y = 143; players[5].facing = -1; players[5].state = STATE_IDLE;
  // Player 4: 2nd Baseman (118, 93) if not fielding
  if (bb_state != BB_HIT_IN_PLAY) {
    players[4].x = 118; players[4].y = 93; players[4].facing = 1; players[4].state = STATE_IDLE;
  }
  // Player 1: Batter at (110, 193) if not running bases
  if (bb_state != BB_HOMERUN && bb_state != BB_HIT_IN_PLAY && goal_banner_timer == 0) {
    players[1].x = 110; players[1].y = 193; players[1].facing = 1;
    if (players[1].state != STATE_KICKING) players[1].state = STATE_IDLE;
  }
}

// ============================================================================
// Kunio-kun Autonomous AI Loop
// ============================================================================
void update_ai() {
  if (current_sport == SPORT_BASEBALL) {
    update_baseball_ai();
    return;
  }
  if (goal_banner_timer > 0) {
    goal_banner_timer--;
    for (int i = 0; i < NUM_PLAYERS; i++) {
      if (players[i].team == goal_scoring_team) {
        players[i].state = STATE_CHEER;
        players[i].anim_tick++;
      } else {
        players[i].state = STATE_DOWN;
      }
    }
    if (goal_banner_timer == 0) {
      // 1 GOAL SCORED -> SWITCH TO NEXT SPORT!
      switch_to_next_sport();
    }
    return;
  }

  // Easter egg flash on time clock: randomly flash avan / merdan / rojda for ~1s
  if (flash_text_timer > 0) {
    flash_text_timer--;
  } else {
    next_flash_countdown--;
    if (next_flash_countdown <= 0) {
      active_flash_idx = rand() % 3;
      flash_text_timer = 32;
      next_flash_countdown = rand() % 140 + 90;
    }
  }

  // --- Ball / Puck Physics ---
  float friction = (current_sport == SPORT_HOCKEY) ? 0.985f : 0.94f;

  if (ball.owner != -1) {
    int pid = ball.owner;
    ball.x = players[pid].x + players[pid].facing * 6.0f;
    ball.y = players[pid].y + 1.0f;
    if (current_sport == SPORT_BASKETBALL) {
      // Dribbling bounce
      ball.z = fabsf(isin((uint8_t)(millis() / 4))) * 0.035f;
    } else {
      ball.z = 0;
    }
    ball.vx = 0;
    ball.vy = 0;
  } else {
    ball.x += ball.vx;
    ball.y += ball.vy;
    ball.vx *= friction;
    ball.vy *= friction;

    if (ball.z > 0 || ball.vz != 0) {
      ball.z += ball.vz;
      ball.vz -= 0.35f; // Gravity
      if (ball.z <= 0) {
        ball.z = 0;
        ball.vz = -ball.vz * 0.5f; // Rebound
      }
    }

    // --- Scoring Check ---
    if (current_sport == SPORT_BASKETBALL) {
      // Basket check (Left Hoop: x ~ 16, Right Hoop: x ~ 224, y ~ 129, ball coming down z < 4)
      if (ball.x <= (PITCH_MIN_X + 18) && fabsf(ball.y - 129.0f) < 8.0f && ball.z < 6.0f) {
        score_blu++;
        goal_banner_timer = 110;
        goal_scoring_team = 1;
        ball.vx = 0; ball.vy = 0;
        return;
      }
      if (ball.x >= (PITCH_MAX_X - 18) && fabsf(ball.y - 129.0f) < 8.0f && ball.z < 6.0f) {
        score_red++;
        goal_banner_timer = 110;
        goal_scoring_team = 0;
        ball.vx = 0; ball.vy = 0;
        return;
      }
    } else {
      // Goal Check: Left Net (Soccer & Hockey)
      if (ball.x <= GOAL_LEFT_X && ball.y >= GOAL_Y_TOP && ball.y <= GOAL_Y_BOT) {
        score_blu++;
        goal_banner_timer = 110;
        goal_scoring_team = 1;
        ball.vx = 0; ball.vy = 0;
        return;
      }
      // Goal Check: Right Net
      if (ball.x >= GOAL_RIGHT_X && ball.y >= GOAL_Y_TOP && ball.y <= GOAL_Y_BOT) {
        score_red++;
        goal_banner_timer = 110;
        goal_scoring_team = 0;
        ball.vx = 0; ball.vy = 0;
        return;
      }
    }

    // Boards & Boundaries Bounces
    if (ball.x < PITCH_MIN_X + 10) { ball.x = PITCH_MIN_X + 10; ball.vx = -ball.vx * 0.8f; }
    if (ball.x > PITCH_MAX_X - 10) { ball.x = PITCH_MAX_X - 10; ball.vx = -ball.vx * 0.8f; }
    if (ball.y < PITCH_MIN_Y + 6)  { ball.y = PITCH_MIN_Y + 6;  ball.vy = -ball.vy * 0.8f; }
    if (ball.y > PITCH_MAX_Y - 8)  { ball.y = PITCH_MAX_Y - 8;  ball.vy = -ball.vy * 0.8f; }

    // Goalkeeper diving reaction on incoming soccer shots
    if (current_sport == SPORT_SOCCER && ball.owner == -1 && fabsf(ball.vx) > 3.0f) {
      int gk_idx = (ball.vx < 0) ? 0 : 3;
      if (players[gk_idx].state != STATE_DIVING && players[gk_idx].state != STATE_DOWN) {
        float dist_x = fabsf(ball.x - players[gk_idx].x);
        if (dist_x < 70.0f && dist_x > 6.0f) {
          players[gk_idx].state = STATE_DIVING;
          players[gk_idx].state_timer = 20;
          players[gk_idx].facing = (ball.vx < 0) ? 1 : -1;
        }
      }
    }
  }

  // --- Players AI ---
  for (int i = 0; i < NUM_PLAYERS; i++) {
    Player &p = players[i];

    if (p.state == STATE_DOWN) {
      p.state_timer--;
      if (p.state_timer <= 0) p.state = STATE_IDLE;
      continue;
    }
    if (p.state == STATE_KICKING) {
      p.state_timer--;
      if (p.state_timer <= 0) p.state = STATE_IDLE;
      continue;
    }
    if (p.state == STATE_DIVING) {
      p.state_timer--;
      // Goalkeeper dives along Y towards ball
      p.y += (ball.y > p.y ? 1.6f : -1.6f);
      if (p.y < GOAL_Y_TOP + 4) p.y = GOAL_Y_TOP + 4;
      if (p.y > GOAL_Y_BOT - 4) p.y = GOAL_Y_BOT - 4;
      if (p.state_timer <= 0) p.state = STATE_IDLE;
      continue;
    }
    if (p.state == STATE_SLIDE) {
      if (current_sport == SPORT_BASKETBALL) {
        // Basketball Jump Block: defender leaps vertically to swat the shot
        p.x += p.facing * 0.8f;
      } else if (current_sport == SPORT_HOCKEY) {
        // Hockey Stick Slash: skate glide chop forward
        p.x += p.facing * 3.2f;
      } else {
        // Soccer Slide Tackle: turf slide
        p.x += p.facing * 2.8f;
      }
      p.state_timer--;

      if (ball.owner != -1 && players[ball.owner].team != p.team) {
        float dx = p.x - players[ball.owner].x;
        float dy = p.y - players[ball.owner].y;
        float reach_x = (current_sport == SPORT_HOCKEY) ? 16.0f : 12.0f;
        float reach_y = (current_sport == SPORT_BASKETBALL) ? 12.0f : 8.0f;

        if (fabsf(dx) < reach_x && fabsf(dy) < reach_y) {
          if (long_press_active) {
            // Long-press evasion active: JUKED! Defensive action whiffs cleanly!
            spawn_particle(p.x, p.y, 0, -2.0f, COL_WHITE, 12);
          } else {
            int tackled_id = ball.owner;
            ball.owner = -1;

            if (current_sport == SPORT_BASKETBALL) {
              // BLOCKED / SWATTED! High rejection arc
              players[tackled_id].state = STATE_DOWN;
              players[tackled_id].state_timer = 16;
              ball.vx = -p.facing * 4.6f;
              ball.vy = (rand() % 20 - 10) * 0.15f;
              ball.vz = 3.8f; // Swatted high into the air!
              spawn_particle(ball.x, ball.y, 0, -2.0f, COL_GOLD, 16);
              Serial.println("Basketball: BLOCKED / SWATTED!");
            } else if (current_sport == SPORT_HOCKEY) {
              // STICK SLASH / CROSS-CHECK: Puck stripped on ice!
              players[tackled_id].state = STATE_DOWN;
              players[tackled_id].state_timer = 22;
              ball.vx = p.facing * 5.5f;
              ball.vy = (rand() % 20 - 10) * 0.1f;
              spawn_particle(p.x + p.facing * 8, p.y, p.facing * 2.0f, -1.0f, COL_STICK_WOOD, 14);
              spawn_particle(p.x, p.y, 0, -1.0f, COL_WHITE, 16); // Ice shavings
              Serial.println("Hockey: STICK SLASH / CHECK!");
            } else {
              // Soccer Slide Tackle
              players[tackled_id].state = STATE_DOWN;
              players[tackled_id].state_timer = 20;
              ball.vx = p.facing * 4.0f;
              ball.vy = (rand() % 20 - 10) * 0.1f;
              spawn_particle(p.x, p.y, 0, -1.0f, COL_GOLD, 15);
              Serial.println("Soccer: SLIDE TACKLE!");
            }
          }
        }
      }
      if (p.state_timer <= 0) p.state = STATE_IDLE;
      continue;
    }

    float target_x = p.x;
    float target_y = p.y;
    float speed = (ball.owner == i) ? 1.80f : 1.45f;
    if (current_sport == SPORT_HOCKEY) speed += 0.25f; // Ice skates glide faster!

    if (p.role == ROLE_GK) {
      // Goalkeepers step out diagonally and cover the full goal mouth
      float base_x = (p.team == 0) ? 32.0f : 208.0f;
      float step_out = (p.team == 0) ? fabsf(ball.x - 32.0f) * 0.12f : -fabsf(ball.x - 208.0f) * 0.12f;
      if (fabsf(step_out) > 14.0f) step_out = (p.team == 0 ? 14.0f : -14.0f);
      target_x = base_x + step_out;
      target_y = ball.y;
      if (target_y < GOAL_Y_TOP + 4) target_y = GOAL_Y_TOP + 4;
      if (target_y > GOAL_Y_BOT - 4) target_y = GOAL_Y_BOT - 4;
      p.facing = (p.team == 0) ? 1 : -1;
    } else {
      if (ball.owner == i) {
        if (long_press_active) {
          // --- LONG PRESS EVASION ---
          // Move up or down, depending which wall is further away!
          float dist_top = p.y - (float)PITCH_MIN_Y;
          float dist_bot = (float)PITCH_MAX_Y - p.y;
          if (evade_direction == 0) {
            evade_direction = (dist_top > dist_bot) ? -1 : 1;
          }

          if (evade_direction < 0) {
            target_y = PITCH_MIN_Y + 16.0f; // Move towards top wall (further away)
          } else {
            target_y = PITCH_MAX_Y - 18.0f; // Move towards bottom wall (further away)
          }

          // Advance towards opponent goal while evading
          target_x = (p.team == 0) ? (p.x + 32.0f) : (p.x - 32.0f);
          if (target_x > GOAL_RIGHT_X - 10) target_x = GOAL_RIGHT_X - 10;
          if (target_x < GOAL_LEFT_X + 10) target_x = GOAL_LEFT_X + 10;

          speed = 2.45f;
          p.lane_y = p.y; // Keep new position when evasion ends
          p.facing = (p.team == 0) ? 1 : -1;

          // Golden dash particles trailing behind
          if (rand() % 2 == 0) {
            spawn_particle(p.x - p.facing * 3.0f, p.y + 3.0f, -p.facing * 1.5f, (evade_direction < 0 ? -0.8f : 0.8f), COL_GOLD, 8);
          }
        } else {
          // Normal ball carrier: diagonal zig-zag runs across full pitch height
          target_x = (p.team == 0) ? GOAL_RIGHT_X - 10 : GOAL_LEFT_X + 10;
          float weave = (float)(isin((uint8_t)(p.weave_phase + (int)(p.x * 2.2f)))) * 0.20f;
          target_y = p.lane_y + weave;
          if (target_y < PITCH_MIN_Y + 14) target_y = PITCH_MIN_Y + 14;
          if (target_y > PITCH_MAX_Y - 18) target_y = PITCH_MAX_Y - 18;
          p.facing = (p.team == 0) ? 1 : -1;

          // Diagonal Passing across the field to teammate on opposite wing!
          int teammate_idx = (i == 1) ? 2 : (i == 2) ? 1 : (i == 4) ? 5 : 4;
          if (rand() % 35 == 0) {
            float t_dx = players[teammate_idx].x - p.x;
            float t_dy = players[teammate_idx].y - p.y;
            float t_dist = hypotf(t_dx, t_dy);
            bool pass_forward = (p.team == 0) ? (t_dx > 10.0f) : (t_dx < -10.0f);
            if (t_dist > 35.0f && pass_forward) {
              ball.owner = -1;
              ball.vx = (t_dx / t_dist) * 5.2f;
              ball.vy = (t_dy / t_dist) * 5.2f;
              ball.vz = (current_sport == SPORT_BASKETBALL) ? 2.5f : 0.8f;
              players[i].state = STATE_KICKING;
              players[i].state_timer = 12;
            }
          }

          // Shoot once crossing into attacking half!
          bool in_shooting_range = (p.team == 0) ? (p.x > 125) : (p.x < 115);
          if (in_shooting_range && (rand() % 8 == 0)) {
            trigger_super_action();
          }
        }
      } else {
        // Defensive moves: Slide Tackle (Soccer), Stick Slash (Hockey), Jump Block (Basketball)
        if (current_sport != SPORT_BASEBALL) {
          static int tackle_cooldown = 0;
          if (tackle_cooldown > 0) tackle_cooldown--;

          if (ball.owner != -1 && players[ball.owner].team != p.team && tackle_cooldown == 0) {
            float dist_carrier = hypotf(p.x - players[ball.owner].x, p.y - players[ball.owner].y);
            int trigger_dist = (current_sport == SPORT_HOCKEY) ? 26 : (current_sport == SPORT_BASKETBALL ? 22 : 24);
            int trigger_chance = (current_sport == SPORT_BASKETBALL) ? 38 : 45;
            if (dist_carrier < (float)trigger_dist && rand() % trigger_chance == 0) {
              p.state = STATE_SLIDE;
              p.state_timer = (current_sport == SPORT_BASKETBALL) ? 14 : 16;
              p.facing = (players[ball.owner].x > p.x) ? 1 : -1;
              tackle_cooldown = 75;
              continue;
            }
          }
        }

        if (p.role == ROLE_FWD) {
          if (ball.owner != -1 && players[ball.owner].team == p.team) {
            // Overlapping diagonal run on opposite flank!
            target_x = ball.x + (p.team == 0 ? 32.0f : -32.0f);
            target_y = (ball.y < 129.0f) ? 175.0f : 80.0f;
          } else {
            target_x = ball.x + ((p.team == 0) ? -8 : 8);
            target_y = ball.y + (float)(isin((uint8_t)(p.weave_phase + millis() / 16))) * 0.08f;
          }
        } else if (p.role == ROLE_DEF) {
          // Defenders patrol distinct upper/lower zones and cut off passing angles
          float anchor_x = (p.team == 0) ? 65.0f : 175.0f;
          target_x = ball.x * 0.35f + anchor_x * 0.65f;
          target_y = p.lane_y + (ball.y - 129.0f) * 0.55f;
        }

        if (target_y < PITCH_MIN_Y + 14) target_y = PITCH_MIN_Y + 14;
        if (target_y > PITCH_MAX_Y - 18) target_y = PITCH_MAX_Y - 18;
      }
    }

    float dx = target_x - p.x;
    float dy = target_y - p.y;
    float dist = hypotf(dx, dy);

    if (dist > 2.0f) {
      p.state = STATE_RUNNING;
      p.vx = (dx / dist) * speed;
      p.vy = (dy / dist) * speed;
      p.x += p.vx;
      p.y += p.vy;
      if (fabsf(p.vx) > 0.3f) p.facing = (p.vx > 0) ? 1 : -1;

      p.anim_tick++;
      if (p.anim_tick >= 4) {
        p.anim_tick = 0;
        p.anim_frame = (p.anim_frame + 1) % 6;
      }
    } else {
      p.state = STATE_IDLE;
    }

    if (ball.owner == -1 && p.state != STATE_DOWN && p.state != STATE_SLIDE && p.state != STATE_DIVING && p.state != STATE_KICKING) {
      float dist_ball = hypotf(p.x - ball.x, p.y - ball.y);
      if (dist_ball < 8.0f && ball.z < 8.0f) {
        ball.owner = i;
        ball.is_super = false;
      }
    }
  }
}

// ============================================================================
// Setup & Main Loop
// ============================================================================
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("Kunio-kun Multi-Sport Championship initializing...");

  // Load last game played and advance to next game on RST!
  prefs.begin("retro_game", false);
  int last_sport = prefs.getInt("last_sport", -1);
  int next_sport;
  if (last_sport < 0 || last_sport > 3) {
    next_sport = 0; // First boot: Soccer
  } else {
    // If board was reset, automatically cycle to next game!
    next_sport = (last_sport + 1) % 4;
  }
  prefs.putInt("last_sport", next_sport);
  current_sport = (SportType)next_sport;

  pinMode(BOOT_BTN_PIN, INPUT_PULLUP);

  init_sine_table();
  reset_kickoff(0);

  lcd.init();
  lcd.setBrightness(255);

  canvas.setColorDepth(16);
  if (!canvas.createSprite(240, 240)) {
    Serial.println("Warning: Falling back to 8-bit sprite");
    canvas.setColorDepth(8);
    canvas.createSprite(240, 240);
  }
  last_button_activity_ms = millis();
  Serial.println("Game started! Tap BOOT to Shoot/Dunk, Hold BOOT to Evade up/down!");
}

void loop() {
  // Check physical BOOT button (GPIO 9) and Serial input
  bool btn_raw = (digitalRead(BOOT_BTN_PIN) == LOW);

  // Inactivity tracking: reset timer on button down
  if (btn_raw) {
    last_button_activity_ms = millis();
  }

  // Serial commands: 's'/space = swing/super action; '0'..'3'/'b' = switch sport; 'l'/'e' = evade
  if (Serial.available()) {
    last_button_activity_ms = millis();
    char c = Serial.read();
    if (c == 's' || c == ' ') {
      Serial.println("Serial: Trigger Super Action / Batter Swing!");
      trigger_super_action();
    } else if (c == '3' || c == 'b') {
      current_sport = SPORT_BASEBALL;
      prefs.putInt("last_sport", 3);
      reset_kickoff(0);
      Serial.println("Serial: Switched directly to BASEBALL!");
    } else if (c == '0') {
      current_sport = SPORT_SOCCER;
      prefs.putInt("last_sport", 0);
      reset_kickoff(0);
      Serial.println("Serial: Switched to SOCCER!");
    } else if (c == '1') {
      current_sport = SPORT_HOCKEY;
      prefs.putInt("last_sport", 1);
      reset_kickoff(0);
      Serial.println("Serial: Switched to HOCKEY!");
    } else if (c == '2') {
      current_sport = SPORT_BASKETBALL;
      prefs.putInt("last_sport", 2);
      reset_kickoff(0);
      Serial.println("Serial: Switched to BASKETBALL!");
    } else if (c == 'l' || c == 'e') {
      long_press_active = !long_press_active;
      if (long_press_active) {
        if (ball.owner != -1) {
          float dist_top = players[ball.owner].y - (float)PITCH_MIN_Y;
          float dist_bot = (float)PITCH_MAX_Y - players[ball.owner].y;
          evade_direction = (dist_top > dist_bot) ? -1 : 1;
        } else {
          evade_direction = -1;
        }
      } else {
        evade_direction = 0;
      }
      Serial.printf("Serial: Long Press Evade %s\n", long_press_active ? "ACTIVE" : "OFF");
    }
  }

  // Physical Button Gesture State Machine
  if (btn_raw) {
    if (!btn_was_down) {
      // Button just pressed down
      btn_was_down = true;
      btn_down_start = millis();
      long_press_fired = false;
      long_press_active = false;
      evade_direction = 0;

      // Zero-latency instant swing on button press down for Baseball!
      if (current_sport == SPORT_BASEBALL) {
        swing_bat();
      }
    } else {
      // Button being held down
      uint32_t hold_time = millis() - btn_down_start;
      if (hold_time >= 300) { // 300ms threshold for long press
        if (!long_press_fired && current_sport != SPORT_BASEBALL) {
          long_press_fired = true;
          long_press_active = true;
          if (ball.owner != -1) {
            float dist_top = players[ball.owner].y - (float)PITCH_MIN_Y;
            float dist_bot = (float)PITCH_MAX_Y - players[ball.owner].y;
            evade_direction = (dist_top > dist_bot) ? -1 : 1;
            Serial.printf("BOOT Long Press ACTIVE! Carrier %d evading %s (dist_top=%.1f, dist_bot=%.1f)\n",
                          ball.owner, evade_direction < 0 ? "UP" : "DOWN", dist_top, dist_bot);
          } else {
            evade_direction = (players[1].y > 129.0f) ? -1 : 1;
            Serial.println("BOOT Long Press ACTIVE! (loose ball)");
          }
        }
      }
    }
  } else {
    if (btn_was_down) {
      uint32_t press_duration = millis() - btn_down_start;
      if (!long_press_fired && press_duration >= 20 && press_duration < 300) {
        if (current_sport != SPORT_BASEBALL) {
          // Quick short click: Super action for Soccer / Hockey / Basketball!
          Serial.printf("BOOT Short Click (%d ms) -> Super Action!\n", press_duration);
          trigger_super_action();
        }
      } else if (long_press_fired) {
        Serial.printf("BOOT Long Press Released (held %d ms)\n", press_duration);
      }
      btn_was_down = false;
      long_press_active = false;
      long_press_fired = false;
      evade_direction = 0;
    }
  }

  // Update Game AI
  update_ai();

  // 1. Draw Field / Rink / Court
  draw_stadium_floor();

  // 2. Draw Players sorted by Y coordinate for 2.5D depth
  int sorted_indices[NUM_PLAYERS];
  for (int i = 0; i < NUM_PLAYERS; i++) sorted_indices[i] = i;
  for (int i = 0; i < NUM_PLAYERS - 1; i++) {
    for (int j = i + 1; j < NUM_PLAYERS; j++) {
      if (players[sorted_indices[i]].y > players[sorted_indices[j]].y) {
        int temp = sorted_indices[i];
        sorted_indices[i] = sorted_indices[j];
        sorted_indices[j] = temp;
      }
    }
  }

  bool ball_drawn = false;
  for (int i = 0; i < NUM_PLAYERS; i++) {
    int pid = sorted_indices[i];
    if (!ball_drawn && ball.y <= players[pid].y) {
      draw_ball();
      ball_drawn = true;
    }
    draw_player_16bit(canvas, players[pid]);
  }
  if (!ball_drawn) draw_ball();

  // 3. Particles
  update_particles();

  // 3b. Baseball Batting Gauge with sweet-spot target & floater
  if (current_sport == SPORT_BASEBALL) {
    draw_batting_gauge();
  }

  // 4. Goal / Score Banner
  draw_goal_banner();

  // 5. Scoreboard HUD
  draw_hud();

  // Screen Flash effect on towering home run
  if (screen_flash_timer > 0) {
    screen_flash_timer--;
    canvas.fillScreen(COL_WHITE);
  }

  // Inactivity Shutdown Check (2 minutes of no button presses)
  if (millis() - last_button_activity_ms >= INACTIVITY_TIMEOUT_MS) {
    enter_device_sleep();
  }

  // 6. Push to round display in one DMA burst
  canvas.pushSprite(0, 0);

  delay(28); // ~33 FPS
}
