# Tempest Weather Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Show one five-orb weather page per Tempest station (SLN, LRD) in the rotation, fed from the Tempest cloud API, compiled out unless `TEMPEST_TOKEN` is defined.

**Architecture:** `WeatherWidget` keeps its drawing code and gets a `WeatherSource*`. `VisualCrossingSource` is today's fetch, moved as is. `TempestSource` streams `better_forecast` through an ArduinoJson filter into the same `WeatherDataModel`. `main.cpp` adds one widget per configured station. Pure parsing and validation live in std-only headers, so `pio test -e native` covers them.

**Tech Stack:** PlatformIO espressif32 7.0.1 (Arduino core 2.0.17), ArduinoJson 7, Unity host tests.

**Spec:** `docs/superpowers/specs/2026-09-24-tempest-weather-design.md`

## Global Constraints

- Flash stays **below 92%** of a 1,966,080-byte slot. It was 87.0% before this work. Measure after every task that adds code.
- The SuperMini has **no PSRAM**. Never hold a whole Tempest reply (~96 KB) in RAM: `useHTTP10(true)` + `getStream()` + `DeserializationOption::Filter`.
- The Tempest token lives only in `firmware/config/secrets.h` (git-ignored). It never appears on the settings page, in the repo, or in any output.
- Station IDs, names and coordinates never go in the repo: it is **public**. They live in the git-ignored `config.h` and on the orb.
- With `TEMPEST_TOKEN` undefined, the build and the orb behave **exactly** as on main.
- Every settings-page state keeps a glyph as well as a color (✖/✔/⚠).
- Flash the orb over HTTP: `pio run -e ota` then `curl -F "firmware=@.pio/build/ota/firmware.bin" http://192.168.30.208/update`. Never use espota.
- Before any commit that changes the build, run the exact CI command, a bare `pio run`, plus `pio test -e native`.

## File structure

| File | Responsibility |
|---|---|
| `firmware/src/widgets/weatherwidget/TempestParse.h` (new) | std + ArduinoJson only: `tempestIcon`, `tempestFilter`, `tempestParse`, `TempestReading` |
| `firmware/src/core/settings/SettingsValidation.h` | + `sv::parseStationId`, `sv::parseStationLabel` |
| `test/test_validation/test_main.cpp` | + tests for the new validation functions |
| `test/test_tempest/test_main.cpp` (new) | icon table, filter and parse against the fixture |
| `test/fixtures/tempest_sample.json` (new) | made-up reply shaped like the real one, with a long `hourly` array |
| `platformio.ini` | native env: ArduinoJson dependency, weatherwidget include path |
| `firmware/src/widgets/weatherwidget/WeatherSource.h` (new) | the interface |
| `firmware/src/widgets/weatherwidget/VisualCrossingSource.h/.cpp` (new) | today's fetch, moved |
| `firmware/src/widgets/weatherwidget/TempestSource.h/.cpp` (new) | Tempest fetch + last-attempt status, `#ifdef TEMPEST_TOKEN` |
| `firmware/src/widgets/weatherwidget/WeatherWidget.h/.cpp` | takes a source; fetch code removed |
| `firmware/src/core/settings/Settings.h/.cpp` | + `tstn1 tlbl1 tstn2 tlbl2` |
| `firmware/src/core/settings/SettingsPage.cpp` | + "Tempest stations" fieldset with status, `#ifdef TEMPEST_TOKEN` |
| `firmware/config/config_helper.h` | + non-empty guard for `TEMPEST_TOKEN` |
| `firmware/config/config.h.template`, `secrets.h.template` | commented examples |
| `firmware/src/main.cpp` | one widget per station |
| `firmware/src/core/widget/WidgetSet.cpp` | `updateAll()` loop counter initialized |

---

### Task 1: Pure logic with host tests

**Files:**
- Create: `firmware/src/widgets/weatherwidget/TempestParse.h`, `test/test_tempest/test_main.cpp`, `test/fixtures/tempest_sample.json`
- Modify: `firmware/src/core/settings/SettingsValidation.h` (append inside `namespace sv`), `test/test_validation/test_main.cpp`, `platformio.ini` `[env:native]`

**Interfaces:**
- Produces:
  - `std::string tempestIcon(const std::string &t)`
  - `void tempestFilter(JsonDocument &filter)`
  - `bool tempestParse(JsonDocument &doc, TempestReading &out, std::string &err)`
  - `struct TempestReading { std::string conditions, icon; float temp; TempestDay days[4]; }` with `struct TempestDay { std::string icon; float high, low; }`
  - `bool sv::parseStationId(const std::string&, uint32_t&, std::string&)`
  - `bool sv::parseStationLabel(const std::string&, uint32_t stationId, std::string&, std::string&)`

