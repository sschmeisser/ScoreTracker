#!/usr/bin/env python3
import os
import sys
import time
import zlib
import select
import struct
import termios
import tty

WIDTH = 320
HEIGHT = 240
TOTAL_BYTES = WIDTH * HEIGHT * 2

def write_png(path, width, height, rgb_bytes):
    stride = width * 3
    raw = bytearray()
    for y in range(height):
        raw.append(0)
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
    out = bytearray(WIDTH * HEIGHT * 3)
    o = 0
    for i in range(0, TOTAL_BYTES, 2):
        b0 = raw_bytes[i]
        b1 = raw_bytes[i + 1]
        val = (b0 << 8) | b1 if byte_order == "be" else b0 | (b1 << 8)
        r = ((val >> 11) & 0x1F) * 255 // 31
        g = ((val >> 5) & 0x3F) * 255 // 63
        b = (val & 0x1F) * 255 // 31
        out[o] = r
        out[o + 1] = g
        out[o + 2] = b
        o += 3
    return out

def flush_fd(fd):
    while True:
        r, _, _ = select.select([fd], [], [], 0.05)
        if r:
            os.read(fd, 4096)
        else:
            break

def capture_frame(fd):
    flush_fd(fd)
    os.write(fd, b"SCREENSHOT\n")
    line_buf = b""
    t0 = time.time()
    started = False
    raw_data = bytearray()
    
    while time.time() - t0 < 4.0:
        r, _, _ = select.select([fd], [], [], 0.1)
        if not r:
            continue
        chunk = os.read(fd, 512)
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
        print("Failed to get SCREENSHOT_START")
        return None

    read_t = time.time()
    while len(raw_data) < TOTAL_BYTES and (time.time() - read_t < 20.0):
        r, _, _ = select.select([fd], [], [], 0.2)
        if r:
            raw_data.extend(os.read(fd, 8192))

    if len(raw_data) < TOTAL_BYTES:
        print(f"Incomplete frame: {len(raw_data)}/{TOTAL_BYTES}")
        return None

    return rgb565_to_rgb888(raw_data[:TOTAL_BYTES], byte_order="be")

def main():
    port = "/dev/ttyUSB0"
    num_pages = int(sys.argv[1]) if len(sys.argv) > 1 else 5
    out_dir = sys.argv[2] if len(sys.argv) > 2 else "Projects/ScoreTracker/tools"
    
    print(f"Opening {port} for multi-page capture ({num_pages} pages)...")
    fd = os.open(port, os.O_RDWR | os.O_NOCTTY)
    attrs = termios.tcgetattr(fd)
    tty.setraw(fd)
    attrs[4] = termios.B115200
    attrs[5] = termios.B115200
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    
    time.sleep(2.0)
    flush_fd(fd)
    
    for p in range(num_pages):
        out_path = os.path.join(out_dir, f"cyd_page_{p}.png")
        print(f"Capturing page {p} -> {out_path}...")
        rgb = capture_frame(fd)
        if rgb:
            write_png(out_path, WIDTH, HEIGHT, rgb)
            print(f"  Saved page {p}")
        else:
            print(f"  Failed page {p}")
            
        if p < num_pages - 1:
            os.write(fd, b"NEXT\n")
            time.sleep(1.0) # allow 700ms Hermite transition to complete

    os.close(fd)
    print("Multi-page capture complete!")

if __name__ == "__main__":
    main()
