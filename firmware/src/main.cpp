#include "Button.h"
#include "DeviceLogStore.h"
#include "GlobalTime.h"
#include "InfoPage.h"
#include "OtaUpdater.h"
#include "PageLock.h"
#include "ScreenManager.h"
#include "Settings.h"
#include "SettingsPage.h"
#include "TimeSyncWatch.h"
#include "Utils.h"
#include "WidgetSet.h"
#include "clockwidget/ClockWidget.h"
#include "config_helper.h"
#include "icons.h"
#include "mqttwidget/MQTTWidget.h"
#include "quakewidget/QuakeWidget.h"
#include "weatherwidget/AmbientSource.h"
#include "weatherwidget/StationTypes.h"
#include "weatherwidget/TempestSource.h"
#include "weatherwidget/VisualCrossingSource.h"
#include "weatherwidget/WeatherWidget.h"
#include "webdatawidget/WebDataWidget.h"
#include "wifiwidget/WifiWidget.h"
#include <Arduino.h>
#include <LittleFS.h>
#include "BootEyes.h" // ScreenGuard and end() are no-ops without BOOT_EYES
#include "LidPalette.h"

#ifdef STOCK_TICKER_LIST
    #include "stockwidget/StockWidget.h"
#endif
#ifdef PARQET_PORTFOLIO_ID
    #include "parqetwidget/ParqetWidget.h"
#endif

TFT_eSPI tft = TFT_eSPI();

unsigned long m_widgetCycleDelay = 0; // ms between automatic widget changes, 0 = off; set from Settings in setup()
unsigned long m_widgetCycleDelayPrev = 0;

Button buttonLeft(BUTTON_LEFT);
Button buttonOK(BUTTON_OK);
Button buttonRight(BUTTON_RIGHT);

GlobalTime *globalTime; // Initialize the global time
devlog::TimeSyncWatch s_timeSync; // logs a missing or overdue time sync (issue #52)
devlog::SyncKick s_syncKick; // asks again every 15 s while there is no real time
uint32_t s_syncsSeen = 0; // time syncs the pages have been told about

String connectingString{""};

WifiWidget *wifiWidget{nullptr};
OtaUpdater *otaUpdater{nullptr};
SettingsPage *settingsPage{nullptr};
InfoPage *infoPage{nullptr}; // on the orbs: hold the middle button 2 seconds
pagelock::PageLock s_pageLock; // hold left and right 2 seconds: stay on this page (#58)

int connectionTimer{0};
const int connectionTimeout{10000};
bool isConnected{true};

ScreenManager *sm;
WidgetSet *widgetSet;
MqttFeed *mqttFeeds[2]{}; // page 1's broker; page 2's own broker, if it has one

// This function should probably be moved somewhere else
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
    if (y >= tft.height())
        return 0;
    // Dim bitmap
    for (int i = 0; i < w * h; i++) {
        bitmap[i] = Utils::rgb565dim(bitmap[i], sm->getBrightness(), true);
    }
    tft.pushImage(x, y, w, h, bitmap);
    return 1;
}

/**
 * The ISR handlers must be static
 */
void isrButtonChangeLeft() { buttonLeft.isrButtonChange(); }
void isrButtonChangeMiddle() { buttonOK.isrButtonChange(); }
void isrButtonChangeRight() { buttonRight.isrButtonChange(); }

void setupButtons() {
    buttonLeft.begin();
    buttonOK.begin();
    buttonRight.begin();

    attachInterrupt(digitalPinToInterrupt(BUTTON_LEFT), isrButtonChangeLeft, CHANGE);
    attachInterrupt(digitalPinToInterrupt(BUTTON_OK), isrButtonChangeMiddle, CHANGE);
    attachInterrupt(digitalPinToInterrupt(BUTTON_RIGHT), isrButtonChangeRight, CHANGE);
}

