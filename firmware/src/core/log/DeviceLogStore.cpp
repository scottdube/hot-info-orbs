#include "DeviceLogStore.h"

#include "Utils.h"
#include "build_id.h"
#include <Arduino.h>
#include <LittleFS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <cstdarg>
#include <esp_system.h>
#include <esp_timer.h>

namespace {

const char *kPath = "/log.txt";
const char *kOldPath = "/log.old.txt";
const char *kOldMark = "---- earlier file ----\n";
const char *kNowMark = "---- current file ----\n";
// The log stops growing rather than take the filesystem's last blocks: the
// start-up picture is uploaded to a second file before it replaces the first
constexpr size_t kKeepFree = 8192;

portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
devlog::Inbox s_inbox; // any task, under s_mux
// Everything below: main loop only
devlog::Writer s_writer;
bool s_fs = false;
size_t s_cap = devlog::kFileCap;
int64_t s_refEpoch = 0; // the last clock reading and the uptime it was taken at
uint32_t s_refUp = 0;
bool s_clockSaid = false;
uint32_t s_notStored = 0; // lines that did not reach the flash
esp_reset_reason_t s_resetReason = ESP_RST_UNKNOWN;

uint32_t upSeconds() {
    return static_cast<uint32_t>(esp_timer_get_time() / 1000000);
}

const char *resetText(esp_reset_reason_t r) {
    switch (r) {
    case ESP_RST_POWERON:
        return "power on";
    case ESP_RST_EXT:
        return "reset pin";
    case ESP_RST_SW:
        return "restart by the firmware";
    case ESP_RST_PANIC:
        return "crash";
    case ESP_RST_INT_WDT:
        return "watchdog (interrupt)";
    case ESP_RST_TASK_WDT:
        return "watchdog (task)";
    case ESP_RST_WDT:
        return "watchdog";
    case ESP_RST_DEEPSLEEP:
        return "wake from deep sleep";
    case ESP_RST_BROWNOUT:
        return "brownout (the supply voltage dipped)";
    default:
        return "not known";
    }
}

size_t fileSize(const char *path) {
    if (!s_fs || !LittleFS.exists(path)) {
        return 0;
    }
    fs::File f = LittleFS.open(path, "r");
    const size_t n = f ? f.size() : 0;
    f.close();
    return n;
}

void append(const std::string &text) {
    if (text.empty()) {
        return;
    }
    if (!s_fs) {
        s_notStored++;
        return;
    }
    if (devlog::needsRotate(fileSize(kPath), text.size(), s_cap)) {
        LittleFS.remove(kOldPath);
        LittleFS.rename(kPath, kOldPath);
    }
    if (LittleFS.totalBytes() - LittleFS.usedBytes() < kKeepFree) {
        s_notStored++;
        return;
    }
    fs::File f = LittleFS.open(kPath, "a");
    if (!f || f.write(reinterpret_cast<const uint8_t *>(text.data()), text.size()) != text.size()) {
        s_notStored++;
    }
    f.close();
}

void drain() {
    for (;;) {
        devlog::Entry e;
        portENTER_CRITICAL(&s_mux);
        const bool got = s_inbox.pop(e);
        portEXIT_CRITICAL(&s_mux);
        if (!got) {
            break;
        }
        append(s_writer.add(devlog::epochAt(s_refEpoch, s_refUp, e.up), e.up, e.msg));
    }
    portENTER_CRITICAL(&s_mux);
    const uint16_t lost = s_inbox.takeLost();
    portEXIT_CRITICAL(&s_mux);
    if (lost) {
        const uint32_t up = upSeconds();
        append(s_writer.add(devlog::epochAt(s_refEpoch, s_refUp, up), up,
                            std::to_string(lost) + " events lost: more than " + std::to_string(devlog::kInboxSize) + " at once"));
    }
}

String kb(size_t bytes) {
    return String(bytes / 1024.0, 1) + " KB";
}

// What a helper needs to know about the orb before reading the events
String header() {
    const uint32_t up = upSeconds();
    String h;
    h.reserve(900);
    h += "Info Orbs log\n";
    h += "Firmware " FIRMWARE_VERSION ", commit " BUILD_COMMIT ", built " BUILD_TIME "\n";
    // The same line format as an event, so the two read alike
    h += "Now          ";
    h += devlog::line(devlog::epochAt(s_refEpoch, s_refUp, up), up, "this page was read").c_str();
    h += "This start   ";
    h += resetText(s_resetReason);
    h += "\n";
    h += "Memory       " + String(ESP.getFreeHeap()) + " bytes free, lowest since start " + String(ESP.getMinFreeHeap()) + "\n";
    h += "WiFi now     ";
    if (WiFi.status() == WL_CONNECTED) {
        const uint8_t *ap = WiFi.BSSID();
        char tail[8];
        snprintf(tail, sizeof(tail), "..:%02x", ap ? ap[5] : 0);
        h += "up, " + String(WiFi.RSSI()) + " dBm, channel " + String(WiFi.channel()) + ", access point " + tail + "\n";
    } else {
        h += "down, state " + String(static_cast<int>(WiFi.status())) + "\n";
    }
    h += "Log storage  ";
    if (s_fs) {
        h += kb(fileSize(kPath) + fileSize(kOldPath)) + " in two files of up to " + kb(s_cap) + "; filesystem " +
             kb(LittleFS.totalBytes() - LittleFS.usedBytes()) + " free of " + kb(LittleFS.totalBytes()) + "\n";
    } else {
        h += "none: the filesystem did not mount, nothing is kept\n";
    }
    if (s_notStored) {
        h += "             " + String(s_notStored) + " lines since start could not be stored\n";
    }
    h += "\n";
    h += "Events only: no network names, passwords or keys are recorded.\n";
    h += "Each line: date and time | time since start | event. Dashes = clock not set yet.\n";
    h += "WiFi reasons: 2 login timed out, 8 left the access point, 15 handshake timed out\n";
    h += "(often a wrong password), 200 lost the access point's signal, 201 network not\n";
    h += "found, 202 password refused. WiFi state: 0 joined but no address, 1 network not\n";
    h += "found, 3 connected, 4 join failed, 5 connection lost, 6 disconnected.\n";
    h += "\n";
    return h;
}

void sendFile(WebServer &server, const char *path, size_t size) {
    fs::File f = LittleFS.open(path, "r");
    char buf[512];
    while (f && size) {
        const size_t n = f.read(reinterpret_cast<uint8_t *>(buf), size < sizeof(buf) ? size : sizeof(buf));
        if (!n) {
            break;
        }
        server.sendContent(buf, n);
        size -= n;
    }
    f.close();
}

} // namespace

