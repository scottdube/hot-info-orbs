#ifndef OTAUPDATER_H
#define OTAUPDATER_H

#include "ScreenManager.h"
#include <Arduino.h>
#include <WebServer.h>

// Over-the-air firmware updates (docs/REQUIREMENTS.md R2). The app does not
// write itself: /update offers a restart into the recovery app in the
// `factory` partition (firmware/src/recovery), which takes the upload and
// writes app0. That is what lets app0 be 3 MB instead of two 1.875 MB slots.
//   - browser: http://<orb IP>/update, "Restart into the updater", then upload
//   - PlatformIO: pio run -e ota -t upload --upload-port <orb IP>
// This class also confirms a fresh update once WiFi is up (see .cpp).
class OtaUpdater {
  public:
    explicit OtaUpdater(ScreenManager &manager);
    void begin(); // once WiFi is connected
    void handle(); // every loop
    bool isUpdating() const { return m_updating; }
    // Shared with SettingsPage: one server on port 80, one password check
    WebServer &server();
    bool authorized(); // false = a 401 challenge has already been sent

  private:
    void setupWebUpdate();
    void drawStatus(const String &line1, const String &line2, uint32_t color);

    ScreenManager &m_manager;
    bool m_started = false;
    bool m_updating = false;
};

#endif
