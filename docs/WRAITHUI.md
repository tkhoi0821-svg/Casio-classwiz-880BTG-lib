# WraithUI 2 guide

A small, LVGL-style retained-mode UI toolkit for the 192x63 LCD. Everything lives in fixed pools (no `malloc`):
`WRAITH_MAX_WIDGETS` widgets (default 40), `WRAITH_TEXT_POOL` bytes of text (2048), 8 animations, 4 timers. Define the two
macros before including the header to change them. Full function list: `docs/API.md`.

```cpp
#include <WraithUI.h>
WraithUI ui(lcd);
void loop() { ui.update(millis()); }          // draws and calls lcd.display() only when something changed
```

## Widgets
Create with `ui.<widget>(screen, x, y, ...)`; you get an id (`WraithUI::NONE` when the pool is full). Screens are just numbers.

| Widget | Notes |
|---|---|
| `label` `button` `checkbox` `toggle` | text widgets copy their text (`setText`); long texts: `setTextRef` (not copied) |
| `bar` `slider` `arc` `meter` `gauge` `spinbox` | numeric; `setValue/value/setRange`; bar, gauge, arc, meter and slider tween toward new values |
| `list` `dropdown` `roller` `tabview` `table` | selection widgets (`value` = selected index / tab / data row) |
| `textarea` `input` | multi-line wrapped log (scrolls, follows the end) / one-line editor |
| `panel` `tabview` | containers: layout, scrolling, clipping |
| `led` `spinner` `image` `chart` | indicators; `image` takes a 2-bit bitmap (tools/img2c.py --bits 2) |
| `custom` | your own drawing callback, clipped to the widget box |

Popups (drawn above everything): `dropdown` list, `msgbox`, `toast`.

## Containers, layout, scrolling
Children are positioned relative to the parent's content area. `setLayout` stacks them for you:
```cpp
uint8_t p = ui.panel(0, 0, 0, 192, 63);
ui.setLayout(p, WraithUI::LAYOUT_COLUMN, /*gap*/ 3, /*pad*/ 3);   // or LAYOUT_ROW, LAYOUT_ROW_WRAP
ui.setScrollable(p, true);                                         // scrollbars; follows the focused child
for (int i = 0; i < 8; i++) { uint8_t b = ui.button(0, 0, 0, 0, 14, names[i]); ui.setParent(b, p); }
```
Width 0 = automatic (stretches in a column, fits the text in a row); height 0 = automatic where the widget has a natural
height. `setAlign(container, CENTER)` aligns children on the cross axis. Tabs: `ui.setParent(child, tabview, tabIndex)`;
only the active tab's children are drawn and focusable.

## Input and focus
Feed keys with `ui.key(WraithUI::KEY_UP|DOWN|LEFT|RIGHT|OK|BACK)`, or `keyDown/keyUp` for long press (OK held 600 ms sends
`EV_LONG`) and key repeat (arrow held 400 ms repeats every 90 ms on sliders, spinboxes, lists...). Up/down moves the focus
(lists, rollers, tables and text areas scroll first); left/right changes sliders, spinboxes, arcs, toggles, tabs and moves
the cursor in an input. OK activates. BACK closes a popup, otherwise pops the screen stack (`push/pop`), otherwise returns
to the previous screen. Text: `ui.typeChar("ệ")` and `ui.backspace()` edit the focused input (UTF-8 aware).

## Events
```cpp
void onEv(uint8_t id, uint8_t ev, int16_t v, void* user) {
  if (ev == WraithUI::EV_CLICK) ...;   // button 1, list/table/roller row, input submit
  if (ev == WraithUI::EV_VALUE) ...;   // slider, toggle, checkbox, spinbox, arc, tab, dropdown, input length
}                                      // also EV_FOCUS, EV_DEFOCUS, EV_LONG, EV_CHAR, EV_OPEN, EV_CLOSE, EV_ANIM_DONE
ui.on(id, onEv, userPointer);
ui.onEvent(globalHandler);             // WraithUI 1 style: (id, value) for clicks and values only
```

## Popups
```cpp
ui.msgbox("Delete", "Remove this item?", "Yes", "No", nullptr, [](int8_t button, void*) { /* 0 = Yes, 1 = No, -1 = BACK */ });
ui.toast("Saved!", 1500);              // slides in at the bottom
```
A msgbox is modal: it takes all keys until a button is chosen. `dropdown` opens its list on OK.

## Animation and timers
```cpp
ui.animate(id, WraithUI::P_X, 120, 300, CasioEase::OUT_BOUNCE);   // P_X P_Y P_W P_H P_VALUE P_SCROLL_X P_SCROLL_Y
uint8_t t = ui.timer(1000, [](void*) { ui.setText(clock, "tick"); });   ui.stopTimer(t);
```
Screens: `ui.show(n, WraithUI::TR_GLITCH|TR_STATIC|TR_MELT|TR_CRT|TR_SLIDE_LEFT|RIGHT|UP|DOWN, ms)`.

## Themes, styles, fonts
`ui.setTheme(THEME_LIGHT | THEME_DARK | THEME_GRAY | THEME_HAUNTED)` (haunted = dark, square corners, a glitch burst on every
focus change or click). Per widget: `static WraithUI::Style s; s.bg = 3; s.fg = 0; s.border = 0; ui.setStyle(id, &s);`
(-1 = inherit; levels 0 white .. 3 black, bg 4 = transparent). Fonts: `ui.setDefaultFont(&UFONT_VI_15)`,
`ui.setFont(id, &UFONT_VI_24)`, `ui.addFallbackFont(&UFONT_TH_15)` (glyph fallback). WraithUI owns the lcd's font selection
while it draws and resets it afterwards.

## Custom widgets
```cpp
void draw(WraithUI& ui, CasioST75256& lcd, uint8_t id, int16_t x, int16_t y, int16_t w, int16_t h, bool focused, void* user) {
  lcd.drawRect(x, y, w, h, 3); if (focused) lcd.fillRect(x + 2, y + 2, w - 4, h - 4, 3);   // clipped to the box
}
ui.custom(0, 10, 10, 60, 30, draw, nullptr, /*focusable*/ true);
```

## Limits
No touch input. Label/button text capacity is 24 bytes unless you pass `cap` (or use `setTextRef`). Widget ids are slots: after
`remove(id)` the slot is reused. Text of removed widgets stays in the text pool. Popups: one msgbox, one open dropdown and one
toast at a time. Layouts are single-pass (no content-size containers): give panels an explicit size.
