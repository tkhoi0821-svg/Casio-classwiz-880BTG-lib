#pragma once
#include <string.h>
#include <stdio.h>
class Print { public: virtual size_t write(uint8_t)=0; size_t write(const uint8_t*b,size_t n){for(size_t i=0;i<n;i++)write(b[i]);return n;}
 size_t print(const char*s){size_t n=0;while(*s){write((uint8_t)*s++);n++;}return n;}
 size_t print(int v){char b[16];snprintf(b,16,"%d",v);return print(b);} virtual ~Print(){} };
