#include "CasioMjpeg.h"

bool CasioMjpegReader::feed(uint8_t b) {
  if (_st == S_SOI0) { if (b == 0xFF) _st = S_SOI1; return false; }
  if (_st == S_SOI1) {
    if (b == 0xD8) { _buf[0] = 0xFF; _buf[1] = 0xD8; _pos = 2; _st = S_MARK_FF; }
    else if (b != 0xFF) _st = S_SOI0;
    return false;
  }
  if (_pos >= _cap) { drop(); return false; }          // frame does not fit
  _buf[_pos++] = b;
  switch (_st) {
    case S_MARK_FF:
      if (b == 0xFF) _st = S_MARK_ID; else drop();
      break;
    case S_MARK_ID:
      if (b == 0xFF) break;                            // fill bytes
      if (b == 0xD9) { _st = S_SOI0; _frames++; return true; }
      if (b == 0x00 || b == 0xD8) { drop(); break; }
      if (b == 0x01 || (b >= 0xD0 && b <= 0xD7)) { _st = S_MARK_FF; break; }
      _m = b; _st = S_LEN_H;
      break;
    case S_LEN_H: _len = (uint16_t)b << 8; _st = S_LEN_L; break;
    case S_LEN_L:
      _len |= b;
      if (_len < 2) { drop(); break; }
      _skip = _len - 2;
      _st = _skip ? S_SKIP : (_m == 0xDA ? S_ENT : S_MARK_FF);
      break;
    case S_SKIP:
      if (--_skip == 0) _st = (_m == 0xDA) ? S_ENT : S_MARK_FF;
      break;
    case S_ENT:
      if (b == 0xFF) _st = S_ENT_FF;
      break;
    case S_ENT_FF:
      if (b == 0x00 || (b >= 0xD0 && b <= 0xD7)) _st = S_ENT;
      else if (b == 0xFF) { }
      else if (b == 0xD9) { _st = S_SOI0; _frames++; return true; }
      else { _m = b; _st = S_LEN_H; }                  // another marker segment (DHT between scans...)
      break;
    default: break;
  }
  return false;
}

bool CasioMjpegReader::poll(size_t* len) {
  while (true) {
    if (_ci >= _cn) {
      int avail = _s.available();
      if (avail <= 0) return false;
      size_t want = avail < (int)sizeof(_chunk) ? avail : sizeof(_chunk);
      int n = _s.readBytes((char*)_chunk, want);
      if (n <= 0) return false;
      _cn = n; _ci = 0;
    }
    while (_ci < _cn) {
      if (feed(_chunk[_ci++])) { *len = _pos; _pos = 0; return true; }
    }
  }
}

bool CasioMjpegPlayer::update(uint32_t now) {
  if (_period && _due && (int32_t)(now - _due) < 0) return false;          // not time for the next frame yet
  for (uint8_t k = 0; k < 8; k++) {
    size_t len;
    if (!_reader.poll(&len)) return false;
    if (_period && _due && (int32_t)(now - _due) > (int32_t)_period) {     // more than a frame late: skip decoding
      _due += _period; _skipped++; continue;
    }
    uint32_t t0 = millis();
    _lastRes = (int8_t)_jpg.decode(_reader.buffer(), len, _out);
    _lastMs = millis() - t0;
    if (_lastRes != CasioJpeg::OK) { _errors++; return false; }
    _lcd.drawBitmap2bpp(0, 0, _out, CasioJpeg::OUT_W, CasioJpeg::OUT_H);
    _lcd.display();
    _shown++;
    _due = (_due ? _due : now) + _period;
    if (_period && (int32_t)(now - _due) > (int32_t)_period) _due = now;   // resync after a long stall
    return true;
  }
  return false;
}
