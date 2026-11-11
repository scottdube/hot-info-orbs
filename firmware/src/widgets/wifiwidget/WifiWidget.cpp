#include "WifiWidget.h"
#include "RoamPolicy.h"
#include "Utils.h"
#include <WiFi.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

const int lineHeight = 40;
const int statusScreenIndex = 3;
const int fontSize = 19;
const int messageDelay = 5000;
const int maxLineWidth = 200; // clears the round edge on every line the portal screens use
// While the setup portal waits for a saved network that wasn't heard: first
// look after a minute, then every two (the "Will retry." on orb 5)
const unsigned long lookFirstMs = 60000;
const unsigned long lookEveryMs = 120000;

WifiWidget::WifiWidget(ScreenManager &manager) : Widget(manager) {}

WifiWidget::~WifiWidget() {}

WiFiManager wifimgr;

// The station's last disconnect reason (wifi_err_reason_t), from the event task
static volatile uint8_t s_lastDisconnectReason = 0;
static_assert(wififail::reason::AuthExpire == WIFI_REASON_AUTH_EXPIRE &&
                  wififail::reason::AssocLeave == WIFI_REASON_ASSOC_LEAVE &&
                  wififail::reason::FourWayHandshakeTimeout == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT &&
                  wififail::reason::NoApFound == WIFI_REASON_NO_AP_FOUND &&
                  wififail::reason::AuthFail == WIFI_REASON_AUTH_FAIL &&
                  wififail::reason::HandshakeTimeout == WIFI_REASON_HANDSHAKE_TIMEOUT,
              "WifiFailPolicy.h reason codes drifted from the SDK's wifi_err_reason_t");
// Password refusals in a row while connect() runs (issue #20)
static volatile bool s_connecting = false;
static volatile int s_refusals = 0;

void WifiWidget::setup() {
    begin(true);
    if (!connect()) {
        showPortal();
    }
}

void WifiWidget::drawConnecting() {
    m_manager.selectScreen(statusScreenIndex);
    m_manager.clearScreen();
    m_manager.drawCenterString("Connecting", ScreenCenterX, ScreenCenterY - lineHeight, fontSize);
}

void WifiWidget::begin(bool showConnecting) {
    m_manager.setFont(DEFAULT_FONT);
    m_manager.setFontColor(TFT_WHITE);
    if (showConnecting) {
        drawConnecting();
    }

    // DHCP hostname, so the router knows the orb by the same name as mDNS does.
    // Must come before WiFi.mode(): that is where the core applies it.
    static bool s_reasonHooked = false;
    if (!s_reasonHooked) {
        s_reasonHooked = true;
        WiFi.onEvent(
            [](arduino_event_id_t, arduino_event_info_t info) {
                const uint8_t reason = info.wifi_sta_disconnected.reason;
                s_lastDisconnectReason = reason;
                Serial.printf("[%lu] WiFi disconnected, reason %u\n", millis(), reason);
                if (!s_connecting) {
                    return;
                }
                s_refusals = wififail::passwordRefused(reason) ? s_refusals + 1 : 0;
                if (s_refusals == wififail::kRefusalsForWrongPassword) {
                    // Same password, same answer: no more attempts. WiFiManager
                    // reads the count on each pass of its retry loop.
                    wifimgr.setConnectRetries(1);
                    Serial.println("WiFi: password refused, no more attempts");
                }
            },
            ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    }
    WiFi.setHostname(OTA_HOSTNAME);
    WiFi.mode(WIFI_STA); // For WiFiManager explicitly set mode to station, ESP defaults to STA+AP
    // The default fast scan joins the first AP it hears, not the strongest
    WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
    WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);

#if (defined WIFI_SSID && defined WIFI_PASS)
    m_hardCodedWiFi = true;
    WiFi.begin(WIFI_SSID, WIFI_PASS);
#endif

    // Remove unwanted buttons from the config portal
    std::vector<const char *> wm_menu = {"wifi"}; // buttons: wifi, info, exit, update
    // Remove unwanted buttons from the Info page
    wifimgr.setShowInfoUpdate(false);
    wifimgr.setShowInfoErase(false);
    wifimgr.setMenu(wm_menu);

    // Hold right button when connecting to power to reset wifi settings
    // these are stored by the ESP WiFi library
    if (digitalRead(BUTTON_RIGHT) == Button::PRESSED_LEVEL) {
        wifimgr.resetSettings();
        m_manager.selectScreen(statusScreenIndex);
        m_manager.drawCenterString("Wifi Settings reset", ScreenCenterX, ScreenCenterY + lineHeight, fontSize);
        delay(messageDelay);
    }

    // Set WiFiManager to non-blocking so status and info can be displayed
    wifimgr.setConfigPortalBlocking(false);

    // If you want the config portal to only be available for so many seconds
    // wm.setConfigPortalTimeout(60);

    // Add the last 2 digits of the MAC address onto the end of the config portal SSID
    // so each Info-Orbs has a unique SSID
    m_apssid = "Info-Orbs_" + WiFi.macAddress().substring(15);

    wifimgr.setCleanConnect(true);
    // Give up on the saved network in under a minute (issue #20). Unset, each
    // attempt waited ESP32's 60 s default: a wrong password took 5 minutes on
    // the rig before the portal came up (2026-11-11). A working router joins in
    // a few seconds; a network not heard keeps being scanned for while the
    // portal runs, so fewer, shorter attempts lose nothing.
    wifimgr.setConnectTimeout(15); // seconds per attempt
    wifimgr.setConnectRetries(3);
}

