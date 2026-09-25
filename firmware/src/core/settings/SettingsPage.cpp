#include "SettingsPage.h"

#include "SettingsValidation.h"
#include "Utils.h"
#include "build_id.h"
#include "clockwidget/ClockWidget.h"
#include "config_helper.h"
#include "icons.h"
#include <LittleFS.h>

static const char *BOOT_PATH = "/boot.jpg";
static const char *BOOT_TMP = "/boot.tmp";
static const size_t BOOT_MAX = 64 * 1024; // the filesystem is 128 KB
static const size_t HEADER_CHECK = 4096; // SOF sits in the first ~600 bytes of a canvas JPEG

bool SettingsPage::s_fsMounted = false;
bool SettingsPage::s_pictureUnreadable = false;

void SettingsPage::setBootStatus(bool fsMounted, bool storedPictureUnreadable) {
    s_fsMounted = fsMounted;
    s_pictureUnreadable = storedPictureUnreadable;
}

// Color is never the only signal (docs: color accessibility) - every state
// carries a glyph and words as well
static const char *PAGE_HEAD =
    "<!DOCTYPE html><html><head><meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>Info Orbs settings</title><style>"
    "body{font-family:sans-serif;max-width:34em;margin:0 auto;padding:0 16px 2em}"
    "fieldset{border:1px solid #999;margin:1em 0}legend{font-weight:bold}"
    "label{display:block;margin:.7em 0 .2em}label.chk{margin:.3em 0}"
    "input[type=text],input[type=number],select{width:100%;box-sizing:border-box;padding:.3em;font-size:1em}"
    ".hint{color:#555;font-size:.85em}.err{color:#b00;font-weight:bold}"
    ".banner{padding:.5em;border:2px solid;margin:1em 0}"
    "button{font-size:1em;padding:.5em 1.2em;margin:.5em .5em 0 0}"
    "</style></head><body><h2>Info Orbs settings</h2>";

static std::string argStr(WebServer &s, const char *name) {
    return std::string(s.arg(name).c_str());
}

static String esc(const std::string &in) {
    return String(sv::htmlEscape(in).c_str());
}

static String uptime() {
    unsigned long s = millis() / 1000;
    char buf[32];
    snprintf(buf, sizeof(buf), "%lud %02luh %02lum", s / 86400, (s / 3600) % 24, (s / 60) % 60);
    return buf;
}

static String hourText(int h) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%02d:00", h);
    return buf;
}

// --- form pieces --------------------------------------------------------------

static String errorLine(const std::map<std::string, std::string> &errors, const char *key) {
    auto it = errors.find(key);
    if (it == errors.end()) {
        return "";
    }
    return "<div class='err'>&#10006; " + esc(it->second) + "</div>";
}

static String hint(const String &text) {
    return "<div class='hint'>" + text + "</div>";
}

static String textField(const char *key, const char *label, const std::string &value, const std::string &def, const String &extra,
                        const std::map<std::string, std::string> &errors) {
    return "<label for='" + String(key) + "'>" + label + "</label><input type='text' id='" + key + "' name='" + key + "' value='" +
           esc(value) + "'>" + hint("Default: " + esc(def) + extra) + errorLine(errors, key);
}

static String option(const String &value, const String &text, bool selected) {
    return "<option value='" + value + "'" + (selected ? " selected" : "") + ">" + text + "</option>";
}

static String boolSelect(const char *key, const char *label, bool value, bool def, const char *offText, const char *onText,
                         const std::map<std::string, std::string> &errors) {
    return "<label for='" + String(key) + "'>" + label + "</label><select id='" + key + "' name='" + key + "'>" +
           option("0", offText, !value) + option("1", onText, value) + "</select>" +
           hint(String("Default: ") + (def ? onText : offText)) + errorLine(errors, key);
}

static String hourSelect(const char *key, const char *label, int value, int def, const std::map<std::string, std::string> &errors) {
    String s = "<label for='" + String(key) + "'>" + label + "</label><select id='" + key + "' name='" + key + "'>";
    for (int h = 0; h < 24; h++) {
        s += option(String(h), hourText(h), h == value);
    }
    return s + "</select>" + hint("Default: " + hourText(def)) + errorLine(errors, key);
}

