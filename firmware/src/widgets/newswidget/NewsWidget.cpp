#include "NewsWidget.h"

#include "DeviceLogStore.h"
#include "GlobalTime.h"
#include "LimitedStream.h"
#include "NewsText.h"
#include "Settings.h"
#include "SettingsValidation.h"
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <time.h>

namespace {
constexpr size_t kMaxResponse = 128 * 1024;

String age(const std::string &published) {
    if (!GlobalTime::getInstance()->timeValid()) return "Time unknown";
    struct tm tm = {};
    char *end = strptime(published.c_str(), "%Y-%m-%dT%H:%M:%S", &tm);
    if (end && *end == '.') {
        ++end;
        if (!isdigit(static_cast<unsigned char>(*end))) return "Age unknown";
        while (isdigit(static_cast<unsigned char>(*end))) ++end;
    }
    if (!end || (*end != 'Z' && *end != '+' && *end != '-')) return "Age unknown";
    if (tm.tm_year < 70 || tm.tm_year > 200) return "Age unknown";
    tmElements_t utc = {};
    utc.Year = tm.tm_year - 70;
    utc.Month = tm.tm_mon + 1;
    utc.Day = tm.tm_mday;
    utc.Hour = tm.tm_hour;
    utc.Minute = tm.tm_min;
    utc.Second = tm.tm_sec;
    time_t when = makeTime(utc);
    if (*end == '+' || *end == '-') {
        int h = 0, m = 0;
        if (sscanf(end + 1, "%2d:%2d", &h, &m) != 2 || h > 23 || m > 59) return "Age unknown";
        int offset = (h * 60 + m) * 60;
        when += *end == '+' ? -offset : offset;
    }
    time_t now = time(nullptr);
    if (when > now) return "Just now";
    unsigned long minutes = static_cast<unsigned long>((now - when) / 60);
    if (minutes < 60) return String(minutes) + "m ago";
    if (minutes < 1440) return String(minutes / 60) + "h ago";
    return String(minutes / 1440) + "d ago";
}
}

NewsWidget::NewsWidget(ScreenManager &manager) : Widget(manager) {}

void NewsWidget::update(bool force) {
    (void)force; // A reconnect, time sync or button must not bypass the quota interval.
    const uint32_t period = static_cast<uint32_t>(Settings::get().newsminutes) * 60000UL;
    if (m_attempted && millis() - m_lastFetch < period) return;
    setBusy(true);
    String previousError = m_error;
    bool ok = fetch();
    setBusy(false);
    if (!ok && m_error != previousError)
        DeviceLog::note("news: %s; wifi %d; heap %u KB", m_error.c_str(), static_cast<int>(WiFi.status()),
                        static_cast<unsigned>(ESP.getFreeHeap() / 1024));
    m_attempted = true;
    m_lastFetch = millis();
    m_fresh.attempt(m_lastFetch, ok, period);
    DeviceLog::page("news", m_logWatch, ok, m_fresh.stale());
    m_dirty = true;
}

void NewsWidget::middleTap() {
    m_dirty = true;
}

bool NewsWidget::fetch() {
    const SettingsValues &settings = Settings::get();
    String url = "https://newsdata.io/api/2/news?apikey=" + String(sv::urlEncode(settings.newskey).c_str()) +
                 "&language=en&size=10&sort=pubdatedesc";
    std::string categories = news::categoryQuery(settings.newscat, settings.newstop);
    if (!categories.empty()) url += "&category=" + String(categories.c_str());
    WiFiClientSecure client;
    client.setInsecure(); // Same HTTPS behavior as HTTPClient::begin(url) used here previously.
    HTTPClient http;
    http.useHTTP10(true);
    http.setConnectTimeout(10000);
    http.setTimeout(10000);
    if (!http.begin(client, url)) {
        m_error = "Connection setup";
        Serial.println("News: connection setup failed");
        return false;
    }
    int code = http.GET();
    bool ok = false;
    if (code != 200) {
        if (code == HTTPC_ERROR_CONNECTION_REFUSED) {
            char description[128] = {};
            int tlsError = client.lastError(description, sizeof(description));
            // This core leaves lastError at zero when DNS lookup fails before
            // the TLS client attempts a socket connection.
            m_error = tlsError == 0 ? "DNS lookup failed" : String("TCP/TLS ") + tlsError;
        } else {
            m_error = code > 0 ? String("HTTP ") + code : String("Net ") + code + " " + http.errorToString(code);
        }
    } else if (http.getSize() > static_cast<int>(kMaxResponse)) {
        m_error = "Reply too large";
    } else {
        JsonDocument filter;
        filter["status"] = true;
        filter["results"][0]["id"] = true;
        filter["results"][0]["title"] = true;
        filter["results"][0]["url"] = true;
        filter["results"][0]["published_at"] = true;
        filter["results"][0]["category"] = true;
        filter["results"][0]["country"] = true;
        filter["results"][0]["source"]["name"] = true;
        filter["results"][0]["source"]["priority"] = true;
        JsonDocument doc;
        LimitedStream in(http.getStream(), kMaxResponse);
        DeserializationError err = deserializeJson(doc, in, DeserializationOption::Filter(filter));
        std::vector<news::Article> articles;
        if (in.exceeded()) m_error = "Reply too large";
        else if (err) m_error = String("JSON ") + err.c_str();
        else if (std::string(doc["status"] | "") != "success") m_error = "API status error";
        else if (!news::parse(doc.as<JsonVariantConst>(), articles)) m_error = "Missing results";
        else {
            if (settings.newstop) news::rank(articles);
            m_articles.swap(articles);
            m_error = "";
            ok = true;
        }
    }
    http.end();
    if (!ok) Serial.printf("News: %s\n", m_error.c_str());
    return ok;
}

