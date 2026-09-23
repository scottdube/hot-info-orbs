#ifndef OTAUPDATER_H
#define OTAUPDATER_H

#include "ScreenManager.h"
#include <Arduino.h>

// Over-the-air firmware updates (docs/REQUIREMENTS.md R2), two ways in:
//   - browser: http://<OTA_HOSTNAME>.local/update, pick firmware.bin, upload
//   - PlatformIO: pio run -e ota -t upload   (ArduinoOTA / espota)
// Both write the idle app slot and switch to it only after the whole image
// has been written and verified, so an interrupted upload leaves the running
// firmware in place. Needs the two-slot layout in partitions.csv.
class OtaUpdater {
  public:
    explicit OtaUpdater(ScreenManager &manager);
    void begin(); // once WiFi is connected
    void handle(); // every loop
    bool isUpdating() const { return m_updating; }

  private:
    void setupArduinoOta();
    void setupWebUpdate();
    void drawStatus(const String &line1, const String &line2, uint32_t color);
    void drawProgress(size_t done, size_t total);

    ScreenManager &m_manager;
    bool m_started = false;
    int m_lastPercent = -1;
    bool m_updating = false;
};

#endif
