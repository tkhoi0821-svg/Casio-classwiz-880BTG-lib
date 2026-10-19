// CasioTween - easing functions and a tiny time-based tween (no allocation).   Author: GrayKazu
//   CasioTween t; t.start(0, 100, millis(), 400, CasioEase::OUT_BOUNCE);
//   float v = t.value(millis());      // 0..100, exactly 100 once finished
#pragma once
#include <math.h>
#include <stdint.h>

namespace CasioEase {
enum Ease : uint8_t { LINEAR, IN_QUAD, OUT_QUAD, INOUT_QUAD, IN_CUBIC, OUT_CUBIC, INOUT_CUBIC,
                      OUT_BACK, OUT_BOUNCE, OUT_ELASTIC };
inline float apply(Ease e, float t) {
  if (t <= 0) return 0;
  if (t >= 1) return 1;
  switch (e) {
    case IN_QUAD:     return t * t;
    case OUT_QUAD:    return 1 - (1 - t) * (1 - t);
    case INOUT_QUAD:  return t < 0.5f ? 2 * t * t : 1 - 2 * (1 - t) * (1 - t);
    case IN_CUBIC:    return t * t * t;
    case OUT_CUBIC:   { float u = 1 - t; return 1 - u * u * u; }
    case INOUT_CUBIC: { float u = 1 - t; return t < 0.5f ? 4 * t * t * t : 1 - 4 * u * u * u; }
    case OUT_BACK:    { const float c1 = 1.70158f, c3 = c1 + 1; float u = t - 1; return 1 + c3 * u * u * u + c1 * u * u; }
    case OUT_BOUNCE:  {
      const float n = 7.5625f, d = 2.75f;
      if (t < 1 / d) return n * t * t;
      if (t < 2 / d) { t -= 1.5f / d;   return n * t * t + 0.75f; }
      if (t < 2.5f / d) { t -= 2.25f / d;  return n * t * t + 0.9375f; }
      t -= 2.625f / d; return n * t * t + 0.984375f;
    }
    case OUT_ELASTIC: { const float c4 = 2.0943951f; return powf(2, -10 * t) * sinf((t * 10 - 0.75f) * c4) + 1; }
    default:          return t;
  }
}
}

struct CasioTween {
  float from = 0, to = 0; uint32_t t0 = 0, dur = 0; CasioEase::Ease ease = CasioEase::LINEAR; bool active = false;
  void start(float f, float t, uint32_t now, uint32_t ms, CasioEase::Ease e = CasioEase::OUT_CUBIC) {
    from = f; to = t; t0 = now; dur = ms; ease = e; active = true;
  }
  bool done(uint32_t now) const { return !active || (uint32_t)(now - t0) >= dur; }
  float value(uint32_t now) const {
    if (!active) return to;
    if (dur == 0 || (uint32_t)(now - t0) >= dur) return to;
    return from + (to - from) * CasioEase::apply(ease, (float)(now - t0) / (float)dur);
  }
};
