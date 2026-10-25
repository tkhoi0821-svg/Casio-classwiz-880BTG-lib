# API reference

Generated from the headers in `src/` by `tools/gen_api.py` (do not edit by hand). Short guides: `README.md`, `docs/WRAITHUI.md`.

## CasioST75256 (core driver)
`#include <CasioST75256.h>`

> CasioST75256 - driver for the 192x63 LCD of the fake Casio fx-880BTG (ST75256-compatible) Author: GrayKazu Screen facts (found by sniffing + replaying on a real unit): * 192 visible columns, RAM columns start at offset 0x40 * 63 dot rows + 1 icon row (row 64 of the same RAM, page 15) * 4-gray mode, 2 bits per pixel, 4 vertical pixels per byte * SPI mode 0, A0 low = command, A0 high = data

### `class CasioST75256 : public Print `

- `static const int16_t WIDTH = 192` - physical size (rotation 0)
- `static const int16_t HEIGHT = 63` - dot rows; the 64th row is the icon row
- `CasioST75256(int8_t cs, int8_t a0, int8_t rst, int8_t sck = -1, int8_t sda = -1, SPIClass* spi = &SPI)` - cs, a0, rst = control pins. sck/sda are only used on ESP32 (custom SPI pins). On other boards use the default SPI pins.
- `bool begin(uint32_t spiHz = 1000000, bool clearScreen = true)` - Reset, init, clear, display ON. Call after setMirror()/setColumnOffset() if you change them.
- `void display(bool all = false)` - Send the framebuffer to the LCD. v0.3.0+: the buffer is diffed against what was last sent, and only the changed column runs of each page are transmitted (pass true to force a full refresh).
- `void markAllDirty()`
- `const uint8_t* frameBuffer() const` - 16 pages x 192 columns
#### panel settings

- `void setSpeed(uint32_t hz)` - change SPI clock after begin()
- `void setColumnOffset(uint8_t off)` - default 0x40
- `void setMirror(bool x, bool y)` - default x=false, y=true
- `void setA0Inverted(bool inv)` - debug only
- `void setBlockWrite(bool on)` - one SPI transaction per column run (faster, less tested)
- `void setInvertColors(bool inv)` - software: swap black/white when drawing
- `void setDisplayInverse(bool inv)` - hardware inverse (A6/A7)
- `void displayOn(bool on)`
- `void sleep(bool on)`
- `void setVop(uint16_t vop)` - contrast, 0..511, default 123 (0x7B)
#### clipping (v1.1.0): every drawing call is limited to this rectangle (logical coordinates)

- `void setClipRect(int16_t x, int16_t y, int16_t w, int16_t h)`
- `void clearClipRect()`
#### rotation (software, drawing only; the icon row is not rotated)

- `void setRotation(uint8_t r)` - 0 = normal 192x63, 1 = 90 deg clockwise (63x192), 2 = 180, 3 = 270.
- `uint8_t getRotation() const`
- `int16_t width() const`
- `int16_t height() const`
#### drawing (color 0 = white .. 3 = black)

- `void clear(uint8_t color = 0)` - clears the dot area, keeps the icon row
- `void drawPixel(int16_t x, int16_t y, uint8_t color = 3)`
- `uint8_t getPixel(int16_t x, int16_t y)`
- `void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color = 3)`
- `void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color = 3)`
- `void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color = 3)`
- `void drawCircle(int16_t cx, int16_t cy, int16_t r, uint8_t color = 3)`
- `void fillCircle(int16_t cx, int16_t cy, int16_t r, uint8_t color = 3)`
- `void drawFastHLine(int16_t x, int16_t y, int16_t w, uint8_t color = 3)`
- `void drawFastVLine(int16_t x, int16_t y, int16_t h, uint8_t color = 3)`
- `void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint8_t color = 3)`
- `void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint8_t color = 3)`
- `void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t color = 3)`
- `void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t color = 3)`
- `void drawBitmap(int16_t x, int16_t y, const uint8_t* bmp, int16_t w, int16_t h, uint8_t color = 3, int16_t bg = -1)` - 1-bit bitmap (PROGMEM ok), rows padded to whole bytes, MSB first. bg -1 = transparent.
- `size_t readFrame2bpp(uint8_t* out)` - 2-bit (4-gray) bitmap (PROGMEM ok), 4 pixels per byte, MSB first, rows padded to whole bytes. transparent = gray value to skip (-1 = draw everything). Read the dot area back (current rotation) into a packed 2bpp buffer: ((width()+3)/4) bytes per row, height() rows. Returns the byte count (3024 for rotation 0, 3072 for 90/270). Same layout drawBitmap2bpp() reads.
- `void drawBitmap2bpp(int16_t x, int16_t y, const uint8_t* bmp, int16_t w, int16_t h, int16_t transparent = -1)`
#### v0.3.0 extras