namespace DeviceLog {

void begin(bool filesystemMounted) {
    s_fs = filesystemMounted;
    if (s_fs) {
        s_cap = devlog::fileCap(LittleFS.totalBytes());
    }
    s_resetReason = esp_reset_reason();
    note("start: firmware " FIRMWARE_VERSION " commit " BUILD_COMMIT ", %s", resetText(s_resetReason));
    // Onto the flash now, not at the first pass of loop(): an orb that
    // crashes while it is still starting must leave one line per start
    drain();
}

void note(const char *format, ...) {
    char msg[devlog::kMsgMax + 1];
    va_list args;
    va_start(args, format);
    vsnprintf(msg, sizeof(msg), format, args);
    va_end(args);
    const uint32_t up = upSeconds();
    portENTER_CRITICAL(&s_mux);
    s_inbox.push(up, msg);
    portEXIT_CRITICAL(&s_mux);
    Serial.printf("[log] %s\n", msg);
}

void tick() {
    drain();
}

void clock(int64_t localEpoch) {
    if (!devlog::clockSet(localEpoch)) {
        return;
    }
    s_refEpoch = localEpoch;
    s_refUp = upSeconds();
    if (!s_clockSaid) {
        s_clockSaid = true;
        note("clock set"); // dates the lines above it too: count back by the uptime
    }
}

void beforeRestart(const char *why) {
    note("restart: %s", why);
    drain();
    append(s_writer.flush());
}

void page(const char *page, devlog::PageWatch &watch, bool ok, bool stale) {
    const std::string news = watch.step(upSeconds(), ok, stale);
    if (!news.empty()) {
        note("%s: %s", page, news.c_str());
    }
}

void send(WebServer &server, bool download) {
    drain();
    const String head = header();
    const std::string pending = s_writer.pending();
    const size_t oldSize = fileSize(kOldPath);
    const size_t nowSize = fileSize(kPath);
    // The length up front: nothing is written to the log while this runs
    // (same task), and a browser then shows a download's progress
    const size_t total = head.length() + (oldSize ? strlen(kOldMark) + oldSize : 0) + strlen(kNowMark) + nowSize + pending.size();
    server.sendHeader("Cache-Control", "no-store");
    if (download) {
        server.sendHeader("Content-Disposition", "attachment; filename=\"info-orbs-log.txt\"");
    }
    server.setContentLength(total);
    server.send(200, "text/plain; charset=utf-8", "");
    server.sendContent(head);
    if (oldSize) {
        server.sendContent(kOldMark, strlen(kOldMark));
        sendFile(server, kOldPath, oldSize);
    }
    server.sendContent(kNowMark, strlen(kNowMark));
    sendFile(server, kPath, nowSize);
    if (!pending.empty()) {
        server.sendContent(pending.data(), pending.size());
    }
}

} // namespace DeviceLog