static String colorField(const char *key, const char *label, uint16_t value, uint16_t def, const std::map<std::string, std::string> &errors) {
    String defHex = sv::rgb565ToHex(def).c_str();
    return "<label for='" + String(key) + "'>" + label + "</label><input type='color' id='" + key + "' name='" + key + "' value='" +
           sv::rgb565ToHex(value).c_str() + "'>" + hint("Default: " + defHex + " (the orbs show 65,536 colors, so a picked color may shift slightly)") +
           errorLine(errors, key);
}

static const char *faceName(int face) {
    switch (face) {
    case (int)ClockType::NIXIE:
        return "Nixie";
    case (int)ClockType::CUSTOM:
        return "Custom";
    default:
        return "Normal (LED digits)";
    }
}

static bool faceCompiledIn(int face) {
    return face == (int)ClockType::NORMAL || (face == (int)ClockType::NIXIE && USE_CLOCK_NIXIE) ||
           (face == (int)ClockType::CUSTOM && USE_CLOCK_CUSTOM);
}

// --- page ---------------------------------------------------------------------

SettingsPage::SettingsPage(OtaUpdater &ota) : m_ota(ota) {}

void SettingsPage::begin() {
    if (m_started) {
        return;
    }
    WebServer &s = m_ota.server();
    s.on("/settings", HTTP_GET, [this]() { handleGet(); });
    s.on("/settings", HTTP_POST, [this]() { handlePost(); });
    s.on("/settings/reset", HTTP_POST, [this]() { handleReset(); });
    s.on("/settings/boot", HTTP_POST, [this]() { handleBootUploadDone(); }, [this]() { handleBootUploadChunk(); });
    s.on("/settings/boot/reset", HTTP_POST, [this]() { handleBootReset(); });
    s.on("/boot.jpg", HTTP_GET, [this]() { handleBootGet(); });
    s.on("/", HTTP_GET, [&s]() {
        s.sendHeader("Location", "/settings");
        s.send(302, "text/plain", "");
    });
    m_started = true;
}

