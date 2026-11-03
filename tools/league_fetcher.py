#!/usr/bin/env python3
"""
league_fetcher.py - Multi-league live sports data aggregator for ScoreTracker.
Fetches and standardizes live, completed, and scheduled games across:
  - NFL (Football)
  - CFB (NCAA College Football with AP Top 25 rankings)
  - NBA (Basketball)
  - NHL (Hockey)
  - MLS (Soccer)

Zero external dependencies (pure Python standard library).
"""

import os
import sys
import time
import json
import datetime
from zoneinfo import ZoneInfo
import urllib.request

LEAGUE_ENDPOINTS = {
    "NFL": "http://site.api.espn.com/apis/site/v2/sports/football/nfl/scoreboard",
    "CFB": "http://site.api.espn.com/apis/site/v2/sports/football/college-football/scoreboard",
    "CBB": "http://site.api.espn.com/apis/site/v2/sports/basketball/mens-college-basketball/scoreboard",
    "NBA": "http://site.api.espn.com/apis/site/v2/sports/basketball/nba/scoreboard",
    "NHL": "http://site.api.espn.com/apis/site/v2/sports/hockey/nhl/scoreboard",
    "MLS": "http://site.api.espn.com/apis/site/v2/sports/soccer/usa.1/scoreboard",
    "EPL": "http://site.api.espn.com/apis/site/v2/sports/soccer/eng.1/scoreboard",
    "LAL": "http://site.api.espn.com/apis/site/v2/sports/soccer/esp.1/scoreboard",
    "BUN": "http://site.api.espn.com/apis/site/v2/sports/soccer/ger.1/scoreboard",
    "UCL": "http://site.api.espn.com/apis/site/v2/sports/soccer/uefa.champions/scoreboard",
    "UEL": "http://site.api.espn.com/apis/site/v2/sports/soccer/uefa.europa/scoreboard",
}

def fetch_espn_json(url, timeout=8):
    req = urllib.request.Request(url, headers={"User-Agent": "curl/8.0.0"})
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        return json.loads(resp.read().decode("utf-8"))

def parse_iso_datetime(iso_str):
    """Convert ISO8601 UTC string to US Pacific Day, Date, and Time."""
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

def extract_competitor(c, league="NFL"):
    team = c.get("team", {})
    code = team.get("abbreviation", league)
    if league == "NFL" and code == "WSH":
        code = "WAS"
    name = team.get("shortDisplayName", code)
    rec = ""
    records = c.get("records", [])
    if records:
        rec = records[0].get("summary", "")
    score = int(c.get("score", 0))
    
    # Linescores (up to 4 quarters/periods)
    linescores = [int(q.get("value", 0)) for q in c.get("linescores", [])][:4]
    while len(linescores) < 4:
        linescores.append(0)
        
    # Team ranking (CFB AP Top 25)
    rank = 0
    curated_rank = c.get("curatedRank", {}).get("current")
    if curated_rank is not None and isinstance(curated_rank, int) and 1 <= curated_rank <= 25:
        rank = curated_rank
        
    return {
        "code": code[:7],
        "name": name[:23],
        "rec": rec[:11],
        "score": score,
        "q": linescores,
        "rank": rank
    }

