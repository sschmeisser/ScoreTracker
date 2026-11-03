import math
from PIL import Image, ImageDraw, ImageFont

W, H = 240, 240
cx, cy = 120, 120

def draw_team_logo(draw, x, y, code, size=28):
    half = size // 2
    r = half
    draw.ellipse([x - r, y - r, x + r, y + r], fill=(22, 26, 36), outline=(48, 54, 70), width=1)

    if code == "KC":
        draw.ellipse([x - r + 2, y - r + 2, x + r - 2, y + r - 2], fill=(227, 24, 55))
        draw.polygon([(x - 7, y + 6), (x, y - 7), (x + 7, y + 6), (x, y + 2)], fill=(255, 184, 28), outline=(0,0,0))
    elif code == "BAL":
        draw.ellipse([x - r + 2, y - r + 2, x + r - 2, y + r - 2], fill=(36, 23, 115))
        draw.polygon([(x - 7, y - 4), (x + 7, y - 1), (x - 2, y + 6)], fill=(158, 124, 12))
        draw.ellipse([x - 2, y - 3, x + 1, y], fill=(227, 24, 55))
    elif code == "SF":
        draw.ellipse([x - r + 2, y - r + 2, x + r - 2, y + r - 2], fill=(170, 0, 0))
        draw.ellipse([x - 8, y - 6, x + 8, y + 6], outline=(179, 153, 93), width=2)
        draw.text((x - 6, y - 5), "SF", fill=(255, 255, 255))
    elif code == "NYJ":
        draw.ellipse([x - r + 2, y - r + 2, x + r - 2, y + r - 2], fill=(18, 87, 64))
        draw.polygon([(x - 7, y), (x + 7, y - 5), (x + 5, y + 5)], fill=(255, 255, 255))
    elif code == "PHI":
        draw.ellipse([x - r + 2, y - r + 2, x + r - 2, y + r - 2], fill=(0, 76, 84))
        draw.polygon([(x - 7, y + 4), (x + 7, y - 5), (x + 5, y + 5), (x - 2, y + 6)], fill=(165, 172, 175), outline=(255,255,255))
    elif code == "GB":
        draw.ellipse([x - r + 2, y - r + 2, x + r - 2, y + r - 2], fill=(32, 55, 49))
        draw.ellipse([x - 6, y - 5, x + 6, y + 5], outline=(255, 182, 18), width=2)
    elif code == "DET":
        draw.ellipse([x - r + 2, y - r + 2, x + r - 2, y + r - 2], fill=(0, 118, 182))
        draw.polygon([(x - 7, y + 5), (x - 3, y - 5), (x + 5, y - 3), (x + 7, y + 5)], fill=(176, 183, 188))
    elif code == "LAR":
        draw.ellipse([x - r + 2, y - r + 2, x + r - 2, y + r - 2], fill=(0, 53, 148))
        draw.arc([x - 6, y - 6, x + 6, y + 6], 45, 270, fill=(255, 163, 0), width=3)
    elif code == "DAL":
        draw.ellipse([x - r + 2, y - r + 2, x + r - 2, y + r - 2], fill=(0, 34, 68))
        pts = []
        for i in range(10):
            a = math.radians(i * 36 - 90)
            radius = 7 if i % 2 == 0 else 3.0
            pts.append((x + radius * math.cos(a), y + radius * math.sin(a)))
        draw.polygon(pts, fill=(255, 255, 255), outline=(134, 147, 151))
    else:
        draw.ellipse([x - r + 2, y - r + 2, x + r - 2, y + r - 2], fill=(35, 45, 65))
        draw.text((x - 6, y - 5), code[:2], fill=(255, 255, 255))