String SettingsPage::renderForm(const SettingsValues &v, const Errors &errors, const String &banner) {
    const SettingsValues d = Settings::defaults();
    String h;
    h.reserve(9000);
    h += PAGE_HEAD;
    h += "<p>Running " FIRMWARE_VERSION ", commit " BUILD_COMMIT ", built " BUILD_TIME "<br>Up " + uptime() + ". ";
    h += Settings::hasStored() ? "Using settings saved from this page." : "Nothing saved yet &mdash; using the values built into the firmware.";
    h += "</p>";
#ifndef OTA_PASSWORD
    h += "<p class='hint'>&#9888; No password is set, so anyone on this network can change these and install firmware. "
         "Set OTA_PASSWORD in secrets.h.</p>";
#endif
    h += banner;
    h += "<form method='POST' action='/settings'>";

    h += "<fieldset><legend>Rotation</legend>";
    h += "<label for='cycle'>Seconds per page</label><input type='number' id='cycle' name='cycle' min='0' max='3600' value='" + String(v.cycle) + "'>";
    h += hint("0 = stay on one page. Otherwise 5 to 3600. Default: " + String(d.cycle)) + errorLine(errors, "cycle");
    if (m_widgets && m_widgets->count() > 0) {
        h += "<label>Show</label><input type='hidden' name='shwform' value='1'>";
        for (int8_t i = 0; i < m_widgets->count(); i++) {
            std::string k = m_widgets->key(i).c_str();
            h += "<label class='chk'><input type='checkbox' name='shw" + String(i) + "' value='1'" +
                 (sv::listContains(v.hidden, k) ? "" : " checked") + "> " + sv::htmlEscape(k).c_str() + "</label>";
        }
        h += hint("Tick one to keep it on screen, or several to rotate through them. The buttons move between ticked pages only.") +
             errorLine(errors, "show");
    }
    h += "</fieldset>";

    h += "<fieldset><legend>Clock</legend>";
    h += "<label for='face'>Clock face at start-up</label><select id='face' name='face'>";
    for (int f = 0; f <= (int)ClockType::CUSTOM; f++) {
        if (faceCompiledIn(f)) {
            h += option(String(f), faceName(f), f == v.face);
        }
    }
    h += "</select>" + hint(String("Default: ") + faceName(d.face) + ". A short press on the middle button still changes it until the next restart; a medium press toggles 12/24-hour the same way.") + errorLine(errors, "face");
    h += boolSelect("h24", "Hours", v.h24, d.h24, "12-hour", "24-hour", errors);
    h += boolSelect("ampm", "AM/PM indicator (12-hour, not Nixie)", v.ampm, d.ampm, "Off", "On", errors);
    h += colorField("clkcol", "Digit color", v.clkcol, d.clkcol, errors);
    h += colorField("shdcol", "Unlit-segment color", v.shdcol, d.shdcol, errors);
    h += textField("tz", "Timezone", v.tz, d.tz,
                   " &mdash; a name from <a href='https://timezonedb.com/time-zones'>timezonedb.com/time-zones</a>. Not checked until the orb asks for the time.",
                   errors);
    h += "</fieldset>";

#ifdef STOCK_TICKER_LIST
    h += "<fieldset><legend>Stocks</legend>";
    h += textField("tickers", "Tickers, comma-separated (1 to 5, one per orb)", v.tickers, d.tickers,
                   ". Crypto as BTC/USD; add &amp;country=Canada to pick an exchange.", errors);
    h += "</fieldset>";
#endif

    h += "<fieldset><legend>Weather</legend>";
    h += textField("wxloc", "Location", v.wxloc, d.wxloc, ". Not checked until the orb fetches the weather.", errors);
    h += boolSelect("wxmetric", "Units", v.wxmetric, d.wxmetric, "&deg;F, mph", "&deg;C, km/h", errors);
    h += boolSelect("wxdark", "Weather screens", v.wxdark, d.wxdark, "Light", "Dark", errors);
    h += "</fieldset>";

    h += "<fieldset><legend>Dim hours</legend>";
    h += boolSelect("dim", "Dim", v.dim, d.dim, "Off", "On", errors);
    h += hint("Darkens the colors drawn; the backlight itself stays on.");
    h += hourSelect("dimstart", "From", v.dimstart, d.dimstart, errors);
    h += hourSelect("dimend", "Until", v.dimend, d.dimend, errors);
    h += "</fieldset>";

    h += "<fieldset><legend>Screens-off hours</legend>";
    h += boolSelect("off", "Screens off", v.off, d.off, "Off", "On", errors);
    h += hint("Blanks the panels; any button wakes them for a minute. Where these hours overlap the dim hours, off wins.");
    h += hourSelect("offstart", "From", v.offstart, d.offstart, errors);
    h += hourSelect("offend", "Until", v.offend, d.offend, errors);
    h += "</fieldset>";

    h += "<fieldset><legend>Mounting</legend>";
    h += boolSelect("invert", "Orbs", v.invert, d.invert, "Right way up", "Upside down", errors);
    h += "</fieldset>";

    h += "<button type='submit'>Save and restart</button></form>";
    h += "<form method='POST' action='/settings/reset' onsubmit=\"return confirm('Forget every saved setting and go back to the built-in values?')\">"
         "<button type='submit'>Reset to built-in values</button></form>";
    h += renderBootSection(); // its own forms - HTML forms cannot nest
    h += "<p><a href='/update'>Firmware update</a></p></body></html>";
    return h;
}

void SettingsPage::handleGet() {
    if (!m_ota.authorized()) {
        return;
    }
    m_ota.server().send(200, "text/html", renderForm(Settings::get(), Errors(), ""));
}

