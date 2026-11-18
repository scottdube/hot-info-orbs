#pragma once
// Device-log lines for issue #52: the clock's colon stops blinking for half a
// minute while the second tick keeps going. The colon is lit on even seconds
// and dark on odd ones, read once per loop pass; when each pass takes about
// two seconds the second it reads is always even (or always odd) and the
// colon holds, while the tick still moves, two marks at a time.
//
// Suspect, NOT established: the NTP client (arduino-libraries/NTPClient 3.2.1)
// waits up to 1 s for an answer, and after a failure it tries again on the
// very next pass, so a time server or DNS that does not answer holds every
// pass. These watches log the sync and any run of slow passes, with what
// each part of the pass took, so the next freeze names its cause.
//
// Pure, so it builds on the host:  pio test -e native -f test_loopwatch
#include "DeviceLog.h"
#include <cstdint>
#include <string>

namespace devlog {

constexpr uint32_t kSlowSyncMs = 500; // a good sync slower than this is news
constexpr uint32_t kSlowPassMs = 1000; // a pass this long lets the clock miss a second
constexpr int kSlowRun = 3; // passes in a row before it is news

inline std::string fit(const std::string &s) {
    return s.size() <= kMsgMax ? s : s.substr(0, kMsgMax);
}

// Call after every GlobalTime::updateTime(). attempted: a sync was due and
// was tried; ok: it got the time; tookMs: how long the call held the loop.
class SyncWatch {
public:
    std::string step(uint32_t upSeconds, bool attempted, bool ok, uint32_t tookMs) {
        if (!attempted) {
            return "";
        }
        if (ok) {
            if (m_failing) {
                m_failing = false;
                return fit("time sync ok again after " + std::to_string(m_failures) + " failures in " +
                           std::to_string(upSeconds - m_since) + " s, " + std::to_string(m_blockedMs) + " ms blocked");
            }
            return tookMs >= kSlowSyncMs ? "time sync slow: " + std::to_string(tookMs) + " ms" : "";
        }
        if (m_failing) {
            m_failures++;
            m_blockedMs += tookMs;
            return "";
        }
        m_failing = true;
        m_since = upSeconds;
        m_failures = 1;
        m_blockedMs = tookMs;
        return fit("time sync failed (" + std::to_string(tookMs) + " ms), retrying every pass");
    }

private:
    bool m_failing{false};
    uint32_t m_since{0};
    uint32_t m_failures{0};
    uint32_t m_blockedMs{0};
};

// What each part of one loop pass took, in ms
struct PassParts {
    uint32_t ntp; // GlobalTime::updateTime
    uint32_t page; // the page on screen fetching its data
    uint32_t mqtt; // MQTT feeds, serviced whatever page is up
    uint32_t draw; // drawing the page, and a page switch
};

inline std::string partsText(const PassParts &p) {
    return "(ntp " + std::to_string(p.ntp) + " page " + std::to_string(p.page) + " mqtt " + std::to_string(p.mqtt) +
           " draw " + std::to_string(p.draw) + ")";
}

// Call at the end of every loop pass that ran the pages
class SlowWatch {
public:
    std::string step(uint32_t upSeconds, uint32_t passMs, const PassParts &parts) {
        if (passMs < kSlowPassMs) {
            if (!m_count) {
                return "";
            }
            const bool news = m_count >= kSlowRun;
            const std::string out = news ? fit("slow loop end: " + std::to_string(m_count) + " passes/" +
                                               std::to_string(upSeconds - m_since) + " s, max " +
                                               std::to_string(m_maxMs) + " ms " + partsText(m_maxParts))
                                         : "";
            m_count = 0;
            return out;
        }
        if (!m_count) {
            m_since = upSeconds;
            m_maxMs = 0;
        }
        m_count++;
        if (passMs > m_maxMs) {
            m_maxMs = passMs;
            m_maxParts = parts;
        }
        if (m_count == kSlowRun) {
            return fit("slow loop: " + std::to_string(kSlowRun) + " passes over 1 s, last " + std::to_string(passMs) +
                       " ms " + partsText(parts));
        }
        return "";
    }

private:
    uint32_t m_count{0};
    uint32_t m_since{0};
    uint32_t m_maxMs{0};
    PassParts m_maxParts{};
};

} // namespace devlog
