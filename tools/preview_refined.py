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

def load_logo(code, size=(28, 22)):
    path = f"/home/stefan/Projects/ScoreTracker/tools/logos_cache/{code}.png"
    if os.path.exists(path):
        im = Image.open(path).convert("RGBA")
        bbox = im.getbbox()
        if bbox:
            im = im.crop(bbox)
        w, h = im.size
        scale = min(size[0] / w, size[1] / h)
        nw, nh = max(1, int(w * scale)), max(1, int(h * scale))
        im_resized = im.resize((nw, nh), Image.Resampling.LANCZOS)
        canvas = Image.new("RGBA", size, (0, 0, 0, 0))
        ox = (size[0] - nw) // 2
        oy = (size[1] - nh) // 2
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
    }
]

# Option 2B: Stacked Table with balanced header (Status Pill Left, Date Right)
def render_refined_stacked(g1, g2, pair_idx=0, total_pairs=8):
    # Deep obsidian / charcoal
    img = Image.new("RGBA", (W, H), (10, 13, 18, 255))
    draw = ImageDraw.Draw(img)

    # Outer bezel: very faint dark ring
    draw.ellipse([1, 1, 238, 238], outline=(22, 28, 38), width=1)

    f_status = get_font("dejavu_bold", 8)
    f_date = get_font("dejavu", 8)
    f_team = get_font("sans_bold", 13)
    f_score = get_font("sans_bold", 16)
    f_rec = get_font("dejavu", 8)

    # Hairline divider between the two games
    draw.line([(48, 120), (192, 120)], fill=(25, 32, 44), width=1)

    # Dot pager at bottom (y = 227)
    start_dot_x = cx - (total_pairs * 8) // 2 + 4
    for i in range(total_pairs):
        dx = start_dot_x + i * 8
        if i == pair_idx:
            draw.ellipse([dx - 2, 227 - 2, dx + 2, 227 + 2], fill=(190, 200, 215))
        else:
            draw.ellipse([dx - 1, 227 - 1, dx + 1, 227 + 1], fill=(38, 48, 62))

    def render_game_stacked(g, y_start):
        # Coordinates:
        # Left boundary of content: x = 40
        # Right boundary of content: x = 200
        # Header: status on left, date on right
        status = g["status"]
        is_ot = "OT" in status
        is_live = "Q" in status or "LIVE" in status

        if is_live:
            bg_pill = (38, 22, 26); bd_pill = (85, 42, 50); tx_pill = (240, 130, 130)
        elif is_ot:
            bg_pill = (34, 28, 18); bd_pill = (78, 62, 38); tx_pill = (225, 185, 115)
        else:
            bg_pill = (18, 24, 34); bd_pill = (36, 46, 62); tx_pill = (140, 158, 180)

        bb = f_status.getbbox(status)
        pw = (bb[2] - bb[0]) + 10
        ph = 13
        draw.rounded_rectangle([40, y_start, 40 + pw, y_start + ph], radius=4, fill=bg_pill, outline=bd_pill)
        draw.text((40 + 5, y_start + 1), status, font=f_status, fill=tx_pill)

        # Date on right
        date_str = f"{g['day']} {g['date']}"
        bb_d = f_date.getbbox(date_str)
        draw.text((200 - (bb_d[2] - bb_d[0]), y_start + 1), date_str, font=f_date, fill=(80, 95, 115))

        away_win = g["away_score"] > g["home_score"]
        home_win = g["home_score"] > g["away_score"]
        
        # Colors:
        # Winner: bright crisp off-white #F2F5F8
        # Loser: muted slate #788698
        c_win = (242, 245, 248)
        c_lose = (120, 134, 152)

        # Away row
        y_a = y_start + 18
        logo_a = load_logo(g["away_code"], size=(26, 20))
        if logo_a:
            img.paste(logo_a, (40, y_a), logo_a)
        draw.text((72, y_a + 2), g["away_code"], font=f_team, fill=c_win if away_win else c_lose)
        draw.text((106, y_a + 4), g["away_rec"], font=f_rec, fill=(75, 88, 105))

        sc_a = str(g["away_score"])
        bb_a = f_score.getbbox(sc_a)
        draw.text((200 - (bb_a[2] - bb_a[0]), y_a + 1), sc_a, font=f_score, fill=c_win if away_win else c_lose)

        # Home row
        y_h = y_start + 41
        logo_h = load_logo(g["home_code"], size=(26, 20))
        if logo_h:
            img.paste(logo_h, (40, y_h), logo_h)
        draw.text((72, y_h + 2), g["home_code"], font=f_team, fill=c_win if home_win else c_lose)
        draw.text((106, y_h + 4), g["home_rec"], font=f_rec, fill=(75, 88, 105))

        sc_h = str(g["home_score"])
        bb_h = f_score.getbbox(sc_h)
        draw.text((200 - (bb_h[2] - bb_h[0]), y_h + 1), sc_h, font=f_score, fill=c_win if home_win else c_lose)

    render_game_stacked(g1, 23)
    render_game_stacked(g2, 135)
    return img

