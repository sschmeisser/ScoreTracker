content = '''#pragma once

#include <WiFi.h>
#include <ArduinoJson.h>
#include "config.h"
#include "nfl_tracker.h"

// =============================================================================
// LIVE NFL SCORE FETCHER (ESPN Hidden JSON API over Wi-Fi)
// =============================================================================

class WiFiChunkedStream {
 private:
  WiFiClient& _client;
  size_t _chunkRemaining = 0;
  bool _ended = false;

  void nextChunk() {
    if (_ended) return;
    String line = readLineWithTimeout();
    while (line.length() == 0 && !_ended) {
      if (!_client.connected() && !_client.available()) {
        _ended = true;
        return;
      }
      line = readLineWithTimeout();
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

  String readLineWithTimeout(uint32_t timeout_ms = 4000) {
    String s = "";
    uint32_t start = millis();
    while (millis() - start < timeout_ms) {
      while (_client.available()) {
        char c = (char)_client.read();
        start = millis();
        if (c == '\n') return s;
        if (c != '\r') s += c;
      }
      if (!_client.connected()) break;
      delay(1);
    }
    return s;
  }

 public:
  WiFiChunkedStream(WiFiClient& client) : _client(client) {
    nextChunk();
  }

  int read() {
    if (_ended) return -1;
    if (_chunkRemaining == 0) {
      // Consume trailing \r\n
      uint32_t start = millis();
      while (!_client.available()) {
        if (!_client.connected() || millis() - start > 3000) { _ended = true; return -1; }
        delay(1);
      }
      int c1 = _client.read();
      if (c1 == '\\r') {
        while (!_client.available()) {
          if (!_client.connected() || millis() - start > 3000) break;
          delay(1);
        }
        _client.read(); // '\\n'
      }
      nextChunk();
      if (_ended) return -1;
    }

    uint32_t start = millis();
    while (!_client.available()) {
      if (!_client.connected() || millis() - start > 3000) return -1;
      delay(1);
    }
    int c = _client.read();
    if (c >= 0) {
      _chunkRemaining--;
    }
    return c;
  }
};

namespace ArduinoJson {
namespace detail {
template <>
struct Reader<WiFiChunkedStream> {
  WiFiChunkedStream& _s;
  Reader(WiFiChunkedStream& s) : _s(s) {}
  int read() { return _s.read(); }
  size_t read(char* buffer, size_t length) {
    size_t i = 0;
    while (i < length) {
      int c = _s.read();
      if (c < 0) break;
      buffer[i++] = (char)c;
    }
    return i;
  }
};
}
}

// Convert ISO8601 UTC date string ("2026-09-18T00:15Z") to US Eastern Day & Date
inline void parse_iso_date(const char* iso, char* day_out, size_t day_sz, char* date_out, size_t date_sz) {
  if (!iso || strlen(iso) < 16) {
    strncpy(day_out, "SUN", day_sz - 1);
    strncpy(date_out, "SEP 20", date_sz - 1);
    return;
  }
  int y = 0, m = 0, d = 0, h = 0, min = 0;
  sscanf(iso, "%4d-%2d-%2dT%2d:%2d", &y, &m, &d, &h, &min);

  // If UTC hour < 4 (e.g. 00:15Z for prime time game), US local date is the previous day!
  if (h < 4) {
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
  int wday = (sy + sy/4 - sy/100 + sy/400 + t[m-1] + d) % 7;

  static const char* DAYS[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
  static const char* MONTHS[] = {"", "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

  if (wday >= 0 && wday < 7) {
    snprintf(day_out, day_sz, "%s", DAYS[wday]);
  } else {
    strncpy(day_out, "SUN", day_sz - 1);
  }
  if (m >= 1 && m <= 12) {
    snprintf(date_out, date_sz, "%s %02d", MONTHS[m], d);
  } else {
    strncpy(date_out, "SEP 20", date_sz - 1);
  }
}

struct LiveGameStorage {
  char day[8];
  char date[16];
  char status[16];
  char away_code[8];
  char away_name[24];
  char away_rec[12];
  char home_code[8];
  char home_name[24];
  char home_rec[12];
  char leader[48];
  char stadium[36];
};

static LiveGameStorage LIVE_STORAGE[32];
static NFLGame LIVE_NFL_GAMES[32];
static int live_nfl_games_count = 0;
static bool live_nfl_ready = false;
static uint32_t last_nfl_fetch_ms = 0;
static uint32_t last_wifi_reconnect_ms = 0;

inline bool fetch_espn_endpoint(const char* path, NFLGame* out_games, LiveGameStorage* out_storage, int max_games, int& out_count, int& out_week, int& out_completed_count) {
  if (WiFi.status() != WL_CONNECTED) return false;

  WiFiClient client;
  client.setTimeout(6);

  Serial.printf("[ESPN] Requesting %s...\n", path);
  if (!client.connect("site.api.espn.com", 80)) {
    Serial.println("[ESPN] Failed to connect to site.api.espn.com:80");
    return false;
  }

  char req[384];
  snprintf(req, sizeof(req),
           "GET %s HTTP/1.1\\r\\n"
           "Host: site.api.espn.com\\r\\n"
           "User-Agent: curl/8.0.0\\r\\n"
           "Connection: close\\r\\n\\r\\n", path);
  client.print(req);

  // Read status line
  String status_line = client.readStringUntil('\\n');
  status_line.trim();
  Serial.printf("[ESPN] HTTP Status: %s\\n", status_line.c_str());
  if (!status_line.startsWith("HTTP/1.1 200") && !status_line.startsWith("HTTP/1.0 200")) {
    client.stop();
    return false;
  }

  // Parse headers
  bool is_chunked = false;
  while (true) {
    String header = client.readStringUntil('\\n');
    header.trim();
    if (header.length() == 0) break;
    if (header.indexOf("Transfer-Encoding: chunked") >= 0 || header.indexOf("Transfer-Encoding:  chunked") >= 0) {
      is_chunked = true;
    }
  }

  // ArduinoJson filter setup
  JsonDocument filter;
  filter["week"]["number"] = true;
  filter["events"][0]["date"] = true;
  filter["events"][0]["status"]["type"]["shortDetail"] = true;
  filter["events"][0]["competitions"][0]["venue"]["fullName"] = true;
  filter["events"][0]["competitions"][0]["competitors"][0]["team"]["abbreviation"] = true;
  filter["events"][0]["competitions"][0]["competitors"][0]["team"]["shortDisplayName"] = true;
  filter["events"][0]["competitions"][0]["competitors"][0]["score"] = true;
  filter["events"][0]["competitions"][0]["competitors"][0]["records"][0]["summary"] = true;
  filter["events"][0]["competitions"][0]["competitors"][0]["linescores"][0]["value"] = true;
  filter["events"][0]["competitions"][0]["competitors"][0]["homeAway"] = true;
  filter["events"][0]["competitions"][0]["leaders"][0]["leaders"][0]["athlete"]["shortName"] = true;
  filter["events"][0]["competitions"][0]["leaders"][0]["leaders"][0]["displayValue"] = true;

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
    Serial.printf("[ESPN] deserializeJson error: %s\\n", err.c_str());
    return false;
  }

  out_week = doc["week"]["number"] | 0;
  JsonArray events = doc["events"].as<JsonArray>();
  int count = 0;
  int completed = 0;

  for (JsonObject ev : events) {
    if (count >= max_games) break;
    NFLGame& g = out_games[count];
    LiveGameStorage& s = out_storage[count];
    memset(&s, 0, sizeof(s));
    memset(&g, 0, sizeof(g));

    parse_iso_date(ev["date"] | "", s.day, sizeof(s.day), s.date, sizeof(s.date));

    const char* detail = ev["status"]["type"]["shortDetail"] | "FINAL";
    if (strstr(detail, "OT") != nullptr) {
      strncpy(s.status, "FINAL/OT", sizeof(s.status) - 1);
      completed++;
    } else if (strcasecmp(detail, "Final") == 0) {
      strncpy(s.status, "FINAL", sizeof(s.status) - 1);
      completed++;
    } else {
      strncpy(s.status, detail, sizeof(s.status) - 1);
    }

    JsonObject comp = ev["competitions"][0];
    const char* stadium = comp["venue"]["fullName"] | "NFL Stadium";
    strncpy(s.stadium, stadium, sizeof(s.stadium) - 1);

    for (JsonObject c : comp["competitors"].as<JsonArray>()) {
      const char* ha = c["homeAway"] | "";
      bool is_home = (strcmp(ha, "home") == 0);
      const char* code = c["team"]["abbreviation"] | (is_home ? "HOM" : "AWA");
      if (strcmp(code, "WSH") == 0) code = "WAS";
      const char* name = c["team"]["shortDisplayName"] | "";
      const char* rec = c["records"][0]["summary"] | "";
      int score = c["score"].as<int>();

      if (is_home) {
        strncpy(s.home_code, code, sizeof(s.home_code) - 1);
        strncpy(s.home_name, name, sizeof(s.home_name) - 1);
        strncpy(s.home_rec, rec, sizeof(s.home_rec) - 1);
        g.home_score = score;
        int q_idx = 0;
        for (JsonObject ls : c["linescores"].as<JsonArray>()) {
          if (q_idx < 4) g.home_q[q_idx++] = ls["value"].as<int>();
        }
      } else {
        strncpy(s.away_code, code, sizeof(s.away_code) - 1);
        strncpy(s.away_name, name, sizeof(s.away_name) - 1);
        strncpy(s.away_rec, rec, sizeof(s.away_rec) - 1);
        g.away_score = score;
        int q_idx = 0;
        for (JsonObject ls : c["linescores"].as<JsonArray>()) {
          if (q_idx < 4) g.away_q[q_idx++] = ls["value"].as<int>();
        }
      }
    }

    // Leaders
    JsonArray leaders = comp["leaders"].as<JsonArray>();
    if (leaders.size() > 0) {
      JsonObject top_l = leaders[0]["leaders"][0];
      const char* ath = top_l["athlete"]["shortName"] | "";
      const char* val = top_l["displayValue"] | "";
      if (strlen(ath) > 0 && strlen(val) > 0) {
        snprintf(s.leader, sizeof(s.leader), "%s: %s", ath, val);
      }
    }
    if (strlen(s.leader) == 0) {
      strncpy(s.leader, s.stadium, sizeof(s.leader) - 1);
    }

    // Point g pointers to s
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

  out_count = count;
  out_completed_count = completed;
  Serial.printf("[ESPN] Parsed %d games (Week %d, %d completed)\\n", count, out_week, completed);
  return (count > 0);
}

inline bool fetch_live_nfl_scores() {
  NFLGame temp_games[32];
  LiveGameStorage temp_storage[32];
  int count = 0, week = 0, completed = 0;

  // 1. Fetch current scoreboard (live week)
  bool ok = fetch_espn_endpoint("/apis/site/v2/sports/football/nfl/scoreboard", temp_games, temp_storage, 16, count, week, completed);
  if (!ok) return false;

  // 2. If current week has no completed games yet (e.g. Wednesday before kickoff),
  // fetch the past 7 days of completed games (Week - 1)
  if (completed == 0 && week > 1) {
    char prev_path[64];
    snprintf(prev_path, sizeof(prev_path), "/apis/site/v2/sports/football/nfl/scoreboard?week=%d", week - 1);
    Serial.printf("[ESPN] Current week %d is all pre-game; fetching completed games from week %d\\n", week, week - 1);

    NFLGame prev_games[16];
    LiveGameStorage prev_storage[16];
    int prev_count = 0, prev_week = 0, prev_completed = 0;
    if (fetch_espn_endpoint(prev_path, prev_games, prev_storage, 16, prev_count, prev_week, prev_completed)) {
      // Put completed games first, followed by the upcoming matchup for tonight!
      int total = 0;
      for (int i = 0; i < prev_count && total < 32; ++i) {
        temp_storage[total] = prev_storage[i];
        temp_games[total] = prev_games[i];
        // Re-point pointers to temp_storage[total]
        temp_games[total].day = temp_storage[total].day;
        temp_games[total].date = temp_storage[total].date;
        temp_games[total].status = temp_storage[total].status;
        temp_games[total].away_code = temp_storage[total].away_code;
        temp_games[total].away_name = temp_storage[total].away_name;
        temp_games[total].away_rec = temp_storage[total].away_rec;
        temp_games[total].home_code = temp_storage[total].home_code;
        temp_games[total].home_name = temp_storage[total].home_name;
        temp_games[total].home_rec = temp_storage[total].home_rec;
        temp_games[total].leader = temp_storage[total].leader;
        temp_games[total].stadium = temp_storage[total].stadium;
        total++;
      }
      count = total;
    }
  }

  // Atomically copy to LIVE_STORAGE and point LIVE_NFL_GAMES
  for (int i = 0; i < count; ++i) {
    LIVE_STORAGE[i] = temp_storage[i];
    LIVE_NFL_GAMES[i] = temp_games[i];
    LIVE_NFL_GAMES[i].day = LIVE_STORAGE[i].day;
    LIVE_NFL_GAMES[i].date = LIVE_STORAGE[i].date;
    LIVE_NFL_GAMES[i].status = LIVE_STORAGE[i].status;
    LIVE_NFL_GAMES[i].away_code = LIVE_STORAGE[i].away_code;
    LIVE_NFL_GAMES[i].away_name = LIVE_STORAGE[i].away_name;
    LIVE_NFL_GAMES[i].away_rec = LIVE_STORAGE[i].away_rec;
    LIVE_NFL_GAMES[i].home_code = LIVE_STORAGE[i].home_code;
    LIVE_NFL_GAMES[i].home_name = LIVE_STORAGE[i].home_name;
    LIVE_NFL_GAMES[i].home_rec = LIVE_STORAGE[i].home_rec;
    LIVE_NFL_GAMES[i].leader = LIVE_STORAGE[i].leader;
    LIVE_NFL_GAMES[i].stadium = LIVE_STORAGE[i].stadium;
  }
  live_nfl_games_count = count;
  live_nfl_ready = true;
  last_nfl_fetch_ms = millis();
  Serial.printf("[ESPN] Live NFL Tracker updated with %d live games!\\n", count);
  return true;
}

inline void init_live_nfl() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("[WIFI] Connecting to %s...\\n", WIFI_SSID);
  last_wifi_reconnect_ms = millis();
}

inline void poll_live_nfl() {
  uint32_t now = millis();

  if (WiFi.status() != WL_CONNECTED) {
    if (now - last_wifi_reconnect_ms > 10000) {
      last_wifi_reconnect_ms = now;
      Serial.println("[WIFI] Retrying connection...");
      WiFi.begin(WIFI_SSID, WIFI_PASS);
    }
    return;
  }

  // Once connected, fetch initial scores immediately or refresh every 90s
  uint32_t refresh_interval = 90000; // 90 seconds
  if (!live_nfl_ready || (now - last_nfl_fetch_ms > refresh_interval)) {
    last_nfl_fetch_ms = now;
    fetch_live_nfl_scores();
  }
}

inline const NFLGame* get_active_nfl_games() {
  if (live_nfl_ready && live_nfl_games_count > 0) {
    return LIVE_NFL_GAMES;
  }
  return NFL_GAMES;
}

inline int get_active_nfl_games_count() {
  if (live_nfl_ready && live_nfl_games_count > 0) {
    return live_nfl_games_count;
  }
  return TOTAL_NFL_GAMES;
}
'''

with open('src/live_nfl.h', 'w') as f:
    f.write(content)
print('Updated src/live_nfl.h successfully')
