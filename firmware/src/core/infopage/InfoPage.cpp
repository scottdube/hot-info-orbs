#include "InfoPage.h"
#include "Utils.h"
#include "WidgetSet.h"
#include "build_id.h"
#include "config_helper.h"
#include "mqttwidget/MqttFeed.h"
#include <Arduino.h>
#include <WiFi.h>
#include <esp_system.h>
#include <esp_timer.h>

namespace {

const unsigned int kFontSize = 19; // the size the WiFi setup screens use

infopage::Start startOf(esp_reset_reason_t r) {
    switch (r) {
    case ESP_RST_POWERON:
        return infopage::Start::PowerOn;
    case ESP_RST_EXT:
        return infopage::Start::ResetPin;
    case ESP_RST_SW:
        return infopage::Start::Restart;
    case ESP_RST_PANIC:
        return infopage::Start::Crash;
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT:
        return infopage::Start::Watchdog;
    case ESP_RST_DEEPSLEEP:
        return infopage::Start::DeepSleep;
    case ESP_RST_BROWNOUT:
        return infopage::Start::Brownout;
    default:
        return infopage::Start::Unknown;
    }
}

// Words carry the meaning; the colors only repeat it (title on top, the
// network name and the addresses as on the WiFi setup screens)
uint32_t colorOf(infopage::Kind kind) {
    switch (kind) {
    case infopage::Kind::Title:
        return TFT_LIGHTGREY;
    case infopage::Kind::Network:
        return TFT_SKYBLUE;
    case infopage::Kind::Address:
        return TFT_GREENYELLOW;
    default:
        return TFT_WHITE;
    }
}

} // namespace

InfoPage::InfoPage(ScreenManager &manager, WidgetSet &pages) : m_manager(manager), m_pages(pages) {}

infopage::Press InfoPage::press(bool okHeldLong) {
    const infopage::Press result = m_session.press(okHeldLong, millis());
    if (result == infopage::Press::Opened) {
        for (infopage::Page &shown : m_shown) {
            shown.clear(); // nothing of ours is on the orbs: the first pass draws them whole
        }
    }
    return result;
}

void InfoPage::close() {
    m_session.close();
}

// The network name and the address go to the orbs only. They are never written
// to the device log, which a member may send to somebody else.
infopage::Facts InfoPage::facts(bool linkUp, const bool mqttInUse[2]) const {
    infopage::Facts f;
    f.version = FIRMWARE_VERSION;
    f.commit = BUILD_COMMIT;
    f.built = BUILD_TIME;
    f.wifiUp = linkUp;
    if (linkUp) {
        f.network = WiFi.SSID().c_str();
        f.rssi = WiFi.RSSI();
        f.channel = WiFi.channel();
        f.ip = WiFi.localIP().toString().c_str();
        f.localName = OTA_HOSTNAME ".local";
    }
    f.upS = static_cast<uint32_t>(esp_timer_get_time() / 1000000); // 64 bit: no wrap at 49 days
    f.start = startOf(esp_reset_reason());
    f.freeBytes = ESP.getFreeHeap();
    const uint32_t now = millis();
    for (int8_t i = 0; i < m_pages.count(); i++) {
        Widget *page = m_pages.page(i);
        if (!m_pages.shown(i) || !page->dataKind()) {
            continue; // a page taken out of the rotation is not refreshed: its age would only alarm
        }
        const stale::LastGood good = page->lastGood();
        f.sources.push_back({page->dataKind(), good.any, (now - good.at) / 1000});
    }
    for (int slot = 0; slot < 2; slot++) {
        const MqttStatus &st = MqttFeed::status(slot);
        f.mqtt[slot].inUse = mqttInUse[slot];
        f.mqtt[slot].connected = st.connected;
        f.mqtt[slot].hasData = st.lastValueAt != 0;
        f.mqtt[slot].dataAgeS = (now - st.lastValueAt) / 1000;
    }
    return f;
}

void InfoPage::drawLine(const infopage::Page &page, int index) {
    const infopage::Line &line = page[index];
    const int y = infopage::lineY(static_cast<int>(page.size()), index);
    const int room = infopage::lineWidthAt(y); // the panel is round
    const String text = line.text.c_str();
    m_manager.setFontColor(colorOf(line.kind), TFT_BLACK);
    int w, h;
    m_manager.measureText(text, kFontSize, w, h);
    if (w > room) {
        m_manager.drawFittedString(text, ScreenCenterX, y, room, infopage::kLinePitch - 8, Align::MiddleCenter);
    } else {
        m_manager.drawCenterString(text, ScreenCenterX, y, kFontSize);
    }
}

bool InfoPage::show(bool linkUp, const bool mqttInUse[2]) {
    const uint32_t now = millis();
    if (m_session.timedOut(now)) {
        return false;
    }
    if (!m_session.refreshDue(now)) {
        return true;
    }
    const infopage::Facts f = facts(linkUp, mqttInUse);
    m_manager.setFont(DEFAULT_FONT); // the page that was on show may have left its own
    for (int orb = 0; orb < infopage::kOrbs; orb++) {
        const infopage::Page page = infopage::page(orb, f);
        const std::vector<int> redraw = infopage::linesToRedraw(m_shown[orb], page);
        if (redraw.empty()) {
            continue;
        }
        m_manager.selectScreen(orb);
        if (redraw[0] == infopage::kWholeOrb) {
            m_manager.fillScreen(TFT_BLACK);
            for (int i = 0; i < static_cast<int>(page.size()); i++) {
                drawLine(page, i);
            }
        } else {
            for (int i : redraw) {
                // Only this line's band: clearing the whole panel every second flickers
                const int y = infopage::lineY(static_cast<int>(page.size()), i);
                m_manager.fillRect(0, y - infopage::kLinePitch / 2, infopage::kPanel, infopage::kLinePitch, TFT_BLACK);
                drawLine(page, i);
            }
        }
        m_shown[orb] = page;
    }
    m_manager.setFontColor(TFT_WHITE, TFT_BLACK);
    return true;
}
