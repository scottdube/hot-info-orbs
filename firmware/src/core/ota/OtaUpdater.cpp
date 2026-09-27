#include "OtaUpdater.h"
#include "RecoveryPolicy.h"
#include "Utils.h"
#include "build_id.h"
#include "config_helper.h"
#include <ESPmDNS.h>
#include <Preferences.h>
#include <WebServer.h>
#include <esp_ota_ops.h>

#ifndef OTA_HOSTNAME
    #define OTA_HOSTNAME "info-orbs"
#endif

static WebServer s_server(80);

// The Arduino core marks a freshly updated image good before setup() runs, so
// an image that boots and then cannot get back on the network would be kept.
// Overriding this hook defers that until begin(): only an image that has
// reached WiFi and is listening for the next update is confirmed. One that
// resets before then is stopped by the bootloader, which starts recovery.
extern "C" bool verifyRollbackLater() {
    return true;
}

// Same warning as the settings page: both pages are open when there is no password
#ifdef OTA_PASSWORD
    #define NO_PASSWORD_WARNING ""
#else
    #define NO_PASSWORD_WARNING "<p>&#9888; No password is set, so anyone on this network can install firmware " \
                                "and change the settings. Set OTA_PASSWORD in secrets.h.</p>"
#endif

static const char *uploadPage =
    "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width'>"
    "<title>Info Orbs update</title></head><body style='font-family:sans-serif'>"
    "<h2>Info Orbs firmware update</h2>"
    "<p>Running: " FIRMWARE_VERSION ", commit " BUILD_COMMIT ", built " BUILD_TIME "</p>"
    NO_PASSWORD_WARNING
    "<p>Updates are installed by a small updater. This restarts the orbs into it; "
    "its upload page opens here in about 20 seconds.</p>"
    "<form method='POST' action='/recovery'><input type='submit' value='Restart into the updater'></form>"
    "<p>From a terminal: <code>pio run -e ota -t upload --upload-port &lt;orb IP&gt;</code></p>"
    "<p><a href='/settings'>Settings</a></p>"
    "</body></html>";

OtaUpdater::OtaUpdater(ScreenManager &manager) : m_manager(manager) {}

void OtaUpdater::begin() {
    if (m_started) {
        return;
    }
    MDNS.begin(OTA_HOSTNAME);
    setupWebUpdate();
    m_started = true;
    esp_ota_mark_app_valid_cancel_rollback(); // harmless when not pending (e.g. after a USB flash)
    Serial.printf("OTA ready: http://%s/update (%s.local)\n", WiFi.localIP().toString().c_str(), OTA_HOSTNAME);
#ifndef OTA_PASSWORD
    Serial.println("OTA has no password - anyone on this network can flash the orbs. Set OTA_PASSWORD in secrets.h.");
#endif
}

WebServer &OtaUpdater::server() {
    return s_server;
}

bool OtaUpdater::authorized() {
#ifdef OTA_PASSWORD
    if (!s_server.authenticate("admin", OTA_PASSWORD)) {
        s_server.requestAuthentication();
        return false;
    }
#endif
    return true;
}

void OtaUpdater::handle() {
    if (!m_started) {
        return;
    }
    s_server.handleClient();
}

void OtaUpdater::setupWebUpdate() {

    s_server.on("/update", HTTP_GET, [this]() {
        if (!authorized()) {
            return;
        }
        s_server.send(200, "text/html", uploadPage);
    });

    // The app has no second slot to write to, so an upload sent here is
    // refused; the body is read and dropped.
    s_server.on("/update", HTTP_POST, [this]() {
        if (!authorized()) {
            return;
        }
        s_server.send(409, "text/plain",
                      "This is the running app, which cannot overwrite itself. POST /recovery first, "
                      "or use tools/ota_upload.py.\n");
    });

    s_server.on("/recovery", HTTP_POST, [this]() {
        if (!authorized()) {
            return;
        }
        const esp_partition_t *factory =
            esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, nullptr);
        if (!factory) {
            s_server.send(500, "text/plain",
                          "No recovery partition: this orb still has the old two-slot layout. "
                          "Flash it once over USB (see platformio.ini).\n");
            return;
        }
        Preferences prefs;
        prefs.begin(RECOVERY_NVS_NAMESPACE, false);
        prefs.putBool(RECOVERY_NVS_KEY, true);
        prefs.end();
        if (esp_ota_set_boot_partition(factory) != ESP_OK) {
            s_server.send(500, "text/plain", "Could not select the recovery partition.\n");
            return;
        }
        String ip = WiFi.localIP().toString();
        s_server.send(200, "text/html",
                      "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width'>"
                      "<meta http-equiv='refresh' content='20;url=/update'></head>"
                      "<body style='font-family:sans-serif'><p>Restarting into the updater. Its page opens here in "
                      "20 seconds; if not, reload <a href='/update'>http://" +
                          ip + "/update</a>.</p></body></html>");
        m_updating = true;
        drawStatus("Updater", ip, TFT_ORANGE); // stays on the panel: recovery has no display code
        delay(1000);
        ESP.restart();
    });

    s_server.begin();
    MDNS.addService("http", "tcp", 80);
}

void OtaUpdater::drawStatus(const String &line1, const String &line2, uint32_t color) {
    m_manager.selectScreen(2);
    m_manager.clearScreen();
    // Otherwise it inherits the current widget's font: on the clock that is
    // DSEG, which has almost no letters. Widgets set their own font per draw.
    m_manager.setFont(DEFAULT_FONT);
    m_manager.setFontColor(color);
    m_manager.drawCenterString(line1, ScreenCenterX, ScreenCenterY - 20, 22);
    m_manager.setFontColor(TFT_WHITE);
    m_manager.drawCenterString(line2, ScreenCenterX, ScreenCenterY + 20, 16);
}
