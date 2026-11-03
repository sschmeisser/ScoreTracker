#!/usr/bin/env python3
"""
generate_all_logos_header.py - Generate C++ PROGMEM header containing official 32x26 RGB565
logos across all 6 leagues: NFL (32), NBA (30), NHL (32), MLS (30), CFB (FBS & Top Programs),
and CBB (NCAA Men's College Basketball, all 360+ Division I programs).
Pre-composited with 0x1103 Deep Forest Moss background and 0xF81F chroma-key transparency.
"""

import os
import glob
from PIL import Image, ImageFilter

PROJECT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CACHE_DIR = os.path.join(PROJECT_DIR, "tools", "logos_cache")
OUT_HEADER = os.path.join(PROJECT_DIR, "src", "all_team_logos.h")

LOGO_W = 32
LOGO_H = 26
CHROMA_KEY = 0xF81F
CARD_BG = (22, 32, 27)  # 0x1103 Deep Forest Moss

def to_rgb565(r, g, b):
    r5 = (r >> 3) & 0x1F
    g6 = (g >> 2) & 0x3F
    b5 = (b >> 3) & 0x1F
    return (r5 << 11) | (g6 << 5) | b5

def process_image(path, target_w=LOGO_W, target_h=LOGO_H, bg_color=CARD_BG):
    im = Image.open(path).convert('RGBA')
    bbox = im.getbbox()
    if bbox:
        im = im.crop(bbox)
    w, h = im.size

    # Check average luminance of non-transparent pixels
    r, g, b, a = im.split()
    gray = im.convert('L')
    mask = a.point(lambda p: 255 if p > 32 else 0)
    hist = gray.histogram(mask=mask)
    total_px = sum(hist)
    avg_lum = (sum(i * count for i, count in enumerate(hist)) / total_px) if total_px > 0 else 255.0

    team_name = os.path.splitext(os.path.basename(path))[0]
    is_dark = (avg_lum < 35.0) or (team_name in ('CAR', 'NYG', 'TB'))

    if is_dark:
        scale = min((target_w - 2) / w, (target_h - 2) / h)
        nw, nh = max(1, int(w * scale)), max(1, int(h * scale))
        im_resized = im.resize((nw, nh), Image.Resampling.LANCZOS)
        canvas = Image.new('RGBA', (target_w, target_h), (0, 0, 0, 0))
        ox = (target_w - nw) // 2
        oy = (target_h - nh) // 2
        canvas.paste(im_resized, (ox, oy), im_resized)

        # 1px soft white/sand stroke behind the logo
        stroke_alpha = canvas.split()[3].filter(ImageFilter.MaxFilter(3))
        stroke_layer = Image.new('RGBA', (target_w, target_h), (235, 235, 225, 0))
        stroke_alpha_mod = stroke_alpha.point(lambda p: int(p * 0.85) if p > 32 else 0)
        stroke_layer.putalpha(stroke_alpha_mod)
        canvas = Image.alpha_composite(stroke_layer, canvas)
    else:
        scale = min(target_w / w, target_h / h)
        nw, nh = max(1, int(w * scale)), max(1, int(h * scale))
        im_resized = im.resize((nw, nh), Image.Resampling.LANCZOS)
        canvas = Image.new('RGBA', (target_w, target_h), (0, 0, 0, 0))
        ox = (target_w - nw) // 2
        oy = (target_h - nh) // 2
        canvas.paste(im_resized, (ox, oy), im_resized)

    pixels_565 = []
    for y in range(target_h):
        for x in range(target_w):
            r, g, b, a = canvas.getpixel((x, y))
            if a < 16:
                pixels_565.append(CHROMA_KEY)
            else:
                br = (r * a + bg_color[0] * (255 - a)) // 255
                bg = (g * a + bg_color[1] * (255 - a)) // 255
                bb = (b * a + bg_color[2] * (255 - a)) // 255
                val = to_rgb565(br, bg, bb)
                if val == CHROMA_KEY:
                    val = 0xF81E
                pixels_565.append(val)
    return pixels_565

