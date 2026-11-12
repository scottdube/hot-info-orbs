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
// Ice: a short ramp that starts at mid ice blue, not near black, so the iris
// reads bright and glowing and the fibres stay faint. Color only; the full
// Night King eye is its own choice, below.
//
// Night King (2026-11-11): a whole eye, not just a color, so it is its own
// choice and Ice stays as it was. From the EW season 8 cover close-ups and the
// GamesRadar piece on them: a dark seven-pointed star for a pupil, small blue
// circles with black dots scattered through the iris ("many eyes"), and the
// whole eyeball glowing blue with no white. The renderer draws these only
// when Iris::nightKing is set; nightking:: below is the geometry, pure so the
// host tests can count the star's points.
//
// The stored value is an index into kIrises, so colors are only ever APPENDED.
#include "LidPalette.h" // lids::Rgb, lids::to565
#include <cstdint>

namespace iriscolor {

struct Iris {
    const char *name;
    lids::Rgb dark, light; // ramp ends; unused for Natural
    bool nightKing;        // star pupil, specks, blue sclera (see nightking::)
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
    {"Ice", {90, 170, 230}, {225, 245, 255}},
    {"Night King", {25, 105, 200}, {180, 235, 255}, true},
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

namespace nightking {

// Colors. The sclera keeps the stock texture's shading (it darkens toward the
// edges) mapped onto a dusky blue ramp, a step below the iris so the iris
// still reads as the brightest part.
constexpr lids::Rgb kScleraDark = {14, 32, 70};
constexpr lids::Rgb kScleraLight = {110, 170, 220};
constexpr lids::Rgb kPupil = {6, 14, 32};      // the star
constexpr lids::Rgb kSpeckRing = {50, 130, 210}; // "blue circle"
constexpr lids::Rgb kSpeck = {8, 16, 36};       // "black dot"

// Iris texture radius scale. The stock 110 leaves the inner 41% of the radius
// to a round black pupil baked into the sclera texture; the star is much
// smaller, so the texture is stretched inward instead (rows clamp at 63).
constexpr uint32_t kIrisScale = 80;

// The star, in polar-map units: raw distance 0 at the iris edge .. 127 at the
// centre; angle 0..511 with 128 straight up. A pixel is pupil when its
// distance from the centre is under kCore, plus up to kTip at a point.
constexpr int kPoints = 7;
constexpr int kCore = 18; // ~14% of the iris radius
constexpr int kTip = 44;  // ~35%, the points' reach
constexpr int kUp = 128;

inline bool pupil(uint32_t raw, uint32_t angle) {
    const int dist = 127 - int(raw);
    if (dist < kCore) {
        return true;
    }
    if (dist >= kTip) {
        return false;
    }
    const int phase = int(((angle + 512 - kUp) * kPoints) % 512); // 0 = on a point
    const int off = phase < 256 ? phase : 512 - phase;            // 0..256 from the nearest point
    const int t = 256 - off;                                      // 256 on a point, 0 between
    const int reach = kCore + (kTip - kCore) * t * t / (256 * 256); // squared: narrow points
    return dist < reach;
}

// Specks, in half-source-pixels from the iris centre (= panel pixels; the iris
// radius is 79). Placed by hand, irregular on purpose, all between the star's
// points and the dark ring at the iris edge. The eye is drawn in source pixels
// (2x2 on the panel), so a dot under r = 4 comes out as a square speck and its
// ring vanishes: radius 2 was tried and read as dust.
struct Speck {
    int8_t x, y, r; // r = the dot; the blue ring is kRing wider
};
static const Speck kSpecks[] = {{-36, -28, 5}, {26, -52, 4}, {58, 6, 4},  {16, 44, 5},
                                {-34, 50, 4},  {-62, 4, 4},  {40, -24, 4}};
constexpr int kSpeckCount = int(sizeof(kSpecks) / sizeof(kSpecks[0]));
constexpr int kRing = 3;

// 0 = plain iris, 1 = a speck's blue ring, 2 = its black dot
inline int speck(int x, int y) {
    for (int i = 0; i < kSpeckCount; i++) {
        const int dx = x - kSpecks[i].x, dy = y - kSpecks[i].y;
        const int d2 = dx * dx + dy * dy, r = kSpecks[i].r, R = r + kRing;
        if (d2 <= R * R) {
            return d2 <= r * r ? 2 : 1;
        }
    }
    return 0;
}

} // namespace nightking

} // namespace iriscolor
