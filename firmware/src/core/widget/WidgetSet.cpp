#include "WidgetSet.h"
#include "SettingsValidation.h"
#include "Settings.h"

WidgetSet::WidgetSet(ScreenManager *sm) : m_screenManager(sm) {
}
void WidgetSet::add(Widget *widget) {
    if (m_widgetCount == MAX_WIDGETS) {
        Serial.println("MAX WIDGETS UNABLE TO ADD");
        return;
    }
    int8_t i = m_widgetCount;
    m_widgets[i] = widget;
    m_widgets[i]->setup();

    String name = widget->getName();
    int same = 1;
    for (int8_t j = 0; j < i; j++) {
        if (m_widgets[j]->getName() == name) {
            same++;
        }
    }
    m_keys[i] = same == 1 ? name : name + " " + String(same);
    m_hidden[i] = sv::listContains(Settings::get().hidden, m_keys[i].c_str());
    m_widgetCount++;
    if (!m_hidden[i]) {
        m_shownCount++;
        if (m_shownCount == 1) {
            m_currentWidget = i; // start on the first shown widget
        }
    }
}

bool WidgetSet::isShown(int8_t i) {
    return !m_hidden[i] || m_shownCount == 0;
}

int8_t WidgetSet::count() {
    return m_widgetCount;
}

String WidgetSet::key(int8_t i) {
    return m_keys[i];
}

bool WidgetSet::shown(int8_t i) {
    return isShown(i);
}

void WidgetSet::drawCurrent(bool force) {
    if (m_clearScreensOnDrawCurrent) {
        m_screenManager->clearAllScreens();
        m_clearScreensOnDrawCurrent = false;
        m_widgets[m_currentWidget]->draw(true);
    } else {
        m_widgets[m_currentWidget]->draw(force);
    }
}

void WidgetSet::updateCurrent() {
    m_widgets[m_currentWidget]->update();
}

Widget *WidgetSet::getCurrent() {
    return m_widgets[m_currentWidget];
}

void WidgetSet::buttonPressed(uint8_t buttonId, ButtonState state) {
    m_widgets[m_currentWidget]->buttonPressed(buttonId, state);
}

void WidgetSet::setClearScreensOnDrawCurrent() {
    m_clearScreensOnDrawCurrent = true;
}

// Next/previous shown widget. With only one shown, nothing changes and
// nothing is redrawn (a static display must not flicker every cycle)
void WidgetSet::next() {
    int8_t i = m_currentWidget;
    for (int8_t n = 0; n < m_widgetCount; n++) {
        i = (i + 1) % m_widgetCount;
        if (isShown(i)) {
            break;
        }
    }
    if (i != m_currentWidget) {
        m_currentWidget = i;
        switchWidget();
    }
}

void WidgetSet::prev() {
    int8_t i = m_currentWidget;
    for (int8_t n = 0; n < m_widgetCount; n++) {
        i = (i + m_widgetCount - 1) % m_widgetCount;
        if (isShown(i)) {
            break;
        }
    }
    if (i != m_currentWidget) {
        m_currentWidget = i;
        switchWidget();
    }
}

void WidgetSet::switchWidget() {
    m_screenManager->clearAllScreens();
    getCurrent()->setup();
    uint32_t start = millis();
    getCurrent()->draw(true);
    uint32_t end = millis();
    Serial.printf("Drawing of %s took %d ms\n", getCurrent()->getName().c_str(), (end - start));
}

void WidgetSet::showCenteredLine(int screen, const String &text) {
    m_screenManager->selectScreen(screen);
    m_screenManager->fillScreen(TFT_BLACK);
    m_screenManager->setFontColor(TFT_WHITE);
    m_screenManager->drawCenterString(text, ScreenCenterX, ScreenCenterY, 22);
}

void WidgetSet::showLoading() {
    showCenteredLine(3, "Loading data:");
}

void WidgetSet::updateAll() {
    for (int8_t i = 0; i < m_widgetCount; i++) { // upstream left i uninitialized
        if (!isShown(i)) {
            continue; // a hidden widget never fetches
        }
        Serial.printf("updating widget %s\n", m_widgets[i]->getName().c_str());
        showCenteredLine(4, m_widgets[i]->getName());
        m_widgets[i]->update();
    }
}

bool WidgetSet::initialUpdateDone() {
    return m_initialized;
}

void WidgetSet::initializeAllWidgetsData() {
    showLoading();
    updateAll();
    m_initialized = true;
}

void WidgetSet::updateBrightnessByTime(uint8_t hour24) {
    const SettingsValues &s = Settings::get();
    bool isDim = s.dim && sv::inHourRange(hour24, s.dimstart, s.dimend);
    bool isOff = s.off && sv::inHourRange(hour24, s.offstart, s.offend);

    if (m_woken && millis() - m_wokenAt >= 60000UL) {
        m_woken = false;
    }
    bool wantAsleep = isOff && !m_woken;
    if (wantAsleep != m_panelsAsleep) {
        m_panelsAsleep = wantAsleep;
        m_screenManager->setPanelsAsleep(wantAsleep);
        if (!wantAsleep) {
            m_screenManager->clearAllScreens();
            drawCurrent(true);
        }
    }

    // Woken during off hours: dimmed if those are also dim hours, full otherwise
    uint8_t brightness = isDim ? SETTINGS_DIM_LEVEL : TFT_BRIGHTNESS;
    if (m_screenManager->setBrightness(brightness)) {
        // brightness was changed -> update widget
        m_screenManager->clearAllScreens();
        drawCurrent(true);
    }
}

bool WidgetSet::panelsAsleep() {
    return m_panelsAsleep;
}

void WidgetSet::wakeForAMinute() {
    m_woken = true;
    m_wokenAt = millis();
    // updateBrightnessByTime() on the next loop pass does the waking
}
