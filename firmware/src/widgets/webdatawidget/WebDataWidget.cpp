
#include "WebDataWidget.h"

WebDataStatus WebDataWidget::s_status;

// Hands ArduinoJson at most `limit` bytes. deserializeJson reads from a
// Stream, so the reply is never held whole in a String (upstream did
// http.getString(), unbounded). Blocking waits happen on the inner stream,
// under its own timeout; this one's is 0 so it never adds a wait of its own.
class LimitedStream : public Stream {
  public:
    LimitedStream(Stream &in, size_t limit) : m_in(in), m_left(limit) { setTimeout(0); }
    int available() override { return m_left ? m_in.available() : 0; }
    int read() override {
        if (m_left == 0) {
            m_exceeded = true;
            return -1;
        }
        char c;
        if (m_in.readBytes(&c, 1) != 1) {
            return -1;
        }
        m_left--;
        return (uint8_t)c;
    }
    int peek() override { return m_left ? m_in.peek() : -1; }
    size_t write(uint8_t) override { return 0; }
    bool exceeded() const { return m_exceeded; }

  private:
    Stream &m_in;
    size_t m_left;
    bool m_exceeded = false;
};

WebDataWidget::WebDataWidget(ScreenManager &manager, String url, String name) : Widget(manager) {
    httpRequestAddress = url;
    m_name = name;
    m_lastUpdate = 0;
    for (int i = 0; i < 5; i++) {
        m_obj[i] = WebDataModel();
    }
}

const WebDataStatus &WebDataWidget::status() {
    return s_status;
}

WebDataWidget::~WebDataWidget() {
}

void WebDataWidget::setup() {
}

void WebDataWidget::buttonPressed(uint8_t buttonId, ButtonState state) {
}

void WebDataWidget::draw(bool force) {
    for (int i = 0; i < 5; i++) {
        WebDataModel *data = &m_obj[i];
        if (force) {
            data->setInitializedStatus(false);
        }
        if (data->isChanged() || force) {
            m_manager.selectScreen(i);
            data->draw(m_manager);

            data->setChangedStatus(false);
        }
    }
}

void WebDataWidget::update(bool force) {
    if (!force && m_lastUpdate != 0 && (millis() - m_lastUpdate) < m_updateDelay) {
        return;
    }
    fetch();
    // Also after a failure: upstream left m_lastUpdate alone on error, so a
    // dead URL was retried on every pass of loop()
    m_lastUpdate = millis();
    if (m_lastUpdate == 0) {
        m_lastUpdate = 1; // 0 means "never fetched"
    }
}

bool WebDataWidget::fetch() {
    WebDataStatus st;
    st.attempted = true;
    uint32_t start = millis();
    HTTPClient http;
    http.useHTTP10(true); // no chunked encoding, so getStream() is plain JSON (as TempestSource)
    http.setConnectTimeout(WEBDATA_TIMEOUT_MS);
    http.setTimeout(WEBDATA_TIMEOUT_MS);
    http.begin(httpRequestAddress); // https without a CA check, the same path as the weather pages
    st.code = http.GET();
    if (st.code == 200) {
        int size = http.getSize(); // -1 when the server sends no Content-Length
        if (size > (int)WEBDATA_MAX_BYTES) {
            st.error = "too big";
        } else {
            LimitedStream in(http.getStream(), WEBDATA_MAX_BYTES);
            JsonDocument doc;
            DeserializationError e = deserializeJson(doc, in);
            if (in.exceeded()) {
                st.error = "too big";
            } else if (e) {
                st.error = String("bad JSON (") + e.c_str() + ")";
            } else {
                m_updateDelay = webDataInterval(doc["interval"].is<long>(), doc["interval"] | 0L);
                JsonArray array = doc["displays"].is<JsonArray>() ? doc["displays"].as<JsonArray>()
                                                                   : doc.as<JsonArray>(); // legacy: a bare array
                if (m_showingProblem) {
                    // Model colors start at -1 (white), so set them or the cleared orb repaints white
                    m_obj[2].setLabel("");
                    m_obj[2].setData("", m_defaultColor, m_defaultBackground);
                    m_obj[2].setBackgroundColor(m_defaultBackground);
                    m_obj[2].setDataColor(m_defaultColor);
                    m_obj[2].setLabelColor(m_defaultColor);
                    m_obj[2].setInitializedStatus(false); // clear the problem text
                    m_showingProblem = false;
                }
                size_t n = array.size() < 5 ? array.size() : 5; // upstream indexed past m_obj[4]
                for (size_t i = 0; i < n; i++) {
                    m_obj[i].parseData(array[i].as<JsonObject>(), m_defaultColor, m_defaultBackground);
                }
                st.ok = true;
                m_hasData = true;
            }
        }
    } else if (st.code == HTTPC_ERROR_READ_TIMEOUT) {
        st.error = "timeout";
    } else {
        st.error = st.code > 0 ? String("HTTP ") + st.code : http.errorToString(st.code);
    }
    http.end();
    st.ms = millis() - start;
    st.at = millis();
    s_status = st;
    if (!st.ok) {
        Serial.printf("%s: %s\n", m_name.c_str(), st.error.c_str());
        if (!m_hasData) {
            showProblem(st.error);
        }
    }
    return st.ok;
}

// No good data yet: the page name and the reason on the center orb, drawn
// through the normal label/data path. Once data has arrived it stays on
// screen through later failures.
void WebDataWidget::showProblem(const String &reason) {
    if (m_obj[2].getData() != reason) {
        m_obj[2].setInitializedStatus(false); // fillScreen before the new text, or it overdraws the old reason
    }
    m_obj[2].setLabel(m_name);
    m_obj[2].setData(reason, m_defaultColor, m_defaultBackground);
    // setData ignores its color arguments and the model defaults to -1 (white on white)
    m_obj[2].setBackgroundColor(m_defaultBackground);
    m_obj[2].setDataColor(m_defaultColor);
    m_obj[2].setLabelColor(m_defaultColor);
    m_showingProblem = true;
}

String WebDataWidget::getName() {
    return m_name;
}
