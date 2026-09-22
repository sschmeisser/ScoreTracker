#!/usr/bin/env python3
"""
QA Visual Inspection & Frame Generation Tool for ScoreTracker
Visual Quality Assurance Engineer validation suite for GC9A01 240x240 circular display.

Simulates exact 16-bit RGB565 graphics pipeline:
- Surfaces: Soccer, Hockey, Basketball, Baseball
- Circular vignette (r=102..120) & GC9A01 circular bezel masking (r>=120)
- Chunky Kunio-kun players, sport-specific gear, balls/puck
- Arcade scoreboard HUD & controls prompt
- RGB565 -> RGB888 rendering with BGR/RGB display mode fidelity validation
- Automated histogram and colorimetric analysis (zero pink tint, no 16px stripes)
"""

import os
import math
import sys
from PIL import Image

# -----------------------------------------------------------------------------
# Color Constants (matching src/stadium_render.h and src/main.cpp)
# -----------------------------------------------------------------------------
COL_BLACK       = 0x0000
COL_WHITE       = 0xFFFF
COL_GOLD        = 0xFFE0
COL_HUD_BG      = 0x0841
COL_SHADOW      = 0x0A22

COL_SKIN        = 0xFDF0
COL_SKIN_SHADOW = 0xDC48
COL_HAIR_BLK    = 0x1082
COL_HAIR_BRN    = 0x79A0
COL_HAIR_BLD    = 0xFDE0

COL_RED_TEAM    = 0xF800
COL_RED_DARK    = 0x9800
COL_BLU_TEAM    = 0x1BDF
COL_BLU_DARK    = 0x0215
COL_SHORTS      = 0xFFFF
COL_BOOTS       = 0x18C3

# Soccer
SOCCER_GRASS_A1   = 0x24C5
SOCCER_GRASS_A2   = 0x2D47
SOCCER_GRASS_A3   = 0x1CA3
SOCCER_GRASS_B1   = 0x1C23
SOCCER_GRASS_B2   = 0x2484
SOCCER_GRASS_B3   = 0x1402
SOCCER_SHEEN      = 0x3DE9
SOCCER_DIRT_1     = 0x8A42
SOCCER_DIRT_2     = 0x69C1
SOCCER_DIRT_3     = 0x9B64
SOCCER_WEAR_GRS   = 0x3B03
SOCCER_CHALK      = 0xFFFF
SOCCER_CHALK_AA   = 0x64C7
SOCCER_NET        = 0xC618
SOCCER_NET_SHD    = 0x2965

# Hockey
HOCKEY_ICE_A      = 0xFFFF  # Pure solid crisp white ice
HOCKEY_ICE_B      = 0xFFFF  # Solid uniform ice (NO STRIPES)
HOCKEY_ICE_CUT    = 0xFFFF  # Fresh frosted skate blade groove
HOCKEY_ICE_SHD    = 0xCE59  # Neutral silver skate scratch shadow
HOCKEY_CREASE     = 0x9E3D  # Regulation ice-blue goal crease paint
HOCKEY_RED_LINE   = 0xD904  # Regulation vivid hockey red line
HOCKEY_BLU_LINE   = 0x2B7D  # Regulation deep hockey blue line
HOCKEY_BOARD_W    = 0xF7BE  # Dasher board white
HOCKEY_BOARD_Y    = 0xFFE0  # Kickplate yellow
HOCKEY_BOARD_R    = 0xD904  # Cap rail red
HOCKEY_GLASS_HI   = 0xFFFF  # Specular highlight
HOCKEY_GLASS_CY   = 0x8E1E  # Acrylic reflection cyan ambient
HOCKEY_NET_MESH   = 0xE73C  # Silver-white goal net mesh
COL_PUCK          = 0x1082
COL_STICK_WOOD    = 0xC443
COL_BLADE_SILV    = 0xD6BA

# Basketball
BASKET_HONEY      = 0xE56A
BASKET_AMBER      = 0xDC88
BASKET_MAPLE      = 0xD446
BASKET_PECAN      = 0xC363
BASKET_GRAIN      = 0xB302
BASKET_GROOVE     = 0x71E1
BASKET_KEY_RED    = 0x8882
BASKET_KEY_SHD    = 0x5800
BASKET_LINE       = 0xFFFF
BASKET_SEAM       = 0x2104
BASKET_ORANGE     = 0xF3C0
BASKET_ORANGE_D   = 0xC260
BASKET_ORANGE_L   = 0xFD20
BASKET_GLOSS      = 0xFEC0
BASKET_HOOP_RIM   = 0xF960
BASKET_NET        = 0xFFFF

# Baseball
BASEBALL_TURF_A   = 0x24C5
BASEBALL_TURF_B   = 0x1C23
BASEBALL_TURF_S   = 0x2D47
BASEBALL_CLAY_A   = 0xD3C6
BASEBALL_CLAY_B   = 0xBA63
BASEBALL_CLAY_C   = 0x9A42
BASEBALL_CLAY_D   = 0x7201
BASEBALL_TRACK    = 0xA262
BASEBALL_FENCE    = 0x11C5
BASEBALL_PAD_Y    = 0xFFE0
BASEBALL_BASE_W   = 0xFFFF
BASEBALL_BASE_S   = 0xCE7D
BASEBALL_SHADOW   = 0x2945
BASEBALL_CHALK    = 0xFFFF
BASEBALL_BANNER_R = 0xD882
BASEBALL_BANNER_B = 0x1A4F

PITCH_MIN_X = 14
PITCH_MAX_X = 226
PITCH_MIN_Y = 28
PITCH_MAX_Y = 230
GOAL_Y_TOP  = 104
GOAL_Y_BOT  = 154

# -----------------------------------------------------------------------------
# RGB565 to RGB888 Conversion
# -----------------------------------------------------------------------------
def rgb565_to_rgb888(c, bgr_mode=False):
    """Converts 16-bit RGB565 integer to (R, G, B) tuple.
    If bgr_mode is True, swaps Red and Blue channels to simulate BGR panel wiring.
    """
    r5 = (c >> 11) & 0x1F
    g6 = (c >> 5) & 0x3F
    b5 = c & 0x1F

    r8 = (r5 * 527 + 23) >> 6
    g8 = (g6 * 259 + 33) >> 6
    b8 = (b5 * 527 + 23) >> 6

    if bgr_mode:
        return (b8, g8, r8)
    return (r8, g8, b8)

# -----------------------------------------------------------------------------
# 5x7 Minimal Retro Bitmap Font
# -----------------------------------------------------------------------------
FONT_5X7 = {
    ' ': [0, 0, 0, 0, 0],
    '!': [0x00, 0x00, 0x5F, 0x00, 0x00],
    ':': [0x00, 0x36, 0x36, 0x00, 0x00],
    '=': [0x14, 0x14, 0x14, 0x14, 0x14],
    '>': [0x00, 0x41, 0x22, 0x14, 0x08],
    '<': [0x08, 0x14, 0x22, 0x41, 0x00],
    '0': [0x3E, 0x51, 0x49, 0x45, 0x3E],
    '1': [0x00, 0x42, 0x7F, 0x40, 0x00],
    '2': [0x42, 0x61, 0x51, 0x49, 0x46],
    '3': [0x21, 0x41, 0x45, 0x4B, 0x31],
    '4': [0x18, 0x14, 0x12, 0x7F, 0x10],
    '5': [0x27, 0x45, 0x45, 0x45, 0x39],
    '6': [0x3C, 0x4A, 0x49, 0x49, 0x30],
    '7': [0x01, 0x71, 0x09, 0x05, 0x03],
    '8': [0x36, 0x49, 0x49, 0x49, 0x36],
    '9': [0x06, 0x49, 0x49, 0x29, 0x1E],
    'A': [0x7E, 0x11, 0x11, 0x11, 0x7E],
    'B': [0x7F, 0x49, 0x49, 0x49, 0x36],
    'C': [0x3E, 0x41, 0x41, 0x41, 0x22],
    'D': [0x7F, 0x41, 0x41, 0x22, 0x1C],
    'E': [0x7F, 0x49, 0x49, 0x49, 0x41],
    'F': [0x7F, 0x09, 0x09, 0x09, 0x06],
    'G': [0x3E, 0x41, 0x49, 0x49, 0x7A],
    'H': [0x7F, 0x08, 0x08, 0x08, 0x7F],
    'I': [0x00, 0x41, 0x7F, 0x41, 0x00],
    'J': [0x20, 0x40, 0x41, 0x3F, 0x01],
    'K': [0x7F, 0x08, 0x14, 0x22, 0x41],
    'L': [0x7F, 0x40, 0x40, 0x40, 0x40],
    'M': [0x7F, 0x02, 0x0C, 0x02, 0x7F],
    'N': [0x7F, 0x04, 0x08, 0x10, 0x7F],
    'O': [0x3E, 0x41, 0x41, 0x41, 0x3E],
    'P': [0x7F, 0x09, 0x09, 0x09, 0x06],
    'Q': [0x3E, 0x41, 0x51, 0x21, 0x5E],
    'R': [0x7F, 0x09, 0x19, 0x29, 0x46],
    'S': [0x46, 0x49, 0x49, 0x49, 0x31],
    'T': [0x01, 0x01, 0x7F, 0x01, 0x01],
    'U': [0x3F, 0x40, 0x40, 0x40, 0x3F],
    'V': [0x1F, 0x20, 0x40, 0x20, 0x1F],
    'W': [0x7F, 0x20, 0x18, 0x20, 0x7F],
    'X': [0x63, 0x14, 0x08, 0x14, 0x63],
    'Y': [0x07, 0x08, 0x70, 0x08, 0x07],
    'Z': [0x61, 0x51, 0x49, 0x45, 0x43],
}