def render_score_card(game):
    img = Image.new("RGBA", (W, H), (8, 10, 14, 255))
    draw = ImageDraw.Draw(img)

    # Outer bezel & dark background
    draw.ellipse([0, 0, 239, 239], fill=(10, 12, 18), outline=(32, 38, 50), width=2)

    # 2-Second Progress Ring
    progress_frac = 0.55
    draw.arc([2, 2, 237, 237], -90, -90 + progress_frac * 360.0, fill=(0, 210, 255), width=2)

    try:
        font_header = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 9)
        font_badge = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 8)
        font_team = ImageFont.truetype("/usr/share/fonts/truetype/freefont/FreeSansBold.ttf", 15)
        font_rec = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 8)
        font_score = ImageFont.truetype("/usr/share/fonts/truetype/freefont/FreeSansBold.ttf", 28)
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

    # Top Header: NFL Badge
    draw.polygon([(cx - 7, 7), (cx + 7, 7), (cx + 7, 13), (cx, 18), (cx - 7, 13)], fill=(12, 28, 70), outline=(215, 180, 50))
    # Centered NFL text
    draw.text((cx - 5, 8), "NFL", font=font_sub, fill=(255, 255, 255))

    # Date line: "THU SEP 17 - GAME 1/14"
    date_txt = f"{game['day'].upper()} {game['date'].upper()}  •  GAME {game['index']+1}/14"
    bb = font_header.getbbox(date_txt)
    draw.text((cx - (bb[2]-bb[0])//2, 22), date_txt, font=font_header, fill=(195, 205, 220))

    # Status Pill Badge: "FINAL" or "FINAL / OT"
    status_text = game["status"]
    sb_w = 48 if "OT" in status_text else 38
    draw.rounded_rectangle([cx - sb_w//2, 35, cx + sb_w//2, 47], radius=4, fill=(16, 52, 30), outline=(40, 160, 80))
    tb_s = font_badge.getbbox(status_text)
    draw.text((cx - (tb_s[2]-tb_s[0])//2, 36), status_text, font=font_badge, fill=(120, 255, 160))

    # Center Matchup Cards
    draw.rounded_rectangle([18, 52, 114, 140], radius=7, fill=(16, 20, 30), outline=(32, 38, 54))
    draw.rounded_rectangle([126, 52, 222, 140], radius=7, fill=(16, 20, 30), outline=(32, 38, 54))

    away_win = game["away_score"] > game["home_score"]
    home_win = game["home_score"] > game["away_score"]

    # AWAY TEAM (Left)
    draw_team_logo(draw, 40, 74, game["away_code"], size=28)
    draw.text((58, 64), game["away_code"], font=font_team, fill=(255, 255, 255))
    draw.text((59, 82), game["away_rec"], font=font_rec, fill=(130, 140, 160))

    score_str_a = str(game["away_score"])
    away_col = (255, 255, 255) if away_win else (135, 145, 165)
    bb_sa = font_score.getbbox(score_str_a)
    draw.text((66 - (bb_sa[2]-bb_sa[0])//2, 98), score_str_a, font=font_score, fill=away_col)
    if away_win:
        draw.polygon([(26, 117), (32, 112), (32, 122)], fill=(255, 210, 50))

    # HOME TEAM (Right)
    draw_team_logo(draw, 148, 74, game["home_code"], size=28)
    draw.text((166, 64), game["home_code"], font=font_team, fill=(255, 255, 255))
    draw.text((167, 82), game["home_rec"], font=font_rec, fill=(130, 140, 160))

    score_str_h = str(game["home_score"])
    home_col = (255, 255, 255) if home_win else (135, 145, 165)
    bb_sh = font_score.getbbox(score_str_h)
    draw.text((174 - (bb_sh[2]-bb_sh[0])//2, 98), score_str_h, font=font_score, fill=home_col)
    if home_win:
        draw.polygon([(214, 117), (208, 112), (208, 122)], fill=(255, 210, 50))

    # Center "@" badge
    draw.ellipse([cx - 8, 87, cx + 8, 103], fill=(24, 30, 44), outline=(44, 52, 72))
    draw.text((cx - 4, 89), "@", font=font_badge, fill=(160, 175, 200))

    # Quarter-by-Quarter Line Score
    draw.rounded_rectangle([28, 145, 212, 184], radius=5, fill=(14, 18, 26), outline=(28, 34, 48))
    draw.text((36, 148), "TEAM", font=font_sub, fill=(110, 120, 140))
    draw.text((76, 148), "Q1", font=font_sub, fill=(110, 120, 140))
    draw.text((102, 148), "Q2", font=font_sub, fill=(110, 120, 140))
    draw.text((128, 148), "Q3", font=font_sub, fill=(110, 120, 140))
    draw.text((154, 148), "Q4", font=font_sub, fill=(110, 120, 140))
    draw.text((188, 148), "TOT", font=font_sub, fill=(210, 175, 55))

    draw.text((36, 159), game["away_code"], font=font_bold_detail, fill=(220, 225, 235))
    draw.text((76, 159), str(game["away_q"][0]), font=font_detail, fill=(180, 190, 205))
    draw.text((102, 159), str(game["away_q"][1]), font=font_detail, fill=(180, 190, 205))
    draw.text((128, 159), str(game["away_q"][2]), font=font_detail, fill=(180, 190, 205))
    draw.text((154, 159), str(game["away_q"][3]), font=font_detail, fill=(180, 190, 205))
    draw.text((188, 159), str(game["away_score"]), font=font_bold_detail, fill=away_col)

    draw.text((36, 171), game["home_code"], font=font_bold_detail, fill=(220, 225, 235))
    draw.text((76, 171), str(game["home_q"][0]), font=font_detail, fill=(180, 190, 205))
    draw.text((102, 171), str(game["home_q"][1]), font=font_detail, fill=(180, 190, 205))
    draw.text((128, 171), str(game["home_q"][2]), font=font_detail, fill=(180, 190, 205))
    draw.text((154, 171), str(game["home_q"][3]), font=font_detail, fill=(180, 190, 205))
    draw.text((188, 171), str(game["home_score"]), font=font_bold_detail, fill=home_col)

    # Bottom Highlight & Stadium
    lead_text = f"★ {game['leader']}"
    tb = font_badge.getbbox(lead_text)
    draw.text((cx - (tb[2]-tb[0])//2, 191), lead_text, font=font_badge, fill=(255, 205, 60))

    stad_text = game["stadium"]
    sb = font_sub.getbbox(stad_text)
    draw.text((cx - (sb[2]-sb[0])//2, 204), stad_text, font=font_sub, fill=(125, 135, 150))

    # Dot pagination (14 dots)
    total_games = 14
    current_idx = game["index"]
    dot_spacing = 8
    start_x = cx - (total_games * dot_spacing) // 2 + 4
    for i in range(total_games):
        dx = start_x + i * dot_spacing
        dy = 220
        if i == current_idx:
            draw.ellipse([dx - 2.5, dy - 2.5, dx + 2.5, dy + 2.5], fill=(0, 210, 255))
        else:
            draw.ellipse([dx - 1.2, dy - 1.2, dx + 1.2, dy + 1.2], fill=(45, 55, 75))

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
img.save("tools/preview_nfl_score_tracker_v3.png")
print("Saved tools/preview_nfl_score_tracker_v3.png")