void SettingsPage::handlePost() {
    if (!m_ota.authorized()) {
        return;
    }
    WebServer &s = m_ota.server();
    SettingsValues v = Settings::get(); // a field that is not sent keeps its value
    Errors errors;
    std::string err;

    auto has = [&s](const char *k) { return s.hasArg(k); };
    auto readBool = [&](const char *k, bool &out) {
        if (!has(k)) {
            return;
        }
        std::string a = argStr(s, k);
        if (a == "0" || a == "1") {
            out = (a == "1");
        } else {
            errors[k] = "Must be 0 or 1";
        }
    };

    if (has("cycle") && !sv::parseCycle(argStr(s, "cycle"), v.cycle, err)) {
        errors["cycle"] = err;
    }
    if (has("shwform") && m_widgets) {
        // An unticked checkbox sends nothing, hence the shwform marker
        std::string hidden;
        int shown = 0;
        for (int8_t i = 0; i < m_widgets->count(); i++) {
            if (has(("shw" + String(i)).c_str())) {
                shown++;
            } else {
                hidden += (hidden.empty() ? "" : "\n") + std::string(m_widgets->key(i).c_str());
            }
        }
        v.hidden = hidden;
        if (shown == 0) {
            errors["show"] = "Tick at least one page";
        }
    }
#ifdef STOCK_TICKER_LIST
    if (has("tickers") && !sv::normalizeTickers(argStr(s, "tickers"), v.tickers, err)) {
        errors["tickers"] = err;
    }
#endif
    if (has("wxloc") && !sv::normalizeText(argStr(s, "wxloc"), 64, v.wxloc, err)) {
        errors["wxloc"] = err;
    }
    if (has("tz") && !sv::normalizeText(argStr(s, "tz"), 48, v.tz, err)) {
        errors["tz"] = err;
    }
    readBool("wxmetric", v.wxmetric);
    readBool("wxdark", v.wxdark);
    readBool("h24", v.h24);
    readBool("ampm", v.ampm);
    readBool("dim", v.dim);
    readBool("off", v.off);
    readBool("invert", v.invert);
    if (has("face")) {
        std::string a = argStr(s, "face");
        if (a.size() == 1 && a[0] >= '0' && a[0] <= '2' && faceCompiledIn(a[0] - '0')) {
            v.face = a[0] - '0';
        } else {
            errors["face"] = "That clock face is not built into this firmware";
        }
    }
    if (has("clkcol") && !sv::parseHexColor(argStr(s, "clkcol"), v.clkcol, err)) {
        errors["clkcol"] = err;
    }
    if (has("shdcol") && !sv::parseHexColor(argStr(s, "shdcol"), v.shdcol, err)) {
        errors["shdcol"] = err;
    }
    if (has("dimstart") && !sv::parseHour(argStr(s, "dimstart"), v.dimstart, err)) {
        errors["dimstart"] = err;
    }
    if (has("dimend") && !sv::parseHour(argStr(s, "dimend"), v.dimend, err)) {
        errors["dimend"] = err;
    }
    if (has("offstart") && !sv::parseHour(argStr(s, "offstart"), v.offstart, err)) {
        errors["offstart"] = err;
    }
    if (has("offend") && !sv::parseHour(argStr(s, "offend"), v.offend, err)) {
        errors["offend"] = err;
    }
    if (v.off && v.offstart == v.offend && !errors.count("offstart") && !errors.count("offend")) {
        errors["offend"] = "Start and end are the same hour";
    }
    // Equal hours would mean "dim around the clock" (WidgetSet's wrap branch)
    if (v.dim && v.dimstart == v.dimend && !errors.count("dimstart") && !errors.count("dimend")) {
        errors["dimend"] = "Start and end are the same hour";
    }

    if (!errors.empty()) {
        String banner = "<div class='banner err'>&#10006; Not saved &mdash; " + String((unsigned)errors.size()) +
                        (errors.size() == 1 ? " field needs" : " fields need") + " fixing. Nothing was changed.</div>";
        s.send(400, "text/html", renderForm(v, errors, banner));
        return;
    }
    if (!Settings::save(v)) {
        String banner = "<div class='banner err'>&#10006; Could not write to the orb's storage. Nothing was changed and it has not restarted.</div>";
        s.send(500, "text/html", renderForm(v, errors, banner));
        return;
    }
    sendRestarting("Saved");
}

