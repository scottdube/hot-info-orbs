#pragma once
// What the orbs say when WiFi doesn't connect at start-up (issue #20). std
// only, so it is covered by `pio test -e native` (test/test_wififail).
//
// Before this, every failure showed the first-setup steps, so an orb whose
// router was off, or whose password had changed, looked as if it had never
// been set up. WiFiManager records why the saved network failed; this turns
// that into one of four cases and the text for orb 1 and orb 5.

#include <cstdint>
#include <string>
#include <vector>

namespace wififail {

// wl_status_t values (ESP32 WiFiType.h), plus WiFiManager's own 7, which it
// sets from the AUTH_FAIL / AUTH_EXPIRE disconnect reason
namespace wl {
constexpr int NoSsidAvail = 1;
constexpr int ConnectFailed = 4;
constexpr int ConnectionLost = 5;
constexpr int Disconnected = 6;
constexpr int WrongPassword = 7;
} // namespace wl

enum class Case {
    NeverSetUp,    // no saved network: the first-setup steps
    NotFound,      // saved network not heard (router off, out of range)
    WrongPassword, // heard, refused the password
    Other,         // heard, failed some other way
};

inline Case classify(bool hasSavedNetwork, int lastResult) {
    if (!hasSavedNetwork) {
        return Case::NeverSetUp;
    }
    if (lastResult == wl::NoSsidAvail) {
        return Case::NotFound;
    }
    if (lastResult == wl::WrongPassword) {
        return Case::WrongPassword;
    }
    return Case::Other;
}

// Station disconnect reasons: ESP-IDF's wifi_err_reason_t (esp_wifi_types.h),
// fixed by the SDK, not by the router. 1..66 are IEEE 802.11 reason codes,
// 200+ are Espressif's. Copied here so the native tests build without the SDK;
// WifiWidget.cpp static_asserts each one against the SDK's.
namespace reason {
constexpr int AuthExpire = 2;             // WIFI_REASON_AUTH_EXPIRE
constexpr int AssocLeave = 8;             // WIFI_REASON_ASSOC_LEAVE: our own disconnect()
constexpr int FourWayHandshakeTimeout = 15; // WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT
constexpr int NoApFound = 201;            // WIFI_REASON_NO_AP_FOUND
constexpr int AuthFail = 202;             // WIFI_REASON_AUTH_FAIL
constexpr int HandshakeTimeout = 204;     // WIFI_REASON_HANDSHAKE_TIMEOUT
} // namespace reason

// The network heard us and refused the password. WiFiManager maps only
// AuthExpire and AuthFail to "wrong password". On WPA2-PSK a wrong password
// shows up as FourWayHandshakeTimeout instead: the AP checks the password in
// the handshake and simply stops answering, on any brand of router. Measured
// 2026-11-11 on the Legacy IoT network: 15 about every 3 s, status stays
// DISCONNECTED, so WiFiManager called it "other" and retried for 5 minutes.
// A weak signal can also time the handshake out, so the caller wants it twice.
inline bool passwordRefused(int r) {
    return r == reason::AuthExpire || r == reason::FourWayHandshakeTimeout || r == reason::AuthFail ||
           r == reason::HandshakeTimeout;
}
constexpr int kRefusalsForWrongPassword = 2;

// Keep scanning for the saved network while the setup portal runs, and
// restart when it is heard. Only when it was not heard: retrying a refused
// password, or a fault we can't name, would restart the orb forever.
inline bool keepsLooking(Case c) { return c == Case::NotFound; }

// The setup portal for a network that was heard but did not join (Case::Other)
// used to wait for a person: WiFiManager turns the station off and never
// tries again. An orb that starts faster than its router after a power cut
// can land there, and so does the restart in RejoinPolicy.h's ladder. So the
// orb restarts to try the join again, after 5, 10, 20, then every 30 minutes,
// with no end (Scott, 2026-11-13), and never while a phone is on the portal.
// REJECTED: stop after three tries. An orb that gives up at 35 minutes needs
// a power cycle again, which is what this removes. The price is an orb the
// network really refuses restarting every half hour until someone fixes that.
//
// A refused password is retried the same way, but only when this name and
// password have joined before. Rig, 2026-11-13: a device blocked at the access
// point is refused with reason 202 on every try, the same as a wrong
// password, so after the ladder's restart the orb opened the "wrong password"
// screen and stayed there after the block was lifted, until a power cycle.
// The orb cannot tell the two apart; what it can know is that a password
// which worked yesterday was not mistyped. A password that never joined
// (freshly typed on the setup page) still stays put: the same password gets
// the same answer. REJECTED: retry every refused password, which restarts an
// orb with a typo every half hour for good. The price of this rule: after a
// real password change at the router the orb restarts every half hour on the
// setup screen until the new one is entered there.
//
// The mark kept in NVS when a join succeeds: FNV-1a over the name, a zero
// byte and the password. Not a secret store and not meant as one: the
// password itself is in NVS already, put there by the WiFi stack.
constexpr uint32_t kNoMark = 0; // nothing stored yet
inline uint32_t credentialsMark(const char *ssid, const char *pass) {
    uint32_t h = 2166136261u;
    auto mix = [&h](unsigned char c) {
        h ^= c;
        h *= 16777619u;
    };
    for (const char *p = ssid; *p; p++) {
        mix(static_cast<unsigned char>(*p));
    }
    mix(0); // where the name ends counts: "ab" + "c" is not "a" + "bc"
    for (const char *p = pass; *p; p++) {
        mix(static_cast<unsigned char>(*p));
    }
    return h == kNoMark ? 1 : h;
}
inline bool joinedBefore(uint32_t storedMark, const char *ssid, const char *pass) {
    return storedMark != kNoMark && storedMark == credentialsMark(ssid, pass);
}
inline bool retriesByRestart(Case c, bool joinedBefore) {
    return c == Case::Other || (c == Case::WrongPassword && joinedBefore);
}

inline uint32_t retryAfterMs(uint32_t retriesDone) {
    constexpr uint32_t minute = 60000;
    switch (retriesDone) {
    case 0:
        return 5 * minute;
    case 1:
        return 10 * minute;
    case 2:
        return 20 * minute;
    default:
        return 30 * minute;
    }
}

// How many of those restarts have happened. Kept in memory that a restart
// leaves alone and a power cycle scrambles (RTC_NOINIT on the ESP32), so a
// power cycle starts from 5 minutes again; cleared when the orb joins. The
// check word tells a real count from whatever was there at power-on.
// REJECTED: flash (Preferences). A write every half hour for as long as the
// fault lasts, and a count that a power cycle would not clear.
struct RetryMemory {
    uint32_t count;
    uint32_t check;
};
constexpr uint32_t kRetryCheck = 0x52455452; // "RETR"

inline uint32_t retriesDone(const RetryMemory &m) { return m.check == (m.count ^ kRetryCheck) ? m.count : 0; }

inline void noteRetry(RetryMemory &m) {
    m.count = retriesDone(m) + 1;
    m.check = m.count ^ kRetryCheck;
}

inline void clearRetries(RetryMemory &m) {
    m.count = 0;
    m.check = kRetryCheck;
}

enum class Kind {
    Plain,   // white
    Network, // a WiFi network name: sky blue, shrunk to fit
    Address, // 192.168.4.1: green-yellow
};

struct Line {
    std::string text;
    Kind kind;
};

struct Screens {
    bool cross;                 // draw an X above the headline (a glyph, not color alone)
    std::vector<Line> headline; // what went wrong, centered
    std::vector<Line> steps;    // what to do, centered, top to bottom
};

constexpr const char *kPortalAddress = "192.168.4.1";

inline Screens screens(Case c, const std::string &savedSsid, const std::string &apSsid) {
    const Line join{"join", Kind::Plain};
    const Line ap{apSsid, Kind::Network};
    const Line open{"and open", Kind::Plain};
    const Line address{kPortalAddress, Kind::Address};
    const Line saved{savedSsid, Kind::Network};
    switch (c) {
    case Case::NeverSetUp:
        return {false,
                {{"Set up WiFi", Kind::Plain}},
                {{"Connect", Kind::Plain},
                 {"phone or PC", Kind::Plain},
                 {"to WiFi network:", Kind::Plain},
                 ap,
                 address}};
    case Case::NotFound:
        return {true,
                {{"WiFi", Kind::Plain}, {"not found", Kind::Plain}, saved},
                {{"To change, join", Kind::Plain}, ap, open, address}}; // Scott dropped "Will retry." 2026-11-11
    case Case::WrongPassword:
        return {true,
                {{"Wrong password", Kind::Plain}, {"for WiFi", Kind::Plain}, saved},
                {{"To fix it, join", Kind::Plain}, ap, open, address}};
    case Case::Other:
    default:
        return {true,
                {{"WiFi didn't", Kind::Plain}, {"connect", Kind::Plain}, saved},
                {{"To check it,", Kind::Plain}, join, ap, open, address}};
    }
}

} // namespace wififail
