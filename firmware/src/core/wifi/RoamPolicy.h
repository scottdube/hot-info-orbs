#pragma once
// Which access point to move to. std only, so it is covered by
// `pio test -e native` (test/test_roam).
//
// The ESP32 joins the first AP it hears with the right network name and stays
// on it until the link dies completely, so an orb moved across a building
// keeps talking to a far AP. Measured 2026-09-25: 341 ms average ping, 5% loss,
// and every 96 KB Tempest reply cut short, while the router answered in 3 ms.

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

struct ApSeen {
    std::string ssid;
    uint8_t bssid[6];
    int rssi; // dBm
    int channel;
};

// Only look for a better AP when the signal is weaker than this
constexpr int ROAM_SCAN_BELOW_DBM = -65;
// A candidate must beat the current AP by this much, so two APs at similar
// strength don't trade the orb back and forth
constexpr int ROAM_MARGIN_DB = 8;

// Index into seen of the AP to move to, or -1 to stay
inline int pickRoamTarget(const std::string &ssid, const uint8_t current[6], int currentRssi,
                          const std::vector<ApSeen> &seen, int margin = ROAM_MARGIN_DB) {
    int best = -1;
    for (int i = 0; i < (int)seen.size(); i++) {
        const ApSeen &ap = seen[i];
        if (ap.ssid != ssid || memcmp(ap.bssid, current, 6) == 0) {
            continue;
        }
        if (ap.rssi >= currentRssi + margin && (best < 0 || ap.rssi > seen[best].rssi)) {
            best = i;
        }
    }
    return best;
}
