# Changelog

All notable changes. Versions before 1.0.0 were developed without access to the real panel for most of the new
features; see "Verified vs not verified" in the README.

## 1.2.0
- **Icons in text**: 53 vector symbols (wifi, bluetooth, battery levels, charge, arrows, play/pause/stop, home, gear, check, close,
  plus/minus, warning, info, bell, lock, mail, heart, star, sun, moon, trash, edit, refresh, search, menu, power, volume, mute,
  clock, file, folder, download, upload, image, user, pin, signal, cloud, calendar, music, keyboard, light, ban ...). Antialiased
  fonts in three sizes (`CasioFont_SYM_15.h`, `_16`, `_26`) plus UTF-8 macros (`SYM_WIFI` ...) in `CasioSymbols.h`; they work in
  `print()`, in `printWrapped`, and inside WraithUI labels through `addFallbackFont`. `tools/make_symbols.py` renders them at any
  size. `docs/SYMBOLS.md` and `docs/symbols.png` show them all.
- **WraithUI keyboard widget** (`keyboard(scr, x, y, w, h, targetInput)`): letters, one-shot shift, digits and symbols page, space,
  delete, cursor left/right, OK key; moves with the arrow keys, types into any input (`setTarget`), `typeChar/backspace` from a
  physical keypad also go through a focused keyboard.
- **`font2c.py --scan DIR`**: builds a font with only the non-ASCII characters used in your sketch files. Any size, any script (Chinese,
  Thai, Japanese ...) for a few hundred bytes of flash. `tools/font_sizes.py` prints the flash cost of every font header.
- Example `WraithKeyboard` (network list with status icons, password entry). Docs and API reference regenerated.

## 1.1.0
- **WraithUI 2**: the widget toolkit grows from 9 to 23 widgets and gains the structure LVGL users expect. New widgets: panel,
  textarea, input, dropdown, roller, spinbox, tabview, arc, meter, led, spinner, image, table, custom (your own draw
  callback). Containers with column / row / wrapping-row layout, padding, gaps, scrolling with scrollbars that follows
  the focus, and clipping. Themes (light, dark, gray, haunted) and per-widget `Style`. Popups: dropdown list, modal `msgbox`
  with up to 3 buttons, `toast`. Per-widget event callbacks (`on`: click, value, focus, defocus, long press, char, open,
  close, anim done), animations of position / size / value / scroll with easing, 4 timers, screen stack (`push/pop`),
  long press and key repeat (`keyDown/keyUp`), UTF-8 text input (`typeChar/backspace`), label long modes (clip, dots,
  wrap, marquee), vertical bars/sliders, disabled widgets. The WraithUI 1 API is unchanged and its tests still pass.
  Guide: `docs/WRAITHUI.md`. Example: `WraithShowcase`.
- **Fonts**: `CasioFont_VI_24.h` (24 px), `CasioFont_VI_15B.h` (bold 15 px), `CasioFont_NUM_36.h` (36 px digits).
  `font2c.py` now warns about glyphs clipped at the top of the cell.
- **Core**: `printWrapped` / `measureWrapped` / `wrapLine` (word wrap for every font, kinsoku for CJK, never splits a letter
  from its combining marks), `setClipRect` / `clearClipRect`, `dumpFrame` + `iconGet` (screenshot as text).
- **Docs**: `docs/API.md` generated from the headers (`tools/gen_api.py`), `docs/WRAITHUI.md`.
- Known: the 15 px Vietnamese fonts lose the top pixel row of capitals with two stacked marks.

## 1.0.0
- Release candidate-quality pass, no new drawing features. Quality gates added: the whole test suite and all 14 examples
  run under AddressSanitizer + UndefinedBehaviorSanitizer; sources compile with `-std=gnu++11` (older ESP32 Arduino cores);
  every example sketch is compiled and run against host stubs (`extras/host_test/smoke.py`).
- README rewritten as a reference; history moved here. Added `LICENSE`, `FONTS-LICENSE.md` (read it before shipping the
  fonts), `library.json` (PlatformIO), version macros `CASIO_ST75256_VERSION*`.

