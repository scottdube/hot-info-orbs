#pragma once
// What the orbs say when WiFi doesn't connect at start-up (issue #20). std
// only, so it is covered by `pio test -e native` (test/test_wififail).
//
// Before this, every failure showed the first-setup steps, so an orb whose
// router was off, or whose password had changed, looked as if it had never
// been set up. WiFiManager records why the saved network failed; this turns
// that into one of four cases and the text for orb 1 and orb 5.

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
                {{"Will retry.", Kind::Plain}, {"To change it,", Kind::Plain}, ap, open, address}};
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
