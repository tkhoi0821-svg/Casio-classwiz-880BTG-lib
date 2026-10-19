// CasioC2v v0.1 - player for pre-baked 192x63 4-gray video (.c2v, made with tools/c2vtool.py).   Author: GrayKazu
// No JPEG decoding at all: frames are stored as delta + run-length coded 2bpp, so a frame costs microseconds to
// unpack and the whole clip is tiny for mostly-static scenes. Works on any Arduino Stream (LittleFS/SD File, ...),
// reads without blocking (data may arrive in pieces), and can loop when you give it a rewind function.
#pragma once
#include <Arduino.h>
#include "CasioST75256.h"

class CasioC2vPlayer {
 public:
  static const size_t FRAME_BYTES = 3024;
  typedef bool (*Rewind)(Stream&);                              // e.g. [](Stream& s) { return ((File&)s).seek(0); }
  CasioC2vPlayer(CasioST75256& lcd, Stream& s) : _lcd(lcd), _s(s) {}

  void setLoop(bool loop, Rewind r = nullptr) { _loop = loop; _rewind = r; }
  void setFps(float fps) { _fpsOverride = fps; if (_hdrOk) applyFps(); }       // 0 = file's rate, <0 = no pacing, >0 = that rate
  // Call from loop() with millis(). Shows at most one frame. Late frames are unpacked (they may be deltas) but not
  // drawn, so playback keeps its speed. Returns true when a frame was shown.
  bool update(uint32_t nowMs);
  void reset();                                                 // after you seeked the stream to 0 yourself
  bool finished() const { return _state == S_DONE; }            // played everything and not looping
  bool failed() const   { return _state == S_ERROR; }           // corrupt stream: call reset() after rewinding
  uint32_t frameIndex() const { return _idx; }                  // frames consumed in the current pass
  uint32_t frameCount() const { return _count; }
  float    fps() const  { return _period ? 1000.0f / _period : 0; }
  uint32_t shown() const   { return _shown; }
  uint32_t skipped() const { return _skipped; }
  const uint8_t* frame() const { return _cur; }                 // last unpacked frame

 private:
  enum State : uint8_t { S_HEADER, S_FRAMEHDR, S_PAYLOAD, S_DONE, S_ERROR };
  bool fill(uint8_t* dst, size_t need);                         // non-blocking read, true when `need` bytes are in
  bool unpack();                                                // decode _pay into _cur according to _type
  void applyFps();
  CasioST75256& _lcd; Stream& _s;
  State _state = S_HEADER; Rewind _rewind = nullptr; bool _loop = false, _hdrOk = false;
  uint8_t _hdr[14]; uint8_t _fh[3]; uint8_t _pay[FRAME_BYTES + 64];
  uint8_t _cur[FRAME_BYTES], _prev[FRAME_BYTES];
  size_t _got = 0, _need = 14; uint16_t _len = 0; uint8_t _type = 0;
  uint32_t _count = 0, _idx = 0, _period = 0, _due = 0, _shown = 0, _skipped = 0;
  uint16_t _fps100 = 0; float _fpsOverride = 0;
};
