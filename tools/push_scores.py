#!/usr/bin/env python3
"""
push_scores.py - Fetch live NFL scores from ESPN and push to ESP32-2424S012 via USB Serial.
Zero external Python dependencies (uses standard library: urllib, json, os, time, select, argparse).
"""

import os
import sys
import time
import json
import select
import termios
import tty
import argparse
import datetime
from zoneinfo import ZoneInfo
import urllib.request
from league_fetcher import fetch_all_multileague_games, fetch_espn_json, parse_games_for_league, LEAGUE_ENDPOINTS

ESPN_SCOREBOARD_URL = "http://site.api.espn.com/apis/site/v2/sports/football/nfl/scoreboard"

def fetch_espn_data(url):
    req = urllib.request.Request(url, headers={"User-Agent": "curl/8.0.0"})
    with urllib.request.urlopen(req, timeout=10) as resp:
        return json.loads(resp.read().decode("utf-8"))

def parse_iso_datetime(iso_str):
    """Convert ISO8601 UTC string ('2026-09-25T00:15Z') to US Pacific day & date."""
    if not iso_str or len(iso_str) < 16:
        return "SUN", "SEP 27", "1:00 PM"
    try:
        # Strip trailing Z
        cleaned = iso_str.replace("Z", "")
        # Parse UTC
        dt_utc = datetime.datetime.fromisoformat(cleaned)
        if dt_utc.tzinfo is None:
            dt_utc = dt_utc.replace(tzinfo=datetime.timezone.utc)
        dt_pacific = dt_utc.astimezone(ZoneInfo("America/Los_Angeles"))
        
        day_str = dt_pacific.strftime("%a").upper()
        date_str = dt_pacific.strftime("%b %d").upper()
        
        # Time string in 12-hour format without leading zero
        hour = dt_pacific.hour % 12
        if hour == 0:
            hour = 12
        time_str = f"{hour}:{dt_pacific.strftime('%M %p')}"
        return day_str, date_str, time_str
    except Exception:
        return "SUN", "SEP 27", "1:00 PM"

def parse_games_from_espn(espn_json):
    games = []
    events = espn_json.get("events", [])
    
    for ev in events:
        status_type = ev.get("status", {}).get("type", {})
        state = status_type.get("state", "pre")  # 'pre', 'in', 'post'
        detail = status_type.get("shortDetail", "FINAL")
        iso_date = ev.get("date", "")
        day_str, date_str, kickoff_time = parse_iso_datetime(iso_date)
        
        # Format status badge text
        if state == "in":
            # Live game: e.g. "4:48 - 2nd" or "Halftime"
            status_text = detail
            if "Halftime" in detail or "End of" in detail:
                status_text = detail.upper()
            elif " - " in detail:
                # e.g. "4:48 - 2nd" -> "2nd 4:48" or keep as is
                parts = detail.split(" - ")
                if len(parts) == 2:
                    clock, period = parts[0], parts[1]
                    status_text = f"{period} {clock}"
                else:
                    status_text = detail
        elif state == "post":
            if "OT" in detail:
                status_text = "FINAL/OT"
            else:
                status_text = "FINAL"
        else:
            # Upcoming game: show kickoff time e.g. "1:00 PM"
            status_text = kickoff_time
            
        comp = ev.get("competitions", [{}])[0]
        venue = comp.get("venue", {}).get("fullName", "NFL Stadium")
        
        competitors = comp.get("competitors", [])
        if len(competitors) < 2:
            continue
            
        c0 = competitors[0]
        c1 = competitors[1]
        away_c = c1 if c1.get("homeAway") == "away" else c0
        home_c = c0 if c0.get("homeAway") == "home" else c1
        
        def extract_team(c):
            code = c.get("team", {}).get("abbreviation", "NFL")
            if code == "WSH":
                code = "WAS"
            name = c.get("team", {}).get("shortDisplayName", code)
            rec = ""
            records = c.get("records", [])
            if records:
                rec = records[0].get("summary", "")
            score = int(c.get("score", 0))
            linescores = [int(q.get("value", 0)) for q in c.get("linescores", [])][:4]
            while len(linescores) < 4:
                linescores.append(0)
            return code, name, rec, score, linescores

        away_code, away_name, away_rec, away_score, away_q = extract_team(away_c)
        home_code, home_name, home_rec, home_score, home_q = extract_team(home_c)
        
        # Leader or venue
        leader_str = venue
        leaders = comp.get("leaders", [])
        if leaders:
            top_l = leaders[0].get("leaders", [])
            if top_l:
                ath = top_l[0].get("athlete", {}).get("shortName", "")
                val = top_l[0].get("displayValue", "")
                if ath and val:
                    leader_str = f"{ath}: {val}"

        games.append({
            "state": state,
            "day": day_str,
            "date": date_str,
            "status": status_text[:23],
            "away": away_code[:7],
            "away_name": away_name[:23],
            "away_rec": away_rec[:11],
            "away_score": away_score,
            "away_q": away_q,
            "home": home_code[:7],
            "home_name": home_name[:23],
            "home_rec": home_rec[:11],
            "home_score": home_score,
            "home_q": home_q,
            "leader": leader_str[:63],
            "stadium": venue[:47]
        })
        
    return games

