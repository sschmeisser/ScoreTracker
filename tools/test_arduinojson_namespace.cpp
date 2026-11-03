#include <iostream>
#include <ArduinoJson.h>

class DummyChunked {
 public:
  int read() { return -1; }
};

namespace ArduinoJson {
inline namespace ARDUINOJSON_NAMESPACE {
namespace detail {
template <>
struct Reader<DummyChunked, void> {
  DummyChunked& _s;
  Reader(DummyChunked& s) : _s(s) {}
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
}

int main() {
  DummyChunked dc;
  JsonDocument doc;
  deserializeJson(doc, dc);
  std::cout << "Success!" << std::endl;
  return 0;
}
