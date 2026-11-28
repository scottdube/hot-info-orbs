#include "VisualCrossingSource.h"

#include "GlobalTime.h" // LOC_LANG

#include "Settings.h"
#include "SettingsValidation.h"
#include "config_helper.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <math.h>

// This is a hack to support old config.h files that have WEATHER_LOCAION instead of LOCATION.
#ifndef WEATHER_LOCATION
    #define WEATHER_LOCATION WEATHER_LOCAION
#endif

// Built from Settings (location and units are on the settings page). The
// location is url-encoded: "The Villages, FL" has spaces and a comma, and upstream
// issue #337 reports spaces failing.
VisualCrossingSource::VisualCrossingSource() : VisualCrossingSource(String(Settings::get().wxloc.c_str())) {}

VisualCrossingSource::VisualCrossingSource(const String &location) {
    const SettingsValues &s = Settings::get();
    m_url = String("https://weather.visualcrossing.com/VisualCrossingWebServices/rest/services/timeline/") +
            sv::urlEncode(location.c_str()).c_str() + "/next3days?key=" + s.wxkey.c_str() +
            "&unitGroup=" + (s.wxmetric ? "metric" : "us") +
            "&include=days,current&iconSet=icons1&lang=" + LOC_LANG;
}

bool VisualCrossingSource::fetch(WeatherDataModel &model) {
    if (Settings::get().wxkey.empty()) {
        m_error = "no Visual Crossing key";
        return false; // no key yet: the request would only be refused
    }
    HTTPClient http;
    http.begin(m_url);
    int httpCode = http.GET();
    if (httpCode > 0) {
        // Check for the return code   TODO: factor out
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, http.getString());
        http.end();

        if (!error) {
            model.setCityName(doc["resolvedAddress"].as<String>());
            // tzoffset is the location's offset from UTC in hours (e.g. -5, 5.5);
            // the World Clock page reads it to show each city's local time. It
            // may arrive as an int (-5) or a float (5.5), so test for presence.
            if (!doc["tzoffset"].isNull()) {
                model.setTzOffset((int32_t)lround(doc["tzoffset"].as<float>() * 3600.0));
            }
            model.setCurrentTemperature(doc["currentConditions"]["temp"].as<float>());
            model.setCurrentText(doc["days"][0]["description"].as<String>());

            model.setCurrentIcon(doc["currentConditions"]["icon"].as<String>());
            m_conditions = doc["currentConditions"]["conditions"] | "";
            model.setTodayHigh(doc["days"][0]["tempmax"].as<float>());
            model.setTodayLow(doc["days"][0]["tempmin"].as<float>());
            model.setSunTimes(doc["days"][0]["sunriseEpoch"] | (int64_t)0, doc["days"][0]["sunsetEpoch"] | (int64_t)0);
            for (int i = 0; i < 3; i++) {
                model.setDayIcon(i, doc["days"][i + 1]["icon"].as<String>());
                model.setDayHigh(i, doc["days"][i + 1]["tempmax"].as<float>());
                model.setDayLow(i, doc["days"][i + 1]["tempmin"].as<float>());
            }
        } else {
            // Handle JSON deserialization error
            switch (error.code()) {
            case DeserializationError::Ok:
                Serial.print(F("Deserialization succeeded"));
                break;
            case DeserializationError::InvalidInput:
                Serial.print(F("Invalid input!"));
                break;
            case DeserializationError::NoMemory:
                Serial.print(F("Not enough memory"));
                break;
            default:
                Serial.print(F("Deserialization failed"));
                break;
            }
            m_error = String("JSON: ") + error.c_str();

            return false;
        }
    } else {
        // Handle HTTP request error
        Serial.printf("HTTP request failed, error: %s\n", http.errorToString(httpCode).c_str());
        m_error = http.errorToString(httpCode);
        http.end();
        return false;
    }
    return true;
}
