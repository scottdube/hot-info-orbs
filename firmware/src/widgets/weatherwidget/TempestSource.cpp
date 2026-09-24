#include "TempestSource.h"
#ifdef TEMPEST_TOKEN

    #include "GlobalTime.h"
    #include "Settings.h"
    #include "TempestParse.h"
    #include <HTTPClient.h>

TempestStatus TempestSource::s_status[TempestSource::SLOTS];

TempestSource::TempestSource(int slot, uint32_t stationId, const std::string &label)
    : m_slot(slot), m_station(stationId), m_label(label.c_str()) {}

const TempestStatus &TempestSource::status(int slot) {
    return s_status[slot];
}

bool TempestSource::fetch(WeatherDataModel &model) {
    TempestStatus st;
    st.attempted = true;
    uint32_t start = millis();
    String url = String("https://swd.weatherflow.com/swd/rest/better_forecast?station_id=") + m_station +
                 "&units_temp=" + (Settings::get().wxmetric ? "c" : "f") + "&token=" + TEMPEST_TOKEN;
    HTTPClient http;
    http.useHTTP10(true); // no chunked encoding, so getStream() is plain JSON
    http.begin(url);
    st.code = http.GET();
    TempestReading r;
    if (st.code == 200) {
        JsonDocument filter;
        tempestFilter(filter);
        JsonDocument doc;
        DeserializationError e = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
        std::string err;
        if (e) {
            st.error = String("JSON: ") + e.c_str();
        } else if (!tempestParse(doc, r, err)) {
            st.error = err.c_str();
        } else {
            st.ok = true;
        }
    } else {
        st.error = st.code > 0 ? String("HTTP ") + st.code : http.errorToString(st.code);
    }
    http.end();
    st.ms = millis() - start;
    GlobalTime *t = GlobalTime::getInstance();
    st.hour = t->getHour24(); // getHour() is 12 h when the clock is set to 12 h
    st.minute = t->getMinute();
    st.freeHeap = ESP.getFreeHeap();
    s_status[m_slot] = st;
    if (!st.ok) {
        Serial.printf("Tempest %s: %s\n", m_label.c_str(), st.error.c_str());
        return false; // the model keeps the previous data
    }
    model.setCityName(m_label);
    model.setCurrentText(r.conditions.c_str());
    model.setCurrentIcon(r.icon.c_str());
    model.setCurrentTemperature(r.temp);
    model.setTodayHigh(r.days[0].high);
    model.setTodayLow(r.days[0].low);
    for (int i = 0; i < 3; i++) {
        model.setDayIcon(i, r.days[i + 1].icon.c_str());
        model.setDayHigh(i, r.days[i + 1].high);
        model.setDayLow(i, r.days[i + 1].low);
    }
    return true;
}

#endif
