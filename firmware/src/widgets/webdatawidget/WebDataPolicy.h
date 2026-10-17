#pragma once
// Web Data fetch limits. std only, so it is covered by `pio test -e native`
// (test/test_custompages).

#include <cstddef>
#include <cstdint>

// Upstream honored any "interval", default 1 s: a page left on screen fetched
// every second. Clamped so one member's JSON cannot keep the orb busy.
constexpr uint32_t WEBDATA_MIN_INTERVAL_MS = 10000;
constexpr uint32_t WEBDATA_DEFAULT_INTERVAL_MS = 60000;
constexpr uint32_t WEBDATA_TIMEOUT_MS = 5000; // connect, and each read
constexpr size_t WEBDATA_MAX_BYTES = 16 * 1024; // the SuperMini has no PSRAM

// Refresh period from the reply's "interval" (ms, as upstream)
inline uint32_t webDataInterval(bool present, long ms) {
    if (!present) {
        return WEBDATA_DEFAULT_INTERVAL_MS;
    }
    if (ms < (long)WEBDATA_MIN_INTERVAL_MS) {
        return WEBDATA_MIN_INTERVAL_MS;
    }
    return (uint32_t)ms;
}
