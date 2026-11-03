import math
from PIL import Image, ImageDraw, ImageFont

def render_preview():
    W, H = 240, 240
    cx, cy = 120, 120
    img = Image.new("RGBA", (W, H), (0, 0, 0, 255))
    draw = ImageDraw.Draw(img)

    # Palette: Coke GMT Pilot
    COL_BEZEL_NIGHT = (14, 15, 17)   # Matte Black
    COL_BEZEL_DAY   = (142, 22, 28)  # Deep Crimson Red
    COL_GMT_HAND    = (235, 45, 35)  # Bright Aviation Red
    COL_DIAL        = (8, 9, 10)
    COL_BEZEL_RIM   = (45, 48, 54)
    COL_WHITE       = (250, 252, 255)
    COL_GRAY_TICK   = (160, 165, 172)
    COL_PILOT       = (130, 135, 140)

    # 1. Bezel Arc
    draw.pieslice([1, 1, 239, 239], 180, 360, fill=COL_BEZEL_NIGHT)
    draw.pieslice([1, 1, 239, 239], 0, 180, fill=COL_BEZEL_DAY)

    # Inner dial mask
    r_dial = 103
    draw.ellipse([cx - r_dial, cy - r_dial, cx + r_dial, cy + r_dial], fill=COL_DIAL)

    # Bezel borders
    draw.ellipse([1, 1, 239, 239], outline=COL_BEZEL_RIM, width=1)
    draw.ellipse([cx - r_dial, cy - r_dial, cx + r_dial, cy + r_dial], outline=COL_BEZEL_RIM, width=1)

    # 2. Bezel 24h Markings
    try:
        font_bezel = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 9)
        font_num = ImageFont.truetype("/usr/share/fonts/truetype/freefont/FreeSansBold.ttf", 15)
        font_label = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 8)
    except:
        font_bezel = ImageFont.load_default()
        font_num = ImageFont.load_default()
        font_label = ImageFont.load_default()

    r_bezel_mid = 111.5
    for h24 in range(24):
        deg = h24 * 15.0 - 90.0
        rad = math.radians(deg)
        cos_a = math.cos(rad)
        sin_a = math.sin(rad)

        if h24 == 0:
            # Inverted triangle at 24/0 (base outer, apex inner)
            t_apex = (cx + 105 * cos_a, cy + 105 * sin_a)
            t_base_l = (cx + 117 * cos_a - 4.5 * sin_a, cy + 117 * sin_a + 4.5 * cos_a)
            t_base_r = (cx + 117 * cos_a + 4.5 * sin_a, cy + 117 * sin_a - 4.5 * cos_a)
            draw.polygon([t_apex, t_base_l, t_base_r], fill=COL_WHITE)
        elif h24 % 2 == 0:
            txt = str(h24)
            tx = cx + r_bezel_mid * cos_a
            ty = cy + r_bezel_mid * sin_a
            bbox = font_bezel.getbbox(txt)
            tw = bbox[2] - bbox[0]
            th = bbox[3] - bbox[1]
            draw.text((tx - tw / 2, ty - th / 2 - 1), txt, font=font_bezel, fill=COL_WHITE)
        else:
            px = cx + r_bezel_mid * cos_a
            py = cy + r_bezel_mid * sin_a
            draw.ellipse([px - 1.2, py - 1.2, px + 1.2, py + 1.2], fill=COL_WHITE)

    # 3. 60-Minute Chapter Track (r=94 to 101)
    for i in range(60):
        if i == 0: continue
        deg = i * 6.0 - 90.0
        rad = math.radians(deg)
        cos_a = math.cos(rad)
        sin_a = math.sin(rad)
        if i % 5 == 0:
            draw.line([(cx + 93 * cos_a, cy + 93 * sin_a),
                       (cx + 101 * cos_a, cy + 101 * sin_a)], fill=COL_WHITE, width=2)
        else:
            draw.line([(cx + 96 * cos_a, cy + 96 * sin_a),
                       (cx + 101 * cos_a, cy + 101 * sin_a)], fill=COL_GRAY_TICK, width=1)

    # 12 o'clock Flieger Triangle & Two Dots
    tri_apex_y = cy - 100
    tri_base_y = cy - 88
    draw.polygon([(cx, tri_apex_y), (cx - 7, tri_base_y), (cx + 7, tri_base_y)], fill=COL_WHITE)
    draw.ellipse([cx - 11 - 2, cy - 94 - 2, cx - 11 + 2, cy - 94 + 2], fill=COL_WHITE)
    draw.ellipse([cx + 11 - 2, cy - 94 - 2, cx + 11 + 2, cy - 94 + 2], fill=COL_WHITE)

    # 4. Dial Arabic Numerals 1..11 (r = 74)
    for h in range(1, 12):
        deg = h * 30.0 - 90.0
        rad = math.radians(deg)
        tx = cx + 74 * math.cos(rad)
        ty = cy + 74 * math.sin(rad)
        txt = str(h)
        bbox = font_num.getbbox(txt)
        tw = bbox[2] - bbox[0]
        th = bbox[3] - bbox[1]
        draw.text((tx - tw / 2, ty - th / 2 - 1), txt, font=font_num, fill=COL_WHITE)

    # Inscriptions: "PILOT" + "GMT"
    txt_pilot = "PILOT"
    txt_gmt = "GMT"
    bp = font_label.getbbox(txt_pilot)
    bg = font_label.getbbox(txt_gmt)
    wp = bp[2] - bp[0]
    wg = bg[2] - bg[0]
    tot_w = wp + 4 + wg
    draw.text((cx - tot_w / 2, cy - 42), txt_pilot, font=font_label, fill=COL_PILOT)
    draw.text((cx - tot_w / 2 + wp + 4, cy - 42), txt_gmt, font=font_label, fill=COL_GMT_HAND)

    draw.text((cx - 8, cy + 34), "PST", font=font_label, fill=(170, 175, 180))
    draw.ellipse([cx - 1.5, cy + 44 - 1.5, cx + 1.5, cy + 44 + 1.5], fill=(0, 200, 80))

    # 5. Hands: PST = 9:30, UTC = 16:30 (4:30 PM UTC)
    h_deg = (9 + 30/60.0) * 30.0
    m_deg = 30.0 * 6.0
    s_deg = 15.0 * 6.0
    gmt_deg = (16.0 + 30.0/60.0) * 15.0 # 247.5 deg

    # GMT Hand
    g_rad = math.radians(gmt_deg - 90.0)
    g_cos = math.cos(g_rad)
    g_sin = math.sin(g_rad)
    g_nx = -g_sin
    g_ny = g_cos
    draw.line([(cx - 14 * g_cos, cy - 14 * g_sin),
               (cx + 86 * g_cos, cy + 86 * g_sin)], fill=COL_GMT_HAND, width=2)
    tip_pt = (cx + 102 * g_cos, cy + 102 * g_sin)
    base_l = (cx + 84 * g_cos + 5.5 * g_nx, cy + 84 * g_sin + 5.5 * g_ny)
    base_r = (cx + 84 * g_cos - 5.5 * g_nx, cy + 84 * g_sin - 5.5 * g_ny)
    draw.polygon([tip_pt, base_l, base_r], fill=COL_GMT_HAND)
    tip_in  = (cx + 98 * g_cos, cy + 98 * g_sin)
    base_in_l = (cx + 86 * g_cos + 3.2 * g_nx, cy + 86 * g_sin + 3.2 * g_ny)
    base_in_r = (cx + 86 * g_cos - 3.2 * g_nx, cy + 86 * g_sin - 3.2 * g_ny)
    draw.polygon([tip_in, base_in_l, base_in_r], fill=(255, 255, 255))

    # Hour Hand
    h_rad = math.radians(h_deg - 90.0)
    h_cos = math.cos(h_rad)
    h_sin = math.sin(h_rad)
    h_nx = -h_sin
    h_ny = h_cos
    h_pts = [
        (cx + 56 * h_cos, cy + 56 * h_sin),
        (cx + 28 * h_cos + 4.2 * h_nx, cy + 28 * h_sin + 4.2 * h_ny),
        (cx + 6 * h_cos + 1.8 * h_nx, cy + 6 * h_sin + 1.8 * h_ny),
        (cx - 10 * h_cos, cy - 10 * h_sin),
        (cx + 6 * h_cos - 1.8 * h_nx, cy + 6 * h_sin - 1.8 * h_ny),
        (cx + 28 * h_cos - 4.2 * h_nx, cy + 28 * h_sin - 4.2 * h_ny),
    ]
    draw.polygon(h_pts, fill=COL_WHITE, outline=(0, 0, 0))

    # Minute Hand
    m_rad = math.radians(m_deg - 90.0)
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

    # Garfield sleeping on hour hand
    gar_img = Image.open("tools/garfield_28_sleep.png").convert("RGBA")
    gar_rot = gar_img.rotate(h_deg - 90.0, resample=Image.NEAREST, expand=True)
    gx = int(cx + 34 * h_cos - gar_rot.width / 2)
    gy = int(cy + 34 * h_sin - gar_rot.height / 2)
    img.paste(gar_rot, (gx, gy), gar_rot)

    # Pinion
    draw.ellipse([cx - 4.5, cy - 4.5, cx + 4.5, cy + 4.5], fill=(30, 32, 36))
    draw.ellipse([cx - 2, cy - 2, cx + 2, cy + 2], fill=(210, 215, 220))

    # Seconds Hand
    s_rad = math.radians(s_deg - 90.0)
    s_cos = math.cos(s_rad)
    s_sin = math.sin(s_rad)
    draw.line([(cx - 20 * s_cos, cy - 20 * s_sin),
               (cx + 88 * s_cos, cy + 88 * s_sin)], fill=COL_WHITE, width=1)
    draw.line([(cx + 88 * s_cos, cy + 88 * s_sin),
               (cx + 102 * s_cos, cy + 102 * s_sin)], fill=(240, 50, 40), width=2)
    pip_x = cx - 13 * s_cos
    pip_y = cy - 13 * s_sin
    draw.ellipse([pip_x - 3, pip_y - 3, pip_x + 3, pip_y + 3], fill=COL_WHITE, outline=(0, 0, 0))

    out_file = "tools/preview_gmt_full.png"
    img.save(out_file)
    print(f"Saved {out_file}")

render_preview()
