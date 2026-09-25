#pragma once
// Tempest (WeatherFlow) better_forecast parsing. std + ArduinoJson only, so it
// is covered by `pio test -e native` (test/test_tempest). Measured reply
// shape: docs/TEMPEST-API.md.

#include <ArduinoJson.h>
#include <cmath>
#include <cstdio>
#include <string>

struct TempestDay {
    std::string icon; // already translated to the orb's icon names
    float high = 0;
    float low = 0;
    int64_t sunrise = 0; // UTC epoch, 0 when absent
    int64_t sunset = 0;
};

struct TempestReading {
    std::string conditions; // "Partly Cloudy"
    std::string icon; // translated
    float temp = 0;
    TempestDay days[4]; // [0] = today, [1..3] = the forecast orb
    // Station readings for orb 1; NAN when the reply lacks them
    float feels = NAN;
    float humidity = NAN;
    float windAvg = NAN;
    float windGust = NAN;
    std::string windDir; // "NNW"
};

// Orb 1 draws at most 4 lines of 18 characters
static const size_t TEMPEST_LINE = 18;

inline std::string tempestFitLine(std::string s) {
    size_t at = s.find("Thunderstorms");
    if (at != std::string::npos) {
        s.replace(at, 13, "T-storms");
    }
    if (s.size() > TEMPEST_LINE) {
        size_t cut = s.rfind(' ', TEMPEST_LINE);
        s = s.substr(0, cut == std::string::npos ? TEMPEST_LINE : cut);
    }
    return s;
}

// Orb 1's text on a Tempest page, one reading per line, '\n'-separated:
// "Clear\nFeels like 49\nHumidity 82%\nWind W 4-7 mph". A reading missing
// from the reply is left out rather than shown as 0.
inline std::string tempestSummary(const TempestReading &r, const char *windUnit) {
    std::string out = tempestFitLine(r.conditions);
    char buf[40];
    if (!std::isnan(r.feels)) {
        snprintf(buf, sizeof(buf), "\nFeels like %d", (int)lroundf(r.feels));
        out += buf;
    }
    if (!std::isnan(r.humidity)) {
        snprintf(buf, sizeof(buf), "\nHumidity %d%%", (int)lroundf(r.humidity));
        out += buf;
    }
    if (!std::isnan(r.windAvg)) {
        int avg = (int)lroundf(r.windAvg);
        int gust = std::isnan(r.windGust) ? avg : (int)lroundf(r.windGust);
        if (avg == 0 && gust == 0) {
            snprintf(buf, sizeof(buf), "\nWind calm");
        } else if (gust > avg) {
            snprintf(buf, sizeof(buf), "\nWind %s %d-%d %s", r.windDir.c_str(), avg, gust, windUnit);
        } else {
            snprintf(buf, sizeof(buf), "\nWind %s %d %s", r.windDir.c_str(), avg, windUnit);
        }
        out += tempestFitLine(buf + 1).insert(0, "\n");
    }
    return out;
}

// Tempest icon names to the set WeatherWidget::drawWeatherIcon knows
inline std::string tempestIcon(const std::string &t) {
    if (t == "clear-day" || t == "clear-night" || t == "cloudy" || t == "partly-cloudy-day" || t == "partly-cloudy-night") {
        return t;
    }
    if (t == "rainy" || t == "thunderstorm" || t.rfind("possibly-rainy", 0) == 0 || t.rfind("possibly-thunderstorm", 0) == 0) {
        return "rain"; // no storm icon on the orb
    }
    if (t == "snow" || t == "sleet" || t.rfind("possibly-snow", 0) == 0 || t.rfind("possibly-sleet", 0) == 0) {
        return "snow";
    }
    if (t == "foggy") {
        return "fog";
    }
    if (t == "windy") {
        return "wind";
    }
    return "cloudy";
}

// Keeps ~1 KB of a ~96 KB reply: the hourly forecast is dropped as it streams
inline void tempestFilter(JsonDocument &filter) {
    filter["current_conditions"]["conditions"] = true;
    filter["current_conditions"]["icon"] = true;
    filter["current_conditions"]["air_temperature"] = true;
    filter["current_conditions"]["feels_like"] = true;
    filter["current_conditions"]["relative_humidity"] = true;
    filter["current_conditions"]["wind_avg"] = true;
    filter["current_conditions"]["wind_gust"] = true;
    filter["current_conditions"]["wind_direction_cardinal"] = true;
    filter["forecast"]["daily"][0]["icon"] = true; // [0] in a filter applies to every element
    filter["forecast"]["daily"][0]["air_temp_high"] = true;
    filter["forecast"]["daily"][0]["air_temp_low"] = true;
    filter["forecast"]["daily"][0]["sunrise"] = true;
    filter["forecast"]["daily"][0]["sunset"] = true;
}

inline bool tempestParse(JsonDocument &doc, TempestReading &out, std::string &err) {
    JsonObject cc = doc["current_conditions"];
    if (cc.isNull() || !cc["air_temperature"].is<float>()) {
        err = "no current conditions in the reply";
        return false;
    }
    JsonArray daily = doc["forecast"]["daily"];
    if (daily.isNull() || daily.size() < 4) {
        err = "fewer than 4 forecast days";
        return false;
    }
    TempestReading r;
    r.conditions = cc["conditions"] | "";
    r.icon = tempestIcon(cc["icon"] | "");
    r.temp = cc["air_temperature"].as<float>();
    r.feels = cc["feels_like"] | NAN;
    r.humidity = cc["relative_humidity"] | NAN;
    r.windAvg = cc["wind_avg"] | NAN;
    r.windGust = cc["wind_gust"] | NAN;
    r.windDir = cc["wind_direction_cardinal"] | "";
    for (int i = 0; i < 4; i++) {
        r.days[i].icon = tempestIcon(daily[i]["icon"] | "");
        r.days[i].high = daily[i]["air_temp_high"] | 0.0f;
        r.days[i].low = daily[i]["air_temp_low"] | 0.0f;
        r.days[i].sunrise = daily[i]["sunrise"] | (int64_t)0;
        r.days[i].sunset = daily[i]["sunset"] | (int64_t)0;
    }
    out = r;
    return true;
}