// Name, team and version. With the boot eyes on, this moves from orb 2 to orb 5
// and gains a "Connecting" line, since orb 4 (the WiFi status orb) is an eye.
static bool s_fsMounted = false;
// The eyes are up from WiFi connect until the first page (orb 3 shows the nose)
static bool s_eyesShowing = false;

// Orb 3's start-up picture: the uploaded one if there is one and it decodes;
// the built-in logo otherwise, drawn over whatever part of a bad file got drawn
static void drawBootPicture() {
    sm->selectScreen(2);
    TJpgDec.setJpgScale(1);
    bool pictureUnreadable = false;
    if (s_fsMounted && LittleFS.exists("/boot.jpg")) {
        pictureUnreadable = TJpgDec.drawFsJpg(0, 0, "/boot.jpg", LittleFS) != JDR_OK;
    }
    if (!s_fsMounted || !LittleFS.exists("/boot.jpg") || pictureUnreadable) {
        TJpgDec.drawJpg(0, 0, logo_start, logo_end - logo_start);
    }
    SettingsPage::setBootStatus(s_fsMounted, pictureUnreadable);
}

static void drawVersionScreen(int screen, bool withConnecting) {
    const int up = withConnecting ? 20 : 0;
    sm->selectScreen(screen);
    sm->setFontColor(TFT_WHITE);
    sm->drawCenterString("HOT Info Orbs", ScreenCenterX, ScreenCenterY - 50 - up, 22);
    sm->drawCenterString("by", ScreenCenterX, ScreenCenterY - 5 - up, 22);
    sm->drawCenterString("HOT-Team", ScreenCenterX, ScreenCenterY + 30 - up, 22);
    sm->setFontColor(TFT_RED);
    sm->drawCenterString("version: " FIRMWARE_VERSION, ScreenCenterX, ScreenCenterY + 65 - up, 14);
    if (withConnecting) {
        sm->setFontColor(TFT_WHITE);
        sm->drawCenterString("Connecting", ScreenCenterX, ScreenCenterY + 85, 19);
    }
    sm->setFontColor(TFT_WHITE);
}

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println("Starting up...");

    // Before anything is constructed: ScreenManager, GlobalTime and every
    // widget read their settings once, at construction
    Settings::load();
    m_widgetCycleDelay = Settings::get().cycle * 1000UL;
#ifdef BOOT_EYES
    // The lid demo plays whatever Waking eyes says: it was asked for by name
    const bool eyesWanted = Settings::get().booteyes || Settings::get().lids == lids::kDemo;
#else
    const bool eyesWanted = false;
#endif

    // Holds the uploaded boot picture. true = format if it will not mount: a
    // new board's partition has never been formatted (a few seconds, once)
    s_fsMounted = LittleFS.begin(true);
    Serial.printf("LittleFS %s\n", s_fsMounted ? "mounted" : "FAILED - built-in logo only");
    DeviceLog::begin(s_fsMounted);

    TJpgDec.setSwapBytes(true); // JPEG rendering setup
    TJpgDec.setCallback(tft_output);
    setupButtons();

    sm = new ScreenManager(tft);
    sm->fillAllScreens(TFT_BLACK);
    sm->setFontColor(TFT_WHITE);

    sm->selectScreen(0);
    sm->drawCenterString("Welcome", ScreenCenterX, ScreenCenterY, 29);
    // Running image against one OTA slot: the figure the 92% stop line is
    // measured against, read from the image so it always matches what runs
    uint32_t used = ESP.getSketchSize(), slot = ESP.getFreeSketchSpace();
    if (slot > 0) {
        char line[32];
        snprintf(line, sizeof(line), "Flash %u%%", (unsigned)((used * 100ULL + slot / 2) / slot));
        sm->drawCenterString(line, ScreenCenterX, ScreenCenterY + 45, 22);
        unsigned u = used / 1024, t = slot / 1024; // KB; under 10,000 on any 4-16 MB part
        snprintf(line, sizeof(line), "%u,%03u / %u,%03u KB", u / 1000, u % 1000, t / 1000, t % 1000);
        sm->drawCenterString(line, ScreenCenterX, ScreenCenterY + 75, 14);
    }

    if (eyesWanted) {
        drawVersionScreen(4, true); // orbs 2 and 4 become the eyes
    } else {
        drawVersionScreen(1, false);
    }

    // With the eyes, orb 3 is the nose between them (BootEyes::start draws
    // it, in the lid style); the picture gets its 2 s once the data has
    // loaded (loop)
    if (!eyesWanted) {
        drawBootPicture();
    }

    widgetSet = new WidgetSet(sm);

