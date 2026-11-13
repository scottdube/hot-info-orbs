#pragma once
// The text side of the device log: what one line looks like, when a repeat
// is counted instead of written, how many lines an hour may take, and when a
// file rolls over. std only, so it is covered by `pio test -e native`
// (test/test_devicelog). Storage and the web page are in DeviceLog.cpp.
//
// The log is for helping a builder whose orb misbehaves: they download it
// from the settings page and send it. So it records events, not values:
//
//   NEVER put a network name, a password, an API key, a station ID, a street
//   address or a web address with a key in it into an event.
//
// It lives on a small flash partition shared with the start-up picture, so
// every rule here is about staying small: one short line per event, the same
// event again is a counter, a chatty hour is capped, two files of kFileCap.

#include <cstdint>
#include <cstdio>
#include <ctime>
#include <string>

namespace devlog {

// Per file; the orb keeps the current file and the one before it. About 230
// lines each at a typical 70 bytes.
constexpr size_t kFileCap = 16384;

// Longest event text; longer is cut and ends in "..."
constexpr size_t kMsgMax = 80;

// Lines written in any one hour, repeats not counted. An outage with a join
// every 30 s and a refusal after each would otherwise fill both files in
// under two hours and push the start-up record out.
constexpr uint16_t kLinesPerHour = 60;

// "12d 03:04:05"
inline std::string upText(uint32_t upSeconds) {
    char buf[24];
    snprintf(buf, sizeof(buf), "%lud %02lu:%02lu:%02lu", static_cast<unsigned long>(upSeconds / 86400),
             static_cast<unsigned long>(upSeconds % 86400 / 3600), static_cast<unsigned long>(upSeconds % 3600 / 60),
             static_cast<unsigned long>(upSeconds % 60));
    return buf;
}

// One plain ASCII line: control characters become spaces, other bytes "?"
inline std::string clean(const std::string &msg) {
    std::string out;
    out.reserve(msg.size() < kMsgMax ? msg.size() : kMsgMax);
    for (const char ch : msg) {
        const unsigned char c = static_cast<unsigned char>(ch);
        if (c < 0x20 || c == 0x7F) {
            out += ' ';
        } else if (c > 0x7F) {
            out += '?';
        } else {
            out += ch;
        }
    }
    if (out.size() > kMsgMax) {
        out.resize(kMsgMax - 3);
        out += "...";
    }
    return out;
}

// "2026-11-13 10:52:18 | 0d 00:16:42 | wifi down, reason 202\n"
// localEpoch is seconds already shifted to the orb's time zone; 0 = the
// clock has not been set yet, shown as dashes of the same width.
inline std::string line(int64_t localEpoch, uint32_t upSeconds, const std::string &msg) {
    char when[24] = "---------- --:--:--";
    if (localEpoch > 0) {
        const time_t t = static_cast<time_t>(localEpoch);
        struct tm parts = {};
        gmtime_r(&t, &parts);
        strftime(when, sizeof(when), "%Y-%m-%d %H:%M:%S", &parts);
    }
    return std::string(when) + " | " + upText(upSeconds) + " | " + msg + "\n";
}

// True when the current file should be set aside before `addSize` more bytes
// go in. An empty file never rolls over, or one oversized text would rotate
// on every write and wipe both files.
inline bool needsRotate(size_t fileSize, size_t addSize) {
    return fileSize > 0 && fileSize + addSize > kFileCap;
}

// Turns events into the text to append to the file
class Writer {
public:
    // The text to write for this event; empty when nothing needs writing
    // (a repeat, or the hour is used up)
    std::string add(int64_t localEpoch, uint32_t upSeconds, const std::string &msg) {
        const std::string m = clean(msg);
        if (m_haveLast && m == m_last) {
            m_repeats++;
            m_repEpoch = localEpoch;
            m_repUp = upSeconds;
            return "";
        }
        if (!allow(upSeconds)) {
            return "";
        }
        // The count first: "the line above" is still the last line written
        std::string out = flush();
        if (m_dropped) {
            out += line(localEpoch, upSeconds,
                        std::to_string(m_dropped) + (m_dropped == 1 ? " event" : " events") +
                            " not logged: more than " + std::to_string(kLinesPerHour) + " in an hour");
            m_dropped = 0;
        }
        out += line(localEpoch, upSeconds, m);
        m_last = m;
        m_haveLast = true;
        return out;
    }

    // A repeat count not written yet, for the page and the download
    std::string pending() const {
        if (!m_repeats) {
            return "";
        }
        return line(m_repEpoch, m_repUp,
                    "same as the line above, " + std::to_string(m_repeats) + (m_repeats == 1 ? " more time" : " more times"));
    }

    // The same text, handed over once: call before a restart
    std::string flush() {
        const std::string out = pending();
        m_repeats = 0;
        return out;
    }

private:
    bool allow(uint32_t upSeconds) {
        if (!m_windowOpen || upSeconds - m_windowStart >= 3600) {
            m_windowOpen = true;
            m_windowStart = upSeconds;
            m_used = 0;
        }
        if (m_used < kLinesPerHour) {
            m_used++;
            return true;
        }
        m_dropped++;
        return false;
    }

    std::string m_last;
    bool m_haveLast{false};
    uint32_t m_repeats{0};
    int64_t m_repEpoch{0};
    uint32_t m_repUp{0};
    bool m_windowOpen{false};
    uint32_t m_windowStart{0};
    uint16_t m_used{0};
    uint32_t m_dropped{0};
};

// When a long-running condition (WiFi down, a page stale) is worth another
// line: after 1, 5, 15 and 60 minutes, then hourly. So an orb that is
// restarted before the condition ends still leaves a record of how long it
// had lasted, without a line per retry.
class Milestones {
public:
    // Call with seconds since the condition began; true once per step
    bool due(uint32_t elapsedSeconds) {
        if (elapsedSeconds < m_next) {
            return false;
        }
        while (m_next <= elapsedSeconds) {
            if (m_next < 300) {
                m_next = 300;
            } else if (m_next < 900) {
                m_next = 900;
            } else if (m_next < 3600) {
                m_next = 3600;
            } else {
                m_next += 3600;
            }
        }
        return true;
    }

    void reset() { m_next = 60; }

private:
    uint32_t m_next{60};
};

} // namespace devlog