- `void fillRectDither(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t level, uint8_t c0 = 0, uint8_t c1 = 3)` - Ordered-dither fill: level 0..16 = how many of the 16 cells in a 4x4 block use c1 (rest use c0). With real grays this gives smooth gradients: e.g. fillRectDither(x,y,w,h,8,0,3).
- `void drawProgressBar(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t percent, uint8_t color = 3)`
- `void invertRect(int16_t x, int16_t y, int16_t w, int16_t h)` - c -> 3 - c inside the rectangle
- `void drawArc(int16_t cx, int16_t cy, int16_t r, int16_t startDeg, int16_t endDeg, uint8_t color = 3)` - Arc of a circle, angles in degrees: 0 = 3 o'clock, increasing clockwise (y points down).
- `void scroll(int16_t dx, int16_t dy, uint8_t fill = 0)` - Shift the dot area by (dx,dy) pixels in current-rotation coordinates; uncovered area gets `fill`.
#### v0.4.0: UI building blocks

- `void drawThickLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t thickness, uint8_t color = 3)` - Line with a square brush of `thickness` pixels.
- `void fillArc(int16_t cx, int16_t cy, int16_t rIn, int16_t rOut, int16_t a0, int16_t a1, uint8_t color = 3)` - Filled ring segment between radii rIn..rOut and angles a0..a1 (same angle convention as drawArc). Gauges: fillArc(cx, cy, 20, 26, 180, 360) draws a half ring; draw a lighter one underneath for the track.
- `void drawGraph(int16_t x, int16_t y, int16_t w, int16_t h, const int16_t* data, uint16_t n, int16_t vmin, int16_t vmax, uint8_t color = 3)` - Line graph of n samples scaled from vmin..vmax into the box (with a frame).
- `size_t printf(const char* fmt, ...) __attribute__((format(printf, 2, 3)))` - print() with a format string
- `void fadeTo(uint16_t target, uint16_t ms)` - Smoothly ramp the contrast (Vop) to `target` over about `ms` milliseconds (blocking).
- `uint16_t getVop() const`
#### text (5x7 font in a 6x8 cell, works with print()/println())

- `void setCursor(int16_t x, int16_t y)`
- `void setTextSize(uint8_t s)`
- `void setTextColor(uint8_t fg, int16_t bg = -1)` - bg -1 = transparent
- `void setTextWrap(bool w)` - default true
- `void setFont(const GFXfont* f)` - Custom font (GFXfont, see gfxfont.h). With a custom font the cursor y is the text BASELINE (like Adafruit GFX); setFont(nullptr) goes back to the built-in 5x7 font (cursor y = top).
- `bool addUFont(const UFont* f)` - Unicode fonts (v0.6.0): UTF-8 text with Vietnamese, Thai, Chinese... Include a font header and add it: #include <CasioFont_VI.h>   ->  lcd.addUFont(&UFONT_VI);   lcd.print("Tiếng Việt"); Up to 4 fonts are searched in the order they were added (so you can mix e.g. VI + TH + ZH_CN). Cursor y is the top of the cell. While a font is added it replaces the built-in 5x7 font.
- `void clearUFonts()`
- `int16_t fontHeight() const` - Line height in pixels at size 1 for the active font (8 for the built-in font).
- `void drawChar(int16_t x, int16_t y, char c, uint8_t fg, int16_t bg, uint8_t size)`
- `void getTextBounds(const char* s, int16_t x, int16_t y, int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h)` - Bounding box of `s` if printed with the cursor at (x,y) (handles '\n', current font and size).
- `int16_t textWidth(const char* s) const` - width in pixels at the current size
- `void printCentered(const char* s, int16_t y)` - horizontally centered at row y
- `void printRight(const char* s, int16_t xRight, int16_t y)` - right edge of the text at xRight
#### word wrap (v1.1.0): works with the built-in, GFX and Unicode fonts (UTF-8 aware)

