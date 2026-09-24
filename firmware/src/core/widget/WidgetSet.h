#ifndef WIDGET_SET_H
#define WIDGET_SET_H

#include "ScreenManager.h"
#include "Utils.h"
#include "Widget.h"

#define MAX_WIDGETS 5

class WidgetSet {
public:
    WidgetSet(ScreenManager *sm);
    void add(Widget *widget);
    void drawCurrent(bool force = false);
    void updateCurrent();
    Widget *getCurrent();
    void next();
    void prev();
    void buttonPressed(uint8_t buttonId, ButtonState state);
    void showLoading();
    void updateAll();
    bool initialUpdateDone();
    void initializeAllWidgetsData();
    void setClearScreensOnDrawCurrent();
    void updateBrightnessByTime(uint8_t hour24);
    bool panelsAsleep();
    void wakeForAMinute(); // a button press during night-off hours

    // For the settings page's Show checkboxes. A widget's key is its name,
    // with " 2" etc. added when two share one (two WebData widgets)
    int8_t count();
    String key(int8_t i);
    bool shown(int8_t i);

private:
    void showCenteredLine(int screen, const String &text);
    ScreenManager *m_screenManager;
    bool m_clearScreensOnDrawCurrent = true;
    Widget *m_widgets[MAX_WIDGETS];
    int8_t m_widgetCount = 0;
    int8_t m_currentWidget = 0;

    bool m_initialized = false;
    String m_keys[MAX_WIDGETS];
    bool m_hidden[MAX_WIDGETS] = {};
    int8_t m_shownCount = 0;
    bool isShown(int8_t i); // hidden widgets are shown anyway if none is left
    bool m_panelsAsleep = false;
    unsigned long m_wokenAt = 0;
    bool m_woken = false; // a button woke the panels; m_wokenAt says when

    void switchWidget();
};
#endif // WIDGET_SET_H