def parse_games_for_league(league_name, espn_json):
    games = []
    events = espn_json.get("events", [])
    
    for ev in events:
        status_obj = ev.get("status", {})
        status_type = status_obj.get("type", {})
        state = status_type.get("state", "pre")  # 'pre', 'in', 'post'
        detail = status_type.get("shortDetail", "FINAL")
        period = status_obj.get("period", 0)
        clock = status_obj.get("displayClock", "")
        
        iso_date = ev.get("date", "")
        day_str, date_str, kickoff_time = parse_iso_datetime(iso_date)
        
        # Format status badge text per league conventions
        if state == "in":
            if "Halftime" in detail or "Half" in detail or detail == "HT":
                status_text = "HALFTIME"
            elif "End of" in detail:
                status_text = detail.upper()
            elif league_name == "NHL":
                # e.g. "P2 14:22" or "OT 2:10"
                if period <= 3:
                    status_text = f"P{period} {clock}" if clock else f"P{period}"
                else:
                    status_text = f"OT {clock}" if clock else "OT"
            elif league_name == "NBA":
                # e.g. "Q3 4:12"
                if period <= 4:
                    status_text = f"Q{period} {clock}" if clock else f"Q{period}"
                else:
                    status_text = f"OT{period-4 if period > 5 else ''} {clock}" if clock else "OT"
            elif league_name == "CBB":
                if "Halftime" in detail or "Half" in detail or detail == "HT":
                    status_text = "HALFTIME"
                elif period <= 2:
                    status_text = f"{period}H {clock}" if clock else f"{period}H"
                else:
                    status_text = f"OT{period-2 if period > 3 else ''} {clock}" if clock else "OT"
            elif league_name == "MLS":
                # Soccer: 1H or 2H with minute clock e.g. "64'"
                if clock:
                    clean_clock = clock.rstrip("'\"").strip()
                    status_text = f"{clean_clock}'" if clean_clock else detail
                else:
                    status_text = detail
            else:
                # Football (NFL, CFB): e.g. "2nd 4:48"
                if " - " in detail:
                    parts = detail.split(" - ")
                    if len(parts) == 2:
                        status_text = f"{parts[1]} {parts[0]}"
                    else:
                        status_text = detail
                else:
                    status_text = detail
        elif state == "post":
            if "OT" in detail:
                status_text = "FINAL/OT"
            elif "SO" in detail:
                status_text = "FINAL/SO"
            elif "PK" in detail or "Pen" in detail:
                status_text = "FINAL/PK"
            else:
                status_text = "FINAL"
        else:
            # Scheduled pre-game: show local game time
            status_text = kickoff_time
            
        comp = ev.get("competitions", [{}])[0]
        venue = comp.get("venue", {}).get("fullName", f"{league_name} Match")
        
        competitors = comp.get("competitors", [])
        if len(competitors) < 2:
            continue
            
        c0 = competitors[0]
        c1 = competitors[1]
        away_raw = c1 if c1.get("homeAway") == "away" else c0
        home_raw = c0 if c0.get("homeAway") == "home" else c1
        
        away_info = extract_competitor(away_raw, league=league_name)
        home_info = extract_competitor(home_raw, league=league_name)
        
        # Leader extraction across sports (Football, Basketball, Hockey, Soccer)
        leader_str = ""
        # 1. Competition-level leaders (NFL, CFB)
        leaders = comp.get("leaders", [])
        if leaders:
            top_l = leaders[0].get("leaders", [])
            if top_l:
                ath = top_l[0].get("athlete", {}).get("shortName", "")
                val = top_l[0].get("displayValue", "")
                if ath and val:
                    leader_str = f"{ath}: {val}"

        # 2. Competitor-level leaders (NBA, CBB, NHL)
        if not leader_str:
            best_ath = ""
            best_stat = ""
            best_pts = -1.0
            for c_raw in (away_raw, home_raw):
                c_leaders = c_raw.get("leaders", [])
                for cat in c_leaders:
                    cat_name = cat.get("name")
                    subs = cat.get("leaders", [])
                    if not subs:
                        continue
                    sub = subs[0]
                    ath = sub.get("athlete", {}).get("shortName") or sub.get("athlete", {}).get("displayName", "")
                    val = sub.get("displayValue", "")
                    val_num = float(sub.get("value", 0))

                    if cat_name == "rating" and val_num > best_pts:
                        best_pts = val_num
                        best_ath = ath
                        best_stat = val
                    elif cat_name == "points" and val_num > best_pts and not best_stat:
                        best_pts = val_num
                        best_ath = ath
                        best_stat = f"{val} PTS"
                    elif cat_name == "goals" and val_num > best_pts and not best_stat:
                        best_pts = val_num
                        best_ath = ath
                        best_stat = f"{val} G"
            if best_ath and best_stat:
                leader_str = f"{best_ath}: {best_stat}"

        # 3. Details (e.g. MLS goal scorers)
        if not leader_str and league_name == "MLS":
            details = comp.get("details", [])
            if details:
                scorers = []
                for d in details[:2]:
                    ath_name = d.get("athletesInvolved", [{}])[0].get("shortName", "")
                    clock_str = d.get("clock", {}).get("displayValue", "")
                    if ath_name:
                        scorers.append(f"{ath_name} {clock_str}")
                if scorers:
                    leader_str = ", ".join(scorers)

        # 4. Scheduled games without stats: TV Broadcast
        if not leader_str:
            bcasts = comp.get("broadcasts", [])
            if bcasts:
                names = bcasts[0].get("names", [])
                if names:
                    leader_str = f"TV: {names[0]}"

        # 5. Final fallback to venue
        if not leader_str:
            leader_str = venue

        games.append({
            "league": league_name,
            "state": state,
            "day": day_str,
            "date": date_str,
            "status": status_text[:23],
            "away": away_info["code"],
            "away_name": away_info["name"],
            "away_rec": away_info["rec"],
            "away_score": away_info["score"],
            "away_q": away_info["q"],
            "away_rank": away_info["rank"],
            "home": home_info["code"],
            "home_name": home_info["name"],
            "home_rec": home_info["rec"],
            "home_score": home_info["score"],
            "home_q": home_info["q"],
            "home_rank": home_info["rank"],
            "leader": leader_str[:63],
            "stadium": venue[:47]
        })
        
    return games

