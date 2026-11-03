#pragma once

#include <Arduino.h>
#include <LovyanGFX.hpp>

// =============================================================================
// BOARD-AGNOSTIC SPORT GAME DATA MODEL + FALLBACK GAMES
// =============================================================================
// Field order/types mirror the round tracker's NFLGame exactly, but the name
// SportGame reflects that this project is board- and league-agnostic.

struct SportGame {
  const char* league;
  const char* day;
  const char* date;
  const char* status;
  const char* away_code;
  const char* away_name;
  const char* away_rec;
  uint16_t    away_score;
  uint8_t     away_q[4];
  uint8_t     away_rank;
  const char* home_code;
  const char* home_name;
  const char* home_rec;
  uint16_t    home_score;
  uint8_t     home_q[4];
  uint8_t     home_rank;
  const char* leader;
  const char* stadium;
};

// 6 board-agnostic fallback games spanning multiple leagues (2 NFL, 1 NBA,
// 1 NHL, 1 MLS, 1 CFB). Header-static and fully string-literal so no pointers
// can dangle.
#define CYD_DEFAULT_GAMES_COUNT 6
static const SportGame CYD_DEFAULT_GAMES[CYD_DEFAULT_GAMES_COUNT] = {
  // 1. NFL — Sunday afternoon showcase
  {
    .league = "NFL", .day = "SUN", .date = "SEP 20", .status = "FINAL",
    .away_code = "BUF", .away_name = "Bills", .away_rec = "1-0", .away_score = 34, .away_q = {3, 7, 14, 10}, .away_rank = 0,
    .home_code = "KC",  .home_name = "Chiefs", .home_rec = "1-1", .home_score = 31, .home_q = {7, 10, 3, 11}, .home_rank = 0,
    .leader = "Allen: 268 YDS, 4 TOTAL TD", .stadium = "GEHA Field at Arrowhead"
  },
  // 2. NFL — Sunday Night Football
  {
    .league = "NFL", .day = "SUN", .date = "SEP 20", .status = "FINAL/OT",
    .away_code = "SF",  .away_name = "49ers", .away_rec = "2-0", .away_score = 26, .away_q = {3, 13, 0, 10}, .away_rank = 0,
    .home_code = "DAL", .home_name = "Cowboys", .home_rec = "1-1", .home_score = 23, .home_q = {7, 7, 3, 6}, .home_rank = 0,
    .leader = "McCaffrey: 121 YDS, 2 TD", .stadium = "AT&T Stadium"
  },
  // 3. NBA — Western Conference clash
  {
    .league = "NBA", .day = "THU", .date = "OCT 22", .status = "FINAL",
    .away_code = "BOS", .away_name = "Celtics", .away_rec = "2-0", .away_score = 112, .away_q = {28, 31, 26, 27}, .away_rank = 0,
    .home_code = "LAL", .home_name = "Lakers",  .home_rec = "1-1", .home_score = 105, .home_q = {24, 27, 29, 25}, .home_rank = 0,
    .leader = "Tatum: 34 PTS, 9 REB", .stadium = "Crypto.com Arena"
  },
  // 4. NHL — Original Six rivalry
  {
    .league = "NHL", .day = "SAT", .date = "OCT 24", .status = "FINAL/OT",
    .away_code = "TOR", .away_name = "Maple Leafs", .away_rec = "3-1-1", .away_score = 4, .away_q = {1, 2, 0, 1}, .away_rank = 0,
    .home_code = "MTL", .home_name = "Canadiens",   .home_rec = "2-2-1", .home_score = 3, .home_q = {1, 1, 1, 0}, .home_rank = 0,
    .leader = "Marner: 2G, 1A", .stadium = "Bell Centre"
  },
  // 5. MLS — Western/Eastern crossover
  {
    .league = "MLS", .day = "WED", .date = "SEP 30", .status = "FINAL",
    .away_code = "SEA", .away_name = "Sounders", .away_rec = "14-9-6", .away_score = 2, .away_q = {1, 1, 0, 0}, .away_rank = 0,
    .home_code = "ATL", .home_name = "United",   .home_rec = "12-11-6", .home_score = 1, .home_q = {0, 1, 0, 0}, .home_rank = 0,
    .leader = "Almada: 1G, 1A", .stadium = "Mercedes-Benz Stadium"
  },
  // 6. CFB — Top-10 ranked matchup
  {
    .league = "CFB", .day = "SAT", .date = "OCT 24", .status = "FINAL",
    .away_code = "ORE", .away_name = "Ducks", .away_rec = "7-0", .away_score = 28, .away_q = {7, 14, 0, 7}, .away_rank = 3,
    .home_code = "PSU", .home_name = "Nittany Lions", .home_rec = "6-1", .home_score = 24, .home_q = {10, 7, 7, 0}, .home_rank = 5,
    .leader = "Gabriel: 244 YDS, 3 TD", .stadium = "Beaver Stadium"
  }
};