- [ ] **Step 1: Native env gets ArduinoJson and the weatherwidget headers.** In `platformio.ini`, `[env:native]`:

```ini
build_flags = -std=c++17 -I firmware/src/core/settings -I firmware/src/widgets/weatherwidget
lib_deps = bblanchon/ArduinoJson@^7.0.4
```

- [ ] **Step 2: Generate the fixture** (made up; no real station). Run once from the repo root:

```bash
/usr/bin/python3 - <<'EOF'
import json
days = [dict(day_start_local=1700000000+i*86400, day_num=10+i, month_num=1, conditions=c, icon=ic,
             sunrise=0, sunset=0, air_temp_high=h, air_temp_low=l, precip_probability=10, precip_icon="chance-rain", precip_type="rain")
        for i,(c,ic,h,l) in enumerate([("Partly Cloudy","partly-cloudy-day",62.0,42.0),("Rain Possible","possibly-rainy-day",66.0,47.0),
                                      ("Rain Likely","rainy",59.0,52.0),("Thunderstorms Possible","possibly-thunderstorm-day",60.0,51.0),
                                      ("Clear","clear-day",58.0,40.0)])]
hourly = [dict(time=1700000000+i*3600, conditions="Clear", icon="clear-night", air_temperature=50.0, sea_level_pressure=30.0,
               relative_humidity=80, precip=0, precip_probability=0, wind_avg=3.0, wind_direction=200, wind_direction_cardinal="SSW",
               wind_gust=5.0, uv=0, feels_like=49.0, local_hour=i%24, local_day=10+i//24) for i in range(231)]
doc = dict(current_conditions=dict(time=1700000000, conditions="Clear", icon="clear-day", air_temperature=50.0, sea_level_pressure=30.1,
                                   station_pressure=29.9, pressure_trend="steady", relative_humidity=82, wind_avg=4.0, wind_direction=270,
                                   wind_direction_cardinal="W", wind_gust=7.0, solar_radiation=300, uv=2, brightness=40000, feels_like=49.0,
                                   dew_point=45.0, wet_bulb_temperature=47.0, delta_t=3.0, air_density=1.2, lightning_strike_count_last_1hr=0,
                                   lightning_strike_count_last_3hr=0, precip_accum_local_day=0.0, precip_probability=0, is_precip_local_day_rain_check=True),
           forecast=dict(daily=days, hourly=hourly),
           latitude=0.0, longitude=0.0, location_name="Test Station", station=dict(agl=2.0, elevation=10.0, is_station_online=True, state=1, station_id=1),
           status=dict(status_code=0, status_message="SUCCESS"), timezone="America/New_York", timezone_offset_minutes=-240,
           units=dict(units_temp="f"))
open("test/fixtures/tempest_sample.json","w").write(json.dumps(doc))
print(len(json.dumps(doc)), "bytes")
EOF
```

Expected: about 90–100 KB. That confirms the fixture exercises the filter the way the real reply does.

- [ ] **Step 3: Write the failing tests.** `test/test_tempest/test_main.cpp`:

