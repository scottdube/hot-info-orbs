#pragma once
// When a page's data is stale, and how the orbs say so (issue #32). std only,
// so it is covered by `pio test -e native` (test/test_stale).
//
// Before this, an orb that lost WiFi after start-up kept showing the last
// weather or prices it had, for hours, with nothing to say they were old: a
// failed refresh kept the previous data and recorded nothing.
//
// The mark (picked by Scott from a sheet of four, 2026-11-12): a red ring
// around every orb that holds the old data, and no words in the ring.
//   broken ring, and "No WiFi" on orb 2 = the orb has no WiFi
//   solid ring                          = WiFi is up, the data is still old
//                                         (internet or the service is down)
// The two causes differ by shape and by the words, never by color alone. A
// clock orb is never marked: it keeps time without WiFi.
// Rejected on that sheet: a thin ring alone (says nothing about the cause),
// and the cause written around or across the bottom of every orb (the
// weather page uses the bottom 20 px of three orbs; the words covered them).
//
// The rule: stale = a refresh has just failed and the data is by then two
// refresh periods old, three minutes at the least.
// - Judged when a refresh fails, not by a timer. A timer fired a few seconds
//   BEFORE the second refresh ran (each try starts a little over one period
//   after the last), so the mark could flash up and clear again.
// - Two periods, not one: a roam or a router restart costs one refresh and
//   must not mark anything.
// - Age, not a count of misses: only the page on show refreshes. One that was
//   off screen for an hour is stale on its first miss.
// - The floor: a Web Data page may refresh every 10 s; two misses there is
//   less than a roam takes.

#include <cstdint>

namespace stale {

constexpr uint32_t kFloorMs = 180000;

// How old data may get before a failed refresh marks the page
inline uint32_t limitMs(uint32_t refreshMs) {
    const uint32_t two = 2 * refreshMs;
    return two > kFloorMs ? two : kFloorMs;
}

// One page's data. Call attempt() after every refresh, good or bad. A feed
// with no refresh time (an MQTT connection) calls it on every pass with
// refreshMs 0: stale once it has been down for the floor.
class Tracker {
public:
    void attempt(uint32_t now, bool ok, uint32_t refreshMs) {
        if (ok || !m_started) {
            // No data since power-on: the clock starts at the first try
            m_started = true;
            m_since = now;
        }
        m_stale = !ok && (now - m_since) >= limitMs(refreshMs); // unsigned: right across the millis() wrap
    }
    bool stale() const { return m_stale; }

private:
    bool m_started = false;
    bool m_stale = false;
    uint32_t m_since = 0; // the last good refresh, or the first try
};

enum class Mark : uint8_t {
    None,
    NoWifi, // broken ring, "No WiFi" on orb 2
    Old, // solid ring
};

inline Mark mark(bool pageStale, bool linkUp) {
    if (!pageStale) {
        return Mark::None;
    }
    return linkUp ? Mark::Old : Mark::NoWifi;
}

// The ring, panel px from the orb's center. 7 px: the 4 px ring on the sheet
// read as a bezel. The stock page's own up/down ring is 2 px, at 118..120.
constexpr int kRingOuter = 120, kRingInner = 113;

// The broken ring: 12 dashes of 16 degrees with 14 degree gaps, in TFT_eSPI's
// degrees (0 at 6 o'clock, clockwise). The gaps sit on 6, 9, 12 and 3 o'clock
// so the left and right halves match and no dash crosses the 0/360 seam.
constexpr int kDashes = 12;
struct Arc {
    int start, end;
};
inline Arc dash(int i) { return {30 * i + 7, 30 * i + 23}; }

// "No WiFi" on orb 2: black words on a red plate near the top, like a lit
// annunciator. A plate, not bare red words, because the page under it may be
// light or dark and may have its own text there (the stock page does).
constexpr int kPlateOrb = 1; // screen index
constexpr int kPlateH = 28, kPlateRadius = 6;
constexpr int kPlateTextSize = 20;
// Where the plate goes: top edge and width, centered left to right. The
// default is clear of the weather and MQTT pages' own text. A page with
// something there says where instead (Widget::stalePlate): half-covered
// text reads as a fault in the orb, a fully covered line does not.
struct Plate {
    int y, w;
};
constexpr Plate kPlate{20, 96};

// True once, on the pass where WiFi is back after being down: the pages then
// refresh at once instead of at their next 10 or 15 minute turn.
class LinkWatch {
public:
    bool cameBack(bool up) {
        const bool back = up && !m_up;
        m_up = up;
        return back;
    }

private:
    bool m_up = true; // pages only run once start-up has joined WiFi
};

// What to draw on this pass. The mark sits on top of a page that keeps
// repainting itself, so it has to go back on every orb the page wipes, and
// it can only come OFF by having the page repaint.
struct Paint {
    bool repaint; // wipe all five orbs and redraw the page first
    uint8_t orbs; // then put the mark on these, bit 0 = orb 1
};
class Overlay {
public:
    // wiped: the orbs the page has filled since the last call.
    // dataOrbs: the orbs holding the page's data (they get the ring).
    Paint step(uint8_t wiped, Mark want, uint8_t dataOrbs) {
        m_on &= ~wiped; // a wipe takes the mark with it
        uint8_t orbs = 0;
        if (want != Mark::None) {
            orbs = dataOrbs & 0x1F;
            if (want == Mark::NoWifi) {
                orbs |= 1u << kPlateOrb; // the words, whether or not orb 2 holds data
            }
        }
        // Old paint that is the wrong shape, or on an orb that should have
        // none, is still on the glass. Going from no mark to a mark never
        // gets here: it goes on over the page with no flicker.
        const bool repaint = m_on && (want != m_shown || (m_on & ~orbs));
        if (repaint) {
            m_on = 0;
        }
        m_shown = want;
        const uint8_t draw = orbs & ~m_on;
        m_on |= draw;
        return {repaint, draw};
    }

private:
    Mark m_shown = Mark::None;
    uint8_t m_on = 0; // the orbs carrying the mark right now
};

} // namespace stale
