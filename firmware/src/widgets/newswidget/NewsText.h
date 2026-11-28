#pragma once

#include "../mqttwidget/MqttText.h"
#include <cstdint>
#include <string>
#include <vector>

namespace news {

// Keep supported UTF-8, but never pass malformed sequences, controls or
// unsupported glyphs to OpenFontRender (which cannot decode four-byte UTF-8).
template <class HasGlyph>
std::string displayText(const std::string &text, HasGlyph hasGlyph) {
    std::string out;
    for (size_t i = 0; i < text.size();) {
        const unsigned char first = static_cast<unsigned char>(text[i]);
        size_t length = first < 0x80 ? 1 : first >= 0xC2 && first <= 0xDF ? 2 :
                        first >= 0xE0 && first <= 0xEF ? 3 :
                        first >= 0xF0 && first <= 0xF4 ? 4 : 0;
        uint32_t cp = length == 1 ? first : length == 2 ? first & 0x1F :
                      length == 3 ? first & 0x0F : first & 0x07;
        bool valid = length && i + length <= text.size();
        for (size_t j = 1; valid && j < length; j++) {
            unsigned char next = static_cast<unsigned char>(text[i + j]);
            if ((next & 0xC0) != 0x80) valid = false;
            else cp = (cp << 6) | (next & 0x3F);
        }
        if (valid && ((length == 3 && cp < 0x800) || (length == 4 && cp < 0x10000) ||
                      (cp >= 0xD800 && cp <= 0xDFFF) || cp > 0x10FFFF)) valid = false;
        if (!valid) {
            out += '?';
            ++i;
            continue;
        }
        if (cp == 0x20 || cp == 0xA0 || cp == '\n' || cp == '\r' || cp == '\t') {
            if (!out.empty() && out.back() != ' ') out += ' ';
        } else if (cp < 0x20 || (cp >= 0x7F && cp <= 0x9F) ||
                   (cp >= 0x200B && cp <= 0x200F) || (cp >= 0x202A && cp <= 0x202E) ||
                   (cp >= 0x2066 && cp <= 0x2069) || cp == 0xFEFF ||
                   (cp >= 0xFE00 && cp <= 0xFE0F)) {
            // Invisible controls must not reach the font renderer.
        } else if (length <= 3 && hasGlyph(cp)) {
            out.append(text, i, length);
        } else if (cp == 0x2018 || cp == 0x2019) {
            out += '\'';
        } else if (cp == 0x201C || cp == 0x201D) {
            out += '"';
        } else if (cp == 0x2013 || cp == 0x2014) {
            out += '-';
        } else if (cp == 0x2026) {
            out += "...";
        } else {
            out += '?';
        }
        i += length;
    }
    if (!out.empty() && out.back() == ' ') out.pop_back();
    return out;
}

constexpr int kHeadlineMinSize = 12;
constexpr int kHeadlineMaxSize = 27;
constexpr int kHeadlineWidth = 175;
constexpr int kHeadlineTop = 73;
constexpr int kHeadlineBottom = 158;

struct HeadlineLayout {
    int size;
    std::vector<std::string> lines;
};

template <class Measure>
HeadlineLayout fitHeadline(const std::string &text, Measure measure) {
    const int height = kHeadlineBottom - kHeadlineTop;
    for (int size = kHeadlineMaxSize; size >= kHeadlineMinSize; --size) {
        std::vector<std::string> lines;
        if (mqttFits(text, size, kHeadlineWidth, height, measure, &lines)) return {size, lines};
    }
    return {kHeadlineMinSize, mqttCutToFit(text, kHeadlineMinSize, kHeadlineWidth, height, measure)};
}

} // namespace news