```cpp
// Host tests for TempestParse.h:  pio test -e native
#include "TempestParse.h"
#include <fstream>
#include <unity.h>

void setUp() {}
void tearDown() {}

static void test_icons_unchanged() {
    const char *same[] = {"clear-day", "clear-night", "cloudy", "partly-cloudy-day", "partly-cloudy-night"};
    for (auto s : same) {
        TEST_ASSERT_EQUAL_STRING(s, tempestIcon(s).c_str());
    }
}

static void test_icons_rain() {
    const char *rain[] = {"rainy", "possibly-rainy-day", "possibly-rainy-night", "thunderstorm",
                          "possibly-thunderstorm-day", "possibly-thunderstorm-night"};
    for (auto s : rain) {
        TEST_ASSERT_EQUAL_STRING("rain", tempestIcon(s).c_str());
    }
}

static void test_icons_snow_fog_wind_unknown() {
    const char *snow[] = {"snow", "sleet", "possibly-snow-day", "possibly-snow-night", "possibly-sleet-day", "possibly-sleet-night"};
    for (auto s : snow) {
        TEST_ASSERT_EQUAL_STRING("snow", tempestIcon(s).c_str());
    }
    TEST_ASSERT_EQUAL_STRING("fog", tempestIcon("foggy").c_str());
    TEST_ASSERT_EQUAL_STRING("wind", tempestIcon("windy").c_str());
    TEST_ASSERT_EQUAL_STRING("cloudy", tempestIcon("something-new").c_str());
    TEST_ASSERT_EQUAL_STRING("cloudy", tempestIcon("").c_str());
}

static void test_parse_fixture_through_filter() {
    std::ifstream in("test/fixtures/tempest_sample.json");
    TEST_ASSERT_TRUE(in.good());
    JsonDocument filter;
    tempestFilter(filter);
    JsonDocument doc;
    TEST_ASSERT_TRUE(deserializeJson(doc, in, DeserializationOption::Filter(filter)) == DeserializationError::Ok);
    TEST_ASSERT_FALSE(doc["forecast"]["hourly"].is<JsonArray>()); // filtered away
    TempestReading r;
    std::string err;
    TEST_ASSERT_TRUE(tempestParse(doc, r, err));
    TEST_ASSERT_EQUAL_STRING("Clear", r.conditions.c_str());
    TEST_ASSERT_EQUAL_STRING("clear-day", r.icon.c_str());
    TEST_ASSERT_EQUAL_FLOAT(50.0f, r.temp);
    TEST_ASSERT_EQUAL_FLOAT(62.0f, r.days[0].high); // daily[0] is today
    TEST_ASSERT_EQUAL_FLOAT(42.0f, r.days[0].low);
    TEST_ASSERT_EQUAL_STRING("rain", r.days[1].icon.c_str()); // translated
    TEST_ASSERT_EQUAL_STRING("rain", r.days[3].icon.c_str());
    TEST_ASSERT_EQUAL_FLOAT(51.0f, r.days[3].low);
    // The filtered document is tiny compared with the 96 KB reply
    TEST_ASSERT_TRUE(measureJson(doc) < 2000);
}

static void test_parse_rejects_short_forecast() {
    JsonDocument doc;
    deserializeJson(doc, R"({"current_conditions":{"conditions":"Clear","icon":"clear-day","air_temperature":50},
                          "forecast":{"daily":[{"icon":"clear-day","air_temp_high":1,"air_temp_low":0}]}})");
    TempestReading r;
    std::string err;
    TEST_ASSERT_FALSE(tempestParse(doc, r, err));
    TEST_ASSERT_TRUE(err.find("days") != std::string::npos);
}

static void test_parse_rejects_missing_current() {
    JsonDocument doc;
    deserializeJson(doc, R"({"status":{"status_code":2,"status_message":"UNAUTHORIZED"}})");
    TempestReading r;
    std::string err;
    TEST_ASSERT_FALSE(tempestParse(doc, r, err));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_icons_unchanged);
    RUN_TEST(test_icons_rain);
    RUN_TEST(test_icons_snow_fog_wind_unknown);
    RUN_TEST(test_parse_fixture_through_filter);
    RUN_TEST(test_parse_rejects_short_forecast);
    RUN_TEST(test_parse_rejects_missing_current);
    return UNITY_END();
}
```

Append to `test/test_validation/test_main.cpp`, before `main`:

```cpp
static void test_station_id() {
    uint32_t id = 99;
    std::string err;
    TEST_ASSERT_TRUE(parseStationId("", id, err));
    TEST_ASSERT_EQUAL_UINT32(0, id); // blank = unused
    TEST_ASSERT_TRUE(parseStationId(" 123456 ", id, err));
    TEST_ASSERT_EQUAL_UINT32(123456, id);
    TEST_ASSERT_TRUE(parseStationId("123456789", id, err));
    TEST_ASSERT_FALSE(parseStationId("1234567890", id, err)); // 10 digits
    TEST_ASSERT_FALSE(parseStationId("12a", id, err));
    TEST_ASSERT_FALSE(parseStationId("-5", id, err));
}

static void test_station_label() {
    std::string out, err;
    TEST_ASSERT_TRUE(parseStationLabel("", 0, out, err)); // unused station, no label needed
    TEST_ASSERT_EQUAL_STRING("", out.c_str());
    TEST_ASSERT_FALSE(parseStationLabel("  ", 123, out, err)); // a station needs a label
    TEST_ASSERT_TRUE(parseStationLabel(" SLN ", 123, out, err));
    TEST_ASSERT_EQUAL_STRING("SLN", out.c_str());
    TEST_ASSERT_FALSE(parseStationLabel("TOOLONGXX", 123, out, err)); // 9 > 8
}
```

and in `main()` add `RUN_TEST(test_station_id);` and `RUN_TEST(test_station_label);`.

- [ ] **Step 4: Run them and watch them fail.** Run `pio test -e native`. Expected: a compile failure, because `TempestParse.h`, `parseStationId` and `parseStationLabel` don't exist yet.

