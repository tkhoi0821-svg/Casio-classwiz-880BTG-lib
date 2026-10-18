#include "CasioJpeg.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static const uint8_t ZZ[64] = {
   0, 1, 8,16, 9, 2, 3,10,17,24,32,25,18,11, 4, 5,12,19,26,33,40,48,41,34,27,20,13, 6, 7,14,21,28,
  35,42,49,56,57,50,43,36,29,22,15,23,30,37,44,51,58,59,52,45,38,31,39,46,53,60,61,54,47,55,62,63};
static const uint8_t BAYER[4][4] = {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};

CasioJpeg::CasioJpeg() {
  memset(_huff, 0, sizeof(_huff));
  memset(_qt, 0, sizeof(_qt));
  for (int t = 0; t < 4; t++) {                      // t: 0 -> N=8, 1 -> N=4, 2 -> N=2, 3 -> N=1
    int N = 8 >> t;
    float sc = sqrtf((float)N / 8.0f);                // the N x N coefficients are scaled by N/8 in total
    for (int x = 0; x < N; x++)
      for (int u = 0; u < N; u++) {
        float cu = (u == 0) ? sqrtf(1.0f / N) : sqrtf(2.0f / N);
        _ct[t][x][u] = cu * cosf((2 * x + 1) * u * 3.14159265f / (2.0f * N)) * sc;
      }
  }
}
CasioJpeg::~CasioJpeg() { if (_acc) free(_acc); }

// ---------------- tables ----------------
void CasioJpeg::buildHuff(Huff& h) {
  memset(h.fast, 0, sizeof(h.fast));
  int code = 0, k = 0;
  for (int l = 1; l <= 16; l++) {
    h.valptr[l] = k;
    h.mincode[l] = code;
    for (int i = 0; i < h.bits[l]; i++, k++, code++) {
      if (l <= 9) {
        int first = code << (9 - l), cnt = 1 << (9 - l);
        for (int j = 0; j < cnt; j++) if (first + j < 512) h.fast[first + j] = (uint16_t)((l << 8) | h.vals[k]);
      }
    }
    h.maxcode[l] = h.bits[l] ? code - 1 : -1;
    code <<= 1;
  }
  h.maxcode[17] = 0x7FFFFFFF;
  h.ok = true;
}

void CasioJpeg::buildLut() {
  for (int i = 0; i < 256; i++) {
    float v = i / 255.0f;
    v = (v - 0.5f) * _contrast + 0.5f + _bright / 255.0f;
    if (v < 0) v = 0;
    if (v > 1) v = 1;
    if (_gamma > 0 && _gamma != 1.0f) v = powf(v, 1.0f / _gamma);
    _lut[i] = (uint8_t)(v * 255.0f + 0.5f);
  }
  _lutDirty = false;
}

