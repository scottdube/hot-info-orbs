#include "VillagesWidget.h"
#include "DeviceLogStore.h"
#include "GlobalTime.h"
#include "NoData.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>

VillagesWidget::VillagesWidget(ScreenManager &manager) : Widget(manager) {
}

void VillagesWidget::setup() {
}

// Streams the script through the scanner and stops at the token: it is 430 KB
// and the token is about a third of the way in.
bool VillagesWidget::fetchToken() {
    HTTPClient http;
    http.begin(VILLAGES_TOKEN_URL);
    http.useHTTP10(true); // no chunk framing in the stream
    http.setTimeout(10000);
    const int code = http.GET();
    bool found = false;
    if (code == HTTP_CODE_OK) {
        WiFiClient *stream = http.getStreamPtr();
        VillagesTokenScanner scanner;
        uint8_t buf[256];
        unsigned long lastData = millis();
        while (!found && (http.connected() || stream->available()) && millis() - lastData < 10000) {
            const int n = stream->readBytes(buf, std::min<size_t>(sizeof(buf), stream->available()));
            if (n <= 0) {
                delay(5);
                continue;
            }
            lastData = millis();
            for (int i = 0; i < n && !found; i++) {
                found = scanner.feed((char)buf[i]);
            }
        }
        if (found) {
            m_token = scanner.token();
        }
    } else {
        Serial.printf("Villages token fetch failed: %d\n", code);
    }
    http.end();
    return found;
}

bool VillagesWidget::fetchEvents() {
    // A refused token is fetched again once; anything else is a miss
    for (int attempt = 0; attempt < 2; attempt++) {
        if (m_token.empty() && !fetchToken()) {
            return false;
        }
        HTTPClient http;
        http.begin(VILLAGES_EVENTS_URL);
        http.useHTTP10(true);
        http.setTimeout(10000);
        http.addHeader("Authorization", m_token.c_str());
        http.addHeader("Accept", "application/json");
        http.addHeader("Origin", "https://www.thevillages.com");
        http.addHeader("Referer", "https://www.thevillages.com/calendar/");
        const int code = http.GET();
        if (code == HTTP_CODE_UNAUTHORIZED || code == HTTP_CODE_FORBIDDEN) {
            http.end();
            m_token.clear();
            continue;
        }
        bool ok = false;
        if (code == HTTP_CODE_OK) {
            JsonDocument filter;
            villagesFilter(filter);
            JsonDocument doc;
            const DeserializationError err = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
            if (!err) {
                GlobalTime *gt = GlobalTime::getInstance();
                ok = villagesParse(doc, m_acts, gt->timeValid() ? (int64_t)gt->getUnixEpoch() : 0);
            } else {
                Serial.printf("Villages JSON error: %s\n", err.c_str());
            }
        } else {
            Serial.printf("Villages events fetch failed: %d\n", code);
        }
        http.end();
        return ok;
    }
    return false;
}

void VillagesWidget::update(bool force) {
    const unsigned long wait = stale::nextWaitMs(kRefreshMs, stale::kFetchLimitMs, m_misses);
    if (force || m_prev == 0 || (millis() - m_prev) >= wait) {
        setBusy(true);
        const bool ok = fetchEvents();
        setBusy(false);
        m_prev = millis();
        m_misses = ok ? 0 : (m_misses < 255 ? m_misses + 1 : 255);
        m_fresh.attemptWithin(m_prev, ok, stale::kFetchLimitMs);
        DeviceLog::page("entertainment", m_logWatch, ok, m_fresh.stale());
        m_changed = true;
    }
}

void VillagesWidget::draw(bool force) {
    if (!m_changed && !force) {
        return;
    }
    m_manager.setFont(DEFAULT_FONT);
    for (int i = 0; i < VILLAGES_VENUES; i++) {
        drawVenue(i);
    }
    m_changed = false;
}

