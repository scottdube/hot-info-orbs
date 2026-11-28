#include "QuakeWidget.h"
#include "DeviceLogStore.h"

#include "GlobalTime.h"
#include "NoData.h"
#include "QuakePolicy.h"
#include "Settings.h"
#include "config_helper.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <algorithm>
#include <ctime>

QuakeWidget::QuakeWidget(ScreenManager &manager) : Widget(manager) {
    m_minMagnitude = Settings::get().quakemag;
}

void QuakeWidget::setup() {}

static uint32_t bandColor(quake::Band band) {
    switch (band) {
    case quake::Band::Red:
        return TFT_RED;
    case quake::Band::Orange:
        return TFT_ORANGE;
    case quake::Band::Yellow:
        return TFT_YELLOW;
    default:
        return TFT_GREEN;
    }
}

long QuakeWidget::nowEpochUtc() {
    // GlobalTime keeps local time; the feed and the model are UTC, so the ages
    // are computed from the raw system clock, not the shifted epoch.
    return (long)time(nullptr);
}

void QuakeWidget::update(bool force) {
    const unsigned long wait = stale::nextWaitMs(m_quakeDelay, stale::limitMs(m_quakeDelay), m_misses);
    if (force || m_quakeDelayPrev == 0 || (millis() - m_quakeDelayPrev) >= wait) {
        setBusy(true);
        bool ok = fetchQuakes();
        setBusy(false);
        m_quakeDelayPrev = millis();
        m_agePrev = millis();
        m_misses = ok ? 0 : (m_misses < 255 ? m_misses + 1 : 255);
        m_fresh.attempt(m_quakeDelayPrev, ok, m_quakeDelay);
        DeviceLog::page("quakes", m_logWatch, ok, m_fresh.stale());
    } else if (m_everFetched && (millis() - m_agePrev) >= 60000) {
        // The feed is unchanged, but a minute has passed: the ages are redrawn
        m_agePrev = millis();
        for (int8_t i = 0; i < m_quakeCount; i++) {
            m_quakes[i].setChangedStatus(true);
        }
    }
}

void QuakeWidget::middleTap() {
    update(true);
}

