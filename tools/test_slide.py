import os, sys, math
from PIL import Image, ImageDraw, ImageFont

W, H = 240, 240
cx, cy = 120, 120

def get_font(name, size):
    try:
        if name == "sans_bold":
            return ImageFont.truetype("/usr/share/fonts/truetype/freefont/FreeSansBold.ttf", size)
        elif name == "sans":
            return ImageFont.truetype("/usr/share/fonts/truetype/freefont/FreeSans.ttf", size)
        elif name == "dejavu_bold":
            return ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", size)
        elif name == "dejavu":
            return ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", size)
    except Exception as e:
        pass
    return ImageFont.load_default()

def load_logo_32x26(code):
    path = f"/home/stefan/Projects/ScoreTracker/tools/logos_cache/{code}.png"
    if os.path.exists(path):
        im = Image.open(path).convert("RGBA")
        bbox = im.getbbox()
        if bbox:
            im = im.crop(bbox)
        w, h = im.size
        scale = min(32 / w, 26 / h)
        nw, nh = max(1, int(w * scale)), max(1, int(h * scale))
        im_resized = im.resize((nw, nh), Image.Resampling.LANCZOS)
        canvas = Image.new("RGBA", (32, 26), (0, 0, 0, 0))
        ox = (32 - nw) // 2
        oy = (26 - nh) // 2
        canvas.paste(im_resized, (ox, oy), im_resized)
        return canvas
    return None

GAMES = [
    {
        "day": "THU", "date": "SEP 17", "status": "FINAL",
        "away_code": "BAL", "away_score": 20, "away_rec": "0-1",
        "home_code": "KC",  "home_score": 27, "home_rec": "1-0",
    },
    {
        "day": "FRI", "date": "SEP 18", "status": "FINAL",
        "away_code": "GB",  "away_score": 29, "away_rec": "0-1",
        "home_code": "PHI", "home_score": 34, "home_rec": "1-0",
    },
    {
        "day": "SUN", "date": "SEP 20", "status": "FINAL/OT",
        "away_code": "LAR", "away_score": 20, "away_rec": "0-1",
        "home_code": "DET", "home_score": 26, "home_rec": "1-0",
    },
    {
        "day": "MON", "date": "SEP 21", "status": "FINAL",
        "away_code": "NYJ", "away_score": 19, "away_rec": "0-1",
        "home_code": "SF",  "home_score": 32, "home_rec": "1-0",
    }
]

