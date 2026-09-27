// Recovery app: lives in the `factory` partition and does one thing, take a
// firmware upload over WiFi and write it to the app slot (ota_0).
//
// Why it exists (docs/VARIANT-ESP32-S3-SUPERMINI.md, 2026-09-27): two OTA slots
// kept a full spare copy of the app, so each held only 1.875 MB. With this
// ~0.7 MB app doing the writing, the one app slot gets 3 MB.
//
// It runs when the app asks for it (POST /recovery), and on its own when the
// bootloader has nothing better: an empty slot, or an update that reset before
// confirming. It has no display code; the panels keep whatever the app drew
// last. Keep it small and rarely changed: every update goes through it.
//
// Build and flash over USB:  pio run -e recovery -t upload
#include "RecoveryPolicy.h"
#include "build_id.h"
#include <Arduino.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_image_format.h>
#include <esp_ota_ops.h>
#include <esp_wifi.h>

#ifndef OTA_HOSTNAME
    #define OTA_HOSTNAME "info-orbs"
#endif

static WebServer server(80);
static const esp_partition_t *appPart = nullptr;
static AppSlot app{false, false};
static bool requested = false;
static bool uploading = false;
static bool serving = false;
static uint32_t lastActivity = 0;

static AppSlot readSlot() {
    AppSlot s{false, false};
    if (!appPart) {
        return s;
    }
    esp_partition_pos_t pos = {appPart->address, appPart->size};
    esp_image_metadata_t meta;
    s.imageValid = esp_image_verify(ESP_IMAGE_VERIFY_SILENT, &pos, &meta) == ESP_OK;
    esp_ota_img_states_t state;
    if (esp_ota_get_state_partition(appPart, &state) == ESP_OK) {
        s.markedBad = state == ESP_OTA_IMG_INVALID || state == ESP_OTA_IMG_ABORTED;
    }
    return s;
}

static void bootApp(const char *why) {
    Serial.printf("Recovery: starting the app (%s)\n", why);
    if (esp_ota_set_boot_partition(appPart) != ESP_OK) {
        Serial.println("Recovery: could not select the app slot, staying");
        return;
    }
    delay(200);
    ESP.restart();
}

static bool authorized() {
#ifdef OTA_PASSWORD
    if (!server.authenticate("admin", OTA_PASSWORD)) {
        server.requestAuthentication();
        return false;
    }
#endif
    return true;
}

static const char *appStateText() {
    if (!app.imageValid) {
        return "&#10006; The app slot holds no complete firmware. Upload one to get the orbs back.";
    }
    if (app.markedBad) {
        return "&#10006; The last update restarted before it reached WiFi, so it was stopped. Upload a working firmware.";
    }
    return "&#10004; The app slot holds a good firmware.";
}

static void sendPage() {
    lastActivity = millis(); // someone is looking; don't leave under them
    String h = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width'>"
               "<title>Info Orbs recovery</title></head><body style='font-family:sans-serif'>"
               "<h2>Info Orbs recovery</h2>"
               "<p>This is the small updater, not the orbs. Recovery " BUILD_COMMIT ", built " BUILD_TIME ".</p><p>";
    h += appStateText();
    h += "</p>"
         "<form method='POST' action='/update' enctype='multipart/form-data'>"
         "<input type='file' name='firmware' accept='.bin'> "
         "<input type='submit' value='Upload'></form>"
         "<p>Use <code>.pio/build/esp32-s3-devkitc-1/firmware.bin</code> (or <code>ota</code>). "
         "The orbs start when it is written.</p>";
    if (bootable(app)) {
        h += "<form method='POST' action='/return'><input type='submit' value='Back to the orbs without updating'></form>"
             "<p>With no upload for 10 minutes it goes back on its own.</p>";
    }
#ifndef OTA_PASSWORD
    h += "<p>&#9888; No password is set, so anyone on this network can install firmware. Set OTA_PASSWORD in secrets.h.</p>";
#endif
    h += "</body></html>";
    server.sendHeader(RECOVERY_HEADER, "1");
    server.send(200, "text/html", h);
}

