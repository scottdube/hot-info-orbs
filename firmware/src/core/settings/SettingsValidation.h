#pragma once
// Pure parsing and validation for the settings page. std-only on purpose, so
// it compiles on the host and is covered by `pio test -e native`
// (test/test_validation). Each parser either writes a normalised value to
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

// 1..5 comma-separated symbols. Upper-cases the symbol but not an exchange
// qualifier after '&' ("SHOP&country=Canada", as config.h documents).
inline bool normaliseTickers(const std::string &in, std::string &out, std::string &err) {
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
inline bool normaliseText(const std::string &in, size_t maxLen, std::string &out, std::string &err) {
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
inline bool parseHexColour(const std::string &in, uint16_t &rgb565, std::string &err) {
    std::string s = trim(in);
    if (s.size() != 7 || s[0] != '#') {
        err = "Colour must look like #ff8800";
        return false;
    }
    for (size_t i = 1; i < 7; i++) {
        if (!isxdigit((unsigned char)s[i])) {
            err = "Colour must look like #ff8800";
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

} // namespace sv