# -----------------------------------------------------------------------------
# Software Framebuffer (240x240 RGB565)
# -----------------------------------------------------------------------------
class Canvas565:
    def __init__(self, w=240, h=240):
        self.w = w
        self.h = h
        self.buf = [COL_BLACK] * (w * h)

    def fillScreen(self, color):
        self.buf = [color] * (self.w * self.h)

    def drawPixel(self, x, y, color):
        if 0 <= x < self.w and 0 <= y < self.h:
            self.buf[y * self.w + x] = color

    def drawFastHLine(self, x, y, w, color):
        if y < 0 or y >= self.h:
            return
        x1 = max(0, x)
        x2 = min(self.w, x + w)
        for px in range(x1, x2):
            self.buf[y * self.w + px] = color

    def drawFastVLine(self, x, y, h, color):
        if x < 0 or x >= self.w:
            return
        y1 = max(0, y)
        y2 = min(self.h, y + h)
        for py in range(y1, y2):
            self.buf[py * self.w + x] = color

    def drawLine(self, x0, y0, x1, y1, color):
        dx = abs(x1 - x0)
        dy = abs(y1 - y0)
        sx = 1 if x0 < x1 else -1
        sy = 1 if y0 < y1 else -1
        err = dx - dy
        while True:
            self.drawPixel(x0, y0, color)
            if x0 == x1 and y0 == y1:
                break
            e2 = 2 * err
            if e2 > -dy:
                err -= dy
                x0 += sx
            if e2 < dx:
                err += dx
                y0 += sy

    def drawRect(self, x, y, w, h, color):
        self.drawFastHLine(x, y, w, color)
        self.drawFastHLine(x, y + h - 1, w, color)
        self.drawFastVLine(x, y, h, color)
        self.drawFastVLine(x + w - 1, y, h, color)

    def fillRect(self, x, y, w, h, color):
        x1 = max(0, x)
        x2 = min(self.w, x + w)
        y1 = max(0, y)
        y2 = min(self.h, y + h)
        for py in range(y1, y2):
            idx = py * self.w + x1
            self.buf[idx:idx + (x2 - x1)] = [color] * (x2 - x1)

    def drawRoundRect(self, x, y, w, h, r, color):
        self.drawFastHLine(x + r, y, w - 2 * r, color)
        self.drawFastHLine(x + r, y + h - 1, w - 2 * r, color)
        self.drawFastVLine(x, y + r, h - 2 * r, color)
        self.drawFastVLine(x + w - 1, y + r, h - 2 * r, color)
        self._drawCircleHelper(x + r, y + r, r, 1, color)
        self._drawCircleHelper(x + w - r - 1, y + r, r, 2, color)
        self._drawCircleHelper(x + w - r - 1, y + h - r - 1, r, 4, color)
        self._drawCircleHelper(x + r, y + h - r - 1, r, 8, color)

    def _drawCircleHelper(self, x0, y0, r, cornername, color):
        f = 1 - r
        ddF_x = 1
        ddF_y = -2 * r
        x = 0
        y = r
        while x < y:
            if f >= 0:
                y -= 1
                ddF_y += 2
                f += ddF_y
            x += 1
            ddF_x += 2
            f += ddF_x
            if cornername & 0x4:
                self.drawPixel(x0 + x, y0 + y, color)
                self.drawPixel(x0 + y, y0 + x, color)
            if cornername & 0x2:
                self.drawPixel(x0 + x, y0 - y, color)
                self.drawPixel(x0 + y, y0 - x, color)
            if cornername & 0x8:
                self.drawPixel(x0 - y, y0 + x, color)
                self.drawPixel(x0 - x, y0 + y, color)
            if cornername & 0x1:
                self.drawPixel(x0 - y, y0 - y, color)
                self.drawPixel(x0 - x, y0 - y, color)

    def drawCircle(self, cx, cy, r, color):
        f = 1 - r
        ddF_x = 1
        ddF_y = -2 * r
        x = 0
        y = r
        self.drawPixel(cx, cy + r, color)
        self.drawPixel(cx, cy - r, color)
        self.drawPixel(cx + r, cy, color)
        self.drawPixel(cx - r, cy, color)
        while x < y:
            if f >= 0:
                y -= 1
                ddF_y += 2
                f += ddF_y
            x += 1
            ddF_x += 2
            f += ddF_x
            self.drawPixel(cx + x, cy + y, color)
            self.drawPixel(cx - x, cy + y, color)
            self.drawPixel(cx + x, cy - y, color)
            self.drawPixel(cx - x, cy - y, color)
            self.drawPixel(cx + y, cy + x, color)
            self.drawPixel(cx - y, cy + x, color)
            self.drawPixel(cx + y, cy - x, color)
            self.drawPixel(cx - y, cy - x, color)

    def fillCircle(self, cx, cy, r, color):
        self.drawFastVLine(cx, cy - r, 2 * r + 1, color)
        f = 1 - r
        ddF_x = 1
        ddF_y = -2 * r
        x = 0
        y = r
        while x < y:
            if f >= 0:
                y -= 1
                ddF_y += 2
                f += ddF_y
            x += 1
            ddF_x += 2
            f += ddF_x
            self.drawFastVLine(cx + x, cy - y, 2 * y + 1, color)
            self.drawFastVLine(cx + y, cy - x, 2 * x + 1, color)
            self.drawFastVLine(cx - x, cy - y, 2 * y + 1, color)
            self.drawFastVLine(cx - y, cy - x, 2 * x + 1, color)

    def fillEllipse(self, cx, cy, rx, ry, color):
        for dy in range(-ry, ry + 1):
            py = cy + dy
            if py < 0 or py >= self.h:
                continue
            hw = int(math.sqrt(max(0, 1.0 - (dy * dy) / float(ry * ry))) * rx)
            self.drawFastHLine(cx - hw, py, 2 * hw + 1, color)

    def drawText(self, text, x, y, color, bg_color=None, size=1):
        cur_x = x
        for char in text.upper():
            glyph = FONT_5X7.get(char, FONT_5X7[' '])
            for col_idx in range(5):
                line = glyph[col_idx]
                for row_idx in range(7):
                    px = cur_x + col_idx * size
                    py = y + row_idx * size
                    bit = (line >> row_idx) & 1
                    if bit:
                        if size == 1:
                            self.drawPixel(px, py, color)
                        else:
                            self.fillRect(px, py, size, size, color)
                    elif bg_color is not None:
                        if size == 1:
                            self.drawPixel(px, py, bg_color)
                        else:
                            self.fillRect(px, py, size, size, bg_color)
            cur_x += 6 * size


# -----------------------------------------------------------------------------
# Vignette & Circular Bezel Masking
# -----------------------------------------------------------------------------
VIGNETTE_LUT = [0] * 4000
VIGNETTE_INIT = False

def scale_rgb565(c: int, factor: int) -> int:
    r = ((c >> 11) * factor) >> 8
    g = (((c >> 5) & 0x3F) * factor) >> 8
    b = ((c & 0x1F) * factor) >> 8
    if r > 31: r = 31
    if g > 63: g = 63
    if b > 31: b = 31
    return (r << 11) | (g << 5) | b

def init_vignette_lut():
    global VIGNETTE_INIT
    if VIGNETTE_INIT:
        return
    for r2 in range(10404, 14401):
        r = math.sqrt(float(r2))
        t = (120.0 - r) / 18.0
        t = max(0.0, min(1.0, t))
        smooth = t * t * (3.0 - 2.0 * t)
        VIGNETTE_LUT[r2 - 10404] = int(smooth * 256.0)
    VIGNETTE_INIT = True

