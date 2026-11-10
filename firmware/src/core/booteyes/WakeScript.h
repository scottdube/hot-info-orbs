#pragma once
// Keyframe script for the boot eyes (docs/superpowers/specs/2026-11-10-boot-eyes-design.md):
// groggy while WiFi connects, then a 2 s finale once it has. Pure logic with no
// Arduino calls, so test/test_booteyes runs it on the host.
#include <cstddef>
#include <cstdint>

struct EyePose {
    uint16_t open; // 0 = lids shut, 256 = lids free (normal tracking)
    int16_t gazeX; // 0..1023, 512 = center
    int16_t gazeY; // 0..1023, 512 = center, low = looking down
    bool done;     // finale finished
};

namespace wake {
struct Key {
    uint32_t ms;
    int16_t v;
};
struct GazeKey {
    uint32_t ms;
    int16_t x, y;
};

// Groggy intro, once from begin()
static const Key kIntroOpen[] = {{0, 0}, {1500, 0}, {2600, 70}, {3200, 60}, {3500, 0}, {3900, 0}};
static const GazeKey kIntroGaze[] = {{0, 512, 380}, {3900, 512, 400}};
// Groggy loop, repeating from kIntroMs until connected
static const Key kLoopOpen[] = {{0, 0}, {1000, 110}, {1700, 90}, {2100, 150}, {2700, 40}, {3000, 0}};
static const GazeKey kLoopGaze[] = {{0, 512, 400}, {1700, 512, 420}, {3000, 512, 400}};
// Finale, ms from its start. 0..kEaseMs eases from the groggy pose to the first key.
static const Key kFinaleOpen[] = {{300, 170}, {1500, 170}, {1600, 0}, {1800, 256}, {2000, 256}};
static const GazeKey kFinaleGaze[] = {{300, 512, 512},  {700, 180, 520},  {1000, 180, 520}, {1300, 850, 520},
                                      {1600, 850, 520}, {1800, 512, 512}, {2000, 512, 512}};

constexpr uint32_t kIntroMs = 3900;
constexpr uint32_t kLoopMs = 3000;
constexpr uint32_t kEarliestFinaleMs = 2600; // the first crack open always shows
constexpr uint32_t kEaseMs = 300;
constexpr uint32_t kFinaleMs = 2000;

inline int32_t smooth(uint32_t t, uint32_t t0, int32_t v0, uint32_t t1, int32_t v1) {
    if (t1 <= t0) {
        return v1;
    }
    float f = float(t - t0) / float(t1 - t0);
    f = f * f * (3 - 2 * f); // smoothstep
    return v0 + int32_t((v1 - v0) * f);
}

// Value of a keyframe track at t; holds the first/last value outside it
template <typename K, typename Get>
inline int32_t track(const K *k, size_t n, uint32_t t, Get get) {
    if (t <= k[0].ms) {
        return get(k[0]);
    }
    for (size_t i = 0; i + 1 < n; i++) {
        if (t < k[i + 1].ms) {
            return smooth(t, k[i].ms, get(k[i]), k[i + 1].ms, get(k[i + 1]));
        }
    }
    return get(k[n - 1]);
}

template <typename K, size_t N>
constexpr size_t countOf(const K (&)[N]) {
    return N;
}
} // namespace wake

class WakeScript {
  public:
    void begin(uint32_t nowMs) {
        m_start = nowMs;
        m_connected = false;
    }

    void connected(uint32_t nowMs) {
        if (m_connected) {
            return;
        }
        m_connected = true;
        const uint32_t since = nowMs - m_start;
        m_finaleAt = since > wake::kEarliestFinaleMs ? since : wake::kEarliestFinaleMs;
        m_from = groggy(m_finaleAt);
    }

    EyePose at(uint32_t nowMs) const {
        const uint32_t t = nowMs - m_start;
        if (!m_connected || t < m_finaleAt) {
            return groggy(t);
        }
        return finale(t - m_finaleAt);
    }

    // t = ms since begin()
    static EyePose groggy(uint32_t t) {
        using namespace wake;
        auto v = [](const Key &k) { return int32_t(k.v); };
        auto x = [](const GazeKey &k) { return int32_t(k.x); };
        auto y = [](const GazeKey &k) { return int32_t(k.y); };
        if (t < kIntroMs) {
            return {uint16_t(track(kIntroOpen, countOf(kIntroOpen), t, v)), int16_t(track(kIntroGaze, countOf(kIntroGaze), t, x)),
                    int16_t(track(kIntroGaze, countOf(kIntroGaze), t, y)), false};
        }
        const uint32_t l = (t - kIntroMs) % kLoopMs;
        return {uint16_t(track(kLoopOpen, countOf(kLoopOpen), l, v)), int16_t(track(kLoopGaze, countOf(kLoopGaze), l, x)),
                int16_t(track(kLoopGaze, countOf(kLoopGaze), l, y)), false};
    }

  private:
    EyePose finale(uint32_t f) const {
        using namespace wake;
        if (f >= kFinaleMs) {
            return {256, 512, 512, true};
        }
        if (f < kEaseMs) {
            const Key &o = kFinaleOpen[0];
            const GazeKey &g = kFinaleGaze[0];
            return {uint16_t(smooth(f, 0, m_from.open, kEaseMs, o.v)), int16_t(smooth(f, 0, m_from.gazeX, kEaseMs, g.x)),
                    int16_t(smooth(f, 0, m_from.gazeY, kEaseMs, g.y)), false};
        }
        auto v = [](const Key &k) { return int32_t(k.v); };
        auto x = [](const GazeKey &k) { return int32_t(k.x); };
        auto y = [](const GazeKey &k) { return int32_t(k.y); };
        return {uint16_t(track(kFinaleOpen, countOf(kFinaleOpen), f, v)), int16_t(track(kFinaleGaze, countOf(kFinaleGaze), f, x)),
                int16_t(track(kFinaleGaze, countOf(kFinaleGaze), f, y)), false};
    }

    uint32_t m_start{0};
    bool m_connected{false};
    uint32_t m_finaleAt{0}; // ms since begin()
    EyePose m_from{0, 512, 512, false};
};
