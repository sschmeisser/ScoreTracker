#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "config.h"
#include "game_data.h"
#include "cyd_wifi_manager.h"

// =============================================================================
// BOARD-AGNOSTIC LIVE SPORTS SCORE / STORAGE / TRANSPORT LAYER
// =============================================================================
// Adapted from the round tracker's ../../src/live_nfl.h. Contains no drawing
// code and no reference to the round tracker's rendering functions. Internal
// globals are prefixed `cyd_` so this header can coexist with the round
// tracker in the same translation unit if ever needed.

class WiFiChunkedStream : public Stream {
 private:
  WiFiClient& _client;
  size_t _chunkRemaining = 0;
  bool _ended = false;

  int readByteWithTimeout(uint32_t timeout_ms = 4000) {
    uint32_t start = millis();
    while (!_client.available()) {
      if (!_client.connected() || millis() - start > timeout_ms) {
        _ended = true;
        return -1;
      }
      delay(1);
    }
    return _client.read();
  }

  void nextChunk() {
    if (_ended) return;
    String line = "";
    while (true) {
      int c = readByteWithTimeout();
      if (c < 0) { _ended = true; return; }
      if (c == '\n') break;
      if (c != '\r') line += (char)c;
    }
    line.trim();
    while (line.length() == 0 && !_ended) {
      while (true) {
        int c = readByteWithTimeout();
        if (c < 0) { _ended = true; return; }
        if (c == '\n') break;
        if (c != '\r') line += (char)c;
      }
      line.trim();
    }
    if (line.length() == 0) {
      _ended = true;
      return;
    }
    _chunkRemaining = strtol(line.c_str(), NULL, 16);
    if (_chunkRemaining == 0) {
      _ended = true;
    }
  }

 public:
  WiFiChunkedStream(WiFiClient& client) : _client(client) {
    nextChunk();
  }

  int available() override {
    if (_ended) return 0;
    return _client.available();
  }

  int read() override {
    if (_ended) return -1;
    if (_chunkRemaining == 0) {
      // Consume \r\n after chunk
      int c1 = readByteWithTimeout();
      if (c1 == '\r') {
        readByteWithTimeout(); // '\n'
      }
      nextChunk();
      if (_ended) return -1;
    }
    int c = readByteWithTimeout();
    if (c >= 0) {
      _chunkRemaining--;
    }
    return c;
  }

  int peek() override {
    if (_ended) return -1;
    return _client.peek();
  }

  void flush() override {}
  size_t write(uint8_t) override { return 0; }
};

