#ifndef WEB_DATA_WIDGET_H
#define WEB_DATA_WIDGET_H

#include "Widget.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>

#include "Utils.h"
#include "WebDataModel.h"
#include "WebDataPolicy.h"

// The last fetch, for the settings page's status line (no serial on the SuperMini)
struct WebDataStatus {
    bool attempted = false;
    bool ok = false;
    int code = 0; // HTTP code, or HTTPClient's negative error
    uint32_t ms = 0; // the whole fetch
    uint32_t at = 0; // millis() when it finished
    String error; // when !ok: "HTTP 404", "bad JSON (...)", "too big", "timeout", ...
};

class WebDataWidget : public Widget {
public:
    WebDataWidget(ScreenManager &manager, String url, String name);
    ~WebDataWidget() override;
    void setup() override;
    void update(bool force = false) override;
    void draw(bool force = false) override;
    void buttonPressed(uint8_t buttonId, ButtonState state) override;
    String getName() override;
    static const WebDataStatus &status();

private:
    bool fetch(); // one bounded GET; true when the data was applied
    void showProblem(const String &reason); // center orb, only before any good data

    unsigned long m_lastUpdate = 0;
    unsigned long m_updateDelay = WEBDATA_DEFAULT_INTERVAL_MS;
    String httpRequestAddress;
    String m_name;
    bool m_hasData = false;
    bool m_showingProblem = false;
    WebDataModel m_obj[5];
    int32_t m_defaultColor = TFT_WHITE;
    int32_t m_defaultBackground = TFT_BLACK;
    static WebDataStatus s_status;
};
#endif // WEB_DATA_WIDGET_H
