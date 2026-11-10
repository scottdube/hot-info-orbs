#pragma once
// Eye-lid looks for the boot eyes, picked on the settings page ("Eye lids",
// NVS key "lids"). Pure logic with no Arduino calls, so test/test_booteyes
// runs it on the host.
//
// Why a choice (2026-11-10): Taupe, picked on the rig in the spike, read as
// "creepy" to a club member. The flat colors are the spike's own (Skin, Tan, Robot,
// then the Taupe/Stone/Ash blends between Skin and Robot); Black is the
// Animated_Eyes_2 original. Chrome and Stainless are new: shaded by distance
// from the lid edge, so the bands follow the lid's curve like a reflection on
// a metal shell. A flat grey read as plastic in the mind's eye, not metal.
// Frost (added the same evening): pale blue-grey, for a Night King look with
// the Ice iris and lashes off; Robot is too dark and saturated for that.
// Lash strands are their own setting ("Eyelashes", any style), not part of a
// style: they were asked for as a switch, metal included.
//
// The stored value is an index into kStyles, so styles are only ever APPENDED
// (an orb that saved "Ash" must still get Ash after an update). kDemo is the
// demo that cycles through them all.
#include <cstddef>
#include <cstdint>

namespace lids {

// Lid-map units: a lid pixel's margin is how far past the closing threshold
// its lid-map value is; small = near the lid edge
constexpr int32_t kLashW = 6;    // lash line
constexpr int32_t kShadowW = 14; // flat lids: shadow band after the lash line

enum class Finish : uint8_t { Flat, Chrome, Stainless };

struct Rgb {
    uint8_t r, g, b;
};

struct Stop {
    uint8_t m; // margin
    Rgb c;
};

struct Style {
    const char *name;
    Rgb skin, shadow, lash; // shadow unused for metal; lash = edge line and strands
    Finish finish;
};

static const Style kStyles[] = {
    {"Taupe", {170, 143, 129}, {128, 106, 95}, {30, 22, 20}, Finish::Flat},
    {"Skin", {224, 172, 138}, {176, 122, 96}, {40, 25, 20}, Finish::Flat},
    {"Tan", {160, 110, 75}, {112, 72, 50}, {35, 20, 15}, Finish::Flat},
    {"Robot", {90, 100, 115}, {55, 62, 72}, {8, 8, 10}, Finish::Flat},
    {"Stone", {157, 136, 126}, {118, 100, 92}, {30, 22, 20}, Finish::Flat},
    {"Ash", {137, 125, 123}, {100, 90, 90}, {28, 22, 22}, Finish::Flat},
    {"Black", {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, Finish::Flat},
    {"Chrome", {0, 0, 0}, {0, 0, 0}, {28, 32, 38}, Finish::Chrome},
    {"Stainless", {0, 0, 0}, {0, 0, 0}, {58, 60, 62}, Finish::Stainless},
    {"Frost", {165, 178, 192}, {118, 132, 150}, {20, 28, 40}, Finish::Flat},
};
constexpr int kCount = int(sizeof(kStyles) / sizeof(kStyles[0]));
constexpr int kDefault = 0; // Taupe, as shipped in 1.3.0
constexpr int kDemo = 100;  // cycle through every style

// Metal profiles by margin, edge to deep. Chrome: a dark rim, a hard white
// highlight, a dark "horizon" band, then a long rise back to light. Stainless:
// the same shape, much softer, and brushed (kBrush) on top.
static const Stop kChrome[] = {{6, {70, 76, 86}},     {12, {250, 252, 255}}, {20, {232, 238, 245}}, {38, {66, 74, 88}},
                               {52, {108, 118, 134}}, {110, {214, 221, 230}}, {255, {236, 240, 246}}};
static const Stop kStainless[] = {{6, {112, 114, 116}},  {14, {214, 216, 217}}, {30, {176, 178, 180}},
                                  {60, {140, 142, 144}}, {130, {186, 188, 190}}, {255, {170, 172, 174}}};
// Brushing: each source row gets one of these brightness steps
constexpr int kBrushLevels = 4;
static const int8_t kBrush[kBrushLevels] = {-10, -3, 3, 10};

inline bool valid(int v) { return (v >= 0 && v < kCount) || v == kDemo; }

inline uint16_t to565(Rgb c) { return uint16_t(((c.r & 0xF8) << 8) | ((c.g & 0xFC) << 3) | (c.b >> 3)); }

inline uint8_t clamp8(int v) { return uint8_t(v < 0 ? 0 : (v > 255 ? 255 : v)); }

template <size_t N>
inline Rgb ramp(const Stop (&s)[N], int32_t m) {
    if (m <= s[0].m) {
        return s[0].c;
    }
    for (size_t i = 0; i + 1 < N; i++) {
        if (m <= s[i + 1].m) {
            const int32_t span = s[i + 1].m - s[i].m, f = m - s[i].m;
            auto mix = [&](uint8_t a, uint8_t b) { return uint8_t(a + (int32_t(b) - a) * f / span); };
            return {mix(s[i].c.r, s[i + 1].c.r), mix(s[i].c.g, s[i + 1].c.g), mix(s[i].c.b, s[i + 1].c.b)};
        }
    }
    return s[N - 1].c;
}

// Which brushing step a source row (0..127) gets. A fixed hash, so the
// streaks stay put frame to frame instead of shimmering.
inline int brushLevel(int row) {
    uint32_t h = uint32_t(row) * 2654435761u;
    return int((h >> 13) % kBrushLevels);
}

// Lid body color (past the lash line) at margin m, for brushing step `level`.
// Flat styles ignore the level.
inline Rgb body(const Style &s, int32_t m, int level) {
    if (m > 255) {
        m = 255;
    }
    switch (s.finish) {
    case Finish::Chrome:
        return ramp(kChrome, m);
    case Finish::Stainless: {
        const Rgb c = ramp(kStainless, m);
        const int d = kBrush[level];
        return {clamp8(c.r + d), clamp8(c.g + d), clamp8(c.b + d)};
    }
    default:
        return m < kLashW + kShadowW ? s.shadow : s.skin;
    }
}

// Demo: one style per wake cycle (asleep, groggy, finale, awake), shown from
// boot. t = ms since the eyes started.
constexpr uint32_t kDemoConnectMs = 4000; // the "WiFi joined" moment in each cycle
constexpr uint32_t kDemoCycleMs = 12000;  // 4 s groggy + 2 s finale + one 6 s awake loop
inline int demoStyle(uint32_t t) { return int((t / kDemoCycleMs) % kCount); }
inline uint32_t demoCycleTime(uint32_t t) { return t % kDemoCycleMs; }

} // namespace lids