// Convert ISO8601 UTC date string ("2026-10-31T20:00Z") to US Pacific Day, Date, and Kickoff Time
inline void parse_iso_pacific(const char* iso, char* day_out, size_t day_sz, char* date_out, size_t date_sz, char* time_out = nullptr, size_t time_sz = 0) {
  if (!iso || strlen(iso) < 16) {
    if (day_out && day_sz > 0) strncpy(day_out, "SUN", day_sz - 1);
    if (date_out && date_sz > 0) strncpy(date_out, "OCT 31", date_sz - 1);
    if (time_out && time_sz > 0) strncpy(time_out, "1:00 PM", time_sz - 1);
    return;
  }
  int y = 0, m = 0, d = 0, h = 0, min = 0;
  sscanf(iso, "%4d-%2d-%2dT%2d:%2d", &y, &m, &d, &h, &min);

  // US Pacific Time offset: UTC-7 (PDT, Mar-Nov) or UTC-8 (PST, Nov-Mar)
  int tz_offset = -7;
  if (m < 3 || (m == 3 && d < 8) || (m == 11 && d > 1) || m > 11) {
    tz_offset = -8;
  }

  h += tz_offset;
  if (h < 0) {
    h += 24;
    d--;
    if (d < 1) {
      m--;
      if (m < 1) { m = 12; y--; }
      static const int DAYS_IN_MONTH[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
      d = DAYS_IN_MONTH[m];
    }
  }

  // Sakamoto algorithm for day of week (0=Sun, 1=Mon, ..., 6=Sat)
  static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  int sy = y;
  if (m < 3) sy -= 1;
  int wday = (sy + sy / 4 - sy / 100 + sy / 400 + t[m - 1] + d) % 7;

  static const char* DAYS[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
  static const char* MONTHS[] = {"", "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

  if (day_out && day_sz > 0) {
    if (wday >= 0 && wday < 7) snprintf(day_out, day_sz, "%s", DAYS[wday]);
    else strncpy(day_out, "SUN", day_sz - 1);
  }
  if (date_out && date_sz > 0) {
    if (m >= 1 && m <= 12) snprintf(date_out, date_sz, "%s %02d", MONTHS[m], d);
    else strncpy(date_out, "OCT 31", date_sz - 1);
  }
  if (time_out && time_sz > 0) {
    int disp_h = h % 12;
    if (disp_h == 0) disp_h = 12;
    const char* ampm = (h >= 12) ? "PM" : "AM";
    snprintf(time_out, time_sz, "%d:%02d %s", disp_h, min, ampm);
  }
}

inline void parse_iso_date(const char* iso, char* day_out, size_t day_sz, char* date_out, size_t date_sz) {
  parse_iso_pacific(iso, day_out, day_sz, date_out, date_sz);
}

inline time_t utc_to_epoch(int y, int m, int d, int h, int min, int sec) {
  static const int days_before_month[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
  if (m < 1 || m > 12) return 0;
  int y_diff = y - 1970;
  int leap_days = (y - 1969) / 4 - (y - 1901) / 100 + (y - 1601) / 400;
  if ((y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)) && m <= 2) {
    leap_days--;
  }
  int total_days = y_diff * 365 + leap_days + days_before_month[m - 1] + (d - 1);
  return (time_t)total_days * 86400 + h * 3600 + min * 60 + sec;
}

inline time_t parse_iso_utc_epoch(const char* iso) {
  if (!iso || strlen(iso) < 16) return 0;
  int y = 0, m = 0, d = 0, h = 0, min = 0;
  if (sscanf(iso, "%4d-%2d-%2dT%2d:%2d", &y, &m, &d, &h, &min) < 5) return 0;
  return utc_to_epoch(y, m, d, h, min, 0);
}

struct LiveGameStorage {
  char league[8];
  char day[8];
  char date[16];
  char status[24];
  char away_code[8];
  char away_name[24];
  char away_rec[12];
  char home_code[8];
  char home_name[24];
  char home_rec[12];
  char leader[64];
  char stadium[48];
};

static LiveGameStorage CYD_LIVE_STORAGE[32];
static SportGame CYD_LIVE_GAMES[32];
static int cyd_live_games_count = 0;
static bool cyd_live_ready = false;
static uint32_t cyd_last_fetch_ms = 0;
static uint32_t cyd_last_wifi_reconnect_ms = 0;
static bool cyd_wifi_enabled = (ENABLE_WIFI != 0);

static char cyd_active_league[8] = "ALL";

inline void set_active_league(const char* league) {
  if (!league || !league[0]) return;
  strncpy(cyd_active_league, league, sizeof(cyd_active_league) - 1);
  cyd_active_league[sizeof(cyd_active_league) - 1] = '\0';
}

inline const char* get_active_league() {
  return cyd_active_league;
}

inline bool cyd_fetch_espn_endpoint(const char* path, const char* league_name, SportGame* out_games, LiveGameStorage* out_storage, int max_games, int& in_out_count, int& out_week, int& out_completed_count) {
  if (WiFi.status() != WL_CONNECTED) return false;

  WiFiClient client;
  client.setTimeout(6);

  Serial.printf("[ESPN] Requesting %s (%s)...\n", path, league_name ? league_name : "UNK");
  if (!client.connect("site.api.espn.com", 80)) {
    Serial.println("[ESPN] Failed to connect to site.api.espn.com:80");
    return false;
  }

  char req[384];
  snprintf(req, sizeof(req),
           "GET %s HTTP/1.1\r\n"
           "Host: site.api.espn.com\r\n"
           "User-Agent: curl/8.0.0\r\n"
           "Connection: close\r\n\r\n", path);
  client.print(req);

  // Read status line
  String status_line = client.readStringUntil('\n');
  status_line.trim();
  Serial.printf("[ESPN] HTTP Status: %s\n", status_line.c_str());
  if (!status_line.startsWith("HTTP/1.1 200") && !status_line.startsWith("HTTP/1.0 200")) {
    client.stop();
    return false;
  }

  // Parse headers
  bool is_chunked = false;
  time_t http_server_epoch = 0;
  while (true) {
    String header = client.readStringUntil('\n');
    header.trim();
    if (header.length() == 0) break;
    if (header.indexOf("Transfer-Encoding: chunked") >= 0 || header.indexOf("Transfer-Encoding:  chunked") >= 0) {
      is_chunked = true;
    }
    if (header.startsWith("Date:") || header.startsWith("date:")) {
      String d_str = header.substring(5);
      d_str.trim();
      int comma = d_str.indexOf(',');
      if (comma >= 0) d_str = d_str.substring(comma + 1);
      d_str.trim();
      char m_str[8];
      int day_v = 0, y_v = 0, h_v = 0, min_v = 0, s_v = 0;
      if (sscanf(d_str.c_str(), "%d %3s %d %d:%d:%d", &day_v, m_str, &y_v, &h_v, &min_v, &s_v) >= 6) {
        static const char* const MONTH_NAMES[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        int mon_idx = -1;
        for (int mi = 0; mi < 12; ++mi) {
          if (strcasecmp(m_str, MONTH_NAMES[mi]) == 0) { mon_idx = mi; break; }
        }
        if (mon_idx >= 0) {
          struct tm t;
          memset(&t, 0, sizeof(t));
          t.tm_year = y_v - 1900;
          t.tm_mon  = mon_idx;
          t.tm_mday = day_v;
          t.tm_hour = h_v;
          t.tm_min  = min_v;
          t.tm_sec  = s_v;
          http_server_epoch = utc_to_epoch(y_v, mon_idx + 1, day_v, h_v, min_v, s_v);
          if (http_server_epoch > 1700000000 && time(nullptr) < 1700000000) {
            struct timeval tv = { http_server_epoch, 0 };
            settimeofday(&tv, nullptr);
            Serial.printf("[TIME] Synced UTC time from HTTP Date header: %ld\n", (long)http_server_epoch);
          }
        }
      }
    }
  }

  // ArduinoJson filter setup
  JsonDocument filter;
  filter["week"]["number"] = true;
  filter["events"][0]["date"] = true;
  filter["events"][0]["status"]["type"]["state"] = true;
  filter["events"][0]["status"]["type"]["completed"] = true;
  filter["events"][0]["status"]["type"]["shortDetail"] = true;
  filter["events"][0]["competitions"][0]["venue"]["fullName"] = true;
  filter["events"][0]["competitions"][0]["competitors"][0]["team"]["abbreviation"] = true;
  filter["events"][0]["competitions"][0]["competitors"][0]["team"]["shortDisplayName"] = true;
  filter["events"][0]["competitions"][0]["competitors"][0]["score"] = true;
  filter["events"][0]["competitions"][0]["competitors"][0]["records"][0]["summary"] = true;
  filter["events"][0]["competitions"][0]["competitors"][0]["linescores"][0]["value"] = true;
  filter["events"][0]["competitions"][0]["competitors"][0]["homeAway"] = true;
  filter["events"][0]["competitions"][0]["competitors"][0]["curatedRank"]["current"] = true;
  filter["events"][0]["competitions"][0]["broadcasts"][0]["names"][0] = true;
  filter["events"][0]["competitions"][0]["leaders"][0]["leaders"][0]["athlete"]["shortName"] = true;
  filter["events"][0]["competitions"][0]["leaders"][0]["leaders"][0]["displayValue"] = true;
  filter["events"][0]["competitions"][0]["details"][0]["type"]["text"] = true;
  filter["events"][0]["competitions"][0]["details"][0]["clock"]["displayValue"] = true;
  filter["events"][0]["competitions"][0]["details"][0]["athletesInvolved"][0]["shortName"] = true;
  filter["events"][0]["competitions"][0]["competitors"][0]["leaders"][0]["name"] = true;
  filter["events"][0]["competitions"][0]["competitors"][0]["leaders"][0]["leaders"][0]["athlete"]["shortName"] = true;
  filter["events"][0]["competitions"][0]["competitors"][0]["leaders"][0]["leaders"][0]["displayValue"] = true;
  filter["events"][0]["competitions"][0]["competitors"][0]["leaders"][0]["leaders"][0]["value"] = true;

  JsonDocument doc;
  DeserializationError err;
  if (is_chunked) {
    WiFiChunkedStream chunked(client);
    err = deserializeJson(doc, chunked, DeserializationOption::Filter(filter), DeserializationOption::NestingLimit(20));
  } else {
    err = deserializeJson(doc, client, DeserializationOption::Filter(filter), DeserializationOption::NestingLimit(20));
  }
  client.stop();

  if (err) {
    Serial.printf("[ESPN] deserializeJson error: %s\n", err.c_str());
    return false;
  }

  out_week = doc["week"]["number"] | 0;
  JsonArray events = doc["events"].as<JsonArray>();
  int count = in_out_count;
  int completed = 0;

  for (JsonObject ev : events) {
    if (count >= max_games) break;
    SportGame& g = out_games[count];
    LiveGameStorage& s = out_storage[count];
    memset(&s, 0, sizeof(s));
    memset(&g, 0, sizeof(g));

    parse_iso_date(ev["date"] | "", s.day, sizeof(s.day), s.date, sizeof(s.date));

    const char* detail = ev["status"]["type"]["shortDetail"] | "FINAL";
    const char* state = ev["status"]["type"]["state"] | "";
    bool is_completed = ev["status"]["type"]["completed"] | false;
    time_t kickoff_epoch = parse_iso_utc_epoch(ev["date"] | "");

    bool is_live = (strcmp(state, "in") == 0) || (!is_completed && (
                   strstr(detail, "4th") != nullptr || strstr(detail, "1st") != nullptr ||
                   strstr(detail, "2nd") != nullptr || strstr(detail, "3rd") != nullptr ||
                   strstr(detail, "OT") != nullptr || strstr(detail, "HALFTIME") != nullptr ||
                   strstr(detail, "Half") != nullptr || strstr(detail, "LIVE") != nullptr));
    bool is_final = is_completed || (strcmp(state, "post") == 0) || (strstr(detail, "Final") != nullptr) || (strstr(detail, "FINAL") != nullptr);
    bool is_upcoming = !is_live && !is_final;

    // Filter rule:
    // 1. Live games: always keep
    // 2. Final games: drop if finished > 6 hours ago (or > 18h if specific league selected)
    // 3. Upcoming games: only keep if upcoming within next 12 hours (or 24h if specific league selected)
    time_t now_utc = time(nullptr);
    if (http_server_epoch > 1700000000 && now_utc < 1700000000) {
      now_utc = http_server_epoch;
    }
    if (now_utc > 1700000000 && kickoff_epoch > 0) {
      if (is_final) {
        time_t finish_est = kickoff_epoch + (time_t)(3.25 * 3600);
        int max_final_hours = (strcasecmp(cyd_active_league, "ALL") == 0) ? 6 : 18;
        if (now_utc > finish_est && (now_utc - finish_est) > (max_final_hours * 3600)) {
          continue; // Finished more than window ago
        }
      } else if (is_upcoming) {
        int max_upcoming_hours = (strcasecmp(cyd_active_league, "ALL") == 0) ? 12 : 24;
        if (kickoff_epoch > now_utc && (kickoff_epoch - now_utc) > (max_upcoming_hours * 3600)) {
          continue; // Upcoming more than window away
        }
      }
    }

    if (strstr(detail, "OT") != nullptr) {
      strncpy(s.status, "FINAL/OT", sizeof(s.status) - 1);
      completed++;
    } else if (strstr(detail, "SO") != nullptr) {
      strncpy(s.status, "FINAL/SO", sizeof(s.status) - 1);
      completed++;
    } else if (strcasecmp(detail, "Final") == 0) {
      strncpy(s.status, "FINAL", sizeof(s.status) - 1);
      completed++;
    } else if (strstr(detail, " - ") != nullptr && (strstr(detail, "PM") != nullptr || strstr(detail, "AM") != nullptr)) {
      // Upcoming kickoff: format in US Pacific Time (e.g. "10:00 AM")
      char p_time[16];
      parse_iso_pacific(ev["date"] | "", nullptr, 0, nullptr, 0, p_time, sizeof(p_time));
      strncpy(s.status, p_time, sizeof(s.status) - 1);
    } else if (strstr(detail, " - ") != nullptr) {
      // Live game detail e.g. "16:49 - 2nd" -> flip to "2nd 16:49"
      char part1[16] = {0}, part2[16] = {0};
      if (sscanf(detail, "%15[^-] - %15s", part1, part2) == 2) {
        char* p1 = part1; while (*p1 == ' ') p1++;
        int len1 = strlen(p1); while (len1 > 0 && p1[len1-1] == ' ') p1[--len1] = '\0';
        snprintf(s.status, sizeof(s.status), "%s %s", part2, p1);
      } else {
        strncpy(s.status, detail, sizeof(s.status) - 1);
      }
    } else {
      strncpy(s.status, detail, sizeof(s.status) - 1);
    }

    JsonObject comp = ev["competitions"][0];
    const char* stadium = comp["venue"]["fullName"] | "Stadium";
    strncpy(s.stadium, stadium, sizeof(s.stadium) - 1);

    for (JsonObject c : comp["competitors"].as<JsonArray>()) {
      const char* ha = c["homeAway"] | "";
      bool is_home = (strcmp(ha, "home") == 0);
      const char* code = c["team"]["abbreviation"] | (is_home ? "HOM" : "AWA");
      if (strcmp(code, "WSH") == 0 && strcmp(league_name ? league_name : "", "NFL") == 0) code = "WAS";
      const char* name = c["team"]["shortDisplayName"] | "";
      const char* rec = c["records"][0]["summary"] | "";
      int score = c["score"].as<int>();
      int rank = c["curatedRank"]["current"] | 0;

      if (is_home) {
        strncpy(s.home_code, code, sizeof(s.home_code) - 1);
        strncpy(s.home_name, name, sizeof(s.home_name) - 1);
        strncpy(s.home_rec, rec, sizeof(s.home_rec) - 1);
        g.home_score = score;
        if (rank >= 1 && rank <= 25) g.home_rank = rank;
        int q_idx = 0;
        for (JsonObject ls : c["linescores"].as<JsonArray>()) {
          if (q_idx < 4) g.home_q[q_idx++] = ls["value"].as<int>();
        }
      } else {
        strncpy(s.away_code, code, sizeof(s.away_code) - 1);
        strncpy(s.away_name, name, sizeof(s.away_name) - 1);
        strncpy(s.away_rec, rec, sizeof(s.away_rec) - 1);
        g.away_score = score;
        if (rank >= 1 && rank <= 25) g.away_rank = rank;
        int q_idx = 0;
        for (JsonObject ls : c["linescores"].as<JsonArray>()) {
          if (q_idx < 4) g.away_q[q_idx++] = ls["value"].as<int>();
        }
      }
    }

    // Leaders across sports (Football, Basketball, Hockey, Soccer)
    // 0. Soccer goal scorers (comp["details"])
    JsonArray comp_details = comp["details"].as<JsonArray>();
    if (comp_details.size() > 0) {
      char goals_buf[64] = "";
      int goals_found = 0;
      for (JsonObject d : comp_details) {
        const char* type_text = d["type"]["text"] | "";
        if (strstr(type_text, "Goal") != nullptr || strstr(type_text, "Penalty") != nullptr) {
          const char* ath = d["athletesInvolved"][0]["shortName"] | "";
          const char* clk = d["clock"]["displayValue"] | "";
          if (strlen(ath) > 0) {
            if (goals_found > 0 && strlen(goals_buf) + 2 < sizeof(goals_buf)) {
              strncat(goals_buf, ", ", sizeof(goals_buf) - strlen(goals_buf) - 1);
            }
            if (strlen(clk) > 0) {
              char single[32];
              snprintf(single, sizeof(single), "%s %s", ath, clk);
              strncat(goals_buf, single, sizeof(goals_buf) - strlen(goals_buf) - 1);
            } else {
              strncat(goals_buf, ath, sizeof(goals_buf) - strlen(goals_buf) - 1);
            }
            goals_found++;
            if (goals_found >= 2) break;
          }
        }
      }
      if (strlen(goals_buf) > 0) {
        snprintf(s.leader, sizeof(s.leader), "%s", goals_buf);
      }
    }

    // 1. Competition-level leaders (NFL, CFB)
    if (strlen(s.leader) == 0) {
      JsonArray comp_leaders = comp["leaders"].as<JsonArray>();
      if (comp_leaders.size() > 0) {
        JsonObject top_l = comp_leaders[0]["leaders"][0];
        const char* ath = top_l["athlete"]["shortName"] | "";
        const char* val = top_l["displayValue"] | "";
        if (strlen(ath) > 0 && strlen(val) > 0) {
          snprintf(s.leader, sizeof(s.leader), "%s: %s", ath, val);
        }
      }
    }

    // 2. Competitor-level leaders (NBA, CBB, NHL)
    if (strlen(s.leader) == 0) {
      const char* best_ath = nullptr;
      const char* best_val = nullptr;
      float best_pts = -1.0f;
      int stat_type = 0; // 0=raw/rating, 1=PTS, 2=G

      for (JsonObject c : comp["competitors"].as<JsonArray>()) {
        for (JsonObject cat : c["leaders"].as<JsonArray>()) {
          const char* cat_name = cat["name"] | "";
          JsonArray subs = cat["leaders"].as<JsonArray>();
          if (subs.size() == 0) continue;
          JsonObject top_sub = subs[0];
          const char* ath = top_sub["athlete"]["shortName"] | "";
          const char* val = top_sub["displayValue"] | "";
          float val_num = top_sub["value"].as<float>();

          if (strcmp(cat_name, "rating") == 0 && val_num > best_pts) {
            best_pts = val_num;
            best_ath = ath;
            best_val = val;
            stat_type = 0;
          } else if (strcmp(cat_name, "points") == 0 && val_num > best_pts) {
            best_pts = val_num;
            best_ath = ath;
            best_val = val;
            stat_type = 1;
          } else if (strcmp(cat_name, "goals") == 0 && val_num > best_pts && stat_type != 1) {
            best_pts = val_num;
            best_ath = ath;
            best_val = val;
            stat_type = 2;
          }
        }
      }

      if (best_ath && best_val && strlen(best_ath) > 0 && strlen(best_val) > 0) {
        if (stat_type == 1) {
          snprintf(s.leader, sizeof(s.leader), "%s: %s PTS", best_ath, best_val);
        } else if (stat_type == 2) {
          snprintf(s.leader, sizeof(s.leader), "%s: %s G", best_ath, best_val);
        } else {
          snprintf(s.leader, sizeof(s.leader), "%s: %s", best_ath, best_val);
        }
      }
    }

    // 3. Scheduled games: TV broadcast
    if (strlen(s.leader) == 0) {
      JsonArray broadcasts = comp["broadcasts"].as<JsonArray>();
      if (broadcasts.size() > 0) {
        const char* tv = broadcasts[0]["names"][0] | "";
        if (strlen(tv) > 0) {
          snprintf(s.leader, sizeof(s.leader), "TV: %s", tv);
        }
      }
    }

    // 4. Fallback to stadium
    if (strlen(s.leader) == 0) {
      strncpy(s.leader, s.stadium, sizeof(s.leader) - 1);
    }

    // Point g pointers to s
    strncpy(s.league, league_name ? league_name : "NFL", sizeof(s.league) - 1);
    g.league = s.league;
    g.day = s.day;
    g.date = s.date;
    g.status = s.status;
    g.away_code = s.away_code;
    g.away_name = s.away_name;
    g.away_rec = s.away_rec;
    g.home_code = s.home_code;
    g.home_name = s.home_name;
    g.home_rec = s.home_rec;
    g.leader = s.leader;
    g.stadium = s.stadium;

    count++;
  }

  in_out_count = count;
  out_completed_count = completed;
  Serial.printf("[ESPN] Parsed %d games (%s, %d completed)\n", count, league_name ? league_name : "", completed);
  return (count > 0);
}

inline bool fetch_live_scores() {
  static SportGame temp_games[32];
  static LiveGameStorage temp_storage[32];
  int count = 0, week = 0, completed = 0;

  if (strcasecmp(cyd_active_league, "NBA") == 0) {
    cyd_fetch_espn_endpoint("/apis/site/v2/sports/basketball/nba/scoreboard", "NBA", temp_games, temp_storage, 32, count, week, completed);
  } else if (strcasecmp(cyd_active_league, "NHL") == 0) {
    cyd_fetch_espn_endpoint("/apis/site/v2/sports/hockey/nhl/scoreboard", "NHL", temp_games, temp_storage, 32, count, week, completed);
  } else if (strcasecmp(cyd_active_league, "CBB") == 0) {
    cyd_fetch_espn_endpoint("/apis/site/v2/sports/basketball/mens-college-basketball/scoreboard", "CBB", temp_games, temp_storage, 32, count, week, completed);
  } else if (strcasecmp(cyd_active_league, "CFB") == 0) {
    cyd_fetch_espn_endpoint("/apis/site/v2/sports/football/college-football/scoreboard", "CFB", temp_games, temp_storage, 32, count, week, completed);
  } else if (strcasecmp(cyd_active_league, "NFL") == 0) {
    cyd_fetch_espn_endpoint("/apis/site/v2/sports/football/nfl/scoreboard", "NFL", temp_games, temp_storage, 32, count, week, completed);
  } else if (strcasecmp(cyd_active_league, "EPL") == 0) {
    cyd_fetch_espn_endpoint("/apis/site/v2/sports/soccer/eng.1/scoreboard", "EPL", temp_games, temp_storage, 32, count, week, completed);
  } else if (strcasecmp(cyd_active_league, "LAL") == 0 || strcasecmp(cyd_active_league, "LALIGA") == 0 || strcasecmp(cyd_active_league, "ESP") == 0) {
    cyd_fetch_espn_endpoint("/apis/site/v2/sports/soccer/esp.1/scoreboard", "LAL", temp_games, temp_storage, 32, count, week, completed);
  } else if (strcasecmp(cyd_active_league, "BUN") == 0 || strcasecmp(cyd_active_league, "BUNDESLIGA") == 0 || strcasecmp(cyd_active_league, "GER") == 0) {
    cyd_fetch_espn_endpoint("/apis/site/v2/sports/soccer/ger.1/scoreboard", "BUN", temp_games, temp_storage, 32, count, week, completed);
  } else if (strcasecmp(cyd_active_league, "UCL") == 0) {
    cyd_fetch_espn_endpoint("/apis/site/v2/sports/soccer/uefa.champions/scoreboard", "UCL", temp_games, temp_storage, 32, count, week, completed);
  } else if (strcasecmp(cyd_active_league, "UEL") == 0) {
    cyd_fetch_espn_endpoint("/apis/site/v2/sports/soccer/uefa.europa/scoreboard", "UEL", temp_games, temp_storage, 32, count, week, completed);
  } else if (strcasecmp(cyd_active_league, "MLS") == 0) {
    cyd_fetch_espn_endpoint("/apis/site/v2/sports/soccer/usa.1/scoreboard", "MLS", temp_games, temp_storage, 32, count, week, completed);
  } else if (strcasecmp(cyd_active_league, "SOCCER") == 0) {
    cyd_fetch_espn_endpoint("/apis/site/v2/sports/soccer/uefa.champions/scoreboard", "UCL", temp_games, temp_storage, 32, count, week, completed);
    if (count < 32) cyd_fetch_espn_endpoint("/apis/site/v2/sports/soccer/eng.1/scoreboard", "EPL", temp_games, temp_storage, 32, count, week, completed);
    if (count < 32) cyd_fetch_espn_endpoint("/apis/site/v2/sports/soccer/esp.1/scoreboard", "LAL", temp_games, temp_storage, 32, count, week, completed);
    if (count < 32) cyd_fetch_espn_endpoint("/apis/site/v2/sports/soccer/ger.1/scoreboard", "BUN", temp_games, temp_storage, 32, count, week, completed);
    if (count < 32) cyd_fetch_espn_endpoint("/apis/site/v2/sports/soccer/uefa.europa/scoreboard", "UEL", temp_games, temp_storage, 32, count, week, completed);
    if (count < 32) cyd_fetch_espn_endpoint("/apis/site/v2/sports/soccer/usa.1/scoreboard", "MLS", temp_games, temp_storage, 32, count, week, completed);
  } else {
    // "ALL": fetch active games across sports (NBA, NHL, CBB, NFL, UCL, EPL)
    cyd_fetch_espn_endpoint("/apis/site/v2/sports/basketball/nba/scoreboard", "NBA", temp_games, temp_storage, 32, count, week, completed);
    if (count < 32) {
      cyd_fetch_espn_endpoint("/apis/site/v2/sports/hockey/nhl/scoreboard", "NHL", temp_games, temp_storage, 32, count, week, completed);
    }
    if (count < 32) {
      cyd_fetch_espn_endpoint("/apis/site/v2/sports/basketball/mens-college-basketball/scoreboard", "CBB", temp_games, temp_storage, 32, count, week, completed);
    }
    if (count < 32) {
      cyd_fetch_espn_endpoint("/apis/site/v2/sports/football/nfl/scoreboard", "NFL", temp_games, temp_storage, 32, count, week, completed);
    }
    if (count < 32) {
      cyd_fetch_espn_endpoint("/apis/site/v2/sports/soccer/uefa.champions/scoreboard", "UCL", temp_games, temp_storage, 32, count, week, completed);
    }
    if (count < 32) {
      cyd_fetch_espn_endpoint("/apis/site/v2/sports/soccer/eng.1/scoreboard", "EPL", temp_games, temp_storage, 32, count, week, completed);
    }
  }

  if (count == 0) return false;

  // Atomically copy to CYD_LIVE_STORAGE and point CYD_LIVE_GAMES
  for (int i = 0; i < count; ++i) {
    CYD_LIVE_STORAGE[i] = temp_storage[i];
    CYD_LIVE_GAMES[i] = temp_games[i];
    CYD_LIVE_GAMES[i].league = CYD_LIVE_STORAGE[i].league;
    CYD_LIVE_GAMES[i].day = CYD_LIVE_STORAGE[i].day;
    CYD_LIVE_GAMES[i].date = CYD_LIVE_STORAGE[i].date;
    CYD_LIVE_GAMES[i].status = CYD_LIVE_STORAGE[i].status;
    CYD_LIVE_GAMES[i].away_code = CYD_LIVE_STORAGE[i].away_code;
    CYD_LIVE_GAMES[i].away_name = CYD_LIVE_STORAGE[i].away_name;
    CYD_LIVE_GAMES[i].away_rec = CYD_LIVE_STORAGE[i].away_rec;
    CYD_LIVE_GAMES[i].away_rank = temp_games[i].away_rank;
    CYD_LIVE_GAMES[i].home_code = CYD_LIVE_STORAGE[i].home_code;
    CYD_LIVE_GAMES[i].home_name = CYD_LIVE_STORAGE[i].home_name;
    CYD_LIVE_GAMES[i].home_rec = CYD_LIVE_STORAGE[i].home_rec;
    CYD_LIVE_GAMES[i].home_rank = temp_games[i].home_rank;
    CYD_LIVE_GAMES[i].leader = CYD_LIVE_STORAGE[i].leader;
    CYD_LIVE_GAMES[i].stadium = CYD_LIVE_STORAGE[i].stadium;
  }
  cyd_live_games_count = count;
  cyd_live_ready = (count > 0);
  cyd_last_fetch_ms = millis();
  Serial.printf("[ESPN] Live Tracker updated with %d active/recent games!\n", count);
  return true;
}

inline void cyd_init_wifi() {
  if (!cyd_wifi_enabled) {
    Serial.println("[WIFI] Wi-Fi disabled (USB Serial push mode active).");
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);

  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    switch (event) {
      case ARDUINO_EVENT_WIFI_STA_START:
        Serial.println("[WIFI] Station started");
        break;
      case ARDUINO_EVENT_WIFI_STA_CONNECTED:
        Serial.println("[WIFI] Associated with AP!");
        break;
      case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        Serial.printf("[WIFI] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
        configTzTime(TIMEZONE_PST, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);
        break;
      case ARDUINO_EVENT_WIFI_STA_DISCONNECTED: {
        static int last_disc_reason = -1;
        if (info.wifi_sta_disconnected.reason != last_disc_reason) {
          last_disc_reason = info.wifi_sta_disconnected.reason;
          Serial.printf("[WIFI] Disconnected, reason: %d\n", info.wifi_sta_disconnected.reason);
        }
        break;
      }

      default:
        break;
    }
  });

  WiFi.setTxPower(WIFI_POWER_19_5dBm);

  Serial.println("[WIFI] Scanning nearby 2.4GHz networks (active scan, 300ms/ch)...");
  int n = WiFi.scanNetworks(false, true, false, 300);
  Serial.printf("[WIFI] Found %d networks:\n", n);
  for (int i = 0; i < n; ++i) {
    Serial.printf("  SSID: '%s' | Ch: %d | RSSI: %d | Auth: %d\n",
                  WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i), WiFi.encryptionType(i));
  }

  char load_ssid[33];
  char load_pass[65];
  cyd_wifi_load_credentials(load_ssid, sizeof(load_ssid), load_pass, sizeof(load_pass));
  Serial.printf("[WIFI] Connecting to %s...\n", load_ssid);
  WiFi.begin(load_ssid, load_pass[0] ? load_pass : nullptr);
  cyd_last_wifi_reconnect_ms = millis();
}

inline void init_live_data() {
  cyd_init_wifi();
}

inline void set_wifi_enabled(bool enabled) {
  cyd_wifi_enabled = enabled;
  if (!enabled) {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    Serial.println("[WIFI] Wi-Fi turned OFF.");
  } else {
    Serial.println("[WIFI] Wi-Fi turned ON. Initializing...");
    cyd_init_wifi();
  }
}

inline bool is_wifi_enabled() {
  return cyd_wifi_enabled;
}

inline void poll_live_data(uint32_t interval_ms = 90000) {
  if (!cyd_wifi_enabled) return;
  uint32_t now = millis();

  if (WiFi.status() != WL_CONNECTED) {
    // Only re-trigger WiFi.begin if disconnected for at least 30 seconds
    if (now - cyd_last_wifi_reconnect_ms > 30000) {
      cyd_last_wifi_reconnect_ms = now;
      char recon_ssid[33];
      char recon_pass[65];
      cyd_wifi_load_credentials(recon_ssid, sizeof(recon_ssid), recon_pass, sizeof(recon_pass));
      Serial.printf("[WIFI] Re-attempting WiFi.begin to %s...\n", recon_ssid);
      WiFi.begin(recon_ssid, recon_pass[0] ? recon_pass : nullptr);
    }
    return;
  }

  // Once connected, fetch initial scores immediately or refresh at interval
  uint32_t refresh_interval = interval_ms;
  if (!cyd_live_ready || (now - cyd_last_fetch_ms > refresh_interval)) {
    cyd_last_fetch_ms = now;
    fetch_live_scores();
  }
}

inline void clear_live_games() {
  cyd_live_games_count = 0;
  cyd_live_ready = false;
}

inline bool set_live_game(int idx,
                          const char* league,
                          const char* day, const char* date, const char* status,
                          const char* away_code, const char* away_name, const char* away_rec, uint16_t away_score, const uint8_t* away_q, uint8_t away_rank,
                          const char* home_code, const char* home_name, const char* home_rec, uint16_t home_score, const uint8_t* home_q, uint8_t home_rank,
                          const char* leader, const char* stadium) {
  if (idx < 0 || idx >= 32) return false;
  LiveGameStorage& s = CYD_LIVE_STORAGE[idx];
  SportGame& g = CYD_LIVE_GAMES[idx];

  memset(&s, 0, sizeof(s));
  memset(&g, 0, sizeof(g));

  if (league && strlen(league) > 0) strncpy(s.league, league, sizeof(s.league) - 1);
  else strncpy(s.league, "NFL", sizeof(s.league) - 1);

  if (day) strncpy(s.day, day, sizeof(s.day) - 1);
  if (date) strncpy(s.date, date, sizeof(s.date) - 1);
  if (status) strncpy(s.status, status, sizeof(s.status) - 1);
  if (away_code) strncpy(s.away_code, away_code, sizeof(s.away_code) - 1);
  if (away_name) strncpy(s.away_name, away_name, sizeof(s.away_name) - 1);
  if (away_rec) strncpy(s.away_rec, away_rec, sizeof(s.away_rec) - 1);
  if (home_code) strncpy(s.home_code, home_code, sizeof(s.home_code) - 1);
  if (home_name) strncpy(s.home_name, home_name, sizeof(s.home_name) - 1);
  if (home_rec) strncpy(s.home_rec, home_rec, sizeof(s.home_rec) - 1);
  if (leader) strncpy(s.leader, leader, sizeof(s.leader) - 1);
  if (stadium) strncpy(s.stadium, stadium, sizeof(s.stadium) - 1);

  g.league = s.league;
  g.day = s.day;
  g.date = s.date;
  g.status = s.status;
  g.away_code = s.away_code;
  g.away_name = s.away_name;
  g.away_rec = s.away_rec;
  g.away_score = away_score;
  g.away_rank = away_rank;
  if (away_q) {
    for (int q = 0; q < 4; ++q) g.away_q[q] = away_q[q];
  }
  g.home_code = s.home_code;
  g.home_name = s.home_name;
  g.home_rec = s.home_rec;
  g.home_score = home_score;
  g.home_rank = home_rank;
  if (home_q) {
    for (int q = 0; q < 4; ++q) g.home_q[q] = home_q[q];
  }
  g.leader = s.leader;
  g.stadium = s.stadium;

  return true;
}

inline void finalize_live_games(int count) {
  if (count > 32) count = 32;
  cyd_live_games_count = count;
  cyd_live_ready = (count > 0);
  cyd_last_fetch_ms = millis();
}

inline bool parse_and_store_game_json(int idx, const char* json_str) {
  if (idx < 0 || idx >= 32) return false;
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, json_str);
  if (err) {
    Serial.printf("[SERIAL_SCORES] JSON error on game %d: %s (len=%d: '%.40s...')\n",
                  idx, err.c_str(), (int)strlen(json_str), json_str);
    return false;
  }

  uint8_t aq[4] = {0};
  uint8_t hq[4] = {0};
  JsonArray aq_arr = doc["away_q"].as<JsonArray>();
  for (size_t q = 0; q < aq_arr.size() && q < 4; ++q) aq[q] = aq_arr[q].as<uint8_t>();
  JsonArray hq_arr = doc["home_q"].as<JsonArray>();
  for (size_t q = 0; q < hq_arr.size() && q < 4; ++q) hq[q] = hq_arr[q].as<uint8_t>();

  const char* away_c = doc["away"] | doc["away_code"] | "";
  const char* home_c = doc["home"] | doc["home_code"] | "";

  return set_live_game(
    idx,
    doc["league"] | "NFL",
    doc["day"] | "SUN",
    doc["date"] | "",
    doc["status"] | "FINAL",
    away_c,
    doc["away_name"] | away_c,
    doc["away_rec"] | "",
    doc["away_score"] | 0,
    aq,
    doc["away_rank"] | 0,
    home_c,
    doc["home_name"] | home_c,
    doc["home_rec"] | "",
    doc["home_score"] | 0,
    hq,
    doc["home_rank"] | 0,
    doc["leader"] | "",
    doc["stadium"] | ""
  );
}

inline int parse_and_store_games_json_array(const char* json_str) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, json_str);
  if (err) {
    Serial.printf("[SERIAL_SCORES] JSON array error: %s\n", err.c_str());
    return -1;
  }
  JsonArray arr = doc.as<JsonArray>();
  int count = 0;
  for (JsonObject item : arr) {
    if (count >= 32) break;
    uint8_t aq[4] = {0};
    uint8_t hq[4] = {0};
    JsonArray aq_arr = item["away_q"].as<JsonArray>();
    for (size_t q = 0; q < aq_arr.size() && q < 4; ++q) aq[q] = aq_arr[q].as<uint8_t>();
    JsonArray hq_arr = item["home_q"].as<JsonArray>();
    for (size_t q = 0; q < hq_arr.size() && q < 4; ++q) hq[q] = hq_arr[q].as<uint8_t>();

    const char* away_c = item["away"] | item["away_code"] | "";
    const char* home_c = item["home"] | item["home_code"] | "";

    set_live_game(
      count,
      item["league"] | "NFL",
      item["day"] | "SUN",
      item["date"] | "",
      item["status"] | "FINAL",
      away_c,
      item["away_name"] | away_c,
      item["away_rec"] | "",
      item["away_score"] | 0,
      aq,
      item["away_rank"] | 0,
      home_c,
      item["home_name"] | home_c,
      item["home_rec"] | "",
      item["home_score"] | 0,
      hq,
      item["home_rank"] | 0,
      item["leader"] | "",
      item["stadium"] | ""
    );
    count++;
  }
  finalize_live_games(count);
  return count;
}

inline const SportGame* get_active_games() {
  if (cyd_live_ready && cyd_live_games_count > 0) {
    return CYD_LIVE_GAMES;
  }
  return CYD_DEFAULT_GAMES;
}

inline int get_active_games_count() {
  if (cyd_live_ready && cyd_live_games_count > 0) {
    return cyd_live_games_count;
  }
  return CYD_DEFAULT_GAMES_COUNT;
}