## 0.9.0
- **Compact antialiased Unicode fonts** (`CasioFont_VI_15.h`, `_TH_15.h`, `_ZH_CN_15.h`, `_ZH_TW_15.h`): 15 px cells drawn with 4 gray levels, so **4 lines** fit on the screen instead of 3. Glyphs are blended over whatever is behind them (white or black text, opaque or transparent background, gray shapes, text size 2+). Vietnamese uses DejaVu Sans, Thai uses Loma, Chinese uses Noto Sans CJK (SC / TC, 14 px). Thai marks are cut out of a shaped base letter, so they sit where the font designer put them, and tone marks that land on a vowel automatically use a *raised* twin. Flash: VI ~8 KB, TH ~6 KB, ZH_CN ~245 KB, ZH_TW ~350 KB. The crisp 16 px Unifont fonts stay available: mix and match, one font chain can hold both kinds.
  - `UFont` has a new `bpp` field (0/1 = 1-bit, 2 = antialiased). Headers made with older versions of `font2c.py` keep working.
  - `tools/font2c.py --aa --size 12 --height 15 --baseline 12 [--index N] [--engine raqm] [--boost 1.1]` makes your own antialiased fonts from any TTF/OTF/TTC.
  - **Note:** I could only judge these from text-art renderings, not on your panel. If 4-gray text looks too soft or too thin, `--boost` (darker strokes) and the crisp 16 px fonts are the knobs.
- **CasioQR**: QR code generator, versions 1-10 (the biggest that fits 63 px), numeric / alphanumeric / byte (UTF-8) modes, all four error-correction levels, automatic mask choice, optional free ECC boost. `qr.encode("https://...")`, `qr.wifi(ssid, password)` (escapes special characters), `qr.drawCentered(lcd)` or `qr.draw(lcd, x, y, scale, quiet)`, `bestScale()`, `capacityBytes()`. About 1.5 KB of RAM. Example: `QrDemo`. Short texts give 2 px modules and scan easily from a phone; version 10 at 1 px per module is possible but hard to scan from a screen.
  - Verified with OpenCV's QR decoder: all 40 full-capacity codes (10 versions x 4 ECC levels) decode, plus numeric/alphanumeric tails, UTF-8 (Vietnamese, Chinese, Thai), URLs, Wi-Fi strings with escapes, and a code with 10 damaged modules still decoding at ECC_H. Not scanned off the real LCD yet.
- Examples: `UnicodeCompact`, `QrDemo`. Tests: `extras/host_test/build.sh` now also covers the 2-bit font blending (exact pixel values), Thai raised tone marks, and the QR codes.

## 0.8.0
- **C2V pre-baked video** (`CasioC2v.h`, `tools/c2vtool.py`): encode a video / animated GIF / folder of images on the PC into a tiny `.c2v` (192x63, 2 bits per pixel, delta + run-length coded), play it with `CasioC2vPlayer`. No JPEG decoding on the ESP, so a frame costs microseconds; mostly static scenes take a few hundred bytes per frame (a test GIF came out at 344 B/frame, raw is 3024). Reads without blocking from any `Stream`, loops with a rewind function, keeps the frame rate (late frames are unpacked but not drawn). `c2vtool.py decode` turns a `.c2v` back into a GIF so you can preview exactly what the LCD shows. Same look options as CasioJpeg (`--fit`, `--dither`, `--contrast`, `--gamma`, `--invert`, `--bg`). Format is documented at the top of the tool. Example: `C2vPlayer`.
- **CasioConsole**: scrolling terminal window, a `Print` object. Word-wraps by pixel width with whatever font is active (5x7, GFX or Unicode: UTF-8 characters are never split), keeps 24 lines, `scrollBack()`, optional cursor, any region of the screen. Example: `ConsoleDemo`.
- **WraithUI slide transitions**: `TR_SLIDE_LEFT / RIGHT / UP / DOWN` (the new screen pushes the old one out, eased).
- **CasioTween.h**: 10 easing functions (`CasioEase::OUT_BOUNCE`, `OUT_BACK`, `OUT_ELASTIC`...) and a tiny time-based `CasioTween` for your own animations.
- `lcd.readFrame2bpp(buf)`: read the screen back into a packed 2bpp buffer (screenshots, transitions).
- All host tests now live in `extras/host_test` (`./build.sh`): core drawing, Unicode fonts, WraithUI, console, easing, slide transitions, JPEG/MJPEG, C2V. The MJPEG decoder was also run on files made by a real ffmpeg (4:2:0, 4:4:4, 4:2:2, odd size): about 1% of the 4-gray pixels differ from a Pillow reference pipeline. Still **not run on hardware**.

