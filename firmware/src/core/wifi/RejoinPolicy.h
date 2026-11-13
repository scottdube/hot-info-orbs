#pragma once
// When the orb starts a WiFi join of its own after the link drops. std only,
// so it is covered by `pio test -e native` (test/test_rejoin).
//
// Before this the firmware left every rejoin to the ESP32 Arduino core, and
// the core (2.0.17, WiFiGeneric.cpp) only tries again after the disconnect
// reasons listed in coreRetries() below. Its retry is driven by the
// disconnect event itself, so the first answer outside that list ends it:
// no event, no further try, and the orb sat on "No WiFi" until it was
// restarted. Seen 2026-11-13 on a test rig: blocked at the access point and
// later unblocked, it never came back on its own. Which reason the access
// point's refusal produced was not measured at the time; the settings page
// now shows it after an outage.
//
// So the orb keeps its own slow clock: every kEveryMs the link stays down
// it starts a join by network name, whatever the core is doing.

#include <cstdint>

namespace rejoin {

// First try this long after the drop, then at the same interval. Long enough
// that the core's own retry and a roam (RoamPolicy.h, 20 s to settle) are
// over first. No back-off: one join costs a scan of a few seconds, and an
// access point that was off for hours should be found within half a minute
// of coming back.
constexpr uint32_t kEveryMs = 30000;

// What to do on this pass of the main loop
struct Step {
    bool join{false}; // start a join now
    bool ended{false}; // the link just came back; true for one pass
    uint32_t outMs{0}; // with ended: how long it was down
    uint16_t tries{0}; // with ended: joins this asked for while it was down
};

class Watch {
public:
    // Call on every pass with millis() and whether the link is up
    Step step(uint32_t now, bool up) {
        Step s;
        if (up) {
            if (m_down) {
                m_down = false;
                s.ended = true;
                s.outMs = now - m_downAt;
                s.tries = m_tries;
            }
            return s;
        }
        if (!m_down) {
            m_down = true;
            m_downAt = now;
            m_lastTry = now;
            m_tries = 0;
            return s;
        }
        // Unsigned difference: right across the millis() wrap at 49.7 days
        if (now - m_lastTry >= kEveryMs) {
            m_lastTry = now;
            if (m_tries < UINT16_MAX) {
                m_tries++;
            }
            s.join = true;
        }
        return s;
    }

    // While it is down, for the device log's "still down" lines
    bool down() const { return m_down; }
    uint32_t downMs(uint32_t now) const { return m_down ? now - m_downAt : 0; }
    uint16_t tries() const { return m_tries; }

private:
    bool m_down{false};
    uint32_t m_downAt{0};
    uint32_t m_lastTry{0};
    uint16_t m_tries{0};
};

// True for the disconnect reasons after which the core tries again by
// itself: _isReconnectableReason() in arduino-esp32 2.0.17. Any other
// reason, 202 AUTH_FAIL and 8 ASSOC_LEAVE among them, and it stops. Used
// only to say on the settings page which kind the last outage ended on.
inline bool coreRetries(uint8_t reason) {
    switch (reason) {
    case 1: // UNSPECIFIED
    case 2: // AUTH_EXPIRE
    case 3: // AUTH_LEAVE
    case 4: // ASSOC_EXPIRE
    case 5: // ASSOC_TOOMANY
    case 6: // NOT_AUTHED
    case 7: // NOT_ASSOCED
    case 9: // ASSOC_NOT_AUTHED
    case 14: // MIC_FAILURE
    case 15: // 4WAY_HANDSHAKE_TIMEOUT
    case 16: // GROUP_KEY_UPDATE_TIMEOUT
    case 17: // IE_IN_4WAY_DIFFERS
    case 23: // 802_1X_AUTH_FAILED
    case 49: // INVALID_PMKID
    case 200: // BEACON_TIMEOUT
    case 201: // NO_AP_FOUND
    case 203: // ASSOC_FAIL
    case 204: // HANDSHAKE_TIMEOUT
    case 205: // CONNECTION_FAIL
    case 206: // AP_TSF_RESET
    case 207: // ROAMING
        return true;
    default:
        return false;
    }
}

} // namespace rejoin
