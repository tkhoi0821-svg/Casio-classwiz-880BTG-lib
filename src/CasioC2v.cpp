#include "CasioC2v.h"
#include <string.h>

void CasioC2vPlayer::applyFps() {
  if (_fpsOverride < 0) { _period = 0; return; }
  float f = _fpsOverride > 0 ? _fpsOverride : _fps100 / 100.0f;
  _period = f > 0 ? (uint32_t)(1000.0f / f) : 0;
}
void CasioC2vPlayer::reset() {
  _state = S_HEADER; _got = 0; _need = 14; _idx = 0; _due = 0; _hdrOk = false;
  memset(_prev, 0, sizeof(_prev));
}
bool CasioC2vPlayer::fill(uint8_t* dst, size_t need) {
  while (_got < need) {
    int avail = _s.available();
    if (avail <= 0) return false;
    size_t n = need - _got; if ((int)n > avail) n = avail;
    int r = _s.readBytes((char*)dst + _got, n);
    if (r <= 0) return false;
    _got += r;
  }
  return true;
}

bool CasioC2vPlayer::unpack() {
  if (_type == 0) { if (_len != FRAME_BYTES) return false; memcpy(_cur, _pay, FRAME_BYTES); return true; }
  if (_type == 3) { memcpy(_cur, _prev, FRAME_BYTES); return _len == 0; }
  if (_type != 1 && _type != 2) return false;
  size_t o = 0, i = 0;
  while (i < _len) {                                   // RLE: 1xxxxxxx zeros, 01xxxxxx 0xFF run, 00xxxxxx literals
    uint8_t t = _pay[i++];
    if (t & 0x80) { size_t n = (t & 0x7F) + 1; if (o + n > FRAME_BYTES) return false; memset(_cur + o, 0x00, n); o += n; }
    else if (t & 0x40) { size_t n = (t & 0x3F) + 1; if (o + n > FRAME_BYTES) return false; memset(_cur + o, 0xFF, n); o += n; }
    else { size_t n = t + 1; if (i + n > _len || o + n > FRAME_BYTES) return false; memcpy(_cur + o, _pay + i, n); i += n; o += n; }
  }
  if (o != FRAME_BYTES) return false;
  if (_type == 2) for (size_t k = 0; k < FRAME_BYTES; k++) _cur[k] ^= _prev[k];
  return true;
}

bool CasioC2vPlayer::update(uint32_t now) {
  if (_state == S_DONE || _state == S_ERROR) return false;
  if (_period && _due && (int32_t)(now - _due) < 0 && _state != S_HEADER) return false;      // wait for the next frame time
  for (uint8_t guard = 0; guard < 16; guard++) {
    switch (_state) {
      case S_HEADER:
        if (!fill(_hdr, 14)) return false;
        if (memcmp(_hdr, "C2V1", 4) != 0 || (_hdr[4] | _hdr[5] << 8) != 192 || (_hdr[6] | _hdr[7] << 8) != 63) { _state = S_ERROR; return false; }
        _fps100 = _hdr[8] | _hdr[9] << 8;
        _count = (uint32_t)_hdr[10] | (uint32_t)_hdr[11] << 8 | (uint32_t)_hdr[12] << 16 | (uint32_t)_hdr[13] << 24;
        _hdrOk = true; applyFps();
        _idx = 0; _got = 0; _state = _count ? S_FRAMEHDR : S_DONE;
        memset(_prev, 0, sizeof(_prev));
        break;
      case S_FRAMEHDR:
        if (_idx >= _count) {                                           // end of the clip
          if (_loop && _rewind && _rewind(_s)) { _state = S_HEADER; _got = 0; continue; }
          _state = S_DONE; return false;
        }
        if (!fill(_fh, 3)) return false;
        _len = _fh[0] | _fh[1] << 8; _type = _fh[2]; _got = 0;
        if (_len > sizeof(_pay)) { _state = S_ERROR; return false; }
        _state = S_PAYLOAD;
        break;
      case S_PAYLOAD: {
        if (_len && !fill(_pay, _len)) return false;
        _got = 0;
        if (!unpack()) { _state = S_ERROR; return false; }
        memcpy(_prev, _cur, FRAME_BYTES);
        _idx++; _state = S_FRAMEHDR;
        if (_period && _due && (int32_t)(now - _due) > (int32_t)_period) {   // more than a frame late: keep the state, skip drawing
          _due += _period; _skipped++; break;
        }
        _lcd.drawBitmap2bpp(0, 0, _cur, 192, 63);
        _lcd.display();
        _shown++;
        _due = (_due ? _due : now) + _period;
        if (_period && (int32_t)(now - _due) > (int32_t)_period) _due = now;
        return true;
      }
      default: return false;
    }
  }
  return false;
}
