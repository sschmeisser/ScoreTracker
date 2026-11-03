#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <ArduinoJson.h>

class SimulatedStream {
 private:
  std::string _data;
  size_t _pos = 0;
 public:
  SimulatedStream(const std::string& data) : _data(data), _pos(0) {}
  int available() { return _pos < _data.size() ? (_data.size() - _pos) : 0; }
  int read() { return _pos < _data.size() ? (unsigned char)_data[_pos++] : -1; }
  int peek() { return _pos < _data.size() ? (unsigned char)_data[_pos] : -1; }
  std::string readStringUntil(char terminator) {
    std::string s;
    while (_pos < _data.size()) {
      char c = _data[_pos++];
      if (c == terminator) break;
      s += c;
    }
    return s;
  }
};

class ChunkedStreamWrapper {
 private:
  SimulatedStream* _stream;
  size_t _chunkRemaining = 0;
  bool _ended = false;

  void readChunkHeader() {
    if (_ended) return;
    std::string line = _stream->readStringUntil('\n');
    while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
    while (!line.empty() && line.front() == ' ') line.erase(line.begin());
    if (line.empty()) {
      line = _stream->readStringUntil('\n');
      while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
      while (!line.empty() && line.front() == ' ') line.erase(line.begin());
    }
    _chunkRemaining = strtol(line.c_str(), NULL, 16);
    if (_chunkRemaining == 0) {
      _ended = true;
    }
  }

 public:
  ChunkedStreamWrapper(SimulatedStream* s) : _stream(s) {
    readChunkHeader();
  }

  int available() {
    if (_ended) return 0;
    return _stream->available();
  }

  int read() {
    if (_ended) return -1;
    if (_chunkRemaining == 0) {
      readChunkHeader();
      if (_ended) return -1;
    }
    int c = _stream->read();
    if (c >= 0) {
      _chunkRemaining--;
      if (_chunkRemaining == 0) {
        while (_stream->available() && (_stream->peek() == '\r' || _stream->peek() == '\n')) {
          _stream->read();
        }
      }
    }
    return c;
  }

  int peek() {
    if (_ended) return -1;
    if (_chunkRemaining == 0) {
      readChunkHeader();
      if (_ended) return -1;
    }
    return _stream->peek();
  }
};

// Custom ArduinoJson Reader for ChunkedStreamWrapper
namespace ArduinoJson {
namespace detail {
template <>
struct Reader<ChunkedStreamWrapper> {
  ChunkedStreamWrapper& _stream;
  Reader(ChunkedStreamWrapper& s) : _stream(s) {}
  int read() { return _stream.read(); }
  size_t read(char* buffer, size_t length) {
    size_t i = 0;
    while (i < length) {
      int c = _stream.read();
      if (c < 0) break;
      buffer[i++] = (char)c;
    }
    return i;
  }
};
}
}

int main() {
  // Test with a chunked JSON string
  std::string chunked_data = 
    "10\r\n{\"message\":\"hello \r\n"
    "0c\r\nworld!\",\"id\":\r\n"
    "06\r\n42}\r\n\r\n"
    "0\r\n\r\n";

  SimulatedStream raw(chunked_data);
  ChunkedStreamWrapper chunked(&raw);

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, chunked);
  if (err) {
    std::cout << "Error: " << err.c_str() << std::endl;
  } else {
    std::cout << "Success! message=" << doc["message"].as<std::string>() 
              << ", id=" << doc["id"].as<int>() << std::endl;
  }
  return 0;
}