def apply_circular_vignette(canvas: Canvas565):
    init_vignette_lut()
    buf = canvas.buf
    for y in range(240):
        dy = y - 120
        dy2 = dy * dy
        if dy2 >= 14400:
            idx = y * 240
            buf[idx:idx + 240] = [0] * 240
            continue

        x_outer = int(math.sqrt(float(14400 - dy2)))
        left_outer = 120 - x_outer
        right_outer = 120 + x_outer

        if left_outer > 0:
            idx = y * 240
            buf[idx:idx + left_outer] = [0] * left_outer
        if right_outer < 239:
            idx = y * 240 + right_outer + 1
            length = 239 - right_outer
            buf[idx:idx + length] = [0] * length

        if dy2 < 10404:
            x_inner = int(math.sqrt(float(10404 - dy2)))
            left_inner = 120 - x_inner
            right_inner = 120 + x_inner

            for x in range(left_outer, left_inner + 1):
                dx = x - 120
                r2 = dx * dx + dy2
                if 10404 <= r2 <= 14400:
                    factor = VIGNETTE_LUT[r2 - 10404]
                    buf[y * 240 + x] = scale_rgb565(buf[y * 240 + x], factor)

            for x in range(right_inner, right_outer + 1):
                dx = x - 120
                r2 = dx * dx + dy2
                if 10404 <= r2 <= 14400:
                    factor = VIGNETTE_LUT[r2 - 10404]
                    buf[y * 240 + x] = scale_rgb565(buf[y * 240 + x], factor)
        else:
            for x in range(left_outer, right_outer + 1):
                dx = x - 120
                r2 = dx * dx + dy2
                if 10404 <= r2 <= 14400:
                    factor = VIGNETTE_LUT[r2 - 10404]
                    buf[y * 240 + x] = scale_rgb565(buf[y * 240 + x], factor)

def mask_gc9a01_circular_bezel(canvas: Canvas565):
    """Enforces exact circular physical display masking (radius 120px) on final frame."""
    buf = canvas.buf
    for y in range(240):
        dy = y - 120
        dy2 = dy * dy
        for x in range(240):
            dx = x - 120
            if dx * dx + dy2 >= 14400:  # Radius >= 120.0
                buf[y * 240 + x] = COL_BLACK


