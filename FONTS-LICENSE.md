# Licenses of the bundled font data

The files `src/CasioFont_*.h` are **generated bitmap data derived from third-party fonts**, made with `tools/font2c.py`.
They are NOT covered by this library's `LICENSE`. They stay under the licenses of the fonts they come from, summarised
below from the license files of the Debian packages the fonts were taken from. This is a summary, not legal advice:
read the original licenses, and ask a lawyer if you plan to ship firmware commercially.

| Header(s) | Derived from | License (as stated by the packages) |
|---|---|---|
| `CasioFont_VI.h`, `_TH.h`, `_ZH_CN.h`, `_ZH_TW.h` (16 px, 1-bit) | GNU Unifont (`fonts-unifont`) | GPL-2+ (the Debian copyright file lists GNU GPL version 2 or later) |
| `CasioFont_TH_15.h` (Thai, 15 px antialiased) | Loma (TLWG `fonts-tlwg`, nectec/Loma) | "GPL-2+ with Font exception" |
| `CasioFont_VI_15.h`, `CasioFont_VI_24.h`, `CasioFont_VI_15B.h`, `CasioFont_NUM_36.h` (Vietnamese / digits) | DejaVu Sans and DejaVu Sans Bold (`fonts-dejavu-core`) | Bitstream Vera license + public-domain DejaVu changes |
| `CasioFont_ZH_CN_15.h`, `_ZH_TW_15.h` (Chinese, 15 px) | Noto Sans CJK SC / TC (`fonts-noto-cjk`) | SIL Open Font License 1.1 |

The symbol fonts (`CasioFont_SYM_*.h`) are original vector artwork drawn for this library by `tools/make_symbols.py`; they follow the library `LICENSE`.

## What this means in practice
- **Noto Sans CJK (OFL 1.1) and DejaVu (Bitstream Vera)** are permissive. Keep their copyright notices and license texts
  with what you distribute. Do not give a derived font a name containing a Reserved Font Name or "Bitstream"/"Vera"
  (the generated symbol names `UFONT_*` do not).
- **Unifont and Loma are GPL-2+.** The "font exception" in the Loma license says that a *document* that embeds the font
  or *unaltered portions of it* is not by itself covered by the GPL. The generated headers contain rasterised glyphs
  (altered form) and end up inside *firmware*, not a document, so it is **not clear** that the exception applies.
  If you distribute firmware images containing `CasioFont_VI.h`, `_TH.h`, `_ZH_CN.h`, `_ZH_TW.h` or `CasioFont_TH_15.h`,
  you may have to offer the corresponding source of the firmware under the GPL. The simplest ways to stay clear:
  - use only the permissive headers (`CasioFont_VI_15.h`, `_VI_24`, `_VI_15B`, `_NUM_36`, `CasioFont_ZH_CN_15.h`, `CasioFont_ZH_TW_15.h`) in firmware you distribute, and generate a Thai font from a font whose license you are comfortable with:
    `python tools/font2c.py --font YourThaiFont.ttf --charset ascii,th --aa --size 13 --height 15 --baseline 12 --engine raqm --name UFONT_TH_15`
  - or keep the firmware for your own use (the GPL only matters when you distribute).
- All of this applies to the font headers only. The library code (`LICENSE`, MIT) can be used as before.
- If you change `LICENSE` to another license, update the pointer lines in the README as well.
