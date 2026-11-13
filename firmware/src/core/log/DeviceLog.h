#pragma once
// The text side of the device log: what one line looks like, when a repeat
// is counted instead of written, how many lines an hour may take, and when a
// file rolls over. std only, so it is covered by `pio test -e native`
// (test/test_devicelog). Storage and the web page are in DeviceLogStore.cpp.
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
#include <cstring>
#include <ctime>
#include <string>

namespace devlog {

// Per file; the orb keeps the current file and the one before it. About 230
// lines each at a typical 70 bytes.
constexpr size_t kFileCap = 16384;

// The same on a small filesystem (128 KB on a 4 MB board against 384 KB):
// half, so the start-up picture keeps its room.
inline size_t fileCap(size_t filesystemBytes) {
    return filesystemBytes >= 256 * 1024 ? kFileCap : kFileCap / 2;
}

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
inline bool needsRotate(size_t fileSize, size_t addSize, size_t cap = kFileCap) {
    return fileSize > 0 && fileSize + addSize > cap;
}

// False until the first time sync: the clock library counts up from zero
// until then, plus or minus the time zone. Any date after 2023 is a real one.
inline bool clockSet(int64_t localEpoch) {
    return localEpoch > 1700000000;
}

// The date of an event at `up` seconds of uptime, from one clock reading
// (refEpoch at refUp). Events wait in the Inbox before they are written, and
// the line before a restart is written after the last reading. 0 = no clock.
inline int64_t epochAt(int64_t refEpoch, uint32_t refUp, uint32_t up) {
    if (!clockSet(refEpoch)) {
        return 0;
    }
    return refEpoch + static_cast<int64_t>(up) - static_cast<int64_t>(refUp);
}

// Events waiting for the main loop to write them. The WiFi task reports
// events too, and it must never touch the flash. Not locked: the caller
// holds a lock around push(), pop() and takeLost().
constexpr int kInboxSize = 16;
struct Entry {
    uint32_t up;
    char msg[kMsgMax + 1];
};
class Inbox {
public:
    // False when full: the event is dropped and counted. The oldest are
    // kept, since what came first says how the trouble began.
    bool push(uint32_t upSeconds, const char *msg) {
        if (m_count == kInboxSize) {
            m_lost++;
            return false;
        }
        Entry &e = m_entries[(m_head + m_count) % kInboxSize];
        e.up = upSeconds;
        strncpy(e.msg, msg, kMsgMax);
        e.msg[kMsgMax] = 0;
        m_count++;
        return true;
    }

    bool pop(Entry &out) {
        if (!m_count) {
            return false;
        }
        out = m_entries[m_head];
        m_head = (m_head + 1) % kInboxSize;
        m_count--;
        return true;
    }

    // Events dropped since the last call
    uint16_t takeLost() {
        const uint16_t n = m_lost;
        m_lost = 0;
        return n;
    }

private:
    Entry m_entries[kInboxSize];
    int m_head{0};
    int m_count{0};
    uint16_t m_lost{0};
};

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

// WiFi disconnect reasons (0-255) already reported in this outage. A join
// every 30 s can fail with two reasons in turn for hours: each is news once,
// and the "still down" lines carry the counts.
class ReasonsSeen {
public:
    bool firstTime(uint8_t reason) {
        uint32_t &word = m_bits[reason / 32];
        const uint32_t bit = 1u << (reason % 32);
        const bool first = !(word & bit);
        word |= bit;
        return first;
    }

    void reset() { memset(m_bits, 0, sizeof(m_bits)); }

private:
    uint32_t m_bits[8] = {};
};

// One page's refreshes as events: the first that fails, the data going
// stale, and the first good one after that. Never one line per refresh.
class PageWatch {
public:
    // Call after every refresh. The event text, "" when it is not news.
    std::string step(uint32_t upSeconds, bool ok, bool stale) {
        if (ok) {
            if (!m_failing) {
                return "";
            }
            m_failing = false;
            return "ok again after " + std::to_string(upSeconds - m_failedAt) + " s"; // since the first failure
        }
        if (!m_failing) {
            m_failing = true;
            m_failedAt = upSeconds;
            m_staleSaid = stale;
            return stale ? "refresh failed, data now stale" : "refresh failed";
        }
        if (stale && !m_staleSaid) {
            m_staleSaid = true;
            return "data now stale";
        }
        return "";
    }

private:
    bool m_failing{false};
    bool m_staleSaid{false};
    uint32_t m_failedAt{0};
};

} // namespace devlog
