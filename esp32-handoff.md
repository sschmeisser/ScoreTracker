# Handoff: ESP32-C3 Round Display Board (NFL Score Tracker)

## Hardware
- **Board:** ESP32-2424S012 ("ESP32-C3+ without touch model") — 1.28" round 240x240 IPS, GC9A01 driver, no touch.
- **Connects via USB-C.**
- **Chip:** ESP32-C3 (QFN32) revision v0.4
- **Features:** Single-core RISC-V @ 160MHz, 4MB Embedded Flash (XMC), 40MHz Crystal
- **MAC:** `70:af:09:1b:58:c0`
- **Interface:** `/dev/ttyACM0` (Native USB-Serial/JTAG)
- **Display:** GC9A01 SPI (SCLK: 6, MOSI: 7, DC: 2, CS: 10, BL: 3 with PWM)
- **Controls:** Physical BOOT button (GPIO 9, active LOW with internal pullup)

## Current Firmware: Multi-Sport Score Tracker (Calm Zen Aesthetic)
- **Application:** Real-time multi-sport carousel tracking live, completed, and scheduled games across 5 leagues:
  - **NFL**: National Football League
  - **CFB**: NCAA College Football (with AP Top 25 rankings e.g. `#1 TEX`, `#5 IU`)
  - **NBA**: National Basketball Association
  - **NHL**: National Hockey League (periods `P1`–`P3`, `OT`, `SO`)
  - **MLS**: Major League Soccer (`1H`, `2H`, `HT`, match minute `64'`)
- **Mellow, Calm UX Design ([`src/nfl_tracker.h`](./src/nfl_tracker.h)):**
  - **Palette:** Deep Forest Moss background (`0x1103`), Warm Sand typography (`0xDEB8`), and Calm Sage records (`0x84B0`). Zero backlight glare, zero eye strain.
  - **Motion:** 6.5s tranquil reading hold (`HOLD_DURATION_MS = 6500`) with an 800ms smooth Hermite cubic liquid glide (`TRANSITION_DURATION_MS = 800`).
  - **Matchup Display (3 games per page):**
    - Status pills: `[ LEAGUE | STATUS ]` (e.g. `[ NFL | HALFTIME ]`, `[ NHL | P1 17:47 ]`, `[ CFB | 8:00 PM ]`). Live games highlighted in muted amber moss (`0x39E4`).
    - Badges & Logos: Official NFL team logos for NFL teams; sleek circular monogram badges with league-tinted serene backgrounds for CFB, NBA, NHL, and MLS.
    - College Rankings: Clean `#1`, `#5`, `#14` prefixes for AP Top 25 ranked teams.
  - **Bottom Pager:** Horizontal dot cluster tracking active page and total pages.
- **Controls & Interaction:**
  - **Physical Button (GPIO 9):** Tapping the physical BOOT button glides smoothly to the next page.
  - **Serial Control (`/dev/ttyACM0`):**
    - `NEXT` / `PREV` / `SET <idx>`: Smooth navigation.
    - `PAUSE` / `RESUME`: Pause or resume auto-advance.
    - `WIFI OFF` / `WIFI ON`: Dynamically control Wi-Fi radio.
    - `SCREENSHOT`: Pull 240x240 RGB565 framebuffer.

## Building, Flashing & Tethered Score Pushing
```bash
# Build firmware
uvx platformio run

# Flash to device
uvx platformio run -t upload

# Push latest live scores across all 5 leagues (NFL, CFB, NBA, NHL, MLS)
python3 tools/push_scores.py

# Push specific league only
python3 tools/push_scores.py --league nfl   # or cfb, nba, nhl, mls

# Push continuously in background during live games (e.g. every 30s)
python3 tools/push_scores.py --watch --interval 30

# Push with floating GTK4 desktop loading popup & notification
python3 tools/sync_popup.py

# Check status of background USB plug-and-play auto-sync service
systemctl --user status scoretracker-sync.service

# Capture live hardware screenshot over USB-CDC directly
python3 tools/capture_screenshot.py --output tools/hardware_screenshot.png
```

