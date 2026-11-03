import math
from PIL import Image, ImageDraw, ImageFont

def render_gmt_watch(variant="pepsi"):
    W, H = 240, 240
    cx, cy = 120, 120
    img = Image.new("RGBA", (W, H), (0, 0, 0, 255))
    draw = ImageDraw.Draw(img)

    # 1. Bezel colors
    # Top half (18:00 to 06:00 = 9 o'clock to 3 o'clock via 12): Night
    # Bottom half (06:00 to 18:00 = 3 o'clock to 9 o'clock via 6): Day
    if variant == "pepsi":
        # Classic Pepsi GMT: Top Deep Blue, Bottom Crimson Red
        COL_BEZEL_NIGHT = (14, 28, 64)   # Deep Navy
        COL_BEZEL_DAY   = (140, 20, 30)  # Crimson Red
        COL_GMT_HAND    = (235, 40, 35)  # Bright Red
    elif variant == "batman":
        # Batman GMT: Top Black, Bottom Royal Blue
        COL_BEZEL_NIGHT = (15, 16, 18)   # Matte Black
        COL_BEZEL_DAY   = (15, 55, 125)  # Royal Blue
        COL_GMT_HAND    = (50, 140, 255) # Electric Blue
    elif variant == "coke":
        # Coke GMT: Top Black, Bottom Crimson Red
        COL_BEZEL_NIGHT = (15, 16, 18)   # Matte Black
        COL_BEZEL_DAY   = (145, 22, 28)  # Rich Red
        COL_GMT_HAND    = (240, 45, 35)  # Bright Red
    else: # "pilot" / "stealth"
        # Pilot GMT: Top Pitch Black, Bottom Deep Slate Charcoal
        COL_BEZEL_NIGHT = (10, 12, 14)
        COL_BEZEL_DAY   = (38, 42, 48)
        COL_GMT_HAND    = (255, 100, 0)  # Aviation Orange

    COL_DIAL = (8, 9, 10)
    COL_WHITE = (250, 252, 255)
    COL_GRAY_TICK = (160, 165, 172)
    COL_BEZEL_TEXT = (255, 255, 255)
    COL_PILOT = (130, 135, 140)
    COL_GMT_TEXT = COL_GMT_HAND

    # 2. Draw 24-Hour Bezel Outer Ring (r=104 to 120)
    # The split line is horizontal: 9 o'clock (-180 / +180 deg) to 3 o'clock (0 deg)
    # Top arc (180 to 360 deg, or -180 to 0): Night
    # Bottom arc (0 to 180 deg): Day
    # In PIL: arc angles are measured clockwise from 3 o'clock!
    # 0 is 3 o'clock, 90 is 6 o'clock, 180 is 9 o'clock, 270 is 12 o'clock.
    # Day (3 o'clock to 9 o'clock): [0, 180]
    # Night (9 o'clock to 3 o'clock): [180, 360]
    draw.pieslice([1, 1, 239, 239], 180, 360, fill=COL_BEZEL_NIGHT)
    draw.pieslice([1, 1, 239, 239], 0, 180, fill=COL_BEZEL_DAY)

    # Dial inner mask (r = 103)
    r_dial = 103
    draw.ellipse([cx - r_dial, cy - r_dial, cx + r_dial, cy + r_dial], fill=COL_DIAL)

    # Bezel divider rings (crisp steel lines)
    draw.ellipse([1, 1, 239, 239], outline=(50, 54, 60), width=1)
    draw.ellipse([cx - r_dial, cy - r_dial, cx + r_dial, cy + r_dial], outline=(40, 44, 50), width=1)

    # Bezel 24-Hour Markings:
    # 24 hours around 360 degrees = 15 deg per hour.
    # 24 (or triangle) is at 12 o'clock (angle = -90 deg / 270 deg)
    # 6 is at 3 o'clock (angle = 0 deg)
    # 12 is at 6 o'clock (angle = 90 deg)
    # 18 is at 9 o'clock (angle = 180 deg)
    # Numerals at even hours: 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22. At 24: Triangle!
    # Dots/ticks at odd hours: 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23.
    try:
        font_bezel = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 9)
        font_num = ImageFont.truetype("/usr/share/fonts/truetype/freefont/FreeSansBold.ttf", 15)
        font_label = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 8)
    except:
        font_bezel = ImageFont.load_default()
        font_num = ImageFont.load_default()
        font_label = ImageFont.load_default()

    r_bezel_mid = 111
    for h24 in range(24):
        deg = h24 * 15.0 - 90.0
        rad = math.radians(deg)
        cos_a = math.cos(rad)
        sin_a = math.sin(rad)

        if h24 == 0:
            # Inverted triangle at 24/0
            tri_rad = math.radians(-90.0)
            t_cx = cx + r_bezel_mid * math.cos(tri_rad)
            t_cy = cy + r_bezel_mid * math.sin(tri_rad)
            draw.polygon([(t_cx, t_cy + 4), (t_cx - 4, t_cy - 4), (t_cx + 4, t_cy - 4)], fill=COL_WHITE)
        elif h24 % 2 == 0:
            # Even hour numerals
            txt = str(h24)
            tx = cx + r_bezel_mid * cos_a
            ty = cy + r_bezel_mid * sin_a
            bbox = font_bezel.getbbox(txt)
            tw = bbox[2] - bbox[0]
            th = bbox[3] - bbox[1]
            draw.text((tx - tw / 2, ty - th / 2 - 1), txt, font=font_bezel, fill=COL_BEZEL_TEXT)
        else:
            # Odd hour pip / dot
            px = cx + r_bezel_mid * cos_a
            py = cy + r_bezel_mid * sin_a
            draw.ellipse([px - 1.5, py - 1.5, px + 1.5, py + 1.5], fill=COL_WHITE)

    # 3. Big Pilot Minute/Second Chapter Ring inside the Bezel (r=95 to 102)
    for i in range(60):
        deg = i * 6.0 - 90.0
        if i == 0:
            continue
        rad = math.radians(deg)
        cos_a = math.cos(rad)
        sin_a = math.sin(rad)
        if i % 5 == 0:
            r_out = 101
            r_in = 93
            draw.line([(cx + r_in * cos_a, cy + r_in * sin_a),
                       (cx + r_out * cos_a, cy + r_out * sin_a)], fill=COL_WHITE, width=2)
        else:
            r_out = 101
            r_in = 97
            draw.line([(cx + r_in * cos_a, cy + r_in * sin_a),
                       (cx + r_out * cos_a, cy + r_out * sin_a)], fill=COL_GRAY_TICK, width=1)

    # 12 o'clock Flieger Triangle & Two Dots
    tri_apex_y = cy - 100
    tri_base_y = cy - 88
    draw.polygon([(cx, tri_apex_y), (cx - 7, tri_base_y), (cx + 7, tri_base_y)], fill=COL_WHITE)
    draw.ellipse([cx - 11 - 2, cy - 94 - 2, cx - 11 + 2, cy - 94 + 2], fill=COL_WHITE)
    draw.ellipse([cx + 11 - 2, cy - 94 - 2, cx + 11 + 2, cy - 94 + 2], fill=COL_WHITE)

    # 4. Dial Arabic Numerals 1..11 (r = 74)
    r_num = 74
    for h in range(1, 12):
        deg = h * 30.0 - 90.0
        rad = math.radians(deg)
        tx = cx + r_num * math.cos(rad)
        ty = cy + r_num * math.sin(rad)
        txt = str(h)
        bbox = font_num.getbbox(txt)
        tw = bbox[2] - bbox[0]
        th = bbox[3] - bbox[1]
        draw.text((tx - tw / 2, ty - th / 2 - 1), txt, font=font_num, fill=COL_WHITE)

    # Dial Inscriptions: "PILOT" + "GMT"
    draw.text((cx - 15, cy - 42), "PILOT", font=font_label, fill=COL_PILOT)
    draw.text((cx + 12, cy - 42), "GMT", font=font_label, fill=COL_GMT_TEXT)
    draw.text((cx - 10, cy + 34), "PST", font=font_label, fill=(170, 175, 180))
    draw.ellipse([cx - 1.5, cy + 44 - 1.5, cx + 1.5, cy + 44 + 1.5], fill=(0, 200, 80)) # Green sync dot

    # 5. HANDS RENDERING
    # Let's show time: 10:10:35 PST
    # For PST = 10:10, UTC is PST + 7 = 17:10 (or 5:10 PM UTC)
    # GMT Hand at 17:10: angle = (17 + 10/60) * 15 - 90 = 257.5 - 90 = 167.5 deg
    hour_angle = (10 + 10/60.0) * 30.0
    min_angle  = 10.0 * 6.0
    sec_angle  = 35.0 * 6.0
    gmt_angle  = (17.0 + 10.0/60.0) * 15.0 # points towards 17 on the 24h bezel!

    # --- Draw GMT Hand (under standard hands) ---
    g_rad = math.radians(gmt_angle - 90.0)
    g_cos = math.cos(g_rad)
    g_sin = math.sin(g_rad)
    g_nx = -g_sin
    g_ny = g_cos
    # GMT Hand stem (slender red needle from center out to r=88)
    draw.line([(cx - 14 * g_cos, cy - 14 * g_sin),
               (cx + 88 * g_cos, cy + 88 * g_sin)], fill=COL_GMT_HAND, width=2)
    # GMT Arrowhead (from r=88 to r=104, with luminous white infill!)
    tip_pt = (cx + 104 * g_cos, cy + 104 * g_sin)
    base_l = (cx + 86 * g_cos + 5.5 * g_nx, cy + 86 * g_sin + 5.5 * g_ny)
    base_r = (cx + 86 * g_cos - 5.5 * g_nx, cy + 86 * g_sin - 5.5 * g_ny)
    draw.polygon([tip_pt, base_l, base_r], fill=COL_GMT_HAND)
    # Luminous white insert inside arrowhead
    tip_in  = (cx + 100 * g_cos, cy + 100 * g_sin)
    base_in_l = (cx + 88 * g_cos + 3.2 * g_nx, cy + 88 * g_sin + 3.2 * g_ny)
    base_in_r = (cx + 88 * g_cos - 3.2 * g_nx, cy + 88 * g_sin - 3.2 * g_ny)
    draw.polygon([tip_in, base_in_l, base_in_r], fill=(255, 255, 255))

    # --- Draw Hour Hand ---
    h_rad = math.radians(hour_angle - 90.0)
    h_cos = math.cos(h_rad)
    h_sin = math.sin(h_rad)
    h_nx = -h_sin
    h_ny = h_cos
    # Lozenge polygon
    h_pts = [
        (cx + 56 * h_cos, cy + 56 * h_sin),
        (cx + 28 * h_cos + 4.2 * h_nx, cy + 28 * h_sin + 4.2 * h_ny),
        (cx + 6 * h_cos + 1.8 * h_nx, cy + 6 * h_sin + 1.8 * h_ny),
        (cx - 10 * h_cos, cy - 10 * h_sin),
        (cx + 6 * h_cos - 1.8 * h_nx, cy + 6 * h_sin - 1.8 * h_ny),
        (cx + 28 * h_cos - 4.2 * h_nx, cy + 28 * h_sin - 4.2 * h_ny),
    ]
    draw.polygon(h_pts, fill=COL_WHITE, outline=(0, 0, 0))

    # --- Draw Minute Hand ---
    m_rad = math.radians(min_angle - 90.0)
    m_cos = math.cos(m_rad)
    m_sin = math.sin(m_rad)
    m_nx = -m_sin
    m_ny = m_cos
    m_pts = [
        (cx + 90 * m_cos, cy + 90 * m_sin),
        (cx + 50 * m_cos + 3.5 * m_nx, cy + 50 * m_sin + 3.5 * m_ny),
        (cx + 6 * m_cos + 1.6 * m_nx, cy + 6 * m_sin + 1.6 * m_ny),
        (cx - 12 * m_cos, cy - 12 * m_sin),
        (cx + 6 * m_cos - 1.6 * m_nx, cy + 6 * m_sin - 1.6 * m_ny),
        (cx + 50 * m_cos - 3.5 * m_nx, cy + 50 * m_sin - 3.5 * m_ny),
    ]
    draw.polygon(m_pts, fill=COL_WHITE, outline=(0, 0, 0))

    # --- Draw Second Hand ---
    s_rad = math.radians(sec_angle - 90.0)
    s_cos = math.cos(s_rad)
    s_sin = math.sin(s_rad)
    # Needle
    draw.line([(cx - 20 * s_cos, cy - 20 * s_sin),
               (cx + 88 * s_cos, cy + 88 * s_sin)], fill=COL_WHITE, width=1)
    # Red tip
    draw.line([(cx + 88 * s_cos, cy + 88 * s_sin),
               (cx + 102 * s_cos, cy + 102 * s_sin)], fill=(240, 50, 40), width=2)
    # Pip
    pip_x = cx - 13 * s_cos
    pip_y = cy - 13 * s_sin
    draw.ellipse([pip_x - 3, pip_y - 3, pip_x + 3, pip_y + 3], fill=COL_WHITE, outline=(0, 0, 0))

    # Pinion Cap
    draw.ellipse([cx - 4.5, cy - 4.5, cx + 4.5, cy + 4.5], fill=(30, 32, 36))
    draw.ellipse([cx - 2, cy - 2, cx + 2, cy + 2], fill=(210, 215, 220))

    out_file = f"tools/preview_gmt_{variant}.png"
    img.save(out_file)
    print(f"Saved {out_file}")

render_gmt_watch("pepsi")
render_gmt_watch("batman")
render_gmt_watch("coke")
render_gmt_watch("pilot")
