# Handoff: ESP32-C3 Round Touchscreen Board

## Hardware
- Board: "ESP32-C3+ without touch model" — 1.28" round 240x240 IPS, GC9A01 driver, LVGL demo preloaded, no touch.
- Connects via USB-C.
- Chip: ESP32-C3 (QFN32) revision v0.4
- Features: Wi-Fi, BLE 5, Single Core @ 160MHz, 4MB Embedded Flash (XMC), 40MHz Crystal
- MAC: `70:af:09:1b:58:c0`
- Interface: `/dev/ttyACM0` (Native USB-Serial/JTAG)

## Status so far
- Factory firmware dumped to [`factory_backup_4MB.bin`](./factory_backup_4MB.bin).
- Permanent udev rule installed to `/etc/udev/rules.d/99-esp32.rules` (auto 0666 on reset/reconnect).
- **Currently Running**: Retro 4-Sport Kunio-kun / Neo-Geo Championship (Soccer ⚽ ➔ Ice Hockey 🏒 ➔ Basketball 🏀 ➔ Baseball ⚾).
- **Phase 1 Stadium Surfaces ([`src/stadium_render.h`](./src/stadium_render.h))**:
  - Circular ambient floodlight vignette with Hermite smoothstep falloff framing the 240x240 circular bezel.
  - Alternating mowed lawn bands with cross-hatch turf stippling and worn goal mouth grass.
  - Solid, uniform, brilliant white ice sheet with ZERO pink stripes, regulation red/blue lines, and goal creases.
  - Varnished parquet hardwood floor in honey/amber oak planks with center court basketball seam emblem and arena spotlight sheen.
  - Terracotta clay infield dirt with 3D drop-shadowed bases and warning track.
- **Phase 2 16-Bit Character Engine ([`src/player_sprites.h`](./src/player_sprites.h))**:
  - 3-tone shading (sunlit highlight, midtone, deep shadow) and sub-pixel anti-aliasing outlines.
  - Dynamic body lean along velocity vectors during sprints and directional evasion cuts.
  - 6-frame fluid stride cycles with rhythmic arm pumping, knee lifts, and footwear details (cleats, skates, sneakers).
  - Sport-specific athletic poses: diving goalkeeper, skate lunge two-handed stick slash, airborne jump-block swat, and coiled batting backswing.
- **Single-Button Baseball Duel & Vertical Batting Gauge**:
  - Rebuilt baseball diamond layout: Pitcher at (120, 140), Catcher at (120, 206), Batter at (110, 193).
  - Downward pitch trajectory toward home plate ($vy > 0$).
  - Vertical batting timing gauge at $x=208, y=155..205$ with a vibrant green sweet-spot target in the exact center ($y=180$).
  - Dynamic floater cursor tracks pitch velocity.
  - Pressing the single button swings the bat: timing close to the middle sweet-spot launches a towering Home Run over the outfield fence!
  - Strict home run validation: ball must travel upward ($vy < 0$) over the fence ($y < 52$)—pitches thrown down can never trigger false home runs. Unhit pitches are caught cleanly by the catcher and returned to the mound.
- **Persistent Memory & RST Button Cycling**: The board saves the current sport to NVS flash via `Preferences`. Every time the physical **RST** button is clicked (or power cycled), the ESP32 boots up and automatically advances to the next sport in sequence!
- **Zero-Lag Button Gestures**:
  - Short Tap (<300ms): Shoot, Slam Dunk, or Baseball Batting Swing.
  - Long Press (>=300ms): Ball carrier evades UP or DOWN toward whichever wall is further away (`dist_top > dist_bot ? UP : DOWN`), with golden dash particles and tackle whiff immunity.
- **Visual QA Certified**: Automated inspection suite (`tools/qa_inspect_frames.py`) verifies frame buffers, RGB565 balance, and zero color skew (`tools/qa_hockey.png`, `tools/qa_baseball.png`).
- Time clock easter egg randomly flashes "avan", "merdan", or "rojda" for 1-second intervals.

## Permanent Fixes (Saved for reference)
1. **Permanent udev rule**:
   ```bash
   echo 'KERNEL=="ttyACM[0-9]*", MODE="0666"' | sudo tee /etc/udev/rules.d/99-esp32.rules && sudo udevadm control --reload-rules && sudo udevadm trigger
   ```
2. **User group**:
   ```bash
   sudo usermod -a -G uucp $USER
   ```

## Factory Firmware Backup
- Backup file: [`factory_backup_4MB.bin`](file:///home/stefan/Projects/ScoreTracker/factory_backup_4MB.bin) (4,194,304 bytes)
- SHA256: `0bf2dbeb8c11dcd44f9efabf22e1b1752f23b6156223937a9537508068ab0306`
- **To restore factory firmware anytime**:
  ```bash
  uvx esptool --port /dev/ttyACM0 -b 921600 write-flash 0x0 factory_backup_4MB.bin
  ```

## ScoreTracker Project Requirements & Decisions
- Firmware framework: (e.g. PlatformIO + Arduino-ESP32 / ESP-IDF)
- Display library: LovyanGFX or TFT_eSPI (configured for GC9A01 240x240)
- Inputs/Controls: (e.g., buttons, web interface, BLE, Wi-Fi sync, etc.)
