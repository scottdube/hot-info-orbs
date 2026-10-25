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
    std::string tickers; // "SPY,QQQ" - normalized, 1..5
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
    std::string hidden; // widget names left out of the rotation, '\n'-separated; "" = show all
    // Custom pages (docs/superpowers/specs/2026-10-17-custom-pages-design.md).
    // A page is registered at boot only when its URL / host is set.
    std::string wdname; // Web Data page name, 1..16
    std::string wdurl; // "" = no Web Data page
    std::string mqname; // MQTT page name, 1..16
    std::string mqhost; // "" = no MQTT page
    int mqport;
    std::string mquser;
    std::string mqpass; // write-only on the page
    std::string mqt[5]; // per-orb topic, "" = orb left blank
    std::string mql[5]; // per-orb label
    std::string mqu[5]; // per-orb unit
    std::string mqsetup; // advanced layout topic, "" = none
    // Second MQTT page: its own five rows, on page 1's broker or one of its own
    std::string mq2name; // 1..16
    bool mq2same; // true = page 1's broker and connection; false = mq2host etc.
    std::string mq2host; // "" = no page 2 when mq2same is false
    int mq2port;
    std::string mq2user;
    std::string mq2pass; // write-only on the page
    std::string mq2t[5];
    std::string mq2l[5];
    std::string mq2u[5];
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
