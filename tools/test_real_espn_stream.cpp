#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <ArduinoJson.h>

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
      // Consume trailing \r\n
      int c1 = _sock.read();
      if (c1 == '\r') { _sock.read(); } // \n
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
  if (!he) { std::cerr << "DNS error\n"; return 1; }

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(80);
  memcpy(&addr.sin_addr, he->h_addr_list[0], sizeof(addr.sin_addr));

  if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
    std::cerr << "Connect error\n"; return 1;
  }

  std::string req = 
    "GET /apis/site/v2/sports/football/nfl/scoreboard?week=2 HTTP/1.1\r\n"
    "Host: site.api.espn.com\r\n"
    "User-Agent: curl/8.0.0\r\n"
    "Connection: close\r\n\r\n";
  write(fd, req.data(), req.size());

  SocketStream sock(fd);
  // Read HTTP response status & headers
  std::string status = sock.readLine();
  std::cout << "Status: " << status << std::endl;
  while (true) {
    std::string h = sock.readLine();
    if (h.empty()) break; // empty line terminates headers
  }

  ChunkedStream chunked(sock);

  JsonDocument filter;
  filter["week"]["number"] = true;
  filter["events"][0]["date"] = true;
  filter["events"][0]["status"]["type"]["shortDetail"] = true;
  filter["events"][0]["competitions"][0]["venue"]["fullName"] = true;
  filter["events"][0]["competitions"][0]["competitors"][0]["team"]["abbreviation"] = true;
  filter["events"][0]["competitions"][0]["competitors"][0]["score"] = true;
  filter["events"][0]["competitions"][0]["competitors"][0]["homeAway"] = true;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, chunked, DeserializationOption::Filter(filter), DeserializationOption::NestingLimit(20));
  close(fd);

  if (err) {
    std::cerr << "deserializeJson error: " << err.c_str() << std::endl;
    return 1;
  }

  std::cout << "Successfully streamed & parsed live ESPN data!" << std::endl;
  JsonArray events = doc["events"].as<JsonArray>();
  std::cout << "Event count: " << events.size() << std::endl;
  for (size_t i = 0; i < std::min((size_t)3, events.size()); ++i) {
    JsonObject ev = events[i];
    std::cout << "Event " << i+1 << ": " 
              << ev["competitions"][0]["competitors"][1]["team"]["abbreviation"].as<const char*>() << " "
              << ev["competitions"][0]["competitors"][1]["score"].as<const char*>() << " @ "
              << ev["competitions"][0]["competitors"][0]["team"]["abbreviation"].as<const char*>() << " "
              << ev["competitions"][0]["competitors"][0]["score"].as<const char*>() << " ("
              << ev["status"]["type"]["shortDetail"].as<const char*>() << ")" << std::endl;
  }
  return 0;
}