void SettingsPage::handleReset() {
    if (!m_ota.authorized()) {
        return;
    }
    if (!Settings::factoryReset()) {
        String banner = "<div class='banner err'>&#10006; Could not clear the saved settings. Nothing was changed.</div>";
        m_ota.server().send(500, "text/html", renderForm(Settings::get(), Errors(), banner));
        return;
    }
    sendRestarting("Reset to built-in values");
}

// --- boot picture --------------------------------------------------------------

// The browser does the cropping and the JPEG encoding: canvas.toBlob always
// writes a baseline JPEG, which is the only kind TJpg_Decoder can draw. The
// orb still checks what arrives (handleBootUploadDone).
static const char *BOOT_JS =
    "<script>"
    "const f=document.getElementById('pic'),cv=document.getElementById('cv'),up=document.getElementById('up'),msg=document.getElementById('msg');"
    "f.onchange=()=>{const file=f.files[0];if(!file)return;const img=new Image();"
    "img.onload=()=>{const s=Math.min(img.width,img.height),c=cv.getContext('2d');"
    "c.fillStyle='#000';c.fillRect(0,0,240,240);"
    "c.drawImage(img,(img.width-s)/2,(img.height-s)/2,s,s,0,0,240,240);"
    "cv.style.display='inline-block';up.disabled=false;msg.textContent='The circle is what the orb shows.';URL.revokeObjectURL(img.src)};"
    "img.onerror=()=>{msg.textContent='✖ That file could not be read as a picture.'};"
    "img.src=URL.createObjectURL(file)};"
    "up.onclick=async()=>{up.disabled=true;let q=.85,b;"
    "do{b=await new Promise(r=>cv.toBlob(r,'image/jpeg',q));q-=.1}while(b.size>65536&&q>.1);"
    "if(b.size>65536){msg.textContent='✖ Could not make it small enough.';return}"
    "const fd=new FormData();fd.append('picture',b,'boot.jpg');"
    "msg.textContent='Uploading '+Math.round(b.size/1024)+' KB…';"
    "try{const r=await fetch('/settings/boot',{method:'POST',body:fd}),t=await r.text();"
    "msg.textContent=(r.ok?'✔ ':'✖ ')+t;if(r.ok)setTimeout(()=>location.reload(),20000);else up.disabled=false}"
    "catch(e){msg.textContent='✖ Upload failed: '+e;up.disabled=false}};"
    "</script>";

String SettingsPage::renderBootSection() {
    bool stored = s_fsMounted && LittleFS.exists(BOOT_PATH);
    String h = "<fieldset><legend>Start-up picture</legend>";
    h += "<img src='/boot.jpg?t=" + String(millis()) + "' width='120' height='120' style='border-radius:50%;vertical-align:middle' alt='current start-up picture'> ";
    if (!s_fsMounted) {
        h += "<span class='err'>&#10006; The orb's picture storage could not be opened, so only the built-in logo can be shown.</span>";
    } else if (stored && s_pictureUnreadable) {
        h += "<span class='err'>&#10006; The stored picture could not be drawn at the last start, so the built-in logo was shown. Reset it or upload another.</span>";
    } else {
        h += stored ? "Your picture." : "Built-in logo.";
    }
    if (s_fsMounted) {
        h += "<label for='pic'>New picture (any photo; it is cropped to a centered square)</label>"
             "<input type='file' id='pic' accept='image/*'>"
             "<canvas id='cv' width='240' height='240' style='display:none;width:120px;height:120px;border-radius:50%;margin-top:.5em'></canvas>"
             "<div><button type='button' id='up' disabled>Upload and restart</button></div><div id='msg' class='hint'></div>";
        h += BOOT_JS;
    }
    h += "</fieldset>";
    if (stored) {
        h += "<form method='POST' action='/settings/boot/reset' onsubmit=\"return confirm('Go back to the built-in logo?')\">"
             "<button type='submit'>Use the built-in logo</button></form>";
    }
    return h;
}

