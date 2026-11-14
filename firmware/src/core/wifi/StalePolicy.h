#pragma once
// When a page's data is stale, and how the orbs say so (issue #32). std only,
// so it is covered by `pio test -e native` (test/test_stale).
//
// Before this, an orb that lost WiFi after start-up kept showing the last
// weather or prices it had, for hours, with nothing to say they were old: a
// failed refresh kept the previous data and recorded nothing.
//
// The mark (picked by Scott from a sheet of four, 2026-11-12): a ring around
// every orb that holds the old data, and no words in the ring.
//   "No WiFi" on orb 2, no ring yet     = WiFi is out, the data is still good
//   broken ring, and "No WiFi" on orb 2 = WiFi is out and the data is old
//   solid ring                          = WiFi is up, the data is still old
//                                         (internet or the service is down)
// The two causes differ by shape and by the words, never by color alone.
//
// Amber, not red, since 2026-11-14 (Scott): red is kept for the one mark
// below, so red is only ever on the clock and amber only on data pages.
//
// "Clock only when WiFi is out" (a setting, off by default; Yang asked for
// it, 2026-11-14): once WiFi counts as out, the orbs go to the clock, with a
// broken red ring on its orbs and "No WiFi" in red on the center orb.
// The page that was up comes back with the WiFi. See ClockOnly below.
// Without that setting a clock orb is never marked: it keeps time without
// WiFi.
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

// Weather and stocks ring at the same age (Scott, 2026-11-13). Weather
// refreshes every 10 min and gets here by the rule above; stocks refresh
// every 15, where two misses came to 30.
constexpr uint32_t kFetchLimitMs = 1200000;

// How long a page waits before its next refresh, for one that rings at
// `limit` whatever its refresh time. Its own time as a rule. After the first
// miss, only as long as it takes for the second to land on the limit: stocks
// try again after 5 min, not 15. One early try per outage and then the
// normal wait again.
// Rejected: refreshing stocks every 10 min like weather. The price service's
// key is shared by every Info Orbs builder, and that is half as many calls
// again from every orb, all day, to move a mark that shows on a bad day.
inline uint32_t nextWaitMs(uint32_t refreshMs, uint32_t limit, uint32_t misses) {
    if (misses == 1 && limit > refreshMs && limit - refreshMs < refreshMs) {
        return limit - refreshMs;
    }
    return refreshMs;
}

// When a page last refreshed well. any = false: not since power-on.
// No default values in it: the ESP32 compiler is C++11, where they would stop
// {true, now} from compiling.
struct LastGood {
    bool any;
    uint32_t at;
};

// One page's data. Call attempt() after every refresh, good or bad. A feed
// with no refresh time (an MQTT connection) calls it on every pass with
// refreshMs 0: stale once it has been down for the floor.
class Tracker {
public:
    void attempt(uint32_t now, bool ok, uint32_t refreshMs) { attemptWithin(now, ok, limitMs(refreshMs)); }
    // The same with the limit given, for a page that does not ring at two of
    // its own refreshes (stocks: kFetchLimitMs)
    void attemptWithin(uint32_t now, bool ok, uint32_t limit) {
        if (ok || !m_started) {
            // No data since power-on: the clock starts at the first try
            m_started = true;
            m_since = now;
        }
        if (ok) {
            m_good = {true, now};
        }
        m_stale = !ok && (now - m_since) >= limit; // unsigned: right across the millis() wrap
    }
    bool stale() const { return m_stale; }
    // For the info page: m_since cannot say this, it is also the first failed try
    LastGood lastGood() const { return m_good; }

private:
    bool m_started = false;
    bool m_stale = false;
    uint32_t m_since = 0; // the last good refresh, or the first try
    LastGood m_good{false, 0};
};

enum class Mark : uint8_t {
    None,
    LinkDown, // "No WiFi" on orb 2, no ring: WiFi is out, the data is still good
    NoWifi, // broken ring, "No WiFi" on orb 2: WiFi is out and the data is old
    Old, // solid ring: WiFi is up, the data is old
    ClockOnly, // broken red ring on the clock, "No WiFi" on its center orb: see ClockOnly below
};

// wifiOut: see LinkGrace. fetches: the page shows data it gets over WiFi.
// The words go up as soon as WiFi is out (Scott on the rig, 2026-11-12: "it
// should know right away"); the first build held them back with the ring
// until the data was old, three minutes or more. The ring still waits:
// weather fetched two minutes ago is good weather, and a ring on it would
// stop meaning "this is old".
inline Mark mark(bool pageStale, bool wifiOut, bool fetches) {
    if (!fetches) {
        return Mark::None;
    }
    if (wifiOut) {
        return pageStale ? Mark::NoWifi : Mark::LinkDown;
    }
    return pageStale ? Mark::Old : Mark::None;
}

