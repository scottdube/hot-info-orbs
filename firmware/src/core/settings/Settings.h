#ifndef SETTINGS_H
#define SETTINGS_H

#include <cstdint>
#include <string>

// Settings the web page can change (docs/superpowers/specs/2026-09-23-web-settings-design.md).
// Stored in NVS, Preferences namespace "orbs". A key that was never saved falls
// back to its config.h macro, so an orb that never opened the page behaves
// exactly as the build says. Read once in setup(); a save restarts the orb, so
// nothing here changes while widgets are running.
struct SettingsValues {
    int cycle; // seconds per widget, 0 = no auto-rotate
    std::string tickers; // "SPY,QQQ" - normalised, 1..5
    std::string wxloc; // weather location as typed, NOT url-encoded
    bool wxmetric;
    bool wxdark;
    std::string tz; // IANA zone, NOT url-encoded
    bool h24;
    bool ampm;
    int face; // (int)ClockType
    uint16_t clkcol; // RGB565
    uint16_t shdcol; // RGB565
    bool dim;
    int dimstart; // hour 0..23
    int dimend; // hour 0..23, may be < dimstart (wraps midnight)
    bool off; // panels off (display-off + sleep) during offstart..offend
    int offstart; // hour 0..23
    int offend; // hour 0..23; where it overlaps the dim hours, off wins
    bool invert;
    uint32_t tstn1; // Tempest station ID, 0 = unused
    std::string tlbl1; // its label on the page, "" when unused
    uint32_t tstn2;
    std::string tlbl2;
};

// Night-dim level. The page sets hours only; the level stays a build choice.
#ifdef DIM_BRIGHTNESS
    #define SETTINGS_DIM_LEVEL DIM_BRIGHTNESS
#else
    #define SETTINGS_DIM_LEVEL 128
#endif

class Settings {
  public:
    static void load(); // once, first thing in setup()
    static const SettingsValues &get();
    static SettingsValues defaults(); // from config.h
    static bool save(const SettingsValues &v); // writes every key + schema
    static bool factoryReset(); // clears namespace "orbs"
    static bool hasStored(); // true once the page has saved at least once

  private:
    static SettingsValues s_values;
    static bool s_stored;
};

#endif
