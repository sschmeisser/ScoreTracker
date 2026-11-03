#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
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

void parse_iso_date(const char* iso, char* day_out, size_t day_sz, char* date_out, size_t date_sz) {
  if (!iso || strlen(iso) < 16) {
    strncpy(day_out, "SUN", day_sz);
    strncpy(date_out, "SEP 20", date_sz);
    return;
  }
  int y = 0, m = 0, d = 0, h = 0, min = 0;
  sscanf(iso, "%4d-%2d-%2dT%2d:%2d", &y, &m, &d, &h, &min);

  struct tm tm_utc;
  memset(&tm_utc, 0, sizeof(tm_utc));
  tm_utc.tm_year = y - 1900;
  tm_utc.tm_mon = m - 1;
  tm_utc.tm_mday = d;
  tm_utc.tm_hour = h;
  tm_utc.tm_min = min;

  time_t t = timegm(&tm_utc);
  t -= 4 * 3600; // US Eastern
  struct tm tm_local;
  gmtime_r(&t, &tm_local);

  static const char* DAYS[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
  static const char* MONTHS[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

  snprintf(day_out, day_sz, "%s", DAYS[tm_local.tm_wday]);
  snprintf(date_out, date_sz, "%s %02d", MONTHS[tm_local.tm_mon], tm_local.tm_mday);
}

class SocketStream {
  int _fd;
 public:
  SocketStream(int fd) : _fd(fd) {}
  int read() {
    unsigned char c;
    int n = ::read(_fd, &c, 1);
    if (n <= 0) return -1;
    return c;
  }
  std::string readLine() {
    std::string s;
    while (true) {
      int c = read();
      if (c < 0) break;
      if (c == '\n') break;
      if (c != '\r') s += (char)c;
    }
    return s;
  }
};

class ChunkedStream {
  SocketStream& _sock;
  size_t _chunkRemaining = 0;
  bool _ended = false;

  void nextChunk() {
    if (_ended) return;
    std::string line = _sock.readLine();
    while (line.empty() && !_ended) {
      line = _sock.readLine();
    }
    if (line.empty()) { _ended = true; return; }
    _chunkRemaining = strtol(line.c_str(), NULL, 16);
    if (_chunkRemaining == 0) {
      _ended = true;
    }
  }

 public:
  ChunkedStream(SocketStream& s) : _sock(s) {
    nextChunk();
  }

  int read() {
    if (_ended) return -1;
    if (_chunkRemaining == 0) {
      int c1 = _sock.read();
      if (c1 == '\r') { _sock.read(); }
      nextChunk();
      if (_ended) return -1;
    }
    int c = _sock.read();
    if (c >= 0) {
      _chunkRemaining--;
    }
    return c;
  }
};

namespace ArduinoJson {
namespace detail {
template <>
struct Reader<ChunkedStream> {
  ChunkedStream& _s;
  Reader(ChunkedStream& s) : _s(s) {}
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

int main() {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  struct hostent* he = gethostbyname("site.api.espn.com");
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(80);
  memcpy(&addr.sin_addr, he->h_addr_list[0], sizeof(addr.sin_addr));
  connect(fd, (struct sockaddr*)&addr, sizeof(addr));

  std::string req = 
    "GET /apis/site/v2/sports/football/nfl/scoreboard HTTP/1.1\r\n"
    "Host: site.api.espn.com\r\n"
    "User-Agent: curl/8.0.0\r\n"
    "Connection: close\r\n\r\n";
  write(fd, req.data(), req.size());

  SocketStream sock(fd);
  std::string status = sock.readLine(); std::cout << "Status line: " << status << std::endl;
  while (true) {
    std::string h = sock.readLine();
    std::cout << "H: " << h << std::endl; if (h.empty()) break;
  }

  ChunkedStream chunked(sock);

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
  DeserializationError err = deserializeJson(doc, chunked, DeserializationOption::Filter(filter), DeserializationOption::NestingLimit(20));
  close(fd);

  if (err) {
    std::cerr << "deserializeJson error: " << err.c_str() << std::endl;
    return 1;
  }

  JsonArray events = doc["events"].as<JsonArray>();
  std::vector<NFLGame> games;
  for (JsonObject ev : events) {
    NFLGame g;
    memset(&g, 0, sizeof(g));

    parse_iso_date(ev["date"] | "", g.day, sizeof(g.day), g.date, sizeof(g.date));

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
  for (size_t i = 0; i < games.size(); ++i) {
    const auto& g = games[i];
    std::cout << i+1 << ". [" << g.day << " " << g.date << "] " 
              << g.away_code << " " << (int)g.away_score << " (" 
              << (int)g.away_q[0] << "-" << (int)g.away_q[1] << "-" << (int)g.away_q[2] << "-" << (int)g.away_q[3] << ") @ " 
              << g.home_code << " " << (int)g.home_score << " ("
              << (int)g.home_q[0] << "-" << (int)g.home_q[1] << "-" << (int)g.home_q[2] << "-" << (int)g.home_q[3] << ") ["
              << g.status << "] | " << g.leader << " | " << g.stadium << std::endl;
  }
  return 0;
}
