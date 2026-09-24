#pragma once
// Sunrise/sunset text and moon phase for the weather icon orb. std only, so
// it is covered by `pio test -e native` (test/test_sunmoon).

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>

// Moon age in days since the last new moon, 0 .. 29.53. Mean lunation from a
// known new moon (2000-01-06 18:14 UTC); the real moon runs up to about half
// a day either side of the mean, which moves a name change by hours at most.
inline double moonAgeDays(int64_t utcEpoch) {
    const double synodic = 29.530588853;
    const double newMoon2000 = 947182440.0; // 2000-01-06 18:14 UTC
    double age = std::fmod((double)(utcEpoch - (int64_t)newMoon2000) / 86400.0, synodic);
    return age < 0 ? age + synodic : age;
}

// The eight traditional names, each centred on its point in the cycle
inline const char *moonPhaseName(double ageDays) {
    static const char *names[] = {"New moon", "Waxing crescent", "First quarter", "Waxing gibbous",
                                  "Full moon", "Waning gibbous", "Last quarter", "Waning crescent"};
    int i = (int)std::floor(ageDays / 29.530588853 * 8.0 + 0.5) % 8;
    return names[i];
}

// "6:42" / "18:51" from an epoch and the zone offset in seconds. 12-hour
// drops AM/PM: a sunrise is morning and a sunset evening. "" when unknown.
inline std::string sunClock(int64_t utcEpoch, long offsetSeconds, bool h24) {
    if (utcEpoch <= 0) {
        return "";
    }
    int64_t local = utcEpoch + offsetSeconds;
    int minutes = (int)(((local % 86400) + 86400) % 86400 / 60);
    int h = minutes / 60;
    if (!h24) {
        h = h % 12 == 0 ? 12 : h % 12;
    }
    char buf[8];
    snprintf(buf, sizeof(buf), "%d:%02d", h, minutes % 60);
    return buf;
}
