import os, sys, time, select, argparse, termios, tty
from PIL import Image

WIDTH = 320
HEIGHT = 240
TOTAL_BYTES = WIDTH * HEIGHT * 2

def capture(port="/dev/ttyUSB0", out_path="/home/stefan/Projects/ScoreTracker/tools/hardware_screenshot.png", width=320, height=240):
    global WIDTH, HEIGHT, TOTAL_BYTES
    WIDTH = width
    HEIGHT = height
    TOTAL_BYTES = WIDTH * HEIGHT * 2
    print(f"Connecting to {port} ({WIDTH}x{HEIGHT})...")
    fd = os.open(port, os.O_RDWR | os.O_NOCTTY)
    try:
        tty.setraw(fd)
        attr = termios.tcgetattr(fd)
        attr[4] = termios.B115200
        attr[5] = termios.B115200
        termios.tcsetattr(fd, termios.TCSANOW, attr)
    except Exception as e:
        print(f"Warning: could not configure tty attributes: {e}")
    time.sleep(0.5)



    # Clear any residual data in buffer
    while True:
        r, _, _ = select.select([fd], [], [], 0.05)
        if r:
            os.read(fd, 4096)
        else:
            break

    print("Requesting SCREENSHOT from ESP32...")
    os.write(fd, b"SCREENSHOT\n")

    # Wait for "SCREENSHOT_START"
    line_buf = b""
    start_time = time.time()
    started = False

    while time.time() - start_time < 6.0:
        r, _, _ = select.select([fd], [], [], 0.2)
        if r:
            chunk = os.read(fd, 256)
            line_buf += chunk
            if b"SCREENSHOT_START\r\n" in line_buf or b"SCREENSHOT_START\n" in line_buf:
                started = True
                idx = line_buf.find(b"SCREENSHOT_START")
                # Strip up to newline after SCREENSHOT_START
                nl_idx = line_buf.find(b"\n", idx)
                raw_data = line_buf[nl_idx + 1:]
                break

    if not started:
        print("Error: Did not receive SCREENSHOT_START")
        os.close(fd)
        return False

    print(f"Reading {TOTAL_BYTES} bytes of frame data...")
    raw_bytes = bytearray(raw_data)
    read_start = time.time()
    while len(raw_bytes) < TOTAL_BYTES and (time.time() - read_start < 30.0):
        r, _, _ = select.select([fd], [], [], 0.2)
        if r:
            chunk = os.read(fd, 16384)
            if chunk:
                raw_bytes.extend(chunk)

    os.close(fd)
    print(f"Received {len(raw_bytes)} bytes.")

    if len(raw_bytes) < TOTAL_BYTES - 256:
        print(f"Error: Incomplete frame (got {len(raw_bytes)}/{TOTAL_BYTES})")
        return False

    if len(raw_bytes) < TOTAL_BYTES:
        raw_bytes.extend(b"\x00" * (TOTAL_BYTES - len(raw_bytes)))
    else:
        raw_bytes = raw_bytes[:TOTAL_BYTES]

    # Convert RGB565 to RGB888 image
    # Note: LovyanGFX uses 16-bit RGB565 little-endian or big-endian
    img = Image.new("RGB", (WIDTH, HEIGHT))
    pixels = img.load()

    for y in range(HEIGHT):
        for x in range(WIDTH):
            idx = (y * WIDTH + x) * 2
            b0 = raw_bytes[idx]
            b1 = raw_bytes[idx + 1]
            # LovyanGFX sprite buffer is big-endian RGB565: b0 is high byte, b1 is low byte
            raw = (b0 << 8) | b1
            r = ((raw >> 11) & 0x1F) * 255 // 31
            g = ((raw >> 5) & 0x3F) * 255 // 63
            b = (raw & 0x1F) * 255 // 31
            pixels[x, y] = (r, g, b)


    img.save(out_path)

    print(f"Successfully saved hardware screenshot to {out_path}")
    return True

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Capture hardware screenshot from ESP32")
    parser.add_argument("--port", default="/dev/ttyUSB0", help="Serial port")
    parser.add_argument("--width", type=int, default=320, help="Screen width")
    parser.add_argument("--height", type=int, default=240, help="Screen height")
    parser.add_argument("--output", default="/home/stefan/Projects/ScoreTracker/tools/hardware_screenshot.png", help="Output file path")
    args = parser.parse_args()

    success = capture(port=args.port, out_path=args.output, width=args.width, height=args.height)
    if not success:
        sys.exit(1)

