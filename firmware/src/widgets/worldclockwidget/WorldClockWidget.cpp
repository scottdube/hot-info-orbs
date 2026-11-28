#include "WorldClockWidget.h"

#include "DeviceLogStore.h"
#include "NoData.h"
#include "Settings.h"
#include "Utils.h"
#include "config_helper.h"
#include "weatherwidget/WeatherIcon.h"
#include <TJpg_Decoder.h>
#include <TimeLib.h>

// Reads the "wcity" list from Settings and makes one Visual Crossing source
// per non-empty city. Each city keeps its own data model; the label shown on
// the orb starts as the city as typed and is replaced by the resolved address
// once the first fetch lands.
WorldClockWidget::WorldClockWidget(ScreenManager &manager) : Widget(manager) {
    const SettingsValues &s = Settings::get();
    for (int i = 0; i < 5 && m_cityCount < MAX_WORLD_CITIES; i++) {
        if (s.wcity[i].empty()) {
            continue;
        }
        String city = s.wcity[i].c_str();
        m_sources[m_cityCount] = new VisualCrossingSource(city);
        m_cityLabels[m_cityCount] = city;
        m_models[m_cityCount].setCityName(city);
        m_cityCount++;
    }
}

WorldClockWidget::~WorldClockWidget() {
    for (int8_t i = 0; i < m_cityCount; i++) {
        delete m_sources[i];
    }
}

void WorldClockWidget::setup() {
    m_time = GlobalTime::getInstance();
    m_isDark = Settings::get().wxdark; // fallback when a city has no sun times
}

void WorldClockWidget::update(bool force) {
    if (force || m_weatherDelayPrev == 0 || (millis() - m_weatherDelayPrev) >= m_weatherDelay) {
        setBusy(true);
        // One good fetch means Visual Crossing answered; a single bad city
        // (a typo) must not mark the whole page stale.
        bool any = false;
        for (int8_t i = 0; i < m_cityCount; i++) {
            bool ok = m_sources[i]->fetch(m_models[i]);
            if (force) {
                int retry = 0;
                while (!ok && retry++ < MAX_RETRIES) {
                    ok = m_sources[i]->fetch(m_models[i]);
                }
            }
            any = ok || any;
        }
        setBusy(false);
        m_weatherDelayPrev = millis();
        m_fresh.attempt(m_weatherDelayPrev, any, m_weatherDelay);
        DeviceLog::page("worldclock", m_logWatch, any, m_fresh.stale());
    }
}

void WorldClockWidget::middleTap() {
    update(true);
}

void WorldClockWidget::draw(bool force) {
    m_manager.setFont(DEFAULT_FONT);
    m_time->updateTime();

    // Redraw every city when the minute turns, so each clock keeps up without
    // a weather fetch.
    int minuteStamp = m_time->timeValid() ? (m_time->getHour24() * 60 + m_time->getMinute()) : -1;
    bool minuteTurned = minuteStamp != m_lastMinuteStamp;
    m_lastMinuteStamp = minuteStamp;

    for (int8_t i = 0; i < m_cityCount; i++) {
        if (force || minuteTurned || m_models[i].isChanged()) {
            drawCity(i);
            m_models[i].setChangedStatus(false);
        }
    }
}

// Build "hh:mm" (or "h:mm" in 12-hour mode) for a city whose offset east of
// UTC is tzOffsetSeconds. UTC comes from GlobalTime's local epoch minus the
// orb's own offset; add the city's offset to get its wall clock.
String WorldClockWidget::localTimeString(int32_t tzOffsetSeconds) {
    time_t utc = (time_t)m_time->getUnixEpoch() - m_time->getTimeZoneOffset();
    time_t cityEpoch = utc + tzOffsetSeconds;
    int h = hour(cityEpoch);
    int m = minute(cityEpoch);
    String hh;
    if (m_time->getFormat24Hour()) {
        hh = (h < 10 ? "0" : "") + String(h);
    } else {
        int h12 = h % 12;
        if (h12 == 0) {
            h12 = 12;
        }
        hh = String(h12);
    }
    String mm = (m < 10 ? "0" : "") + String(m);
    return hh + ":" + mm;
}