- [ ] **Step 5: Implement.** Append to `SettingsValidation.h` inside `namespace sv`:

```cpp
// Tempest station ID: blank = unused (0), otherwise 1..9 digits
inline bool parseStationId(const std::string &in, uint32_t &out, std::string &err) {
    std::string s = trim(in);
    if (s.empty()) {
        out = 0;
        return true;
    }
    if (s.size() > 9) {
        err = "A station ID is at most 9 digits";
        return false;
    }
    for (char c : s) {
        if (!isdigit((unsigned char)c)) {
            err = "Digits only - the number in the station's tempestwx.com address";
            return false;
        }
    }
    out = (uint32_t)std::stoul(s);
    return true;
}

// Label shown on the orb in place of the town name: 1..8 characters, and
// only required when the station is in use
inline bool parseStationLabel(const std::string &in, uint32_t stationId, std::string &out, std::string &err) {
    if (trim(in).empty() && stationId == 0) {
        out = "";
        return true;
    }
    return normalizeText(in, 8, out, err);
}
```

Create `firmware/src/widgets/weatherwidget/TempestParse.h`:

```cpp
#pragma once
// Tempest (WeatherFlow) better_forecast parsing. std + ArduinoJson only, so it
// is covered by `pio test -e native` (test/test_tempest). Measured reply
// shape: docs/TEMPEST-API.md.

#include <ArduinoJson.h>
#include <string>

struct TempestDay {
    std::string icon; // already translated to the orb's icon names
    float high = 0;
    float low = 0;
};

struct TempestReading {
    std::string conditions; // "Partly Cloudy"
    std::string icon; // translated
    float temp = 0;
    TempestDay days[4]; // [0] = today, [1..3] = the forecast orb
};

// Tempest icon names to the set WeatherWidget::drawWeatherIcon knows
inline std::string tempestIcon(const std::string &t) {
    if (t == "clear-day" || t == "clear-night" || t == "cloudy" || t == "partly-cloudy-day" || t == "partly-cloudy-night") {
        return t;
    }
    if (t == "rainy" || t == "thunderstorm" || t.rfind("possibly-rainy", 0) == 0 || t.rfind("possibly-thunderstorm", 0) == 0) {
        return "rain"; // no storm icon on the orb
    }
    if (t == "snow" || t == "sleet" || t.rfind("possibly-snow", 0) == 0 || t.rfind("possibly-sleet", 0) == 0) {
        return "snow";
    }
    if (t == "foggy") {
        return "fog";
    }
    if (t == "windy") {
        return "wind";
    }
    return "cloudy";
}

// Keeps ~1 KB of a ~96 KB reply: the hourly forecast is dropped as it streams
inline void tempestFilter(JsonDocument &filter) {
    filter["current_conditions"]["conditions"] = true;
    filter["current_conditions"]["icon"] = true;
    filter["current_conditions"]["air_temperature"] = true;
    filter["forecast"]["daily"][0]["icon"] = true; // [0] in a filter applies to every element
    filter["forecast"]["daily"][0]["air_temp_high"] = true;
    filter["forecast"]["daily"][0]["air_temp_low"] = true;
}

inline bool tempestParse(JsonDocument &doc, TempestReading &out, std::string &err) {
    JsonObject cc = doc["current_conditions"];
    if (cc.isNull() || !cc["air_temperature"].is<float>()) {
        err = "no current conditions in the reply";
        return false;
    }
    JsonArray daily = doc["forecast"]["daily"];
    if (daily.isNull() || daily.size() < 4) {
        err = "fewer than 4 forecast days";
        return false;
    }
    TempestReading r;
    r.conditions = cc["conditions"] | "";
    r.icon = tempestIcon(cc["icon"] | "");
    r.temp = cc["air_temperature"].as<float>();
    for (int i = 0; i < 4; i++) {
        r.days[i].icon = tempestIcon(daily[i]["icon"] | "");
        r.days[i].high = daily[i]["air_temp_high"] | 0.0f;
        r.days[i].low = daily[i]["air_temp_low"] | 0.0f;
    }
    out = r;
    return true;
}
```

- [ ] **Step 6: Run the tests and watch them pass.** Run `pio test -e native`. Expected: both suites PASS (11 old + 2 new validation tests, 6 Tempest tests).

- [ ] **Step 7: Check the fixture holds nothing real.** Grep the fixture for the real station IDs and names (kept in the git-ignored `config.h` and private notes, never in this repo). Expected: no hits.