bool WifiWidget::connect() {
    // WiFiManager automatically connects using saved credentials...
    s_refusals = 0;
    s_connecting = true;
    const bool ok = wifimgr.autoConnect(m_apssid.c_str());
    s_connecting = false;
    if (ok) {
        Serial.print("WifiManager connected.");
        return true;
    }
    // ...if that fails (no saved credentials), it has started an access point
    // with a WiFi setup portal at 192.168.4.1
    return false;
}

void WifiWidget::drawLines(const std::vector<wififail::Line> &lines, int centerY) {
    int y = centerY - lineHeight * (int(lines.size()) - 1) / 2;
    for (const wififail::Line &line : lines) {
        const String text = line.text.c_str();
        switch (line.kind) {
        case wififail::Kind::Network: {
            // A network name can be 32 characters: shrink a long one to the orb's width
            m_manager.setFontColor(TFT_SKYBLUE);
            int w, h;
            m_manager.measureText(text, fontSize, w, h);
            if (w > maxLineWidth) {
                m_manager.drawFittedString(text, ScreenCenterX, y, maxLineWidth, lineHeight - 8, Align::MiddleCenter);
            } else {
                m_manager.drawCenterString(text, ScreenCenterX, y, fontSize);
            }
            break;
        }
        case wififail::Kind::Address:
            m_manager.setFontColor(TFT_GREENYELLOW);
            m_manager.drawCenterString(text, ScreenCenterX, y, fontSize);
            break;
        default:
            m_manager.setFontColor(TFT_WHITE);
            m_manager.drawCenterString(text, ScreenCenterX, y, fontSize);
            break;
        }
        y += lineHeight;
    }
    m_manager.setFontColor(TFT_WHITE);
}

void WifiWidget::showPortal(bool eyesShown) {
    m_configPortalRunning = true;
    m_portalOverEyes = eyesShown;
    // Why the saved network failed, recorded by autoConnect() (issue #20)
    const bool saved = wifimgr.getWiFiIsSaved();
    m_savedSsid = saved ? wifimgr.getWiFiSSID(true) : String("");
    const int result = s_refusals >= wififail::kRefusalsForWrongPassword ? wififail::wl::WrongPassword
                                                                          : wifimgr.getLastConxResult();
    m_failCase = wififail::classify(saved, result);
    Serial.printf("[%lu] Configuration portal running. Saved network \"%s\", last result %d, reason %u, case %d\n",
                  millis(), m_savedSsid.c_str(), result, s_lastDisconnectReason, (int)m_failCase);
    const wififail::Screens text = wififail::screens(m_failCase, m_savedSsid.c_str(), m_apssid.c_str());

    m_manager.setFont(DEFAULT_FONT);
    m_headlineScreen = eyesShown ? 0 : statusScreenIndex;
    m_manager.selectScreen(m_headlineScreen);
    m_manager.clearScreen();
    int headlineY = ScreenCenterY;
    if (text.cross) {
        // An X above the words: the failure reads without color
        const int c = ScreenCenterY - 72, r = 16;
        for (int w = -2; w <= 2; w++) {
            m_manager.drawLine(ScreenCenterX - r + w, c - r, ScreenCenterX + r + w, c + r, TFT_RED);
            m_manager.drawLine(ScreenCenterX + r + w, c - r, ScreenCenterX - r + w, c + r, TFT_RED);
        }
        headlineY += 12;
    }
    drawLines(text.headline, headlineY);

    m_manager.selectScreen(statusScreenIndex + 1);
    m_manager.clearScreen(); // the boot eyes put the version text here
    drawLines(text.steps, ScreenCenterY);

    if (wififail::keepsLooking(m_failCase)) {
        m_nextLook = millis() + lookFirstMs;
    }
}

