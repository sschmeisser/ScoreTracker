# CYD Score Tracker — Frozen Architecture & Interface Spec

Conductor-owned spec. Subagents implement against these signatures **verbatim**. Do not rename
symbols, change signatures, or introduce cross-file assumptions not listed here. If something is
missing, leave a `// TODO(interface):` comment and report it — do not improvise an interface.

## Target hardware (verified)
- Board: **ESP32-2432S028R "Cheap Yellow Display"**
- SoC: ESP32-D0WD-V3, dual-core 240 MHz, 4MB flash (verified via `esptool flash_id`)
- Display: **ILI9341** 2.8" 240x320, SPI on **HSPI**
  - SCLK=14, MOSI/SDI=13, MISO/SDO=12, DC/RS=2, CS=15, RST=-1, BL=21 (PWM)
  - Panel: `panel_width=320`, `panel_height=240`, `memory_width=320`, `memory_height=240`,
    `invert=false`, `rgb_order=true`, `readable=true`, `bus_shared=true`, `dummy_read_pixel=8`,
    `dummy_read_bits=1`
  - Bus: `freq_write=55000000`, `freq_read=16000000`, `use_lock=true`, `dma_channel=SPI_DMA_CH_AUTO`
- Touch: **XPT2046** resistive, SPI on **VSPI** (separate bus)
  - SCLK=25, MOSI=32, MISO=39, CS=33, INT/IRQ=-1 (polling)
  - Calibration defaults: `XPT_XMIN=200`, `XPT_XMAX=3700`, `XPT_YMIN=240`, `XPT_YMAX=3800`,
    `offset_rotation=0`. Must be runtime-tunable (see `TOUCHCAL`).
- BOOT button: GPIO 0 (active LOW, has internal pullup) → page advance.
- Serial/upload port: `/dev/ttyUSB0` (CH340) @ 115200. User is in group `uucp`.

## Screen / UX
- Landscape **320x240**, rendered with a **two-band RGB565 framebuffer** (`LGFX_Sprite canvas`).
- **Why bands (verified on hardware):** the largest contiguous heap block on this ESP32-D0WD-V3 is
  only **110,580 bytes** (`heap_caps_get_largest_free_block`, total free ~284 KB but split across
  non-contiguous regions). A full 320x240 16bpp frame is `153600` bytes, which is **impossible** in
  one allocation and also fails LovyanGFX's `createSprite` (it uses `AllocationSource::Dma`).
  Therefore the canvas is **320x120** (`= 76800` bytes, fits) and the scene is rendered **twice per
  frame** with a `camera_y` of `0` and `120`, then each band is pushed to its screen half. Render
  functions take `camera_y` and draw at `absolute_y - camera_y`; the sprite clips the rest.
- The caller allocates the band buffer with `heap_caps_malloc(320*120*2, MALLOC_CAP_8BIT)` and hands
  it to `canvas.setBuffer(buf, 320, 120, 16)` (preallocated buffers are not freed by the sprite).
- Aesthetic: same "calm zen" palette as the round tracker:
  - BG deep forest moss `0x1103`, warm sand text `0xDEB8`, sage records `0x84B0`,
    live/amber accent `0x39E4`, winner gold `0xFE60`, card bg `0x18E3` (slightly lifted).
- Layout: header bar (league/title/page+paused), **3 game cards** per page (2 team rows each),
  footer pager dots + touch hints.
- Touch zones: bottom-left third = PREV, bottom-right third = NEXT, bottom-center = PAUSE/RESUME,
  tap on a card = open detail overlay (quarter line scores, leader, stadium); tap anywhere in
  detail = return.

## File ownership (non-overlapping)
| File | Owner |
|---|---|
| `cheap-yellow-display/platformio.ini` | hardware agent |
| `cheap-yellow-display/src/config.h` | hardware agent |
| `cheap-yellow-display/src/LGFX_config.h` | hardware agent |
| `cheap-yellow-display/README.md` | hardware agent |
| `cheap-yellow-display/src/game_data.h` | data agent |
| `cheap-yellow-display/src/live_data.h` | data agent |
| `cheap-yellow-display/src/nfl_logos.h` | data agent |
| `cheap-yellow-display/src/cyd_ui.h` | ui agent |
| `cheap-yellow-display/src/main.cpp` | main agent |
| `cheap-yellow-display/tools/*` | tools agent |
| `cheap-yellow-display/ARCHITECTURE.md` | Conductor only |

## Frozen interfaces

### `src/game_data.h` (data agent)
```cpp
#pragma once
#include <Arduino.h>
#include <LovyanGFX.hpp>

struct SportGame {
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

#define CYD_DEFAULT_GAMES_COUNT 6
static const SportGame CYD_DEFAULT_GAMES[CYD_DEFAULT_GAMES_COUNT] = { /* board-agnostic fallback */ };
```

### `src/nfl_logos.h` (data agent)
Shim only — reuses the round tracker's baked logo bitmaps (do NOT copy 223KB):
```cpp
#pragma once
#include "../../src/nfl_logos.h"   // NFL_LOGO_WIDTH/HEIGHT, NFL_LOGO_CHROMA_KEY,
                                   // get_nfl_team_logo_bitmap(const char*), NFL_SHIELD_DATA, ...
```

