// Host stub of the Arduino/ESP32 API used by the examples (just enough to compile and run them).
#pragma once
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <algorithm>
#include <string>
#include <vector>
#include "Print.h"
using std::min; using std::max;
#define PROGMEM
#define pgm_read_byte(a) (*(const uint8_t*)(a))
#define HIGH 1
#define LOW 0
#define OUTPUT 1
#define INPUT 0
#define MSBFIRST 1
#define SPI_MODE0 0
#define FSPI 0
#define A0 36
#define F(x) x
extern unsigned long g_ms;
inline unsigned long millis() { return g_ms += 1; }          // every call advances time, so busy-waits terminate
inline unsigned long micros() { return (g_ms += 1) * 1000; }
inline void delay(unsigned long ms) { g_ms += ms; }
inline void pinMode(int, int) {}
inline void digitalWrite(int, int) {}
inline int analogRead(int) { return rand() & 1023; }
inline uint32_t esp_random() { return (uint32_t)rand(); }

class String {
 public:
  std::string s;
  String() {}
  String(const char* c) : s(c) {}
  String(const std::string& c) : s(c) {}
  void trim() { size_t a = s.find_first_not_of(" \t\r\n"); size_t b = s.find_last_not_of(" \t\r\n"); s = a == std::string::npos ? "" : s.substr(a, b - a + 1); }
  bool operator==(const char* o) const { return s == o; }
  const char* c_str() const { return s.c_str(); }
  size_t length() const { return s.size(); }
};

inline size_t Print::print(const String& s) { return print(s.c_str()); }
inline size_t Print::println(const String& s) { return println(s.c_str()); }

class Stream : public Print {
 public:
  const uint8_t* data = nullptr; size_t size = 0, pos = 0, trickle = 1 << 30;
  int available() { size_t r = size - pos; return (int)(r < trickle ? r : trickle); }
  int readBytes(char* b, size_t n) { size_t r = size - pos; if (n > r) n = r; if (n > trickle) n = trickle; memcpy(b, data + pos, n); pos += n; return (int)n; }
  int read() { return pos < size ? data[pos++] : -1; }
  size_t write(uint8_t) override { return 1; }
};

class SerialClass : public Stream {
 public:
  std::vector<uint8_t> in;
  void begin(unsigned long) {}
  void feed(const char* s) { in.insert(in.end(), s, s + strlen(s)); data = in.data(); size = in.size(); }
  size_t write(uint8_t c) override { putchar(c); return 1; }
  using Print::write;
  using Print::print; using Print::println;
  size_t println(const String& v) { return println(v.c_str()); }
  size_t printf(const char* f, ...) { va_list ap; va_start(ap, f); int n = vprintf(f, ap); va_end(ap); return n; }
  String readStringUntil(char t) { std::string o; int c; while ((c = read()) >= 0 && c != t) o += (char)c; return String(o); }
};
extern SerialClass Serial;

class File : public Stream {
 public:
  std::vector<uint8_t> store; bool open_ = false;
  File() {}
  File(const File& o) { *this = o; }
  File& operator=(const File& o) { store = o.store; open_ = o.open_; data = store.data(); size = store.size(); pos = o.pos; return *this; }
  explicit operator bool() const { return open_; }
  bool seek(uint32_t p) { pos = p; return true; }
  void close() { open_ = false; store.clear(); data = nullptr; size = pos = 0; }
};
class FSClass {
 public:
  bool begin(bool = false) { return true; }
  File open(const char* path, const char* = "r") {
    File f; std::string p = std::string("smoke_data") + path; FILE* fp = fopen(p.c_str(), "rb");
    if (!fp) return f;
    int c; while ((c = fgetc(fp)) != EOF) f.store.push_back((uint8_t)c);
    fclose(fp); f.open_ = true; f.data = f.store.data(); f.size = f.store.size(); return f;
  }
};
