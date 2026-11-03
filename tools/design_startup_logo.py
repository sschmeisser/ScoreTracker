#!/usr/bin/env python3
"""
design_startup_logo.py - Refined, ultra-crisp ScoreTracker logo and boot screen.
"""

import os
import math
from PIL import Image, ImageDraw, ImageFont

W, H = 320, 240
BG_COLOR = (13, 22, 17)         # Deep Forest Moss (#0D1611)
CARD_BG = (22, 34, 27)          # Card background
HAIR_COLOR = (36, 52, 42)       # Subtle border
AMBER = (212, 163, 89)          # #D4A359 Warm Amber / Gold
AMBER_LIGHT = (245, 195, 120)   # Highlight
LIVE_GREEN = (46, 213, 115)     # #2ED573 Electric neon green
SAND = (230, 226, 215)          # Warm sand typography
MUTED = (130, 150, 140)         # Secondary typography
DARK_BG = (16, 26, 21)

def create_emblem(size=84):
    """Generates a high-precision, antialiased ScoreTracker emblem."""
    scale = 4
    S = size * scale
    im = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)

    cx, cy = S // 2, S // 2
    
    # 1. Outer chamfered shield / faceted diamond badge
    # 8-sided symmetrical octagon / athletic badge
    r_out = S * 0.46
    chamfer = S * 0.14
    pts_out = [
        (cx - r_out + chamfer, cy - r_out),
        (cx + r_out - chamfer, cy - r_out),
        (cx + r_out, cy - r_out + chamfer),
        (cx + r_out, cy + r_out - chamfer),
        (cx + r_out - chamfer, cy + r_out),
        (cx - r_out + chamfer, cy + r_out),
        (cx - r_out, cy + r_out - chamfer),
        (cx - r_out, cy - r_out + chamfer),
    ]

    r_in = S * 0.41
    pts_in = [
        (cx - r_in + chamfer*0.9, cy - r_in),
        (cx + r_in - chamfer*0.9, cy - r_in),
        (cx + r_in, cy - r_in + chamfer*0.9),
        (cx + r_in, cy + r_in - chamfer*0.9),
        (cx + r_in - chamfer*0.9, cy + r_in),
        (cx - r_in + chamfer*0.9, cy + r_in),
        (cx - r_in, cy + r_in - chamfer*0.9),
        (cx - r_in, cy - r_in + chamfer*0.9),
    ]

    # Fill badge background
    d.polygon(pts_out, fill=DARK_BG)
    d.polygon(pts_out, outline=AMBER, width=int(2.5 * scale))
    d.polygon(pts_in, outline=HAIR_COLOR, width=int(1.2 * scale))

    # Corner stadium light pips (top left, top right, bottom left, bottom right)
    pip_r = int(1.8 * scale)
    for px, py in [(cx - r_out + chamfer*0.4, cy - r_out + chamfer*0.4),
                   (cx + r_out - chamfer*0.4, cy - r_out + chamfer*0.4),
                   (cx - r_out + chamfer*0.4, cy + r_out - chamfer*0.4),
                   (cx + r_out - chamfer*0.4, cy + r_out - chamfer*0.4)]:
        d.ellipse([px - pip_r, py - pip_r, px + pip_r, py + pip_r], fill=AMBER_LIGHT)

    # 2. Geometric ST Monogram + Stadium Bowl Curves
    # Top Crossbar of T: bold athletic trapezoid
    tb_w = S * 0.52
    tb_h = S * 0.10
    d.polygon([
        (cx - tb_w/2, cy - S*0.22),
        (cx + tb_w/2, cy - S*0.22),
        (cx + tb_w/2 - S*0.04, cy - S*0.22 + tb_h),
        (cx - tb_w/2 + S*0.04, cy - S*0.22 + tb_h),
    ], fill=AMBER)

    # Vertical Stem of T
    ts_w = S * 0.12
    ts_top = cy - S*0.22 + tb_h
    ts_bot = cy + S*0.25
    d.rectangle([cx - ts_w/2, ts_top, cx + ts_w/2, ts_bot], fill=(240, 240, 240, 255))

    # Bold stylized "S" wing cutting through the T
    # Left wing (top loop of S)
    sw_thick = int(4.5 * scale)
    # Upper left wing of S
    d.arc([cx - S*0.30, cy - S*0.25, cx + S*0.02, cy + S*0.02], start=135, end=340, fill=LIVE_GREEN, width=sw_thick)
    # Lower right wing of S
    d.arc([cx - S*0.02, cy - S*0.02, cx + S*0.30, cy + S*0.25], start=-45, end=160, fill=LIVE_GREEN, width=sw_thick)

    # 3. Live Digital Waveform cutting across the center
    wave_y = cy + S * 0.02
    wave_pts = [
        (cx - S * 0.36, wave_y),
        (cx - S * 0.18, wave_y),
        (cx - S * 0.10, wave_y - S * 0.12),
        (cx, wave_y + S * 0.14),
        (cx + S * 0.10, wave_y - S * 0.10),
        (cx + S * 0.18, wave_y),
        (cx + S * 0.36, wave_y),
    ]
    # Wave glow shadow
    d.line(wave_pts, fill=(20, 100, 50, 180), width=int(4 * scale))
    # Crisp white/green laser line
    d.line(wave_pts, fill=(255, 255, 255, 255), width=int(1.8 * scale))

    # Active Live Beacon (glowing dot)
    beacon_x = cx
    beacon_y = cy - r_out + int(3 * scale)
    b_r = int(2.5 * scale)
    d.ellipse([beacon_x - b_r - int(2*scale), beacon_y - b_r - int(2*scale),
               beacon_x + b_r + int(2*scale), beacon_y + b_r + int(2*scale)], fill=(46, 213, 115, 80))
    d.ellipse([beacon_x - b_r, beacon_y - b_r, beacon_x + b_r, beacon_y + b_r], fill=LIVE_GREEN)

    # Downscale with Lanczos
    return im.resize((size, size), Image.Resampling.LANCZOS)

