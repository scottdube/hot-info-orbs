#pragma once

#include <ArduinoJson.h>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>
#include <vector>

namespace news {

struct Category {
    const char *code;
    const char *name;
    const char *label;
};

// "us" is a country filter, not a NewsData.io category.
static const Category categories[] = {
    {"world", "World", "WORLD"}, {"us", "United States", "US"},
    {"business", "Business", "BIZ"}, {"technology", "Technology", "TECH"},
    {"science", "Science", "SCI"}, {"breaking", "Breaking", "BREAKING"},
    {"crime", "Crime", "CRIME"}, {"domestic", "Domestic", "DOMESTIC"},
    {"education", "Education", "EDUCATION"}, {"entertainment", "Entertainment", "ENTERTAIN"},
    {"environment", "Environment", "ENVIRON"}, {"food", "Food", "FOOD"},
    {"health", "Health", "HEALTH"}, {"lifestyle", "Lifestyle", "LIFESTYLE"},
    {"other", "Other", "OTHER"}, {"politics", "Politics", "POLITICS"},
    {"sports", "Sports", "SPORTS"}, {"top", "Top", "TOP"},
    {"tourism", "Tourism", "TOURISM"}
};
constexpr size_t categoryCount = sizeof(categories) / sizeof(categories[0]);

inline const Category *category(const std::string &code) {
    for (const auto &c : categories) {
        if (code == c.code) return &c;
    }
    return nullptr;
}

inline std::string normalizedTitle(const std::string &title) {
    std::string out;
    bool space = false;
    for (unsigned char c : title) {
        if (std::isalnum(c) || c >= 128) {
            if (space && !out.empty()) out += ' ';
            out += static_cast<char>(std::tolower(c));
            space = false;
        } else {
            space = true;
        }
    }
    return out;
}

inline std::string canonicalUrl(std::string url) {
    size_t fragment = url.find('#');
    if (fragment != std::string::npos) url.resize(fragment);
    size_t query = url.find('?');
    std::string path = url.substr(0, query);
    while (!path.empty() && path.back() == '/') path.pop_back();
    if (query == std::string::npos) return path;
    std::string kept;
    size_t start = query + 1;
    while (start < url.size()) {
        size_t end = url.find('&', start);
        std::string param = url.substr(start, end == std::string::npos ? std::string::npos : end - start);
        std::string key = param.substr(0, param.find('='));
        std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return std::tolower(c); });
        if (!param.empty() && key.compare(0, 4, "utm_") != 0 && key != "gclid" && key != "fbclid")
            kept += (kept.empty() ? "?" : "&") + param;
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return path + kept;
}

struct Article {
    std::string id, title, url, source, published;
    std::vector<std::string> category, country;
    int priority;
};

inline bool matches(const Article &a, const std::string &code) {
    const auto &list = code == "us" ? a.country : a.category;
    const std::string wanted = code == "us" ? "united states of america" : code;
    for (const auto &item : list) {
        if (item == wanted || (code == "us" && (item == "united states" || item == "us"))) return true;
    }
    return false;
}

// Category results are requested in descending publication order.
inline const Article *latest(const std::vector<Article> &articles, const std::string &code) {
    for (const auto &article : articles) {
        if (matches(article, code)) return &article;
    }
    return nullptr;
}

inline std::string boundedText(const char *text, size_t limit) {
    if (std::strlen(text) <= limit) return text;
    while (limit > 0 && (static_cast<unsigned char>(text[limit]) & 0xC0) == 0x80) --limit;
    return std::string(text, limit) + "...";
}

// Preserve the provider's title; only exact normalized titles and URLs are
// deduplicated, not stories merely guessed to describe the same event.
inline bool parse(JsonVariantConst root, std::vector<Article> &out) {
    if (std::string(root["status"] | "") != "success" || !root["results"].is<JsonArrayConst>()) return false;
    std::vector<Article> articles;
    for (JsonObjectConst item : root["results"].as<JsonArrayConst>()) {
        Article a;
        a.id = item["id"] | "";
        a.title = boundedText(item["title"] | "", 512);
        a.url = item["url"] | "";
        a.source = boundedText(item["source"]["name"] | "", 256);
        a.published = item["published_at"] | "";
        a.priority = item["source"]["priority"] | 999999;
        if (normalizedTitle(a.title).empty()) continue;
        for (JsonVariantConst c : item["category"].as<JsonArrayConst>()) {
            if (c.is<const char *>()) a.category.push_back(c.as<const char *>());
        }
        for (JsonVariantConst c : item["country"].as<JsonArrayConst>()) {
            if (c.is<const char *>()) a.country.push_back(c.as<const char *>());
        }
        bool duplicate = false;
        for (const auto &existing : articles) {
            if ((!a.id.empty() && a.id == existing.id) ||
                (!a.url.empty() && canonicalUrl(a.url) == canonicalUrl(existing.url)) ||
                normalizedTitle(a.title) == normalizedTitle(existing.title)) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) articles.push_back(a);
        if (articles.size() == 10) break;
    }
    out.swap(articles);
    return true;
}

inline void rank(std::vector<Article> &articles) {
    std::stable_sort(articles.begin(), articles.end(),
                     [](const Article &a, const Article &b) { return a.priority < b.priority; });
}

inline std::string categoryQuery(const std::string (&selected)[5], bool topFive) {
    if (topFive) return "";
    std::string result;
    for (const auto &code : selected) {
        if (code == "us") return "";
        if (!category(code)) continue;
        bool already = false;
        for (const auto &previous : selected) {
            if (&previous == &code) break;
            if (previous == code) already = true;
        }
        if (!already) result += (result.empty() ? "" : ",") + code;
    }
    return result;
}

} // namespace news
