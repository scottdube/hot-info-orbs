#include "WidgetSet.h"
#include "BootEyes.h"
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

String WidgetSet::label(int8_t i) {
    // Two widgets sharing a name keep the key's " 2" so their boxes differ
    String name = m_widgets[i]->getName();
    String suffix = m_keys[i].substring(name.length());
    return m_widgets[i]->showLabel() + suffix;
}

bool WidgetSet::shown(int8_t i) {
    return isShown(i);
}

Widget *WidgetSet::page(int8_t i) {
    return m_widgets[i];
}

int8_t WidgetSet::indexOf(Widget *w) {
    for (int8_t i = 0; i < m_widgetCount; i++) {
        if (m_widgets[i] == w) {
            return i;
        }
    }
    return -1;
}

void WidgetSet::drawCurrent(bool force) {
    if (m_clearScreensOnDrawCurrent) {
        m_screenManager->clearAllScreens();
        m_clearScreensOnDrawCurrent = false;
        m_widgets[m_currentWidget]->draw(true);
    } else {
        m_widgets[m_currentWidget]->draw(force);
    }
    drawStaleMark();
}

void WidgetSet::updateCurrent() {
    // WiFi came back while this page was stale: refresh now, not at its
    // next 10 or 15 minute turn. A page off screen gets it when it comes up.
    const bool owed = m_refreshOwed[m_currentWidget];
    m_refreshOwed[m_currentWidget] = false;
    m_widgets[m_currentWidget]->update(owed);
}

void WidgetSet::setLinkUp(bool up) {
    if (m_linkWatch.cameBack(up)) {
        for (int8_t i = 0; i < m_widgetCount; i++) {
            m_refreshOwed[i] = m_widgets[i]->dataStale();
        }
    }
    m_wifiOut = m_linkGrace.out(millis(), up);
}

// Issue #32. "No WiFi" on orb 2 as soon as WiFi is out, and a red ring on
// each orb holding stale data: broken when the orb has no WiFi, solid when
// WiFi is up and the data is old all the same. Drawn here and not in each
// page so every page gets the same mark and none has to leave room for it.
void WidgetSet::drawStaleMark() {
    Widget *w = getCurrent();
    const uint8_t dataOrbs = w->dataOrbs() & 0x1F;
    const stale::Mark want = stale::mark(w->dataStale(), m_wifiOut, dataOrbs != 0);
    const stale::Paint paint = m_staleOverlay.step(m_screenManager->takeFilled(), want, dataOrbs);
    if (paint.repaint) {
        m_screenManager->clearAllScreens();
        w->draw(true);
        m_screenManager->takeFilled(); // the overlay already counts these as wiped
    }
    if (!paint.orbs) {
        return;
    }
    for (int i = 0; i < NUM_SCREENS; i++) {
        if (!(paint.orbs & (1u << i))) {
            continue;
        }
        m_screenManager->selectScreen(i);
        if (stale::hasRing(want) && (dataOrbs & (1u << i))) {
            const uint32_t bg = m_screenManager->lastFill(i); // for the ring's smoothed edge
            if (want == stale::Mark::Old) {
                m_screenManager->drawArc(ScreenCenterX, ScreenCenterY, stale::kRingOuter, stale::kRingInner, 0, 360, TFT_RED, bg);
            } else {
                // The gaps are the message, so wipe the band first: the stock
                // page has its own thin ring out here, and red showed through
                // the gaps of a falling stock.
                m_screenManager->drawArc(ScreenCenterX, ScreenCenterY, stale::kRingOuter, stale::kRingInner, 0, 360, bg, bg);
                for (int d = 0; d < stale::kDashes; d++) {
                    const stale::Arc a = stale::dash(d);
                    m_screenManager->drawArc(ScreenCenterX, ScreenCenterY, stale::kRingOuter, stale::kRingInner, a.start, a.end, TFT_RED, bg);
                }
            }
        }
        if (stale::hasWords(want) && i == stale::kPlateOrb) {
            const stale::Plate plate = w->stalePlate();
            m_screenManager->drawPlate("No WiFi", ScreenCenterX, plate.y, plate.w, stale::kPlateH, stale::kPlateRadius, stale::kPlateTextSize, TFT_RED, TFT_BLACK);
        }
    }
    m_screenManager->reset();
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
    drawStaleMark();
}

void WidgetSet::showCenteredLine(int screen, const String &text) {
    m_screenManager->selectScreen(screen);
    m_screenManager->fillScreen(TFT_BLACK);
    m_screenManager->setFontColor(TFT_WHITE);
    m_screenManager->drawCenterString(text, ScreenCenterX, ScreenCenterY, 22);
}

void WidgetSet::showLoading() {
    if (m_loadingOnLastOrb) {
        return; // updateAll() draws "Loading" with each widget's name
    }
    showCenteredLine(3, "Loading data:");
}

void WidgetSet::updateAll() {
    for (int8_t i = 0; i < m_widgetCount; i++) { // upstream left i uninitialized
        if (!isShown(i)) {
            continue; // a hidden widget never fetches
        }
        Serial.printf("updating widget %s\n", m_widgets[i]->getName().c_str());
        if (m_loadingOnLastOrb) {
            BootEyes::ScreenGuard guard; // the eyes are still moving on orbs 2 and 4
            m_screenManager->selectScreen(4);
            m_screenManager->fillScreen(TFT_BLACK);
            m_screenManager->setFontColor(TFT_WHITE);
            m_screenManager->drawCenterString("Loading", ScreenCenterX, ScreenCenterY - 20, 22);
            m_screenManager->drawCenterString(m_widgets[i]->getName(), ScreenCenterX, ScreenCenterY + 20, 22);
        } else {
            showCenteredLine(4, m_widgets[i]->getName());
        }
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

void WidgetSet::updateBrightnessByTime(int minuteOfDay) {
    const SettingsValues &s = Settings::get();
    bool isDim = s.dim && sv::inTimeRange(minuteOfDay, s.dimstartm, s.dimendm);
    bool isOff = s.off && sv::inTimeRange(minuteOfDay, s.offstartm, s.offendm);

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