void WorldClockWidget::drawCity(int8_t index) {
    WeatherDataModel &model = m_models[index];
    // The resolved address, trimmed to its first part ("New York, NY, USA" ->
    // "New York"); before the first fetch it is the city as typed.
    String city = model.getCityName();
    if (city.length() == 0) {
        city = m_cityLabels[index];
    }
    int comma = city.indexOf(',');
    if (comma > 0) {
        city.remove(comma);
    }

    const bool gotData = model.hasTzOffset();
    if (!gotData) {
        drawNoData(index);
        return;
    }

    // Day in that city: white orb with black text and the dark-glyph icon art.
    // Night: black orb with white text and the light-glyph art. Each orb is
    // judged on its own sunrise/sunset, so east orbs can be dark while west
    // ones are still light.
    const bool day = cityIsDay(model);
    const uint16_t bg = day ? TFT_WHITE : TFT_BLACK;
    const uint16_t fg = day ? TFT_BLACK : TFT_WHITE;

    m_manager.selectScreen(index);
    m_manager.setBackgroundColor(bg);
    m_manager.fillScreen(bg);
    m_manager.setFontColor(fg, bg);

    // City name across the top. A narrower fit box than the orb is wide: near
    // the round top the usable width is less, and a long name ("Los Angeles")
    // ran off the curve at the old size.
    m_manager.drawFittedString(city, ScreenCenterX, 42, 150, 34, Align::MiddleCenter);

    // The city's local time, large and centered
    if (m_time->timeValid()) {
        m_manager.drawCenterString(localTimeString(model.getTzOffset()), ScreenCenterX, 96, 54);
    } else {
        m_manager.setFontColor(stale::kAmber, bg);
        m_manager.drawCenterString("--:--", ScreenCenterX, 96, 54);
        m_manager.setFontColor(fg, bg);
    }

    // Weather icon (quarter size) and the current temperature side by side.
    // The icon art matches the orb: dark glyphs on the white day orb.
    const byte *iconStart = nullptr;
    int iconSize = 0;
    if (wxicon::resolve(model.getCurrentIcon(), !day, iconStart, iconSize)) {
        m_manager.selectScreen(index);
        TJpgDec.setJpgScale(4);
        TJpgDec.drawJpg(26, 150, iconStart, iconSize);
    }
    m_manager.setFontColor(fg, bg);
    m_manager.drawString(model.getCurrentTemperature(0), 150, 172, 40, Align::MiddleCenter);
}

// Is the sun up in this city right now? Compares UTC now against the city's
// sunrise and sunset (both UTC epochs from Visual Crossing). With no sun times
// (an old fetch, or a source that gave none) it falls back to the page's
// Light/Dark setting.
bool WorldClockWidget::cityIsDay(WeatherDataModel &model) {
    int64_t sunrise = model.getSunrise();
    int64_t sunset = model.getSunset();
    if (sunrise == 0 || sunset == 0) {
        return !m_isDark;
    }
    int64_t utcNow = (int64_t)m_time->getUnixEpoch() - m_time->getTimeZoneOffset();
    return utcNow >= sunrise && utcNow < sunset;
}

// No weather for this city since power-on: its name and why, not a blank orb.
// No sun times yet, so the orb follows the page's Light/Dark setting.
void WorldClockWidget::drawNoData(int8_t index) {
    String city = m_cityLabels[index];
    int comma = city.indexOf(',');
    if (comma > 0) {
        city.remove(comma);
    }
    const uint16_t bg = m_isDark ? TFT_BLACK : TFT_WHITE;
    const uint16_t fg = m_isDark ? TFT_WHITE : TFT_BLACK;
    m_manager.selectScreen(index);
    m_manager.setBackgroundColor(bg);
    m_manager.fillScreen(bg);
    m_manager.setFontColor(fg, bg);
    m_manager.drawFittedString(city, ScreenCenterX, 50, 150, 34, Align::MiddleCenter);
    m_manager.setFontColor(stale::kAmber, bg);
    m_manager.drawCenterString("No weather", ScreenCenterX, 110, 24);
    m_manager.setFontColor(fg, bg);
    m_manager.drawCenterString(nodata::reason(m_time->syncCurrent()), ScreenCenterX, 145, 16);
}

String WorldClockWidget::getName() {
    return "World Clock";
}