def fetch_all_relevant_games(include_prev_week=True):
    print("[ESPN] Fetching live NFL scoreboard...")
    curr_data = fetch_espn_data(ESPN_SCOREBOARD_URL)
    week_num = curr_data.get("week", {}).get("number", 1)
    print(f"[ESPN] Retrieved Week {week_num} data.")
    
    curr_games = parse_games_from_espn(curr_data)
    
    live_games = [g for g in curr_games if g["state"] == "in"]
    final_games = [g for g in curr_games if g["state"] == "post"]
    pre_games = [g for g in curr_games if g["state"] == "pre"]
    
    print(f"[ESPN] Week {week_num} breakdown: {len(live_games)} live, {len(final_games)} final, {len(pre_games)} scheduled.")
    
    # If this week has very few completed games (e.g. Thursday or mid-week),
    # fetch previous week completed games to fill the screen with exciting action
    prev_games = []
    if include_prev_week and len(final_games) < 4 and week_num > 1:
        try:
            prev_url = f"{ESPN_SCOREBOARD_URL}?week={week_num - 1}"
            print(f"[ESPN] Fetching Week {week_num - 1} completed games...")
            prev_data = fetch_espn_data(prev_url)
            all_prev = parse_games_from_espn(prev_data)
            prev_games = [g for g in all_prev if g["state"] == "post"]
            print(f"[ESPN] Retrieved {len(prev_games)} completed games from Week {week_num - 1}.")
        except Exception as e:
            print(f"[ESPN] Warning: could not fetch previous week: {e}")
            
    # Priority sorting:
    # 1. Live games right now (e.g. Falcons @ Packers)
    # 2. Completed games from this week
    # 3. Completed games from last week (up to 6)
    # 4. Upcoming scheduled games for this week
    combined = []
    combined.extend(live_games)
    combined.extend(final_games)
    if prev_games:
        # Take up to 6 exciting completed games from last week
        combined.extend(prev_games[:6])
    combined.extend(pre_games)
    
    # Limit to 32 games (hardware max storage)
    return combined[:32]

