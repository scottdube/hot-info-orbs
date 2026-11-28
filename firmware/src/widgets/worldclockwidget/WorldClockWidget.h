#ifndef WORLDCLOCKWIDGET_H
#define WORLDCLOCKWIDGET_H

#include "DeviceLog.h"
#include "GlobalTime.h"
#include "StalePolicy.h"
#include "Widget.h"
#include "weatherwidget/VisualCrossingSource.h"
#include "weatherwidget/WeatherDataModel.h"

#define MAX_WORLD_CITIES 5

// One orb per city: its name, its own local time (from the location's UTC
// offset, which Visual Crossing returns with the weather) and current weather
// with the same icons the weather page uses. The cities are the "wcity" list
// in Settings; the page is registered at boot only when at least one is set.
class WorldClockWidget : public Widget {
public:
    WorldClockWidget(ScreenManager &manager);
    ~WorldClockWidget() override;
    void setup() override;
    void update(bool force = false) override;
    void draw(bool force = false) override;
    void middleTap() override;
    String getName() override;
    bool dataStale() override { return m_fresh.stale(); }
    bool fetchFailed() override { return m_fresh.lastFailed(); }
    const char *dataKind() override { return "weather"; }
    stale::LastGood lastGood() override { return m_fresh.lastGood(); }
    uint8_t dataOrbs() override { return (uint8_t)((1u << m_cityCount) - 1); } // one orb per city

    // How many cities this page actually has (0 = nothing to register)
    int8_t cityCount() const { return m_cityCount; }

private:
    void drawCity(int8_t index);
    void drawNoData(int8_t index);
    String localTimeString(int32_t tzOffsetSeconds);
    // true = the sun is up in that city now, from its sunrise/sunset (both UTC
    // epochs). When the source gave no sun times, falls back to isDark below.
    bool cityIsDay(WeatherDataModel &model);

    GlobalTime *m_time = nullptr;
    bool m_isDark = true; // the weather page's Light/Dark choice; the fallback when sun times are missing

    const unsigned long m_weatherDelay = 600000; // 10 min, like the weather page
    unsigned long m_weatherDelayPrev = 0;
    stale::Tracker m_fresh;
    devlog::PageWatch m_logWatch;

    int m_lastMinuteStamp = -1; // redraw the clocks when the minute turns

    WeatherDataModel m_models[MAX_WORLD_CITIES];
    VisualCrossingSource *m_sources[MAX_WORLD_CITIES] = {};
    String m_cityLabels[MAX_WORLD_CITIES];
    int8_t m_cityCount = 0;
};
#endif // WORLDCLOCKWIDGET_H