# Option 1B: Refined Horizontal Split with centered status and elegant spacing
def render_refined_horizontal(g1, g2, pair_idx=0, total_pairs=8):
    img = Image.new("RGBA", (W, H), (10, 13, 18, 255))
    draw = ImageDraw.Draw(img)

    draw.ellipse([1, 1, 238, 238], outline=(22, 28, 38), width=1)

    f_status = get_font("dejavu_bold", 8)
    f_team = get_font("sans_bold", 12)
    f_score = get_font("sans_bold", 17)
    f_sub = get_font("dejavu", 8)

    # Hairline divider
    draw.line([(45, 120), (195, 120)], fill=(25, 32, 44), width=1)

    # Dot pager at bottom
    start_dot_x = cx - (total_pairs * 8) // 2 + 4
    for i in range(total_pairs):
        dx = start_dot_x + i * 8
        if i == pair_idx:
            draw.ellipse([dx - 2, 227 - 2, dx + 2, 227 + 2], fill=(190, 200, 215))
        else:
            draw.ellipse([dx - 1, 227 - 1, dx + 1, 227 + 1], fill=(38, 48, 62))

    def render_game_horizontal(g, cy_row):
        status = g["status"]
        is_ot = "OT" in status
        is_live = "Q" in status or "LIVE" in status

        if is_live:
            bg_pill = (38, 22, 26); bd_pill = (85, 42, 50); tx_pill = (240, 130, 130)
        elif is_ot:
            bg_pill = (34, 28, 18); bd_pill = (78, 62, 38); tx_pill = (225, 185, 115)
        else:
            bg_pill = (18, 24, 34); bd_pill = (36, 46, 62); tx_pill = (140, 158, 180)

        # Centered status pill
        bb = f_status.getbbox(status)
        pw = (bb[2] - bb[0]) + 12
        py = cy_row - 26
        draw.rounded_rectangle([cx - pw//2, py, cx + pw//2, py + 13], radius=4, fill=bg_pill, outline=bd_pill)
        draw.text((cx - (bb[2] - bb[0])//2, py + 1), status, font=f_status, fill=tx_pill)

        away_win = g["away_score"] > g["home_score"]
        home_win = g["home_score"] > g["away_score"]
        c_win = (242, 245, 248)
        c_lose = (120, 134, 152)

        # Away: Logo + Code + Score
        # Logo at x = 32..58
        logo_a = load_logo(g["away_code"], size=(26, 20))
        if logo_a:
            img.paste(logo_a, (32, cy_row - 4), logo_a)
        draw.text((62, cy_row - 3), g["away_code"], font=f_team, fill=c_win if away_win else c_lose)
        sc_a = str(g["away_score"])
        bb_a = f_score.getbbox(sc_a)
        draw.text((106 - (bb_a[2] - bb_a[0]), cy_row - 6), sc_a, font=f_score, fill=c_win if away_win else c_lose)

        # Center separator
        draw.text((cx - 3, cy_row - 5), "-", font=f_sub, fill=(60, 72, 90))

        # Home: Score + Code + Logo
        sc_h = str(g["home_score"])
        draw.text((134, cy_row - 6), sc_h, font=f_score, fill=c_win if home_win else c_lose)
        draw.text((155, cy_row - 3), g["home_code"], font=f_team, fill=c_win if home_win else c_lose)
        logo_h = load_logo(g["home_code"], size=(26, 20))
        if logo_h:
            img.paste(logo_h, (182, cy_row - 4), logo_h)

        # Sub line: date & records
        sub_s = f"{g['away_rec']}   {g['day']} {g['date']}   {g['home_rec']}"
        bb_s = f_sub.getbbox(sub_s)
        draw.text((cx - (bb_s[2] - bb_s[0])//2, cy_row + 17), sub_s, font=f_sub, fill=(75, 88, 105))

    render_game_horizontal(g1, 62)
    render_game_horizontal(g2, 168)
    return img

img_s = render_refined_stacked(GAMES[0], GAMES[1])
img_s.save("/home/stefan/Projects/ScoreTracker/tools/preview_refined_stacked.png")

img_h = render_refined_horizontal(GAMES[0], GAMES[1])
img_h.save("/home/stefan/Projects/ScoreTracker/tools/preview_refined_horizontal.png")

print("Saved preview_refined_stacked.png and preview_refined_horizontal.png")
