#pragma once

#include "Widget.h"
#include "DeviceLog.h"
#include "NewsData.h"

class NewsWidget : public Widget {
public:
    explicit NewsWidget(ScreenManager &manager);
    void setup() override {}
    void update(bool force = false) override;
    void draw(bool force = false) override;
    void middleTap() override;
    String getName() override { return "News"; }
    bool dataStale() override { return m_fresh.stale(); }
    bool fetchFailed() override { return m_fresh.lastFailed(); }
    const char *dataKind() override { return "news"; }
    stale::LastGood lastGood() override { return m_fresh.lastGood(); }

private:
    bool fetch();
    void drawOrb(int orb);
    std::vector<news::Article> m_articles;
    stale::Tracker m_fresh;
    devlog::PageWatch m_logWatch;
    uint32_t m_lastFetch = 0;
    uint32_t m_lastDrawMinute = 0;
    bool m_attempted = false;
    bool m_dirty = true;
    String m_error;
};