#ifdef GC9A01_DRIVER
    Serial.println("GC9A01 Driver");
#endif
#if HARDWARE == WOKWI
    Serial.println("Wokwi Build");
#endif

    pinMode(BUSY_PIN, OUTPUT);
    Serial.println("Connecting to WiFi");

    wifiWidget = new WifiWidget(*sm);
    otaUpdater = new OtaUpdater(*sm);
    settingsPage = new SettingsPage(*otaUpdater);
    infoPage = new InfoPage(*sm, *widgetSet);
    // Every splash screen is drawn before the eyes start: from here until
    // BootEyes::finish the eye task is the only thing drawing
    // The forget-WiFi countdown (#60) draws on orb 3 and leaves it blank:
    // without the eyes the boot picture goes back; with them the nose does
    if (wifiWidget->begin(!eyesWanted) && !eyesWanted) {
        drawBootPicture();
    }
    bool eyesRan = false;
#ifdef BOOT_EYES
    if (eyesWanted) {
        const SettingsValues &v = Settings::get();
        eyesRan = BootEyes::start(*sm, tft, {v.lids, v.iris, v.lashes});
        if (!eyesRan) { // fall back to today's screens on orbs 2, 3 and 4
            drawBootPicture();
            drawVersionScreen(1, false);
            wifiWidget->drawConnecting();
        }
    }
#endif
    const bool wifiOk = wifiWidget->connect();
#ifdef BOOT_EYES
    if (eyesRan) {
        BootEyes::finish(wifiOk); // on failure: crossed eyes on orbs 1 and 3, the nose on orb 2
    }
#endif
    if (!wifiOk) {
        wifiWidget->showPortal(eyesRan); // what went wrong, on orbs 4 and 5
    }
    // Eyes still showing (not blanked for the portal): they keep moving on orbs
    // 2 and 4 until the first page draws (BootEyes::end() in loop), so orb 4
    // gets no "Success" and no "Loading data:". Nothing in the rest of setup()
    // draws; loop() holds a ScreenGuard for what it puts on orb 5.
    s_eyesShowing = eyesRan && wifiOk;
    wifiWidget->setEyesRan(s_eyesShowing);
    widgetSet->setLoadingOnLastOrb(s_eyesShowing);

    globalTime = GlobalTime::getInstance();

    ClockWidget *clock = new ClockWidget(*sm);
    widgetSet->add(clock);
    widgetSet->setClockPage(clock); // for "Clock only when WiFi is out"
#ifdef PARQET_PORTFOLIO_ID
    widgetSet->add(new ParqetWidget(*sm));
#endif
#ifdef STOCK_TICKER_LIST
    if (!Settings::get().tickers.empty()) {
        widgetSet->add(new StockWidget(*sm));
    }
