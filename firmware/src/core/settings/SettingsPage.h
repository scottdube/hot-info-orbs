#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include "OtaUpdater.h"
#include "Settings.h"
#include <Arduino.h>
#include <map>
#include <string>

// http://<orb>/settings - change the Settings values from a browser
// (docs/superpowers/specs/2026-09-23-web-settings-design.md). Registers on the
// WebServer OtaUpdater owns and uses its password check. A save writes NVS and
// restarts the orb; nothing is applied live.
class SettingsPage {
  public:
    explicit SettingsPage(OtaUpdater &ota);
    void begin(); // once OtaUpdater::begin() has run; safe to call every loop

  private:
    typedef std::map<std::string, std::string> Errors;

    void handleGet();
    void handlePost();
    void handleReset();
    String renderForm(const SettingsValues &v, const Errors &errors, const String &banner);
    void sendRestarting(const String &what);

    OtaUpdater &m_ota;
    bool m_started = false;
};

#endif