void SettingsPage::handleBootUploadChunk() {
    WebServer &s = m_ota.server();
    HTTPUpload &up = s.upload();
    if (up.status == UPLOAD_FILE_START) {
        m_uploadError = "";
        m_uploadSize = 0;
#ifdef OTA_PASSWORD
        // No challenge mid-upload; the done handler sends the 401
        if (!s.authenticate("admin", OTA_PASSWORD)) {
            m_uploadError = "not authorized";
            return;
        }
#endif
        if (!s_fsMounted) {
            m_uploadError = "The orb's picture storage is not available.";
            return;
        }
        File f = LittleFS.open(BOOT_TMP, "w");
        if (!f) {
            m_uploadError = "Could not open storage for writing.";
        }
        f.close();
        return;
    }
    if (!m_uploadError.isEmpty()) {
        return;
    }
    if (up.status == UPLOAD_FILE_WRITE) {
        m_uploadSize += up.currentSize;
        if (m_uploadSize > BOOT_MAX) {
            m_uploadError = "Picture is over 64 KB.";
            LittleFS.remove(BOOT_TMP);
            return;
        }
        File f = LittleFS.open(BOOT_TMP, "a");
        if (!f || f.write(up.buf, up.currentSize) != up.currentSize) {
            m_uploadError = "The orb's storage is full.";
        }
        f.close();
    } else if (up.status == UPLOAD_FILE_ABORTED) {
        m_uploadError = "Upload was interrupted.";
        LittleFS.remove(BOOT_TMP);
    }
}

void SettingsPage::handleBootUploadDone() {
    if (!m_ota.authorized()) {
        return;
    }
    WebServer &s = m_ota.server();
    if (m_uploadError.isEmpty() && m_uploadSize == 0) {
        m_uploadError = "No picture was received.";
    }
    if (m_uploadError.isEmpty()) {
        uint8_t *buf = (uint8_t *)malloc(HEADER_CHECK);
        File f = LittleFS.open(BOOT_TMP, "r");
        size_t n = (buf && f) ? f.read(buf, HEADER_CHECK) : 0;
        f.close();
        std::string err;
        if (!buf || !n) {
            m_uploadError = "Could not read the upload back.";
        } else if (!sv::checkJpegHeader(buf, n, err)) {
            m_uploadError = err.c_str();
        }
        free(buf);
    }
    if (!m_uploadError.isEmpty()) {
        LittleFS.remove(BOOT_TMP);
        s.send(400, "text/plain", m_uploadError);
        return;
    }
    LittleFS.remove(BOOT_PATH);
    if (!LittleFS.rename(BOOT_TMP, BOOT_PATH)) {
        s.send(500, "text/plain", "Could not store the picture.");
        return;
    }
    s.send(200, "text/plain", "Saved (" + String((unsigned)(m_uploadSize / 1024)) + " KB). The orbs are restarting to show it.");
    delay(1000);
    ESP.restart();
}

void SettingsPage::handleBootReset() {
    if (!m_ota.authorized()) {
        return;
    }
    if (s_fsMounted) {
        LittleFS.remove(BOOT_PATH);
    }
    sendRestarting("Back to the built-in logo");
}

void SettingsPage::handleBootGet() {
    if (!m_ota.authorized()) {
        return;
    }
    WebServer &s = m_ota.server();
    s.sendHeader("Cache-Control", "no-store");
    if (s_fsMounted && LittleFS.exists(BOOT_PATH)) {
        File f = LittleFS.open(BOOT_PATH, "r");
        s.streamFile(f, "image/jpeg");
        f.close();
        return;
    }
    s.send_P(200, "image/jpeg", (const char *)logo_start, logo_end - logo_start);
}

void SettingsPage::sendRestarting(const String &what) {
    // The restart takes about 15 s (measured for OTA); reload after 20
    String h = PAGE_HEAD;
    h += "<meta http-equiv='refresh' content='20;url=/settings'>";
    h += "<div class='banner'>&#10004; " + what + ". The orbs are restarting &mdash; this page reloads in 20 seconds.</div>";
    h += "<p><a href='/settings'>Back to settings</a></p></body></html>";
    m_ota.server().send(200, "text/html", h);
    delay(1000); // let the reply leave before the network goes down
    ESP.restart();
}
