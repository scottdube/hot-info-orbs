#pragma once
// Host stub: just enough of Arduino.h for EyeRenderer.cpp (tools/eye_preview)
#include <cstdint>
#include <cstring>
#define PROGMEM
#define pgm_read_byte(p) (*(const uint8_t *)(p))
#define pgm_read_word(p) (*(const uint16_t *)(p))
inline long map(long x, long a, long b, long c, long d) { return (x - a) * (d - c) / (b - a) + c; }
template <class T, class L, class H> inline T constrain(T x, L lo, H hi) { return x < lo ? lo : (x > hi ? hi : x); }