// The setup portal turns the station off, so WiFiManager never tries the saved
// network again: an orb started while the router was off waited in setup until
// someone power-cycled it. While nobody is on the portal, scan for the saved
// network now and then, and restart when it is heard; autoConnect() then joins.
void WifiWidget::lookForSavedNetwork() {
    if (!m_configPortalRunning || !wififail::keepsLooking(m_failCase) || m_savedSsid.isEmpty()) {
        return;
    }
    if (m_lookScanning) {
        const int n = WiFi.scanComplete();
        if (n == WIFI_SCAN_RUNNING) {
            return;
        }
        m_lookScanning = false;
        bool heard = false;
        for (int i = 0; i < n; i++) {
            heard = heard || WiFi.SSID(i) == m_savedSsid;
        }
        WiFi.scanDelete();
        Serial.printf("Setup: %s %s\n", m_savedSsid.c_str(), heard ? "heard, restarting to join it" : "still not heard");
        if (heard && WiFi.softAPgetStationNum() == 0) {
            delay(100);
            ESP.restart();
        }
        return;
    }
    if ((long)(millis() - m_nextLook) < 0) {
        return;
    }
    m_nextLook = millis() + lookEveryMs;
    // A scan hops channels and would drop a phone part-way through setup
    if (WiFi.softAPgetStationNum() > 0) {
        return;
    }
    if (WiFi.scanNetworks(true) == WIFI_SCAN_RUNNING) {
        m_lookScanning = true;
    }
}

void WifiWidget::update(bool force) {
    // Force is currently unhandled due to not knowing what behavior it would change

    // If WiFiManager is non-blocking, this keeps the configuration portal running
    wifimgr.process();
    lookForSavedNetwork();

    if (WiFi.status() == WL_CONNECTED) {
        // Joined from the setup portal over the crossed eyes: start again, so
        // the start-up runs as on any boot (waking eyes on orbs 2 and 4 while
        // the data loads). Carrying on left orb 2 frozen crossed and put
        // "Loading" on orb 4 (rig, 2026-11-11). The network is saved by now.
        if (m_configPortalRunning && m_portalOverEyes) {
            Serial.println("Joined from the setup portal, restarting");
            delay(1000); // let the portal's reply reach the phone
            ESP.restart();
        }
        if (wifimgr.getConfigPortalActive()) {
            wifimgr.stopConfigPortal(); // frees port 80 for the OTA update page
        }
        m_isConnected = true;
        m_connectionString = "Connected";
        m_ipaddress = WiFi.localIP().toString();
        Serial.print("IP address: ");
        Serial.println(m_ipaddress);
    } else {
        m_connectionTimer += 500;
        m_dotsString += " . ";
        Serial.print(".");
        if (m_dotsString.length() > 9) {
            m_dotsString = "";
        }
        if (m_connectionTimer > m_connectionTimeout && !m_configPortalRunning) {
            m_connectionFailed = true;
            connectionTimedOut();
        }
    }
}

// First look 20 s after connecting (so a restart lands on the right AP), then
// every 2 minutes. Scans only while the signal is weak.
const unsigned long roamFirstCheckMs = 20000;
const unsigned long roamCheckMs = 120000;
// If the chosen AP hasn't taken us within this long, rejoin by name only
const unsigned long roamJoinTimeoutMs = 20000;

