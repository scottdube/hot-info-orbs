#pragma once
// The rules of the on-orb info page, kept free of Arduino so they can be tested
// on a computer:  pio test -e native
//
// The page is asked for with the middle button held 2 seconds, from any page. It
// shows what a club member is asked for when their orb misbehaves: the firmware,
// the WiFi link, the address, the health and the data feed. Any button closes it,
// and so does a minute without one, so an orb never sits on it by accident.
//
// REJECTED: a sixth page in the rotation. It would come around on its own on every
// orb, and the page is only wanted when somebody asks.
// REJECTED: closing with the same 2 second hold. Any button is what a hand tries
// first, and the closing press does nothing else, so it cannot change a page.
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace infopage {

constexpr uint32_t kTimeoutMs = 120000; // closes by itself after this (one minute at first; Scott, 2026-11-14: two)
constexpr uint32_t kRefreshMs = 1000;  // the values are read again this often
constexpr int kOrbs = 5;
constexpr int kWholeOrb = -1; // linesToRedraw: clear the orb and draw every line

enum class Press {
    Opened, // the page was asked for
    Closed, // the page was open: the press closed it and does nothing else
    PassOn, // not ours: the page on show gets the press
    Ignored // the hold that opened the page being let go: nothing happens
};

class Session {
public:
    bool isOpen() const { return m_open; }

    // The middle button has been down for 2 seconds and still is. True on the
    // pass where that opens the page: it appears under the finger (Scott,
    // 2026-11-14: waiting for the release felt like nothing had happened).
    // With the page open already a hold does nothing; its release closes.
    bool hold(uint32_t now) {
        if (m_open || m_ignoreRelease) {
            return false;
        }
        open(now);
        m_ignoreRelease = true; // the finger still has to come off
        return true;
    }

    // Every finished button press (a button let go) goes through here first.
    // middle: it was the middle button. heldLong: it was down 2 seconds.
    Press press(bool middle, bool heldLong, uint32_t now) {
        if (middle && m_ignoreRelease) {
            m_ignoreRelease = false;
            return Press::Ignored;
        }
        if (m_open) {
            m_open = false;
            return Press::Closed;
        }
        if (!(middle && heldLong)) {
            return Press::PassOn;
        }
        // A long hold that hold() never saw: the loop was busy fetching
        open(now);
        return Press::Opened;
    }

    // True once, on the pass where the time runs out. Subtraction, so it holds
    // across the millisecond counter wrapping.
    bool timedOut(uint32_t now) {
        if (!m_open || now - m_openedAt < kTimeoutMs) {
            return false;
        }
        m_open = false;
        return true;
    }

    void close() { m_open = false; }

    // True when the values should be read and drawn: at once on opening, then
    // once a second.
    bool refreshDue(uint32_t now) {
        if (!m_open) {
            return false;
        }
        if (m_drawn && now - m_lastDraw < kRefreshMs) {
            return false;
        }
        m_drawn = true;
        m_lastDraw = now;
        return true;
    }

private:
    void open(uint32_t now) {
        m_open = true;
        m_drawn = false;
        m_openedAt = now;
    }

    bool m_open = false;
    bool m_ignoreRelease = false; // the hold that opened the page has not been let go yet
    bool m_drawn = false;
    uint32_t m_openedAt = 0;
    uint32_t m_lastDraw = 0;
};

// --- wording --------------------------------------------------------------------

// A span in its two largest units. Seconds stop showing after a minute so a line
// does not change every second for as long as the page is open.
inline std::string spanText(uint32_t seconds) {
    char buf[24];
    if (seconds < 60) {
        snprintf(buf, sizeof(buf), "%lu s", static_cast<unsigned long>(seconds));
    } else if (seconds < 3600) {
        snprintf(buf, sizeof(buf), "%lu min", static_cast<unsigned long>(seconds / 60));
    } else if (seconds < 86400) {
        snprintf(buf, sizeof(buf), "%lu h %02lu min", static_cast<unsigned long>(seconds / 3600),
                 static_cast<unsigned long>(seconds % 3600 / 60));
    } else {
        snprintf(buf, sizeof(buf), "%lu d %02lu h", static_cast<unsigned long>(seconds / 86400),
                 static_cast<unsigned long>(seconds % 86400 / 3600));
    }
    return buf;
}

// Signal strength as a word, with the number beside it on the orb. Below -65 dBm
// the orb starts looking for a nearer access point (RoamPolicy.h), so "good" ends
// there.
inline const char *signalWord(int rssiDbm) {
    if (rssiDbm >= -55) {
        return "strong";
    }
    if (rssiDbm >= -65) {
        return "good";
    }
    if (rssiDbm >= -75) {
        return "fair";
    }
    return "weak";
}

inline std::string memoryText(uint32_t freeBytes) {
    char buf[24];
    snprintf(buf, sizeof(buf), "memory %lu KB free", static_cast<unsigned long>(freeBytes / 1024));
    return buf;
}

// Why the orb last started, short enough for one line on a round panel. The
// device log keeps the longer wording.
enum class Start { PowerOn, ResetPin, Restart, Crash, Watchdog, DeepSleep, Brownout, Unknown };

inline const char *startWord(Start s) {
    switch (s) {
    case Start::PowerOn:
        return "power on";
    case Start::ResetPin:
        return "reset pin";
    case Start::Restart:
        return "restart";
    case Start::Crash:
        return "crash";
    case Start::Watchdog:
        return "watchdog";
    case Start::DeepSleep:
        return "deep sleep";
    case Start::Brownout:
        return "brownout";
    case Start::Unknown:
        break;
    }
    return "not known";
}

// --- the five orbs ----------------------------------------------------------------

enum class Kind {
    Title,   // the orb's heading
    Plain,   //
    Network, // a network name: any length, shrunk to fit
    Address  // something to type into a browser
};

struct Line {
    std::string text;
    Kind kind;
    bool operator==(const Line &o) const { return text == o.text && kind == o.kind; }
    bool operator!=(const Line &o) const { return !(*this == o); }
};
using Page = std::vector<Line>;

struct Mqtt {
    bool inUse = false;    // set up, and one of its pages is in the rotation
    bool connected = false;
    bool hasData = false;  // a value has arrived since the orb started
    uint32_t dataAgeS = 0; // how long ago the newest one arrived
};

// One data page in the rotation: "weather", "stocks", "web data"
struct Source {
    std::string name;
    bool hasData; // false: nothing fetched since power-on
    uint32_t ageS; // since the last good refresh
};

// Everything the page shows, gathered by the firmware once a second.
struct Facts {
    std::string version, commit, built; // built: "2026-11-13 21:05 UTC"
    bool wifiUp = false;
    std::string network;
    int rssi = 0;
    int channel = 0;
    std::string ip, localName;
    uint32_t upS = 0;
    Start start = Start::Unknown;
    uint32_t freeBytes = 0;
    std::vector<Source> sources;
    Mqtt mqtt[2];
};

// Orb 5 holds a title and four lines, and an orb can have more data pages
// than that (three weather pages, stocks, two web data pages, two brokers), so
// each kind of page gets one line, and the line tells the worst of its pages:
// one that has fetched nothing yet, or else the oldest.
inline std::vector<Source> byKind(const std::vector<Source> &sources) {
    std::vector<Source> kinds; // in the order the pages come in the rotation
    for (const Source &s : sources) {
        Source *kind = nullptr;
        for (Source &k : kinds) {
            if (k.name == s.name) {
                kind = &k;
            }
        }
        if (!kind) {
            kinds.push_back(s);
        } else if (!s.hasData) {
            kind->hasData = false;
        } else if (kind->hasData && s.ageS > kind->ageS) {
            kind->ageS = s.ageS;
        }
    }
    return kinds;
}

// The one MQTT line, empty when no broker is in use. A broker that is not
// connected comes first: the age of data that has stopped arriving says less
// than that. It is numbered only when the other broker is fine.
inline std::string mqttLine(const Mqtt m[2]) {
    int inUse = 0, down = 0, lastDown = 0;
    bool waiting = false;
    uint32_t oldest = 0;
    for (int i = 0; i < 2; i++) {
        if (!m[i].inUse) {
            continue;
        }
        inUse++;
        if (!m[i].connected) {
            down++;
            lastDown = i + 1;
        } else if (!m[i].hasData) {
            waiting = true;
        } else if (m[i].dataAgeS > oldest) {
            oldest = m[i].dataAgeS;
        }
    }
    if (inUse == 0) {
        return "";
    }
    if (down > 0) {
        return inUse == 2 && down == 1 ? "MQTT " + std::to_string(lastDown) + " not connected" : std::string("MQTT not connected");
    }
    return waiting ? std::string("MQTT no data yet") : "MQTT " + spanText(oldest) + " ago";
}

// The lines of one orb, 0 to 4 from the left: a title and up to four lines.
inline Page page(int orb, const Facts &f) {
    Page p;
    switch (orb) {
    case 0:
        p.push_back({"Firmware", Kind::Title});
        p.push_back({f.version, Kind::Plain});
        p.push_back({"commit " + f.commit, Kind::Plain});
        p.push_back({"built " + f.built.substr(0, f.built.find(' ')), Kind::Plain}); // the day is enough
        break;
    case 1:
        p.push_back({"WiFi", Kind::Title});
        if (!f.wifiUp) {
            p.push_back({"No WiFi", Kind::Plain});
            break;
        }
        p.push_back({f.network, Kind::Network});
        p.push_back({std::string(signalWord(f.rssi)) + ", " + std::to_string(f.rssi) + " dBm", Kind::Plain});
        p.push_back({"channel " + std::to_string(f.channel), Kind::Plain});
        break;
    case 2:
        p.push_back({"Address", Kind::Title});
        if (!f.wifiUp) {
            p.push_back({"none, no WiFi", Kind::Plain});
            break;
        }
        p.push_back({f.ip, Kind::Address});
        p.push_back({f.localName, Kind::Address});
        break;
    case 3:
        p.push_back({"Health", Kind::Title});
        p.push_back({"up " + spanText(f.upS), Kind::Plain});
        p.push_back({std::string("last start: ") + startWord(f.start), Kind::Plain});
        p.push_back({memoryText(f.freeBytes), Kind::Plain});
        break;
    default:
        p.push_back({"Data", Kind::Title});
        {
            const std::string mqtt = mqttLine(f.mqtt);
            const size_t room = mqtt.empty() ? 4 : 3; // MQTT keeps its line when the kinds run over
            for (const Source &k : byKind(f.sources)) {
                if (p.size() > room) {
                    break;
                }
                p.push_back({k.name + (k.hasData ? " " + spanText(k.ageS) + " ago" : std::string(" no data yet")), Kind::Plain});
            }
            if (!mqtt.empty()) {
                p.push_back({mqtt, Kind::Plain});
            }
            if (p.size() == 1) {
                p.push_back({"no data pages", Kind::Plain});
            }
        }
        break;
    }
    return p;
}

// --- drawing only what changed ------------------------------------------------------

// Which lines to draw again when an orb's lines went from `shown` to `now`. A
// whole-panel clear every second flickers, so a changed line is redrawn alone.
// A different number of lines moves every line: {kWholeOrb}.
inline std::vector<int> linesToRedraw(const Page &shown, const Page &now) {
    if (shown.size() != now.size()) {
        return {kWholeOrb};
    }
    std::vector<int> changed;
    for (size_t i = 0; i < now.size(); i++) {
        if (shown[i] != now[i]) {
            changed.push_back(static_cast<int>(i));
        }
    }
    return changed;
}

// --- where the lines go on a round 240 pixel panel -------------------------------------

constexpr int kPanel = 240;
constexpr int kLinePitch = 34;     // five lines span 136 pixels around the middle
constexpr int kTextHalfHeight = 10; // of the font the lines are drawn in
constexpr int kEdgeMargin = 8;      // kept clear at each end of a line

// The vertical middle of line `index` of `count`, the block centered on the panel.
inline int lineY(int count, int index) {
    return kPanel / 2 + (2 * index - (count - 1)) * (kLinePitch / 2);
}

// The widest a line can be at that height: the panel is round, so a line near the
// top or bottom has less room than one across the middle.
inline int lineWidthAt(int y) {
    const int r = kPanel / 2;
    const int dy = (y > r ? y - r : r - y) + kTextHalfHeight;
    if (dy >= r) {
        return 0;
    }
    const int w = static_cast<int>(2 * std::sqrt(static_cast<double>(r * r - dy * dy))) - 2 * kEdgeMargin;
    return w > 0 ? w : 0;
}

} // namespace infopage
