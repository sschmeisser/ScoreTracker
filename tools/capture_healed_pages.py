import os, sys, time, select, termios, tty
from PIL import Image

WIDTH = 240
HEIGHT = 240
TOTAL_BYTES = WIDTH * HEIGHT * 2

def capture_page(fd, page_idx, out_path):
    print(f"\n--- Capturing Page {page_idx} -> {out_path} ---")
    
    # Pause auto-tick and set page
    os.write(fd, b"PAUSE\n")
    time.sleep(0.1)
    os.write(fd, f"SET {page_idx}\n".encode())
    time.sleep(0.3)
    
    # Flush any unread text
    while True:
        r, _, _ = select.select([fd], [], [], 0.05)
        if r:
            os.read(fd, 4096)
        else:
            break

    os.write(fd, b"SCREENSHOT\n")
    
    line_buf = b""
    start_time = time.time()
    started = False
    raw_data = b""

    while time.time() - start_time < 3.0:
        r, _, _ = select.select([fd], [], [], 0.2)
        if r:
            chunk = os.read(fd, 256)
            line_buf += chunk
            if b"SCREENSHOT_START\r\n" in line_buf or b"SCREENSHOT_START\n" in line_buf:
                started = True
                idx = line_buf.find(b"SCREENSHOT_START")
                nl_idx = line_buf.find(b"\n", idx)
                raw_data = line_buf[nl_idx + 1:]
                break

    if not started:
        print(f"Error: Did not receive SCREENSHOT_START for page {page_idx}")
        return False

    raw_bytes = bytearray(raw_data)
    read_start = time.time()
    while len(raw_bytes) < TOTAL_BYTES and (time.time() - read_start < 10.0):
        r, _, _ = select.select([fd], [], [], 0.2)
        if r:
            chunk = os.read(fd, 4096)
            raw_bytes.extend(chunk)

    if len(raw_bytes) < TOTAL_BYTES:
        raw_bytes.extend(b"\x00" * (TOTAL_BYTES - len(raw_bytes)))
    else:
        raw_bytes = raw_bytes[:TOTAL_BYTES]

    img = Image.new("RGB", (WIDTH, HEIGHT))
    pixels = img.load()

    for y in range(HEIGHT):
        for x in range(WIDTH):
            idx = (y * WIDTH + x) * 2
            b0 = raw_bytes[idx]
            b1 = raw_bytes[idx + 1]
            raw = (b0 << 8) | b1
            r = ((raw >> 11) & 0x1F) * 255 // 31
            g = ((raw >> 5) & 0x3F) * 255 // 63
            b = (raw & 0x1F) * 255 // 31
            pixels[x, y] = (r, g, b)

    img.save(out_path)
    print(f"Saved {out_path} ({len(raw_bytes)} bytes)")
    return True

def main():
    port = "/dev/ttyACM0"
    pages = [0, 1, 2, 3, 4, 9]
    fd = os.open(port, os.O_RDWR | os.O_NOCTTY)
    try:
        tty.setraw(fd)
    except Exception:
        pass
    time.sleep(0.5)

    try:
        for p in pages:
            out_file = f"/home/stefan/Projects/ScoreTracker/tools/healed_page_{p}.png"
            capture_page(fd, p, out_file)
            time.sleep(0.3)
        # Resume auto-tick
        os.write(fd, b"RESUME\n")
    finally:
        os.close(fd)
    print("\nAll requested screenshots successfully captured!")

if __name__ == "__main__":
    main()
