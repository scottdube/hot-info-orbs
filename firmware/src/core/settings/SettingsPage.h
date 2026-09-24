#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include "OtaUpdater.h"
#include "Settings.h"
#include "WidgetSet.h"
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

    // From setup(), which mounts the filesystem and draws the boot picture
    // before the page exists
    static void setBootStatus(bool fsMounted, bool storedPictureUnreadable);
    // Once every widget is added: the Show checkboxes list them
    void setWidgets(WidgetSet *widgets) { m_widgets = widgets; }

  private:
    typedef std::map<std::string, std::string> Errors;

    void handleGet();
    void handlePost();
    void handleReset();
    void handleBootUploadChunk();
    void handleBootUploadDone();
    void handleBootReset();
    void handleBootGet();
    String renderBootSection();
    String renderForm(const SettingsValues &v, const Errors &errors, const String &banner);
    void sendRestarting(const String &what);

    OtaUpdater &m_ota;
    bool m_started = false;
    WidgetSet *m_widgets = nullptr;

    // Upload in progress: /boot.tmp is written chunk by chunk, checked, then
    // renamed over /boot.jpg - an interrupted upload never replaces the picture
    String m_uploadError;
    size_t m_uploadSize = 0;

    static bool s_fsMounted;
    static bool s_pictureUnreadable;
};

#endif
