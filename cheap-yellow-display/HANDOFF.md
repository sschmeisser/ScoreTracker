# Handoff: CYD Score Tracker (ESP32-2432S028R "Cheap Yellow Display")

## Hardware
- **Board:** ESP32-2432S028R "Cheap Yellow Display" (CYD).
- **Chip:** ESP32-D0WD-V3, dual-core Xtensa @ 240 MHz, 4MB flash (verified via `esptool flash_id`).
- **MAC:** `7c:d5:44:3c:48:ec`
- **Display:** 2.8" **ILI9341** 240x320 LCD, configured as native landscape **320x240**.
  - Driven over **HSPI**: SCLK=14, MOSI/SDI=13, MISO/SDO=12, DC/RS=2, CS=15, RST=-1 (tied to reset), BL=21 (PWM).
  - Panel: `panel_width=320`, `panel_height=240`, `memory_width=320`, `memory_height=240`, `invert=false`, `rgb_order=true`, `readable=true`, `bus_shared=true`, `dummy_read_pixel=8`, `dummy_read_bits=1`.
  - Bus: `freq_write=55 MHz`, `freq_read=16 MHz`, `use_lock=true`, `dma_channel=SPI_DMA_CH_AUTO`.
- **Touch:** **XPT2046** resistive controller on a **separate VSPI bus** (polling, no IRQ).
  - SCLK=25, MOSI=32, MISO=39, CS=33, INT/IRQ=-1.
  - Compile-time defaults: `XPT_XMIN=200`, `XPT_XMAX=3700`, `XPT_YMIN=240`, `XPT_YMAX=3800`, `offset_rotation=0`.
- **Button:** BOOT on GPIO 0 (active LOW, internal pullup) → next page.
- **Interface:** **CH340** USB-serial bridge on **`/dev/ttyUSB0`** @ 115200 (contrast: `/dev/ttyACM0` native USB on the round board). User is in group `uucp`.

## Firmware Architecture (file-by-file)
All under [`src/`](./src/) plus [`platformio.ini`](./platformio.ini):

- [`platformio.ini`](./platformio.ini) — `[env:cyd]`, `espressif32@^6.9.0`, `board=esp32dev`, `framework=arduino`, `huge_app.csv`, LovyanGFX + ArduinoJson.
- [`src/config.h`](./src/config.h) — `WIFI_SSID`/`WIFI_PASS`/`ENABLE_WIFI` and shared tuning constants.
- [`src/LGFX_config.h`](./src/LGFX_config.h) — LovyanGFX `LGFX` class: ILI9341/HSPI + XPT2046/VSPI panel config and touch calibration bounds.
- [`src/game_data.h`](./src/game_data.h) — `SportGame` struct and the `CYD_DEFAULT_GAMES` board-agnostic fallback set.
- [`src/live_data.h`](./src/live_data.h) — header-only data/storage/transport layer (Wi-Fi reconnect, ESPN fetch, JSON ingest). No drawing code.
- [`src/nfl_logos.h`](./src/nfl_logos.h) — shim that reuses the round tracker's baked NFL logo bitmaps via `#include "../../src/nfl_logos.h"` (no 223KB copy).
- [`src/cyd_ui.h`](./src/cyd_ui.h) — all drawing: `cyd_draw_score_tracker`, `cyd_draw_sync_screen`, `cyd_draw_detail_screen`, and `cyd_touch_hit` (LANDSCAPE 320x240, 3 cards/page).
- [`src/main.cpp`](./src/main.cpp) — setup/loop, `LGFX_Sprite canvas` (320x240, 16-bit), serial command dispatch, touch debounce, Hermite auto-advance (`HOLD_DURATION_MS = 6000`, `TRANSITION_DURATION_MS = 700`), BOOT button.

### UX / Aesthetic
Landscape 320x240, single full-screen sprite pushed once per frame. Calm "zen" palette: BG `0x1103`, text `0xDEB8`, records `0x84B0`, live accent `0x39E4`, winner gold `0xFE60`, card bg `0x18E3`. Header bar (league/title/page/paused), 3 game cards per page (2 team rows each), footer pager dots + touch hints. Touch: bottom-left third = PREV, bottom-right third = NEXT, bottom-center = PAUSE/RESUME, tap a card = detail overlay (tap anywhere to return).

## Serial Commands (`/dev/ttyUSB0` @ 115200)
Protocol is **identical** to the round tracker, so existing host tools work:
- `NEXT` / `PREV` / `SET <n>` — navigation.
- `PAUSE` / `RESUME` — auto-advance control.
- `SCREENSHOT` — streams `320*240*2 = 153600` raw RGB565 bytes of the **sprite** between `SCREENSHOT_START` / `SCREENSHOT_END`.
- `SYNC_START` — syncing progress screen.
- `SCORES_START <n>` + per-line JSON + `SCORES_END` (or `ABORT`) — handshaked score push.
- `SCORES_JSON <array>` — ingest a full JSON array of games.
- `WIFI OFF` / `WIFI ON` — toggle the Wi-Fi radio.
- `STATUS` — Wi-Fi state, loaded count, page, paused.
- `REFRESH` (alias `SYNC`) — force an ESPN refresh.
- `TOUCHCAL` — stream raw XPT2046 coordinates for calibration.
- `TOUCHDEBUG` — stream decoded touch coordinates for verification.

## Touch Calibration (`TOUCHCAL`)
1. Send `TOUCHCAL` over serial; the firmware streams raw touch coordinates.
2. Tap near each corner and note the min/max raw X/Y values.
3. Update `XPT_XMIN` / `XPT_XMAX` / `XPT_YMIN` / `XPT_YMAX` in [`src/LGFX_config.h`](./src/LGFX_config.h) and reflash.
4. Use `TOUCHDEBUG` any time to verify decoded coordinates.

## Build, Flash, Capture & Push
```bash
# Build (Conductor-owned verify)
uvx platformio run

# Flash
uvx platformio run -t upload

# Capture a screenshot (dependency-free, 320x240)
python3 tools/capture_screenshot.py --port /dev/ttyUSB0

# Push live scores by reusing the parent tool with the CYD port
python3 ../tools/push_scores.py --port /dev/ttyUSB0
python3 ../tools/push_scores.py --port /dev/ttyUSB0 --league nfl
python3 ../tools/push_scores.py --port /dev/ttyUSB0 --watch --interval 30
```

## Open Items
- **Touch calibration values need on-hardware tuning.** The `XPT_*` defaults are starting points only; run `TOUCHCAL` and update [`src/LGFX_config.h`](./src/LGFX_config.h).
- **`ENABLE_WIFI` defaults to 0** (USB push mode). Set to 1 in [`src/config.h`](./src/config.h) for standalone Wi-Fi/ESPN polling.
- **Screenshot byte order:** [`tools/capture_screenshot.py`](./tools/capture_screenshot.py) defaults to high-byte-first (`--byte-order be`) to match the round tracker's capture tool; if colors render swapped on hardware, use `--byte-order le`.
- **Factory backup** lives at `../factory_backup_4MB.bin`; restore with `uvx esptool --port /dev/ttyUSB0 -b 921600 write-flash 0x0 ../factory_backup_4MB.bin` if needed.