static void setupServer() {
    server.on("/", HTTP_GET, []() {
        if (authorized()) {
            sendPage();
        }
    });
    server.on("/update", HTTP_GET, []() {
        if (authorized()) {
            sendPage();
        }
    });
    server.on("/return", HTTP_POST, []() {
        if (!authorized()) {
            return;
        }
        server.sendHeader(RECOVERY_HEADER, "1");
        if (!bootable(app)) {
            server.send(409, "text/plain", "No good firmware in the app slot. Upload one first.\n");
            return;
        }
        server.send(200, "text/plain", "Starting the orbs.\n");
        bootApp("asked to return");
    });
    server.on(
        "/update", HTTP_POST,
        // Runs after the upload has finished
        []() {
            if (!authorized()) {
                return;
            }
            uploading = false;
            bool ok = !Update.hasError() && Update.isFinished();
            server.sendHeader(RECOVERY_HEADER, "1");
            server.send(ok ? 200 : 500, "text/plain", ok ? "Update OK, restarting.\n" : "Update FAILED, still in recovery. Try again.\n");
            if (ok) {
                delay(1000);
                ESP.restart(); // Update.end() already made ota_0 the boot slot
            }
            app = readSlot(); // a failed write leaves the slot incomplete
        },
        // Runs once per chunk while the upload streams in
        []() {
#ifdef OTA_PASSWORD
            if (!server.authenticate("admin", OTA_PASSWORD)) {
                return;
            }
#endif
            HTTPUpload &upload = server.upload();
            lastActivity = millis();
            if (upload.status == UPLOAD_FILE_START) {
                Serial.printf("Recovery: upload %s\n", upload.filename.c_str());
                uploading = true;
                if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
                    Update.printError(Serial);
                }
            } else if (upload.status == UPLOAD_FILE_WRITE) {
                if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                    Update.printError(Serial);
                }
            } else if (upload.status == UPLOAD_FILE_END) {
                // true: accept the size as whatever arrived. Only here is ota_0 marked bootable.
                if (!Update.end(true)) {
                    Update.printError(Serial);
                }
            } else if (upload.status == UPLOAD_FILE_ABORTED) {
                Update.abort();
                uploading = false;
                Serial.println("Recovery: upload aborted");
            }
        });
    server.begin();
    MDNS.begin(OTA_HOSTNAME);
    MDNS.addService("http", "tcp", 80);
    serving = true;
    Serial.printf("Recovery ready: http://%s/update\n", WiFi.localIP().toString().c_str());
}

void setup() {
    Serial.begin(115200);
    Serial.println("\nInfo Orbs recovery " BUILD_COMMIT);

    appPart = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, nullptr);
    app = readSlot();

    Preferences prefs;
    prefs.begin(RECOVERY_NVS_NAMESPACE, false);
    requested = prefs.getBool(RECOVERY_NVS_KEY, false);
    if (requested) {
        prefs.remove(RECOVERY_NVS_KEY);
    }
    prefs.end();

    Serial.printf("Recovery: requested=%d image=%d rolledBack=%d\n", requested, app.imageValid, app.markedBad);
    if (onBoot(requested, app) == RecoveryAction::BootApp) {
        bootApp("not asked for, app is good");
    }

    WiFi.mode(WIFI_STA);
    WiFi.setHostname(OTA_HOSTNAME);
#if (defined WIFI_SSID && defined WIFI_PASS)
    WiFi.begin(WIFI_SSID, WIFI_PASS);
#else
    // The network the app joined (WiFiManager stores it in the WiFi driver's
    // own NVS). By name, not the saved BSSID: the app may have locked onto an
    // access point that is not the best one from here.
    wifi_config_t conf;
    esp_wifi_get_config(WIFI_IF_STA, &conf);
    WiFi.begin((const char *)conf.sta.ssid, (const char *)conf.sta.password);
#endif
    lastActivity = millis();
}

void loop() {
    if (!serving && WiFi.status() == WL_CONNECTED) {
        setupServer();
    }
    if (serving) {
        server.handleClient();
    }
    if (idleReturn(millis() - lastActivity, uploading, app)) {
        bootApp("no upload for 10 minutes");
        lastActivity = millis(); // selecting the slot failed; wait another round
    }
    delay(2);
}