// ---------------- header parsing ----------------
CasioJpeg::Result CasioJpeg::parse(const uint8_t* d, size_t n) {
  if (n < 4 || d[0] != 0xFF || d[1] != 0xD8) return ERR_FORMAT;
  size_t pos = 2;
  bool gotSof = false;
  _ri = 0; _nf = 0;
  _end = d + n;
  while (true) {
    while (pos < n && d[pos] != 0xFF) pos++;            // resync on a marker
    while (pos < n && d[pos] == 0xFF) pos++;
    if (pos >= n) return ERR_TRUNCATED;
    uint8_t m = d[pos++];
    if (m == 0x00 || m == 0x01 || (m >= 0xD0 && m <= 0xD8)) continue;      // standalone / stray
    if (m == 0xD9) return ERR_FORMAT;                                       // EOI before any scan
    if (pos + 2 > n) return ERR_TRUNCATED;
    size_t len = ((size_t)d[pos] << 8) | d[pos + 1];
    if (len < 2 || pos + len > n) return ERR_TRUNCATED;
    const uint8_t* s = d + pos + 2;
    size_t sl = len - 2;
    switch (m) {
      case 0xDB: {                                      // DQT
        while (sl >= 1) {
          uint8_t pq = s[0] >> 4, tq = s[0] & 15; s++; sl--;
          if (tq > 3) return ERR_FORMAT;
          size_t need = pq ? 128 : 64;
          if (sl < need) return ERR_TRUNCATED;
          for (int k = 0; k < 64; k++) _qt[tq][k] = pq ? ((uint16_t)s[2 * k] << 8 | s[2 * k + 1]) : s[k];
          s += need; sl -= need;
        }
        break;
      }
      case 0xC4: {                                      // DHT
        while (sl >= 17) {
          uint8_t tc = s[0] >> 4, th = s[0] & 15;
          if (tc > 1 || th > 3) return ERR_FORMAT;
          Huff& h = _huff[tc][th];
          int total = 0;
          h.bits[0] = 0;
          for (int i = 1; i <= 16; i++) { h.bits[i] = s[i]; total += s[i]; }
          if (total > 256 || sl < (size_t)17 + total) return ERR_TRUNCATED;
          memcpy(h.vals, s + 17, total);
          buildHuff(h);
          s += 17 + total; sl -= 17 + total;
        }
        break;
      }
      case 0xC0: case 0xC1: {                           // SOF0 / SOF1: Huffman, 8 bit
        if (sl < 6) return ERR_TRUNCATED;
        if (s[0] != 8) return ERR_UNSUPPORTED;
        _H = (s[1] << 8) | s[2]; _W = (s[3] << 8) | s[4]; _nf = s[5];
        if (!_W || !_H) return ERR_SIZE;
        if ((_nf != 1 && _nf != 3) || sl < (size_t)6 + 3 * _nf) return ERR_UNSUPPORTED;
        _hmax = _vmax = 1;
        for (int i = 0; i < _nf; i++) {
          _c[i].id = s[6 + 3 * i]; _c[i].h = s[7 + 3 * i] >> 4; _c[i].v = s[7 + 3 * i] & 15; _c[i].tq = s[8 + 3 * i] & 3;
          if (!_c[i].h || !_c[i].v || _c[i].h > 4 || _c[i].v > 4) return ERR_FORMAT;
          if (_c[i].h > _hmax) _hmax = _c[i].h;
          if (_c[i].v > _vmax) _vmax = _c[i].v;
        }
        gotSof = true;
        break;
      }
      case 0xC2: case 0xC3: case 0xC5: case 0xC6: case 0xC7: case 0xC9: case 0xCA: case 0xCB: case 0xCD: case 0xCE: case 0xCF:
        return ERR_UNSUPPORTED;                         // progressive, lossless, arithmetic
      case 0xDD: if (sl >= 2) _ri = (s[0] << 8) | s[1]; break;
      case 0xDA: {                                      // SOS
        if (!gotSof || sl < 1) return ERR_FORMAT;
        uint8_t ns = s[0];
        if (ns != _nf || sl < (size_t)1 + 2 * ns + 3) return ERR_UNSUPPORTED;     // single interleaved scan only
        for (int i = 0; i < ns; i++) {
          uint8_t cid = s[1 + 2 * i], t = s[2 + 2 * i];
          int idx = -1;
          for (int k = 0; k < _nf; k++) if (_c[k].id == cid) idx = k;
          if (idx < 0) return ERR_FORMAT;
          _c[idx].td = (t >> 4) & 3; _c[idx].ta = t & 3;
        }
        _scan = d + pos + len;
        return OK;
      }
      default: break;                                   // APPn, COM, ...
    }
    pos += len;
  }
}