## 0.7.0: MJPEG / JPEG -> 192x63 at 2 bits per pixel
- **CasioJpeg**: a from-scratch baseline JPEG decoder that outputs the LCD's native format: 63 rows x 48 bytes, 4 pixels per byte MSB first, 0 = white .. 3 = black (exactly what `drawBitmap2bpp(0, 0, buf, 192, 63)` draws, 3024 bytes per frame).
  ```cpp
  CasioJpeg jpg; uint8_t frame[CasioJpeg::FRAME_BYTES];
  if (jpg.decode(data, len, frame) == CasioJpeg::OK) { lcd.drawBitmap2bpp(0, 0, frame, 192, 63); lcd.display(); }
  ```
  - Luma only: chroma blocks are entropy-decoded and skipped, no colour conversion. Reduced-size IDCT (1/2, 1/4, 1/8) is used automatically when the source is much bigger than 192x63, so big frames cost little more than small ones.
  - `setFit(STRETCH | FIT | FILL)` (letterbox / crop), `setDither(DITHER_BAYER | DITHER_FLOYD | DITHER_NONE)` (Bayer is the default: stable between frames, so video does not shimmer), `setBrightness()`, `setContrast()`, `setGamma()`, `setInvert()`, `setBackground(level)` for the letterbox bars.
  - Supports baseline and extended-sequential Huffman JPEG, grayscale or YCbCr, 4:4:4 / 4:2:2 / 4:2:0 and other subsamplings, restart markers, custom Huffman tables, odd sizes up to 8192 px. Rejects progressive, arithmetic-coded, 12-bit and CMYK files with an error code instead of drawing garbage; corrupt data returns an error and never overwrites the output buffer.
  - Memory: about 30 KB inside the object + a 48 KB accumulator allocated on the first decode.
- **CasioMjpegReader / CasioMjpegPlayer**: split any Arduino `Stream` into JPEG frames (LittleFS/SD `File`, `WiFiClient` on an ESP32-CAM `/stream`, Serial) and play them with frame-rate pacing. Late frames are skipped (not decoded) so playback keeps its speed. Frames are found by walking JPEG markers, so multipart boundaries, junk between frames and EXIF thumbnails are handled. `shown()`, `skipped()`, `errors()`, `lastDecodeMs()` for diagnostics. See `examples/MjpegPlayer`.
- Making a video: `ffmpeg -i input.mp4 -an -vf "scale=384:-2,fps=15" -c:v mjpeg -q:v 6 video.mjpeg`
- Tests (`extras/host_test/build.sh`, needs g++, Python, Pillow, numpy): decoded luma stays within ~1-4 gray levels (out of 255) of Pillow's resize on 12 test images (4:4:4/4:2:2/4:2:0, q30-95, 8x8 to 4000x300, restart markers, optimized Huffman, grayscale), fit/fill geometry, dither tone preservation, progressive/truncated/garbage/bit-flipped input (no crashes, output untouched on error), splitter with odd chunk sizes + EXIF thumbnail + multipart junk + oversized frames, player pacing and catch-up. Decoding takes 0.3-2.4 ms per frame on a PC; I have **not** measured it on the ESP32-S3 yet (expect tens of times slower than a PC, so roughly 5-60 ms per frame depending on size and bitrate). Still **not run on hardware**.

## 0.6.0: Unicode fonts + WraithUI
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

## 0.5.0: HAUNTED FX
Effects that work directly on the framebuffer (call `display()` after them). The diff-based refresh makes them cheap to push.
- `fxDecay(n)` ghost trails: fade all pixels n gray levels toward white each frame, use instead of `clear()`.
- `fxNoise(density, maxLevel)` TV static. `fxScanlines(step, dim)` CRT rows.
- `fxGlitch(intensity)` shifted/inverted bands. `fxMelt(strength)` columns slide down, call repeatedly.
- `printCursed(text, intensity)` jittering letters that sometimes turn into garbage (works with custom fonts).
- `fxFlicker()`, `fxHeartbeat()`: contrast effects through Vop (blocking; need `setVop()` working on your unit).
- `fxCrtOff(ms)` old-TV shutdown to a dot. `fxSeed()` seeds the effect RNG.
- Example `Haunted` plays all of them in a loop. Host tests cover each effect (decay keeps the icon row, glitch conserves pixels, melt moves a pixel down, CRT-off ends blank). Still **not run on hardware**.