- [ ] **Step 8: Run the bare CI build and commit.** Run `pio run`. Expected: SUCCESS for both firmware envs, with flash unchanged (nothing firmware-side includes the new code yet).

```bash
git add platformio.ini firmware/src/widgets/weatherwidget/TempestParse.h firmware/src/core/settings/SettingsValidation.h test/
git commit   # message: why a made-up fixture (public repo), why std-only (host tests)
```

---

### Task 2: Split fetching out of WeatherWidget (no behavior change)

**Files:**
- Create: `firmware/src/widgets/weatherwidget/WeatherSource.h`, `VisualCrossingSource.h`, `VisualCrossingSource.cpp`
- Modify: `WeatherWidget.h`, `WeatherWidget.cpp`, `firmware/src/main.cpp:156`

**Interfaces:**
- Produces:
  - `class WeatherSource { virtual bool fetch(WeatherDataModel&) = 0; virtual String label() { return ""; } }`
  - `WeatherWidget(ScreenManager&, WeatherSource *source)`, which owns and deletes the source
  - `VisualCrossingSource()`

- [ ] **Step 1: Write `WeatherSource.h`.**

```cpp
#ifndef WEATHERSOURCE_H
#define WEATHERSOURCE_H

#include "WeatherDataModel.h"
#include <Arduino.h>

// Where a weather page gets its data. WeatherWidget draws; a source fetches
// and fills the model. docs/superpowers/specs/2026-09-24-tempest-weather-design.md
class WeatherSource {
  public:
    virtual ~WeatherSource() = default;
    virtual bool fetch(WeatherDataModel &model) = 0; // true = model filled
    virtual String label() { return ""; } // shown on orb 1 before the first fetch; "" = none
    virtual String name() { return "Weather"; } // WidgetSet's loading screen and log
};

#endif
```

- [ ] **Step 2: Move the Visual Crossing fetch.** Create `VisualCrossingSource.h`:

```cpp
#ifndef VISUALCROSSINGSOURCE_H
#define VISUALCROSSINGSOURCE_H

#include "WeatherSource.h"

// Visual Crossing timeline API, location and units from Settings. The body
// moved from WeatherWidget::getWeatherData() unchanged.
class VisualCrossingSource : public WeatherSource {
  public:
    VisualCrossingSource();
    bool fetch(WeatherDataModel &model) override;

  private:
    String m_url;
};

#endif
```

`VisualCrossingSource.cpp`:
- The constructor builds `m_url` exactly as `WeatherWidget`'s constructor builds `httpRequestAddress` today (`Settings::get()`, `sv::urlEncode(s.wxloc)`, `WEATHER_API_KEY`, unit group, `LOC_LANG`).
- `fetch(WeatherDataModel &model)` is the current `getWeatherData()` body verbatim, with `model.` referring to the parameter and `httpRequestAddress` renamed to `m_url`.
- Keep its includes: `Settings.h`, `SettingsValidation.h`, `config_helper.h`, `<ArduinoJson.h>`, `<HTTPClient.h>`.
- Keep the `WEATHER_LOCAION` fallback block from `WeatherWidget.h` in this file, above the constructor.

- [ ] **Step 3: Make WeatherWidget use the source.**

In `WeatherWidget.h`:
- Replace `WeatherWidget(ScreenManager &manager);` with `WeatherWidget(ScreenManager &manager, WeatherSource *source);`.
- Add `#include "WeatherSource.h"` and the member `WeatherSource *m_source;`.
- Delete `getWeatherData()`, `weatherApiKey`, `httpRequestAddress` and the `WEATHER_LOCAION` block.

In `WeatherWidget.cpp`:
- The constructor becomes:

```cpp
WeatherWidget::WeatherWidget(ScreenManager &manager, WeatherSource *source) : Widget(manager), m_source(source) {
    m_mode = MODE_HIGHS;
    String label = m_source->label();
    if (label.length() > 0) {
        model.setCityName(label); // orb 1 names the page before the first fetch
    }
}

WeatherWidget::~WeatherWidget() {
    delete m_source;
}
```

- In `update()`, replace both calls to `getWeatherData()` with `m_source->fetch(model)`.
- Delete `WeatherWidget::getWeatherData()`.
- `getName()` returns `m_source->name();`.

In `main.cpp`:
- Add `#include "weatherwidget/VisualCrossingSource.h"`.
- Change line 156 to `widgetSet->add(new WeatherWidget(*sm, new VisualCrossingSource()));`.

- [ ] **Step 4: Build, test, and compare flash.** Run `pio run` and `pio test -e native`. Expected: SUCCESS, all tests pass, flash within about ±200 bytes of 1,709,685. A virtual call adds a few bytes.