- `enum TextAlign : uint8_t { ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT }`
- `uint16_t printWrapped(const char* s, int16_t x, int16_t y, int16_t w, uint16_t maxLines = 0, TextAlign align = ALIGN_LEFT, bool ellipsis = false)` - Print s inside a box w pixels wide, first line at (x, y) (y = top of the line, or the baseline with a GFX font). Breaks at spaces and after '-', between any two CJK characters (never before closing punctuation such as ，。！？), never between a letter and its combining marks, and inside a word that is wider than the box. '\n' forces a break. maxLines 0 = no limit; with ellipsis the last allowed line ends in "..." when text is left over. Returns the number of lines drawn; the cursor ends up below the text.
- `uint16_t measureWrapped(const char* s, int16_t w) const` - number of lines printWrapped would use
- `const char* wrapLine(const char* p, int16_t w, const char** start, size_t* len) const` - Low level: find the next wrapped line starting at p. *start/*len = the line (trailing spaces removed). Returns where the next line begins, or nullptr when the text is exhausted.
- `size_t write(uint8_t c) override`
#### icon row (FIX, SCI, arrows ...): x is the same 0..191 column space as drawing

- `void iconClear()`
- `void iconFill()`
- `void iconSet(int16_t x0, int16_t x1, bool on = true)`
- `static const uint8_t MAX_ICONS = 32` - Name your own icons once you know their columns (see the IconSweep example): lcd.defineIcon(0, 8, 15);   // id 0 = columns 8..15 lcd.icon(0, true);
- `void defineIcon(uint8_t id, int16_t x0, int16_t x1)`
- `void icon(uint8_t id, bool on = true)`
#### HAUNTED FX (v0.5.0): effects that work on the framebuffer, call display() afterwards

- `void fxSeed(uint32_t s)` - seed the effect RNG (esp_random() etc.)
- `void fxDecay(uint8_t amount = 1)` - Fade every pixel `amount` gray levels toward white: call each frame instead of clear() for ghost trails.
- `void fxNoise(uint8_t density, uint8_t maxLevel = 3)` - TV static: density 0..255 = chance a pixel is replaced by a random gray (<= maxLevel).
- `void fxScanlines(uint8_t step = 2, uint8_t dim = 1)` - CRT look: every `step`-th row is dimmed by `dim` gray levels.
- `void fxGlitch(uint8_t intensity)` - Corrupt random horizontal bands (shifted with wrap-around, above 128 some are inverted). 0..255.
- `void fxMelt(uint8_t strength = 2)` - Every column slides down by a random 0..strength pixels. Call repeatedly: the picture melts away.
- `void fxFlicker(uint8_t times = 5, uint16_t dim = 40)` - Contrast flicker like a dying lamp (blocking; needs a working setVop).
- `void fxHeartbeat(uint8_t beats = 3, uint16_t amp = 60)` - Contrast pulse: lub-dub, lub-dub (blocking; needs a working setVop).
- `void fxCrtOff(uint16_t ms = 400)` - Old TV switching off: the picture collapses to a line, then to a dot, then nothing (blocking, clears the screen).
- `void printCursed(const char* s, uint8_t intensity = 40)` - Like print(), but letters jitter and sometimes turn into garbage. intensity 0..255.
#### screenshot over a stream (v1.1.0)

- `void dumpFrame(Print& out, bool icons = true)` - Prints the dot area (current rotation) and the icon row as text: a header line, one hex line per row (2 bits per pixel, 4 pixels per byte), an ICON line and a CRC32 trailer. Capture it from the Serial Monitor and convert it to a PNG with tools/lcdshot.py. Typical use: lcd.dumpFrame(Serial);
- `bool iconGet(int16_t x)` - is the icon-row pixel at column x lit?
#### low level

- `void writeCommand(uint8_t c)`
- `void writeData(uint8_t d)`

## WraithUI (widget toolkit)
`#include <WraithUI.h>`

> WraithUI 2 - retained-mode UI toolkit for CasioST75256 (key-driven, no touch, no malloc).   Author: GrayKazu Widgets : label, button, bar, slider, toggle, checkbox, list, gauge, chart (v1) and panel, textarea, input, dropdown, roller, spinbox, tabview, arc, meter, led, spinner, image, table, custom (v2) Popups  : dropdown list, msgbox (modal, up to 3 buttons), toast Layout  : containers (panel / tabview) with column / row / wrapping-row layout, padding, gaps, scrolling that follows focus Style   : 4 themes (light, dark, gray, haunted) + per-widget Style overrides + per-widget fonts (UFont, GFX) Input   : keyDown/keyUp/key (up, down, left, right, ok, back), long press, key repeat, typeChar/backspace for text input Logic   : per-widget event callbacks, animations (position, size, value, scroll), timers, screen stack (push/pop), screen transitions built from the HAUNTED FX (glitch, static, 

### `class WraithUI`

- `enum Key { KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_OK, KEY_BACK }`
- `enum Trans { TR_NONE, TR_GLITCH, TR_STATIC, TR_MELT, TR_CRT, TR_SLIDE_LEFT, TR_SLIDE_RIGHT, TR_SLIDE_UP, TR_SLIDE_DOWN }` - slide: the new screen pushes the old one out
- `enum Align { LEFT, CENTER, RIGHT }`
- `enum Ev : uint8_t { EV_CLICK, EV_VALUE, EV_FOCUS, EV_DEFOCUS, EV_LONG, EV_CHAR, EV_OPEN, EV_CLOSE, EV_ANIM_DONE }` - events delivered to a widget's callback (see on()); the global onEvent() handler only sees EV_CLICK and EV_VALUE
- `enum Layout : uint8_t { LAYOUT_NONE, LAYOUT_COLUMN, LAYOUT_ROW, LAYOUT_ROW_WRAP }`
- `enum LongMode : uint8_t { LONG_CLIP, LONG_DOTS, LONG_WRAP, LONG_SCROLL }` - label text wider than the label
- `enum Prop : uint8_t { P_X, P_Y, P_W, P_H, P_VALUE, P_SCROLL_X, P_SCROLL_Y }` - animatable properties
- `enum Theme : uint8_t { THEME_LIGHT, THEME_DARK, THEME_GRAY, THEME_HAUNTED }`
- `static const uint8_t NONE = 0xFF` - "no widget" / "no parent" / "no tab" / pool full
- `typedef void (*Handler)(uint8_t id, int16_t value)` - global: click (button 1, list index) / value
- `typedef void (*EventCb)(uint8_t id, uint8_t ev, int16_t value, void* user)` - per widget, see on()
- `typedef void (*DrawCb)(WraithUI& ui, CasioST75256& lcd, uint8_t id, int16_t x, int16_t y, int16_t w, int16_t h, bool focused, void* user)` - custom widget; drawing is clipped to its box
- `typedef void (*TimerCb)(void* user)`
- `typedef void (*MsgCb)(int8_t button, void* user)` - msgbox result: button index, -1 = cancelled
- `struct Style { int8_t fg = -1, bg = -1, border = -1, borderColor = -1, radius = -1, pad = -1; }` - Per-widget look. -1 = inherit from the theme. fg/bg/borderColor are gray levels 0 (white) .. 3 (black), bg 4 = transparent.
- `explicit WraithUI(CasioST75256& lcd)`
#### creating widgets (all return the widget id, or NONE when the pool is full)

- `uint8_t panel (uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t h)` - scr = screen number the widget belongs to. x/y = top-left in pixels, relative to the parent's content area when the widget has a parent (see setParent). w/h = 0 means "automatic" where that makes sense (labels, buttons). container, optional border/fill
- `uint8_t label (uint8_t scr, int16_t x, int16_t y, int16_t w, const char* text, Align a = LEFT, uint16_t cap = 0)`
- `uint8_t button (uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t h, const char* text, uint16_t cap = 0)`
- `uint8_t bar (uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t h, int16_t value = 0, int16_t mn = 0, int16_t mx = 100)`
- `uint8_t slider (uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t mn, int16_t mx, int16_t value, int16_t step = 1)`
- `uint8_t toggle (uint8_t scr, int16_t x, int16_t y, bool on = false)`
- `uint8_t checkbox(uint8_t scr, int16_t x, int16_t y, int16_t w, const char* text, bool checked = false, uint16_t cap = 0)`
- `uint8_t list (uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t h, const char* const* items, uint8_t n)`
- `uint8_t gauge (uint8_t scr, int16_t cx, int16_t cy, int16_t r, int16_t mn, int16_t mx, int16_t value = 0)` - x,y = CENTER of the half ring
- `uint8_t chart (uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t h, int16_t* buf, uint16_t n, int16_t vmin, int16_t vmax)`
#### new in WraithUI 2

- `uint8_t textarea(uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t cap = 192)` - wrapped, scrollable text / log
- `uint8_t input (uint8_t scr, int16_t x, int16_t y, int16_t w, uint16_t cap = 24)` - one-line editor (typeChar/backspace)
- `uint8_t dropdown(uint8_t scr, int16_t x, int16_t y, int16_t w, const char* const* items, uint8_t n)`
- `uint8_t roller (uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t h, const char* const* items, uint8_t n)` - picker, selected row centered
- `uint8_t spinbox (uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t mn, int16_t mx, int16_t value, int16_t step = 1)`
- `uint8_t tabview (uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t h, const char* const* tabs, uint8_t n)` - children: setParent(child, tabview, tabIndex)
- `uint8_t arc (uint8_t scr, int16_t x, int16_t y, int16_t size, int16_t mn, int16_t mx, int16_t value = 0, bool interactive = true)`
- `uint8_t meter (uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t mn, int16_t mx, int16_t value = 0, uint8_t ticks = 5)` - dial with ticks + needle
- `uint8_t led (uint8_t scr, int16_t x, int16_t y, int16_t size, uint8_t level = 3)` - level 0 (off) .. 3
- `uint8_t spinner (uint8_t scr, int16_t x, int16_t y, int16_t size)` - loading indicator
- `uint8_t image (uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t* bmp2bpp, int16_t transparent = -1)`
- `uint8_t table (uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t h, const char* const* cells, uint8_t cols, uint8_t rows, bool header = true)`
- `uint8_t keyboard(uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t h, uint8_t targetInput = NONE)` - on-screen keyboard: 5 rows, shift / symbols / space / delete / cursor / OK
- `uint8_t custom (uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t h, DrawCb draw, void* user = nullptr, bool focusable = false)`
#### widget properties

- `void setParent(uint8_t child, uint8_t parent, uint8_t tab = NONE)` - parent = panel or tabview; tab = tab index (tabview only)
- `void setLayout(uint8_t container, Layout l, uint8_t gap = 2, uint8_t pad = 2)`
- `void setScrollable(uint8_t container, bool on)` - vertical + horizontal; follows focus; scrollbars
- `void scrollTo(uint8_t container, int16_t x, int16_t y, uint16_t animMs = 0)`
- `void setTarget(uint8_t keyboard, uint8_t input)` - which input a keyboard types into
- `void setText(uint8_t id, const char* text)` - copied; truncated to the widget's capacity
- `void setTextRef(uint8_t id, const char* text)` - NOT copied: must stay valid (flash strings, long texts)
- `const char* text(uint8_t id) const`
- `void appendText(uint8_t id, const char* text)` - textarea: add text, follow the end if the view was at the end
- `void clearText(uint8_t id)`
- `void setLongMode(uint8_t label, LongMode m)`
- `void setAlign(uint8_t id, Align a)` - text alignment (label, button, input, dropdown...) / cross-axis of layouts
- `void setValue(uint8_t id, int16_t v)` - clamped; bar/gauge/slider animate toward it
- `int16_t value(uint8_t id) const`
- `void setRange(uint8_t id, int16_t mn, int16_t mx, int16_t step = 0)`
- `void setItems(uint8_t id, const char* const* items, uint8_t n)` - list, dropdown, roller, tab titles
- `void chartPush(uint8_t id, int16_t sample)`
- `void setVertical(uint8_t id, bool v)` - bar / slider orientation
- `void setVisible(uint8_t id, bool v)`
- `void setEnabled(uint8_t id, bool e)` - disabled widgets are drawn gray and skipped by focus
- `void setPos(uint8_t id, int16_t x, int16_t y)`
- `void setSize(uint8_t id, int16_t w, int16_t h)`
- `void setStyle(uint8_t id, const Style* s)` - s must outlive the widget (nullptr = theme)
- `void setFont(uint8_t id, const UFont* f)`
- `void focus(uint8_t id)`
- `uint8_t focused()`
- `bool exists(uint8_t id) const`
- `void remove(uint8_t id)` - frees the slot (and children); its text stays in the pool
#### theme and fonts

- `void setTheme(Theme t)`
- `Theme theme() const`
- `void setDefaultFont(const UFont* f)`
- `void addFallbackFont(const UFont* f)` - glyph fallback after a widget's own font
#### events

- `void onEvent(Handler h)` - global, compatible with WraithUI 1
- `void on(uint8_t id, EventCb cb, void* user = nullptr)`
- `void onBack(void (*h)())` - KEY_BACK when nothing else handles it
- `void onScreenLoad(void (*h)(uint8_t screen))`
#### screens

- `void show(uint8_t screen, Trans t = TR_NONE, uint16_t ms = 450)`
- `void push(uint8_t screen, Trans t = TR_NONE, uint16_t ms = 450)` - remember the current screen (depth 6) ...
- `bool pop(Trans t = TR_NONE, uint16_t ms = 450)` - ... and come back to it; KEY_BACK does this by default
- `uint8_t screen() const`
#### popups

- `void msgbox(const char* title, const char* text, const char* b1, const char* b2 = nullptr, const char* b3 = nullptr, MsgCb cb = nullptr, void* user = nullptr)` - modal until a button is pressed (BACK = cancel, -1)
- `bool msgboxOpen() const`
- `void toast(const char* text, uint16_t ms = 1500)`
- `void closeDropdown()`
#### animation and timers

- `bool animate(uint8_t id, Prop p, int16_t to, uint16_t ms, CasioEase::Ease e = CasioEase::OUT_CUBIC)` - Move a property to `to` over `ms` milliseconds. EV_ANIM_DONE is sent to the widget when it ends. Returns false if the pool (8) is full.
- `uint8_t timer(uint16_t periodMs, TimerCb cb, void* user = nullptr, bool repeat = true)` - returns 0..3 or NONE
- `void stopTimer(uint8_t t)`
#### input and frame loop

- `void keyDown(Key k)` - long press (OK held 600 ms -> EV_LONG) and key repeat need keyUp
- `void keyUp(Key k)`
- `void key(Key k)` - a complete press
- `bool typeChar(const char* utf8)` - into the focused input; returns false if nothing took it
- `bool backspace()`
- `bool update(uint32_t nowMs)` - Call from loop() with millis(). Redraws and calls lcd.display() only when something changed (animations ~30 fps); returns true if a frame was sent.
- `void draw()` - render the current screen into the framebuffer only

## CasioConsole
`#include <CasioConsole.h>`

> CasioConsole v0.1 - scrolling text terminal with scrollback for CasioST75256.   Author: GrayKazu A Print object: console.println("boot ok"); console.printf-style via snprintf is up to you. Word-wraps by pixel width using whatever font the lcd currently has (5x7, GFX font, or a Unicode UFont: UTF-8 sequences are never split). Keeps the last MAX_LINES lines; scrollBack() looks at older ones. draw() renders into the framebuffer (region cleared first); call lcd.display() afterwards.

### `class CasioConsole : public Print `

- `static const uint8_t MAX_LINES = 24` - ring of logical lines
- `static const uint8_t LINE_BYTES = 80` - bytes per line (long lines wrap earlier if needed)
- `explicit CasioConsole(CasioST75256& lcd, int16_t x = 0, int16_t y = 0, int16_t w = 0, int16_t h = 0)` - Region in logical lcd coordinates; w/h <= 0 means "to the right/bottom edge".
- `size_t write(uint8_t c) override`
- `void clear()`
- `void setColor(uint8_t fg)`
- `void showCursor(bool on)` - underscore after the last character
- `void scrollBack(int16_t lines)` - >0 = look at older lines, <0 = newer; clamped. New output snaps back to the bottom.
- `uint16_t lineCount() const`
- `const char* line(uint16_t i) const` - i = 0 oldest kept line, lineCount()-1 = current line
- `void draw()`

## CasioQR
`#include <CasioQR.h>`

> CasioQR v0.1 - QR code generator for CasioST75256 (versions 1..10, numeric / alphanumeric / byte modes).   Author: GrayKazu CasioQR qr; if (qr.encode("https://example.com")) qr.drawCentered(lcd);          // picks the biggest scale that fits 192x63 lcd.display(); Version 10 (57x57 modules, up to 271 bytes at ECC_L) is the largest that still fits the 63-pixel-high screen at 1 px per module. Short texts give bigger modules, which scan much more easily from a phone: version 1-2 (up to 32 bytes at ECC_L) is drawn at 2 px per module with a quiet zone. A Wi-Fi sign-in code: qr.wifi(ssid, password) then drawCentered().

### `class CasioQR`

- `enum Ecc : uint8_t { ECC_L = 0, ECC_M = 1, ECC_Q = 2, ECC_H = 3 }`
- `static const uint8_t MAX_VERSION = 10`
- `bool encode(const char* text, Ecc ecc = ECC_L, bool boostEcc = true, uint8_t minVersion = 1)` - Encode text (UTF-8 bytes). Picks numeric / alphanumeric / byte mode and the smallest version that fits. boostEcc: raise the error correction level for free when the chosen version has room. Returns false if the text does not fit version MAX_VERSION at the requested level.
- `bool wifi(const char* ssid, const char* password, const char* type = "WPA", Ecc ecc = ECC_M)` - "WIFI:T:WPA;S:<ssid>;P:<password>;;" with the required escaping (type: "WPA", "WEP" or "nopass").
- `uint8_t version() const` - 0 = nothing encoded
- `uint8_t size() const` - modules per side
- `Ecc ecc() const`
- `uint8_t mask() const`
- `bool module(int8_t x, int8_t y) const`
- `uint8_t bestScale(int16_t availW, int16_t availH, uint8_t quiet = 2) const` - Largest integer module size for which size()*scale + 2*quiet*scale fits in availW x availH (0 = does not fit).
- `void draw(CasioST75256& lcd, int16_t x, int16_t y, uint8_t scale, uint8_t quiet = 2, uint8_t dark = 3, uint8_t light = 0)` - Draw at (x, y), the top-left of the quiet zone. quiet = white border in modules (4 is the spec, 2 works on screens).
- `uint8_t drawCentered(CasioST75256& lcd, uint8_t quiet = 2, uint8_t dark = 3, uint8_t light = 0)` - Centered in the lcd (current rotation) with the best scale; returns the scale used (0 = too big for the screen).
- `static uint16_t capacityBytes(uint8_t version, Ecc ecc)` - Capacity in bytes (byte mode) of a version at an ECC level.

## CasioJpeg
`#include <CasioJpeg.h>`

> CasioJpeg v0.1 - baseline JPEG -> 192x63 4-gray (2 bits per pixel) decoder for CasioST75256.   Author: GrayKazu * Luma only: chroma blocks are entropy-decoded and skipped, so it is much faster than a full decoder. * Decodes at 1/1, 1/2, 1/4 or 1/8 scale (reduced IDCT) when the source is much bigger than the LCD. * Box-filter resize into 192x63 (stretch / letterbox / crop-fill), then 4-level quantisation with ordered (stable, flicker-free for video) or Floyd-Steinberg dithering. * Output = 63 rows x 48 bytes, 4 pixels per byte MSB first, 0 = white .. 3 = black: exactly what lcd.drawBitmap2bpp(0, 0, buf, 192, 63) draws. * Supports baseline / extended-sequential Huffman JPEG, 1 or 3 components, any common subsampling, restart markers, custom Huffman tables. Not supported: progressive, arithmetic coding, 12-bit, CMYK. Uses about 50 KB of heap (one 192x63 accumulator) on the first decode.

### `class CasioJpeg`

- `static const int16_t OUT_W = 192, OUT_H = 63`
- `static const size_t FRAME_BYTES = OUT_W * OUT_H / 4` - 3024
- `enum Fit : uint8_t { STRETCH, FIT, FILL }` - ignore aspect / letterbox / crop to fill
- `enum Dither : uint8_t { DITHER_NONE, DITHER_BAYER, DITHER_FLOYD }`
- `enum Result : int8_t { OK = 0, ERR_FORMAT = -1, ERR_TRUNCATED = -2, ERR_UNSUPPORTED = -3, ERR_DATA = -4, ERR_SIZE = -5, ERR_MEMORY = -6 }`
- `CasioJpeg()`
- `~CasioJpeg()`
- `Result decode(const uint8_t* jpg, size_t len, uint8_t* out)` - Decode one JPEG held in memory into `out` (FRAME_BYTES bytes). `out` is only written on success.
#### look

- `void setFit(Fit f)`
- `void setDither(Dither d)`
- `void setBrightness(int16_t b)` - -255..255 added to luma
- `void setContrast(float c)` - 1.0 = unchanged
- `void setGamma(float g)` - >1 brightens midtones
- `void setInvert(bool i)` - bright pixels -> black
- `void setBackground(uint8_t lvl)` - letterbox bars: 0 white .. 3 black
#### info about the last decoded frame

- `uint16_t srcWidth() const`
- `uint16_t srcHeight() const`
- `uint8_t scaleShift() const` - IDCT reduction used (0..3)
- `const uint8_t* gray() const` - 192x63 luma after brightness/contrast/gamma

## CasioMjpegReader / CasioMjpegPlayer
`#include <CasioMjpeg.h>`

> CasioMjpeg v0.1 - MJPEG stream splitter + player for CasioST75256 / CasioJpeg.   Author: GrayKazu Works with anything that is an Arduino Stream: a File on LittleFS/SD, a WiFiClient (HTTP multipart MJPEG from an ESP32-CAM...), Serial. Frames are found by walking the JPEG markers, so multipart boundaries, junk between frames and embedded EXIF thumbnails are all fine.

### `class CasioMjpegReader`

- `CasioMjpegReader(Stream& s, uint8_t* buf, size_t cap)` - buf: memory that holds one frame at a time (cap bytes: size it for your biggest frame, 16-32 KB is typical)
- `bool poll(size_t* len)` - Consume the bytes that are available. Returns true when a complete frame (SOI..EOI) is in buf; *len = its size. Never blocks: call it again later if it returns false. Oversized frames are dropped.
- `void reset()` - call after seeking the stream
- `uint32_t dropped() const`
- `uint32_t frames() const`
- `const uint8_t* buffer() const` - holds the frame after poll() returned true
### `class CasioMjpegPlayer`

- `CasioMjpegPlayer(CasioST75256& lcd, Stream& s, uint8_t* frameBuf, size_t cap)`
- `CasioJpeg& decoder()` - fit, dither, brightness, contrast, gamma...
- `void setFps(float fps)` - 0 = show frames as they arrive
- `bool update(uint32_t nowMs)` - Call from loop() with millis(). Decodes and shows at most one frame; late frames are skipped to catch up. Returns true when a frame was shown.
- `void reset()`
- `uint32_t shown() const`
- `uint32_t skipped() const` - dropped to keep the frame rate
- `uint32_t errors() const` - frames the decoder rejected
- `uint32_t lastDecodeMs() const`
- `int8_t lastResult() const`
- `const uint8_t* frame() const` - last decoded 192x63 2bpp frame

## CasioC2vPlayer
`#include <CasioC2v.h>`

> CasioC2v v0.1 - player for pre-baked 192x63 4-gray video (.c2v, made with tools/c2vtool.py).   Author: GrayKazu No JPEG decoding at all: frames are stored as delta + run-length coded 2bpp, so a frame costs microseconds to unpack and the whole clip is tiny for mostly-static scenes. Works on any Arduino Stream (LittleFS/SD File, ...), reads without blocking (data may arrive in pieces), and can loop when you give it a rewind function.

### `class CasioC2vPlayer`

- `static const size_t FRAME_BYTES = 3024`
- `typedef bool (*Rewind)(Stream&)` - e.g. [](Stream& s) { return ((File&)s).seek(0); }
- `CasioC2vPlayer(CasioST75256& lcd, Stream& s)`
- `void setLoop(bool loop, Rewind r = nullptr)`
- `void setFps(float fps)` - 0 = file's rate, <0 = no pacing, >0 = that rate
- `bool update(uint32_t nowMs)` - Call from loop() with millis(). Shows at most one frame. Late frames are unpacked (they may be deltas) but not drawn, so playback keeps its speed. Returns true when a frame was shown.
- `void reset()` - after you seeked the stream to 0 yourself
- `bool finished() const` - played everything and not looping
- `bool failed() const` - corrupt stream: call reset() after rewinding
- `uint32_t frameIndex() const` - frames consumed in the current pass
- `uint32_t frameCount() const`
- `float fps() const`
- `uint32_t shown() const`
- `uint32_t skipped() const`
- `const uint8_t* frame() const` - last unpacked frame

## CasioTween / CasioEase
`#include <CasioTween.h>`

> CasioTween - easing functions and a tiny time-based tween (no allocation).   Author: GrayKazu CasioTween t; t.start(0, 100, millis(), 400, CasioEase::OUT_BOUNCE); float v = t.value(millis());      // 0..100, exactly 100 once finished

### `struct CasioTween`

- `void start(float f, float t, uint32_t now, uint32_t ms, CasioEase::Ease e = CasioEase::OUT_CUBIC)`
- `bool done(uint32_t now) const`
- `float value(uint32_t now) const`

## UFont / UGlyph (Unicode font format)
`#include <CasioUFont.h>`

> Unicode bitmap font format used by CasioST75256 (build fonts with tools/font2c.py).

### `struct UGlyph`

### `struct UFont`


## GFXfont (Adafruit GFX font format)
`#include <gfxfont.h>`

> Font structs, binary-compatible with Adafruit GFX fonts (same include guard, so both libraries can coexist). Fonts made with Adafruit's fontconvert, or the FreeSans*/FreeMono* headers, work with CasioST75256::setFont().

