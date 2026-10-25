# Symbols (icons in text)

53 vector icons, rendered at 12, 14 and 22 px with 4 gray levels, living in the Unicode private use area (U+E000 ...).
They are a normal Unicode font, so they go inside any text:

```cpp
#include <CasioFont_VI_15.h>
#include <CasioFont_SYM_15.h>        // matches the 15 px fonts; SYM_16 matches the 16 px Unifont fonts, SYM_26 the 24 px font
#include <CasioSymbols.h>            // SYM_WIFI, SYM_BATTERY_FULL, ... (UTF-8 string macros)
lcd.addUFont(&UFONT_VI_15); lcd.addUFont(&UFONT_SYM_15);      // symbols AFTER the text font
lcd.print(SYM_WIFI " Home " SYM_BATTERY_2);
// in WraithUI: ui.setFont(label, &UFONT_VI_15); ui.addFallbackFont(&UFONT_SYM_15);
```
`docs/symbols.png` shows all of them. Make other sizes with `tools/make_symbols.py --size 18 --cell 20 --top 1 --name UFONT_SYM_20 --out ...`
(icons are vectors, so any size is crisp). The artwork is original to this library (MIT).

| Macro | Code point |
|---|---|
| `SYM_WIFI` | U+E000 |
| `SYM_BLUETOOTH` | U+E001 |
| `SYM_BATTERY_EMPTY` | U+E002 |
| `SYM_BATTERY_1` | U+E003 |
| `SYM_BATTERY_2` | U+E004 |
| `SYM_BATTERY_FULL` | U+E005 |
| `SYM_CHARGE` | U+E006 |
| `SYM_LEFT` | U+E007 |
| `SYM_RIGHT` | U+E008 |
| `SYM_UP` | U+E009 |
| `SYM_DOWN` | U+E00A |
| `SYM_PLAY` | U+E00B |
| `SYM_PAUSE` | U+E00C |
| `SYM_STOP` | U+E00D |
| `SYM_HOME` | U+E00E |
| `SYM_SETTINGS` | U+E00F |
| `SYM_CHECK` | U+E010 |
| `SYM_CLOSE` | U+E011 |
| `SYM_PLUS` | U+E012 |
| `SYM_MINUS` | U+E013 |
| `SYM_WARNING` | U+E014 |
| `SYM_INFO` | U+E015 |
| `SYM_BELL` | U+E016 |
| `SYM_LOCK` | U+E017 |
| `SYM_UNLOCK` | U+E018 |
| `SYM_MAIL` | U+E019 |
| `SYM_HEART` | U+E01A |
| `SYM_STAR` | U+E01B |
| `SYM_SUN` | U+E01C |
| `SYM_MOON` | U+E01D |
| `SYM_TRASH` | U+E01E |
| `SYM_EDIT` | U+E01F |
| `SYM_REFRESH` | U+E020 |
| `SYM_SEARCH` | U+E021 |
| `SYM_MENU` | U+E022 |
| `SYM_POWER` | U+E023 |
| `SYM_VOLUME` | U+E024 |
| `SYM_MUTE` | U+E025 |
| `SYM_CLOCK` | U+E026 |
| `SYM_FILE` | U+E027 |
| `SYM_FOLDER` | U+E028 |
| `SYM_DOWNLOAD` | U+E029 |
| `SYM_UPLOAD` | U+E02A |
| `SYM_IMAGE` | U+E02B |
| `SYM_USER` | U+E02C |
| `SYM_PIN` | U+E02D |
| `SYM_SIGNAL` | U+E02E |
| `SYM_CLOUD` | U+E02F |
| `SYM_CALENDAR` | U+E030 |
| `SYM_MUSIC` | U+E031 |
| `SYM_KEYBOARD` | U+E032 |
| `SYM_LIGHT` | U+E033 |
| `SYM_BAN` | U+E034 |