#endif
    bool stationPages = false;
    // One weather page per station (Tempest or Ambient, issue #51), plus
    // Visual Crossing for the location whenever a VC key is set (a third
    // place, e.g. a town away from the stations). With station pages and no VC
    // key, VC is left out: it would only show a key error. A station without
    // its account keys would fail every fetch with 401, so it gets no page
    // (the settings page says why).
    const SettingsValues &wx = Settings::get();
    auto addStation = [&](int slot, int type, uint32_t id, const std::string &mac, const std::string &label) {
        const stations::Type t = stations::typeFrom(type);
        if (!stations::slotUsed(t, id, mac, label)) {
            return;
        }
        if (t == stations::Type::Tempest && !wx.tsttok.empty()) {
            widgetSet->add(new WeatherWidget(*sm, new TempestSource(slot, id, label)));
            stationPages = true;
        } else if (t == stations::Type::Ambient && !wx.awapp.empty() && !wx.awkey.empty()) {
            widgetSet->add(new WeatherWidget(*sm, new StationPage(slot, new AmbientSource(mac), label)));
            stationPages = true;
        }
    };
    addStation(0, wx.sttype1, wx.tstn1, wx.stmac1, wx.tlbl1);
    addStation(1, wx.sttype2, wx.tstn2, wx.stmac2, wx.tlbl2);
    if (!stationPages || !wx.wxkey.empty()) {
        widgetSet->add(new WeatherWidget(*sm, new VisualCrossingSource()));
    }
    // Always registered so it appears in Rotation > Show; its tick box turns
    // it on. New orbs start with it unticked (Settings::defaults hides it).
    widgetSet->add(new QuakeWidget(*sm));
    // Custom pages: set up on /settings, registered only when configured.
    // Saving restarts the orb, so this runs again with the new values.
    const SettingsValues &cp = Settings::get();
    if (!cp.wdurl.empty()) {
        widgetSet->add(new WebDataWidget(*sm, cp.wdurl.c_str(), cp.wdname.c_str()));
    }
#ifdef WEB_DATA_STOCK_WIDGET_URL
    widgetSet->add(new WebDataWidget(*sm, WEB_DATA_STOCK_WIDGET_URL, "WebData"));
#endif
    // Two MQTT pages. Page 2 shares page 1's broker connection unless it names its own.
    const MqttPlan mp = mqttPlan(cp.mqhost, cp.mqt, cp.mqsetup, cp.mq2same, cp.mq2host, cp.mq2t);
    if (mp.page1 || (mp.page2 && !mp.page2OwnBroker)) {
        mqttFeeds[0] = new MqttFeed(0, cp.mqhost, cp.mqport, cp.mquser, cp.mqpass);
    }
    if (mp.page2OwnBroker) {
        mqttFeeds[1] = new MqttFeed(1, cp.mq2host, cp.mq2port, cp.mq2user, cp.mq2pass);
    }
    MQTTWidget *mqttPages[2]{};
    if (mp.page1) {
        mqttPages[0] = new MQTTWidget(*sm, *mqttFeeds[0], 1, cp.mqname, cp.mqt, cp.mql, cp.mqu, cp.mqsetup, cp.mqfit, cp.mqsize);
        widgetSet->add(mqttPages[0]);
    }
    if (mp.page2) {
        mqttPages[1] = new MQTTWidget(*sm, *mqttFeeds[mp.page2OwnBroker ? 1 : 0], 2, cp.mq2name, cp.mq2t, cp.mq2l, cp.mq2u, "", cp.mq2fit, cp.mq2size);
        widgetSet->add(mqttPages[1]);
    }
    settingsPage->setWidgets(widgetSet);
    for (MQTTWidget *page : mqttPages) {
        if (page) {
            // After every add(): shown() depends on how many pages are shown.
            // A connection stays up only while one of its pages can come up.
            int8_t i = widgetSet->indexOf(page);
            if (i >= 0 && widgetSet->shown(i)) {
                page->feed().setWanted(true);
            }
        }
    }

    m_widgetCycleDelayPrev = millis();
}

void checkCycleWidgets() {
    if (m_widgetCycleDelay > 0 && (m_widgetCycleDelayPrev == 0 || (millis() - m_widgetCycleDelayPrev) >= m_widgetCycleDelay)) {
        widgetSet->rotate();
        m_widgetCycleDelayPrev = millis();
    }
}

