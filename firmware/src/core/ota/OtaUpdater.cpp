#include "OtaUpdater.h"
#include "Utils.h"
#include "build_id.h"
#include "config_helper.h"
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <Update.h>
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
// resets before then is rolled back to the previous slot by the bootloader.
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
    "<form method='POST' action='/update' enctype='multipart/form-data'>"
    "<input type='file' name='firmware' accept='.bin'> "
    "<input type='submit' value='Upload'></form>"
    "<p>Use <code>.pio/build/&lt;env&gt;/firmware.bin</code>. The orbs restart when it is done.</p>"
    "<p><a href='/settings'>Settings</a></p>"
    "</body></html>";

OtaUpdater::OtaUpdater(ScreenManager &manager) : m_manager(manager) {}

void OtaUpdater::begin() {
    if (m_started) {
        return;
    }
    setupArduinoOta(); // starts mDNS with OTA_HOSTNAME
    setupWebUpdate();
    m_started = true;
    esp_ota_mark_app_valid_cancel_rollback(); // harmless when not pending (e.g. after a USB flash)
    Serial.printf("OTA ready: http://%s.local/update (%s)\n", OTA_HOSTNAME, WiFi.localIP().toString().c_str());
#ifndef OTA_PASSWORD
    Serial.println("OTA has no password - anyone on this network can flash the orbs. Set OTA_PASSWORD in secrets.h.");
#endif
}

WebServer &OtaUpdater::server() {
    return s_server;
}

bool OtaUpdater::authorised() {
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
    ArduinoOTA.handle();
    s_server.handleClient();
}

void OtaUpdater::setupArduinoOta() {
    ArduinoOTA.setHostname(OTA_HOSTNAME);
#ifdef OTA_PASSWORD
    ArduinoOTA.setPassword(OTA_PASSWORD);
#endif
    ArduinoOTA.onStart([this]() {
        m_updating = true;
        m_lastPercent = -1;
        drawStatus("Updating", "do not unplug", TFT_ORANGE);
    });
    ArduinoOTA.onProgress([this](unsigned int done, unsigned int total) { drawProgress(done, total); });
    ArduinoOTA.onEnd([this]() { drawStatus("Update OK", "restarting", TFT_GREEN); });
    ArduinoOTA.onError([this](ota_error_t error) {
        Serial.printf("OTA error %u\n", error);
        drawStatus("Update failed", "old firmware kept", TFT_RED);
        delay(3000);
        ESP.restart(); // screens are in an unknown state; a restart redraws everything
    });
    ArduinoOTA.begin();
}

void OtaUpdater::setupWebUpdate() {
    s_server.on("/update", HTTP_GET, [this]() {
        if (!authorised()) {
            return;
        }
        s_server.send(200, "text/html", uploadPage);
    });

    s_server.on(
        "/update", HTTP_POST,
        // Runs after the upload has finished
        [this]() {
            if (!authorised()) {
                return;
            }
            bool ok = !Update.hasError();
            s_server.send(ok ? 200 : 500, "text/plain", ok ? "Update OK, restarting.\n" : "Update FAILED, old firmware kept.\n");
            drawStatus(ok ? "Update OK" : "Update failed", ok ? "restarting" : "old firmware kept", ok ? TFT_GREEN : TFT_RED);
            delay(1000);
            ESP.restart();
        },
        // Runs once per chunk while the upload streams in
        [this]() {
#ifdef OTA_PASSWORD
            if (!s_server.authenticate("admin", OTA_PASSWORD)) {
                return;
            }
#endif
            HTTPUpload &upload = s_server.upload();
            if (upload.status == UPLOAD_FILE_START) {
                Serial.printf("OTA upload: %s\n", upload.filename.c_str());
                m_updating = true;
                m_lastPercent = -1;
                drawStatus("Updating", "do not unplug", TFT_ORANGE);
                if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
                    Update.printError(Serial);
                }
            } else if (upload.status == UPLOAD_FILE_WRITE) {
                if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                    Update.printError(Serial);
                }
                // Total size is unknown for a browser upload; show kilobytes
                if (upload.totalSize / 65536 != (upload.totalSize - upload.currentSize) / 65536) {
                    drawStatus("Updating", String(upload.totalSize / 1024) + " KB", TFT_ORANGE);
                }
            } else if (upload.status == UPLOAD_FILE_END) {
                // true: accept the size as whatever arrived. Only here is the new slot marked bootable.
                if (!Update.end(true)) {
                    Update.printError(Serial);
                }
            } else if (upload.status == UPLOAD_FILE_ABORTED) {
                Update.abort();
                m_updating = false;
                Serial.println("OTA upload aborted");
            }
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
    m_manager.drawCentreString(line1, ScreenCenterX, ScreenCenterY - 20, 22);
    m_manager.setFontColor(TFT_WHITE);
    m_manager.drawCentreString(line2, ScreenCenterX, ScreenCenterY + 20, 16);
}

void OtaUpdater::drawProgress(size_t done, size_t total) {
    int percent = total ? (done * 100) / total : 0;
    if (percent / 5 == m_lastPercent / 5) {
        return; // redraw every 5% - each redraw slows the transfer
    }
    m_lastPercent = percent;
    drawStatus("Updating", String(percent) + "%", TFT_ORANGE);
}
