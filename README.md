# CasioST75256 v0.3.0

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