def send_scores_to_serial(games, port="/dev/ttyACM0"):
    print(f"[SERIAL] Connecting to {port}...")
    fd = os.open(port, os.O_RDWR | os.O_NOCTTY)
    try:
        attrs = termios.tcgetattr(fd)
        tty.setraw(fd)
        attrs[4] = termios.B115200
        attrs[5] = termios.B115200
        termios.tcsetattr(fd, termios.TCSANOW, attrs)
    except Exception:
        pass
    # If using USB-UART (CH340/CP210x/FTDI), DTR toggle causes ESP32 reset; wait for boot
    if "USB" in port:
        time.sleep(2.0)
    else:
        time.sleep(0.2)
    
    # Flush existing buffer
    while True:
        r, _, _ = select.select([fd], [], [], 0.05)
        if r:
            os.read(fd, 4096)
        else:
            break
            
    total = len(games)
    start_cmd = f"SCORES_START {total}\n".encode("utf-8")
    print(f"[SERIAL] Sending SCORES_START for {total} games...")
    os.write(fd, start_cmd)
    
    # Wait for READY
    ready = False
    rx_buf = b""
    t0 = time.time()
    while time.time() - t0 < 3.0:
        r, _, _ = select.select([fd], [], [], 0.05)
        if r:
            rx_buf += os.read(fd, 512)
            if b"READY" in rx_buf:
                ready = True
                break
                
    if not ready:
        print("[SERIAL] Device didn't send READY, proceeding anyway...")
        
    # Send each game as a single JSON line and wait for ACK
    loaded_count = 0
    for i, g in enumerate(games):
        line = json.dumps(g, separators=(',', ':')) + "\n"
        os.write(fd, line.encode("utf-8"))
        
        # Wait for ACK or NACK
        t_ack = time.time()
        ack_buf = b""
        while time.time() - t_ack < 2.0:
            r, _, _ = select.select([fd], [], [], 0.02)
            if r:
                chunk = os.read(fd, 512)
                ack_buf += chunk
                if b"ACK" in ack_buf:
                    loaded_count += 1
                    break
                elif b"NACK" in ack_buf:
                    print(f"[SERIAL] Warning: Game {i + 1} was NACKed: {ack_buf.decode('utf-8', errors='replace').strip()}")
                    break
                    
    os.write(fd, b"SCORES_END\n")
    
    # Wait for SUCCESS
    success = False
    rx_buf = b""
    t0 = time.time()
    while time.time() - t0 < 4.0:
        r, _, _ = select.select([fd], [], [], 0.1)
        if r:
            rx_buf += os.read(fd, 512)
            if b"SUCCESS" in rx_buf:
                success = True
                break
                
    os.close(fd)
    
    if success:
        print(f"[SERIAL] Successfully updated ESP32 with {total} games!")
        return True
    else:
        print(f"[SERIAL] Finished transfer (received: {rx_buf.decode('utf-8', errors='replace').strip()})")
        return True

def capture_screenshot(port="/dev/ttyACM0", out_path="/home/stefan/Projects/ScoreTracker/tools/hardware_screenshot.png"):
    try:
        from PIL import Image
    except ImportError:
        print("[SCREENSHOT] PIL not installed, skipping screenshot capture.")
        return
        
    print(f"[SCREENSHOT] Requesting display screenshot from {port}...")
    fd = os.open(port, os.O_RDWR | os.O_NOCTTY)
    try:
        tty.setraw(fd)
    except Exception:
        pass
    time.sleep(0.3)
    
    # Flush
    while True:
        r, _, _ = select.select([fd], [], [], 0.05)
        if r:
            os.read(fd, 4096)
        else:
            break
            
    os.write(fd, b"SCREENSHOT\n")
    
    line_buf = b""
    t0 = time.time()
    started = False
    raw_data = bytearray()
    TOTAL_BYTES = 240 * 240 * 2
    
    while time.time() - t0 < 4.0:
        r, _, _ = select.select([fd], [], [], 0.1)
        if r:
            chunk = os.read(fd, 4096)
            line_buf += chunk
            if not started and b"SCREENSHOT_START" in line_buf:
                started = True
                idx = line_buf.find(b"SCREENSHOT_START")
                nl_idx = line_buf.find(b"\n", idx)
                if nl_idx >= 0:
                    raw_data = bytearray(line_buf[nl_idx + 1:])
                break
                
    if not started:
        print("[SCREENSHOT] Error: Did not receive SCREENSHOT_START")
        os.close(fd)
        return
        
    while len(raw_data) < TOTAL_BYTES and (time.time() - t0 < 6.0):
        r, _, _ = select.select([fd], [], [], 0.1)
        if r:
            raw_data.extend(os.read(fd, 8192))
            
    os.close(fd)
    
    if len(raw_data) >= TOTAL_BYTES:
        pixels = []
        for i in range(0, TOTAL_BYTES, 2):
            raw_val = raw_data[i] | (raw_data[i + 1] << 8)
            r5 = (raw_val >> 11) & 0x1F
            g6 = (raw_val >> 5) & 0x3F
            b5 = raw_val & 0x1F
            pixels.append((int(r5 * 255 / 31), int(g6 * 255 / 63), int(b5 * 255 / 31)))
            
        img = Image.new("RGB", (240, 240))
        img.putdata(pixels)
        img.save(out_path)
        print(f"[SCREENSHOT] Saved hardware display capture to: {out_path}")
    else:
        print(f"[SCREENSHOT] Incomplete frame: {len(raw_data)}/{TOTAL_BYTES} bytes")