def main():
    print("Generating comprehensive multi-league logos header...")

    # Discover teams
    nfl_teams = sorted([os.path.splitext(os.path.basename(p))[0] for p in glob.glob(f"{CACHE_DIR}/nfl/*.png")])
    nba_teams = sorted([os.path.splitext(os.path.basename(p))[0] for p in glob.glob(f"{CACHE_DIR}/nba/*.png")])
    nhl_teams = sorted([os.path.splitext(os.path.basename(p))[0] for p in glob.glob(f"{CACHE_DIR}/nhl/*.png")])
    mls_teams = sorted([os.path.splitext(os.path.basename(p))[0] for p in glob.glob(f"{CACHE_DIR}/mls/*.png")])

    # CFB: Top programs + FBS + today's active teams
    cfb_candidates = [
        # Today's active slate
        'ARIZ', 'ARK', 'ASU', 'AUB', 'BOIS', 'BYU', 'CIN', 'FLA', 'IOWA', 'ISU',
        'IU', 'MIA', 'MINN', 'MISS', 'MIZ', 'MSST', 'NAVY', 'ND', 'NEV', 'NU',
        'OKST', 'ORE', 'OSU', 'SMU', 'SYR', 'TEX', 'TTU', 'TXST', 'UCLA', 'UGA',
        'UNC', 'USC', 'UTAH', 'UVA', 'WAKE', 'WIS',
        # Premier FBS & Power 4 programs
        'ALA', 'CLEM', 'FSU', 'LSU', 'MICH', 'OU', 'PENN', 'PSU', 'TENN', 'TAMU',
        'COLO', 'NEB', 'WASH', 'KAN', 'BAY', 'TCU', 'LOU', 'NCST', 'VT', 'GT',
        'PITT', 'STAN', 'CAL', 'MD', 'RUTG', 'ILL', 'PUR', 'MSU', 'UK', 'VAN',
        'SC', 'WVU', 'KSU', 'MEM', 'TULN', 'FRES', 'SDSU', 'UNLV', 'CONN', 'ARMY'
    ]
    cfb_teams = []
    seen = set()
    for t in cfb_candidates:
        p = os.path.join(CACHE_DIR, "cfb", f"{t}.png")
        if os.path.exists(p) and t not in seen:
            seen.add(t)
            cfb_teams.append(t)

    cbb_all = sorted([os.path.splitext(os.path.basename(p))[0] for p in glob.glob(f"{CACHE_DIR}/cbb/*.png")])
    cbb_teams = cbb_all

    # European Soccer leagues
    epl_teams = sorted([os.path.splitext(os.path.basename(p))[0] for p in glob.glob(f"{CACHE_DIR}/epl/*.png")])
    lal_teams = sorted([os.path.splitext(os.path.basename(p))[0] for p in glob.glob(f"{CACHE_DIR}/lal/*.png")])
    bun_teams = sorted([os.path.splitext(os.path.basename(p))[0] for p in glob.glob(f"{CACHE_DIR}/bun/*.png")])
    ucl_teams = sorted([os.path.splitext(os.path.basename(p))[0] for p in glob.glob(f"{CACHE_DIR}/ucl/*.png")])
    uel_teams = sorted([os.path.splitext(os.path.basename(p))[0] for p in glob.glob(f"{CACHE_DIR}/uel/*.png")])

    # Find unique soccer teams across all 5 European leagues
    all_euro_teams = {}
    for lg, teams in [("epl", epl_teams), ("lal", lal_teams), ("bun", bun_teams), ("ucl", ucl_teams), ("uel", uel_teams)]:
        for t in teams:
            if t not in all_euro_teams:
                all_euro_teams[t] = os.path.join(CACHE_DIR, lg, f"{t}.png")

    print(f"Discovered teams: NFL={len(nfl_teams)}, NBA={len(nba_teams)}, NHL={len(nhl_teams)}, MLS={len(mls_teams)}, CFB={len(cfb_teams)}, CBB={len(cbb_teams)}, EuroSoccer={len(all_euro_teams)}")
    
    cfb_set = set(cfb_teams)
    cbb_unique_count = len([t for t in cbb_teams if t not in cfb_set])
    total_bitmaps = len(nfl_teams) + len(nba_teams) + len(nhl_teams) + len(mls_teams) + len(cfb_teams) + cbb_unique_count + len(all_euro_teams)
    print(f"Total unique bitmaps to embed: {total_bitmaps} (approx {total_bitmaps * LOGO_W * LOGO_H * 2 / 1024:.1f} KB)")

    with open(OUT_HEADER, "w") as f:
        f.write("// Autogenerated Multi-League Logos for ESP32 (RGB565 with 0xF81F chroma-key)\n")
        f.write("// Covers NFL (32), NBA (30), NHL (32), MLS (30), CFB (FBS & Top Programs), and CBB (360+ NCAA D1)\n")
        f.write("#pragma once\n\n")
        f.write("#include <Arduino.h>\n\n")
        f.write(f"#define LOGO_WIDTH {LOGO_W}\n")
        f.write(f"#define LOGO_HEIGHT {LOGO_H}\n")
        f.write(f"#define LOGO_CHROMA_KEY 0x{CHROMA_KEY:04X}\n\n")

        f.write("struct TeamLogoEntry {\n")
        f.write("  const char* code;\n")
        f.write("  const uint16_t* data;\n")
        f.write("};\n\n")

        league_groups = [
            ("NFL", "nfl", nfl_teams),
            ("NBA", "nba", nba_teams),
            ("NHL", "nhl", nhl_teams),
            ("MLS", "mls", mls_teams),
            ("CFB", "cfb", cfb_teams),
        ]

        for league_name, dir_name, teams in league_groups:
            f.write(f"// ==================== {league_name} LOGOS ({len(teams)} teams) ====================\n\n")
            for t in teams:
                path = os.path.join(CACHE_DIR, dir_name, f"{t}.png")
                data = process_image(path, LOGO_W, LOGO_H, CARD_BG)
                safe_name = t.replace('&', '_').replace('-', '_')
                f.write(f"static const uint16_t LOGO_{league_name}_{safe_name}[{LOGO_W} * {LOGO_H}] PROGMEM = {{\n")
                for i, p in enumerate(data):
                    if i % 16 == 0:
                        f.write("  ")
                    f.write(f"0x{p:04X}, ")
                    if i % 16 == 15:
                        f.write("\n")
                f.write("};\n\n")

            f.write(f"static const TeamLogoEntry {league_name}_LOGOS[] = {{\n")
            for t in teams:
                safe_name = t.replace('&', '_').replace('-', '_')
                f.write(f'  {{ "{t}", LOGO_{league_name}_{safe_name} }},\n')
            if league_name == "NFL" and "WAS" in teams:
                f.write('  { "WSH", LOGO_NFL_WAS },\n')
            f.write("};\n\n")

        # CBB Logos: generate unique ones, alias shared ones to CFB
        f.write(f"// ==================== CBB LOGOS ({len(cbb_teams)} teams) ====================\n\n")
        for t in cbb_teams:
            safe_name = t.replace('&', '_').replace('-', '_')
            if t in cfb_set:
                continue
            path = os.path.join(CACHE_DIR, "cbb", f"{t}.png")
            data = process_image(path, LOGO_W, LOGO_H, CARD_BG)
            f.write(f"static const uint16_t LOGO_CBB_{safe_name}[{LOGO_W} * {LOGO_H}] PROGMEM = {{\n")
            for i, p in enumerate(data):
                if i % 16 == 0:
                    f.write("  ")
                f.write(f"0x{p:04X}, ")
                if i % 16 == 15:
                    f.write("\n")
            f.write("};\n\n")

        f.write(f"static const TeamLogoEntry CBB_LOGOS[] = {{\n")
        for t in cbb_teams:
            safe_name = t.replace('&', '_').replace('-', '_')
            if t in cfb_set:
                f.write(f'  {{ "{t}", LOGO_CFB_{safe_name} }},\n')
            else:
                f.write(f'  {{ "{t}", LOGO_CBB_{safe_name} }},\n')
        f.write("};\n\n")

        # ==================== EUROPEAN SOCCER LOGOS ====================
        f.write(f"// ==================== EUROPEAN SOCCER LOGOS ({len(all_euro_teams)} unique teams) ====================\n\n")
        for t, path in sorted(all_euro_teams.items()):
            safe_name = t.replace('&', '_').replace('-', '_')
            data = process_image(path, LOGO_W, LOGO_H, CARD_BG)
            f.write(f"static const uint16_t LOGO_SOC_{safe_name}[{LOGO_W} * {LOGO_H}] PROGMEM = {{\n")
            for i, p in enumerate(data):
                if i % 16 == 0:
                    f.write("  ")
                f.write(f"0x{p:04X}, ")
                if i % 16 == 15:
                    f.write("\n")
            f.write("};\n\n")

        euro_league_subsets = [
            ("EPL", epl_teams),
            ("LAL", lal_teams),
            ("BUN", bun_teams),
            ("UCL", ucl_teams),
            ("UEL", uel_teams),
        ]
        for lg_name, lg_teams in euro_league_subsets:
            f.write(f"static const TeamLogoEntry {lg_name}_LOGOS[] = {{\n")
            for t in lg_teams:
                safe_name = t.replace('&', '_').replace('-', '_')
                f.write(f'  {{ "{t}", LOGO_SOC_{safe_name} }},\n')
            f.write("};\n\n")

        f.write(f"static const TeamLogoEntry SOCCER_EURO_LOGOS[] = {{\n")
        for t in sorted(all_euro_teams.keys()):
            safe_name = t.replace('&', '_').replace('-', '_')
            f.write(f'  {{ "{t}", LOGO_SOC_{safe_name} }},\n')
        f.write("};\n\n")

        # Master lookup function
        f.write("// Master League-Aware Team Logo Lookup\n")
        f.write("inline const uint16_t* get_team_logo_bitmap(const char* league, const char* code) {\n")
        f.write("  if (!code || !code[0]) return nullptr;\n\n")

        # European soccer league checks
        f.write('  if (league != nullptr && (strcmp(league, "EPL") == 0 || strcmp(league, "ENG.1") == 0)) {\n')
        f.write('    for (size_t i = 0; i < sizeof(EPL_LOGOS) / sizeof(EPL_LOGOS[0]); ++i) {\n')
        f.write('      if (strcmp(EPL_LOGOS[i].code, code) == 0) return EPL_LOGOS[i].data;\n')
        f.write('    }\n')
        f.write('    for (size_t i = 0; i < sizeof(SOCCER_EURO_LOGOS) / sizeof(SOCCER_EURO_LOGOS[0]); ++i) {\n')
        f.write('      if (strcmp(SOCCER_EURO_LOGOS[i].code, code) == 0) return SOCCER_EURO_LOGOS[i].data;\n')
        f.write('    }\n')
        f.write('    return nullptr;\n')
        f.write('  }\n\n')

        f.write('  if (league != nullptr && (strcmp(league, "LAL") == 0 || strcmp(league, "ESP.1") == 0 || strcmp(league, "LALIGA") == 0)) {\n')
        f.write('    for (size_t i = 0; i < sizeof(LAL_LOGOS) / sizeof(LAL_LOGOS[0]); ++i) {\n')
        f.write('      if (strcmp(LAL_LOGOS[i].code, code) == 0) return LAL_LOGOS[i].data;\n')
        f.write('    }\n')
        f.write('    for (size_t i = 0; i < sizeof(SOCCER_EURO_LOGOS) / sizeof(SOCCER_EURO_LOGOS[0]); ++i) {\n')
        f.write('      if (strcmp(SOCCER_EURO_LOGOS[i].code, code) == 0) return SOCCER_EURO_LOGOS[i].data;\n')
        f.write('    }\n')
        f.write('    return nullptr;\n')
        f.write('  }\n\n')

        f.write('  if (league != nullptr && (strcmp(league, "BUN") == 0 || strcmp(league, "GER.1") == 0 || strcmp(league, "BUNDESLIGA") == 0)) {\n')
        f.write('    for (size_t i = 0; i < sizeof(BUN_LOGOS) / sizeof(BUN_LOGOS[0]); ++i) {\n')
        f.write('      if (strcmp(BUN_LOGOS[i].code, code) == 0) return BUN_LOGOS[i].data;\n')
        f.write('    }\n')
        f.write('    for (size_t i = 0; i < sizeof(SOCCER_EURO_LOGOS) / sizeof(SOCCER_EURO_LOGOS[0]); ++i) {\n')
        f.write('      if (strcmp(SOCCER_EURO_LOGOS[i].code, code) == 0) return SOCCER_EURO_LOGOS[i].data;\n')
        f.write('    }\n')
        f.write('    return nullptr;\n')
        f.write('  }\n\n')

        f.write('  if (league != nullptr && (strcmp(league, "UCL") == 0 || strstr(league, "CHAMPIONS") != nullptr)) {\n')
        f.write('    for (size_t i = 0; i < sizeof(UCL_LOGOS) / sizeof(UCL_LOGOS[0]); ++i) {\n')
        f.write('      if (strcmp(UCL_LOGOS[i].code, code) == 0) return UCL_LOGOS[i].data;\n')
        f.write('    }\n')
        f.write('    for (size_t i = 0; i < sizeof(SOCCER_EURO_LOGOS) / sizeof(SOCCER_EURO_LOGOS[0]); ++i) {\n')
        f.write('      if (strcmp(SOCCER_EURO_LOGOS[i].code, code) == 0) return SOCCER_EURO_LOGOS[i].data;\n')
        f.write('    }\n')
        f.write('    return nullptr;\n')
        f.write('  }\n\n')

        f.write('  if (league != nullptr && (strcmp(league, "UEL") == 0 || strstr(league, "EUROPA") != nullptr)) {\n')
        f.write('    for (size_t i = 0; i < sizeof(UEL_LOGOS) / sizeof(UEL_LOGOS[0]); ++i) {\n')
        f.write('      if (strcmp(UEL_LOGOS[i].code, code) == 0) return UEL_LOGOS[i].data;\n')
        f.write('    }\n')
        f.write('    for (size_t i = 0; i < sizeof(SOCCER_EURO_LOGOS) / sizeof(SOCCER_EURO_LOGOS[0]); ++i) {\n')
        f.write('      if (strcmp(SOCCER_EURO_LOGOS[i].code, code) == 0) return SOCCER_EURO_LOGOS[i].data;\n')
        f.write('    }\n')
        f.write('    return nullptr;\n')
        f.write('  }\n\n')

        # Specific league checks
        for league_name, _, _ in league_groups:
            if league_name == "CFB":
                continue
            f.write(f'  if (league != nullptr && strcmp(league, "{league_name}") == 0) {{\n')
            f.write(f'    for (size_t i = 0; i < sizeof({league_name}_LOGOS) / sizeof({league_name}_LOGOS[0]); ++i) {{\n')
            f.write(f'      if (strcmp({league_name}_LOGOS[i].code, code) == 0) return {league_name}_LOGOS[i].data;\n')
            f.write('    }\n')
            f.write('    return nullptr;\n')
            f.write('  }\n\n')

        # CBB / CFB cross-pollination
        f.write('  if (league != nullptr && strcmp(league, "CBB") == 0) {\n')
        f.write('    for (size_t i = 0; i < sizeof(CBB_LOGOS) / sizeof(CBB_LOGOS[0]); ++i) {\n')
        f.write('      if (strcmp(CBB_LOGOS[i].code, code) == 0) return CBB_LOGOS[i].data;\n')
        f.write('    }\n')
        f.write('    for (size_t i = 0; i < sizeof(CFB_LOGOS) / sizeof(CFB_LOGOS[0]); ++i) {\n')
        f.write('      if (strcmp(CFB_LOGOS[i].code, code) == 0) return CFB_LOGOS[i].data;\n')
        f.write('    }\n')
        f.write('    return nullptr;\n')
        f.write('  }\n\n')

        f.write('  if (league != nullptr && strcmp(league, "CFB") == 0) {\n')
        f.write('    for (size_t i = 0; i < sizeof(CFB_LOGOS) / sizeof(CFB_LOGOS[0]); ++i) {\n')
        f.write('      if (strcmp(CFB_LOGOS[i].code, code) == 0) return CFB_LOGOS[i].data;\n')
        f.write('    }\n')
        f.write('    for (size_t i = 0; i < sizeof(CBB_LOGOS) / sizeof(CBB_LOGOS[0]); ++i) {\n')
        f.write('      if (strcmp(CBB_LOGOS[i].code, code) == 0) return CBB_LOGOS[i].data;\n')
        f.write('    }\n')
        f.write('    return nullptr;\n')
        f.write('  }\n\n')

        # Fallback search across NFL then other leagues if league is null or unmatched
        f.write("  for (size_t i = 0; i < sizeof(NFL_LOGOS) / sizeof(NFL_LOGOS[0]); ++i) {\n")
        f.write("    if (strcmp(NFL_LOGOS[i].code, code) == 0) return NFL_LOGOS[i].data;\n")
        f.write("  }\n")
        f.write("  for (size_t i = 0; i < sizeof(SOCCER_EURO_LOGOS) / sizeof(SOCCER_EURO_LOGOS[0]); ++i) {\n")
        f.write("    if (strcmp(SOCCER_EURO_LOGOS[i].code, code) == 0) return SOCCER_EURO_LOGOS[i].data;\n")
        f.write("  }\n")
        f.write("  for (size_t i = 0; i < sizeof(CBB_LOGOS) / sizeof(CBB_LOGOS[0]); ++i) {\n")
        f.write("    if (strcmp(CBB_LOGOS[i].code, code) == 0) return CBB_LOGOS[i].data;\n")
        f.write("  }\n")
        f.write("  for (size_t i = 0; i < sizeof(CFB_LOGOS) / sizeof(CFB_LOGOS[0]); ++i) {\n")
        f.write("    if (strcmp(CFB_LOGOS[i].code, code) == 0) return CFB_LOGOS[i].data;\n")
        f.write("  }\n")
        f.write("  for (size_t i = 0; i < sizeof(NBA_LOGOS) / sizeof(NBA_LOGOS[0]); ++i) {\n")
        f.write("    if (strcmp(NBA_LOGOS[i].code, code) == 0) return NBA_LOGOS[i].data;\n")
        f.write("  }\n")
        f.write("  for (size_t i = 0; i < sizeof(NHL_LOGOS) / sizeof(NHL_LOGOS[0]); ++i) {\n")
        f.write("    if (strcmp(NHL_LOGOS[i].code, code) == 0) return NHL_LOGOS[i].data;\n")
        f.write("  }\n")
        f.write("  for (size_t i = 0; i < sizeof(MLS_LOGOS) / sizeof(MLS_LOGOS[0]); ++i) {\n")
        f.write("    if (strcmp(MLS_LOGOS[i].code, code) == 0) return MLS_LOGOS[i].data;\n")
        f.write("  }\n")
        f.write("  return nullptr;\n")
        f.write("}\n")

    print(f"Successfully generated {OUT_HEADER} ({os.path.getsize(OUT_HEADER)} bytes)")

if __name__ == "__main__":
    main()