// ---------------- scaling maps ----------------
CasioJpeg::Result CasioJpeg::setupMaps() {
  _shift = 0;
  for (int s = 3; s >= 1; s--) {
    int rw = (_W + (1 << s) - 1) >> s, rh = (_H + (1 << s) - 1) >> s;
    if (rw >= OUT_W && rh >= OUT_H) { _shift = s; break; }
  }
  _rw = (_W + (1 << _shift) - 1) >> _shift;
  _rh = (_H + (1 << _shift) - 1) >> _shift;
  if (_rw > 1024 || _rh > 1024) return ERR_SIZE;
  int32_t rw = _rw, rh = _rh;
  _rx0 = 0; _ry0 = 0; _rw2 = OUT_W; _rh2 = OUT_H;
  for (int i = 0; i < rw; i++) _xmap[i] = (int16_t)(i * OUT_W / rw);
  for (int i = 0; i < rh; i++) _ymap[i] = (int16_t)(i * OUT_H / rh);
  bool wider = rw * OUT_H > rh * OUT_W;
  if (_fit == FIT) {
    int32_t dw = OUT_W, dh = OUT_H;
    if (wider) { dh = (rh * OUT_W + rw / 2) / rw; if (dh < 1) dh = 1; if (dh > OUT_H) dh = OUT_H; }
    else       { dw = (rw * OUT_H + rh / 2) / rh; if (dw < 1) dw = 1; if (dw > OUT_W) dw = OUT_W; }
    _rw2 = dw; _rh2 = dh; _rx0 = (OUT_W - dw) / 2; _ry0 = (OUT_H - dh) / 2;
    for (int i = 0; i < rw; i++) _xmap[i] = (int16_t)(_rx0 + i * dw / rw);
    for (int i = 0; i < rh; i++) _ymap[i] = (int16_t)(_ry0 + i * dh / rh);
  } else if (_fit == FILL) {
    if (wider) {
      int32_t cw = rh * OUT_W / OUT_H; if (cw < 1) cw = 1; int32_t x0 = (rw - cw) / 2;
      for (int i = 0; i < rw; i++) _xmap[i] = (i >= x0 && i < x0 + cw) ? (int16_t)((i - x0) * OUT_W / cw) : -1;
    } else {
      int32_t ch = rw * OUT_H / OUT_W; if (ch < 1) ch = 1; int32_t y0 = (rh - ch) / 2;
      for (int i = 0; i < rh; i++) _ymap[i] = (i >= y0 && i < y0 + ch) ? (int16_t)((i - y0) * OUT_H / ch) : -1;
    }
  }
  memset(_xcnt, 0, sizeof(_xcnt)); memset(_ycnt, 0, sizeof(_ycnt));
  for (int i = 0; i < rw; i++) if (_xmap[i] >= 0) _xcnt[_xmap[i]]++;
  for (int i = 0; i < rh; i++) if (_ymap[i] >= 0) _ycnt[_ymap[i]]++;
  return OK;
}

// ---------------- entropy decoding ----------------
void CasioJpeg::bitsFill(Bits& b) {
  while (b.cnt <= 24) {
    uint32_t v = 0;
    if (!b.marker && b.p < b.end) {
      v = *b.p;
      if (v == 0xFF) {
        if (b.p + 1 < b.end && b.p[1] == 0x00) b.p += 2;
        else { b.marker = true; v = 0; b.zeros++; }
      } else b.p++;
    } else { b.zeros++; if (b.p >= b.end) b.marker = true; }
    b.buf |= v << (24 - b.cnt);
    b.cnt += 8;
  }
}

int CasioJpeg::huffDecode(Bits& b, const Huff& h) {
  if (b.cnt < 16) bitsFill(b);
  uint16_t f = h.fast[b.buf >> 23];
  if (f) { int l = f >> 8; b.buf <<= l; b.cnt -= l; return f & 255; }
  for (int l = 10; l <= 16; l++) {
    int32_t code = b.buf >> (32 - l);
    if (h.maxcode[l] >= 0 && code <= h.maxcode[l] && code >= h.mincode[l]) {
      b.buf <<= l; b.cnt -= l;
      return h.vals[h.valptr[l] + code - h.mincode[l]];
    }
  }
  return -1;
}

static inline int extend(int v, int n) { return v < (1 << (n - 1)) ? v - (1 << n) + 1 : v; }

bool CasioJpeg::skipBlock(Bits& b, Comp& c) {
  const Huff& dc = _huff[0][c.td]; const Huff& ac = _huff[1][c.ta];
  if (!dc.ok || !ac.ok) return false;
  int t = huffDecode(b, dc);
  if (t < 0 || t > 15) return false;
  int diff = 0;
  if (t) { if (b.cnt < 16) bitsFill(b); diff = extend(b.buf >> (32 - t), t); b.buf <<= t; b.cnt -= t; }
  c.pred += diff;
  for (int k = 1; k < 64;) {
    int s = huffDecode(b, ac);
    if (s < 0) return false;
    int r = s >> 4, sz = s & 15;
    if (!sz) { if (r == 15) { k += 16; continue; } break; }
    k += r + 1;
    if (b.cnt < 16) bitsFill(b);
    b.buf <<= sz; b.cnt -= sz;
  }
  return true;
}