void WifiWidget::roamIfWeak() {
    const unsigned long now = millis();

    if (m_roamJoinStarted) {
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("Roam: now on %s, %d dBm\n", WiFi.BSSIDstr().c_str(), WiFi.RSSI());
            m_roamJoinStarted = 0;
        } else if (now - m_roamJoinStarted > roamJoinTimeoutMs) {
            Serial.println("Roam: chosen AP didn't take us, rejoining any AP");
            WiFi.begin(m_roamSsid.c_str(), WiFi.psk().c_str());
            m_roamJoinStarted = 0;
        }
        return;
    }

    if (m_roamScanning) {
        const int n = WiFi.scanComplete();
        if (n == WIFI_SCAN_RUNNING) {
            return;
        }
        m_roamScanning = false;
        if (n < 0) {
            return; // scan failed; try again at the next check
        }
        std::vector<ApSeen> seen;
        for (int i = 0; i < n; i++) {
            ApSeen ap{WiFi.SSID(i).c_str(), {}, (int)WiFi.RSSI(i), (int)WiFi.channel(i)};
            memcpy(ap.bssid, WiFi.BSSID(i), 6);
            seen.push_back(ap);
        }
        WiFi.scanDelete();
        if (WiFi.status() != WL_CONNECTED) {
            return;
        }
        uint8_t current[6];
        memcpy(current, WiFi.BSSID(), 6);
        const int rssi = WiFi.RSSI();
        const String ssid = WiFi.SSID();
        const int pick = pickRoamTarget(ssid.c_str(), current, rssi, seen);
        if (pick < 0) {
            Serial.printf("Roam: staying on %s, %d dBm; no AP %d dB better\n", WiFi.BSSIDstr().c_str(), rssi, ROAM_MARGIN_DB);
            return;
        }
        Serial.printf("Roam: %d dBm here, moving to an AP at %d dBm on channel %d\n", rssi, seen[pick].rssi, seen[pick].channel);
        m_roamSsid = ssid;
        WiFi.begin(ssid.c_str(), WiFi.psk().c_str(), seen[pick].channel, seen[pick].bssid);
        m_roamJoinStarted = now ? now : 1;
        return;
    }

    if (m_nextRoamCheck == 0) {
        m_nextRoamCheck = now + roamFirstCheckMs;
    }
    if ((long)(now - m_nextRoamCheck) < 0) {
        return;
    }
    m_nextRoamCheck = now + roamCheckMs;
    if (WiFi.status() != WL_CONNECTED || WiFi.RSSI() >= ROAM_SCAN_BELOW_DBM) {
        return;
    }
    if (WiFi.scanNetworks(true) == WIFI_SCAN_RUNNING) {
        m_roamScanning = true;
    }
}

void WifiWidget::draw(bool force) {
    // Force is currently unhandled due to not knowing what behavior it would change
    m_manager.selectScreen(statusScreenIndex);
    const int blankRectTop = ScreenCenterY + lineHeight / 2;

    if (!m_isConnected && m_configPortalRunning) {
        // The portal screens stand as drawn; orb 4 may hold a crossed eye
    } else if (!m_isConnected && !m_connectionFailed) {
        m_manager.fillRect(0, blankRectTop, ScreenWidth, ScreenHeight - blankRectTop, TFT_BLACK);
        m_manager.drawCenterString(m_dotsString, ScreenCenterX, ScreenCenterY + lineHeight, fontSize);
    } else if (m_isConnected && !m_hasDisplayedSuccess) {
        m_hasDisplayedSuccess = true;
        if (m_configPortalRunning && m_headlineScreen != statusScreenIndex) {
            m_manager.selectScreen(m_headlineScreen); // "WiFi not found" is no longer true
            m_manager.clearScreen();
            m_manager.selectScreen(statusScreenIndex);
        }
        if (!m_eyesRan) { // with the boot eyes, orb 4 keeps the awake eye
            m_manager.clearScreen();
            m_manager.drawCenterString("Success", ScreenCenterX, ScreenCenterY, fontSize);
        }
        m_manager.selectScreen(statusScreenIndex + 1);
        m_manager.clearScreen();
        m_manager.drawCenterString("IP Address", ScreenCenterX, ScreenCenterY - lineHeight, fontSize);
        m_manager.drawCenterString(m_ipaddress, ScreenCenterX, ScreenCenterY + lineHeight, fontSize);
        Serial.println();
        Serial.println("Connected to WiFi");
        m_isConnected = true;
        delay(messageDelay);
    } else if (m_connectionFailed && !m_hasDisplayedError) {
        m_hasDisplayedError = true;
        m_manager.fillRect(0, blankRectTop, ScreenWidth, ScreenHeight - blankRectTop, TFT_BLACK);
        m_manager.drawCenterString(m_connectionString, ScreenCenterX, ScreenCenterY + lineHeight, fontSize);
        delay(messageDelay);
    }
}

void WifiWidget::buttonPressed(uint8_t buttonId, ButtonState state) {
}

void WifiWidget::connectionTimedOut() {
    switch (WiFi.status()) {
    case WL_CONNECTED:
        m_connectionString = "Connected";
        break;
    case WL_NO_SSID_AVAIL:
        m_connectionString = "No SSID available";
        break;
    case WL_CONNECT_FAILED:
        m_connectionString = "Connection failed";
        break;
    case WL_IDLE_STATUS:
        m_connectionString = "Idle status";
        break;
    case WL_DISCONNECTED:
        m_connectionString = "Disconnected";
        break;
    default:
        m_connectionString = "Unknown";
        break;
    }
}

String WifiWidget::getName() {
    return "WiFi";
}