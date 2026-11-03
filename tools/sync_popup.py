#!/usr/bin/env python3
"""
sync_popup.py - Modern floating GTK4 loading screen & score pusher for ScoreTracker.
Pops up on laptop screen when ESP32-2424S012 is plugged in over USB,
shows live sync progress, updates the watch screen, and closes automatically.
"""

import os
import sys
import time
import json
import select
import termios
import tty
import threading
import subprocess
import datetime
from zoneinfo import ZoneInfo
import urllib.request

import gi
gi.require_version('Gtk', '4.0')
from gi.repository import Gtk, GLib, Gdk

from league_fetcher import fetch_all_multileague_games

PROJECT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

def parse_iso_datetime(iso_str):
    if not iso_str or len(iso_str) < 16:
        return "SUN", "SEP 27", "1:00 PM"
    try:
        cleaned = iso_str.replace("Z", "")
        dt_utc = datetime.datetime.fromisoformat(cleaned)
        if dt_utc.tzinfo is None:
            dt_utc = dt_utc.replace(tzinfo=datetime.timezone.utc)
        dt_pacific = dt_utc.astimezone(ZoneInfo("America/Los_Angeles"))
        day_str = dt_pacific.strftime("%a").upper()
        date_str = dt_pacific.strftime("%b %d").upper()
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
        state = status_type.get("state", "pre")
        detail = status_type.get("shortDetail", "FINAL")
        iso_date = ev.get("date", "")
        day_str, date_str, kickoff_time = parse_iso_datetime(iso_date)
        
        if state == "in":
            status_text = detail
            if "Halftime" in detail or "End of" in detail:
                status_text = detail.upper()
            elif " - " in detail:
                parts = detail.split(" - ")
                if len(parts) == 2:
                    status_text = f"{parts[1]} {parts[0]}"
        elif state == "post":
            status_text = "FINAL/OT" if "OT" in detail else "FINAL"
        else:
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
            rec = c.get("records", [{}])[0].get("summary", "") if c.get("records") else ""
            score = int(c.get("score", 0))
            linescores = [int(q.get("value", 0)) for q in c.get("linescores", [])][:4]
            while len(linescores) < 4:
                linescores.append(0)
            return code, name, rec, score, linescores

        away_code, away_name, away_rec, away_score, away_q = extract_team(away_c)
        home_code, home_name, home_rec, home_score, home_q = extract_team(home_c)
        
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