bool CasioJpeg::lumaBlock(Bits& b, Comp& c, uint8_t* pix) {
  const Huff& dc = _huff[0][c.td]; const Huff& ac = _huff[1][c.ta];
  if (!dc.ok || !ac.ok) return false;
  const uint16_t* q = _qt[c.tq];
  int N = 8 >> _shift;
  float F[64];
  bool hasAC = false;
  memset(F, 0, sizeof(F));
  int t = huffDecode(b, dc);
  if (t < 0 || t > 15) return false;
  int diff = 0;
  if (t) { if (b.cnt < 16) bitsFill(b); diff = extend(b.buf >> (32 - t), t); b.buf <<= t; b.cnt -= t; }
  c.pred += diff;
  F[0] = (float)(c.pred * q[0]);
  for (int k = 1; k < 64;) {
    int s = huffDecode(b, ac);
    if (s < 0) return false;
    int r = s >> 4, sz = s & 15;
    if (!sz) { if (r == 15) { k += 16; continue; } break; }
    k += r;
    if (k > 63) return false;
    if (b.cnt < 16) bitsFill(b);
    int v = extend(b.buf >> (32 - sz), sz); b.buf <<= sz; b.cnt -= sz;
    int nat = ZZ[k];
    if ((nat >> 3) < N && (nat & 7) < N) { F[nat] = (float)(v * q[k]); hasAC = true; }
    k++;
  }
  if (!hasAC || N == 1) {                               // flat block: every pixel = DC / 8
    int v = (int)lroundf(F[0] / 8.0f) + 128;
    if (v < 0) v = 0;
    if (v > 255) v = 255;
    memset(pix, v, N * N);
    return true;
  }
  const float (*C)[8] = _ct[_shift];
  float tmp[8][8];
  for (int v = 0; v < N; v++)
    for (int x = 0; x < N; x++) {
      float s = 0;
      for (int u = 0; u < N; u++) s += C[x][u] * F[v * 8 + u];
      tmp[v][x] = s;
    }
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      float s = 0;
      for (int v = 0; v < N; v++) s += C[y][v] * tmp[v][x];
      int p = (int)lroundf(s) + 128;
      pix[y * N + x] = p < 0 ? 0 : (p > 255 ? 255 : p);
    }
  return true;
}

void CasioJpeg::placeBlock(const uint8_t* pix, int N, int X0, int Y0) {
  for (int i = 0; i < N; i++) {
    int Y = Y0 + i;
    if (Y >= _rh) break;
    int dy = _ymap[Y];
    if (dy < 0) continue;
    uint32_t* row = _acc + dy * OUT_W;
    for (int j = 0; j < N; j++) {
      int X = X0 + j;
      if (X >= _rw) break;
      int dx = _xmap[X];
      if (dx >= 0) row[dx] += pix[i * N + j];
    }
  }
}

CasioJpeg::Result CasioJpeg::decodeScan() {
  Comp* Y = &_c[0];
  int hy = Y->h, vy = Y->v, hm = _hmax, vm = _vmax;
  if (_nf == 1) { hy = vy = hm = vm = 1; }              // non-interleaved scan: one block per MCU
  else if (Y->h != _hmax || Y->v != _vmax) return ERR_UNSUPPORTED;
  int mcusX = (_W + 8 * hm - 1) / (8 * hm), mcusY = (_H + 8 * vm - 1) / (8 * vm);
  int N = 8 >> _shift;
  Bits b = {_scan, _end, 0, 0, false, 0};
  for (int i = 0; i < _nf; i++) _c[i].pred = 0;
  uint8_t pix[64];
  uint32_t restartCount = 0;
  for (int my = 0; my < mcusY; my++)
    for (int mx = 0; mx < mcusX; mx++) {
      if (_ri && restartCount == _ri) {                 // restart marker: realign, reset predictors
        if (b.zeros * 8 > b.cnt) return ERR_DATA;
        const uint8_t* p = b.p;
        while (p + 1 < _end && !(p[0] == 0xFF && p[1] >= 0xD0 && p[1] <= 0xD7)) p++;
        if (p + 1 >= _end) return ERR_DATA;
        b.p = p + 2; b.buf = 0; b.cnt = 0; b.marker = false; b.zeros = 0;
        for (int i = 0; i < _nf; i++) _c[i].pred = 0;
        restartCount = 0;
      }
      for (int ci = 0; ci < _nf; ci++) {
        Comp& c = _c[ci];
        int ch = (_nf == 1) ? 1 : c.h, cv = (_nf == 1) ? 1 : c.v;
        for (int v = 0; v < cv; v++)
          for (int h = 0; h < ch; h++) {
            if (ci == 0) {
              if (!lumaBlock(b, c, pix)) return ERR_DATA;
              placeBlock(pix, N, (mx * hy + h) * N, (my * vy + v) * N);
            } else if (!skipBlock(b, c)) return ERR_DATA;
          }
      }
      restartCount++;
    }
  if (b.zeros * 8 > b.cnt) return ERR_TRUNCATED;        // we consumed padding that was never in the file
  return OK;
}

