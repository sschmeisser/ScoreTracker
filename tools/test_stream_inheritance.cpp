#include <iostream>
#include <ArduinoJson.h>

class CustomStream : public Stream {
 public:
  int available() override { return 0; }
  int read() override { return -1; }
  int peek() override { return -1; }
  void flush() override {}
  size_t write(uint8_t) override { return 0; }
};

int main() {
  CustomStream cs;
  JsonDocument doc;
  deserializeJson(doc, cs);
  std::cout << "Compiled successfully!" << std::endl;
  return 0;
}