class SyncAppWindow(Gtk.ApplicationWindow):
    def __init__(self, app, port="/dev/ttyACM0"):
        super().__init__(application=app, title="ScoreTracker Sync")
        self.port = port
        self.set_default_size(380, 200)
        self.set_resizable(False)

        # Apply CSS styling matching Zen Deep Forest Moss palette
        css_provider = Gtk.CssProvider()
        css_data = """
        window {
            background-color: #11201B;
            border: 2px solid #294035;
            border-radius: 14px;
        }
        .title-label {
            color: #DEB894;
            font-size: 16px;
            font-weight: bold;
            font-family: 'Inter', 'FreeSans', sans-serif;
        }
        .sub-label {
            color: #748A7C;
            font-size: 11px;
            font-family: monospace;
        }
        .status-label {
            color: #9DD4A4;
            font-size: 13px;
        }
        progressbar trough {
            background-color: #192C25;
            border-radius: 6px;
            min-height: 10px;
        }
        progressbar progress {
            background-color: #6AE7A0;
            border-radius: 6px;
        }
        """
        css_provider.load_from_data(css_data.encode("utf-8"))
        Gtk.StyleContext.add_provider_for_display(
            Gdk.Display.get_default(),
            css_provider,
            Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        )

        # Layout
        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=12)
        vbox.set_margin_top(20)
        vbox.set_margin_bottom(20)
        vbox.set_margin_start(24)
        vbox.set_margin_end(24)

        # Header box with football icon
        hbox_header = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        icon_label = Gtk.Label(label="🏈")
        icon_label.set_css_classes(["title-label"])
        
        vbox_titles = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=2)
        self.lbl_title = Gtk.Label(label="ScoreTracker Multi-Sport Sync")
        self.lbl_title.set_xalign(0)
        self.lbl_title.set_css_classes(["title-label"])
        
        self.lbl_sub = Gtk.Label(label="NFL · CFB · NBA · NHL · MLS")
        self.lbl_sub.set_xalign(0)
        self.lbl_sub.set_css_classes(["sub-label"])
        
        vbox_titles.append(self.lbl_title)
        vbox_titles.append(self.lbl_sub)
        hbox_header.append(icon_label)
        hbox_header.append(vbox_titles)
        vbox.append(hbox_header)

        # Progress bar
        self.progress_bar = Gtk.ProgressBar()
        self.progress_bar.set_fraction(0.1)
        vbox.append(self.progress_bar)

        # Status text
        self.lbl_status = Gtk.Label(label="Initializing connection to watch...")
        self.lbl_status.set_xalign(0)
        self.lbl_status.set_css_classes(["status-label"])
        vbox.append(self.lbl_status)

        self.set_child(vbox)
        
        # Start worker thread
        self.worker_thread = threading.Thread(target=self._run_sync, daemon=True)
        self.worker_thread.start()

    def update_ui(self, fraction, text, subtext=None):
        def _apply():
            self.progress_bar.set_fraction(fraction)
            self.lbl_status.set_text(text)
            if subtext:
                self.lbl_sub.set_text(subtext)
            return False
        GLib.idle_add(_apply)

    def _run_sync(self):
        try:
            # 1. Fetch live multi-league ESPN data
            self.update_ui(0.15, "Fetching live scores across NFL, CFB, NBA, NHL, MLS...")
            games_to_push = fetch_all_multileague_games(max_games=32)
            total = len(games_to_push)

            live_count = sum(1 for g in games_to_push if g.get("state") == "in")
            final_count = sum(1 for g in games_to_push if g.get("state") == "post")
            sched_count = sum(1 for g in games_to_push if g.get("state") == "pre")

            self.update_ui(0.45, f"Found {total} games ({live_count} Live, {final_count} Final)...", "Connecting to watch over USB")

            # 2. Open serial port
            time.sleep(0.2)
            fd = None
            for attempt in range(5):
                try:
                    fd = os.open(self.port, os.O_RDWR | os.O_NOCTTY)
                    try:
                        tty.setraw(fd)
                    except Exception:
                        pass
                    break
                except OSError:
                    time.sleep(0.3)
                    
            if fd is None:
                self.update_ui(0.0, f"Error: Cannot open {self.port}")
                GLib.timeout_add(3000, self.get_application().quit)
                return

            # Clear buffer
            while True:
                r, _, _ = select.select([fd], [], [], 0.05)
                if r: os.read(fd, 4096)
                else: break

            # 3. Stream games to watch
            os.write(fd, f"SCORES_START {total}\n".encode("utf-8"))
            
            # Wait for READY
            t0 = time.time()
            ready_buf = b""
            while time.time() - t0 < 3.0:
                r, _, _ = select.select([fd], [], [], 0.05)
                if r:
                    ready_buf += os.read(fd, 512)
                    if b"READY" in ready_buf:
                        break

            # Send each game with ACK handshaking
            loaded = 0
            for i, g in enumerate(games_to_push):
                frac = 0.60 + 0.35 * (float(i + 1) / float(total))
                lg = g.get('league', 'NFL')
                self.update_ui(frac, f"Pushing [{lg}] {g['away']} @ {g['home']} ({i + 1}/{total})")
                
                line = json.dumps(g, separators=(',', ':')) + "\n"
                os.write(fd, line.encode("utf-8"))
                
                # Wait for ACK
                t_ack = time.time()
                ack_buf = b""
                while time.time() - t_ack < 2.0:
                    r, _, _ = select.select([fd], [], [], 0.02)
                    if r:
                        ack_buf += os.read(fd, 512)
                        if b"ACK" in ack_buf:
                            loaded += 1
                            break
                        elif b"NACK" in ack_buf:
                            print(f"[SYNC] Game {i + 1} NACKed: {ack_buf.decode('utf-8', errors='replace').strip()}")
                            break

            os.write(fd, b"SCORES_END\n")
            time.sleep(0.1)
            os.close(fd)

            # 4. Success feedback
            if loaded > 0:
                self.update_ui(1.0, f"✅ Done! {loaded} games across 5 leagues synced.", "Display live & updated")
                
                # Send desktop notification
                subprocess.run([
                    "notify-send",
                    "🏆 Multi-Sport ScoreTracker Synced",
                    f"Updated {loaded} games across NFL, CFB, NBA, NHL, MLS.\n{live_count} Live in action right now.",
                    "-a", "ScoreTracker",
                    "-t", "4000"
                ], check=False)
            else:
                self.update_ui(0.0, "⚠️ Sync failed: 0 games accepted by watch.", "Check USB connection")

            # Auto close window after 2.2 seconds
            GLib.timeout_add(2200, self.get_application().quit)

        except Exception as e:
            self.update_ui(0.0, f"Error: {e}")
            GLib.timeout_add(3500, self.get_application().quit)

def run_headless_sync(port):
    print(f"[SYNC_POPUP] Running headless fallback score sync on {port}...")
    push_script = os.path.join(PROJECT_DIR, "tools", "push_scores.py")
    try:
        subprocess.run([sys.executable, push_script, "--port", port], check=True)
    except Exception as e:
        print(f"[SYNC_POPUP] Headless sync error: {e}")

def main():
    port = "/dev/ttyACM0"
    if len(sys.argv) > 1 and sys.argv[1].startswith("/dev/"):
        port = sys.argv[1]
        
    gui_failed = False
    try:
        app = Gtk.Application(application_id="com.scoretracker.sync")
        def on_activate(app):
            nonlocal gui_failed
            try:
                win = SyncAppWindow(app, port=port)
                win.present()
            except Exception as e:
                print(f"[SYNC_POPUP] Failed to create GTK window: {e}")
                gui_failed = True
                app.quit()
            
        app.connect("activate", on_activate)
        app.run(None)
    except Exception as e:
        print(f"[SYNC_POPUP] GTK Application error: {e}")
        gui_failed = True

    if gui_failed:
        run_headless_sync(port)

if __name__ == "__main__":
    main()

