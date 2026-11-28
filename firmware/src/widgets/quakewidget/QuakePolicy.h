#pragma once
// Pure helpers for the Earthquake Monitor page: the ring band from a
// magnitude, a short location label, and the elapsed-time text. std-only on
// purpose, so it compiles on the host and is covered by `pio test -e native`
// (test/test_quake). The widget maps a band to a TFT color; keeping the math
// here keeps the color code out of the host tests.

#include <cstdint>
#include <string>

namespace quake {

// Ring color bands (QuakeWidget maps these to TFT colors). These are display
// conventions, not official impact or danger classes: real risk depends on
// location, depth, shaking and exposure.
//   below 4.0  green
//   4.0-4.9    yellow
//   5.0-5.9    orange
//   6.0+       red
enum class Band : uint8_t { Green, Yellow, Orange, Red };

inline Band bandFor(float magnitude) {
    if (magnitude >= 6.0f) {
        return Band::Red;
    }
    if (magnitude >= 5.0f) {
        return Band::Orange;
    }
    if (magnitude >= 4.0f) {
        return Band::Yellow;
    }
    return Band::Green;
}

// A short label for the small orb. USGS places read like "92km SSW of Example,
// Country", "Near the coast of Japan" or "South of the Fiji Islands". The most
// useful short name is the region: the text after the last comma, or failing
// that the text after the last " of " ("Near the coast of Japan" -> "Japan").
// Without either, the whole string is used. The result is trimmed and
// upper-cased, cut to maxLen with no trailing space.
inline std::string shortPlace(const std::string &place, size_t maxLen = 10) {
    std::string s = place;
    size_t comma = s.rfind(',');
    if (comma != std::string::npos && comma + 1 < s.size()) {
        s = s.substr(comma + 1);
    } else {
        size_t of = s.rfind(" of ");
        if (of != std::string::npos) {
            s = s.substr(of + 4);
        }
    }
    size_t start = s.find_first_not_of(" \t");
    size_t end = s.find_last_not_of(" \t");
    if (start == std::string::npos) {
        return "";
    }
    s = s.substr(start, end - start + 1);
    std::string out;
    for (char c : s) {
        if (c >= 'a' && c <= 'z') {
            out += (char)(c - 'a' + 'A');
        } else {
            out += c;
        }
    }
    if (out.size() > maxLen) {
        out.resize(maxLen);
        size_t last = out.find_last_not_of(' ');
        out = last == std::string::npos ? std::string() : out.substr(0, last + 1);
    }
    return out;
}

// Elapsed time since an event, from two UTC epochs. "now" / "8m" / "2h" /
// "3d". A future or equal time reads as "now"; days cap the display at the
// 24-48 h the feed covers, but older values still render.
inline std::string ageText(long eventEpoch, long nowEpoch) {
    long secs = nowEpoch - eventEpoch;
    if (secs < 60) {
        return "now";
    }
    long mins = secs / 60;
    if (mins < 60) {
        return std::to_string(mins) + "m";
    }
    long hours = mins / 60;
    if (hours < 24) {
        return std::to_string(hours) + "h";
    }
    return std::to_string(hours / 24) + "d";
}

// Magnitude as one decimal, e.g. "5.4" or "6.0". Rounded to the tenth.
inline std::string magText(float magnitude) {
    if (magnitude < 0.0f) {
        magnitude = 0.0f;
    }
    int tenths = (int)(magnitude * 10.0f + 0.5f);
    return std::to_string(tenths / 10) + "." + std::to_string(tenths % 10);
}

// The USGS summary feed to pull, chosen from the minimum magnitude. The feeds
// come in fixed thresholds; the smallest threshold at or below the wanted
// magnitude is used, and the widget filters the rest out. "significant" is not
// used: it is a curated subset, not a plain magnitude cut.
//   >= 4.5  the 4.5 feed
//   >= 2.5  the 2.5 feed
//   >= 1.0  the 1.0 feed
//   else    all
inline const char *feedUrl(float minMagnitude) {
    if (minMagnitude >= 4.5f) {
        return "https://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/4.5_day.geojson";
    }
    if (minMagnitude >= 2.5f) {
        return "https://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/2.5_day.geojson";
    }
    if (minMagnitude >= 1.0f) {
        return "https://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/1.0_day.geojson";
    }
    return "https://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/all_day.geojson";
}

} // namespace quake
