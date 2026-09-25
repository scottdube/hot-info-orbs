#ifndef TEMPESTSOURCE_H
#define TEMPESTSOURCE_H

#include "config_helper.h"
#ifdef TEMPEST_TOKEN

    #include "WeatherSource.h"
    #include <string>

// A station's last fetch, for the settings page
struct TempestStatus {
    bool attempted = false;
    bool ok = false;
    int code = 0; // HTTP code, or HTTPClient's negative error
    uint32_t ms = 0; // the whole fetch, connect to parse
    uint32_t at = 0; // millis() when it finished. Not wall-clock time: the
                     // start-up fetch runs before the clock has synced
    uint32_t freeHeap = 0; // just after the fetch
    String error; // when !ok
};

// WeatherFlow Tempest better_forecast for one station. The reply is ~96 KB and
// the SuperMini has no PSRAM, so it is streamed through a filter rather than
// read into a String. docs/TEMPEST-API.md
class TempestSource : public WeatherSource {
  public:
    static const int SLOTS = 2;
    TempestSource(int slot, uint32_t stationId, const std::string &label);
    bool fetch(WeatherDataModel &model) override;
    String label() override { return m_label; }
    String name() override { return "Weather " + m_label; }
    static const TempestStatus &status(int slot);

  private:
    int m_slot;
    uint32_t m_station;
    String m_label;
    static TempestStatus s_status[SLOTS];
};

#endif
#endif
