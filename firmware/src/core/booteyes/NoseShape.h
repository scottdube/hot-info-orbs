#pragma once
// The nose between the boot eyes, on orb 3 (issue #24). Drawn in code, alone
// on a black orb, in the color of the eye lids picked on the settings page
// ("Eye lids", LidPalette.h), so the three orbs read as one face. Pure logic
// with no Arduino calls, so test/test_booteyes runs it on the host;
// NoseRenderer.cpp puts it on the panel.
//
// Why drawn (2026-11-12): the nose was a fixed JPEG, a mosaic of board
// photos. On an orb it read as an oversized logo, and a club member took it
// for his own start-up picture drawn wrong. Scott: "draw it normally and use
// the same skin tone as selected for the eye lids". Rejected: a whole orb of
// skin with the nose shaded onto it (Scott picked the nose on black from a
// side-by-side sheet), and another JPEG per lid style (eleven images in flash
// for something forty lines can draw).
//
// The outline is a signed distance field: a tapered bridge, a round tip and
// two wings, blended. Flat lid styles shade from the lid's shadow color at
// the outline to its skin color inside. Metal styles run their lid ramp in
// from the outline, as the lids run it in from the lid edge.
#include "LidPalette.h"
#include <cmath>

namespace nose {

// The outline's bounding box, panel px: [kLeft, kRight) x [kTop, kBottom).
// Everything outside is black; the renderer does no math there.
constexpr int kLeft = 54, kRight = 186, kTop = 54, kBottom = 208;

// Lid style for a stored "lids" value; the demo value and anything out of
// range give the default, as on the eyes
inline const lids::Style &style(int index) { return lids::kStyles[index >= 0 && index < lids::kCount ? index : lids::kDefault]; }

namespace detail {
inline float clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
// 0 at a, 1 at b, eased
inline float smooth(float a, float b, float x) {
    const float t = clamp01((x - a) / (b - a));
    return t * t * (3.0f - 2.0f * t);
}
// The smaller of two distances, with the crease between the shapes rounded over k px
inline float blend(float a, float b, float k) {
    const float h = clamp01(0.5f + 0.5f * (b - a) / k);
    return b + (a - b) * h - k * h * (1.0f - h);
}
inline float disc(float x, float y, float cx, float cy, float r) { return std::sqrt((x - cx) * (x - cx) + (y - cy) * (y - cy)) - r; }
// A capsule on the center line, radius r0 at y0 widening to r1 at y0 + len
inline float taper(float x, float y, float y0, float r0, float r1, float len) {
    x = std::fabs(x);
    y -= y0;
    const float b = (r0 - r1) / len, a = std::sqrt(1.0f - b * b);
    const float k = a * y - b * x;
    if (k < 0.0f) {
        return std::sqrt(x * x + y * y) - r0;
    }
    if (k > a * len) {
        return std::sqrt(x * x + (y - len) * (y - len)) - r1;
    }
    return a * x + b * y - r0;
}
inline lids::Rgb mix(lids::Rgb a, lids::Rgb b, float t) {
    t = clamp01(t);
    auto ch = [t](uint8_t p, uint8_t q) { return uint8_t(float(p) + (float(q) - float(p)) * t + 0.5f); };
    return {ch(a.r, b.r), ch(a.g, b.g), ch(a.b, b.b)};
}
} // namespace detail

// Signed distance to the outline, px; negative inside. x from the orb's
// center line, y from the top of the orb.
inline float outline(float x, float y) {
    float d = detail::taper(x, y, 72.0f, 16.0f, 27.0f, 84.0f);                           // bridge
    d = detail::blend(d, detail::disc(x, y, 0.0f, 166.0f, 38.0f), 16.0f);                // tip
    d = detail::blend(d, detail::disc(std::fabs(x), y, 39.0f, 180.0f, 24.0f), 12.0f);    // wings
    return d;
}

// 1 in a nostril, 0 clear of it
inline float nostril(float x, float y) {
    const float ex = (std::fabs(x) - 21.0f) / 13.0f, ey = (y - 198.0f) / 6.5f;
    return 1.0f - detail::smooth(0.75f, 1.1f, std::sqrt(ex * ex + ey * ey));
}

inline bool inside(int px, int py) { return outline(float(px) - 119.5f, float(py) + 0.5f) < 0.0f; }

// The color of panel pixel (px, py), 0..239 each, in lid style s
inline lids::Rgb pixel(const lids::Style &s, int px, int py) {
    using namespace detail;
    const lids::Rgb black = {0, 0, 0};
    if (px < kLeft || px >= kRight || py < kTop || py >= kBottom) {
        return black;
    }
    const float x = float(px) - 119.5f, y = float(py) + 0.5f;
    const float d = outline(x, y);
    if (d >= 0.0f) {
        return black;
    }
    const float depth = -d;
    lids::Rgb c;
    if (s.finish == lids::Finish::Chrome || s.finish == lids::Finish::Stainless) {
        // 3 lid-map units per px puts the rim, highlight and horizon bands
        // inside the first 13 px, about as wide as they are on the lids
        c = lids::body(s, int32_t(depth * 3.0f), lids::brushLevel(py / 2));
    } else {
        lids::Rgb skin = s.skin, shadow = s.shadow;
        if (s.finish == lids::Finish::Socket) {
            // Night King: the socket ramp is near black for most of its
            // length, so run as it is the nose all but vanished. The face
            // those sockets sit in is the ramp's far end (Frost's skin);
            // shade it with the ramp's deep blue-grey.
            skin = lids::ramp(lids::kSocket, 255);
            shadow = lids::ramp(lids::kSocket, 165);
        }
        if (skin.r + skin.g + skin.b == 0) {
            // Black lids: a black nose on a black orb is a dead display.
            // A thin line of rim light on the outline says "nose".
            c = mix({72, 80, 96}, black, smooth(0.0f, 3.5f, depth)); // {46, 52, 62} was barely there on the host sheet
        } else {
            c = mix(shadow, skin, smooth(0.0f, 22.0f, depth));
            // Light from the upper left: a streak down the ridge and a spot on the tip
            const float ridge = std::exp(-((x + 5.0f) * (x + 5.0f)) / 128.0f) * smooth(60.0f, 110.0f, y) * (1.0f - smooth(150.0f, 185.0f, y));
            const float tip = std::exp(-((x + 9.0f) * (x + 9.0f) + (y - 156.0f) * (y - 156.0f)) / 338.0f);
            c = mix(c, {255, 255, 255}, 0.16f * (0.55f * ridge + tip));
        }
    }
    c = mix(c, s.lash, nostril(x, y));
    return mix(black, c, smooth(62.0f, 112.0f, y)); // the bridge fades into the forehead
}

} // namespace nose