// The info page is off the orbs again: the page that was on show draws itself
// whole, and gets its full turn before the rotation moves on.
void leaveInfoPage() {
    infoPage->close();
    widgetSet->setClearScreensOnDrawCurrent();
    m_widgetCycleDelayPrev = millis();
}

void checkButtons() {
    // While the panels are off, a press only wakes them; it does not also act
    if (widgetSet->panelsAsleep()) {
        bool pressed = buttonLeft.getState() != BTN_NOTHING;
        pressed = buttonOK.getState() != BTN_NOTHING || pressed;
        pressed = buttonRight.getState() != BTN_NOTHING || pressed;
        if (pressed) {
            Serial.println("Button pressed -> wake panels for a minute");
            m_widgetCycleDelayPrev = millis();
            widgetSet->wakeForAMinute();
        }
        return;
    }
    // Left and right down together for 2 seconds: lock or unlock the page
    // (issue #58, PageLock.h). Both buttons' presses in such a hold, the
    // release included, do nothing else. Works from the info page too, and
    // closes it, so the note is seen.
    const pagelock::Step lock = s_pageLock.step(millis(), buttonLeft.heldFor(0), buttonRight.heldFor(0));
    if (lock.event != pagelock::Event::None) {
        const bool locked = lock.event == pagelock::Event::Locked;
        Serial.printf("Left and right held -> page %s\n", locked ? "locked" : "unlocked");
        DeviceLog::note("page %s by the buttons", locked ? "locked" : "unlocked");
        if (infoPage->isOpen()) {
            leaveInfoPage();
        }
        widgetSet->setPageLock(locked);
        m_widgetCycleDelayPrev = millis();
    }
    if (lock.swallow) {
        buttonLeft.getState(); // read and dropped: part of the hold
        buttonRight.getState();
        return;
    }
    // The middle button down for 2 seconds: the info page, from any page, while
    // the finger is still on the button
    if (buttonOK.heldFor(Button::LONG_PRESS_TIME) && infoPage->hold()) {
        Serial.println("Middle button held -> info page");
        m_widgetCycleDelayPrev = millis();
    }
    // While the info page is open, any press closes it and does nothing else.
    // The hold that opened it being let go is not a press (Ignored).
    if (infoPage->isOpen()) {
        const ButtonState middleState = buttonOK.getState();
        bool closed = middleState != BTN_NOTHING && infoPage->press(true, middleState == BTN_HOLD) == infopage::Press::Closed;
        bool side = buttonLeft.getState() != BTN_NOTHING;
        side = buttonRight.getState() != BTN_NOTHING || side;
        if (!closed && side && infoPage->isOpen()) {
            closed = infoPage->press(false, false) == infopage::Press::Closed;
        }
        if (closed) {
            Serial.println("Button pressed -> info page closed");
            leaveInfoPage();
        }
        return;
    }
    // One job per button and gesture (issue #60, PressPolicy.h). A tap is any
    // release under 2 s; a left or right hold does nothing on its own. Every
    // button is read on every pass and each press it holds is acted on, so
    // presses made during a slow pass (a data fetch) are not dropped. A handled
    // press restarts the rotation timer.
    const ButtonState leftState = buttonLeft.getState();
    const ButtonState middleState = buttonOK.getState();
    const ButtonState rightState = buttonRight.getState();
    if (leftState == BTN_TAP) {
        Serial.println("Left tap -> previous page");
        m_widgetCycleDelayPrev = millis();
        widgetSet->prev();
    }
    if (rightState == BTN_TAP) {
        Serial.println("Right tap -> next page");
        m_widgetCycleDelayPrev = millis();
        widgetSet->next();
    }
    if (middleState != BTN_NOTHING) {
        m_widgetCycleDelayPrev = millis();
        // A 2 second hold is the info page's (opened above while it was still
        // down, or here if the loop was busy then). The page on show only
        // gets taps.
        if (infoPage->press(true, middleState == BTN_HOLD) == infopage::Press::PassOn) {
            Serial.println("Middle tap -> the page's action");
            widgetSet->middleTap();
        }
    }
}