bool QuakeWidget::fetchQuakes() {
    HTTPClient http;
    http.begin(quake::feedUrl(m_minMagnitude));
    // The feed carries a day of events; the filter adds the useful JSON fields
    http.useHTTP10(true);
    int httpCode = http.GET();
    bool applied = false;

    if (httpCode > 0) {
        JsonDocument doc;
        // Only the fields the page uses; the rest of each feature is skipped
        JsonDocument filter;
        filter["features"][0]["id"] = true;
        filter["features"][0]["properties"]["mag"] = true;
        filter["features"][0]["properties"]["place"] = true;
        filter["features"][0]["properties"]["time"] = true;
        filter["features"][0]["properties"]["tsunami"] = true;
        filter["features"][0]["properties"]["status"] = true;
        filter["features"][0]["geometry"]["coordinates"] = true;
        DeserializationError error = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));

        if (!error) {
            // Keep events at or above the minimum, newest first, up to five
            QuakeDataModel incoming[MAX_QUAKES];
            int8_t count = 0;
            for (JsonObject feature : doc["features"].as<JsonArray>()) {
                JsonObject props = feature["properties"];
                if (props["mag"].isNull()) {
                    continue;
                }
                float mag = props["mag"].as<float>();
                if (mag < m_minMagnitude) {
                    continue;
                }
                QuakeDataModel q;
                q.setId(feature["id"].as<String>());
                q.setMagnitude(mag);
                q.setPlace(props["place"].as<String>());
                q.setEventEpoch((long)(props["time"].as<long long>() / 1000)); // ms -> s
                q.setTsunami(props["tsunami"].as<int>() != 0);
                q.setProvisional(props["status"].as<String>() != "reviewed");
                JsonArray coords = feature["geometry"]["coordinates"];
                if (coords.size() >= 3) {
                    q.setDepthKm(coords[2].as<float>()); // lon, lat, depth
                }
                // Insert by event time, newest first
                int8_t pos = count;
                while (pos > 0 && incoming[pos - 1].getEventEpoch() < q.getEventEpoch()) {
                    if (pos < MAX_QUAKES) {
                        incoming[pos] = incoming[pos - 1];
                    }
                    pos--;
                }
                if (pos < MAX_QUAKES) {
                    incoming[pos] = q;
                    if (count < MAX_QUAKES) {
                        count++;
                    }
                }
            }
            // Mark an orb changed when its event is new or its magnitude moved
            // (USGS revises magnitudes); redraw all on the first good fetch
            for (int8_t i = 0; i < count; i++) {
                bool changed = !m_everFetched || i >= m_quakeCount || m_quakes[i].getId() != incoming[i].getId() ||
                               m_quakes[i].getMagnitude() != incoming[i].getMagnitude();
                incoming[i].setChangedStatus(changed);
                m_quakes[i] = incoming[i];
            }
            if (count < m_quakeCount) {
                // Fewer events than before: the freed orbs are cleared
                for (int8_t i = count; i < m_quakeCount; i++) {
                    m_quakes[i] = QuakeDataModel();
                    m_quakes[i].setChangedStatus(true);
                }
            }
            m_quakeCount = count;
            m_everFetched = true;
            applied = true;
        } else {
            Serial.println("quake deserializeJson() failed");
        }
    } else {
        Serial.printf("quake HTTP request failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
    return applied;
}

void QuakeWidget::draw(bool force) {
    m_manager.setFont(DEFAULT_FONT);
    if (!m_everFetched) {
        if (force) {
            for (int8_t i = 0; i < NUM_SCREENS; i++) {
                displayEmpty(i);
            }
        }
        return;
    }
    for (int8_t i = 0; i < NUM_SCREENS; i++) {
        if (i < m_quakeCount) {
            if (m_quakes[i].isChanged() || force) {
                displayQuake(i, m_quakes[i]);
                m_quakes[i].setChangedStatus(false);
            }
        } else if (m_quakes[i].isChanged() || force) {
            displayEmpty(i);
            m_quakes[i].setChangedStatus(false);
        }
    }
}

void QuakeWidget::displayEmpty(int8_t displayIndex) {
    m_manager.selectScreen(displayIndex);
    m_manager.fillScreen(TFT_BLACK);
    m_manager.setFontColor(TFT_WHITE, TFT_BLACK);
    if (!m_everFetched) {
        m_manager.drawString("Earthquakes", 120, 100, 22, Align::MiddleCenter);
        m_manager.setFontColor(stale::kAmber, TFT_BLACK);
        m_manager.drawString("Loading", 120, 135, 18, Align::MiddleCenter);
        m_manager.setFontColor(TFT_WHITE, TFT_BLACK);
        m_manager.drawString(nodata::reason(GlobalTime::getInstance()->syncCurrent()), 120, 160, 16, Align::MiddleCenter);
    } else {
        m_manager.drawString("No quakes", 120, 110, 20, Align::MiddleCenter);
        m_manager.drawString("over M" + String(m_minMagnitude, 1), 120, 140, 16, Align::MiddleCenter);
    }
}

void QuakeWidget::displayQuake(int8_t displayIndex, QuakeDataModel &quake) {
    m_manager.selectScreen(displayIndex);
    m_manager.fillScreen(TFT_BLACK);

    const int center = 120;
    const uint32_t ring = bandColor(quake::bandFor(quake.getMagnitude()));
    m_manager.drawArc(center, center, 120, 112, 0, 360, ring, TFT_BLACK);

    // Large magnitude
    m_manager.setFontColor(ring, TFT_BLACK);
    m_manager.drawString(String(quake::magText(quake.getMagnitude()).c_str()), center, 88, 48, Align::MiddleCenter);

    // Short location, cut to fit the orb
    m_manager.setFontColor(TFT_WHITE, TFT_BLACK);
    String place = String(quake::shortPlace(quake.getPlace().c_str()).c_str());
    m_manager.drawFittedString(place.length() ? place : String("UNKNOWN"), center, 135, 200, 26, Align::MiddleCenter);

    // Age since the event
    m_manager.setFontColor(TFT_LIGHTGREY, TFT_BLACK);
    m_manager.drawString(String(quake::ageText(quake.getEventEpoch(), nowEpochUtc()).c_str()), center, 165, 20, Align::MiddleCenter);

    // Depth, and the provisional / tsunami flags
    m_manager.setFontColor(TFT_DARKGREY, TFT_BLACK);
    m_manager.drawString(String(quake.getDepthKm(), 0) + " km", center, 190, 15, Align::MiddleCenter);
    if (quake.getTsunami()) {
        m_manager.setFontColor(TFT_RED, TFT_BLACK);
        m_manager.drawString("TSUNAMI", center, 210, 16, Align::MiddleCenter);
    } else if (quake.getProvisional()) {
        m_manager.setFontColor(stale::kAmber, TFT_BLACK);
        m_manager.drawString("provisional", center, 210, 14, Align::MiddleCenter);
    }
}

String QuakeWidget::getName() {
    return "Quake";
}
