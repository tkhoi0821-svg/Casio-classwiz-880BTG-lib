// tiny stand-in for Adafruit's FreeSans9pt7b so examples/Fonts compiles on the host
#pragma once
#include "gfxfont.h"
static const uint8_t FreeSans9pt7bBitmaps[] PROGMEM = {0xF0};
static const GFXglyph FreeSans9pt7bGlyphs[95] PROGMEM = {};
static const GFXfont FreeSans9pt7b = {(uint8_t*)FreeSans9pt7bBitmaps, (GFXglyph*)FreeSans9pt7bGlyphs, 0x20, 0x7E, 12};