// A title of more than one word, long enough to need it, in two lines
static void splitTitle(const std::string &t, std::string &a, std::string &b) {
    a = t;
    b.clear();
    if (t.size() < 14) {
        return;
    }
    const size_t mid = t.size() / 2;
    size_t cut = std::string::npos, best = t.size();
    for (size_t i = 1; i + 1 < t.size(); i++) {
        if (t[i] == ' ') {
            const size_t d = i > mid ? i - mid : mid - i;
            if (d < best) {
                best = d;
                cut = i;
            }
        }
    }
    if (cut != std::string::npos) {
        a = t.substr(0, cut);
        b = t.substr(cut + 1);
    }
}

void VillagesWidget::drawVenue(int orb) {
    m_manager.selectScreen(orb);
    m_manager.fillScreen(TFT_BLACK);
    const uint16_t accent = TFT_ORANGE;
    m_manager.drawArc(120, 120, 120, 118, 0, 360, accent, accent);
    m_manager.setFontColor(accent, TFT_BLACK);
    m_manager.drawFittedString(VILLAGES_VENUE_LIST[orb].name, 120, 40, 120, 18, Align::MiddleCenter);
    m_manager.drawLine(50, 60, 190, 60, accent);

    const std::vector<VillagesAct> &acts = m_acts[orb];
    if (acts.empty()) {
        m_manager.setFontColor(TFT_WHITE, TFT_BLACK);
        if (m_fresh.lastGood().any) {
            m_manager.drawString("Nothing", 120, 112, 26, Align::MiddleCenter);
            m_manager.drawString("scheduled", 120, 145, 26, Align::MiddleCenter);
        } else {
            m_manager.drawString("No data yet", 120, 115, 24, Align::MiddleCenter);
            m_manager.setFontColor(stale::kAmber, TFT_BLACK);
            m_manager.drawString(nodata::reason(GlobalTime::getInstance()->syncCurrent()), 120, 150, 18, Align::MiddleCenter);
        }
        return;
    }

    GlobalTime *gt = GlobalTime::getInstance();
    const int n = std::min<int>(acts.size(), VILLAGES_MAX_ACTS);
    const int top = 68, span = 135; // the round edge leaves less room low down
    const int blockH = span / n;
    const int fitW = n == 1 ? 196 : n == 2 ? 180 : 160;
    for (int k = 0; k < n; k++) {
        const int mid = top + blockH * k + blockH / 2;
        std::string a, b;
        splitTitle(acts[k].title, a, b);
        const int titleH = n == 1 ? 34 : n == 2 ? 28 : 20;
        const int timeY = mid + (n == 1 ? 38 : n == 2 ? 24 : 18) + (b.empty() ? 0 : (n == 1 ? 14 : 0));
        m_manager.setFontColor(TFT_WHITE, TFT_BLACK);
        if (b.empty()) {
            m_manager.drawFittedString(a.c_str(), 120, mid - (n == 1 ? 8 : 4), fitW, titleH, Align::MiddleCenter);
        } else {
            const int dy = n == 1 ? 18 : 10;
            m_manager.drawFittedString(a.c_str(), 120, mid - dy - 6, fitW, titleH, Align::MiddleCenter);
            m_manager.drawFittedString(b.c_str(), 120, mid + dy - 6, fitW, titleH, Align::MiddleCenter);
        }
        if (acts[k].start > 0) {
            std::string when = villagesFormatTime(acts[k].start, gt->getTimeZoneOffset(), gt->getFormat24Hour());
            if (acts[k].end > 0) {
                when += " - " + villagesFormatTime(acts[k].end, gt->getTimeZoneOffset(), gt->getFormat24Hour());
            }
            m_manager.setFontColor(TFT_CYAN, TFT_BLACK);
            m_manager.drawFittedString(when.c_str(), 120, timeY, fitW - 36, n == 1 ? 18 : 14, Align::MiddleCenter);
        }
    }
}
