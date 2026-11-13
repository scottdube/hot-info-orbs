#ifndef WIDGET_SET_H
#define WIDGET_SET_H

#include "ScreenManager.h"
#include "StalePolicy.h"
#include "Utils.h"
#include "Widget.h"

// Clock, Parqet, Stocks, up to three weather pages (two Tempest, plus Visual
// Crossing where its tick box exists), Web Data, the second Web Data define,
// two MQTT pages = 10.
// At 5 an orb with two Tempest pages dropped the custom pages.
#define MAX_WIDGETS 10

class WidgetSet {
public:
    WidgetSet(ScreenManager *sm);
    void add(Widget *widget);
    void drawCurrent(bool force = false);
    void updateCurrent();
    // Every pass of loop(), before updateCurrent(): is WiFi up right now?
    // Picks the stale mark's shape, and gets stale pages a refresh the
    // moment WiFi is back (issue #32, StalePolicy.h)
    void setLinkUp(bool up);
    Widget *getCurrent();
    void next();
    void prev();
    void buttonPressed(uint8_t buttonId, ButtonState state);
    void showLoading();
    // true = the boot eyes are still on orbs 2 and 4: the start-up "Loading"
    // lines all go on orb 5 instead of orbs 4 and 5
    void setLoadingOnLastOrb(bool lastOrb) { m_loadingOnLastOrb = lastOrb; }
    void updateAll();
    bool initialUpdateDone();
    void initializeAllWidgetsData();
    void setClearScreensOnDrawCurrent();
    void updateBrightnessByTime(int minuteOfDay); // 0..1439
    bool panelsAsleep();
    void wakeForAMinute(); // a button press during night-off hours

    // For the settings page's Show checkboxes. A widget's key is its name,
    // with " 2" etc. added when two share one (two WebData widgets)
    int8_t count();
    String key(int8_t i);
    String label(int8_t i); // what its tick box says, e.g. "MQTT 1 (Town Squares)"
    bool shown(int8_t i);
    int8_t indexOf(Widget *w); // -1 if add() refused it

private:
    void showCenteredLine(int screen, const String &text);
    ScreenManager *m_screenManager;
    bool m_clearScreensOnDrawCurrent = true;
    Widget *m_widgets[MAX_WIDGETS];
    int8_t m_widgetCount = 0;
    int8_t m_currentWidget = 0;

    bool m_initialized = false;
    bool m_loadingOnLastOrb = false;
    String m_keys[MAX_WIDGETS];
    bool m_hidden[MAX_WIDGETS] = {};
    int8_t m_shownCount = 0;
    bool isShown(int8_t i); // hidden widgets are shown anyway if none is left
    bool m_panelsAsleep = false;
    unsigned long m_wokenAt = 0;
    bool m_woken = false; // a button woke the panels; m_wokenAt says when

    void switchWidget();

    void drawStaleMark(); // over the page, after every draw of it
    bool m_linkUp = true;
    stale::LinkWatch m_linkWatch;
    stale::Overlay m_staleOverlay;
    bool m_refreshOwed[MAX_WIDGETS] = {};
};
#endif // WIDGET_SET_H