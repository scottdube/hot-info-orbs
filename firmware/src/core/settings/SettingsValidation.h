#pragma once
// Pure parsing and validation for the settings page. std-only on purpose, so
// it compiles on the host and is covered by `pio test -e native`
// (test/test_validation). Each parser either writes a normalized value to
// `out` and returns true, or writes a message for the page to `err` and
// returns false.

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace sv {

inline std::string trim(const std::string &in) {
    size_t start = in.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    size_t end = in.find_last_not_of(" \t\r\n");
    return in.substr(start, end - start + 1);
}

inline bool parseInt(const std::string &in, int &out) {
    std::string s = trim(in);
    if (s.empty() || s.size() > 6) {
        return false;
    }
    size_t i = (s[0] == '-') ? 1 : 0;
    if (i == s.size()) {
        return false;
    }
    for (size_t j = i; j < s.size(); j++) {
        if (!isdigit((unsigned char)s[j])) {
            return false;
        }
    }
    out = std::stoi(s);
    return true;
}

// Seconds per page: 0 turns rotation off, otherwise 5..3600
inline bool parseCycle(const std::string &in, int &out, std::string &err) {
    int v;
    if (!parseInt(in, v) || (v != 0 && (v < 5 || v > 3600))) {
        err = "Enter 0 (off) or 5 to 3600 seconds";
        return false;
    }
    out = v;
    return true;
}

inline bool parseHour(const std::string &in, int &out, std::string &err) {
    int v;
    if (!parseInt(in, v) || v < 0 || v > 23) {
        err = "Enter an hour from 0 to 23";
        return false;
    }
    out = v;
    return true;
}

// Whether a '\n'-separated list holds item exactly (the hidden-widget list;
// widget names cannot contain a line break, normalizeText refuses one)
inline bool listContains(const std::string &list, const std::string &item) {
    size_t start = 0;
    while (start <= list.size()) {
        size_t end = list.find('\n', start);
        if (end == std::string::npos) {
            end = list.size();
        }
        if (list.compare(start, end - start, item) == 0 && !item.empty()) {
            return true;
        }
        start = end + 1;
    }
    return false;
}

// True when hour24 falls in [start, end), wrapping midnight when end < start
// (22..7 covers 22:00-06:59). start == end is an empty range; the page refuses it.
inline bool inHourRange(int hour24, int start, int end) {
    if (start < end) {
        return hour24 >= start && hour24 < end;
    }
    if (start > end) {
        return hour24 >= start || hour24 < end;
    }
    return false;
}

// 1..5 comma-separated symbols. Upper-cases the symbol but not an exchange
// qualifier after '&' ("SHOP&country=Canada", as config.h documents).
inline bool normalizeTickers(const std::string &in, std::string &out, std::string &err) {
    const size_t maxTickers = 5; // StockWidget.h MAX_STOCKS
    std::vector<std::string> items;
    size_t start = 0;
    while (start <= in.size()) {
        size_t comma = in.find(',', start);
        std::string item = trim(in.substr(start, comma == std::string::npos ? std::string::npos : comma - start));
        if (!item.empty()) {
            for (char c : item) {
                if (!(isalnum((unsigned char)c) || std::string("./&=:_^-").find(c) != std::string::npos)) {
                    err = "Ticker \"" + item + "\" contains a character that is not allowed";
                    return false;
                }
            }
            size_t amp = item.find('&');
            for (size_t i = 0; i < item.size() && i < amp; i++) {
                item[i] = (char)toupper((unsigned char)item[i]);
            }
            items.push_back(item);
        }
        if (comma == std::string::npos) {
            break;
        }
        start = comma + 1;
    }
    if (items.empty()) {
        err = "Enter at least one ticker";
        return false;
    }
    if (items.size() > maxTickers) {
        err = "At most 5 tickers - one per orb";
        return false;
    }
    std::string joined;
    for (size_t i = 0; i < items.size(); i++) {
        joined += (i ? "," : "") + items[i];
    }
    if (joined.size() > 128) {
        err = "Ticker list is too long";
        return false;
    }
    out = joined;
    return true;
}

// Trims, collapses runs of spaces, 1..maxLen, no control characters
inline bool normalizeText(const std::string &in, size_t maxLen, std::string &out, std::string &err) {
    std::string collapsed;
    for (char c : in) {
        if ((unsigned char)c < 0x20 || c == 0x7F) {
            err = "Contains a control character (tab or line break)";
            return false;
        }
        if (c == ' ' && !collapsed.empty() && collapsed.back() == ' ') {
            continue;
        }
        collapsed += c;
    }
    collapsed = trim(collapsed);
    if (collapsed.empty()) {
        err = "Cannot be empty";
        return false;
    }
    if (collapsed.size() > maxLen) {
        err = "At most " + std::to_string(maxLen) + " characters";
        return false;
    }
    out = collapsed;
    return true;
}

// Tempest station ID: blank = unused (0), otherwise 1..9 digits. parseInt
// stops at 6 characters, and station IDs are already 6 digits long.
inline bool parseStationId(const std::string &in, uint32_t &out, std::string &err) {
    std::string s = trim(in);
    if (s.empty()) {
        out = 0;
        return true;
    }
    if (s.size() > 9) {
        err = "A station ID is at most 9 digits";
        return false;
    }
    for (char c : s) {
        if (!isdigit((unsigned char)c)) {
            err = "Digits only - the number in the station's tempestwx.com address";
            return false;
        }
    }
    out = (uint32_t)std::stoul(s);
    return true;
}

// A Tempest page ticked on the settings page needs a station behind it. An
// ID left in place with the page unticked is fine: the page is off, the ID kept.
inline bool checkStationEnabled(bool enabled, uint32_t stationId, std::string &err) {
    if (enabled && stationId == 0) {
        err = "Enter a station ID or untick the page";
        return false;
    }
    return true;
}

// Label shown on the orb in place of the town name: 1..16 characters (orb 1
// shrinks the text to fit, "Salt Lake City" is 14), and
// only required when the station is in use
inline bool parseStationLabel(const std::string &in, uint32_t stationId, std::string &out, std::string &err) {
    if (trim(in).empty() && stationId == 0) {
        out = "";
        return true;
    }
    return normalizeText(in, 16, out, err);
}

// RFC 3986: unreserved characters kept, everything else %XX
inline std::string urlEncode(const std::string &in) {
    static const char hex[] = "0123456789ABCDEF";
    std::string out;
    for (unsigned char c : in) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out += (char)c;
        } else {
            out += '%';
            out += hex[c >> 4];
            out += hex[c & 0x0F];
        }
    }
    return out;
}

