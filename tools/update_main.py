with open('src/main.cpp', 'r') as f:
    c = f.read()

# 1. Add #include "live_nfl.h"
c = c.replace('#include "nfl_tracker.h"', '#include "nfl_tracker.h"\n#include "live_nfl.h"')

# 2. In setup(), add init_live_nfl()
c = c.replace('last_tick_ms = millis();', 'last_tick_ms = millis();\n  init_live_nfl();')

# 3. In loop(), poll live scores and get active games
old_loop_top = '''void loop() {
  uint32_t now_ms = millis();'''

new_loop_top = '''void loop() {
  uint32_t now_ms = millis();

  // Poll Wi-Fi & ESPN live score updates
  poll_live_nfl();
  const NFLGame* games = get_active_nfl_games();
  int total_games = get_active_nfl_games_count();'''

c = c.replace(old_loop_top, new_loop_top)

# 4. Replace TOTAL_NFL_GAMES in loop with total_games, and NFL_GAMES with games
c = c.replace('TOTAL_NFL_GAMES', 'total_games')
c = c.replace('NFL_GAMES[current_game_idx]', 'games[current_game_idx]')

# 5. Add REFRESH command
old_resume = '''} else if (cmd == "RESUME") {
      auto_tick_enabled = true;
      last_tick_ms = now_ms;
      Serial.println("[NFL] Auto-tick resumed");
    }'''

new_resume = '''} else if (cmd == "RESUME") {
      auto_tick_enabled = true;
      last_tick_ms = now_ms;
      Serial.println("[NFL] Auto-tick resumed");
    } else if (cmd == "REFRESH" || cmd == "SYNC") {
      Serial.println("[NFL] Manually triggering ESPN live fetch...");
      fetch_live_nfl_scores();
    }'''

c = c.replace(old_resume, new_resume)

# 6. Call draw_nfl_score_card with games and total_games
c = c.replace('draw_nfl_score_card(canvas, current_game_idx, elapsed);',
              'draw_nfl_score_card(canvas, current_game_idx, elapsed, games, total_games);')

with open('src/main.cpp', 'w') as f:
    f.write(c)

print('Updated src/main.cpp successfully!')
