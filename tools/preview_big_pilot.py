import math
from PIL import Image, ImageDraw, ImageFont

def render_dial(lume_mode=False):
    W, H = 240, 240
    cx, cy = 120, 120
    img = Image.new("RGBA", (W, H), (0, 0, 0, 255))
    draw = ImageDraw.Draw(img)

    # Color palette
    if not lume_mode:
        # Day Mode (Matte black dial, crisp white markers, high contrast)
        COL_DIAL = (8, 9, 10)
        COL_BEZEL = (35, 38, 42)
        COL_MAJOR_TICK = (250, 252, 255)
        COL_MINOR_TICK = (180, 185, 190)
        COL_TRIANGLE = (250, 252, 255)
        COL_NUMERAL = (250, 252, 255)
        COL_TEXT_PILOT = (130, 135, 140)
        COL_TEXT_PST = (160, 165, 170)
        COL_HAND_BORDER = (10, 10, 12)
        COL_HAND_FILL = (245, 248, 252)
        COL_SEC_HAND = (255, 255, 255)
        COL_SEC_PIP = (255, 255, 255)
    else:
        # Night Lume Mode (Simulated Super-LumiNova C3/BGW9 glowing green/cyan)
        COL_DIAL = (3, 7, 5)
        COL_BEZEL = (10, 20, 15)
        COL_MAJOR_TICK = (130, 255, 180)
        COL_MINOR_TICK = (60, 180, 110)
        COL_TRIANGLE = (150, 255, 200)
        COL_NUMERAL = (140, 255, 190)
        COL_TEXT_PILOT = (40, 120, 75)
        COL_TEXT_PST = (50, 140, 90)
        COL_HAND_BORDER = (5, 15, 10)
        COL_HAND_FILL = (160, 255, 210)
        COL_SEC_HAND = (170, 255, 210)
        COL_SEC_PIP = (170, 255, 210)

    # Dial circle
    draw.ellipse([2, 2, 237, 237], fill=COL_DIAL, outline=COL_BEZEL, width=1)

    # 60 Ticks
    for i in range(60):
        angle = math.radians(i * 6.0 - 90.0)
        cos_a = math.cos(angle)
        sin_a = math.sin(angle)

        if i == 0:
            continue  # 12 o'clock triangle occupies this space

        if i % 5 == 0:
            # 5-min major tick
            r_out = 114
            r_in = 104
            draw.line([(cx + r_in * cos_a, cy + r_in * sin_a),
                       (cx + r_out * cos_a, cy + r_out * sin_a)],
                      fill=COL_MAJOR_TICK, width=3)
        else:
            # 1-min minor tick
            r_out = 114
            r_in = 109
            draw.line([(cx + r_in * cos_a, cy + r_in * sin_a),
                       (cx + r_out * cos_a, cy + r_out * sin_a)],
                      fill=COL_MINOR_TICK, width=1)

    # 12 o'clock Flieger Triangle & Two Dots
    tri_apex_y = cy - 114
    tri_base_y = cy - 99
    tri_w = 8
    draw.polygon([
        (cx, tri_apex_y),
        (cx - tri_w, tri_base_y),
        (cx + tri_w, tri_base_y)
    ], fill=COL_TRIANGLE, outline=COL_TRIANGLE)

    # Flanking dots
    dot_y = cy - 107
    draw.ellipse([cx - 14, dot_y - 2.5, cx - 9, dot_y + 2.5], fill=COL_TRIANGLE)
    draw.ellipse([cx + 9, dot_y - 2.5, cx + 14, dot_y + 2.5], fill=COL_TRIANGLE)

    # Arabic Numerals 1..11
    try:
        font_num = ImageFont.truetype("/usr/share/fonts/Adwaita/AdwaitaSans-Regular.ttf", 20)
        font_sub = ImageFont.truetype("/usr/share/fonts/Adwaita/AdwaitaSans-Regular.ttf", 9)
    except:
        font_num = ImageFont.load_default()
        font_sub = ImageFont.load_default()

    num_r = 83
    for h in range(1, 12):
        angle = math.radians(h * 30.0 - 90.0)
        nx = cx + num_r * math.cos(angle)
        ny = cy + num_r * math.sin(angle)
        txt = str(h)
        bbox = draw.textbbox((0, 0), txt, font=font_num)
        tw = bbox[2] - bbox[0]
        th = bbox[3] - bbox[1]
        draw.text((nx - tw / 2, ny - th / 2 - 1), txt, fill=COL_NUMERAL, font=font_num)

    # Inscriptions: "PILOT" and "PST" (No Date)
    bbox_p = draw.textbbox((0, 0), "PILOT", font=font_sub)
    draw.text((cx - (bbox_p[2] - bbox_p[0])/2, cy - 42), "PILOT", fill=COL_TEXT_PILOT, font=font_sub)

    bbox_t = draw.textbbox((0, 0), "PST", font=font_sub)
    draw.text((cx - (bbox_t[2] - bbox_t[0])/2, cy + 34), "PST", fill=COL_TEXT_PST, font=font_sub)

    # Hands at 10:08:42
    h_deg = (10 + 8/60 + 42/3600) * 30.0
    m_deg = (8 + 42/60) * 6.0
    s_deg = 42 * 6.0

    def draw_hand(angle_deg, tip_len, shoulder_dist, max_w, root_w, tail_len):
        rad = math.radians(angle_deg - 90.0)
        cos_a = math.cos(rad)
        sin_a = math.sin(rad)
        px = -sin_a
        py = cos_a

        # Outer border
        b_max_w = max_w + 1.2
        b_root_w = root_w + 1.0
        b_tail = tail_len + 1.0
        b_tip = tip_len + 1.0
        border_pts = [
            (cx + b_tip * cos_a, cy + b_tip * sin_a),
            (cx + shoulder_dist * cos_a + b_max_w * px, cy + shoulder_dist * sin_a + b_max_w * py),
            (cx + 6 * cos_a + b_root_w * px, cy + 6 * sin_a + b_root_w * py),
            (cx - b_tail * cos_a, cy - b_tail * sin_a),
            (cx + 6 * cos_a - b_root_w * px, cy + 6 * sin_a - b_root_w * py),
            (cx + shoulder_dist * cos_a - b_max_w * px, cy + shoulder_dist * sin_a - b_max_w * py)
        ]
        draw.polygon(border_pts, fill=COL_HAND_BORDER)

        # Inner fill
        inner_pts = [
            (cx + tip_len * cos_a, cy + tip_len * sin_a),
            (cx + shoulder_dist * cos_a + max_w * px, cy + shoulder_dist * sin_a + max_w * py),
            (cx + 6 * cos_a + root_w * px, cy + 6 * sin_a + root_w * py),
            (cx - tail_len * cos_a, cy - tail_len * sin_a),
            (cx + 6 * cos_a - root_w * px, cy + 6 * sin_a - root_w * py),
            (cx + shoulder_dist * cos_a - max_w * px, cy + shoulder_dist * sin_a - max_w * py)
        ]
        draw.polygon(inner_pts, fill=COL_HAND_FILL)

    # Draw Hour hand
    draw_hand(h_deg, tip_len=62, shoulder_dist=32, max_w=4.5, root_w=2.0, tail_len=11)

    # Draw Minute hand
    draw_hand(m_deg, tip_len=98, shoulder_dist=54, max_w=3.8, root_w=1.8, tail_len=14)

    # Draw Second Hand
    s_rad = math.radians(s_deg - 90.0)
    sx_tip = cx + 111 * math.cos(s_rad)
    sy_tip = cy + 111 * math.sin(s_rad)
    sx_tail = cx - 22 * math.cos(s_rad)
    sy_tail = cy - 22 * math.sin(s_rad)
    pip_x = cx - 14 * math.cos(s_rad)
    pip_y = cy - 14 * math.sin(s_rad)

    # Pip border + fill
    draw.ellipse([pip_x - 3.5, pip_y - 3.5, pip_x + 3.5, pip_y + 3.5], fill=COL_HAND_BORDER)
    draw.ellipse([pip_x - 2.5, pip_y - 2.5, pip_x + 2.5, pip_y + 2.5], fill=COL_SEC_PIP)

    # Needle
    draw.line([(sx_tail, sy_tail), (sx_tip, sy_tip)], fill=COL_SEC_HAND, width=1)

    # Center Cap
    draw.ellipse([cx - 4.5, cy - 4.5, cx + 4.5, cy + 4.5], fill=(30, 32, 36), outline=(12, 12, 15))
    draw.ellipse([cx - 2.0, cy - 2.0, cx + 2.0, cy + 2.0], fill=(200, 205, 210))

    name = "lume" if lume_mode else "day"
    out_path = f"/home/stefan/Projects/ScoreTracker/tools/preview_bp_{name}.png"
    img.save(out_path)
    print(f"Saved {out_path}")

render_dial(lume_mode=False)
render_dial(lume_mode=True)
