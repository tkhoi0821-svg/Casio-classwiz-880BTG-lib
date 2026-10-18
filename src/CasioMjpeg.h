// CasioMjpeg v0.1 - MJPEG stream splitter + player for CasioST75256 / CasioJpeg.   Author: GrayKazu
// Works with anything that is an Arduino Stream: a File on LittleFS/SD, a WiFiClient (HTTP multipart MJPEG from
// an ESP32-CAM...), Serial. Frames are found by walking the JPEG markers, so multipart boundaries, junk between
// frames and embedded EXIF thumbnails are all fine.
#pragma once
#include <Arduino.h>
#include "CasioST75256.h"
#include "CasioJpeg.h"

class CasioMjpegReader {
 public:
  // buf: memory that holds one frame at a time (cap bytes: size it for your biggest frame, 16-32 KB is typical)
  CasioMjpegReader(Stream& s, uint8_t* buf, size_t cap) : _s(s), _buf(buf), _cap(cap) {}
  // Consume the bytes that are available. Returns true when a complete frame (SOI..EOI) is in buf; *len = its size.
  // Never blocks: call it again later if it returns false. Oversized frames are dropped.
  bool poll(size_t* len);
  void reset() { _st = S_SOI0; _pos = 0; _ci = _cn = 0; }       // call after seeking the stream
  uint32_t dropped() const { return _dropped; }
  uint32_t frames() const  { return _frames; }
  const uint8_t* buffer() const { return _buf; }               // holds the frame after poll() returned true
 private:
  enum St : uint8_t { S_SOI0, S_SOI1, S_MARK_FF, S_MARK_ID, S_LEN_H, S_LEN_L, S_SKIP, S_ENT, S_ENT_FF };
  bool feed(uint8_t b);
  void drop() { _st = S_SOI0; _pos = 0; _dropped++; }
  Stream& _s; uint8_t* _buf; size_t _cap, _pos = 0;
  uint8_t _chunk[128]; uint8_t _ci = 0, _cn = 0;
  St _st = S_SOI0; uint8_t _m = 0; uint16_t _len = 0, _skip = 0;
  uint32_t _dropped = 0, _frames = 0;
};

class CasioMjpegPlayer {
 public:
  CasioMjpegPlayer(CasioST75256& lcd, Stream& s, uint8_t* frameBuf, size_t cap)
      : _lcd(lcd), _reader(s, frameBuf, cap) {}
  CasioJpeg& decoder() { return _jpg; }                         // fit, dither, brightness, contrast, gamma...
  void setFps(float fps) { _period = fps > 0 ? (uint32_t)(1000.0f / fps) : 0; }   // 0 = show frames as they arrive
  // Call from loop() with millis(). Decodes and shows at most one frame; late frames are skipped to catch up.
  // Returns true when a frame was shown.
  bool update(uint32_t nowMs);
  void reset() { _reader.reset(); _due = 0; }
  uint32_t shown() const { return _shown; }
  uint32_t skipped() const { return _skipped; }                 // dropped to keep the frame rate
  uint32_t errors() const { return _errors; }                   // frames the decoder rejected
  uint32_t lastDecodeMs() const { return _lastMs; }
  int8_t   lastResult() const { return _lastRes; }
  const uint8_t* frame() const { return _out; }                 // last decoded 192x63 2bpp frame
 private:
  CasioST75256& _lcd; CasioMjpegReader _reader; CasioJpeg _jpg;
  uint8_t _out[CasioJpeg::FRAME_BYTES];
  uint32_t _period = 0, _due = 0, _shown = 0, _skipped = 0, _errors = 0, _lastMs = 0;
  int8_t _lastRes = 0;
};