// "#rrggbb" (what <input type=color> sends) to RGB565
inline bool parseHexColor(const std::string &in, uint16_t &rgb565, std::string &err) {
    std::string s = trim(in);
    if (s.size() != 7 || s[0] != '#') {
        err = "Color must look like #ff8800";
        return false;
    }
    for (size_t i = 1; i < 7; i++) {
        if (!isxdigit((unsigned char)s[i])) {
            err = "Color must look like #ff8800";
            return false;
        }
    }
    unsigned long v = std::stoul(s.substr(1), nullptr, 16);
    uint8_t r = (v >> 16) & 0xFF, g = (v >> 8) & 0xFF, b = v & 0xFF;
    rgb565 = (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
    return true;
}

// RGB565 back to "#rrggbb", bit-replicated so white stays #ffffff
inline std::string rgb565ToHex(uint16_t c) {
    uint8_t r5 = (c >> 11) & 0x1F, g6 = (c >> 5) & 0x3F, b5 = c & 0x1F;
    uint8_t r = (r5 << 3) | (r5 >> 2), g = (g6 << 2) | (g6 >> 4), b = (b5 << 3) | (b5 >> 2);
    char buf[8];
    snprintf(buf, sizeof(buf), "#%02x%02x%02x", r, g, b);
    return buf;
}

inline std::string htmlEscape(const std::string &in) {
    std::string out;
    for (char c : in) {
        switch (c) {
        case '&': out += "&amp;"; break;
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '"': out += "&quot;"; break;
        case '\'': out += "&#39;"; break;
        default: out += c;
        }
    }
    return out;
}

// The boot picture must be a 240x240 baseline JPEG: TJpg_Decoder cannot
// decode progressive ones. Walks the markers in `buf` (the start of the file)
// up to the first SOF.
inline bool checkJpegHeader(const uint8_t *buf, size_t len, std::string &err) {
    if (len < 4 || buf[0] != 0xFF || buf[1] != 0xD8) {
        err = "This is not a JPEG file";
        return false;
    }
    size_t pos = 2;
    while (pos + 3 < len) {
        if (buf[pos] != 0xFF) {
            err = "Damaged JPEG header";
            return false;
        }
        uint8_t marker = buf[pos + 1];
        if (marker == 0xFF) { // fill byte
            pos++;
            continue;
        }
        if (marker == 0xD9 || marker == 0xDA) {
            break; // end of image or start of scan, and no frame header seen
        }
        if (marker == 0x01 || (marker >= 0xD0 && marker <= 0xD7)) {
            pos += 2; // standalone markers carry no length
            continue;
        }
        size_t segLen = ((size_t)buf[pos + 2] << 8) | buf[pos + 3];
        bool isSof = marker >= 0xC0 && marker <= 0xCF && marker != 0xC4 && marker != 0xC8 && marker != 0xCC;
        if (isSof) {
            if (marker == 0xC2) {
                err = "This is a progressive JPEG, which the orbs cannot show - re-save it as baseline";
                return false;
            }
            if (marker != 0xC0) {
                err = "Unsupported JPEG type - save it as a standard (baseline) JPEG";
                return false;
            }
            if (pos + 8 >= len) {
                break;
            }
            int height = (buf[pos + 5] << 8) | buf[pos + 6];
            int width = (buf[pos + 7] << 8) | buf[pos + 8];
            if (width != 240 || height != 240) {
                err = "Picture is " + std::to_string(width) + "x" + std::to_string(height) + ", must be 240x240";
                return false;
            }
            return true;
        }
        pos += 2 + segLen;
    }
    err = "This is not a JPEG file - no image header found";
    return false;
}

// --- custom pages (docs/superpowers/specs/2026-10-17-custom-pages-design.md) --

// What the settings page shows in place of a URL's query string, which often
// carries an API key. UTF-8 for "?•••"; the page is served as utf-8.
constexpr const char *URL_MASK = "?\xE2\x80\xA2\xE2\x80\xA2\xE2\x80\xA2";

inline std::string maskUrl(const std::string &url) {
    size_t q = url.find('?');
    return q == std::string::npos ? url : url.substr(0, q) + URL_MASK;
}

// Web Data URL. Blank = no page. The masked form of the stored URL, sent back
// unchanged, keeps the stored URL; anything else replaces it.
inline bool parseWebUrl(const std::string &in, const std::string &stored, std::string &out, std::string &err) {
    std::string s = trim(in);
    if (s.empty()) {
        out = "";
        return true;
    }
    if (!stored.empty() && s == maskUrl(stored)) {
        out = stored;
        return true;
    }
    if (s.find(URL_MASK) != std::string::npos) {
        err = "The part after ? is hidden, so it cannot be kept with a changed address - type the whole address";
        return false;
    }
    if (s.size() > 256) {
        err = "At most 256 characters";
        return false;
    }
    std::string lower;
    for (char c : s.substr(0, 8)) {
        lower += (char)tolower((unsigned char)c);
    }
    size_t hostStart;
    if (lower.compare(0, 7, "http://") == 0) {
        hostStart = 7;
    } else if (lower.compare(0, 8, "https://") == 0) {
        hostStart = 8;
    } else {
        err = "Must start with http:// or https://";
        return false;
    }
    for (char c : s) {
        if ((unsigned char)c <= 0x20 || c == 0x7F) {
            err = "No spaces or control characters";
            return false;
        }
    }
    size_t hostEnd = s.find_first_of("/?:#", hostStart);
    if ((hostEnd == std::string::npos ? s.size() : hostEnd) == hostStart) {
        err = "No host name after " + s.substr(0, hostStart);
        return false;
    }
    out = s;
    return true;
}

// MQTT broker: blank = no page, otherwise a host name or IPv4 address
inline bool parseHost(const std::string &in, std::string &out, std::string &err) {
    std::string s = trim(in);
    if (s.empty()) {
        out = "";
        return true;
    }
    if (s.size() > 64) {
        err = "At most 64 characters";
        return false;
    }
    for (char c : s) {
        if (!(isalnum((unsigned char)c) || c == '.' || c == '-')) {
            err = "Host name or IP address only - letters, digits, dots and dashes (no mqtt:// and no :port)";
            return false;
        }
    }
    if (s.front() == '.' || s.front() == '-' || s.back() == '.' || s.back() == '-') {
        err = "Cannot start or end with a dot or dash";
        return false;
    }
    out = s;
    return true;
}

inline bool parsePort(const std::string &in, int &out, std::string &err) {
    int v;
    if (!parseInt(in, v) || v < 1 || v > 65535) {
        err = "Enter a port from 1 to 65535 (MQTT is usually 1883)";
        return false;
    }
    out = v;
    return true;
}

// Blank allowed; otherwise normalizeText's rules
inline bool parseOptionalText(const std::string &in, size_t maxLen, std::string &out, std::string &err) {
    if (trim(in).empty()) {
        out = "";
        return true;
    }
    return normalizeText(in, maxLen, out, err);
}

// One MQTT topic to subscribe to. Blank leaves that orb empty. Wildcards are
// refused: one orb shows one value.
inline bool parseTopic(const std::string &in, std::string &out, std::string &err) {
    std::string s = trim(in);
    if (s.size() > 128) {
        err = "At most 128 characters";
        return false;
    }
    for (char c : s) {
        if ((unsigned char)c < 0x20 || c == 0x7F) {
            err = "Contains a control character (tab or line break)";
            return false;
        }
        if (c == '#' || c == '+') {
            err = "No # or + wildcards - one topic per orb";
            return false;
        }
    }
    out = s;
    return true;
}

// Write-only: the page never shows the stored password. Blank keeps it,
// `clear` (the checkbox) removes it. Not trimmed - a space may be part of it.
inline bool parsePassword(const std::string &in, bool clear, const std::string &stored, std::string &out, std::string &err) {
    if (clear) {
        out = "";
        return true;
    }
    if (in.empty()) {
        out = stored;
        return true;
    }
    if (in.size() > 64) {
        err = "At most 64 characters";
        return false;
    }
    for (char c : in) {
        if ((unsigned char)c < 0x20 || c == 0x7F) {
            err = "Contains a control character (tab or line break)";
            return false;
        }
    }
    out = in;
    return true;
}

// Hint under the password field. `v` is what the form holds after parsing,
// `stored` what is saved. They differ only on a rejected save (some other
// field failed), where the field comes back empty: say so, or the member
// resubmits blank and silently keeps the OLD password. Never echoes either.
inline const char *passwordHint(const std::string &v, const std::string &stored) {
    if (v == stored) {
        return stored.empty() ? "No password stored." : "&#10004; A password is stored. It is never shown; leave this blank to keep it.";
    }
    if (v.empty()) {
        return "&#9888; Not saved: tick the box below again to clear the stored password. A password is still stored.";
    }
    return stored.empty() ? "&#9888; Not saved: type the password again. No password is stored."
                          : "&#9888; Not saved: type the password again. The stored one is unchanged.";
}

} // namespace sv
