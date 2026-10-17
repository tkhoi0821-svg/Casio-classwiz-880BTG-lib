# CasioST75256 v0.6.0 (HAUNTED + WraithUI)

Author: GrayKazu

Arduino library for the 192x63 LCD (ST75256-compatible) of the fake Casio fx-880BTG, plus its icon row.

## Install
Arduino IDE: Sketch > Include Library > Add .ZIP Library, pick `CasioST75256.zip`.

## Wiring (ESP32-S3 used in the tests)
CS=GPIO14, A0=GPIO10, RST=GPIO11, SCK=GPIO12, SDA=GPIO13. Keep the calculator's own MCU in reset while the ESP drives the LCD.
On non-ESP32 boards the SPI pins are the board's default SPI pins (sck/sda arguments are ignored).

## Quick use
```cpp
SPIClass lcdSpi(FSPI);                       // ESP32-S3; elsewhere pass &SPI
CasioST75256 lcd(14, 10, 11, 12, 13, &lcdSpi);
lcd.begin();
lcd.clear();
lcd.setCursor(4, 4); lcd.print("Hello");
lcd.iconFill();                              // light the icon row
lcd.display();                               // send framebuffer to the LCD
```

## Screen facts
- 192 columns (RAM column offset 0x40), 63 dot rows, plus the icon row (row 64 of the same RAM).
- 4 gray levels: color 0 = white ... 3 = black. Coordinates are (0,0) top-left.
- Default orientation: mirrorX = false, mirrorY = true (matches the unit it was tested on). Change with `setMirror(x, y)` before `begin()`.

## Status
Tested on one unit (ESP32-S3, SPI mode 0, 1 MHz): init, drawing, 4-gray, icon row as a whole.
Not verified: `setVop()` range, `begin(4000000)` stability, `setDisplayInverse()`, `sleep()`, `setBlockWrite(true)`, icon name -> column table (use the IconSweep example to map icons).
The init sequence is copied from sniffed frames; `0x6E` and the delays around the OTP commands (E0/E3/E1) are unverified.

## What's new in 0.6.0: Unicode fonts + WraithUI
### Vietnamese, Thai, Chinese
`print()` now understands UTF-8 once you add a Unicode font:
```cpp
#include <CasioFont_VI.h>      // ASCII + Vietnamese (245 glyphs, ~4 KB)
#include <CasioFont_TH.h>      // ASCII + Thai, vowel/tone marks stack on the previous letter (182 glyphs, ~3 KB)
#include <CasioFont_ZH_CN.h>   // ASCII + Simplified Chinese: GB2312 level 1 + punctuation (4110 glyphs, ~160 KB flash)
#include <CasioFont_ZH_TW.h>   // ASCII + Traditional Chinese: Big5 common set + punctuation (5837 glyphs, ~215 KB flash)
lcd.addUFont(&UFONT_VI); lcd.addUFont(&UFONT_TH); lcd.addUFont(&UFONT_ZH_CN);   // up to 4, searched in order
lcd.print("Tiếng Việt / สวัสดี / 你好");
```
Fonts are 16 px bitmaps, so the screen holds 3 lines (24 half-width or 12 full-width glyphs each). Only the headers you `#include` take flash.
`textWidth()`, `printCentered()`, `printRight()`, `getTextBounds()`, wrapping, text size and `printCursed()` all work with them. Missing glyphs print `?`. Vietnamese must be precomposed (NFC), which is what normal keyboards produce.
- `addUFont()`, `clearUFonts()`, `fontHeight()`. While a Unicode font is active the cursor y is the top of the 16 px cell and the 5x7 font is not used.
- **tools/font2c.py** builds your own font header from any TTF/OTF and character set (`--charset vi,th,gb2312,big5` or `--text "..."`): `python tools/font2c.py --font MyFont.ttf --size 12 --text "ありがとう" --name UFONT_JA > CasioFont_JA.h`. Needs Pillow + fontTools. The shipped fonts were generated from GNU Unifont (GPLv2+ with the font-embedding exception, also available under the SIL OFL 1.1): keep that notice if you redistribute them.

### WraithUI (mini widget toolkit)
`#include <WraithUI.h>`: label, button, bar, slider, toggle, checkbox, list (scrolling), gauge, chart. Several screens, key-driven focus (up/down/left/right/OK/back), `onEvent(id, value)` callback, smooth tweening of bars/gauges, per-widget fonts (`setFont(id, &UFONT_VI)`, `setDefaultFont()`, `addFallbackFont()`), and **screen transitions made from the HAUNTED FX**: `ui.show(screen, WraithUI::TR_GLITCH | TR_STATIC | TR_MELT | TR_CRT)`.
Fixed pool of 24 widgets (`#define WRAITH_MAX_WIDGETS` to change), no malloc. Call `ui.update(millis())` in `loop()`: it redraws and calls `display()` only when something changed. WraithUI owns the lcd's font selection while drawing (it resets to the built-in font afterwards). See `examples/WraithDemo` and `examples/UnicodeDemo`.
Host-tested: UTF-8 decoding, glyph lookup, Thai mark stacking, exact glyph pixels, wrapping/bounds, focus navigation, events, tweening, transitions ending on a clean frame. Still **not run on hardware**.