def fetch_all_multileague_games(max_games=32):
    """
    Fetches all 5 leagues (NFL, CFB, NBA, NHL, MLS) from ESPN endpoints.
    Priority order:
      1. ALL LIVE games currently in progress across all leagues (Page 1 guaranteed).
      2. Recently completed FINALS from today / this week across all leagues.
      3. Top upcoming scheduled matchups (ranked CFB teams, prime-time games).
    Capped at max_games (32 for watch hardware storage).
    """
    all_games = []
    
    for league_name, endpoint in LEAGUE_ENDPOINTS.items():
        try:
            print(f"[LEAGUES] Fetching {league_name}...")
            data = fetch_espn_json(endpoint, timeout=6)
            parsed = parse_games_for_league(league_name, data)
            all_games.extend(parsed)
            print(f"[LEAGUES]   {league_name}: {len(parsed)} games retrieved.")
        except Exception as e:
            print(f"[LEAGUES]   {league_name} error: {e}")
            
    live_games = [g for g in all_games if g["state"] == "in"]
    final_games = [g for g in all_games if g["state"] == "post"]
    pre_games = [g for g in all_games if g["state"] == "pre"]
    
    # Sub-sort CFB / NFL pre-games so ranked teams appear first
    pre_games.sort(key=lambda g: (
        0 if (g.get("away_rank", 0) > 0 or g.get("home_rank", 0) > 0) else 1,
        0 if g.get("league") == "NFL" else 1
    ))
    
    combined = []
    combined.extend(live_games)
    combined.extend(final_games)
    combined.extend(pre_games)
    
    print(f"[LEAGUES] Total aggregated: {len(all_games)} games ({len(live_games)} live, {len(final_games)} final, {len(pre_games)} scheduled).")
    return combined[:max_games]

if __name__ == "__main__":
    games = fetch_all_multileague_games(32)
    print(f"\nFetched {len(games)} multi-league games:")
    for i, g in enumerate(games[:10]):
        rank_a = f"#{g['away_rank']} " if g.get('away_rank', 0) > 0 else ""
        rank_h = f"#{g['home_rank']} " if g.get('home_rank', 0) > 0 else ""
        print(f" {i+1:2d}. [{g['league']:3s}] {rank_a}{g['away']:3s} {g['away_score']:2d} @ {rank_h}{g['home']:3s} {g['home_score']:2d} | {g['status']:14s} | {g['day']} {g['date']}")
