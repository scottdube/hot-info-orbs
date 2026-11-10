#pragma once
// Iris colors for the boot eyes, picked on the settings page ("Iris", NVS key
// "iris"). Pure logic with no Arduino calls, so test/test_booteyes runs it on
// the host.
//
// "Natural" draws the example's iris texture as it is. Every other color
// keeps the texture's light and dark (the fibres, the dark ring at the edge)
// and maps it onto a ramp from a dark to a light shade of that color: each
// texel's brightness, stretched over the texture's own range, picks a point
// on the ramp. Rejected: hue-rotating the texture. It is green-hazel with
// little saturation in places, so a rotation gives muddy, uneven colors;
// a ramp gives a clean blue every time.
//
// The stored value is an index into kIrises, so colors are only ever APPENDED.
#include "LidPalette.h" // lids::Rgb, lids::to565
#include <cstdint>

namespace iriscolor {

struct Iris {
    const char *name;
    lids::Rgb dark, light; // ramp ends; unused for Natural
};

static const Iris kIrises[] = {
    {"Natural", {0, 0, 0}, {0, 0, 0}},
    {"Blue", {12, 30, 80}, {150, 200, 250}},
    {"Brown", {30, 14, 4}, {175, 115, 60}},
    {"Green", {10, 45, 20}, {150, 215, 120}},
    {"Grey", {35, 40, 48}, {200, 210, 220}},
    {"Amber", {55, 28, 0}, {250, 195, 70}},
    {"Violet", {35, 10, 60}, {200, 160, 245}},
    {"Red", {60, 0, 0}, {255, 90, 60}},
};
constexpr int kCount = int(sizeof(kIrises) / sizeof(kIrises[0]));
constexpr int kNatural = 0;
constexpr int kDefault = kNatural; // as shipped in 1.3.0

inline bool valid(int v) { return v >= 0 && v < kCount; }

// Brightness 0..~250 of an RGB565 texel (Rec. 601 weights). Shifts only, no
// division: it runs per iris pixel, and the ramp stretches the range anyway.
inline uint8_t luma565(uint16_t p) {
    const uint32_t r = (p >> 8) & 0xF8, g = (p >> 3) & 0xFC, b = (p << 3) & 0xF8;
    return uint8_t((r * 77 + g * 150 + b * 29) >> 8);
}

// Ramp for one color: out[luma] = RGB565. lo..hi is the texture's brightness
// range (see range()), stretched over the whole ramp.
inline void buildRamp(const Iris &c, int lo, int hi, uint16_t out[256]) {
    if (hi <= lo) {
        hi = lo + 1;
    }
    for (int y = 0; y < 256; y++) {
        int t = (y - lo) * 255 / (hi - lo);
        t = t < 0 ? 0 : (t > 255 ? 255 : t);
        auto mix = [t](uint8_t a, uint8_t b) { return uint8_t(a + (int(b) - a) * t / 255); };
        out[y] = lids::to565({mix(c.dark.r, c.light.r), mix(c.dark.g, c.light.g), mix(c.dark.b, c.light.b)});
    }
}

// Brightness range of a texture from its luma histogram, ignoring the darkest
// and brightest 2% so a few specks do not flatten the ramp
inline void range(const uint32_t hist[256], int &lo, int &hi) {
    uint32_t total = 0;
    for (int i = 0; i < 256; i++) {
        total += hist[i];
    }
    const uint32_t cut = total / 50;
    uint32_t acc = 0;
    lo = 0;
    while (lo < 255 && acc + hist[lo] <= cut) {
        acc += hist[lo++];
    }
    acc = 0;
    hi = 255;
    while (hi > 0 && acc + hist[hi] <= cut) {
        acc += hist[hi--];
    }
    if (hi <= lo) {
        hi = lo + 1;
    }
}

} // namespace iriscolor
