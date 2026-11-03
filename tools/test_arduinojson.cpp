#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <ArduinoJson.h>

struct NFLGame {
  char day[8];
  char date[16];
  char status[16];
  char away_code[8];
  char away_name[24];
  char away_rec[12];
  uint8_t away_score;
  uint8_t away_q[4];
  char home_code[8];
  char home_name[24];
  char home_rec[12];
  uint8_t home_score;
  uint8_t home_q[4];
  char leader[48];
  char stadium[36];
};

int main() {
  std::ifstream file("tools/espn_week2.json");
  if (!file.is_open()) {
    std::cerr << "Cannot open tools/espn_week2.json" << std::endl;
    return 1;
  }

  // Filter definition
  JsonDocument filter;
  filter["week"]["number"] = true;
  filter["events"][0]["date"] = true;
  filter["events"][0]["status"]["type"]["shortDetail"] = true;
  filter["events"][0]["status"]["type"]["state"] = true;
  filter["events"][0]["status"]["type"]["completed"] = true;
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
  DeserializationError err = deserializeJson(doc, file, DeserializationOption::Filter(filter), DeserializationOption::NestingLimit(20));
  if (err) {
    std::cerr << "deserializeJson failed: " << err.c_str() << std::endl;
    return 1;
  }

  std::cout << "Doc memory usage: " << doc.size() << " elements" << std::endl;
  std::cout << "Week: " << doc["week"]["number"].as<int>() << std::endl;

  JsonArray events = doc["events"].as<JsonArray>();
  std::cout << "Number of events: " << events.size() << std::endl;

  std::vector<NFLGame> games;
  for (JsonObject ev : events) {
    NFLGame g;
    memset(&g, 0, sizeof(g));

    const char* date_str = ev["date"] | "";
    // e.g. 2026-09-18T00:15Z -> day of week / date
    strncpy(g.day, "SUN", sizeof(g.day) - 1);
    strncpy(g.date, "SEP 20", sizeof(g.date) - 1);

    const char* detail = ev["status"]["type"]["shortDetail"] | "FINAL";
    if (strstr(detail, "OT") != nullptr) {
      strncpy(g.status, "FINAL/OT", sizeof(g.status) - 1);
    } else if (strcasecmp(detail, "Final") == 0) {
      strncpy(g.status, "FINAL", sizeof(g.status) - 1);
    } else {
      strncpy(g.status, detail, sizeof(g.status) - 1);
    }

    JsonObject comp = ev["competitions"][0];
    const char* stadium = comp["venue"]["fullName"] | "NFL Stadium";
    strncpy(g.stadium, stadium, sizeof(g.stadium) - 1);

    for (JsonObject c : comp["competitors"].as<JsonArray>()) {
      const char* ha = c["homeAway"] | "";
      bool is_home = (strcmp(ha, "home") == 0);
      const char* code = c["team"]["abbreviation"] | (is_home ? "HOM" : "AWA");
      if (strcmp(code, "WSH") == 0) code = "WAS";
      const char* name = c["team"]["shortDisplayName"] | "";
      const char* rec = c["records"][0]["summary"] | "";
      int score = c["score"].as<int>();

      if (is_home) {
        strncpy(g.home_code, code, sizeof(g.home_code) - 1);
        strncpy(g.home_name, name, sizeof(g.home_name) - 1);
        strncpy(g.home_rec, rec, sizeof(g.home_rec) - 1);
        g.home_score = score;
        int q_idx = 0;
        for (JsonObject ls : c["linescores"].as<JsonArray>()) {
          if (q_idx < 4) g.home_q[q_idx++] = ls["value"].as<int>();
        }
      } else {
        strncpy(g.away_code, code, sizeof(g.away_code) - 1);
        strncpy(g.away_name, name, sizeof(g.away_name) - 1);
        strncpy(g.away_rec, rec, sizeof(g.away_rec) - 1);
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
        snprintf(g.leader, sizeof(g.leader), "%s: %s", ath, val);
      }
    }
    if (strlen(g.leader) == 0) {
      strncpy(g.leader, g.stadium, sizeof(g.leader) - 1);
    }

    games.push_back(g);
  }

  std::cout << "Successfully extracted " << games.size() << " games!" << std::endl;
  for (size_t i = 0; i < std::min((size_t)5, games.size()); ++i) {
    const auto& g = games[i];
    std::cout << i+1 << ". " << g.away_code << " " << (int)g.away_score << " @ " 
              << g.home_code << " " << (int)g.home_score << " (" << g.status << ") | " 
              << g.leader << " | " << g.stadium << std::endl;
  }
  return 0;
}
