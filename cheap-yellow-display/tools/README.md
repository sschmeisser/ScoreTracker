# CYD Tools

Host-side utilities for the **ESP32-2432S028R "Cheap Yellow Display"** (CYD)
score tracker. The CYD speaks the **same serial protocol** as the round tracker;
only the USB-serial bridge and framebuffer size differ.

| | Round tracker | CYD |
|---|---|---|
| USB-serial | Native USB-Serial/JTAG | **CH340** |
| Port | `/dev/ttyACM0` | **`/dev/ttyUSB0`** |
| Framebuffer | 240x240 (`115200` bytes) | **320x240 (`153600` bytes)** |
| Sprite | `LGFX_Sprite` | `LGFX_Sprite` (landscape) |

All tools run at **115200 baud**. Your user must be in the `uucp` group to open
the port.

## Build & Flash

Run from `cheap-yellow-display/`:

```bash
# Build firmware
uvx platformio run

# Flash to the CYD
uvx platformio run -t upload
```

`platformio.ini` already sets `upload_port` / `monitor_port` to `/dev/ttyUSB0`,
so no `--upload-port` is needed.

## Capture a Screenshot

`tools/capture_screenshot.py` is standalone and dependency-free (standard library
only — no PIL). It sends `SCREENSHOT`, reads the raw RGB565 sprite, and writes a
PNG.

```bash
# Default: /dev/ttyUSB0 -> tools/hardware_screenshot.png
python3 tools/capture_screenshot.py

# Explicit port and output
python3 tools/capture_screenshot.py --port /dev/ttyUSB0 --output tools/shot.png
```

Notes:
- The transfer takes ~14s (153600 bytes @ 115200 baud); the tool waits up to 40s.
- If colors look byte-swapped, add `--byte-order le` (default `be` matches the
  round tracker's capture tool).
- Errors are reported for a missing device or a missing `SCREENSHOT_START` frame.

## Push Live Scores (reuse the parent tool)

The parent repo's `tools/push_scores.py` already supports `--port`, so **do not
duplicate it** — just point it at the CYD:

```bash
# Push all leagues to the CYD
python3 ../tools/push_scores.py --port /dev/ttyUSB0

# Single league
python3 ../tools/push_scores.py --port /dev/ttyUSB0 --league nfl   # or cfb, nba, nhl, mls

# Continuously poll + push during live games (every 30s)
python3 ../tools/push_scores.py --port /dev/ttyUSB0 --watch --interval 30

# Push then grab a screenshot (requires PIL, which the parent tool uses)
python3 ../tools/push_scores.py --port /dev/ttyUSB0 --screenshot
```

The protocol (`SYNC_START`, `SCORES_START <n>` + JSON lines + `SCORES_END`,
`SCORES_JSON <array>`, `NEXT`/`PREV`/`SET`, `PAUSE`/`RESUME`, `WIFI ON|OFF`,
`STATUS`, `REFRESH`, `SCREENSHOT`) is unchanged, so the round tracker's daemon
and popup tooling work against the CYD by overriding the port to `/dev/ttyUSB0`.

## Touch Calibration

The CYD adds `TOUCHCAL` (streams raw XPT2046 coordinates) and `TOUCHDEBUG`
(streams decoded coordinates). See the main [`../README.md`](../README.md) for the
calibration workflow.
