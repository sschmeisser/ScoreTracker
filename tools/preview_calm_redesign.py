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

def load_logo(code, size=(32, 26)):
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
        "detail": "Mahomes: 291 YDS, 2 TD"
    },
    {
        "day": "FRI", "date": "SEP 18", "status": "FINAL",
        "away_code": "GB",  "away_score": 29, "away_rec": "0-1",
        "home_code": "PHI", "home_score": 34, "home_rec": "1-0",
        "detail": "Barkley: 109 YDS, 3 TD"
    },
    {
        "day": "SUN", "date": "SEP 20", "status": "FINAL/OT",
        "away_code": "LAR", "away_score": 20, "away_rec": "0-1",
        "home_code": "DET", "home_score": 26, "home_rec": "1-0",
        "detail": "Montgomery: 91 YDS, 1 TD"
    },
    {
        "day": "MON", "date": "SEP 21", "status": "FINAL",
        "away_code": "NYJ", "away_score": 19, "away_rec": "0-1",
        "home_code": "SF",  "home_score": 32, "home_rec": "1-0",
        "detail": "Mason: 147 YDS, 1 TD"
    }
]

def render_option_horizontal(g1, g2, pair_idx=0, total_pairs=8):
    # Deep obsidian / slate palette
    # BG: #0B0E14 (11, 14, 20)
    img = Image.new("RGBA", (W, H), (11, 14, 20, 255))
    draw = ImageDraw.Draw(img)

    # Subtle outer circular boundary
    draw.ellipse([0, 0, 239, 239], fill=(11, 14, 20), outline=(26, 32, 44), width=1)

    f_status = get_font("dejavu_bold", 8)
    f_code = get_font("sans_bold", 12)
    f_score = get_font("sans_bold", 18)
    f_sub = get_font("dejavu", 7)
    f_header = get_font("dejavu", 8)

    # Hairline divider between the two games
    draw.line([(45, 120), (195, 120)], fill=(30, 37, 50), width=1)

    # Tiny subtle NFL header or dots at top/bottom
    # Dot pager at bottom (y = 224)
    start_dot_x = cx - (total_pairs * 8) // 2 + 4
    for i in range(total_pairs):
        dx = start_dot_x + i * 8
        if i == pair_idx:
            draw.ellipse([dx - 2, 224 - 2, dx + 2, 224 + 2], fill=(180, 190, 205))
        else:
            draw.ellipse([dx - 1, 224 - 1, dx + 1, 224 + 1], fill=(45, 55, 70))

    def render_row(g, cy_row):
        # 1. Status pill
        status = g["status"]
        is_ot = "OT" in status
        is_live = "Q" in status or "LIVE" in status
        
        # Soft muted status colors
        if is_live:
            bg_pill = (35, 20, 24)
            bd_pill = (85, 40, 48)
            tx_pill = (235, 120, 120)
        elif is_ot:
            bg_pill = (32, 26, 18)
            bd_pill = (75, 60, 35)
            tx_pill = (220, 180, 110)
        else:
            # Final / serene slate
            bg_pill = (18, 23, 32)
            bd_pill = (38, 48, 64)
            tx_pill = (140, 155, 175)

        bb = f_status.getbbox(status)
        pw = (bb[2] - bb[0]) + 12
        ph = 13
        py = cy_row - 26
        draw.rounded_rectangle([cx - pw//2, py, cx + pw//2, py + ph], radius=4, fill=bg_pill, outline=bd_pill)
        draw.text((cx - (bb[2] - bb[0])//2, py + 1), status, font=f_status, fill=tx_pill)

        # 2. Matchup row
        away_win = g["away_score"] > g["home_score"]
        home_win = g["home_score"] > g["away_score"]
        c_win = (240, 243, 248)     # Soft off-white
        c_lose = (105, 118, 135)    # Muted stone slate
        c_code_win = (225, 230, 238)
        c_code_lose = (130, 142, 160)

        # Away Team (Left)
        # Logo at x = 38
        logo_a = load_logo(g["away_code"], size=(28, 22))
        if logo_a:
            img.paste(logo_a, (38 - 14, cy_row - 1), logo_a)
        
        # Code
        draw.text((58, cy_row - 4), g["away_code"], font=f_code, fill=c_code_win if away_win else c_code_lose)

        # Away Score (right-aligned at x=105)
        sc_a = str(g["away_score"])
        bb_a = f_score.getbbox(sc_a)
        w_a = bb_a[2] - bb_a[0]
        draw.text((105 - w_a, cy_row - 8), sc_a, font=f_score, fill=c_win if away_win else c_lose)

        # Center separator: serene subtle dot or dash
        draw.ellipse([cx - 1, cy_row + 1, cx + 1, cy_row + 3], fill=(60, 72, 90))

        # Home Score (left-aligned at x=135)
        sc_h = str(g["home_score"])
        draw.text((135, cy_row - 8), sc_h, font=f_score, fill=c_win if home_win else c_lose)

        # Home Code
        draw.text((160, cy_row - 4), g["home_code"], font=f_code, fill=c_code_win if home_win else c_code_lose)

        # Home Logo at x = 202
        logo_h = load_logo(g["home_code"], size=(28, 22))
        if logo_h:
            img.paste(logo_h, (202 - 14, cy_row - 1), logo_h)

        sub_txt = f"{g['away_rec']}  -  {g['day']} {g['date']}  -  {g['home_rec']}"
        bb_s = f_sub.getbbox(sub_txt)
        draw.text((cx - (bb_s[2] - bb_s[0])//2, cy_row + 19), sub_txt, font=f_sub, fill=(85, 98, 115))

    render_row(g1, 62)
    render_row(g2, 168)
    return img

def render_option_stacked(g1, g2, pair_idx=0, total_pairs=8):

    # Deep obsidian / slate palette
    img = Image.new("RGBA", (W, H), (11, 14, 20, 255))
    draw = ImageDraw.Draw(img)

    # Subtle outer circular boundary
    draw.ellipse([0, 0, 239, 239], fill=(11, 14, 20), outline=(24, 30, 40), width=1)

    f_status = get_font("dejavu_bold", 8)
    f_date = get_font("dejavu", 8)
    f_team = get_font("sans_bold", 12)
    f_score = get_font("sans_bold", 15)
    f_rec = get_font("dejavu", 8)

    # Hairline divider between the two games
    draw.line([(40, 120), (200, 120)], fill=(28, 35, 48), width=1)

    # Dot pager at bottom (y = 227)
    start_dot_x = cx - (total_pairs * 8) // 2 + 4
    for i in range(total_pairs):
        dx = start_dot_x + i * 8
        if i == pair_idx:
            draw.ellipse([dx - 2, 227 - 2, dx + 2, 227 + 2], fill=(185, 195, 210))
        else:
            draw.ellipse([dx - 1, 227 - 1, dx + 1, 227 + 1], fill=(42, 50, 65))

    def render_stacked_game(g, y_start):
        # Status Pill & Date
        status = g["status"]
        is_ot = "OT" in status
        is_live = "Q" in status or "LIVE" in status
        
        if is_live:
            bg_pill = (35, 20, 24)
            bd_pill = (85, 40, 48)
            tx_pill = (235, 120, 120)
        elif is_ot:
            bg_pill = (32, 26, 18)
            bd_pill = (75, 60, 35)
            tx_pill = (220, 180, 110)
        else:
            bg_pill = (18, 23, 32)
            bd_pill = (38, 48, 64)
            tx_pill = (140, 155, 175)

        bb = f_status.getbbox(status)
        pw = (bb[2] - bb[0]) + 10
        ph = 13
        pill_x = 42
        draw.rounded_rectangle([pill_x, y_start, pill_x + pw, y_start + ph], radius=4, fill=bg_pill, outline=bd_pill)
        draw.text((pill_x + 5, y_start + 1), status, font=f_status, fill=tx_pill)

        # Date text right next to status pill
        date_str = f"{g['day']} {g['date']}"
        draw.text((pill_x + pw + 8, y_start + 1), date_str, font=f_date, fill=(85, 98, 115))

        away_win = g["away_score"] > g["home_score"]
        home_win = g["home_score"] > g["away_score"]
        c_win = (245, 248, 252)     # Crisp off-white
        c_lose = (110, 122, 140)    # Muted stone slate

        # Row 1: Away Team
        y_away = y_start + 17
        logo_a = load_logo(g["away_code"], size=(26, 20))
        if logo_a:
            img.paste(logo_a, (42, y_away), logo_a)
        draw.text((74, y_away + 3), g["away_code"], font=f_team, fill=c_win if away_win else c_lose)
        draw.text((108, y_away + 4), g["away_rec"], font=f_rec, fill=(75, 88, 105))
        
        sc_a = str(g["away_score"])
        bb_a = f_score.getbbox(sc_a)
        draw.text((198 - (bb_a[2] - bb_a[0]), y_away + 1), sc_a, font=f_score, fill=c_win if away_win else c_lose)

        # Row 2: Home Team
        y_home = y_start + 40
        logo_h = load_logo(g["home_code"], size=(26, 20))
        if logo_h:
            img.paste(logo_h, (42, y_home), logo_h)
        draw.text((74, y_home + 3), g["home_code"], font=f_team, fill=c_win if home_win else c_lose)
        draw.text((108, y_home + 4), g["home_rec"], font=f_rec, fill=(75, 88, 105))
        
        sc_h = str(g["home_score"])
        bb_h = f_score.getbbox(sc_h)
        draw.text((198 - (bb_h[2] - bb_h[0]), y_home + 1), sc_h, font=f_score, fill=c_win if home_win else c_lose)

    render_stacked_game(g1, 24)
    render_stacked_game(g2, 134)

    return img

def render_option_cards(g1, g2, pair_idx=0, total_pairs=8):
    # Soft rounded cards on obsidian canvas
    img = Image.new("RGBA", (W, H), (9, 11, 16, 255))
    draw = ImageDraw.Draw(img)

    draw.ellipse([0, 0, 239, 239], fill=(9, 11, 16), outline=(22, 28, 38), width=1)

    f_status = get_font("dejavu_bold", 8)
    f_code = get_font("sans_bold", 13)
    f_score = get_font("sans_bold", 20)
    f_sub = get_font("dejavu", 8)

    # Dot pager at bottom
    start_dot_x = cx - (total_pairs * 8) // 2 + 4
    for i in range(total_pairs):
        dx = start_dot_x + i * 8
        if i == pair_idx:
            draw.ellipse([dx - 2, 226 - 2, dx + 2, 226 + 2], fill=(185, 195, 210))
        else:
            draw.ellipse([dx - 1, 226 - 1, dx + 1, 226 + 1], fill=(38, 46, 60))

    def render_card(g, top_y, bot_y):
        # Card bounds: width 184 (from 28 to 212)
        card_x1, card_x2 = 26, 214
        draw.rounded_rectangle([card_x1, top_y, card_x2, bot_y], radius=10, fill=(15, 19, 28), outline=(28, 35, 48))

        # Status badge
        status = g["status"]
        is_ot = "OT" in status
        is_live = "Q" in status or "LIVE" in status
        if is_live:
            bg_p = (40, 22, 26); bd_p = (90, 45, 52); tx_p = (240, 130, 130)
        elif is_ot:
            bg_p = (36, 28, 18); bd_p = (80, 62, 36); tx_p = (225, 185, 115)
        else:
            bg_p = (20, 26, 38); bd_p = (42, 54, 72); tx_p = (145, 162, 185)

        bb = f_status.getbbox(status)
        pw = (bb[2] - bb[0]) + 12
        py = top_y + 6
        draw.rounded_rectangle([cx - pw//2, py, cx + pw//2, py + 12], radius=3, fill=bg_p, outline=bd_p)
        draw.text((cx - (bb[2] - bb[0])//2, py + 1), status, font=f_status, fill=tx_p)

        # Matchup
        cy_m = top_y + 38
        away_win = g["away_score"] > g["home_score"]
        home_win = g["home_score"] > g["away_score"]
        c_win = (245, 248, 252)
        c_lose = (105, 118, 136)

        # Away: Logo + Code + Score
        logo_a = load_logo(g["away_code"], size=(28, 22))
        if logo_a:
            img.paste(logo_a, (34, cy_m - 10), logo_a)
        draw.text((66, cy_m - 8), g["away_code"], font=f_code, fill=c_win if away_win else c_lose)
        sc_a = str(g["away_score"])
        bb_a = f_score.getbbox(sc_a)
        draw.text((108 - (bb_a[2] - bb_a[0]), cy_m - 10), sc_a, font=f_score, fill=c_win if away_win else c_lose)

        # Center separator
        draw.text((cx - 3, cy_m - 9), "-", font=f_score, fill=(50, 60, 78))

        # Home: Score + Code + Logo
        sc_h = str(g["home_score"])
        draw.text((132, cy_m - 10), sc_h, font=f_score, fill=c_win if home_win else c_lose)
        draw.text((154, cy_m - 8), g["home_code"], font=f_code, fill=c_win if home_win else c_lose)
        logo_h = load_logo(g["home_code"], size=(28, 22))
        if logo_h:
            img.paste(logo_h, (178, cy_m - 10), logo_h)

        # Date / Record subline
        sub_s = f"{g['away_rec']}  -  {g['day']} {g['date']}  -  {g['home_rec']}"
        bb_s = f_sub.getbbox(sub_s)
        draw.text((cx - (bb_s[2] - bb_s[0])//2, top_y + 63), sub_s, font=f_sub, fill=(78, 90, 108))

    render_card(g1, 24, 108)
    render_card(g2, 126, 210)

    return img

img_opt1 = render_option_horizontal(GAMES[0], GAMES[1])
img_opt1.save("/home/stefan/Projects/ScoreTracker/tools/preview_calm_opt1.png")

img_opt2 = render_option_stacked(GAMES[0], GAMES[1])
img_opt2.save("/home/stefan/Projects/ScoreTracker/tools/preview_calm_opt2.png")

img_opt3 = render_option_cards(GAMES[0], GAMES[1])
img_opt3.save("/home/stefan/Projects/ScoreTracker/tools/preview_calm_opt3.png")

print("Generated opt1, opt2, opt3")

