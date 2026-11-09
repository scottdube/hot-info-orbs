#pragma once
// Time zones without a web service (2026-11-09). The C library applies a POSIX
// TZ rule string itself, daylight saving included, so the orb needs no
// TimeZoneDB key and no network call to know its offset. The settings page
// offers the names below; the firmware turns the chosen name into its rule.
//
// std-only, so `pio test -e native` covers it (test/test_validation).
//
// A rule reads like "EST5EDT,M3.2.0,M11.1.0": standard name, hours WEST of
// UTC, summer name, then start/end as month.week.weekday[/hour] (week 5 = the
// last one; hour defaults to 02:00 local). The names inside the rule are only
// labels; nothing here shows them.
//
// Rejected: the full IANA list (~460 zones, ~20 KB of rules as page options).
// The club is in Florida; this list covers the US and Canada in full and the
// places members are likely to have family. A missing zone is one line here.
// Rejected: <+08>-style quoted names (fine on the host, unverified on the
// ESP32's newlib), so zones without a letter abbreviation get a made-up one.
//
// The table goes stale only when a government changes its rules (the US last
// did in 2007, Mexico dropped DST in 2022); a firmware update carries the fix.

#include <cctype>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <string>

namespace tzdb {

struct Zone {
    const char *name; // IANA name, what the settings page stores (key "tz")
    const char *rule; // POSIX TZ string
};

// Order = order on the settings page
static const Zone ZONES[] = {
    {"America/New_York", "EST5EDT,M3.2.0,M11.1.0"},
    {"America/Detroit", "EST5EDT,M3.2.0,M11.1.0"},
    {"America/Indiana/Indianapolis", "EST5EDT,M3.2.0,M11.1.0"},
    {"America/Kentucky/Louisville", "EST5EDT,M3.2.0,M11.1.0"},
    {"America/Toronto", "EST5EDT,M3.2.0,M11.1.0"},
    {"America/Chicago", "CST6CDT,M3.2.0,M11.1.0"},
    {"America/Winnipeg", "CST6CDT,M3.2.0,M11.1.0"},
    {"America/Regina", "CST6"},
    {"America/Denver", "MST7MDT,M3.2.0,M11.1.0"},
    {"America/Boise", "MST7MDT,M3.2.0,M11.1.0"},
    {"America/Edmonton", "MST7MDT,M3.2.0,M11.1.0"},
    {"America/Phoenix", "MST7"},
    {"America/Los_Angeles", "PST8PDT,M3.2.0,M11.1.0"},
    {"America/Vancouver", "PST8PDT,M3.2.0,M11.1.0"},
    {"America/Anchorage", "AKST9AKDT,M3.2.0,M11.1.0"},
    {"Pacific/Honolulu", "HST10"},
    {"America/Halifax", "AST4ADT,M3.2.0,M11.1.0"},
    {"America/St_Johns", "NST3:30NDT,M3.2.0,M11.1.0"},
    {"America/Puerto_Rico", "AST4"},
    {"America/Mexico_City", "CST6"},
    {"America/Bogota", "COT5"},
    {"America/Sao_Paulo", "BRT3"},
    {"America/Argentina/Buenos_Aires", "ART3"},
    {"Europe/London", "GMT0BST,M3.5.0/1,M10.5.0"},
    {"Europe/Dublin", "GMT0IST,M3.5.0/1,M10.5.0"},
    {"Europe/Lisbon", "WET0WEST,M3.5.0/1,M10.5.0"},
    {"Europe/Paris", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Berlin", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Amsterdam", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Brussels", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Zurich", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Rome", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Madrid", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Vienna", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Stockholm", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Oslo", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Copenhagen", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Warsaw", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Prague", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Athens", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Europe/Helsinki", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Europe/Kyiv", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Europe/Moscow", "MSK-3"},
    {"Africa/Johannesburg", "SAST-2"},
    {"Asia/Dubai", "GST-4"},
    {"Asia/Kolkata", "IST-5:30"},
    {"Asia/Singapore", "SGT-8"},
    {"Asia/Hong_Kong", "HKT-8"},
    {"Asia/Shanghai", "CST-8"},
    {"Asia/Taipei", "CST-8"},
    {"Asia/Manila", "PST-8"},
    {"Asia/Seoul", "KST-9"},
    {"Asia/Tokyo", "JST-9"},
    {"Australia/Perth", "AWST-8"},
    {"Australia/Adelaide", "ACST-9:30ACDT,M10.1.0,M4.1.0/3"},
    {"Australia/Brisbane", "AEST-10"},
    {"Australia/Sydney", "AEST-10AEDT,M10.1.0,M4.1.0/3"},
    {"Australia/Melbourne", "AEST-10AEDT,M10.1.0,M4.1.0/3"},
    {"Pacific/Auckland", "NZST-12NZDT,M9.5.0,M4.1.0/3"},
    {"UTC", "UTC0"},
};
static const int ZONE_COUNT = sizeof(ZONES) / sizeof(ZONES[0]);

// Accepts the case and spacing people type (an old saved "america/new york"
// would otherwise be lost); returns nullptr when the name is not in the table.
inline const Zone *find(const std::string &name) {
    std::string want;
    for (char c : name) {
        if (c == ' ') {
            c = '_';
        }
        want += (char)std::tolower((unsigned char)c);
    }
    for (int i = 0; i < ZONE_COUNT; i++) {
        std::string have;
        for (const char *p = ZONES[i].name; *p; p++) {
            have += (char)std::tolower((unsigned char)*p);
        }
        if (have == want) {
            return &ZONES[i];
        }
    }
    return nullptr;
}

// The rule for a name; UTC when the name is unknown, so the clock still runs.
inline const char *ruleFor(const std::string &name) {
    const Zone *z = find(name);
    return z ? z->rule : "UTC0";
}

// Days since 1970-01-01 for a civil date (Howard Hinnant's algorithm)
inline int64_t daysFromCivil(int64_t y, unsigned m, unsigned d) {
    y -= m <= 2;
    const int64_t era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (int64_t)doe - 719468;
}

// Seconds EAST of UTC at the instant `utc`, under the TZ the process has
// already set (setenv("TZ", rule) + tzset()). newlib's struct tm has no
// tm_gmtoff, so the offset is the local wall clock read back as if it were UTC,
// minus UTC.
inline long offsetAt(time_t utc) {
    struct tm lt;
    localtime_r(&utc, &lt);
    const int64_t wall = daysFromCivil(lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday) * 86400 + lt.tm_hour * 3600 +
                         lt.tm_min * 60 + lt.tm_sec;
    return (long)(wall - (int64_t)utc);
}

} // namespace tzdb