## 0.4.0
- **Gauges and charts**: `fillArc(cx, cy, rIn, rOut, a0, a1)` (ring segments, wraps over 0 deg), `drawGraph(x, y, w, h, data, n, vmin, vmax)`, `drawThickLine()`. See the `Dashboard` example.
- `printf("%3d%%", v)` on the lcd object.
- `fadeTo(vop, ms)` ramps the contrast for fade in/out effects (needs a working `setVop()`, which is still unverified on hardware), `getVop()`.
- **tools/img2c.py**: converts PNG/JPG to a `PROGMEM` array for `drawBitmap()` (1-bit) or `drawBitmap2bpp()` (4-gray), with resize and Floyd-Steinberg dithering:
  `python tools/img2c.py logo.png --name LOGO --width 48 --bits 2 --dither > logo.h`
  Checked by converting a test image and reading it back through the library.
- Host tests extended. Still **not run on hardware**.

## 0.3.1
- **Custom fonts**: `setFont(&font)` accepts Adafruit-GFX-format fonts (FreeSans9pt7b, fontconvert output...) via the bundled `gfxfont.h`; Adafruit_GFX does not need to be installed. With a custom font the cursor y is the baseline. `setFont(nullptr)` returns to the 5x7 font. `textWidth()`, `printCentered()`, `printRight()` and wrapping all follow the active font.
- `getTextBounds()` for layout, `drawArc()` (0 deg = 3 o'clock, clockwise), `scroll(dx, dy, fill)` for the dot area.
- Host test extended for all of the above. Still **not run on hardware**.

## 0.3.0
- **Diff-based refresh**: the library keeps a copy of what the LCD holds and `display()` sends only the column runs that really differ. `clear()` + redraw every frame is now cheap (an unchanged frame sends 0 bytes). `display(true)` still forces a full refresh.
- **Speed**: `setSpeed(hz)` (try `begin(4000000)`), `setBlockWrite(true)` now uses `writeBytes()` on ESP32/ESP8266, clipped fast paths for lines/rects, span-based `fillCircle`, scanline `fillTriangle`.
- **Rotation**: `setRotation(0..3)`, `width()`, `height()` (icon row is not rotated).
- **New drawing**: `fillRectDither()` (ordered dither, 17 levels between two colors), `drawBitmap2bpp()` (4-gray images), `drawBitmap(..., bg)`, `drawProgressBar()`, `invertRect()`.
- **Text**: `printRight()`, `setTextWrap()`; wrapping and centering follow the rotated width.
- `setVop()` now clamps to 511. New `Showcase` example.
- Behaviour note: the diff replaces the old per-page dirty bits; `markAllDirty()` now forces one full send on the next `display()`.

## 0.2.0
- **Dirty pages**: `display()` only sends the pages that changed (`display(true)` forces a full refresh). Small animations are much faster.
- **New shapes**: `drawRoundRect`, `fillRoundRect`, `drawTriangle`, `fillTriangle`, `drawFastHLine`, `drawFastVLine`.
- **Text helpers**: `textWidth()`, `printCentered()`.
- **Icon registry**: `defineIcon(id, x0, x1)` then `icon(id, on)`. Fill it in after running the IconSweep example.
- `begin(hz, clearScreen)` can keep the existing RAM content, `frameBuffer()` and `markAllDirty()` for advanced use.
- New `Animation` example (bouncing ball + FPS), `keywords.txt` for syntax highlighting.

## Early test notes (0.2.0)
Compiles against an Arduino stub; a host test with a fake LCD checked: the LCD RAM always equals the framebuffer after `display()`, unchanged frames send 0 bytes, small changes send a few bytes, rotation round trip for all 4 orientations, exact `fillCircle`, `fillTriangle`, dither level 8 = 50%, 2bpp bitmap, progress bar, block-write path. **Not run on hardware yet.**
