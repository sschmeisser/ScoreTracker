#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "config.h"
#include "nfl_tracker.h"

// =============================================================================
// LIVE NFL SCORE FETCHER (ESPN Hidden JSON API over Wi-Fi)
// =============================================================================

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

// Convert ISO8601 UTC date string ("2026-09-18T00:15Z") to US Eastern Day & Date
inline void parse_iso_date(const char* iso, char* day_out, size_t day_sz, char* date_out, size_t date_sz) {
  if (!iso || strlen(iso) < 16) {
    strncpy(day_out, "SUN", day_sz - 1);
    strncpy(date_out, "SEP 20", date_sz - 1);
    return;
  }
  int y = 0, m = 0, d = 0, h = 0, min = 0;
  sscanf(iso, "%4d-%2d-%2dT%2d:%2d", &y, &m, &d, &h, &min);

  // If UTC hour < 4 (e.g. 00:15Z for prime time game), US local date is previous day
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
  int wday = (sy + sy / 4 - sy / 100 + sy / 400 + t[m - 1] + d) % 7;

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

static LiveGameStorage LIVE_STORAGE[32];
static NFLGame LIVE_NFL_GAMES[32];
static int live_nfl_games_count = 0;
static bool live_nfl_ready = false;
static uint32_t last_nfl_fetch_ms = 0;
static uint32_t last_wifi_reconnect_ms = 0;
static bool g_wifi_enabled = (ENABLE_WIFI != 0);

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
  while (true) {
    String header = client.readStringUntil('\n');
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
    Serial.printf("[ESPN] deserializeJson error: %s\n", err.c_str());
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
    strncpy(s.league, "NFL", sizeof(s.league) - 1);
    g.league = s.league;
    g.day = s.day;
    g.date = s.date;
    g.status = s.status;
    g.away_code = s.away_code;
    g.away_name = s.away_name;
    g.away_rec = s.away_rec;
    g.away_rank = 0;
    g.home_code = s.home_code;
    g.home_name = s.home_name;
    g.home_rec = s.home_rec;
    g.home_rank = 0;
    g.leader = s.leader;
    g.stadium = s.stadium;

    count++;
  }

  out_count = count;
  out_completed_count = completed;
  Serial.printf("[ESPN] Parsed %d games (Week %d, %d completed)\n", count, out_week, completed);
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
    Serial.printf("[ESPN] Current week %d is all pre-game; fetching completed games from week %d\n", week, week - 1);

    NFLGame prev_games[16];
    LiveGameStorage prev_storage[16];
    int prev_count = 0, prev_week = 0, prev_completed = 0;
    if (fetch_espn_endpoint(prev_path, prev_games, prev_storage, 16, prev_count, prev_week, prev_completed)) {
      int total = 0;
      for (int i = 0; i < prev_count && total < 32; ++i) {
        temp_storage[total] = prev_storage[i];
        temp_games[total] = prev_games[i];
        temp_games[total].league = temp_storage[total].league;
        temp_games[total].day = temp_storage[total].day;
        temp_games[total].date = temp_storage[total].date;
        temp_games[total].status = temp_storage[total].status;
        temp_games[total].away_code = temp_storage[total].away_code;
        temp_games[total].away_name = temp_storage[total].away_name;
        temp_games[total].away_rec = temp_storage[total].away_rec;
        temp_games[total].away_rank = prev_games[i].away_rank;
        temp_games[total].home_code = temp_storage[total].home_code;
        temp_games[total].home_name = temp_storage[total].home_name;
        temp_games[total].home_rec = temp_storage[total].home_rec;
        temp_games[total].home_rank = prev_games[i].home_rank;
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
    LIVE_NFL_GAMES[i].league = LIVE_STORAGE[i].league;
    LIVE_NFL_GAMES[i].day = LIVE_STORAGE[i].day;
    LIVE_NFL_GAMES[i].date = LIVE_STORAGE[i].date;
    LIVE_NFL_GAMES[i].status = LIVE_STORAGE[i].status;
    LIVE_NFL_GAMES[i].away_code = LIVE_STORAGE[i].away_code;
    LIVE_NFL_GAMES[i].away_name = LIVE_STORAGE[i].away_name;
    LIVE_NFL_GAMES[i].away_rec = LIVE_STORAGE[i].away_rec;
    LIVE_NFL_GAMES[i].away_rank = temp_games[i].away_rank;
    LIVE_NFL_GAMES[i].home_code = LIVE_STORAGE[i].home_code;
    LIVE_NFL_GAMES[i].home_name = LIVE_STORAGE[i].home_name;
    LIVE_NFL_GAMES[i].home_rec = LIVE_STORAGE[i].home_rec;
    LIVE_NFL_GAMES[i].home_rank = temp_games[i].home_rank;
    LIVE_NFL_GAMES[i].leader = LIVE_STORAGE[i].leader;
    LIVE_NFL_GAMES[i].stadium = LIVE_STORAGE[i].stadium;
  }
  live_nfl_games_count = count;
  live_nfl_ready = true;
  last_nfl_fetch_ms = millis();
  Serial.printf("[ESPN] Live NFL Tracker updated with %d live games!\n", count);
  return true;
}

inline void init_live_nfl() {
  if (!g_wifi_enabled) {
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

  Serial.printf("[WIFI] Connecting to %s...\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  last_wifi_reconnect_ms = millis();
}

inline void set_wifi_enabled(bool enabled) {
  g_wifi_enabled = enabled;
  if (!enabled) {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    Serial.println("[WIFI] Wi-Fi turned OFF.");
  } else {
    Serial.println("[WIFI] Wi-Fi turned ON. Initializing...");
    init_live_nfl();
  }
}

inline bool is_wifi_enabled() {
  return g_wifi_enabled;
}

inline void poll_live_nfl() {
  if (!g_wifi_enabled) return;
  uint32_t now = millis();

  if (WiFi.status() != WL_CONNECTED) {
    // Only re-trigger WiFi.begin if disconnected for at least 30 seconds
    if (now - last_wifi_reconnect_ms > 30000) {
      last_wifi_reconnect_ms = now;
      Serial.println("[WIFI] Re-attempting WiFi.begin...");
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

inline void clear_live_nfl_games() {
  live_nfl_games_count = 0;
  live_nfl_ready = false;
}

inline bool set_live_nfl_game(int idx,
                              const char* league,
                              const char* day, const char* date, const char* status,
                              const char* away_code, const char* away_name, const char* away_rec, uint16_t away_score, const uint8_t* away_q, uint8_t away_rank,
                              const char* home_code, const char* home_name, const char* home_rec, uint16_t home_score, const uint8_t* home_q, uint8_t home_rank,
                              const char* leader, const char* stadium) {
  if (idx < 0 || idx >= 32) return false;
  LiveGameStorage& s = LIVE_STORAGE[idx];
  NFLGame& g = LIVE_NFL_GAMES[idx];

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

inline void finalize_live_nfl_games(int count) {
  if (count > 32) count = 32;
  live_nfl_games_count = count;
  live_nfl_ready = (count > 0);
  last_nfl_fetch_ms = millis();
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

  return set_live_nfl_game(
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

    set_live_nfl_game(
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
  finalize_live_nfl_games(count);
  return count;
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
