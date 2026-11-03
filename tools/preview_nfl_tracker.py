import math
from PIL import Image, ImageDraw, ImageFont

W, H = 240, 240
cx, cy = 120, 120

def draw_nfl_logo(draw, x, y, size=28):
    # NFL Shield icon
    shield_pts = [
        (x - size//2, y - size//2),
        (x + size//2, y - size//2),
        (x + size//2, y + size//6),
        (x, y + size//2),
        (x - size//2, y + size//6)
    ]
    draw.polygon(shield_pts, fill=(10, 25, 60), outline=(210, 175, 55))
    # Football in center
    draw.ellipse([x - 4, y - 6, x + 4, y + 2], fill=(160, 60, 20), outline=(255, 255, 255))

def draw_team_logo(draw, x, y, code, size=28):
    half = size // 2
    r = half
    # Generic base badge
    draw.ellipse([x - r, y - r, x + r, y + r], fill=(20, 24, 30), outline=(50, 56, 68), width=1)

    if code == "KC":
        # Chiefs: Red oval with gold arrowhead
        draw.ellipse([x - r + 2, y - r + 2, x + r - 2, y + r - 2], fill=(227, 24, 55))
        draw.polygon([(x - 8, y + 6), (x, y - 8), (x + 8, y + 6), (x, y + 2)], fill=(255, 184, 28), outline=(0,0,0))
    elif code == "BAL":
        # Ravens: Deep Purple with gold beak / raven profile
        draw.ellipse([x - r + 2, y - r + 2, x + r - 2, y + r - 2], fill=(36, 23, 115))
        draw.polygon([(x - 8, y - 4), (x + 8, y - 1), (x - 2, y + 7)], fill=(158, 124, 12))
        draw.ellipse([x - 2, y - 3, x + 1, y], fill=(227, 24, 55))
    elif code == "SF":
        # 49ers: Scarlet red with gold inner oval
        draw.ellipse([x - r + 2, y - r + 2, x + r - 2, y + r - 2], fill=(170, 0, 0))
        draw.ellipse([x - 9, y - 6, x + 9, y + 6], outline=(179, 153, 93), width=2)
        # SF text
        draw.text((x - 6, y - 5), "SF", fill=(255, 255, 255))
    elif code == "NYJ":
        # Jets: Gotham green with white oval & wing
        draw.ellipse([x - r + 2, y - r + 2, x + r - 2, y + r - 2], fill=(18, 87, 64))
        draw.polygon([(x - 8, y), (x + 8, y - 5), (x + 5, y + 5)], fill=(255, 255, 255))
    elif code == "PHI":
        # Eagles: Midnight green with white eagle wing
        draw.ellipse([x - r + 2, y - r + 2, x + r - 2, y + r - 2], fill=(0, 76, 84))
        draw.polygon([(x - 8, y + 4), (x + 8, y - 6), (x + 6, y + 6), (x - 2, y + 7)], fill=(165, 172, 175), outline=(255,255,255))
    elif code == "GB":
        # Packers: Dark green oval with gold "G"
        draw.ellipse([x - r + 2, y - r + 2, x + r - 2, y + r - 2], fill=(32, 55, 49))
        draw.ellipse([x - 7, y - 6, x + 7, y + 6], outline=(255, 182, 18), width=2)
    elif code == "DET":
        # Lions: Honolulu blue with silver lion silhouette
        draw.ellipse([x - r + 2, y - r + 2, x + r - 2, y + r - 2], fill=(0, 118, 182))
        draw.polygon([(x - 8, y + 6), (x - 4, y - 6), (x + 6, y - 4), (x + 8, y + 6)], fill=(176, 183, 188))
    elif code == "LAR":
        # Rams: Royal blue with sol gold ram horn
        draw.ellipse([x - r + 2, y - r + 2, x + r - 2, y + r - 2], fill=(0, 53, 148))
        draw.arc([x - 7, y - 7, x + 7, y + 7], 45, 270, fill=(255, 163, 0), width=3)
    elif code == "DAL":
        # Cowboys: Navy blue with silver star
        draw.ellipse([x - r + 2, y - r + 2, x + r - 2, y + r - 2], fill=(0, 34, 68))
        # 5-point star
        pts = []
        for i in range(10):
            a = math.radians(i * 36 - 90)
            radius = 8 if i % 2 == 0 else 3.5
            pts.append((x + radius * math.cos(a), y + radius * math.sin(a)))
        draw.polygon(pts, fill=(255, 255, 255), outline=(134, 147, 151))
    elif code == "BUF":
        # Bills: Royal blue with red charging stripe
        draw.ellipse([x - r + 2, y - r + 2, x + r - 2, y + r - 2], fill=(0, 51, 141))
        draw.line([(x - 8, y - 2), (x + 8, y - 2)], fill=(198, 12, 48), width=3)
        draw.ellipse([x - 6, y - 6, x - 1, y - 1], fill=(255, 255, 255))
    else:
        # Default helmet badge with team code
        draw.ellipse([x - r + 2, y - r + 2, x + r - 2, y + r - 2], fill=(40, 50, 70))
        draw.text((x - 6, y - 5), code[:2], fill=(255, 255, 255))

def render_score_card(game):
    img = Image.new("RGBA", (W, H), (10, 12, 16, 255))
    draw = ImageDraw.Draw(img)

    # 1. Subtle stadium background vignette & outer bezel
    draw.ellipse([0, 0, 239, 239], fill=(12, 14, 20), outline=(35, 40, 52), width=2)
    # Pitch line grid
    draw.arc([15, 15, 225, 225], 0, 360, fill=(24, 28, 38), width=1)

    # 2. 2-Second Progress Ring along outer rim
    # Simulate progress at 65% (1.3s out of 2.0s)
    progress_deg = 234.0
    draw.arc([3, 3, 236, 236], -90, -90 + progress_deg, fill=(0, 200, 255), width=2)

    # Fonts
    try:
        font_header = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 9)
        font_badge = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 8)
        font_team = ImageFont.truetype("/usr/share/fonts/truetype/freefont/FreeSansBold.ttf", 16)
        font_rec = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 8)
        font_score = ImageFont.truetype("/usr/share/fonts/truetype/freefont/FreeSansBold.ttf", 26)
        font_detail = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 8)
        font_bold_detail = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 8)
        font_sub = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 7)
    except:
        font_header = ImageFont.load_default()
        font_badge = font_header
        font_team = font_header
        font_rec = font_header
        font_score = font_header
        font_detail = font_header
        font_bold_detail = font_header
        font_sub = font_header

    # 3. Top Header: NFL Badge & Date & Status
    # NFL Shield small
    draw.polygon([(cx - 40, 16), (cx - 28, 16), (cx - 28, 26), (cx - 34, 30), (cx - 40, 26)], fill=(10, 25, 60), outline=(210, 175, 55))
    draw.text((cx - 38, 17), "NFL", font=font_sub, fill=(255, 255, 255))

    # Date / Game Index: e.g. "GAME 1/14 • THU SEP 17"
    header_text = f"{game['day'].upper()} • {game['date'].upper()}"
    draw.text((cx - 22, 19), header_text, font=font_header, fill=(200, 210, 225))

    # Status Pill Badge: "FINAL" or "FINAL / OT"
    status_text = game["status"]
    sb_w = 46 if "OT" in status_text else 38
    draw.rounded_rectangle([cx - sb_w//2, 34, cx + sb_w//2, 46], radius=4, fill=(18, 55, 32), outline=(40, 160, 80))
    draw.text((cx - sb_w//2 + 5, 36), status_text, font=font_badge, fill=(120, 255, 160))

    # 4. Center Matchup Card
    # Background glass cards
    draw.rounded_rectangle([18, 52, 114, 142], radius=8, fill=(18, 22, 32), outline=(32, 38, 54))
    draw.rounded_rectangle([126, 52, 222, 142], radius=8, fill=(18, 22, 32), outline=(32, 38, 54))

    # AWAY TEAM (Left)
    draw_team_logo(draw, 44, 76, game["away_code"], size=28)
    draw.text((64, 66), game["away_code"], font=font_team, fill=(255, 255, 255))
    draw.text((65, 84), game["away_rec"], font=font_rec, fill=(130, 140, 160))
    
    # AWAY SCORE
    away_win = game["away_score"] > game["home_score"]
    away_col = (255, 255, 255) if away_win else (140, 150, 170)
    score_str_a = str(game["away_score"])
    draw.text((48, 104), score_str_a, font=font_score, fill=away_col)
    if away_win:
        # Gold Winner Indicator
        draw.polygon([(34, 118), (40, 113), (40, 123)], fill=(255, 210, 50))

    # HOME TEAM (Right)
    draw_team_logo(draw, 152, 76, game["home_code"], size=28)
    draw.text((172, 66), game["home_code"], font=font_team, fill=(255, 255, 255))
    draw.text((173, 84), game["home_rec"], font=font_rec, fill=(130, 140, 160))

    # HOME SCORE
    home_win = game["home_score"] > game["away_score"]
    home_col = (255, 255, 255) if home_win else (140, 150, 170)
    score_str_h = str(game["home_score"])
    draw.text((156, 104), score_str_h, font=font_score, fill=home_col)
    if home_win:
        draw.polygon([(206, 118), (200, 113), (200, 123)], fill=(255, 210, 50))

    # Center "@" badge
    draw.ellipse([cx - 9, 88, cx + 9, 106], fill=(26, 32, 46), outline=(48, 56, 78))
    draw.text((cx - 4, 91), "@", font=font_badge, fill=(160, 175, 200))

    # 5. Quarter-by-Quarter Box / Line Score
    # Box header
    draw.rounded_rectangle([28, 148, 212, 188], radius=6, fill=(14, 18, 26), outline=(28, 34, 48))
    # Columns: TEAM  1   2   3   4   TOT
    draw.text((36, 152), "TEAM", font=font_sub, fill=(110, 120, 140))
    draw.text((76, 152), "Q1", font=font_sub, fill=(110, 120, 140))
    draw.text((102, 152), "Q2", font=font_sub, fill=(110, 120, 140))
    draw.text((128, 152), "Q3", font=font_sub, fill=(110, 120, 140))
    draw.text((154, 152), "Q4", font=font_sub, fill=(110, 120, 140))
    draw.text((188, 152), "TOT", font=font_sub, fill=(210, 175, 55))

    # Away Qs
    draw.text((36, 163), game["away_code"], font=font_bold_detail, fill=(220, 225, 235))
    draw.text((76, 163), str(game["away_q"][0]), font=font_detail, fill=(180, 190, 205))
    draw.text((102, 163), str(game["away_q"][1]), font=font_detail, fill=(180, 190, 205))
    draw.text((128, 163), str(game["away_q"][2]), font=font_detail, fill=(180, 190, 205))
    draw.text((154, 163), str(game["away_q"][3]), font=font_detail, fill=(180, 190, 205))
    draw.text((188, 163), str(game["away_score"]), font=font_bold_detail, fill=away_col)

    # Home Qs
    draw.text((36, 175), game["home_code"], font=font_bold_detail, fill=(220, 225, 235))
    draw.text((76, 175), str(game["home_q"][0]), font=font_detail, fill=(180, 190, 205))
    draw.text((102, 175), str(game["home_q"][1]), font=font_detail, fill=(180, 190, 205))
    draw.text((128, 175), str(game["home_q"][2]), font=font_detail, fill=(180, 190, 205))
    draw.text((154, 175), str(game["home_q"][3]), font=font_detail, fill=(180, 190, 205))
    draw.text((188, 175), str(game["home_score"]), font=font_bold_detail, fill=home_col)

    # 6. Bottom Arc: Top Leader & Game Counter Dots
    # Top highlight
    lead_text = f"★ {game['leader']}"
    tb = font_badge.getbbox(lead_text)
    lw = tb[2] - tb[0]
    draw.text((cx - lw//2, 196), lead_text, font=font_badge, fill=(255, 200, 60))

    # Stadium & Pagination Dots
    stad_text = game["stadium"]
    sb = font_sub.getbbox(stad_text)
    sw = sb[2] - sb[0]
    draw.text((cx - sw//2, 208), stad_text, font=font_sub, fill=(120, 130, 145))

    # Dot pagination (14 games)
    total_games = 14
    current_idx = game["index"]
    dot_spacing = 8
    start_x = cx - (total_games * dot_spacing) // 2 + 4
    for i in range(total_games):
        dx = start_x + i * dot_spacing
        dy = 222
        if i == current_idx:
            draw.ellipse([dx - 2.5, dy - 2.5, dx + 2.5, dy + 2.5], fill=(0, 200, 255))
        else:
            draw.ellipse([dx - 1.2, dy - 1.2, dx + 1.2, dy + 1.2], fill=(50, 60, 80))

    return img

test_game = {
    "index": 0,
    "day": "Thu",
    "date": "Sep 17",
    "status": "FINAL",
    "away_code": "BAL",
    "away_rec": "0-1",
    "away_score": 20,
    "away_q": [7, 3, 7, 3],
    "home_code": "KC",
    "home_rec": "1-0",
    "home_score": 27,
    "home_q": [7, 10, 3, 7],
    "leader": "Mahomes: 291 YDS, 2 TD",
    "stadium": "GEHA Field at Arrowhead"
}

img = render_score_card(test_game)
img.save("tools/preview_nfl_score_tracker.png")
print("Saved tools/preview_nfl_score_tracker.png")