- [ ] **Step 5: Flash the orb and confirm Visual Crossing still works.** Run `pio run -e ota && curl -s -F "firmware=@.pio/build/ota/firmware.bin" http://192.168.30.208/update`. Wait 20 s, then `curl -s http://192.168.30.208/settings | grep -o "commit [^,]*"`. Expected: the new commit is shown. The Visual Crossing page can only be checked by eye, so note it for Scott's check.

- [ ] **Step 6: Commit.** The message says why the source is a pointer the widget owns: `WidgetSet` never deletes widgets, but the destructor stays correct if one ever does.

---

### Task 3: Tempest settings, source, page section and wiring

**Files:**
- Create: `firmware/src/widgets/weatherwidget/TempestSource.h`, `TempestSource.cpp`
- Modify: `Settings.h/.cpp`, `SettingsPage.cpp`, `config_helper.h`, `config.h.template`, `secrets.h.template`, `main.cpp`, `WidgetSet.cpp:79`

**Interfaces:**
- Consumes: `tempestFilter`, `tempestParse`, `TempestReading`, `sv::parseStationId`, `sv::parseStationLabel` (Task 1); `WeatherSource` (Task 2)
- Produces:
  - `SettingsValues::tstn1, tstn2` (`uint32_t`) and `tlbl1, tlbl2` (`std::string`)
  - `TempestSource(int slot, uint32_t stationId, const std::string &label)`
  - `struct TempestStatus { bool attempted; bool ok; int code; uint32_t ms; int hour, minute; uint32_t freeHeap; String error; }`
  - `static const TempestStatus &TempestSource::status(int slot)`

- [ ] **Step 1: Add the guard and the templates.**
  - `config_helper.h`, beside the other `static_assert`s:

```cpp
    #if defined(__cplusplus) && defined(TEMPEST_TOKEN)
static_assert(sizeof(TEMPEST_TOKEN) > 1, "TEMPEST_TOKEN is empty in firmware/config/secrets.h - delete the line or paste a token from tempestwx.com > Settings > Data Authorizations");
    #endif
```

  - `secrets.h.template`, after `TIMEZONE_API_KEY`:

```c
// OPTIONAL: WeatherFlow Tempest station owners. With this set, each station
// in config.h gets its own weather page instead of the Visual Crossing one.
// Token: tempestwx.com > Settings > Data Authorizations > Create Token.
//#define TEMPEST_TOKEN ""
```

  - `config.h.template`, under WEATHER CONFIGURATION:

```c
// Tempest stations (only used when TEMPEST_TOKEN is set in secrets.h). The ID
// is the number in the station's tempestwx.com address; the label replaces the
// town name on the page (8 characters at most). Changeable on /settings.
//#define TEMPEST_STATION_1 123456
//#define TEMPEST_LABEL_1 "HOME"
//#define TEMPEST_STATION_2 0
//#define TEMPEST_LABEL_2 ""
```

- [ ] **Step 2: Settings fields.**
  - `Settings.h`, in `SettingsValues`, after `invert`:

```cpp
    uint32_t tstn1; // Tempest station ID, 0 = unused
    std::string tlbl1; // its label on the page, "" when unused
    uint32_t tstn2;
    std::string tlbl2;
```

  - `Settings.cpp`, `defaults()`, before `return d;`:

```cpp
#ifdef TEMPEST_STATION_1
    d.tstn1 = TEMPEST_STATION_1;
#else
    d.tstn1 = 0;
#endif
#ifdef TEMPEST_LABEL_1
    d.tlbl1 = TEMPEST_LABEL_1;
#else
    d.tlbl1 = "";
#endif
#ifdef TEMPEST_STATION_2
    d.tstn2 = TEMPEST_STATION_2;
#else
    d.tstn2 = 0;
#endif
#ifdef TEMPEST_LABEL_2
    d.tlbl2 = TEMPEST_LABEL_2;
#else
    d.tlbl2 = "";
#endif
```

  - `load()`, after `invert`:

```cpp
            v.tstn1 = p.getUInt("tstn1", v.tstn1);
            v.tlbl1 = p.getString("tlbl1", v.tlbl1.c_str()).c_str();
            v.tstn2 = p.getUInt("tstn2", v.tstn2);
            v.tlbl2 = p.getString("tlbl2", v.tlbl2.c_str()).c_str();
```

  - `save()`, before the `schema` line:

