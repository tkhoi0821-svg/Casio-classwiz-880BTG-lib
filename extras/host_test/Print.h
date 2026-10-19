#pragma once
#include <string.h>
#include <stdio.h>
#include <stdint.h>
class String;
class Print {
 public:
  virtual size_t write(uint8_t) = 0;
  size_t write(const uint8_t* b, size_t n) { for (size_t i = 0; i < n; i++) write(b[i]); return n; }
  size_t print(const char* s) { size_t n = 0; while (*s) { write((uint8_t)*s++); n++; } return n; }
  size_t print(int v) { char b[16]; snprintf(b, 16, "%d", v); return print(b); }
  size_t print(unsigned v) { char b[16]; snprintf(b, 16, "%u", v); return print(b); }
  size_t print(unsigned char v) { return print((unsigned)v); }
  size_t print(unsigned long v) { char b[24]; snprintf(b, 24, "%lu", v); return print(b); }
  size_t print(float v) { char b[24]; snprintf(b, 24, "%.2f", v); return print(b); }
  size_t print(const String& s);
  size_t println(const String& s);
  size_t println() { return write('\n'); }
  size_t println(const char* s) { size_t n = print(s); return n + write('\n'); }
  size_t println(unsigned v) { size_t n = print(v); return n + write('\n'); }
  size_t println(int v) { size_t n = print(v); return n + write('\n'); }
  virtual ~Print() {}
};
