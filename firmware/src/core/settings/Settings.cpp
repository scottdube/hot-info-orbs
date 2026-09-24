#include "Settings.h"

#include "Utils.h"
#include "clockwidget/ClockWidget.h"
#include "config_helper.h"
#include <Preferences.h>

// Same fallback as WeatherWidget.h: old config.h files spell it LOCAION
#ifndef WEATHER_LOCATION
    #define WEATHER_LOCATION WEATHER_LOCAION
#endif

static const char *NS = "orbs";
static const int SCHEMA = 1; // bump when a key changes meaning

SettingsValues Settings::s_values = Settings::defaults();
bool Settings::s_stored = false;

SettingsValues Settings::defaults() {
    SettingsValues d;
#ifdef WIDGET_CYCLE_DELAY
    d.cycle = WIDGET_CYCLE_DELAY;
#else
    d.cycle = 0;
#endif
#ifdef STOCK_TICKER_LIST
    d.tickers = STOCK_TICKER_LIST;
#else
    d.tickers = "";
#endif
    d.wxloc = WEATHER_LOCATION;
#ifdef WEATHER_UNITS_METRIC
    d.wxmetric = true;
#else
    d.wxmetric = false;
#endif
#ifdef WEATHER_SCREEN_MODE
    d.wxdark = (WEATHER_SCREEN_MODE == Dark);
#else
    d.wxdark = true; // WeatherWidget.h default
#endif
    d.tz = TIMEZONE_API_LOCATION;
    d.h24 = FORMAT_24_HOUR;
    d.ampm = SHOW_AM_PM_INDICATOR;
    d.face = (int)DEFAULT_CLOCK;
    d.clkcol = CLOCK_COLOR;
    d.shdcol = CLOCK_SHADOW_COLOR;
#if defined(DIM_START_HOUR) && defined(DIM_END_HOUR)
    d.dim = true;
    d.dimstart = DIM_START_HOUR;
    d.dimend = DIM_END_HOUR;
#else
    d.dim = false;
    d.dimstart = 22; // config.h.template's commented values
    d.dimend = 7;
#endif
    d.invert = INVERTED_ORBS;
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
    return d;
}

void Settings::load() {
    SettingsValues v = defaults();
    Preferences p;
    // Read-only begin fails when the namespace has never been written - that
    // is the normal state of an orb that never saved, not an error
    if (p.begin(NS, true)) {
        s_stored = p.isKey("schema");
        if (s_stored) {
            v.cycle = p.getInt("cycle", v.cycle);
            v.tickers = p.getString("tickers", v.tickers.c_str()).c_str();
            v.wxloc = p.getString("wxloc", v.wxloc.c_str()).c_str();
            v.wxmetric = p.getBool("wxmetric", v.wxmetric);
            v.wxdark = p.getBool("wxdark", v.wxdark);
            v.tz = p.getString("tz", v.tz.c_str()).c_str();
            v.h24 = p.getBool("h24", v.h24);
            v.ampm = p.getBool("ampm", v.ampm);
            v.face = p.getInt("face", v.face);
            v.clkcol = p.getUShort("clkcol", v.clkcol);
            v.shdcol = p.getUShort("shdcol", v.shdcol);
            v.dim = p.getBool("dim", v.dim);
            v.dimstart = p.getInt("dimstart", v.dimstart);
            v.dimend = p.getInt("dimend", v.dimend);
            v.invert = p.getBool("invert", v.invert);
#ifdef TEMPEST_TOKEN
            // Added after schema 1 without a bump: an absent key falls back
            // to the config.h default like every other key. Compiled out
            // without a token, so that build matches main.
            v.tstn1 = p.getUInt("tstn1", v.tstn1);
            v.tlbl1 = p.getString("tlbl1", v.tlbl1.c_str()).c_str();
            v.tstn2 = p.getUInt("tstn2", v.tstn2);
            v.tlbl2 = p.getString("tlbl2", v.tlbl2.c_str()).c_str();
#endif
        }
        p.end();
    }
    s_values = v;
    Serial.printf("Settings: %s\n", s_stored ? "loaded from NVS" : "config.h defaults");
}

const SettingsValues &Settings::get() {
    return s_values;
}

bool Settings::save(const SettingsValues &v) {
    Preferences p;
    if (!p.begin(NS, false)) {
        return false;
    }
    // put* returns the bytes written, 0 on failure. An empty string also
    // writes 0, so strings compare against their own length.
    bool ok = p.putInt("cycle", v.cycle) &&
              p.putString("tickers", v.tickers.c_str()) == v.tickers.size() &&
              p.putString("wxloc", v.wxloc.c_str()) == v.wxloc.size() &&
              p.putBool("wxmetric", v.wxmetric) &&
              p.putBool("wxdark", v.wxdark) &&
              p.putString("tz", v.tz.c_str()) == v.tz.size() &&
              p.putBool("h24", v.h24) &&
              p.putBool("ampm", v.ampm) &&
              p.putInt("face", v.face) &&
              p.putUShort("clkcol", v.clkcol) &&
              p.putUShort("shdcol", v.shdcol) &&
              p.putBool("dim", v.dim) &&
              p.putInt("dimstart", v.dimstart) &&
              p.putInt("dimend", v.dimend) &&
              p.putBool("invert", v.invert) &&
#ifdef TEMPEST_TOKEN
              p.putUInt("tstn1", v.tstn1) &&
              p.putString("tlbl1", v.tlbl1.c_str()) == v.tlbl1.size() &&
              p.putUInt("tstn2", v.tstn2) &&
              p.putString("tlbl2", v.tlbl2.c_str()) == v.tlbl2.size() &&
#endif
              // schema last: a save that dies part-way leaves no schema key
              // on a fresh orb, so load() keeps using config.h
              p.putInt("schema", SCHEMA);
    p.end();
    return ok;
}

bool Settings::factoryReset() {
    Preferences p;
    if (!p.begin(NS, false)) {
        return false;
    }
    bool ok = p.clear();
    p.end();
    return ok;
}

bool Settings::hasStored() {
    return s_stored;
}
