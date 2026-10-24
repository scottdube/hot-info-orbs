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
constexpr size_t WEBDATA_MAX_BYTES = 16 * 1024; // sized to fit without PSRAM; the XIAO's 8 MB is not relied on yet

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

// HTTPClient result codes (std only here, so the numbers are spelled out)
constexpr int WEBDATA_HTTPC_CONNECT_FAILED = -1; // HTTPC_ERROR_CONNECTION_REFUSED
constexpr int WEBDATA_HTTPC_READ_TIMEOUT = -11;  // HTTPC_ERROR_READ_TIMEOUT

// A timeout stops the clock a little early, so a run this close to the limit counts as one
constexpr uint32_t WEBDATA_TIMEOUT_SLACK_MS = 250;

// Did the fetch run out its 5 s? True for a slow reply too, hence also used for a body that stalls
inline bool webDataRanToTimeout(uint32_t ms) {
    return ms + WEBDATA_TIMEOUT_SLACK_MS >= WEBDATA_TIMEOUT_MS;
}

// HTTPClient returns the same -1 for a refusal, a DNS failure and a connect
// that timed out, so the elapsed time is the only way to tell "nothing
// answered" (the commonest mistake: host off, wrong address) from an
// immediate refusal. nullptr = not a connect failure; the caller words it.
inline const char *webDataConnectError(int code, uint32_t ms) {
    if (code == WEBDATA_HTTPC_READ_TIMEOUT) {
        return "timeout";
    }
    if (code == WEBDATA_HTTPC_CONNECT_FAILED) {
        return webDataRanToTimeout(ms) ? "timeout" : "can't connect";
    }
    return nullptr;
}

// After a failed fetch: at least a minute before the next try. A silent host
// blocks loop() for the full 5 s each time, so retrying at the 10 s a good
// reply may have asked for would cost half of every cycle.
constexpr uint32_t WEBDATA_RETRY_AFTER_FAIL_MS = 60000;
inline uint32_t webDataRetryDelay(uint32_t currentDelayMs) {
    return currentDelayMs < WEBDATA_RETRY_AFTER_FAIL_MS ? WEBDATA_RETRY_AFTER_FAIL_MS : currentDelayMs;
}
