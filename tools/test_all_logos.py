from PIL import Image, ImageDraw
import preview_nfl_v3 as p3

games = [
    {"index": 0, "day": "Thu", "date": "Sep 17", "status": "FINAL", "away_code": "BAL", "away_rec": "0-1", "away_score": 20, "away_q": [7,3,7,3], "home_code": "KC", "home_rec": "1-0", "home_score": 27, "home_q": [7,10,3,7], "leader": "Mahomes: 291 YDS, 2 TD", "stadium": "GEHA Field at Arrowhead"},
    {"index": 1, "day": "Fri", "date": "Sep 18", "status": "FINAL", "away_code": "GB", "away_rec": "0-1", "away_score": 29, "away_q": [6,13,7,3], "home_code": "PHI", "home_rec": "1-0", "home_score": 34, "home_q": [0,14,14,6], "leader": "Barkley: 109 YDS, 3 TD", "stadium": "Arena Corinthians (BR)"},
    {"index": 2, "day": "Sun", "date": "Sep 20", "status": "FINAL / OT", "away_code": "LAR", "away_rec": "0-1", "away_score": 20, "away_q": [3,0,14,3], "home_code": "DET", "home_rec": "1-0", "home_score": 26, "home_q": [3,14,0,3], "leader": "Montgomery: 91 YDS, 1 TD", "stadium": "Ford Field"},
    {"index": 3, "day": "Mon", "date": "Sep 21", "status": "FINAL", "away_code": "NYJ", "away_rec": "0-1", "away_score": 19, "away_q": [7,6,0,6], "home_code": "SF", "home_rec": "1-0", "home_score": 32, "home_q": [3,13,10,6], "leader": "Mason: 147 YDS, 1 TD", "stadium": "Levi's Stadium"}
]

for g in games:
    img = p3.render_score_card(g)
    img.save(f"tools/preview_game_{g['index']}_{g['away_code']}_{g['home_code']}.png")
print("Saved all preview games!")