def generate_boot_screen(output_path, status="SYNCING LIVE ESPN FEEDS...", progress=0.75):
    im = Image.new("RGB", (W, H), BG_COLOR)
    draw = ImageDraw.Draw(im)

    # Background subtle concentric stadium arcs
    for i in range(5):
        arc_r = 160 + i * 22
        arc_box = [W//2 - arc_r, -arc_r + 45, W//2 + arc_r, arc_r + 45]
        draw.arc(arc_box, start=35, end=145, fill=(18 + i*2, 28 + i*2, 22 + i*2), width=1)

    # Subtle horizontal hash grid marks on sides
    for y in range(40, 200, 20):
        draw.line([10, y, 22, y], fill=(22, 34, 27))
        draw.line([W - 22, y, W - 10, y], fill=(22, 34, 27))

    # 1. Place the Emblem
    emblem_size = 80
    emblem = create_emblem(emblem_size)
    ex = (W - emblem_size) // 2
    ey = 18
    im.paste(emblem, (ex, ey), emblem)

    # 2. Typography
    font_bold = ImageFont.truetype("/usr/share/fonts/liberation/LiberationSans-Bold.ttf", 20)
    font_med = ImageFont.truetype("/usr/share/fonts/liberation/LiberationSans-Bold.ttf", 10)
    font_mono = ImageFont.truetype("/usr/share/fonts/TTF/JetBrainsMonoNerdFont-Bold.ttf", 9)
    font_tiny = ImageFont.truetype("/usr/share/fonts/liberation/LiberationSans-Bold.ttf", 8)

    # Main Brand Title
    s_text = "SCORE"
    t_text = "TRACKER"
    bbox_s = draw.textbbox((0, 0), s_text, font=font_bold)
    bbox_t = draw.textbbox((0, 0), t_text, font=font_bold)
    ws = bbox_s[2] - bbox_s[0]
    wt = bbox_t[2] - bbox_t[0]
    gap = 4
    total_w = ws + gap + wt
    tx = (W - total_w) // 2
    ty = 106

    draw.text((tx, ty), s_text, fill=SAND, font=font_bold)
    draw.text((tx + ws + gap, ty), t_text, fill=AMBER, font=font_bold)

    # Subtitle with horizontal rules
    sub = "LIVE MULTI-SPORT DESK"
    bbox_sub = draw.textbbox((0, 0), sub, font=font_tiny)
    w_sub = bbox_sub[2] - bbox_sub[0]
    sy = 132
    draw.text(((W - w_sub) // 2, sy), sub, fill=MUTED, font=font_tiny)
    # Side rules
    rule_y = sy + 5
    draw.line([((W - w_sub) // 2) - 28, rule_y, ((W - w_sub) // 2) - 8, rule_y], fill=HAIR_COLOR)
    draw.line([((W + w_sub) // 2) + 8, rule_y, ((W + w_sub) // 2) + 28, rule_y], fill=HAIR_COLOR)

    # 3. Supported Leagues Pill
    leagues = ["NFL", "CFB", "NBA", "CBB", "NHL", "MLS", "EPL", "UCL", "LAL", "BUN"]
    badge_str = " | ".join(leagues)
    bbox_b = draw.textbbox((0, 0), badge_str, font=font_mono)
    wb = bbox_b[2] - bbox_b[0]
    pw = wb + 16
    ph = 16
    px = (W - pw) // 2
    py = 148
    draw.rounded_rectangle([px, py, px + pw, py + ph], radius=8, fill=CARD_BG, outline=HAIR_COLOR)
    draw.text(((W - wb) // 2, py + 2), badge_str, fill=AMBER, font=font_mono)

    # 4. Progress & Status
    bbox_stat = draw.textbbox((0, 0), status, font=font_med)
    w_stat = bbox_stat[2] - bbox_stat[0]
    st_x = (W - w_stat) // 2 + 5
    st_y = 175
    # Glowing pulsing LED
    draw.ellipse([st_x - 14, st_y + 2, st_x - 6, st_y + 10], fill=(46, 213, 115, 80))
    draw.ellipse([st_x - 12, st_y + 4, st_x - 8, st_y + 8], fill=LIVE_GREEN)
    draw.text((st_x, st_y), status, fill=SAND, font=font_med)

    # Progress bar
    bar_w = 210
    bar_h = 6
    bx = (W - bar_w) // 2
    by = 194
    draw.rounded_rectangle([bx, by, bx + bar_w, by + bar_h], radius=3, fill=(18, 28, 22), outline=HAIR_COLOR)
    fill_w = int(bar_w * max(0.0, min(1.0, progress)))
    if fill_w > 0:
        draw.rounded_rectangle([bx, by, bx + fill_w, by + bar_h], radius=3, fill=LIVE_GREEN)
        # Leading edge white pip
        draw.ellipse([bx + fill_w - 4, by, bx + fill_w + 2, by + bar_h], fill=(255, 255, 255))

    # 5. Bottom Hardware Strip
    foot = "ESP32-2432S028R  •  591 EMBEDDED LOGOS  •  V2.4"
    bbox_f = draw.textbbox((0, 0), foot, font=font_tiny)
    wf = bbox_f[2] - bbox_f[0]
    draw.text(((W - wf) // 2, 218), foot, fill=(80, 105, 95), font=font_tiny)

    im.save(output_path)
    print(f"Rendered boot screen to {output_path}")

if __name__ == "__main__":
    out_dir = "/home/stefan/.gemini/antigravity-cli/brain/9365e6f9-d886-4966-9167-c85e5a81615d"
    generate_boot_screen(os.path.join(out_dir, "scoretracker_boot_screen.png"))
