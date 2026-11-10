#pragma once
// Text layout for an MQTT orb's value. std only, so it is covered by
// `pio test -e native` (test/test_custompages).
//
// WHY: the value used to be drawn on one line with OpenFontRender's
// calculateFitFontSize, which ESTIMATES the size by extrapolating from sizes 10
// and 50 (its own doc: "may not exactly meet the size limit"). Long text such as
// "Tue 5pm Bobby Blackmon & The B3 Band" came out at ~10 px, and every orb
// picked its own size, so five orbs side by side jumped from 10 px to 40 px
// (measured on the Villages town-square feed 2026-11-09). Here the text is
// wrapped onto up to MQTT_TEXT_MAX_LINES lines and each candidate size is
// measured, not estimated. A newline in the payload forces a line break, so a
// publisher can put "5pm\nThe Rundown Band" on two lines of its choosing.

#include <string>
#include <vector>

constexpr int MQTT_TEXT_MAX_LINES = 3;
constexpr int MQTT_TEXT_MIN_SIZE = 8;
constexpr int MQTT_TEXT_MAX_SIZE = 120;
// Baseline-to-baseline distance as a fraction of the font size (Roboto's own
// line spacing is ~1.17)
constexpr float MQTT_TEXT_LINE_PITCH = 1.15f;

struct TextExtent {
    int w; // ink width, px
    int h; // ink height, px
};

// measure(text, size) -> TextExtent of one line at that font size
template <class Measure>
std::vector<std::string> mqttWrap(const std::string &text, int size, int maxW, Measure measure) {
    std::vector<std::string> lines;
    size_t start = 0;
    while (true) {
        size_t nl = text.find('\n', start);
        std::string para = text.substr(start, nl == std::string::npos ? std::string::npos : nl - start);
        // Greedy word wrap; a word wider than maxW stays whole on its own line
        // (the size search then rejects this size)
        std::string cur;
        size_t i = 0;
        while (i < para.size()) {
            while (i < para.size() && para[i] == ' ') {
                i++;
            }
            size_t j = para.find(' ', i);
            if (j == std::string::npos) {
                j = para.size();
            }
            if (j > i) {
                std::string word = para.substr(i, j - i);
                std::string trial = cur.empty() ? word : cur + " " + word;
                if (cur.empty() || measure(trial, size).w <= maxW) {
                    cur = trial;
                } else {
                    lines.push_back(cur);
                    cur = word;
                }
            }
            i = j;
        }
        lines.push_back(cur); // an empty paragraph keeps its blank line
        if (nl == std::string::npos) {
            break;
        }
        start = nl + 1;
    }
    // A trailing newline adds nothing
    while (lines.size() > 1 && lines.back().empty()) {
        lines.pop_back();
    }
    return lines;
}

// Height of a block of lines: pitch between baselines plus the tallest ink
inline int mqttBlockHeight(int lines, int size, int inkH) {
    return (int)((lines - 1) * size * MQTT_TEXT_LINE_PITCH) + inkH;
}

// True when text wrapped at this size fits maxW x maxH in maxLines lines
template <class Measure>
bool mqttFits(const std::string &text, int size, int maxW, int maxH, Measure measure,
              std::vector<std::string> *out = nullptr) {
    std::vector<std::string> lines = mqttWrap(text, size, maxW, measure);
    if ((int)lines.size() > MQTT_TEXT_MAX_LINES) {
        return false;
    }
    int inkH = 0;
    for (const auto &l : lines) {
        TextExtent e = measure(l.empty() ? std::string("X") : l, size);
        if (e.w > maxW) {
            return false;
        }
        if (e.h > inkH) {
            inkH = e.h;
        }
    }
    if (mqttBlockHeight((int)lines.size(), size, inkH) > maxH) {
        return false;
    }
    if (out) {
        *out = lines;
    }
    return true;
}

// Largest font size at which the text fits; MQTT_TEXT_MIN_SIZE when none does.
// Binary search: a smaller size never needs more lines or more width.
template <class Measure>
int mqttBestSize(const std::string &text, int maxW, int maxH, Measure measure) {
    int lo = MQTT_TEXT_MIN_SIZE, hi = MQTT_TEXT_MAX_SIZE;
    if (!mqttFits(text, lo, maxW, maxH, measure)) {
        return lo;
    }
    while (lo < hi) {
        int mid = (lo + hi + 1) / 2;
        if (mqttFits(text, mid, maxW, maxH, measure)) {
            lo = mid;
        } else {
            hi = mid - 1;
        }
    }
    return lo;
}

// The page's shared size: the smallest of the orbs' best sizes; 0 = no orb has a value
inline int mqttSharedSize(const std::vector<int> &bestSizes) {
    int s = 0;
    for (int b : bestSizes) {
        if (b > 0 && (s == 0 || b < s)) {
            s = b;
        }
    }
    return s;
}