inline bool hasRing(Mark m) { return m == Mark::NoWifi || m == Mark::Old || m == Mark::ClockOnly; }
inline bool hasWords(Mark m) { return m == Mark::NoWifi || m == Mark::LinkDown || m == Mark::ClockOnly; }
// Broken = this orb has no WiFi. Solid = WiFi is up and the service is not
// answering.
inline bool broken(Mark m) { return m == Mark::NoWifi || m == Mark::ClockOnly; }

// RGB565. Amber and not yellow: a light page (weather by day) is near white,
// where yellow all but disappears. Darker than signal amber for the same
// reason. Not checked on the panels in daylight yet.
// Not TFT_RED (0xF800): on these panels pure red read as orange beside the
// amber. A little blue pulls it away from orange (255, 0, 49).
constexpr uint16_t kRed = 0xF806;
constexpr uint16_t kAmber = 0xF4A0; // 240, 150, 0
inline uint16_t ringColor(Mark m) { return m == Mark::ClockOnly ? kRed : kAmber; }

// The "Clock only when WiFi is out" setting. Call step() on every pass that
// a page is on show; it answers with the page to switch to, or kStay.
// - In: once, when WiFi counts as out (LinkGrace, the same 10 s as "No
//   WiFi"). Not on every pass: a button still moves to another page, with
//   that page's usual marks. The rotation turn brings the clock back
//   (WidgetSet::rotate), so with rotation off a page picked by button stays.
// - Out: back to the page that was up, unless a button has moved off the
//   clock since; whoever did that is reading the page they picked.
// - The clock page shows even when its Show box is unticked.
// Rejected: making this the only behavior. Scott prefers the page staying
// up with its marks, so it is a setting and off unless switched on, and no
// orb changes what it does on an update.
constexpr int8_t kStay = -1;
class ClockOnly {
public:
    // on: the setting. clock: the clock page's number, -1 if there is none.
    int8_t step(bool on, bool wifiOut, int8_t current, int8_t clock) {
        const bool want = on && wifiOut && clock >= 0;
        if (want == m_active) {
            return kStay;
        }
        m_active = want;
        if (want) {
            m_clock = clock;
            m_returnTo = current;
            return current == clock ? kStay : clock;
        }
        return (current == m_clock && m_returnTo != m_clock) ? m_returnTo : kStay;
    }
    bool active() const { return m_active; }
    // What goes on the page on show: the clock's own ring, or whatever mark
    // the page would have anyway
    Mark mark(int8_t current, Mark page) const { return (m_active && current == m_clock) ? Mark::ClockOnly : page; }

private:
    bool m_active = false;
    int8_t m_clock = -1;
    int8_t m_returnTo = -1;
};

// WiFi counts as out once it has been down this long. Not at once: a roam
// between access points drops the link for a second or two, and the words
// would flash up on every one.
constexpr uint32_t kLinkGraceMs = 10000;
class LinkGrace {
public:
    bool out(uint32_t now, bool up) {
        if (up) {
            m_down = false;
            return false;
        }
        if (!m_down) {
            m_down = true;
            m_since = now;
        }
        return (now - m_since) >= kLinkGraceMs; // unsigned: right across the millis() wrap
    }

private:
    bool m_down = false;
    uint32_t m_since = 0;
};

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

// "No WiFi" on orb 2: black words on an amber plate near the top, like a lit
// annunciator (red until 2026-11-14, see the top of this file). A plate, not bare red words, because the page under it may be
// light or dark and may have its own text there (the stock page does).
constexpr int kPlateOrb = 1; // screen index
constexpr int kPlateH = 28, kPlateRadius = 6;
constexpr int kPlateTextSize = 20;
// On the clock the words go on the center orb: orb 2 holds a digit there,
// and the colon orb has room above the colon. The plate takes its ring's
// color, so red stays on the clock and amber on the data pages.
constexpr int kClockPlateOrb = 2; // screen index
inline int plateOrb(Mark m) { return m == Mark::ClockOnly ? kClockPlateOrb : kPlateOrb; }
inline uint16_t plateColor(Mark m) { return ringColor(m); }
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
    // dataOrbs: the orbs holding the page's data (they get the ring). For
    // the clock's ring: the clock's orbs (Widget::clockOnlyOrbs).
    Paint step(uint8_t wiped, Mark want, uint8_t dataOrbs) {
        m_on &= ~wiped; // a wipe takes the mark with it
        uint8_t orbs = 0;
        if (hasRing(want)) {
            orbs = dataOrbs & 0x1F;
        }
        if (hasWords(want)) {
            orbs |= 1u << plateOrb(want); // whether or not that orb holds data
        }
        // The rings joining the words (WiFi out, and now the data is old too)
        // go on over what is there. Orb 2 is painted again: it needs a ring.
        const bool adds = m_shown == Mark::LinkDown && want == Mark::NoWifi;
        // Otherwise old paint that is the wrong shape, or on an orb that
        // should have none, is still on the glass. Going from no mark to a
        // mark never gets here: it goes on over the page with no flicker.
        const bool repaint = m_on && !adds && (want != m_shown || (m_on & ~orbs));
        if (repaint || adds) {
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