void loop() {
    DeviceLog::tick(); // writes the events waiting; first, so every path below gets it
    Settings::tick(); // a clock face picked with the middle button, once the taps stop (#60)
    if (wifiWidget->isConnected() == false) {
        wifiWidget->update();
        {
            BootEyes::ScreenGuard guard; // the IP address on orb 5, beside the eyes
            wifiWidget->draw();
        }
        widgetSet->setClearScreensOnDrawCurrent(); // Clear screen after wifiWidget
        delay(100);
    } else {
#ifdef BOOT_EYES
        if (s_eyesShowing && BootEyes::demo()) {
            // Lid demo: the eyes stay up for good and only the web server
            // runs, so the settings page can end it. No pages, no buttons.
            otaUpdater->begin();
            settingsPage->begin();
            {
                BootEyes::ScreenGuard guard; // an OTA upload draws its progress
                otaUpdater->handle();
            }
            delay(2);
            return;
        }
#endif
        if (!widgetSet->initialUpdateDone()) {
            widgetSet->initializeAllWidgetsData();
            if (s_eyesShowing) {
                // Data loaded: the face cuts to the picture alone for 2 s,
                // then the first page (which clears all five orbs)
                BootEyes::end();
                s_eyesShowing = false;
                sm->fillAllScreens(TFT_BLACK);
                drawBootPicture();
                delay(2000);
            }
        }
        otaUpdater->begin(); // no-op after the first call
        settingsPage->begin(); // same server, so after OTA's
        otaUpdater->handle();
        if (otaUpdater->isUpdating()) {
            return; // leave the CPU and screen 2 to the upload
        }
        wifiWidget->roamIfWeak();
        wifiWidget->rejoinIfDown();
        globalTime->updateTime();
        DeviceLog::clock(globalTime->getUnixEpoch());
        {
            const std::string news =
                s_timeSync.step(millis() / 1000, globalTime->syncCount(), globalTime->lastSyncUpSeconds());
            if (!news.empty()) {
                DeviceLog::note("%s", news.c_str());
            }
        }
        if (s_syncKick.step(millis(), globalTime->syncCurrent(), wifiWidget->linkUp())) {
            globalTime->kickSync();
        }
        if (globalTime->syncCount() != s_syncsSeen) {
            s_syncsSeen = globalTime->syncCount();
            widgetSet->timeSynced(); // the internet is up: failed pages refresh now
        }

        checkButtons();
        if (s_pageLock.noteEnded(millis())) {
            widgetSet->endLockNote(); // "Locked" / "Unlocked" comes off
        }

        widgetSet->setLinkUp(wifiWidget->linkUp());
        if (!infoPage->isOpen()) {
            widgetSet->updateCurrent(); // waits: nothing else draws under the info page
        }
        for (MqttFeed *feed : mqttFeeds) {
            if (feed && feed->wanted()) {
                feed->service(); // values current when a page comes up
            }
        }
        widgetSet->updateBrightnessByTime(globalTime->timeValid() ? globalTime->getHour24() * 60 + globalTime->getMinute() : -1);
        if (infoPage->isOpen()) {
            // The info page has the orbs: the rotation waits. It closes on any
            // button (checkButtons), after a minute, or when the panels go off.
            const bool mqttInUse[2] = {mqttFeeds[0] && mqttFeeds[0]->wanted(), mqttFeeds[1] && mqttFeeds[1]->wanted()};
            if (widgetSet->panelsAsleep() || !infoPage->show(wifiWidget->linkUp(), mqttInUse)) {
                leaveInfoPage();
            }
        } else if (!widgetSet->panelsAsleep()) {
            if (widgetSet->followOutage()) {
                m_widgetCycleDelayPrev = millis(); // to the clock, or back from it
            }
            widgetSet->drawCurrent();
            checkCycleWidgets(); // a switch redraws all five screens
        }
    }
}