void NewsWidget::draw(bool force) {
    uint32_t minute = millis() / 60000;
    if (!force && !m_dirty && minute == m_lastDrawMinute) return;
    m_dirty = false;
    m_lastDrawMinute = minute;
    m_manager.setFont(DEFAULT_FONT);
    for (int i = 0; i < 5; i++) drawOrb(i);
}

void NewsWidget::drawOrb(int orb) {
    const SettingsValues &settings = Settings::get();
    m_manager.selectScreen(orb);
    m_manager.fillScreen(TFT_BLACK);
    m_manager.setFontColor(TFT_WHITE, TFT_BLACK);
    const news::Article *selected = settings.newstop
        ? (static_cast<size_t>(orb) < m_articles.size() ? &m_articles[orb] : nullptr)
        : news::latest(m_articles, settings.newscat[orb]);
    if (!selected) {
        const news::Category *cat = news::category(settings.newscat[orb]);
        m_manager.drawCenterString(settings.newstop ? "TOP" : (cat ? cat->label : "NEWS"), 120, 45, 18);
        m_manager.setFontColor(stale::kAmber, TFT_BLACK);
        m_manager.drawFittedString(m_error.length() && m_articles.empty() ? m_error : "No stories",
                                   120, 115, 160, 30, Align::MiddleCenter);
        return;
    }
    const news::Category *cat = news::category(settings.newscat[orb]);
    if (settings.newstop) {
        cat = nullptr;
        for (const auto &code : selected->category) {
            cat = news::category(code);
            if (cat) break;
        }
    }
    m_manager.drawCenterString(cat ? cat->label : (settings.newstop ? "TOP" : "NEWS"), 120, 45, 18);
    auto hasGlyph = [this](uint32_t cp) { return m_manager.hasGlyph(cp); };
    auto measure = [this](const std::string &text, int size) {
        TextExtent extent;
        m_manager.measureText(String(text.c_str()), size, extent.w, extent.h);
        return extent;
    };
    std::string headline = news::displayText(selected->title, hasGlyph);
    if (headline.empty()) headline = "?";
    news::HeadlineLayout layout = news::fitHeadline(headline, measure);
    int inkHeight = 0;
    for (const auto &line : layout.lines) {
        TextExtent extent = measure(line, layout.size);
        inkHeight = std::max(inkHeight, extent.h);
    }
    int blockHeight = mqttBlockHeight(layout.lines.size(), layout.size, inkHeight);
    int y = (news::kHeadlineTop + news::kHeadlineBottom - blockHeight + inkHeight) / 2;
    for (size_t i = 0; i < layout.lines.size(); i++) {
        int lineY = y + static_cast<int>(i * layout.size * MQTT_TEXT_LINE_PITCH);
        m_manager.drawString(layout.lines[i].c_str(), 120, lineY, layout.size, Align::MiddleCenter, -1, -1, false);
    }
    std::string source = news::displayText(selected->source, hasGlyph);
    if (!source.empty()) m_manager.drawFittedString(source.c_str(), 120, 174, 150, 18, Align::MiddleCenter);
    m_manager.drawCenterString(age(selected->published), 120, 197, 15);
}
