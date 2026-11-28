#pragma once
// The Villages entertainment feed: auth token scan, event parsing and time
// formatting. std + ArduinoJson only, so it is covered by `pio test -e native`
// (test/test_villages). Flow ported from netnutmike/python-villages-events:
// the public API wants a Basic token that sits in a CDN script (dp_AUTH_TOKEN),
// and one call with locationCategories=town-squares returns all five venues.

#include <ArduinoJson.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#define VILLAGES_VENUES 5
#define VILLAGES_MAX_ACTS 3 // acts shown per orb

static const char *const VILLAGES_TOKEN_URL = "https://cdn.thevillages.com/web_components/myvillages-auth-forms/main.js";
static const char *const VILLAGES_EVENTS_URL = "https://api.v2.thevillages.com/events/?cancelled=false&startRow=0&endRow=49"
                                               "&dateRange=today&categories=entertainment&locationCategories=town-squares&subcategoriesQueryType=and";

// Orb order. `match` is found in the API's location.title.
struct VillagesVenue {
    const char *match;
    const char *name;
};
static const VillagesVenue VILLAGES_VENUE_LIST[VILLAGES_VENUES] = {
    {"Spanish Springs", "Spanish Springs"},
    {"Lake Sumter", "Lake Sumter"},
    {"Brownwood", "Brownwood"},
    {"Sawgrass", "Sawgrass"},
    {"Eastport", "Eastport"},
};

struct VillagesAct {
    std::string title;
    int64_t start = 0; // UTC epoch, 0 when unreadable
    int64_t end = 0;
};

// Finds  dp_AUTH_TOKEN = "Basic <token>"  in a byte stream, one byte at a time,
// so the 430 KB script is never held in memory. A use of the name that is not
// an assignment (dp_AUTH_TOKEN,"Content-Type") starts the search over.
class VillagesTokenScanner {
public:
    // true once the token is complete; token() then holds "Basic <token>"
    bool feed(char c) {
        static const char name[] = "dp_AUTH_TOKEN";
        static const char basic[] = "Basic";
        switch (m_state) {
        case Name:
            if (c == name[m_pos]) {
                if (name[++m_pos] == '\0') {
                    m_state = Equals;
                }
            } else {
                m_pos = (c == name[0]) ? 1 : 0;
            }
            break;
        case Equals:
            if (c == '=') {
                m_state = Quote;
            } else if (!isSpace(c)) {
                restart(c);
            }
            break;
        case Quote:
            if (c == '"' || c == '\'') {
                m_quote = c;
                m_pos = 0;
                m_state = Word;
            } else if (!isSpace(c)) {
                restart(c);
            }
            break;
        case Word:
            if (c == basic[m_pos]) {
                if (basic[++m_pos] == '\0') {
                    m_state = Gap;
                }
            } else {
                restart(c);
            }
            break;
        case Gap:
            if (isSpace(c)) {
                m_seenGap = true;
            } else if (m_seenGap && isTokenChar(c)) {
                m_token = c;
                m_state = Token;
            } else {
                restart(c);
            }
            break;
        case Token:
            if (isTokenChar(c) && m_token.size() < 200) {
                m_token += c;
            } else if (c == m_quote) {
                m_state = Done;
            } else {
                restart(c);
            }
            break;
        case Done:
            break;
        }
        return m_state == Done;
    }
    bool done() const { return m_state == Done; }
    std::string token() const { return done() ? "Basic " + m_token : ""; }

private:
    enum State { Name, Equals, Quote, Word, Gap, Token, Done };
    static bool isSpace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
    static bool isTokenChar(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '+' || c == '/' || c == '=';
    }
    // Not the assignment after all: search again, this byte included
    void restart(char c) {
        m_state = Name;
        m_pos = 0;
        m_seenGap = false;
        m_token.clear();
        feed(c);
    }
    State m_state = Name;
    size_t m_pos = 0;
    char m_quote = '"';
    bool m_seenGap = false;
    std::string m_token;
};

// "2026-11-26T22:00:00.000Z" -> UTC epoch; 0 when it does not read as one
inline int64_t villagesParseIso(const char *s) {
    int y, mo, d, h, mi, sec;
    if (!s || sscanf(s, "%d-%d-%dT%d:%d:%d", &y, &mo, &d, &h, &mi, &sec) != 6) {
        return 0;
    }
    if (mo < 1 || mo > 12 || d < 1 || d > 31) {
        return 0;
    }
    // days from civil (Howard Hinnant)
    y -= mo <= 2;
    const int era = (y >= 0 ? y : y - 399) / 400;
    const int yoe = y - era * 400;
    const int doy = (153 * (mo + (mo > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    const int64_t days = (int64_t)era * 146097 + doe - 719468;
    return days * 86400 + h * 3600 + mi * 60 + sec;
}

// "5:00 PM" or "17:00" for a UTC epoch and the zone's offset in seconds
inline std::string villagesFormatTime(int64_t utc, int offsetS, bool hour24) {
    int64_t local = utc + offsetS;
    int minutes = (int)((((local % 86400) + 86400) % 86400) / 60);
    int h = minutes / 60, m = minutes % 60;
    char buf[16];
    if (hour24) {
        snprintf(buf, sizeof(buf), "%02d:%02d", h, m);
    } else {
        snprintf(buf, sizeof(buf), "%d:%02d %s", h % 12 == 0 ? 12 : h % 12, m, h >= 12 ? "PM" : "AM");
    }
    return buf;
}

inline void villagesFilter(JsonDocument &filter) {
    JsonObject e = filter["events"][0].to<JsonObject>();
    e["title"] = true;
    e["location"]["title"] = true;
    e["start"]["date"] = true;
    e["end"]["date"] = true;
}

// Fills acts[venue] from the reply, earliest first, dropping acts that ended
// before `now` (kept all when now is 0: the clock is not set). Returns false
// when the reply has no events list at all.
inline bool villagesParse(JsonDocument &doc, std::vector<VillagesAct> acts[VILLAGES_VENUES], int64_t now) {
    JsonArray events = doc["events"].as<JsonArray>();
    if (events.isNull()) {
        return false;
    }
    for (int i = 0; i < VILLAGES_VENUES; i++) {
        acts[i].clear();
    }
    for (JsonObject e : events) {
        const std::string where = e["location"]["title"] | "";
        const char *title = e["title"] | "";
        if (!*title) {
            continue;
        }
        for (int i = 0; i < VILLAGES_VENUES; i++) {
            if (where.find(VILLAGES_VENUE_LIST[i].match) == std::string::npos) {
                continue;
            }
            VillagesAct a;
            a.title = title;
            a.start = villagesParseIso(e["start"]["date"] | "");
            a.end = villagesParseIso(e["end"]["date"] | "");
            if (!(now > 0 && a.end > 0 && a.end < now)) {
                acts[i].push_back(a);
            }
            break;
        }
    }
    for (int i = 0; i < VILLAGES_VENUES; i++) {
        std::stable_sort(acts[i].begin(), acts[i].end(), [](const VillagesAct &a, const VillagesAct &b) { return a.start < b.start; });
    }
    return true;
}