### `src/live_data.h` (data agent)
Board-agnostic data/storage/transport layer adapted from `../../src/live_nfl.h`.
Header-only, all functions `inline`. Exposes exactly:
```cpp
void               init_live_data();
void               poll_live_data();               // WiFi reconnect + periodic ESPN refresh
bool               fetch_live_scores();            // synchronous ESPN fetch (also used by REFRESH cmd)
const SportGame*   get_active_games();             // live games if ready, else CYD_DEFAULT_GAMES
int                get_active_games_count();       // live count, else CYD_DEFAULT_GAMES_COUNT
void               set_wifi_enabled(bool enabled);
bool               is_wifi_enabled();
void               clear_live_games();
void               finalize_live_games(int count);
bool               set_live_game(int idx,
                                 const char* league,
                                 const char* day, const char* date, const char* status,
                                 const char* away_code, const char* away_name, const char* away_rec,
                                 uint16_t away_score, const uint8_t* away_q, uint8_t away_rank,
                                 const char* home_code, const char* home_name, const char* home_rec,
                                 uint16_t home_score, const uint8_t* home_q, uint8_t home_rank,
                                 const char* leader, const char* stadium);
bool               parse_and_store_game_json(int idx, const char* json_str);
int                parse_and_store_games_json_array(const char* json_str);
```
Depends on: `config.h` (WIFI_SSID/PASS/ENABLE_WIFI), `game_data.h`, ArduinoJson, WiFi.
No drawing code in this file.

### `src/cyd_ui.h` (ui agent)
```cpp
#pragma once
#include <Arduino.h>
#include <LovyanGFX.hpp>
#include "game_data.h"
#include "nfl_logos.h"

#define CYD_W 320
#define CYD_H 240
#define CYD_BAND_H 120          // two-band framebuffer; 2 * 120 = 240

enum CydTouchAction { CYD_TOUCH_NONE, CYD_TOUCH_PREV, CYD_TOUCH_NEXT, CYD_TOUCH_TOGGLE, CYD_TOUCH_CARD, CYD_TOUCH_DETAIL_BACK };

void cyd_draw_score_tracker(LGFX_Sprite& canvas, int camera_y,
                            int current_page, int offset_y, int next_page, bool transitioning,
                            const SportGame* games, int total, bool paused, bool wifi_on);

void cyd_draw_sync_screen(LGFX_Sprite& canvas, int camera_y, int loaded, int total);
void cyd_draw_detail_screen(LGFX_Sprite& canvas, int camera_y, const SportGame& g);

CydTouchAction cyd_touch_hit(int x, int y, int* card_index);   // card_index set for CYD_TOUCH_CARD
```
`camera_y` is the vertical origin of the band in the 320x240 scene: every draw uses screen-y minus
`camera_y` (so `camera_y=0` fills the top band, `camera_y=120` the bottom band). It is orthogonal to
`offset_y`: `offset_y` in `[0, 240]` is the transition progress; when `transitioning`, the current
page is drawn at `y = -offset_y - camera_y` and the next page at `y = CYD_H - offset_y - camera_y`.

### `src/main.cpp` (main agent)
- Instantiates `static LGFX lcd;` and `static LGFX_Sprite canvas(&lcd);`. The sprite is **320x120**
  (16-bit) backed by a caller-owned `heap_caps_malloc(320*120*2, MALLOC_CAP_8BIT)` buffer via
  `canvas.setBuffer(...)` — `createSprite(320,240)` is forbidden (see Screen/UX). Keep the buffer at
  file scope so it outlives the sprite.
- `setup()`: `Serial.begin(115200)`, `pinMode(0, INPUT_PULLUP)`, `lcd.init()`, `lcd.setRotation(0)`,
  `lcd.setBrightness(...)`, allocate + `setBuffer`, `init_live_data()`, log
  `ESP.getFreeHeap()` / largest DMA / largest free block. Push an initial frame.
- Rendering: for each frame, loop `for (int band = 0; band < 2; ++band)` with
  `camera_y = band * CYD_BAND_H`, draw the scene, and `canvas.pushSprite(0, band * CYD_BAND_H)`.
- Serial commands (verbatim, same protocol as the round tracker, so existing tools work):
  `NEXT`, `PREV`, `SET <n>`, `PAUSE`, `RESUME`, `SCREENSHOT`, `SYNC_START`,
  `SCORES_START <n>` + per-line JSON + `SCORES_END`/`ABORT`, `SCORES_JSON <array>`,
  `WIFI OFF`, `WIFI ON`, `STATUS`, `REFRESH`/`SYNC`, plus new `TOUCHCAL` (stream raw touch
  coordinates for calibration) and `TOUCHDEBUG`.
- `SCREENSHOT` streams the full `CYD_W*CYD_H*2` (153600) raw RGB565 bytes of the assembled frame
  between `SCREENSHOT_START` / `SCREENSHOT_END`: render band 0 (`camera_y=0`) and stream its 76800
  bytes, then band 1 (`camera_y=120`) and stream its 76800 bytes. The capture tool is unchanged
  (still expects a 320x240 image).
- Touch via `lcd.getTouch(&x, &y)` → `cyd_touch_hit()` → act; debounce with a ~250ms cooldown and
  require release between taps.
- Auto-advance: `HOLD_DURATION_MS = 6000`, `TRANSITION_DURATION_MS = 700`, Hermite `ease` slide
  (mirror `../../src/main.cpp` logic). Detail overlay suspends auto-advance until dismissed.
- `loop()` ends with `delay(16)`.

### `platformio.ini` (hardware agent)
`[env:cyd]`, `platform=espressif32@^6.9.0`, `board=esp32dev`, `framework=arduino`,
`board_build.partitions=huge_app.csv`, `monitor_speed=115200`, `upload_speed=460800`,
`upload_port=/dev/ttyUSB0`, `monitor_port=/dev/ttyUSB0`, libs
`lovyan03/LovyanGFX@^1.1.16` and `bblanchon/ArduinoJson@^7.2.1`.

## Build & verify (Conductor runs)
```bash
cd /home/stefan/Projects/ScoreTracker/cheap-yellow-display
uvx platformio run
```
Flash/verify commands are Conductor-owned (hardware in the loop).
