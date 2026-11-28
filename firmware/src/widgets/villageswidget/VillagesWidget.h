#ifndef VILLAGES_WIDGET_H
#define VILLAGES_WIDGET_H

#include <string>
#include <vector>

#include "DeviceLog.h"
#include "StalePolicy.h"
#include "VillagesParse.h"
#include "Widget.h"

// Who is playing today at five Villages venues, one venue on each orb.
// No settings: the venues are fixed (VillagesParse.h).
class VillagesWidget : public Widget {
public:
    VillagesWidget(ScreenManager &manager);
    void setup() override;
    void update(bool force = false) override;
    void draw(bool force = false) override;
    void middleTap() override { update(true); }
    String getName() override { return "Villages"; }
    String showLabel() override { return "Villages Entertainment"; }
    bool dataStale() override { return m_fresh.stale(); }
    bool fetchFailed() override { return m_fresh.lastFailed(); }
    const char *dataKind() override { return "entertainment"; }
    stale::LastGood lastGood() override { return m_fresh.lastGood(); }
    stale::Plate stalePlate() override { return {34, 112}; }

private:
    bool fetchToken();
    bool fetchEvents(); // true when the service answered with an events list
    void drawVenue(int orb);

    static constexpr unsigned long kRefreshMs = 900000; // 15 min: acts change by the hour
    unsigned long m_prev = 0;
    stale::Tracker m_fresh;
    devlog::PageWatch m_logWatch;
    uint8_t m_misses = 0;
    bool m_changed = true;

    std::string m_token; // kept until the service refuses it
    std::vector<VillagesAct> m_acts[VILLAGES_VENUES];
};

#endif // VILLAGES_WIDGET_H
