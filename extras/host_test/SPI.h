#pragma once
#include <stdint.h>
struct SPISettings{ SPISettings(uint32_t,int,int){} };
struct SPIClass{ SPIClass(int=0){} void begin(){} void begin(int,int,int,int){} void beginTransaction(SPISettings){} void endTransaction(){}
 uint8_t transfer(uint8_t b); void writeBytes(const uint8_t*p,uint32_t n){ for(uint32_t i=0;i<n;i++) transfer(p[i]); } };
extern SPIClass SPI;