## Tethered USB Mode & Automated Plug-and-Play
- **Automated USB Plug Detection**: Systemd user service [`~/.config/systemd/user/scoretracker-sync.service`](file:///home/stefan/.config/systemd/user/scoretracker-sync.service) runs [`tools/scoretracker_daemon.py`](./tools/scoretracker_daemon.py). Whenever the ESP32 (vendor `303a`, product `1001`) is plugged in:
  1. A modern floating GTK4 loading dialog ([`tools/sync_popup.py`](./tools/sync_popup.py)) pops up on the laptop desktop with live progress.
  2. The circular watch screen switches to a dedicated **SYNCING SCORES** screen with a glowing circular progress ring.
  3. Live scores across all 5 leagues are fetched using [`tools/league_fetcher.py`](./tools/league_fetcher.py) and streamed over USB CDC.
  4. The watch smoothly transitions to Page 1 with the updated live games.
  5. A desktop notification (`notify-send`) announces completion and the popup auto-closes.
- **Wi-Fi Silencing**: With `ENABLE_WIFI = 0` (configured in [`src/config.h`](./src/config.h)), the ESP32 disables Wi-Fi scanning and reconnection loops, preventing RF reflection damage and serial disconnect spam when the physical antenna is missing.
- **Auto-Sync Reliability**:
  - `scoretracker_daemon.py` dynamically resolves `WAYLAND_DISPLAY`, `DISPLAY`, and DBus endpoints from `systemctl --user show-environment`, ensuring GTK popup dialogs render properly even if the daemon launched before the Wayland compositor at system boot.
  - `sync_popup.py` incorporates an automatic headless fallback if a display server is temporarily unavailable.
  - `src/main.cpp` uses a dedicated line-accumulation buffer for USB-CDC ingestion, preventing `readStringUntil` timeout truncation on multi-league JSON payloads.
- **Multi-League Official Logos**:
  - Sourced and cached 593 official high-resolution transparent logos directly from ESPN CDN via [`tools/source_all_logos.py`](./tools/source_all_logos.py):
    - **NFL**: 32/32 teams in [`tools/logos_cache/nfl/`](./tools/logos_cache/nfl/)
    - **NBA**: 30/30 teams in [`tools/logos_cache/nba/`](./tools/logos_cache/nba/)
    - **NHL**: 32/32 teams in [`tools/logos_cache/nhl/`](./tools/logos_cache/nhl/)
    - **MLS**: 30/30 teams in [`tools/logos_cache/mls/`](./tools/logos_cache/mls/)
    - **CFB**: 464 NCAA teams (all FBS programs & Top 25) in [`tools/logos_cache/cfb/`](./tools/logos_cache/cfb/)
    - **League Insignias**: Official shield logos for all 5 leagues (`tools/logos_cache/*_shield.png`)
  - Manifest and metadata stored at [`tools/logos_cache/teams_manifest.json`](./tools/logos_cache/teams_manifest.json) with team abbreviations, hex colors, and CDN sources.
  - Visual contact sheet generated at [`tools/all_league_logos_preview.png`](./tools/all_league_logos_preview.png).
- **Serial Commands (`/dev/ttyACM0` @ 115200)**:
  - `SYNC_START`: Puts watch in circular syncing progress screen.
  - `SCORES_START <count>` ... `<json_game_lines>` ... `SCORES_END`: Stream live game data with per-line ACK handshaking.
  - `SCORES_JSON <array>`: Ingest full JSON array of games.
  - `WIFI OFF` / `WIFI ON`: Dynamically disable or enable Wi-Fi radio.
  - `STATUS`: Query current Wi-Fi status, loaded game count, active page, and auto-advance state.
  - `NEXT` / `PREV` / `SET <page>`: Navigation.
  - `PAUSE` / `RESUME`: Pause or resume auto-page cycling.
  - `SCREENSHOT`: Pull 240x240 RGB565 framebuffer.
