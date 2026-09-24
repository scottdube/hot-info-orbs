#pragma once
// Tempest (WeatherFlow) better_forecast parsing. std + ArduinoJson only, so it
// is covered by `pio test -e native` (test/test_tempest). Measured reply
// shape: docs/TEMPEST-API.md.

#include <ArduinoJson.h>
#include <string>

struct TempestDay {
    std::string icon; // already translated to the orb's icon names
    float high = 0;
    float low = 0;
};

struct TempestReading {
    std::string conditions; // "Partly Cloudy"
    std::string icon; // translated
    float temp = 0;
    TempestDay days[4]; // [0] = today, [1..3] = the forecast orb
};

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
    filter["forecast"]["daily"][0]["icon"] = true; // [0] in a filter applies to every element
    filter["forecast"]["daily"][0]["air_temp_high"] = true;
    filter["forecast"]["daily"][0]["air_temp_low"] = true;
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
    for (int i = 0; i < 4; i++) {
        r.days[i].icon = tempestIcon(daily[i]["icon"] | "");
        r.days[i].high = daily[i]["air_temp_high"] | 0.0f;
        r.days[i].low = daily[i]["air_temp_low"] | 0.0f;
    }
    out = r;
    return true;
}
