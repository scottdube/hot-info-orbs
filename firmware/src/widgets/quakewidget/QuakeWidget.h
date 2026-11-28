#ifndef QUAKE_WIDGET_H
#define QUAKE_WIDGET_H

#include <TFT_eSPI.h>

#include "DeviceLog.h"
#include "QuakeDataModel.h"
#include "StalePolicy.h"
#include "Widget.h"

#define MAX_QUAKES 5

// Earthquake Monitor (docs/REQUIREMENTS §6): the five most recent earthquakes
// at or above a minimum magnitude, one per orb, with the magnitude large, a
// short location, the age, and a ring colored by magnitude band. The feed is
// the public USGS GeoJSON summary, refreshed every 60 s; no key is needed.
class QuakeWidget : public Widget {
public:
    QuakeWidget(ScreenManager &manager);
    void setup() override;
    void update(bool force = false) override;
    void draw(bool force = false) override;
    void middleTap() override;
    String getName() override;
    String showLabel() override { return "Earthquakes"; } // its Show tick box; the saved key stays "Quake"
    bool dataStale() override { return m_fresh.stale(); }
    bool fetchFailed() override { return m_fresh.lastFailed(); }
    const char *dataKind() override { return "quakes"; }
    stale::LastGood lastGood() override { return m_fresh.lastGood(); }
    uint8_t dataOrbs() override { return 0x1F; } // the page uses all five orbs

private:
    bool fetchQuakes(); // true when the feed answered with usable data
    void displayQuake(int8_t displayIndex, QuakeDataModel &quake);
    void displayEmpty(int8_t displayIndex);
    long nowEpochUtc(); // UTC seconds, for the age text

    unsigned long m_quakeDelay = 60000; // USGS updates the feed every minute
    unsigned long m_quakeDelayPrev = 0;
    unsigned long m_agePrev = 0; // ages are redrawn every minute
    stale::Tracker m_fresh;
    devlog::PageWatch m_logWatch;
    uint8_t m_misses = 0; // failed refreshes in a row

    float m_minMagnitude = 4.5f;
    QuakeDataModel m_quakes[MAX_QUAKES];
    int8_t m_quakeCount = 0;
    bool m_everFetched = false;
};

#endif // QUAKE_WIDGET_H