## What's new in 0.5.0: HAUNTED FX
Effects that work directly on the framebuffer (call `display()` after them). The diff-based refresh makes them cheap to push.
- `fxDecay(n)` ghost trails: fade all pixels n gray levels toward white each frame, use instead of `clear()`.
- `fxNoise(density, maxLevel)` TV static. `fxScanlines(step, dim)` CRT rows.
- `fxGlitch(intensity)` shifted/inverted bands. `fxMelt(strength)` columns slide down, call repeatedly.
- `printCursed(text, intensity)` jittering letters that sometimes turn into garbage (works with custom fonts).
- `fxFlicker()`, `fxHeartbeat()`: contrast effects through Vop (blocking; need `setVop()` working on your unit).
- `fxCrtOff(ms)` old-TV shutdown to a dot. `fxSeed()` seeds the effect RNG.
- Example `Haunted` plays all of them in a loop. Host tests cover each effect (decay keeps the icon row, glitch conserves pixels, melt moves a pixel down, CRT-off ends blank). Still **not run on hardware**.

## What's new in 0.4.0
- **Gauges and charts**: `fillArc(cx, cy, rIn, rOut, a0, a1)` (ring segments, wraps over 0 deg), `drawGraph(x, y, w, h, data, n, vmin, vmax)`, `drawThickLine()`. See the `Dashboard` example.
- `printf("%3d%%", v)` on the lcd object.
- `fadeTo(vop, ms)` ramps the contrast for fade in/out effects (needs a working `setVop()`, which is still unverified on hardware), `getVop()`.
- **tools/img2c.py**: converts PNG/JPG to a `PROGMEM` array for `drawBitmap()` (1-bit) or `drawBitmap2bpp()` (4-gray), with resize and Floyd-Steinberg dithering:
  `python tools/img2c.py logo.png --name LOGO --width 48 --bits 2 --dither > logo.h`
  Checked by converting a test image and reading it back through the library.
- Host tests extended. Still **not run on hardware**.

## What's new in 0.3.1
- **Custom fonts**: `setFont(&font)` accepts Adafruit-GFX-format fonts (FreeSans9pt7b, fontconvert output...) via the bundled `gfxfont.h`; Adafruit_GFX does not need to be installed. With a custom font the cursor y is the baseline. `setFont(nullptr)` returns to the 5x7 font. `textWidth()`, `printCentered()`, `printRight()` and wrapping all follow the active font.
- `getTextBounds()` for layout, `drawArc()` (0 deg = 3 o'clock, clockwise), `scroll(dx, dy, fill)` for the dot area.
- Host test extended for all of the above. Still **not run on hardware**.

## What's new in 0.3.0
- **Diff-based refresh**: the library keeps a copy of what the LCD holds and `display()` sends only the column runs that really differ. `clear()` + redraw every frame is now cheap (an unchanged frame sends 0 bytes). `display(true)` still forces a full refresh.
- **Speed**: `setSpeed(hz)` (try `begin(4000000)`), `setBlockWrite(true)` now uses `writeBytes()` on ESP32/ESP8266, clipped fast paths for lines/rects, span-based `fillCircle`, scanline `fillTriangle`.
- **Rotation**: `setRotation(0..3)`, `width()`, `height()` (icon row is not rotated).
- **New drawing**: `fillRectDither()` (ordered dither, 17 levels between two colors), `drawBitmap2bpp()` (4-gray images), `drawBitmap(..., bg)`, `drawProgressBar()`, `invertRect()`.
- **Text**: `printRight()`, `setTextWrap()`; wrapping and centering follow the rotated width.
- `setVop()` now clamps to 511. New `Showcase` example.
- Behaviour note: the diff replaces the old per-page dirty bits; `markAllDirty()` now forces one full send on the next `display()`.

## What's new in 0.2.0
- **Dirty pages**: `display()` only sends the pages that changed (`display(true)` forces a full refresh). Small animations are much faster.
- **New shapes**: `drawRoundRect`, `fillRoundRect`, `drawTriangle`, `fillTriangle`, `drawFastHLine`, `drawFastVLine`.
- **Text helpers**: `textWidth()`, `printCentered()`.
- **Icon registry**: `defineIcon(id, x0, x1)` then `icon(id, on)`. Fill it in after running the IconSweep example.
- `begin(hz, clearScreen)` can keep the existing RAM content, `frameBuffer()` and `markAllDirty()` for advanced use.
- New `Animation` example (bouncing ball + FPS), `keywords.txt` for syntax highlighting.

## Tests done
Compiles against an Arduino stub; a host test with a fake LCD checked: the LCD RAM always equals the framebuffer after `display()`, unchanged frames send 0 bytes, small changes send a few bytes, rotation round trip for all 4 orientations, exact `fillCircle`, `fillTriangle`, dither level 8 = 50%, 2bpp bitmap, progress bar, block-write path. **Not run on hardware yet.**