```cpp
              p.putUInt("tstn1", v.tstn1) &&
              p.putString("tlbl1", v.tlbl1.c_str()) == v.tlbl1.size() &&
              p.putUInt("tstn2", v.tstn2) &&
              p.putString("tlbl2", v.tlbl2.c_str()) == v.tlbl2.size() &&
```

- [ ] **Step 3: Write TempestSource.** `TempestSource.h`:

```cpp
#ifndef TEMPESTSOURCE_H
#define TEMPESTSOURCE_H

#include "config_helper.h"
#ifdef TEMPEST_TOKEN

    #include "WeatherSource.h"
    #include <string>

// What the settings page shows per station: there is no serial on the
// SuperMini, so this is how a failed fetch is seen without the orbs
struct TempestStatus {
    bool attempted = false;
    bool ok = false;
    int code = 0; // HTTP code, or HTTPClient's negative error
    uint32_t ms = 0; // duration of the whole fetch
    int hour = 0;
    int minute = 0;
    uint32_t freeHeap = 0; // after the fetch
    String error; // parse/HTTP error text when !ok
};

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
```

`TempestSource.cpp`:

```cpp
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
    http.useHTTP10(true); // no chunked encoding, so the stream is plain JSON
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
```

- [ ] **Step 4: Wire it up.** In `main.cpp`, add `#include "weatherwidget/TempestSource.h"` and replace the single weather line with:

```cpp
    bool tempestPages = false;
#ifdef TEMPEST_TOKEN
    const SettingsValues &sv = Settings::get();
    if (sv.tstn1) {
        widgetSet->add(new WeatherWidget(*sm, new TempestSource(0, sv.tstn1, sv.tlbl1)));
        tempestPages = true;
    }
    if (sv.tstn2) {
        widgetSet->add(new WeatherWidget(*sm, new TempestSource(1, sv.tstn2, sv.tlbl2)));
        tempestPages = true;
    }
#endif
    if (!tempestPages) {
        widgetSet->add(new WeatherWidget(*sm, new VisualCrossingSource()));
    }
```

In `WidgetSet.cpp`, `updateAll()`: change `for (int8_t i; i < m_widgetCount; i++)` to `for (int8_t i = 0; i < m_widgetCount; i++)`. The counter was never initialized, so the start-up pass that loads every page's data only worked if the stack happened to hold a zero.

- [ ] **Step 5: Add the settings page section.** In `SettingsPage.cpp`, add `#include "TempestSource.h"` at the top, then add a helper above `renderForm`:

```cpp
#ifdef TEMPEST_TOKEN
static String tempestStatusLine(int slot) {
    const TempestStatus &st = TempestSource::status(slot);
    if (!st.attempted) {
        return hint("&#9888; Not fetched yet since start-up (a page fetches when it is first shown).");
    }
    char when[6];
    snprintf(when, sizeof(when), "%02d:%02d", st.hour, st.minute);
    String line = String(st.ok ? "&#10004; " : "&#10006; ") + when + ", " + String(st.ms / 1000.0, 1) + " s, heap " +
                  String(st.freeHeap / 1024) + " KB";
    if (!st.ok) {
        line += " &mdash; " + esc(st.error.c_str());
    }
    return "<p class='" + String(st.ok ? "hint" : "err") + "'>" + line + "</p>";
}

static String stationRow(int slot, const char *idKey, const char *lblKey, uint32_t id, const std::string &lbl, uint32_t defId,
                         const std::string &defLbl, const std::map<std::string, std::string> &errors) {
    String n = String(slot + 1);
    String h = "<label for='" + String(idKey) + "'>Station " + n + " ID</label><input type='text' inputmode='numeric' id='" +
               idKey + "' name='" + idKey + "' value='" + (id ? String(id) : String("")) + "'>";
    h += hint("Blank = no page. Default: " + (defId ? String(defId) : String("none"))) + errorLine(errors, idKey);
    h += textField(lblKey, ("Station " + n + " label (8 characters)").c_str(), lbl, defLbl, "", errors);
    if (id) {
        h += tempestStatusLine(slot);
    }
    return h;
}
#endif
```

In `renderForm`, after the Weather fieldset:

```cpp
#ifdef TEMPEST_TOKEN
    h += "<fieldset><legend>Tempest stations</legend>";
    h += hint("Token set in secrets.h. With a station set, each one gets its own weather page and the location above is not used.");
    h += stationRow(0, "tstn1", "tlbl1", v.tstn1, v.tlbl1, d.tstn1, d.tlbl1, errors);
    h += stationRow(1, "tstn2", "tlbl2", v.tstn2, v.tlbl2, d.tstn2, d.tlbl2, errors);
    h += "</fieldset>";
#endif
```