def main():
    parser = argparse.ArgumentParser(description="Push live multi-league scores to ESP32 ScoreTracker over USB Serial")
    parser.add_argument("--port", default="/dev/ttyUSB0", help="Serial port (default: /dev/ttyUSB0)")
    parser.add_argument("--league", choices=["all", "nfl", "cfb", "cbb", "nba", "nhl", "mls"], default="all", help="League to push (default: all)")
    parser.add_argument("--watch", action="store_true", help="Continuously poll ESPN and push updates")
    parser.add_argument("--interval", type=int, default=30, help="Poll interval in seconds for --watch (default: 30)")
    parser.add_argument("--screenshot", action="store_true", help="Capture hardware screenshot after push")
    parser.add_argument("--current-week-only", action="store_true", help="Do not include previous week completed games (NFL only)")
    
    args = parser.parse_args()
    
    def run_push():
        league_choice = args.league.upper()
        if league_choice == "ALL":
            games = fetch_all_multileague_games(max_games=32)
        elif league_choice in LEAGUE_ENDPOINTS:
            data = fetch_espn_json(LEAGUE_ENDPOINTS[league_choice])
            games = parse_games_for_league(league_choice, data)[:32]
        else:
            games = fetch_all_relevant_games(include_prev_week=(not args.current_week_only))
            
        print(f"\n[SUMMARY] Prepared {len(games)} games to push:")
        for idx, g in enumerate(games[:8]):
            lg = g.get('league', 'NFL')
            rank_a = f"#{g.get('away_rank', 0)} " if g.get('away_rank', 0) > 0 else ""
            rank_h = f"#{g.get('home_rank', 0)} " if g.get('home_rank', 0) > 0 else ""
            print(f"  {idx + 1:2d}. [{lg:3s}] {rank_a}{g['away']:3s} {g['away_score']:2d} @ {rank_h}{g['home']:3s} {g['home_score']:2d} | {g['status']:14s} | {g['day']} {g['date']}")
        if len(games) > 8:
            print(f"  ... and {len(games) - 8} more games across leagues.")
            
        send_scores_to_serial(games, port=args.port)
        
        if args.screenshot:
            time.sleep(0.5)
            capture_screenshot(port=args.port)

    if args.watch:
        print(f"[WATCH] Starting continuous score polling every {args.interval}s. Press Ctrl+C to stop.")
        while True:
            try:
                run_push()
                time.sleep(args.interval)
            except KeyboardInterrupt:
                print("\n[WATCH] Stopped by user.")
                break
            except Exception as e:
                print(f"[WATCH] Error: {e}, retrying in {args.interval}s...")
                time.sleep(args.interval)
    else:
        run_push()

if __name__ == "__main__":
    main()
