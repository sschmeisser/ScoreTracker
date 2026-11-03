#!/usr/bin/env python3
"""
capture_screenshot.py - Pull the 320x240 RGB565 sprite framebuffer from the CYD
(ESP32-2432S028R "Cheap Yellow Display") over USB serial and write it to a PNG.

Adapted from the round tracker's tools/capture_screenshot.py, but:
  * The CYD sprite is landscape 320x240 (320*240*2 = 153600 bytes).
  * The CYD is a CH340 on /dev/ttyUSB0 (vs /dev/ttyACM0 for the round board).
  * Zero external dependencies (standard library only -- no PIL).

Serial protocol is identical to the round tracker: send "SCREENSHOT\\n", the
firmware replies "SCREENSHOT_START\\n" then streams CYD_W*CYD_H*2 raw RGB565
bytes of the sprite, then "SCREENSHOT_END".
"""

import os
import sys
import time
import zlib
import select
import struct
import argparse
import termios
import tty

WIDTH = 320
HEIGHT = 240
TOTAL_BYTES = WIDTH * HEIGHT * 2  # 153600

DEFAULT_PORT = "/dev/ttyUSB0"
DEFAULT_OUTPUT = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                              "hardware_screenshot.png")

# Streaming 153600 bytes at 115200 baud takes ~13.3s, so be generous.
START_TIMEOUT_S = 5.0
READ_TIMEOUT_S = 40.0
READ_CHUNK = 8192


def write_png(path, width, height, rgb_bytes):
    """Write an 8-bit truecolor PNG from a flat RGB byte string (no deps).

    rgb_bytes must be len(width * height * 3), rows top-to-bottom.
    """
    stride = width * 3
    raw = bytearray()
    for y in range(height):
        raw.append(0)  # filter type 0 (None) for each scanline
        raw.extend(rgb_bytes[y * stride:(y + 1) * stride])

    def chunk(tag, data):
        return (struct.pack(">I", len(data)) + tag + data +
                struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))

    ihdr = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", ihdr))
        f.write(chunk(b"IDAT", zlib.compress(bytes(raw), 9)))
        f.write(chunk(b"IEND", b""))


def rgb565_to_rgb888(raw_bytes, byte_order="be"):
    """Convert 320*240*2 raw RGB565 bytes to a flat RGB888 bytearray.

    byte_order "be" => b0 is the high byte (matches round tracker capture tool).
    byte_order "le" => b0 is the low byte.
    """
    out = bytearray(WIDTH * HEIGHT * 3)
    o = 0
    for i in range(0, TOTAL_BYTES, 2):
        b0 = raw_bytes[i]
        b1 = raw_bytes[i + 1]
        if byte_order == "be":
            val = (b0 << 8) | b1
        else:
            val = b0 | (b1 << 8)
        r = ((val >> 11) & 0x1F) * 255 // 31
        g = ((val >> 5) & 0x3F) * 255 // 63
        b = (val & 0x1F) * 255 // 31
        out[o] = r
        out[o + 1] = g
        out[o + 2] = b
        o += 3
    return out


def _flush(fd):
    while True:
        r, _, _ = select.select([fd], [], [], 0.05)
        if r:
            try:
                os.read(fd, 4096)
            except OSError:
                break
        else:
            break


def capture(port=DEFAULT_PORT, out_path=DEFAULT_OUTPUT, byte_order="be"):
    print(f"Connecting to {port}...")
    try:
        fd = os.open(port, os.O_RDWR | os.O_NONBLOCK)
    except OSError as e:
        print(f"Error: cannot open {port}: {e}")
        print("  Is the CYD plugged in and on /dev/ttyUSB0? Is the user in 'uucp'?")
        return False

    try:
        try:
            attrs = termios.tcgetattr(fd)
            tty.setraw(fd)
            attrs[4] = termios.B115200
            attrs[5] = termios.B115200
            termios.tcsetattr(fd, termios.TCSANOW, attrs)
        except Exception:
            pass
        if "USB" in port:
            time.sleep(2.0)
        else:
            time.sleep(0.5)
        _flush(fd)

        print("Requesting SCREENSHOT from CYD...")
        os.write(fd, b"SCREENSHOT\n")

        # Wait for "SCREENSHOT_START" framing.
        line_buf = b""
        start_time = time.time()
        started = False
        raw_data = bytearray()

        while time.time() - start_time < START_TIMEOUT_S:
            r, _, _ = select.select([fd], [], [], 0.2)
            if not r:
                continue
            chunk = os.read(fd, 256)
            if not chunk:
                continue
            line_buf += chunk
            idx = line_buf.find(b"SCREENSHOT_START")
            if idx >= 0:
                nl_idx = line_buf.find(b"\n", idx)
                if nl_idx >= 0:
                    started = True
                    raw_data = bytearray(line_buf[nl_idx + 1:])
                    break

        if not started:
            print("Error: did not receive SCREENSHOT_START "
                  f"within {START_TIMEOUT_S:.0f}s (device busy or wrong firmware?)")
            return False

        print(f"Reading {TOTAL_BYTES} bytes of frame data "
              f"(320x240 RGB565)...")
        read_start = time.time()
        while len(raw_data) < TOTAL_BYTES and (time.time() - read_start < READ_TIMEOUT_S):
            r, _, _ = select.select([fd], [], [], 0.5)
            if r:
                raw_data.extend(os.read(fd, READ_CHUNK))
    finally:
        os.close(fd)

    print(f"Received {len(raw_data)} bytes.")
    if len(raw_data) < TOTAL_BYTES:
        print(f"Error: incomplete frame (got {len(raw_data)}/{TOTAL_BYTES} bytes)")
        return False

    raw_data = raw_data[:TOTAL_BYTES]
    rgb = rgb565_to_rgb888(raw_data, byte_order=byte_order)
    try:
        write_png(out_path, WIDTH, HEIGHT, rgb)
    except OSError as e:
        print(f"Error: could not write {out_path}: {e}")
        return False

    print(f"Successfully saved hardware screenshot to {out_path}")
    return True


def main():
    parser = argparse.ArgumentParser(
        description="Capture a hardware screenshot from the CYD (320x240 RGB565).")
    parser.add_argument("--port", default=DEFAULT_PORT,
                        help=f"Serial port (default: {DEFAULT_PORT})")
    parser.add_argument("--output", default=DEFAULT_OUTPUT,
                        help=f"Output PNG path (default: {DEFAULT_OUTPUT})")
    parser.add_argument("--byte-order", choices=["be", "le"], default="be",
                        help="RGB565 byte order: 'be' high-byte-first (matches "
                             "round tracker capture tool, default) or 'le'")
    args = parser.parse_args()

    success = capture(port=args.port, out_path=args.output,
                      byte_order=args.byte_order)
    if not success:
        sys.exit(1)


if __name__ == "__main__":
    main()