In `handlePost`, before the dim equal-hours check:

```cpp
#ifdef TEMPEST_TOKEN
    if (has("tstn1") && !sv::parseStationId(argStr(s, "tstn1"), v.tstn1, err)) {
        errors["tstn1"] = err;
    }
    if (has("tstn2") && !sv::parseStationId(argStr(s, "tstn2"), v.tstn2, err)) {
        errors["tstn2"] = err;
    }
    if (has("tlbl1") && !sv::parseStationLabel(argStr(s, "tlbl1"), v.tstn1, v.tlbl1, err)) {
        errors["tlbl1"] = err;
    }
    if (has("tlbl2") && !sv::parseStationLabel(argStr(s, "tlbl2"), v.tstn2, v.tlbl2, err)) {
        errors["tlbl2"] = err;
    }
#endif
```

`textField` takes `const char *label`; the `.c_str()` of a temporary is valid for the duration of the call.

- [ ] **Step 6: Build without a token, then compare against main.** Scott's local `secrets.h` has no `TEMPEST_TOKEN` yet. Run `pio run` and `pio test -e native`. Expected: SUCCESS, flash within about ±300 bytes of Task 2's figure. That's the compiled-out proof: record both numbers.

- [ ] **Step 7: Set up Scott's local files and build with the token.** Both files are git-ignored.
  - Add `#define TEMPEST_TOKEN "<value>"` to `firmware/config/secrets.h`. The value comes from `/private/tmp/claude-501/-Users-scottdube-code/02bc8833-f4c7-4cc8-8417-be780123c14e/scratchpad/tempest_token`, written by a script that never prints it.
  - Add the SLN and LRD station defaults to `firmware/config/config.h`, with the IDs from memory `info-orbs-project.md` and labels `"SLN"` and `"LRD"`.
  - Run `git status --short` and expect nothing from `firmware/config/`.
  - Run `pio run -e ota` and record the flash %. **Stop if it is ≥ 92%.**

- [ ] **Step 8: Commit** (templates, source and page, never the two local files). The message records both flash numbers and why the token stays off the page (R1: a key on the page makes a password mandatory).

---

### Task 4: On the orb, measured, then the docs

**Files:**
- Modify: `docs/TEMPEST-API.md`, `docs/SETUP.md`, `README.md`, `docs/REQUIREMENTS.md`, `docs/TRAPS.md` (only if something surprising turns up)

- [ ] **Step 1: Flash the token build over HTTP.**

```bash
~/.platformio/penv/bin/pio run -e ota && curl -s -F "firmware=@.pio/build/ota/firmware.bin" http://192.168.30.208/update
```

Wait 20 s. Then `curl -s http://192.168.30.208/settings`. Expected: the new commit, and a "Tempest stations" section showing both IDs and labels. The token value must not appear anywhere in the page. Check with a grep that reads the token file and prints only a count.

- [ ] **Step 2: Measure both fetches.** A page fetches when the rotation shows it, and the start-up pass fetches every page once. After 2 minutes, re-read `/settings`. Expected: both rows show "✔", with duration and free heap. Record duration, heap and flash.
  - If either row shows ✖, read its error and fix it before going on. `HTTP 401` means the token; a `JSON:` error means the stream (check `useHTTP10`).

- [ ] **Step 3: Check the settings page validation on the orb.** With `curl -X POST` against `/settings`:
  - `tstn1=abc` → 400, "Digits only", nothing saved.
  - `tstn1=` plus `tlbl1=` → accepted, which means station 1 is removed. **Don't send this against Scott's orb**: it restarts the orb and drops SLN. Only check the 400 case, which saves nothing.

- [ ] **Step 4: Update the docs.**
  - `TEMPEST-API.md`: add the measured on-orb fetch time, heap and flash %.
  - `SETUP.md`: add an optional step, "Tempest stations": token in `secrets.h`, then IDs and labels in `config.h` or on `/settings`.
  - `README.md`: in the changes table, add a row for Tempest pages (branch `tempest`).
  - `REQUIREMENTS.md`: under R1, note that Tempest station IDs and labels are on the page and the token deliberately is not.

- [ ] **Step 5: Commit, push `tempest`, and confirm CI is green.** Run `gh run list -R scottdube/hot-info-orbs -b tempest -L 2`.

- [ ] **Step 6: Report to Scott.** Include:
  - flash %, fetch time, heap, and both ✔ lines;
  - what only his eye can check: SLN and LRD pages in the rotation, the labels, the icons, whether the clock orb stalls while a page fetches, and that the Visual Crossing page no longer appears;
  - that merging `tempest` into main is his call.