// ---------------- output ----------------
void CasioJpeg::finish(uint8_t* out) {
  if (_lutDirty) buildLut();
  bool rowOk[OUT_H];
  for (int y = 0; y < OUT_H; y++) {
    rowOk[y] = _ycnt[y] != 0;
    if (!rowOk[y]) continue;
    bool colOk[OUT_W];
    int first = -1;
    for (int x = 0; x < OUT_W; x++) {
      colOk[x] = _xcnt[x] != 0;
      if (colOk[x] && first < 0) first = x;
      uint32_t cnt = (uint32_t)_xcnt[x] * _ycnt[y];
      _gray[y * OUT_W + x] = colOk[x] ? _lut[(_acc[y * OUT_W + x] + cnt / 2) / cnt] : 0;
    }
    if (first < 0) first = 0;
    for (int x = 0; x < first; x++) _gray[y * OUT_W + x] = _gray[y * OUT_W + first];      // upscaling: fill empty columns
    for (int x = first + 1; x < OUT_W; x++) if (!colOk[x]) _gray[y * OUT_W + x] = _gray[y * OUT_W + x - 1];
  }
  int firstRow = 0;
  while (firstRow < OUT_H && !rowOk[firstRow]) firstRow++;
  if (firstRow >= OUT_H) firstRow = 0;
  for (int y = 0; y < firstRow; y++) memcpy(&_gray[y * OUT_W], &_gray[firstRow * OUT_W], OUT_W);   // upscaling: fill empty rows
  for (int y = firstRow + 1; y < OUT_H; y++) if (!rowOk[y]) memcpy(&_gray[y * OUT_W], &_gray[(y - 1) * OUT_W], OUT_W);

  memset(out, 0, FRAME_BYTES);
  static int16_t errA[OUT_W + 2], errB[OUT_W + 2];
  int16_t *cur = errA, *nxt = errB;
  memset(errA, 0, sizeof(errA)); memset(errB, 0, sizeof(errB));
  for (int y = 0; y < OUT_H; y++) {
    bool inY = y >= _ry0 && y < _ry0 + _rh2;
    memset(nxt, 0, sizeof(errA));
    for (int x = 0; x < OUT_W; x++) {
      uint8_t lvl;
      if (!inY || x < _rx0 || x >= _rx0 + _rw2) lvl = _bg;
      else {
        int d = _invert ? _gray[y * OUT_W + x] : 255 - _gray[y * OUT_W + x];   // darkness 0..255
        if (_dither == DITHER_NONE) lvl = (d * 3 + 127) / 255;
        else if (_dither == DITHER_BAYER) {
          int l = (d * 48 + (((BAYER[y & 3][x & 3] * 2 + 1) * 255) >> 1)) / (255 * 16);
          lvl = l > 3 ? 3 : l;
        } else {
          int v = d + cur[x + 1];
          if (v < 0) v = 0;
    if (v > 255) v = 255;
          int l = (v * 3 + 127) / 255;
          lvl = l > 3 ? 3 : l;
          int e = v - l * 85;
          cur[x + 2] += (e * 7) >> 4; nxt[x] += (e * 3) >> 4; nxt[x + 1] += (e * 5) >> 4; nxt[x + 2] += e >> 4;
        }
      }
      out[y * (OUT_W / 4) + (x >> 2)] |= lvl << (6 - 2 * (x & 3));
    }
    int16_t* t = cur; cur = nxt; nxt = t;
  }
}

CasioJpeg::Result CasioJpeg::decode(const uint8_t* jpg, size_t len, uint8_t* out) {
  Result r = parse(jpg, len);
  if (r != OK) return r;
  if (_nf == 3 && (_c[0].h != _hmax || _c[0].v != _vmax)) return ERR_UNSUPPORTED;
  r = setupMaps();
  if (r != OK) return r;
  if (!_acc) { _acc = (uint32_t*)malloc(sizeof(uint32_t) * OUT_W * OUT_H); if (!_acc) return ERR_MEMORY; }
  memset(_acc, 0, sizeof(uint32_t) * OUT_W * OUT_H);
  r = decodeScan();
  if (r != OK) return r;
  finish(out);
  return OK;
}
