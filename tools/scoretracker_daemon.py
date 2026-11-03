#!/usr/bin/env python3
"""
scoretracker_daemon.py - Background monitor daemon for ScoreTracker.
Listens for USB plug events. When the ESP32 ScoreTracker is plugged in,
it automatically launches the sync popup to push the latest scores.
"""

import os
import sys
import time
import signal
import subprocess

PROJECT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
POPUP_SCRIPT = os.path.join(PROJECT_DIR, "tools", "sync_popup.py")

last_sync_time = 0
DEBOUNCE_SECONDS = 6.0

def is_scoretracker_device(port):
    if not os.path.exists(port):
        return False
    try:
        out = subprocess.check_output(["udevadm", "info", "-a", "-n", port], text=True, stderr=subprocess.DEVNULL)
        # Supports ESP32 native USB-CDC (303a), CH340 on CYD (1a86), CP210x (10c4), FTDI (0403)
        return any(vid in out for vid in ['idVendor=="303a"', 'idVendor=="1a86"', 'idVendor=="10c4"', 'idVendor=="0403"'])
    except Exception:
        return False

def clean_env_value(v):
    if v.startswith(r"$'") and v.endswith("'"):
        try:
            return v[2:-1].encode("utf-8").decode("unicode_escape")
        except Exception:
            return v[2:-1]
    if (v.startswith("'") and v.endswith("'")) or (v.startswith('"') and v.endswith('"')):
        return v[1:-1]
    return v

def get_desktop_env():
    env = os.environ.copy()
    try:
        out = subprocess.check_output(["systemctl", "--user", "show-environment"], text=True, stderr=subprocess.DEVNULL)
        for line in out.splitlines():
            if "=" in line:
                k, v = line.split("=", 1)
                env[k] = clean_env_value(v)
    except Exception:
        pass
    runtime_dir = env.get("XDG_RUNTIME_DIR", f"/run/user/{os.getuid()}")
    if "WAYLAND_DISPLAY" not in env:
        for w in ["wayland-1", "wayland-0"]:
            if os.path.exists(os.path.join(runtime_dir, w)):
                env["WAYLAND_DISPLAY"] = w
                break
    if "DISPLAY" not in env:
        env["DISPLAY"] = ":0"
    return env

def trigger_sync(port="/dev/ttyACM0"):
    global last_sync_time
    now = time.time()
    if now - last_sync_time < DEBOUNCE_SECONDS:
        return
    last_sync_time = now
    
    desktop_env = get_desktop_env()
    print(f"[DAEMON] ScoreTracker detected on {port}! Launching sync popup (WAYLAND_DISPLAY={desktop_env.get('WAYLAND_DISPLAY')}, DISPLAY={desktop_env.get('DISPLAY')})...")
    try:
        subprocess.Popen([sys.executable, POPUP_SCRIPT, port], env=desktop_env)
    except Exception as e:
        print(f"[DAEMON] Error launching sync popup: {e}")

def main():
    print("[DAEMON] ScoreTracker auto-sync monitor started.")
    
    # Check if already plugged in on start
    for p in ["/dev/ttyACM0", "/dev/ttyACM1", "/dev/ttyUSB0", "/dev/ttyUSB1"]:
        if is_scoretracker_device(p):
            print(f"[DAEMON] Device already present on {p}. Triggering initial sync...")
            trigger_sync(p)
            break

    # Monitor kernel uevents for tty subsystem
    proc = subprocess.Popen(
        ["udevadm", "monitor", "--kernel", "--subsystem-match=tty"],
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        text=True,
        bufsize=1
    )

    def handle_exit(signum, frame):
        print("[DAEMON] Stopping monitor...")
        proc.terminate()
        sys.exit(0)

    signal.signal(signal.SIGINT, handle_exit)
    signal.signal(signal.SIGTERM, handle_exit)

    for line in iter(proc.stdout.readline, ""):
        line = line.strip()
        # Look for add events on ttyACM or ttyUSB
        if "add" in line and ("ttyACM" in line or "ttyUSB" in line):
            parts = line.split()
            dev_node = None
            for part in parts:
                if "ttyACM" in part or "ttyUSB" in part:
                    dev_name = part.split("/")[-1].strip("()")
                    dev_node = f"/dev/{dev_name}"
                    break
            
            if not dev_node:
                dev_node = "/dev/ttyUSB0" if "ttyUSB" in line else "/dev/ttyACM0"

            # Wait briefly for device permissions to settle
            time.sleep(0.6)
            if is_scoretracker_device(dev_node):
                trigger_sync(dev_node)

if __name__ == "__main__":
    main()
