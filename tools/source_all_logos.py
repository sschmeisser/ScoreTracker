#!/usr/bin/env python3
"""
source_all_logos.py - Source and cache official logos for all teams across all leagues:
  - NFL (32 teams)
  - NBA (30 teams)
  - NHL (32 teams)
  - MLS (30 teams)
  - CFB (NCAA Football Bowl Subdivision & Top programs)

Downloads official high-resolution transparent PNG logos directly from ESPN CDN
and organizes them into tools/logos_cache/<league>/ with a complete teams_manifest.json.
"""

import os
import sys
import json
import time
import urllib.request
from concurrent.futures import ThreadPoolExecutor, as_completed

PROJECT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CACHE_DIR = os.path.join(PROJECT_DIR, "tools", "logos_cache")

LEAGUE_TEAMS_ENDPOINTS = {
    "NFL": "http://site.api.espn.com/apis/site/v2/sports/football/nfl/teams",
    "NBA": "http://site.api.espn.com/apis/site/v2/sports/basketball/nba/teams",
    "NHL": "http://site.api.espn.com/apis/site/v2/sports/hockey/nhl/teams",
    "MLS": "http://site.api.espn.com/apis/site/v2/sports/soccer/usa.1/teams",
    "CFB": "http://site.api.espn.com/apis/site/v2/sports/football/college-football/teams?limit=500",
    "CBB": "http://site.api.espn.com/apis/site/v2/sports/basketball/mens-college-basketball/teams?limit=500",
    "EPL": "http://site.api.espn.com/apis/site/v2/sports/soccer/eng.1/teams",
    "LAL": "http://site.api.espn.com/apis/site/v2/sports/soccer/esp.1/teams",
    "BUN": "http://site.api.espn.com/apis/site/v2/sports/soccer/ger.1/teams",
    "UCL": "http://site.api.espn.com/apis/site/v2/sports/soccer/uefa.champions/teams",
    "UEL": "http://site.api.espn.com/apis/site/v2/sports/soccer/uefa.europa/teams",
}

def fetch_json(url):
    req = urllib.request.Request(url, headers={"User-Agent": "curl/8.0.0"})
    with urllib.request.urlopen(req, timeout=15) as resp:
        return json.loads(resp.read().decode("utf-8"))

def download_file(url, target_path):
    if os.path.exists(target_path) and os.path.getsize(target_path) > 500:
        return True, target_path  # Already cached
    try:
        req = urllib.request.Request(url, headers={"User-Agent": "curl/8.0.0"})
        with urllib.request.urlopen(req, timeout=15) as resp:
            data = resp.read()
            if len(data) > 200:
                with open(target_path, "wb") as f:
                    f.write(data)
                return True, target_path
    except Exception as e:
        return False, f"{url}: {e}"
    return False, "Empty response"

def main():
    os.makedirs(CACHE_DIR, exist_ok=True)
    manifest = {}
    download_tasks = []

    print("=" * 60)
    print("SOURCING OFFICIAL TEAM LOGOS ACROSS ALL LEAGUES")
    print("=" * 60)

    for league, endpoint in LEAGUE_TEAMS_ENDPOINTS.items():
        league_dir = os.path.join(CACHE_DIR, league.lower())
        os.makedirs(league_dir, exist_ok=True)
        print(f"\n[QUERY] Fetching team registry for {league}...")

        try:
            data = fetch_json(endpoint)
            sports = data.get("sports", [{}])[0]
            leagues = sports.get("leagues", [{}])[0]
            teams_raw = leagues.get("teams", [])
            print(f"[{league}] Found {len(teams_raw)} teams in ESPN registry.")

            manifest[league] = {}
            for entry in teams_raw:
                t = entry.get("team", {})
                abbr = t.get("abbreviation")
                name = t.get("displayName") or t.get("name")
                team_id = t.get("id")
                color = t.get("color", "")
                alt_color = t.get("alternateColor", "")
                logos = t.get("logos", [])
                
                if not abbr or not logos:
                    continue

                if league == "NFL" and abbr == "WSH":
                    abbr = "WAS"

                logo_url = logos[0].get("href")
                target_filename = f"{abbr}.png"
                target_path = os.path.join(league_dir, target_filename)

                manifest[league][abbr] = {
                    "abbreviation": abbr,
                    "name": name,
                    "id": team_id,
                    "color": color,
                    "alternateColor": alt_color,
                    "logo_url": logo_url,
                    "cached_file": os.path.relpath(target_path, PROJECT_DIR)
                }

                download_tasks.append((logo_url, target_path, league, abbr))

        except Exception as e:
            print(f"[{league}] Error fetching directory: {e}")

    # Also fetch league insignia shields
    league_shields = {
        "NFL": "https://a.espncdn.com/i/teamlogos/leagues/500/nfl.png",
        "NBA": "https://a.espncdn.com/i/teamlogos/leagues/500/nba.png",
        "NHL": "https://a.espncdn.com/i/teamlogos/leagues/500/nhl.png",
        "MLS": "https://a.espncdn.com/i/teamlogos/leagues/500/mls.png",
        "CFB": "https://a.espncdn.com/i/teamlogos/ncaa/500/ncaa.png",
    }
    for lg, sh_url in league_shields.items():
        sh_path = os.path.join(CACHE_DIR, f"{lg.lower()}_shield.png")
        download_tasks.append((sh_url, sh_path, "SHIELD", lg))

    print(f"\n[DOWNLOAD] Sourcing {len(download_tasks)} high-resolution logos in parallel...")
    success_count = 0
    fail_count = 0

    with ThreadPoolExecutor(max_workers=16) as executor:
        futures = {executor.submit(download_file, url, path): (lg, abbr) for url, path, lg, abbr in download_tasks}
        for fut in as_completed(futures):
            lg, abbr = futures[fut]
            ok, res = fut.result()
            if ok:
                success_count += 1
            else:
                fail_count += 1
                print(f"  [FAIL] {lg} {abbr}: {res}")

    manifest_path = os.path.join(CACHE_DIR, "teams_manifest.json")
    with open(manifest_path, "w") as f:
        json.dump(manifest, f, indent=2)

    print("\n" + "=" * 60)
    print(f"DOWNLOAD COMPLETE: {success_count} logos cached successfully ({fail_count} failed).")
    print(f"Manifest written to: {os.path.relpath(manifest_path, PROJECT_DIR)}")
    for lg in manifest:
        print(f"  - {lg:4s}: {len(manifest[lg])} teams")
    print("=" * 60)

if __name__ == "__main__":
    main()
