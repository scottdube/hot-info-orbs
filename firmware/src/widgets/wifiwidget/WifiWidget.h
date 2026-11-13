#pragma once
#ifndef WIFIWIDGET_H
    #define WIFIWIDGET_H

    #include "RejoinPolicy.h"
    #include "Widget.h"
    #include "WifiFailPolicy.h"

class WifiWidget : public Widget {
public:
    WifiWidget(ScreenManager &manager);
    ~WifiWidget() override;
    void setup() override;
    // setup() is begin(true) + connect() + showPortal() on failure. main.cpp
    // calls the parts itself so the boot eyes can run during connect().
    void begin(bool showConnecting);
    void drawConnecting();
    bool connect();
    // What went wrong goes on orbs 4 and 5. eyesShown: BootEyes left crossed
    // eyes on orbs 1 and 3, so joining from the portal restarts the orb
    void showPortal(bool eyesShown = false);
    void setEyesRan(bool ran) { m_eyesRan = ran; }
    void update(bool force = false) override;
    void draw(bool force = false) override;
    void buttonPressed(uint8_t buttonId, ButtonState state) override;
    String getName() override;

    bool isConnected() { return m_isConnected; }
    // The link right now. isConnected() only says start-up joined once and
    // must stay that way: false there sends loop() back to the connect screen.
    bool linkUp();
    // Call from the main loop once connected; see RoamPolicy.h for why
    void roamIfWeak();
    // Call from the main loop once connected; see RejoinPolicy.h for why
    void rejoinIfDown();
    // One line for the settings page about the last outage of 10 s or more
    // since start-up, "" when there was none
    static String lastOutageNote();

private:
    void connectionTimedOut();
    void drawLines(const std::vector<wififail::Line> &lines, int centerY);
    void lookForSavedNetwork();

    bool m_isConnected{false};
    bool m_connectionFailed{false};
    bool m_hardCodedWiFi{false};
    bool m_hasDisplayedError{false};
    bool m_hasDisplayedSuccess{false};
    bool m_configPortalRunning{false};
    bool m_portalOverEyes{false}; // the portal came up over the crossed eyes
    bool m_eyesRan{false}; // orb 4 holds an awake eye: no "Success" there

    String m_connectionString{""};
    String m_dotsString{""};
    String m_ipaddress{""};
    String m_apssid{""};
    int m_connectionTimer{0};
    const int m_connectionTimeout{10000};

    unsigned long m_nextRoamCheck{0};
    bool m_roamScanning{false};
    unsigned long m_roamJoinStarted{0};
    String m_roamSsid{""};

    rejoin::Watch m_rejoin;
    bool m_rejoinPrimed{false};

    // Portal running because the saved network wasn't heard: look for it
    wififail::Case m_failCase{wififail::Case::NeverSetUp};
    String m_savedSsid{""};
    unsigned long m_nextLook{0};
    bool m_lookScanning{false};
};

#endif // WIFIWIDGET_H