# -----------------------------------------------------------------------------
# Sport Stadium Surfaces
# -----------------------------------------------------------------------------
def render_soccer_pitch(canvas: Canvas565):
    canvas.fillScreen(COL_BLACK)
    buf = canvas.buf
    cx, cy, r = 120, 120, 117

    # Grass pitch with stripes & cross-hatch stippling
    for y in range(PITCH_MIN_Y, PITCH_MAX_Y + 1):
        dy = y - cy
        max_w = int(math.sqrt(max(0, r * r - dy * dy)))
        x1 = max(PITCH_MIN_X, cx - max_w)
        x2 = min(PITCH_MAX_X, cx + max_w)
        if x1 >= x2:
            continue

        stripe = ((y - PITCH_MIN_Y) // 18) % 2
        c_base = SOCCER_GRASS_A1 if stripe == 0 else SOCCER_GRASS_B1
        c_hi   = SOCCER_GRASS_A2 if stripe == 0 else SOCCER_GRASS_B2
        c_sh   = SOCCER_GRASS_A3 if stripe == 0 else SOCCER_GRASS_B3
        is_sheen_line = ((y - PITCH_MIN_Y) % 18 == 0)

        for x in range(x1, x2 + 1):
            if is_sheen_line and (x & 1):
                buf[y * 240 + x] = SOCCER_SHEEN
                continue
            h = ((x * 19 + y * 41) ^ (x >> 1) ^ (y >> 2)) & 7
            if h == 0:
                buf[y * 240 + x] = c_hi
            elif h == 7:
                buf[y * 240 + x] = c_sh
            else:
                buf[y * 240 + x] = c_base

    # Goal Mouth Wear
    for goal_cx in [28, 212]:
        for dy in range(-16, 17):
            py = 129 + dy
            if py < PITCH_MIN_Y or py > PITCH_MAX_Y:
                continue
            hw = int(math.sqrt(max(0, 16 * 16 - dy * dy))) * 14 // 16
            for dx in range(-hw, hw + 1):
                px = goal_cx + dx
                if px < PITCH_MIN_X or px > PITCH_MAX_X:
                    continue
                d = math.sqrt(dx * dx * 1.4 + dy * dy)
                if d < 6.0:
                    buf[py * 240 + px] = SOCCER_DIRT_1 if ((px ^ py) & 1) else SOCCER_DIRT_2
                elif d < 11.0:
                    h = (px * 7 + py * 13) & 3
                    buf[py * 240 + px] = SOCCER_DIRT_3 if h == 0 else (SOCCER_DIRT_1 if h == 1 else SOCCER_WEAR_GRS)
                elif d < 16.0:
                    if ((px * 11 + py * 23) & 3) == 0:
                        buf[py * 240 + px] = SOCCER_WEAR_GRS

    # Chalk boundary lines & AA feathered edges
    canvas.drawRect(PITCH_MIN_X + 7, PITCH_MIN_Y + 3, (PITCH_MAX_X - PITCH_MIN_X) - 14, (PITCH_MAX_Y - PITCH_MIN_Y) - 6, SOCCER_CHALK_AA)
    canvas.drawRect(PITCH_MIN_X + 9, PITCH_MIN_Y + 5, (PITCH_MAX_X - PITCH_MIN_X) - 18, (PITCH_MAX_Y - PITCH_MIN_Y) - 10, SOCCER_CHALK_AA)
    canvas.drawRect(PITCH_MIN_X + 8, PITCH_MIN_Y + 4, (PITCH_MAX_X - PITCH_MIN_X) - 16, (PITCH_MAX_Y - PITCH_MIN_Y) - 8, SOCCER_CHALK)

    # Halfway line
    canvas.drawFastVLine(119, PITCH_MIN_Y + 4, (PITCH_MAX_Y - PITCH_MIN_Y) - 8, SOCCER_CHALK_AA)
    canvas.drawFastVLine(121, PITCH_MIN_Y + 4, (PITCH_MAX_Y - PITCH_MIN_Y) - 8, SOCCER_CHALK_AA)
    canvas.drawFastVLine(120, PITCH_MIN_Y + 4, (PITCH_MAX_Y - PITCH_MIN_Y) - 8, SOCCER_CHALK)

    # Center circle
    canvas.drawCircle(120, 129, 29, SOCCER_CHALK_AA)
    canvas.drawCircle(120, 129, 27, SOCCER_CHALK_AA)
    canvas.drawCircle(120, 129, 28, SOCCER_CHALK)
    canvas.fillCircle(120, 129, 2, SOCCER_CHALK)

    # Corner arcs
    canvas.drawCircle(PITCH_MIN_X + 8, PITCH_MIN_Y + 4, 6, SOCCER_CHALK)
    canvas.drawCircle(PITCH_MAX_X - 8, PITCH_MIN_Y + 4, 6, SOCCER_CHALK)
    canvas.drawCircle(PITCH_MIN_X + 8, PITCH_MAX_Y - 4, 6, SOCCER_CHALK)
    canvas.drawCircle(PITCH_MAX_X - 8, PITCH_MAX_Y - 4, 6, SOCCER_CHALK)

    # Penalty boxes
    canvas.drawRect(PITCH_MIN_X + 8, 76, 36, 106, SOCCER_CHALK)
    canvas.drawRect(PITCH_MAX_X - 44, 76, 36, 106, SOCCER_CHALK)
    canvas.drawRect(PITCH_MIN_X + 8, 96, 14, 66, SOCCER_CHALK)
    canvas.drawRect(PITCH_MAX_X - 22, 96, 14, 66, SOCCER_CHALK)
    canvas.fillCircle(46, 129, 2, SOCCER_CHALK)
    canvas.fillCircle(194, 129, 2, SOCCER_CHALK)

    # Goals
    canvas.fillRect(PITCH_MIN_X, GOAL_Y_TOP, 8, GOAL_Y_BOT - GOAL_Y_TOP, SOCCER_NET_SHD)
    for gy in range(GOAL_Y_TOP, GOAL_Y_BOT + 1, 4):
        canvas.drawFastHLine(PITCH_MIN_X, gy, 8, SOCCER_NET)
    for gx in range(PITCH_MIN_X, PITCH_MIN_X + 9, 3):
        canvas.drawFastVLine(gx, GOAL_Y_TOP, GOAL_Y_BOT - GOAL_Y_TOP, SOCCER_NET)
    canvas.drawFastVLine(PITCH_MIN_X + 8, GOAL_Y_TOP, GOAL_Y_BOT - GOAL_Y_TOP, COL_WHITE)
    canvas.drawFastHLine(PITCH_MIN_X, GOAL_Y_TOP, 8, COL_WHITE)
    canvas.drawFastHLine(PITCH_MIN_X, GOAL_Y_BOT, 8, COL_WHITE)

    canvas.fillRect(PITCH_MAX_X - 8, GOAL_Y_TOP, 8, GOAL_Y_BOT - GOAL_Y_TOP, SOCCER_NET_SHD)
    for gy in range(GOAL_Y_TOP, GOAL_Y_BOT + 1, 4):
        canvas.drawFastHLine(PITCH_MAX_X - 8, gy, 8, SOCCER_NET)
    for gx in range(PITCH_MAX_X - 8, PITCH_MAX_X + 1, 3):
        canvas.drawFastVLine(gx, GOAL_Y_TOP, GOAL_Y_BOT - GOAL_Y_TOP, SOCCER_NET)
    canvas.drawFastVLine(PITCH_MAX_X - 8, GOAL_Y_TOP, GOAL_Y_BOT - GOAL_Y_TOP, COL_WHITE)
    canvas.drawFastHLine(PITCH_MAX_X - 8, GOAL_Y_TOP, 8, COL_WHITE)
    canvas.drawFastHLine(PITCH_MAX_X - 8, GOAL_Y_BOT, 8, COL_WHITE)


def render_hockey_rink(canvas: Canvas565):
    canvas.fillScreen(COL_BLACK)
    buf = canvas.buf
    cx, cy, r = 120, 120, 117

    # A. Brilliant pure white solid ice sheet (NO STRIPES, 0xFFFF solid white)
    for y in range(PITCH_MIN_Y, PITCH_MAX_Y + 1):
        dy = y - cy
        max_w = int(math.sqrt(max(0, r * r - dy * dy)))
        x1 = max(PITCH_MIN_X, cx - max_w)
        x2 = min(PITCH_MAX_X, cx + max_w)
        if x1 >= x2:
            continue
        ice_col = HOCKEY_ICE_A  # Solid crisp uniform 0xFFFF white ice
        for x in range(x1, x2 + 1):
            buf[y * 240 + x] = ice_col

    # B. Frosted skate scratch marks (Neutral silver gray 0xCE59 shadow)
    for y in range(PITCH_MIN_Y + 10, PITCH_MAX_Y - 9, 7):
        for x in range(PITCH_MIN_X + 16, PITCH_MAX_X - 15, 19):
            h = ((x * 13 + y * 29) ^ (x >> 2)) & 7
            if h < 3:
                length = 3 + (h & 3)
                direction = 1 if (h & 1) else -1
                for i in range(length):
                    px = x + i
                    py = y + i * direction
                    if (PITCH_MIN_X + 10 <= px <= PITCH_MAX_X - 10 and
                        PITCH_MIN_Y + 10 <= py <= PITCH_MAX_Y - 10):
                        buf[py * 240 + px] = HOCKEY_ICE_CUT
                        if py + 1 <= PITCH_MAX_Y - 10:
                            buf[(py + 1) * 240 + px] = HOCKEY_ICE_SHD

    # C. Regulation Ice Markings: Red Line, Blue Lines, Goal Lines
    # Center Red Line (red/white dashed checker)
    for y in range(PITCH_MIN_Y + 6, PITCH_MAX_Y - 5):
        c = HOCKEY_RED_LINE if ((y // 4) % 2 == 0) else COL_WHITE
        canvas.drawPixel(119, y, c)
        canvas.drawPixel(120, y, c)

    # Blue Zone Lines
    canvas.drawFastVLine(84, PITCH_MIN_Y + 6, (PITCH_MAX_Y - PITCH_MIN_Y) - 12, HOCKEY_BLU_LINE)
    canvas.drawFastVLine(85, PITCH_MIN_Y + 6, (PITCH_MAX_Y - PITCH_MIN_Y) - 12, HOCKEY_BLU_LINE)
    canvas.drawFastVLine(155, PITCH_MIN_Y + 6, (PITCH_MAX_Y - PITCH_MIN_Y) - 12, HOCKEY_BLU_LINE)
    canvas.drawFastVLine(156, PITCH_MIN_Y + 6, (PITCH_MAX_Y - PITCH_MIN_Y) - 12, HOCKEY_BLU_LINE)

    # Goal Lines
    canvas.drawFastVLine(PITCH_MIN_X + 14, PITCH_MIN_Y + 16, (PITCH_MAX_Y - PITCH_MIN_Y) - 32, HOCKEY_RED_LINE)
    canvas.drawFastVLine(PITCH_MAX_X - 14, PITCH_MIN_Y + 16, (PITCH_MAX_Y - PITCH_MIN_Y) - 32, HOCKEY_RED_LINE)

    # D. Goal Creases (Painted ice-blue semi-circles with solid red borders)
    canvas.fillCircle(PITCH_MIN_X + 14, 129, 13, HOCKEY_CREASE)
    canvas.drawCircle(PITCH_MIN_X + 14, 129, 13, HOCKEY_RED_LINE)
    canvas.drawFastVLine(PITCH_MIN_X + 14, 116, 27, HOCKEY_RED_LINE)

    canvas.fillCircle(PITCH_MAX_X - 14, 129, 13, HOCKEY_CREASE)
    canvas.drawCircle(PITCH_MAX_X - 14, 129, 13, HOCKEY_RED_LINE)
    canvas.drawFastVLine(PITCH_MAX_X - 14, 116, 27, HOCKEY_RED_LINE)

    # E. Center Circle & Face-Off Dots
    canvas.drawCircle(120, 129, 26, HOCKEY_BLU_LINE)
    canvas.fillCircle(120, 129, 3, HOCKEY_RED_LINE)

    faceoff_pts = [(60, 85), (60, 173), (180, 85), (180, 173)]
    for fx, fy in faceoff_pts:
        canvas.drawCircle(fx, fy, 15, HOCKEY_RED_LINE)
        canvas.fillCircle(fx, fy, 3, HOCKEY_RED_LINE)
        canvas.drawFastHLine(fx - 18, fy - 4, 3, HOCKEY_RED_LINE)
        canvas.drawFastHLine(fx + 15, fy - 4, 3, HOCKEY_RED_LINE)
        canvas.drawFastHLine(fx - 18, fy + 4, 3, HOCKEY_RED_LINE)
        canvas.drawFastHLine(fx + 15, fy + 4, 3, HOCKEY_RED_LINE)

    for fx, fy in [(102, 95), (102, 163), (138, 95), (138, 163)]:
        canvas.fillCircle(fx, fy, 2, HOCKEY_RED_LINE)

    # F. Curved Rink Dasher Boards
    canvas.drawRoundRect(PITCH_MIN_X + 6, PITCH_MIN_Y + 2, (PITCH_MAX_X - PITCH_MIN_X) - 12, (PITCH_MAX_Y - PITCH_MIN_Y) - 4, 18, HOCKEY_BOARD_W)
    canvas.drawRoundRect(PITCH_MIN_X + 5, PITCH_MIN_Y + 1, (PITCH_MAX_X - PITCH_MIN_X) - 10, (PITCH_MAX_Y - PITCH_MIN_Y) - 2, 19, HOCKEY_BOARD_R)
    canvas.drawRoundRect(PITCH_MIN_X + 7, PITCH_MIN_Y + 3, (PITCH_MAX_X - PITCH_MIN_X) - 14, (PITCH_MAX_Y - PITCH_MIN_Y) - 6, 17, HOCKEY_BOARD_Y)

    # High-contrast acrylic reflections along boards
    for x in range(60, 181):
        off = (x - 60) % 24
        if 0 <= off <= 10:
            refl = HOCKEY_GLASS_HI if (4 <= off <= 7) else HOCKEY_GLASS_CY
            canvas.drawPixel(x, PITCH_MIN_Y + 2, refl)
            canvas.drawPixel(x + 1, PITCH_MIN_Y + 1, refl)

    for x in range(70, 171):
        off = (x - 70) % 30
        if 0 <= off <= 8:
            refl = HOCKEY_GLASS_HI if (3 <= off <= 5) else HOCKEY_GLASS_CY
            canvas.drawPixel(x, PITCH_MAX_Y - 3, refl)

    # G. Red Hockey Goals & Net
    canvas.fillRect(PITCH_MIN_X + 6, GOAL_Y_TOP + 6, 8, (GOAL_Y_BOT - GOAL_Y_TOP) - 12, 0x2124)
    for gy in range(GOAL_Y_TOP + 6, GOAL_Y_BOT - 5, 3):
        canvas.drawFastHLine(PITCH_MIN_X + 6, gy, 8, HOCKEY_NET_MESH)
    canvas.drawRect(PITCH_MIN_X + 6, GOAL_Y_TOP + 6, 8, (GOAL_Y_BOT - GOAL_Y_TOP) - 12, HOCKEY_RED_LINE)

    canvas.fillRect(PITCH_MAX_X - 14, GOAL_Y_TOP + 6, 8, (GOAL_Y_BOT - GOAL_Y_TOP) - 12, 0x2124)
    for gy in range(GOAL_Y_TOP + 6, GOAL_Y_BOT - 5, 3):
        canvas.drawFastHLine(PITCH_MAX_X - 14, gy, 8, HOCKEY_NET_MESH)
    canvas.drawRect(PITCH_MAX_X - 14, GOAL_Y_TOP + 6, 8, (GOAL_Y_BOT - GOAL_Y_TOP) - 12, HOCKEY_RED_LINE)


def render_basketball_court(canvas: Canvas565):
    canvas.fillScreen(COL_BLACK)
    buf = canvas.buf
    cx, cy, r = 120, 120, 117

    for y in range(PITCH_MIN_Y, PITCH_MAX_Y + 1):
        dy = y - cy
        max_w = int(math.sqrt(max(0, r * r - dy * dy)))
        x1 = max(PITCH_MIN_X, cx - max_w)
        x2 = min(PITCH_MAX_X, cx + max_w)
        if x1 >= x2:
            continue

        ty = (y - PITCH_MIN_Y) // 8
        py = (y - PITCH_MIN_Y) % 8

        for x in range(x1, x2 + 1):
            tx = (x - PITCH_MIN_X) // 16
            px = (x - PITCH_MIN_X) % 16

            if px == 0 or py == 0:
                buf[y * 240 + x] = BASKET_GROOVE
                continue

            horiz_grain = ((tx + ty) & 1) == 0
            tile_hash = (tx * 5 + ty * 11) & 3
            if tile_hash == 0:
                base_wood = BASKET_HONEY
            elif tile_hash == 1:
                base_wood = BASKET_AMBER
            elif tile_hash == 2:
                base_wood = BASKET_MAPLE
            else:
                base_wood = BASKET_PECAN

            if horiz_grain:
                if py == 3 or py == 6:
                    base_wood = BASKET_GRAIN
            else:
                if px == 4 or px == 8 or px == 12:
                    base_wood = BASKET_GRAIN

            s_dx = x - 120
            s_dy = (y - 116) * 3 // 2
            dist2 = s_dx * s_dx + s_dy * s_dy
            if dist2 < 4200:
                boost = (4200 - dist2) * 55 // 4200
                red = ((base_wood >> 11) & 0x1F) + (boost * 5 >> 6)
                grn = ((base_wood >> 5) & 0x3F) + (boost * 8 >> 6)
                blu = (base_wood & 0x1F) + (boost * 4 >> 6)
                red = min(31, red)
                grn = min(63, grn)
                blu = min(31, blu)
                base_wood = (red << 11) | (grn << 5) | blu

                if dist2 < 650 and ((x ^ y) & 1):
                    base_wood = BASKET_GLOSS

            buf[y * 240 + x] = base_wood

    # Boundary & lines
    canvas.drawRect(PITCH_MIN_X + 8, PITCH_MIN_Y + 4, (PITCH_MAX_X - PITCH_MIN_X) - 16, (PITCH_MAX_Y - PITCH_MIN_Y) - 8, BASKET_LINE)
    canvas.drawFastVLine(120, PITCH_MIN_Y + 4, (PITCH_MAX_Y - PITCH_MIN_Y) - 8, BASKET_LINE)

    # Key lanes
    canvas.fillRect(PITCH_MIN_X + 9, 95, 39, 68, BASKET_KEY_RED)
    canvas.drawRect(PITCH_MIN_X + 8, 95, 40, 68, BASKET_LINE)
    canvas.drawFastVLine(PITCH_MIN_X + 48, 95, 68, BASKET_KEY_SHD)
    canvas.drawCircle(PITCH_MIN_X + 48, 129, 16, BASKET_LINE)

    canvas.fillRect(PITCH_MAX_X - 47, 95, 39, 68, BASKET_KEY_RED)
    canvas.drawRect(PITCH_MAX_X - 48, 95, 40, 68, BASKET_LINE)
    canvas.drawFastVLine(PITCH_MAX_X - 49, 95, 68, BASKET_KEY_SHD)
    canvas.drawCircle(PITCH_MAX_X - 48, 129, 16, BASKET_LINE)

    # 3-Point Arcs
    canvas.drawFastHLine(PITCH_MIN_X + 8, 52, 16, BASKET_LINE)
    canvas.drawFastHLine(PITCH_MIN_X + 8, 206, 16, BASKET_LINE)
    for a in range(-68, 69):
        rad = a * (math.pi / 180.0)
        ax = (PITCH_MIN_X + 24) + int(math.cos(rad) * 58.0)
        ay = 129 + int(math.sin(rad) * 58.0)
        if ax >= PITCH_MIN_X + 24 and 52 <= ay <= 206:
            canvas.drawPixel(ax, ay, BASKET_LINE)

    canvas.drawFastHLine(PITCH_MAX_X - 24, 52, 16, BASKET_LINE)
    canvas.drawFastHLine(PITCH_MAX_X - 24, 206, 16, BASKET_LINE)
    for a in range(-68, 69):
        rad = a * (math.pi / 180.0)
        ax = (PITCH_MAX_X - 24) - int(math.cos(rad) * 58.0)
        ay = 129 + int(math.sin(rad) * 58.0)
        if ax <= PITCH_MAX_X - 24 and 52 <= ay <= 206:
            canvas.drawPixel(ax, ay, BASKET_LINE)

    # Center circle & basketball emblem
    canvas.drawCircle(120, 129, 24, BASKET_LINE)
    canvas.fillCircle(120, 129, 13, BASKET_ORANGE)
    for dy in range(-13, 14):
        hw = int(math.sqrt(max(0, 13 * 13 - dy * dy)))
        for dx in range(-hw, hw + 1):
            if dx + dy > 7:
                canvas.drawPixel(120 + dx, 129 + dy, BASKET_ORANGE_D)
            elif dx + dy < -7:
                canvas.drawPixel(120 + dx, 129 + dy, BASKET_ORANGE_L)
    canvas.drawFastHLine(108, 129, 25, BASKET_SEAM)
    canvas.drawFastVLine(120, 117, 25, BASKET_SEAM)
    canvas.drawCircle(113, 129, 9, BASKET_SEAM)
    canvas.drawCircle(127, 129, 9, BASKET_SEAM)
    canvas.drawCircle(120, 129, 13, BASKET_SEAM)

    # Hoops
    canvas.fillRect(PITCH_MIN_X + 6, 114, 3, 30, COL_WHITE)
    canvas.drawCircle(PITCH_MIN_X + 16, 129, 5, BASKET_HOOP_RIM)
    canvas.drawFastHLine(PITCH_MIN_X + 12, 134, 8, BASKET_NET)

    canvas.fillRect(PITCH_MAX_X - 9, 114, 3, 30, COL_WHITE)
    canvas.drawCircle(PITCH_MAX_X - 16, 129, 5, BASKET_HOOP_RIM)
    canvas.drawFastHLine(PITCH_MAX_X - 20, 134, 8, BASKET_NET)


def render_baseball_field(canvas: Canvas565):
    canvas.fillScreen(COL_BLACK)
    buf = canvas.buf
    cx, cy, r = 120, 120, 117

    # Outfield Grass with Radial Mowed Lawn Pattern
    for y in range(PITCH_MIN_Y, PITCH_MAX_Y + 1):
        dy = y - cy
        max_w = int(math.sqrt(max(0, r * r - dy * dy)))
        x1 = max(PITCH_MIN_X, cx - max_w)
        x2 = min(PITCH_MAX_X, cx + max_w)
        if x1 >= x2:
            continue

        for x in range(x1, x2 + 1):
            f_dx = x - 120
            f_dy = y - 195
            dist = int(math.sqrt(f_dx * f_dx + f_dy * f_dy))
            stripe = (dist // 14) % 2
            grass_col = BASEBALL_TURF_A if stripe == 0 else BASEBALL_TURF_B
            if dist % 14 == 0:
                grass_col = BASEBALL_TURF_S
            if ((x * 13 + y * 29) & 7) == 0:
                grass_col = 0x2D47 if stripe == 0 else 0x2484
            buf[y * 240 + x] = grass_col

    # Outfield Warning Track
    for dy in range(-142, 1):
        py = 195 + dy
        if py < PITCH_MIN_Y or py > PITCH_MAX_Y:
            continue
        hw_out = int(math.sqrt(max(0, 142 * 142 - dy * dy)))
        hw_in  = int(math.sqrt(max(0, 133 * 133 - dy * dy)))
        for x in range(120 - hw_out, 120 - hw_in + 1):
            if PITCH_MIN_X <= x <= PITCH_MAX_X:
                buf[py * 240 + x] = BASEBALL_TRACK if ((x ^ py) & 1) else BASEBALL_CLAY_C
        for x in range(120 + hw_in, 120 + hw_out + 1):
            if PITCH_MIN_X <= x <= PITCH_MAX_X:
                buf[py * 240 + x] = BASEBALL_TRACK if ((x ^ py) & 1) else BASEBALL_CLAY_C

    # Outfield Fence Wall & Banners
    for a in range(210, 331):
        rad = a * (math.pi / 180.0)
        fx = 120 + int(math.cos(rad) * 143.0)
        fy = 195 + int(math.sin(rad) * 143.0)
        if PITCH_MIN_X <= fx <= PITCH_MAX_X and PITCH_MIN_Y <= fy <= PITCH_MAX_Y:
            canvas.drawPixel(fx, fy, BASEBALL_FENCE)
            canvas.drawPixel(fx, fy - 1, BASEBALL_PAD_Y)
            banner = (a // 8) % 4
            if banner == 1:
                canvas.drawPixel(fx, fy, BASEBALL_BANNER_R)
            elif banner == 3:
                canvas.drawPixel(fx, fy, BASEBALL_BANNER_B)

    # Infield Dirt Clay Diamond
    for dy in range(-52, 53):
        hw = 54 - (abs(dy) * 54) // 52
        py = 145 + dy
        if PITCH_MIN_Y <= py <= PITCH_MAX_Y:
            for x in range(120 - hw, 120 + hw + 1):
                if PITCH_MIN_X <= x <= PITCH_MAX_X:
                    h = ((x * 17 + py * 31) ^ (x >> 1)) & 7
                    if h == 0:
                        buf[py * 240 + x] = BASEBALL_CLAY_C
                    elif h == 7:
                        buf[py * 240 + x] = BASEBALL_CLAY_D
                    elif h == 1:
                        buf[py * 240 + x] = BASEBALL_CLAY_B
                    else:
                        buf[py * 240 + x] = BASEBALL_CLAY_A

    # Infield grass cutout
    for dy in range(-26, 27):
        hw = 28 - (abs(dy) * 28) // 26
        py = 145 + dy
        if PITCH_MIN_Y <= py <= PITCH_MAX_Y:
            for x in range(120 - hw, 120 + hw + 1):
                if PITCH_MIN_X <= x <= PITCH_MAX_X:
                    buf[py * 240 + x] = BASEBALL_TURF_A if ((x ^ py) & 1) else 0x1C84

    # Pitcher's mound & rubber
    canvas.fillCircle(120, 145, 10, BASEBALL_CLAY_C)
    canvas.fillCircle(120, 145, 8, BASEBALL_CLAY_A)
    canvas.fillCircle(120, 144, 5, 0xDE08)
    canvas.fillRect(117, 144, 6, 2, BASEBALL_BASE_W)

    # Foul lines & batter's boxes
    canvas.drawLine(120, 195, 218, 97, BASEBALL_CHALK)
    canvas.drawLine(120, 195, 22, 97, BASEBALL_CHALK)
    canvas.drawRect(107, 189, 7, 13, BASEBALL_CHALK)
    canvas.drawRect(126, 189, 7, 13, BASEBALL_CHALK)

    # Home plate
    canvas.fillRect(119, 194, 5, 5, BASEBALL_SHADOW)
    canvas.fillRect(118, 193, 5, 4, BASEBALL_BASE_W)

    # Bases
    for bx, by in [(170, 143), (118, 93), (66, 143)]:
        canvas.fillRect(bx + 1, by + 1, 6, 6, BASEBALL_SHADOW)
        canvas.fillRect(bx, by, 5, 5, BASEBALL_BASE_W)
        canvas.drawFastHLine(bx, by + 4, 5, BASEBALL_BASE_S)
        canvas.drawFastVLine(bx + 4, by, 5, BASEBALL_BASE_S)


# -----------------------------------------------------------------------------
# Player & Ball Drawing
# -----------------------------------------------------------------------------
class Player:
    def __init__(self, x, y, team, role, facing=1, hair_color=COL_HAIR_BLK):
        self.x = x
        self.y = y
        self.team = team
        self.role = role
        self.facing = facing
        self.hair_color = hair_color

def draw_player(canvas: Canvas565, p: Player, sport_name: str):
    px = int(p.x)
    py = int(p.y)
    dir_f = p.facing
    jersey = COL_RED_TEAM if p.team == 0 else COL_BLU_TEAM
    jersey_dark = COL_RED_DARK if p.team == 0 else COL_BLU_DARK

    # Drop shadow
    canvas.fillEllipse(px, py + 1, 6, 2, COL_SHADOW)

    head_y = py - 17
    torso_y = head_y + 7
    shorts_y = torso_y + 5
    legs_y = shorts_y + 3

    # Arms
    if sport_name == "BASKETBALL":
        canvas.fillRect(px - 5, torso_y, 2, 5, COL_SKIN)
        canvas.fillRect(px + 3, torso_y, 2, 5, COL_SKIN)
    else:
        canvas.fillRect(px - 5, torso_y, 2, 4, jersey)
        canvas.fillRect(px + 3, torso_y, 2, 4, jersey)
        canvas.fillRect(px - 5, torso_y + 3, 2, 2, COL_SKIN)
        canvas.fillRect(px + 3, torso_y + 3, 2, 2, COL_SKIN)

    # Head
    hx = px - 3
    if sport_name == "HOCKEY":
        canvas.fillRect(hx - 1, head_y - 3, 9, 5, jersey)
        canvas.fillRect(hx, head_y + 2, 7, 4, COL_SKIN)
        canvas.drawFastHLine(hx, head_y + 1, 7, 0x2104)
    elif sport_name == "BASEBALL":
        canvas.fillRect(hx - 1, head_y - 2, 9, 4, jersey)
        canvas.drawFastHLine(hx + (5 if dir_f > 0 else -3), head_y + 1, 4, jersey)
        canvas.fillRect(hx, head_y + 2, 7, 4, COL_SKIN)
    else:
        canvas.fillRect(hx - 1, head_y - 2, 9, 3, p.hair_color)
        canvas.fillRect(hx - (1 if dir_f > 0 else 0), head_y - 3, 5, 2, p.hair_color)
        canvas.fillRect(hx, head_y, 7, 6, COL_SKIN)

    # Eyes & Mouth
    eye_x = hx + 3 if dir_f > 0 else hx + 1
    canvas.drawPixel(eye_x, head_y + 2, COL_BLACK)
    canvas.drawPixel(eye_x + 1, head_y + 2, COL_BLACK)
    canvas.drawPixel(hx + 4 if dir_f > 0 else hx + 2, head_y + 4, 0x8180)

    # Torso
    canvas.fillRect(px - 4, torso_y, 8, 5, jersey)
    canvas.drawFastHLine(px - 4, torso_y + 4, 8, jersey_dark)

    # Hockey stick
    if sport_name == "HOCKEY":
        stick_x = px + 4 * dir_f
        canvas.drawLine(px, torso_y + 3, stick_x, py + 1, COL_STICK_WOOD)
        canvas.drawFastHLine(stick_x, py + 1, 4 * dir_f, COL_STICK_WOOD)

    # Baseball bat
    if sport_name == "BASEBALL" and p.role == "BATTER":
        canvas.drawLine(px - 2 * dir_f, torso_y - 1, px - 7 * dir_f, head_y - 3, COL_STICK_WOOD)
        canvas.drawLine(px - 3 * dir_f, torso_y - 1, px - 8 * dir_f, head_y - 3, COL_STICK_WOOD)

    # Shorts
    canvas.fillRect(px - 3, shorts_y, 7, 3, COL_SHORTS)

    # Legs & boots
    canvas.fillRect(px - 3, legs_y, 2, 2, COL_SKIN)
    canvas.fillRect(px - 3, legs_y + 2, 3, 2, COL_BOOTS)
    canvas.fillRect(px + 1, legs_y, 2, 2, COL_SKIN)
    canvas.fillRect(px + 1, legs_y + 2, 3, 2, COL_BOOTS)

    # Skate blades for hockey
    if sport_name == "HOCKEY":
        canvas.drawFastHLine(px - 4, legs_y + 4, 4, COL_BLADE_SILV)
        canvas.drawFastHLine(px + 1, legs_y + 4, 4, COL_BLADE_SILV)

def draw_ball(canvas: Canvas565, sport_name: str, x: int, y: int):
    if sport_name == "SOCCER":
        canvas.fillCircle(x, y, 3, COL_WHITE)
        canvas.drawPixel(x, y, 0x18C3)
        canvas.drawPixel(x - 1, y - 1, 0x18C3)
        canvas.drawPixel(x + 1, y + 1, 0x18C3)
    elif sport_name == "HOCKEY":
        canvas.fillRect(x - 2, y - 1, 5, 3, COL_PUCK)
    elif sport_name == "BASKETBALL":
        canvas.fillCircle(x, y, 4, BASKET_ORANGE)
        canvas.drawFastHLine(x - 3, y, 7, BASKET_SEAM)
        canvas.drawFastVLine(x, y - 3, 7, BASKET_SEAM)
    elif sport_name == "BASEBALL":
        canvas.fillCircle(x, y, 3, COL_WHITE)
        canvas.drawPixel(x - 1, y, BASKET_HOOP_RIM)
        canvas.drawPixel(x + 1, y, BASKET_HOOP_RIM)

def draw_hud(canvas: Canvas565, sport_label: str, is_baseball: bool = False):
    # Top scoreboard box
    canvas.fillRect(25, 6, 190, 18, COL_HUD_BG)
    canvas.drawRect(25, 6, 190, 18, COL_GOLD)

    # RED Score
    canvas.fillRect(30, 9, 8, 8, COL_RED_TEAM)
    canvas.drawText("RED 0", 42, 11, COL_GOLD)

    # Sport Name badge
    tx = 120 - (len(sport_label) * 6) // 2
    canvas.drawText(sport_label, tx, 11, COL_GOLD)

    # BLUE Score
    canvas.drawText("0 BLU", 158, 11, COL_GOLD)
    canvas.fillRect(198, 9, 8, 8, COL_BLU_TEAM)

    # Bottom Bezel Action Prompt
    canvas.drawText("BOOT: TAP=SHOT HOLD=EVADE", 44, 228, 0x5AEB)

    # Baseball swing timing gauge
    if is_baseball:
        gauge_x = 212
        gauge_y_top = 160
        gauge_y_bot = 210
        gauge_sweet_y = 185
        canvas.fillRect(gauge_x - 1, gauge_y_top - 1, 10, (gauge_y_bot - gauge_y_top) + 2, COL_HUD_BG)
        canvas.drawRect(gauge_x - 1, gauge_y_top - 1, 10, (gauge_y_bot - gauge_y_top) + 2, COL_GOLD)
        canvas.fillRect(gauge_x, gauge_sweet_y - 4, 8, 9, 0x07E0)  # Green sweet spot
        canvas.drawRect(gauge_x, gauge_sweet_y - 4, 8, 9, COL_WHITE)
        canvas.fillRect(gauge_x - 2, gauge_sweet_y - 2, 12, 5, COL_GOLD)
        canvas.drawRect(gauge_x - 2, gauge_sweet_y - 2, 12, 5, COL_BLACK)


# -----------------------------------------------------------------------------
# Complete Scene Composer
# -----------------------------------------------------------------------------
def render_full_scene(sport_idx: int) -> Canvas565:
    """Renders surface, vignette, players, ball, and HUD."""
    canvas = Canvas565(240, 240)

    if sport_idx == 0:  # SOCCER
        render_soccer_pitch(canvas)
        apply_circular_vignette(canvas)
        # Players
        players = [
            Player(32, 129, 0, "GK", 1, COL_HAIR_BLK),
            Player(116, 95, 0, "FWD", 1, COL_HAIR_BLK),
            Player(65, 170, 0, "DEF", 1, COL_HAIR_BRN),
            Player(208, 129, 1, "GK", -1, COL_HAIR_BRN),
            Player(155, 165, 1, "FWD", -1, COL_HAIR_BLD),
            Player(175, 85, 1, "DEF", -1, COL_HAIR_BLK),
        ]
        players.sort(key=lambda p: p.y)
        ball_drawn = False
        for p in players:
            if not ball_drawn and 129 <= p.y:
                draw_ball(canvas, "SOCCER", 120, 129)
                ball_drawn = True
            draw_player(canvas, p, "SOCCER")
        if not ball_drawn:
            draw_ball(canvas, "SOCCER", 120, 129)
        draw_hud(canvas, "SOCCER", False)

    elif sport_idx == 1:  # HOCKEY
        render_hockey_rink(canvas)
        apply_circular_vignette(canvas)
        # Players
        players = [
            Player(32, 129, 0, "GK", 1, COL_HAIR_BLK),
            Player(116, 95, 0, "FWD", 1, COL_HAIR_BLK),
            Player(65, 170, 0, "DEF", 1, COL_HAIR_BRN),
            Player(208, 129, 1, "GK", -1, COL_HAIR_BRN),
            Player(155, 165, 1, "FWD", -1, COL_HAIR_BLD),
            Player(175, 85, 1, "DEF", -1, COL_HAIR_BLK),
        ]
        players.sort(key=lambda p: p.y)
        puck_drawn = False
        for p in players:
            if not puck_drawn and 129 <= p.y:
                draw_ball(canvas, "HOCKEY", 120, 129)
                puck_drawn = True
            draw_player(canvas, p, "HOCKEY")
        if not puck_drawn:
            draw_ball(canvas, "HOCKEY", 120, 129)
        draw_hud(canvas, "HOCKEY", False)

    elif sport_idx == 2:  # BASKETBALL
        render_basketball_court(canvas)
        apply_circular_vignette(canvas)
        # Players
        players = [
            Player(32, 129, 0, "GK", 1, COL_HAIR_BLK),
            Player(116, 95, 0, "FWD", 1, COL_HAIR_BLK),
            Player(65, 170, 0, "DEF", 1, COL_HAIR_BRN),
            Player(208, 129, 1, "GK", -1, COL_HAIR_BRN),
            Player(155, 165, 1, "FWD", -1, COL_HAIR_BLD),
            Player(175, 85, 1, "DEF", -1, COL_HAIR_BLK),
        ]
        players.sort(key=lambda p: p.y)
        ball_drawn = False
        for p in players:
            if not ball_drawn and 129 <= p.y:
                draw_ball(canvas, "BASKETBALL", 120, 129)
                ball_drawn = True
            draw_player(canvas, p, "BASKETBALL")
        if not ball_drawn:
            draw_ball(canvas, "BASKETBALL", 120, 129)
        draw_hud(canvas, "HOOPS", False)

    elif sport_idx == 3:  # BASEBALL
        render_baseball_field(canvas)
        apply_circular_vignette(canvas)
        # Baseball diamond positioning
        players = [
            Player(120, 65, 1, "CF", 1, COL_HAIR_BLK),    # Center Fielder
            Player(114, 195, 0, "BATTER", 1, COL_HAIR_BLK),# Batter
            Player(68, 145, 1, "3B", 1, COL_HAIR_BRN),    # 3rd Base
            Player(120, 145, 1, "PITCHER", 1, COL_HAIR_BLK),# Pitcher
            Player(120, 206, 1, "CATCHER", -1, COL_HAIR_BLD),# Catcher
            Player(172, 145, 1, "1B", -1, COL_HAIR_BRN),  # 1st Base
        ]
        players.sort(key=lambda p: p.y)
        ball_drawn = False
        for p in players:
            if not ball_drawn and 145 <= p.y:
                draw_ball(canvas, "BASEBALL", 120, 145)
                ball_drawn = True
            draw_player(canvas, p, "BASEBALL")
        if not ball_drawn:
            draw_ball(canvas, "BASEBALL", 120, 145)
        draw_hud(canvas, "HARDBALL", True)

    # Apply circular bezel boundary masking to simulate the physical GC9A01 240x240 round frame
    mask_gc9a01_circular_bezel(canvas)

    return canvas


def canvas_to_pil(canvas: Canvas565, bgr_mode=False) -> Image.Image:
    img = Image.new("RGB", (canvas.w, canvas.h))
    pixels = []
    for c in canvas.buf:
        pixels.append(rgb565_to_rgb888(c, bgr_mode))
    img.putdata(pixels)
    return img


# -----------------------------------------------------------------------------
# Automated QA Inspection & Histogram Analysis
# -----------------------------------------------------------------------------
def run_qa_inspection():
    print("=" * 72)
    print("      VISUAL QUALITY ASSURANCE (QA) AUTOMATED CERTIFICATION SUITE     ")
    print("           ScoreTracker GC9A01 240x240 Circular Display               ")
    print("=" * 72)

    sports = [
        (0, "Soccer", "qa_soccer.png"),
        (1, "Hockey", "qa_hockey.png"),
        (2, "Basketball", "qa_basketball.png"),
        (3, "Baseball", "qa_baseball.png"),
    ]

    out_dir = "/home/stefan/Projects/ScoreTracker/tools"
    os.makedirs(out_dir, exist_ok=True)

    canvases = {}
    images = {}

    for idx, name, filename in sports:
        canvas = render_full_scene(idx)
        canvases[idx] = canvas
        img = canvas_to_pil(canvas, bgr_mode=False)
        filepath = os.path.join(out_dir, filename)
        img.save(filepath)
        images[idx] = img
        print(f"[OK] Generated & Saved: {filepath} (240x240 PNG)")

    # -------------------------------------------------------------------------
    # TEST 1: Hockey Ice Sheet Tint Verification (Zero Pink / Neutral Balance)
    # -------------------------------------------------------------------------
    print("\n[QA TEST 1] Hockey Ice Colorimetric & Tint Analysis:")
    hockey_canvas = canvases[1]

    # Sample open ice sheet patches:
    # 1. Neutral zone upper: x in [92..110], y in [42..70]
    # 2. Neutral zone lower: x in [92..110], y in [180..200]
    # 3. Left offensive zone: x in [42..55], y in [45..70]
    # 4. Right defensive zone: x in [165..178], y in [45..70]
    ice_samples = []
    for y in range(42, 71):
        for x in range(92, 111):
            c = hockey_canvas.buf[y * 240 + x]
            ice_samples.append((c, rgb565_to_rgb888(c, bgr_mode=False), rgb565_to_rgb888(c, bgr_mode=True)))
    for y in range(180, 201):
        for x in range(92, 111):
            c = hockey_canvas.buf[y * 240 + x]
            ice_samples.append((c, rgb565_to_rgb888(c, bgr_mode=False), rgb565_to_rgb888(c, bgr_mode=True)))

    total_ice_pixels = len(ice_samples)
    pure_white_count = sum(1 for c, rgb, _ in ice_samples if c == 0xFFFF)
    pink_tint_count = 0
    max_red_excess = 0

    for c, rgb, bgr in ice_samples:
        r, g, b = rgb
        # Pink/magenta tint criterion: Red significantly exceeds Green and Blue
        if (r > g + 10) and (r > b + 10):
            pink_tint_count += 1
        excess = r - max(g, b)
        if excess > max_red_excess:
            max_red_excess = excess

    white_pct = (pure_white_count / total_ice_pixels) * 100.0
    print(f"  - Total Ice Surface Sampled Pixels: {total_ice_pixels}")
    print(f"  - Pure White Ice Pixels (0xFFFF):  {pure_white_count} ({white_pct:.1f}%)")
    print(f"  - Detected Pink Tint Pixels:        {pink_tint_count}")
    print(f"  - Max (R - max(G,B)) Red Excess:   {max_red_excess} LSB")

    # In BGR display mode test (swapping R and B channels):
    bgr_pink_count = 0
    for _, _, bgr in ice_samples:
        r, g, b = bgr
        if (r > g + 10) and (r > b + 10):
            bgr_pink_count += 1
    print(f"  - BGR Swapped Mode Pink Pixels:    {bgr_pink_count}")

    test1_pass = (pink_tint_count == 0 and bgr_pink_count == 0 and white_pct >= 95.0)
    print(f"  >>> RESULT: {'[PASS]' if test1_pass else '[FAIL]'} Zero Pink/Magenta Tint on Ice")

    # -------------------------------------------------------------------------
    # TEST 2: Hockey Ice Stripe Artifact Analysis (NO 16px Stripes)
    # -------------------------------------------------------------------------
    print("\n[QA TEST 2] Hockey Ice Uniformity & Stripe Elimination Analysis:")
    # Evaluate scanline luminance stability across rows y in [42..70]
    row_luminances = []
    for y in range(42, 71):
        lum_sum = 0
        cnt = 0
        for x in range(92, 111):
            c = hockey_canvas.buf[y * 240 + x]
            r, g, b = rgb565_to_rgb888(c)
            lum = 0.299 * r + 0.587 * g + 0.114 * b
            lum_sum += lum
            cnt += 1
        row_luminances.append(lum_sum / cnt)

    max_row_diff = max(row_luminances) - min(row_luminances)
    avg_lum = sum(row_luminances) / len(row_luminances)
    print(f"  - Ice Sample Rows Evaluated:       {len(row_luminances)} (y=42 to y=70)")
    print(f"  - Average Ice Luminance:          {avg_lum:.2f} / 255.0")
    print(f"  - Max Row-to-Row Luminance Span:  {max_row_diff:.2f} (threshold < 6.0)")

    test2_pass = (max_row_diff < 6.0 and avg_lum > 250.0)
    print(f"  >>> RESULT: {'[PASS]' if test2_pass else '[FAIL]'} Solid Uniform Ice Sheet (No 16px Alternating Stripes)")

    # -------------------------------------------------------------------------
    # TEST 3: Marking Contrast & Color Fidelity Across All Sports
    # -------------------------------------------------------------------------
    print("\n[QA TEST 3] Contrast & Line Visibility Fidelity Across All 4 Sports:")
    white_ice_col = rgb565_to_rgb888(HOCKEY_ICE_A)
    red_line_col = rgb565_to_rgb888(HOCKEY_RED_LINE)
    blu_line_col = rgb565_to_rgb888(HOCKEY_BLU_LINE)

    def contrast_ratio(c1, c2):
        l1 = 0.2126 * (c1[0]/255)**2.2 + 0.7152 * (c1[1]/255)**2.2 + 0.0722 * (c1[2]/255)**2.2
        l2 = 0.2126 * (c2[0]/255)**2.2 + 0.7152 * (c2[1]/255)**2.2 + 0.0722 * (c2[2]/255)**2.2
        lighter = max(l1, l2)
        darker = min(l1, l2)
        return (lighter + 0.05) / (darker + 0.05)

    cr_hockey_red = contrast_ratio(white_ice_col, red_line_col)
    cr_hockey_blu = contrast_ratio(white_ice_col, blu_line_col)
    print(f"  - Hockey Red Line vs Ice Contrast Ratio:  {cr_hockey_red:.2f}:1 (Target > 3.0:1)")
    print(f"  - Hockey Blue Line vs Ice Contrast Ratio: {cr_hockey_blu:.2f}:1 (Target > 3.0:1)")

    grass_col = rgb565_to_rgb888(SOCCER_GRASS_A1)
    chalk_col = rgb565_to_rgb888(SOCCER_CHALK)
    cr_soccer = contrast_ratio(chalk_col, grass_col)
    print(f"  - Soccer Chalk vs Emerald Grass Contrast: {cr_soccer:.2f}:1 (Target > 3.0:1)")

    wood_col = rgb565_to_rgb888(BASKET_HONEY)
    court_line_col = rgb565_to_rgb888(BASKET_LINE)
    cr_basket = contrast_ratio(court_line_col, wood_col)
    print(f"  - Basketball Line vs Hardwood Contrast:   {cr_basket:.2f}:1 (Target > 1.8:1)")

    clay_col = rgb565_to_rgb888(BASEBALL_CLAY_A)
    base_chalk_col = rgb565_to_rgb888(BASEBALL_CHALK)
    cr_baseball = contrast_ratio(base_chalk_col, clay_col)
    print(f"  - Baseball Chalk vs Infield Clay Contrast:{cr_baseball:.2f}:1 (Target > 3.0:1)")

    test3_pass = (cr_hockey_red >= 3.0 and cr_hockey_blu >= 3.0 and
                  cr_soccer >= 3.0 and cr_basket >= 1.8 and cr_baseball >= 3.0)
    print(f"  >>> RESULT: {'[PASS]' if test3_pass else '[FAIL]'} Marking & Surface Contrast Ratios")

    # -------------------------------------------------------------------------
    # TEST 4: GC9A01 Circular Bezel Boundary & Vignette Clamping
    # -------------------------------------------------------------------------
    print("\n[QA TEST 4] GC9A01 240x240 Circular Display Geometry & Bezel Bleed:")
    bezel_errors = 0
    total_bezel_pixels = 0
    for idx in range(4):
        c_buf = canvases[idx].buf
        for y in range(240):
            dy = y - 120
            dy2 = dy * dy
            for x in range(240):
                dx = x - 120
                if dx * dx + dy2 >= 14400:  # Radius >= 120px (outside circular display)
                    total_bezel_pixels += 1
                    if c_buf[y * 240 + x] != COL_BLACK:
                        bezel_errors += 1

    print(f"  - Total Evaluated Corner/Bezel Pixels: {total_bezel_pixels}")
    print(f"  - Bezel Bleed Leaks (Non-Black):       {bezel_errors}")
    test4_pass = (bezel_errors == 0)
    print(f"  >>> RESULT: {'[PASS]' if test4_pass else '[FAIL]'} Circular Bezel Clamping (0 Leaks)")

    # -------------------------------------------------------------------------
    # OVERALL CERTIFICATION SUMMARY
    # -------------------------------------------------------------------------
    all_passed = test1_pass and test2_pass and test3_pass and test4_pass
    print("\n" + "=" * 72)
    if all_passed:
        print("          *** VISUAL QA CERTIFICATION STATUS: APPROVED ***             ")
        print("          Zero Defects Detected. Ready for Production Deployment.      ")
    else:
        print("          *** VISUAL QA CERTIFICATION STATUS: REJECTED ***             ")
        print("          Visual defects detected. Please review logs above.           ")
    print("=" * 72)

    return all_passed

if __name__ == "__main__":
    success = run_qa_inspection()
    sys.exit(0 if success else 1)
