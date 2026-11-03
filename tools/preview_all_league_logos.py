#!/usr/bin/env python3
"""
preview_all_league_logos.py - Create a high-res showcase montage of all sourced logos.
Displays the 5 league shields, all 32 NFL, 30 NBA, 32 NHL, 30 MLS teams, and CFB Top 25.
"""

import os
import glob
from PIL import Image, ImageDraw, ImageFont

PROJECT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CACHE_DIR = os.path.join(PROJECT_DIR, "tools", "logos_cache")
OUTPUT_PATH = os.path.join(PROJECT_DIR, "tools", "all_league_logos_preview.png")

CFB_TOP_PROGRAMS = [
    "TEX", "UGA", "OSU", "ALA", "ND", "MIA", "ORE", "PSU",
    "IU", "TENN", "MISS", "LSU", "USC", "CLEM", "MICH", "OU",
    "UTAH", "ISU", "BYU", "KSU", "ILL", "BOIS", "SMU", "PITT",
    "FLA", "FSU", "A&M", "COLO", "NEB", "WASH", "WIS", "IOWA"
]

def make_montage():
    # Overall dimensions
    cell_w, cell_h = 72, 72
    cols = 8
    pad_x = 24
    header_h = 50
    bg_color = (22, 32, 27)  # Deep Forest Moss
    text_color = (223, 213, 198) # Warm Sand
    sub_color = (132, 176, 150)  # Calm Sage

    leagues = [
        ("NFL", "nfl", sorted([os.path.splitext(os.path.basename(p))[0] for p in glob.glob(f"{CACHE_DIR}/nfl/*.png")])),
        ("NBA", "nba", sorted([os.path.splitext(os.path.basename(p))[0] for p in glob.glob(f"{CACHE_DIR}/nba/*.png")])),
        ("NHL", "nhl", sorted([os.path.splitext(os.path.basename(p))[0] for p in glob.glob(f"{CACHE_DIR}/nhl/*.png")])),
        ("MLS", "mls", sorted([os.path.splitext(os.path.basename(p))[0] for p in glob.glob(f"{CACHE_DIR}/mls/*.png")])),
        ("CFB (Top Programs & FBS)", "cfb", CFB_TOP_PROGRAMS),
    ]

    total_rows = 0
    for title, dir_name, teams in leagues:
        r = (len(teams) + cols - 1) // cols
        total_rows += r

    width = pad_x * 2 + cols * cell_w
    height = 90 + len(leagues) * header_h + total_rows * cell_h + 40

    img = Image.new("RGBA", (width, height), bg_color)
    draw = ImageDraw.Draw(img)

    # Title Banner
    draw.text((width // 2, 32), "OFFICIAL SOURCED TEAM LOGOS ACROSS ALL 5 LEAGUES", fill=text_color, anchor="mm")
    draw.text((width // 2, 54), "588 Professional & NCAA Teams Sourced via ESPN CDN", fill=sub_color, anchor="mm")

    y = 80
    for title, dir_name, teams in leagues:
        # League Header
        sh_path = os.path.join(CACHE_DIR, f"{dir_name.split()[0]}_shield.png")
        if os.path.exists(sh_path):
            try:
                sh_im = Image.open(sh_path).convert("RGBA")
                sh_im.thumbnail((28, 28), Image.Resampling.LANCZOS)
                img.paste(sh_im, (pad_x, y + 10), sh_im)
            except Exception:
                pass
        
        draw.text((pad_x + 38, y + 24), f"{title} ({len(teams)} Teams)", fill=text_color, anchor="lm")
        draw.line([(pad_x, y + 42), (width - pad_x, y + 42)], fill=(45, 60, 52), width=1)
        y += header_h

        # Grid of logos
        for i, team in enumerate(teams):
            col = i % cols
            row = i // cols
            cx = pad_x + col * cell_w + cell_w // 2
            cy = y + row * cell_h + (cell_h - 16) // 2

            # Badge circular backing
            draw.ellipse([cx - 24, cy - 24, cx + 24, cy + 24], fill=(30, 44, 38), outline=(50, 72, 62))

            # Team logo
            logo_path = os.path.join(CACHE_DIR, dir_name.split()[0], f"{team}.png")
            if os.path.exists(logo_path):
                try:
                    logo_im = Image.open(logo_path).convert("RGBA")
                    # Crop transparent borders
                    bbox = logo_im.getbbox()
                    if bbox:
                        logo_im = logo_im.crop(bbox)
                    logo_im.thumbnail((38, 38), Image.Resampling.LANCZOS)
                    lx = cx - logo_im.width // 2
                    ly = cy - logo_im.height // 2
                    img.paste(logo_im, (lx, ly), logo_im)
                except Exception:
                    draw.text((cx, cy), team[:3], fill=text_color, anchor="mm")
            else:
                draw.text((cx, cy), team[:3], fill=text_color, anchor="mm")

            # Team abbreviation label below
            draw.text((cx, cy + 28), team, fill=sub_color, anchor="mm")

        rows = (len(teams) + cols - 1) // cols
        y += rows * cell_h + 12

    img.save(OUTPUT_PATH)
    print(f"Montage saved to: {OUTPUT_PATH} ({os.path.getsize(OUTPUT_PATH)} bytes)")

if __name__ == "__main__":
    make_montage()