def render_frame_slide(offset_y, pair_idx=0, total_pairs=8):
    # Obsidian slate background
    img = Image.new("RGBA", (W, H), (10, 13, 18, 255))
    draw = ImageDraw.Draw(img)

    f_status = get_font("dejavu_bold", 8)
    f_team = get_font("sans_bold", 12)
    f_score = get_font("sans_bold", 18)
    f_sub = get_font("dejavu", 8)

    def draw_row(g, cy_row):
        if cy_row < -50 or cy_row > 290:
            return
        status = g["status"]
        is_ot = "OT" in status
        is_live = "Q" in status or "LIVE" in status

        if is_live:
            bg_p = (36, 20, 24); bd_p = (80, 40, 48); tx_p = (235, 125, 125)
        elif is_ot:
            bg_p = (32, 26, 18); bd_p = (75, 58, 36); tx_p = (220, 180, 110)
        else:
            bg_p = (17, 23, 32); bd_p = (34, 44, 60); tx_p = (135, 150, 172)

        # Centered status pill
        bb = f_status.getbbox(status)
        pw = (bb[2] - bb[0]) + 12
        py = cy_row - 25
        draw.rounded_rectangle([cx - pw//2, py, cx + pw//2, py + 12], radius=4, fill=bg_p, outline=bd_p)
        draw.text((cx - (bb[2] - bb[0])//2, py + 1), status, font=f_status, fill=tx_p)

        away_win = g["away_score"] > g["home_score"]
        home_win = g["home_score"] > g["away_score"]

        c_score_win = (245, 247, 250)
        c_score_lose = (112, 125, 142)
        c_team_win = (225, 230, 238)
        c_team_lose = (130, 142, 158)

        # Away side
        logo_a = load_logo_32x26(g["away_code"])
        if logo_a:
            img.paste(logo_a, (38 - 16, cy_row - 13 + 3), logo_a)
        draw.text((60, cy_row - 5 + 3), g["away_code"], font=f_team, fill=c_team_win if away_win else c_team_lose)

        sc_a = str(g["away_score"])
        bb_a = f_score.getbbox(sc_a)
        draw.text((105 - (bb_a[2] - bb_a[0]), cy_row - 9 + 2), sc_a, font=f_score, fill=c_score_win if away_win else c_score_lose)

        # Center separator: serene dot
        draw.ellipse([cx - 1, cy_row + 2, cx + 1, cy_row + 4], fill=(55, 66, 82))

        # Home side
        sc_h = str(g["home_score"])
        draw.text((135, cy_row - 9 + 2), sc_h, font=f_score, fill=c_score_win if home_win else c_score_lose)

        bb_hcode = f_team.getbbox(g["home_code"])
        draw.text((180 - (bb_hcode[2] - bb_hcode[0]), cy_row - 5 + 3), g["home_code"], font=f_team, fill=c_team_win if home_win else c_team_lose)

        logo_h = load_logo_32x26(g["home_code"])
        if logo_h:
            img.paste(logo_h, (202 - 16, cy_row - 13 + 3), logo_h)

        # Sub line
        sub_s = f"{g['away_rec']}  ·  {g['day']} {g['date']}  ·  {g['home_rec']}"
        bb_s = f_sub.getbbox(sub_s)
        draw.text((cx - (bb_s[2] - bb_s[0])//2, cy_row + 18), sub_s, font=f_sub, fill=(72, 84, 100))

    # Draw current pair scrolling up
    draw_row(GAMES[0], 62 - offset_y)
    draw_row(GAMES[1], 168 - offset_y)

    # Hairline divider scrolling up
    draw.line([(45, 120 - offset_y), (195, 120 - offset_y)], fill=(24, 30, 42), width=1)

    # Draw next pair scrolling in
    draw_row(GAMES[2], 62 + 240 - offset_y)
    draw_row(GAMES[3], 168 + 240 - offset_y)

    # Hairline divider scrolling in
    draw.line([(45, 120 + 240 - offset_y), (195, 120 + 240 - offset_y)], fill=(24, 30, 42), width=1)

    # Fixed elements: Outer boundary & Dot pager
    # Mask out outside the circle with black
    mask = Image.new("L", (W, H), 0)
    dmask = ImageDraw.Draw(mask)
    dmask.ellipse([0, 0, 239, 239], fill=255)
    
    # Create final circular image
    final_img = Image.new("RGBA", (W, H), (0, 0, 0, 255))
    final_img.paste(img, (0, 0), mask)
    fdraw = ImageDraw.Draw(final_img)

    fdraw.ellipse([1, 1, 238, 238], outline=(20, 26, 36), width=1)

    # Dot pager at bottom
    start_dot_x = cx - (total_pairs * 8) // 2 + 4
    for i in range(total_pairs):
        dx = start_dot_x + i * 8
        if i == pair_idx:
            fdraw.ellipse([dx - 2, 227 - 2, dx + 2, 227 + 2], fill=(185, 195, 210))
        else:
            fdraw.ellipse([dx - 1, 227 - 1, dx + 1, 227 + 1], fill=(36, 44, 58))

    return final_img

# Test at t=0.5 (offset_y = 120)
f_mid = render_frame_slide(120, pair_idx=0)
f_mid.save("/home/stefan/Projects/ScoreTracker/tools/preview_slide_mid.png")
print("Saved preview_slide_mid.png")
