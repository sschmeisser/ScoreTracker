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

def render_zen_tracker(g1, g2, pair_idx=0, total_pairs=8, score_size=18, show_cards=False):
    # Palette: Zen obsidian slate
    img = Image.new("RGBA", (W, H), (10, 13, 18, 255))
    draw = ImageDraw.Draw(img)

    # Subtle boundary ring
    draw.ellipse([1, 1, 238, 238], outline=(20, 26, 36), width=1)

    f_status = get_font("dejavu_bold", 8)
    f_team = get_font("sans_bold", 12)
    f_score = get_font("sans_bold", score_size)
    f_sub = get_font("dejavu", 8)

    # Center hairline separator
    draw.line([(45, 120), (195, 120)], fill=(24, 30, 42), width=1)

    # Dot pager at bottom (y = 227)
    start_dot_x = cx - (total_pairs * 8) // 2 + 4
    for i in range(total_pairs):
        dx = start_dot_x + i * 8
        if i == pair_idx:
            draw.ellipse([dx - 2, 227 - 2, dx + 2, 227 + 2], fill=(185, 195, 210))
        else:
            draw.ellipse([dx - 1, 227 - 1, dx + 1, 227 + 1], fill=(36, 44, 58))

    def render_row(g, cy_row):
        # Optional subtle card backing
        if show_cards:
            draw.rounded_rectangle([24, cy_row - 34, 216, cy_row + 34], radius=8, fill=(14, 18, 26), outline=(26, 33, 46))

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

        # High-contrast yet calm colors
        c_score_win = (245, 247, 250)      # Soft off-white
        c_score_lose = (112, 125, 142)     # Muted slate stone
        c_team_win = (225, 230, 238)
        c_team_lose = (130, 142, 158)

        # Away side (Left)
        # 32x26 logo centered at x = 38
        logo_a = load_logo_32x26(g["away_code"])
        if logo_a:
            img.paste(logo_a, (38 - 16, cy_row - 13 + 3), logo_a)

        # Away Code
        draw.text((60, cy_row - 5 + 3), g["away_code"], font=f_team, fill=c_team_win if away_win else c_team_lose)

        # Away Score (right-aligned to x=105)
        sc_a = str(g["away_score"])
        bb_a = f_score.getbbox(sc_a)
        draw.text((105 - (bb_a[2] - bb_a[0]), cy_row - (score_size // 2) + 2), sc_a, font=f_score, fill=c_score_win if away_win else c_score_lose)

        # Center separator: serene dot
        draw.ellipse([cx - 1, cy_row + 2, cx + 1, cy_row + 4], fill=(55, 66, 82))

        # Home Score (left-aligned at x=135)
        sc_h = str(g["home_score"])
        draw.text((135, cy_row - (score_size // 2) + 2), sc_h, font=f_score, fill=c_score_win if home_win else c_score_lose)

        # Home Code (right-aligned at x=180)
        bb_hcode = f_team.getbbox(g["home_code"])
        draw.text((180 - (bb_hcode[2] - bb_hcode[0]), cy_row - 5 + 3), g["home_code"], font=f_team, fill=c_team_win if home_win else c_team_lose)

        # Home Logo centered at x = 202
        logo_h = load_logo_32x26(g["home_code"])
        if logo_h:
            img.paste(logo_h, (202 - 16, cy_row - 13 + 3), logo_h)

        # Sub line: e.g. "0-1  ·  THU SEP 17  ·  1-0"
        sub_s = f"{g['away_rec']}  ·  {g['day']} {g['date']}  ·  {g['home_rec']}"
        bb_s = f_sub.getbbox(sub_s)
        draw.text((cx - (bb_s[2] - bb_s[0])//2, cy_row + 18), sub_s, font=f_sub, fill=(72, 84, 100))

    render_row(g1, 62)
    render_row(g2, 168)
    return img

img_zen_clean = render_zen_tracker(GAMES[0], GAMES[1], score_size=18, show_cards=False)
img_zen_clean.save("/home/stefan/Projects/ScoreTracker/tools/preview_zen_clean.png")

img_zen_cards = render_zen_tracker(GAMES[0], GAMES[1], score_size=18, show_cards=True)
img_zen_cards.save("/home/stefan/Projects/ScoreTracker/tools/preview_zen_cards.png")

img_zen_pair2 = render_zen_tracker(GAMES[2], GAMES[3], pair_idx=1, total_pairs=8, score_size=18, show_cards=False)
img_zen_pair2.save("/home/stefan/Projects/ScoreTracker/tools/preview_zen_pair2.png")

print("Generated zen previews")
