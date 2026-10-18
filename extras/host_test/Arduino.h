#pragma once
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <algorithm>
#include <vector>
using std::min; using std::max;
#define PROGMEM
#define pgm_read_byte(a) (*(const uint8_t*)(a))
#define HIGH 1
#define LOW 0
#define OUTPUT 1
#define MSBFIRST 1
#define SPI_MODE0 0
#define FSPI 0
inline void pinMode(int,int){}
extern int g_a0;
inline void digitalWrite(int pin,int v){ extern int PIN_A0; if(pin==PIN_A0) g_a0=v; extern int PIN_CS; extern void csEdge(int); if(pin==PIN_CS) csEdge(v);}
inline void delay(int){}
#include "Print.h"
#include <stdint.h>
inline unsigned long millis() { return 0; }
class Stream {
 public:
  const uint8_t* data = nullptr; size_t size = 0, pos = 0, trickle = 1 << 30;
  int available() { size_t r = size - pos; return (int)(r < trickle ? r : trickle); }
  int readBytes(char* b, size_t n) { size_t r = size - pos; if (n > r) n = r; if (n > trickle) n = trickle; memcpy(b, data + pos, n); pos += n; return (int)n; }
};
