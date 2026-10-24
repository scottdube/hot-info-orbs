# Custom Pages Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** A member sets up a Web Data page (JSON URL) and an MQTT page (broker + per-orb topics) on `/settings`, with no firmware rebuild, and neither page can hang the orb.

**Architecture:** Both widgets are compiled into every build and registered at boot only when `/settings` (or the old `config.h`/`secrets.h` defines, now defaults) gives them a URL or host. All parsing and timing decisions are pure std-only headers covered by `pio test -e native`; the widgets call them. Status for each page is kept in a static struct the settings page reads, as `TempestSource` already does.

**Tech Stack:** ESP32-S3 Arduino (PlatformIO `espressif32`), TFT_eSPI + OpenFontRender via `ScreenManager`, ArduinoJson 7, HTTPClient, PubSubClient 2.8, Preferences (NVS), Unity host tests.

**Spec:** `docs/superpowers/specs/2026-10-17-custom-pages-design.md` (commit 943dd56). Read it first; this plan argues from it.

## Global Constraints

- Work in the worktree `~/code/hot-info-orbs-main`, branch `custom-pages` (off `main` 454f340). Do not touch `xiao-esp32s3-plus`.
- `firmware/config/config.h` and `firmware/config/secrets.h` are git-ignored. **Never commit them.** Only the `.template` files are committed.
- **Public repo.** No IP addresses, host names, site names, station IDs, tokens or passwords in any committed file, commit message or test record.
- NVS keys ≤ 15 characters, namespace `orbs`, **no schema bump**; an absent key falls back to its default.
- Limits, copied from the spec: page name 1–16 chars; URL blank or `http://`/`https://`, ≤ 256; broker host blank or hostname/IPv4, ≤ 64; port 1–65535 (default 1883); username ≤ 64; password ≤ 64, write-only; topic ≤ 128 with no `#`/`+`; label ≤ 16; unit ≤ 8; layout topic ≤ 128.
- Web Data: interval clamped to ≥ 10 s, 60 s when absent; 5 s connect/read timeout; reply refused above 16 KB; keep the last good data on failure.
- MQTT: one attempt with a 3 s socket timeout, then at most one attempt every 30 s; never block the loop.
- Flash stop line 92% of the 1,966,080-byte OTA slot.
- Every state on the page carries a glyph (✔ `&#10004;` / ✖ `&#10006;` / ⚠ `&#9888;`) and words, never color alone.
- US English. Commit after each task; the message says why the obvious alternative was rejected. End messages with the `Co-Authored-By` line the session gives.
- Build check used by every firmware task: `pio run -e esp32-s3-devkitc-1` must end in `SUCCESS`. Host tests: `pio test -e native` must end with every test `PASSED`.

## File map

| File | Change | Responsibility |
|---|---|---|
| `firmware/src/core/settings/SettingsValidation.h` | modify | New parsers: `maskUrl`, `parseWebUrl`, `parseHost`, `parsePort`, `parseOptionalText`, `parseTopic`, `parsePassword` |
| `test/test_validation/test_main.cpp` | modify | Tests for the above |
| `firmware/src/widgets/webdatawidget/WebDataPolicy.h` | create | Interval clamp, size and timeout constants |
| `firmware/src/widgets/mqttwidget/MqttPolicy.h` | create | Retry timing, PubSubClient state → reason text |
| `test/test_custompages/test_main.cpp` | create | Tests for both policy headers |
| `platformio.ini` | modify | `[env:native]` include paths for the two widget folders |
| `firmware/src/core/settings/Settings.h/.cpp` | modify | New fields, defaults, load, save |
| `firmware/src/widgets/webdatawidget/WebDataWidget.h/.cpp` | modify | Runtime name, bounded fetch, last-good fallback, status |
| `firmware/src/widgets/mqttwidget/MQTTWidget.h/.cpp` | modify | Settings-driven, non-blocking connect, background `service()`, row layout, draw only when current, status |
| `firmware/src/core/widget/WidgetSet.h/.cpp` | modify | `MAX_WIDGETS` 5 → 8, `indexOf()` |
| `firmware/src/main.cpp` | modify | Register both pages from Settings; call `service()` |
| `firmware/src/core/settings/SettingsPage.cpp` | modify | Custom pages fieldset, POST handling, status lines |
| `firmware/config/config.h.template`, `secrets.h.template` | modify | Defines are now defaults |
| `firmware/src/widgets/mqttwidget/README.md` | modify | Points at the settings page; layout topic is the advanced path |
| `docs/CUSTOM-PAGES.md` | create | Member guide: JSON format, sample, MQTT + Home Assistant recipe, test record |
| `docs/custom-pages/sample.json` | create | Sample Web Data page |
| `docs/features.html`, `docs/REQUIREMENTS.md` | modify | Feature sheet and requirements |

Why `MAX_WIDGETS` must rise: it is 5 today. An orb with Clock + Stocks + two Tempest pages already holds 4, so Web Data + MQTT would make 6 and `WidgetSet::add` would drop the sixth with only a serial line. Upper bound with everything on: Clock, Parqet, Stocks, two weather pages, Web Data, the second Web Data define, MQTT = 8.

---

### Task 1: Settings validation for the new fields

**Files:**
- Modify: `firmware/src/core/settings/SettingsValidation.h` (append inside `namespace sv`, before the closing `} // namespace sv`)
- Test: `test/test_validation/test_main.cpp`

**Interfaces:**
- Consumes: existing `sv::trim`, `sv::parseInt`, `sv::normalizeText`.
- Produces (all `inline`, in `namespace sv`):
  - `std::string maskUrl(const std::string &url)`
  - `bool parseWebUrl(const std::string &in, const std::string &stored, std::string &out, std::string &err)`
  - `bool parseHost(const std::string &in, std::string &out, std::string &err)`
  - `bool parsePort(const std::string &in, int &out, std::string &err)`
  - `bool parseOptionalText(const std::string &in, size_t maxLen, std::string &out, std::string &err)`
  - `bool parseTopic(const std::string &in, std::string &out, std::string &err)`
  - `bool parsePassword(const std::string &in, bool clear, const std::string &stored, std::string &out, std::string &err)`
  - `constexpr const char *URL_MASK = "?\xE2\x80\xA2\xE2\x80\xA2\xE2\x80\xA2";` (`?•••` in UTF-8)

- [ ] **Step 1: Write the failing tests**

Add to `test/test_validation/test_main.cpp`, above `int main()`:

```cpp
// --- custom pages ------------------------------------------------------------

void test_mask_url() {
    TEST_ASSERT_EQUAL_STRING("https://example.com/a.json", maskUrl("https://example.com/a.json").c_str());
    TEST_ASSERT_EQUAL_STRING("https://example.com/a.json?\xE2\x80\xA2\xE2\x80\xA2\xE2\x80\xA2",
                             maskUrl("https://example.com/a.json?key=SECRET&x=1").c_str());
    TEST_ASSERT_EQUAL_STRING("", maskUrl("").c_str());
}

void test_web_url() {
    std::string out, err;
    const std::string stored = "https://example.com/a.json?key=SECRET";
    // blank clears
    TEST_ASSERT_TRUE(parseWebUrl("  ", stored, out, err));
    TEST_ASSERT_EQUAL_STRING("", out.c_str());
    // the masked value sent back unchanged keeps the stored URL, secret included
    TEST_ASSERT_TRUE(parseWebUrl(maskUrl(stored), stored, out, err));
    TEST_ASSERT_EQUAL_STRING(stored.c_str(), out.c_str());
    // a new URL replaces it
    TEST_ASSERT_TRUE(parseWebUrl(" http://example.org/b ", stored, out, err));
    TEST_ASSERT_EQUAL_STRING("http://example.org/b", out.c_str());
    TEST_ASSERT_TRUE(parseWebUrl("HTTPS://example.org/b", stored, out, err));
    // an edited path that kept the mask cannot be rebuilt
    out = "unchanged";
    TEST_ASSERT_FALSE(parseWebUrl("https://example.com/other.json?\xE2\x80\xA2\xE2\x80\xA2\xE2\x80\xA2", stored, out, err));
    TEST_ASSERT_EQUAL_STRING("unchanged", out.c_str()); // out untouched on failure
    TEST_ASSERT_FALSE(err.empty());
    TEST_ASSERT_FALSE(parseWebUrl("ftp://example.com/a", stored, out, err));
    TEST_ASSERT_FALSE(parseWebUrl("example.com/a.json", stored, out, err));
    TEST_ASSERT_FALSE(parseWebUrl("https://", stored, out, err)); // no host
    TEST_ASSERT_FALSE(parseWebUrl("https://exa mple.com/", stored, out, err)); // space
    TEST_ASSERT_FALSE(parseWebUrl("https://example.com/" + std::string(250, 'a'), stored, out, err)); // 270 > 256
}

void test_host() {
    std::string out, err;
    TEST_ASSERT_TRUE(parseHost("", out, err));
    TEST_ASSERT_EQUAL_STRING("", out.c_str());
    TEST_ASSERT_TRUE(parseHost(" broker.local ", out, err));
    TEST_ASSERT_EQUAL_STRING("broker.local", out.c_str());
    TEST_ASSERT_TRUE(parseHost("10.0.0.5", out, err));
    TEST_ASSERT_TRUE(parseHost("my-broker", out, err));
    TEST_ASSERT_FALSE(parseHost("mqtt://broker", out, err)); // scheme typed in
    TEST_ASSERT_FALSE(parseHost("broker:1883", out, err)); // port goes in its own field
    TEST_ASSERT_FALSE(parseHost("-broker", out, err));
    TEST_ASSERT_FALSE(parseHost("broker.", out, err));
    TEST_ASSERT_FALSE(parseHost(std::string(65, 'a'), out, err));
}

void test_port() {
    int out = 0;
    std::string err;
    TEST_ASSERT_TRUE(parsePort("1883", out, err));
    TEST_ASSERT_EQUAL(1883, out);
    TEST_ASSERT_TRUE(parsePort("1", out, err));
    TEST_ASSERT_TRUE(parsePort("65535", out, err));
    TEST_ASSERT_FALSE(parsePort("0", out, err));
    TEST_ASSERT_FALSE(parsePort("65536", out, err));
    TEST_ASSERT_FALSE(parsePort("", out, err));
    TEST_ASSERT_FALSE(parsePort("18a3", out, err));
}

void test_optional_text() {
    std::string out, err;
    TEST_ASSERT_TRUE(parseOptionalText("   ", 8, out, err));
    TEST_ASSERT_EQUAL_STRING("", out.c_str());
    TEST_ASSERT_TRUE(parseOptionalText(" kWh ", 8, out, err));
    TEST_ASSERT_EQUAL_STRING("kWh", out.c_str());
    TEST_ASSERT_FALSE(parseOptionalText("123456789", 8, out, err)); // 9 > 8
}

void test_topic() {
    std::string out, err;
    TEST_ASSERT_TRUE(parseTopic("", out, err));
    TEST_ASSERT_EQUAL_STRING("", out.c_str());
    TEST_ASSERT_TRUE(parseTopic(" homeassistant/statestream/sensor/outdoor_temp/state ", out, err));
    TEST_ASSERT_EQUAL_STRING("homeassistant/statestream/sensor/outdoor_temp/state", out.c_str());
    TEST_ASSERT_FALSE(parseTopic("home/#", out, err));
    TEST_ASSERT_FALSE(parseTopic("home/+/temp", out, err));
    TEST_ASSERT_FALSE(parseTopic(std::string(129, 'a'), out, err));
    TEST_ASSERT_FALSE(parseTopic("a\tb", out, err));
}

void test_password() {
    std::string out, err;
    TEST_ASSERT_TRUE(parsePassword("", false, "old", out, err)); // blank keeps
    TEST_ASSERT_EQUAL_STRING("old", out.c_str());
    TEST_ASSERT_TRUE(parsePassword("new pass", false, "old", out, err)); // spaces kept, not trimmed
    TEST_ASSERT_EQUAL_STRING("new pass", out.c_str());
    TEST_ASSERT_TRUE(parsePassword("ignored", true, "old", out, err)); // clear wins
    TEST_ASSERT_EQUAL_STRING("", out.c_str());
    TEST_ASSERT_FALSE(parsePassword(std::string(65, 'p'), false, "old", out, err));
    TEST_ASSERT_FALSE(parsePassword("a\nb", false, "old", out, err));
}
```

And register them in `main()` after `RUN_TEST(test_station_label);`:

```cpp
    RUN_TEST(test_mask_url);
    RUN_TEST(test_web_url);
    RUN_TEST(test_host);
    RUN_TEST(test_port);
    RUN_TEST(test_optional_text);
    RUN_TEST(test_topic);
    RUN_TEST(test_password);
```

- [ ] **Step 2: Run the tests to verify they fail**

Run: `pio test -e native -f test_validation`
Expected: compile error, `'maskUrl' was not declared in this scope` (and the other new names).

- [ ] **Step 3: Implement**

Append inside `namespace sv` in `SettingsValidation.h`, just before `} // namespace sv`:

```cpp
// --- custom pages (docs/superpowers/specs/2026-10-17-custom-pages-design.md) --

// What the settings page shows in place of a URL's query string, which often
// carries an API key. UTF-8 for "?•••"; the page is served as utf-8.
constexpr const char *URL_MASK = "?\xE2\x80\xA2\xE2\x80\xA2\xE2\x80\xA2";

inline std::string maskUrl(const std::string &url) {
    size_t q = url.find('?');
    return q == std::string::npos ? url : url.substr(0, q) + URL_MASK;
}

// Web Data URL. Blank = no page. The masked form of the stored URL, sent back
// unchanged, keeps the stored URL; anything else replaces it.
inline bool parseWebUrl(const std::string &in, const std::string &stored, std::string &out, std::string &err) {
    std::string s = trim(in);
    if (s.empty()) {
        out = "";
        return true;
    }
    if (!stored.empty() && s == maskUrl(stored)) {
        out = stored;
        return true;
    }
    if (s.find(URL_MASK) != std::string::npos) {
        err = "The part after ? is hidden, so it cannot be kept with a changed address - type the whole address";
        return false;
    }
    if (s.size() > 256) {
        err = "At most 256 characters";
        return false;
    }
    std::string lower;
    for (char c : s.substr(0, 8)) {
        lower += (char)tolower((unsigned char)c);
    }
    size_t hostStart;
    if (lower.compare(0, 7, "http://") == 0) {
        hostStart = 7;
    } else if (lower.compare(0, 8, "https://") == 0) {
        hostStart = 8;
    } else {
        err = "Must start with http:// or https://";
        return false;
    }
    for (char c : s) {
        if ((unsigned char)c <= 0x20 || c == 0x7F) {
            err = "No spaces or control characters";
            return false;
        }
    }
    size_t hostEnd = s.find_first_of("/?:#", hostStart);
    if ((hostEnd == std::string::npos ? s.size() : hostEnd) == hostStart) {
        err = "No host name after " + s.substr(0, hostStart);
        return false;
    }
    out = s;
    return true;
}

// MQTT broker: blank = no page, otherwise a host name or IPv4 address
inline bool parseHost(const std::string &in, std::string &out, std::string &err) {
    std::string s = trim(in);
    if (s.empty()) {
        out = "";
        return true;
    }
    if (s.size() > 64) {
        err = "At most 64 characters";
        return false;
    }
    for (char c : s) {
        if (!(isalnum((unsigned char)c) || c == '.' || c == '-')) {
            err = "Host name or IP address only - letters, digits, dots and dashes (no mqtt:// and no :port)";
            return false;
        }
    }
    if (s.front() == '.' || s.front() == '-' || s.back() == '.' || s.back() == '-') {
        err = "Cannot start or end with a dot or dash";
        return false;
    }
    out = s;
    return true;
}

inline bool parsePort(const std::string &in, int &out, std::string &err) {
    int v;
    if (!parseInt(in, v) || v < 1 || v > 65535) {
        err = "Enter a port from 1 to 65535 (MQTT is usually 1883)";
        return false;
    }
    out = v;
    return true;
}

// Blank allowed; otherwise normalizeText's rules
inline bool parseOptionalText(const std::string &in, size_t maxLen, std::string &out, std::string &err) {
    if (trim(in).empty()) {
        out = "";
        return true;
    }
    return normalizeText(in, maxLen, out, err);
}

// One MQTT topic to subscribe to. Blank leaves that orb empty. Wildcards are
// refused: one orb shows one value.
inline bool parseTopic(const std::string &in, std::string &out, std::string &err) {
    std::string s = trim(in);
    if (s.size() > 128) {
        err = "At most 128 characters";
        return false;
    }
    for (char c : s) {
        if ((unsigned char)c < 0x20 || c == 0x7F) {
            err = "Contains a control character (tab or line break)";
            return false;
        }
        if (c == '#' || c == '+') {
            err = "No # or + wildcards - one topic per orb";
            return false;
        }
    }
    out = s;
    return true;
}

// Write-only: the page never shows the stored password. Blank keeps it,
// `clear` (the checkbox) removes it. Not trimmed - a space may be part of it.
inline bool parsePassword(const std::string &in, bool clear, const std::string &stored, std::string &out, std::string &err) {
    if (clear) {
        out = "";
        return true;
    }
    if (in.empty()) {
        out = stored;
        return true;
    }
    if (in.size() > 64) {
        err = "At most 64 characters";
        return false;
    }
    for (char c : in) {
        if ((unsigned char)c < 0x20 || c == 0x7F) {
            err = "Contains a control character (tab or line break)";
            return false;
        }
    }
    out = in;
    return true;
}
```

- [ ] **Step 4: Run the tests to verify they pass**

Run: `pio test -e native -f test_validation`
Expected: every test `PASSED`, including the seven new ones.

- [ ] **Step 5: Commit**

```bash
git add firmware/src/core/settings/SettingsValidation.h test/test_validation/test_main.cpp
git commit -m "settings: validation for the Web Data and MQTT fields

URL query strings are masked as ?••• and the unchanged mask keeps the stored
URL, so a key in the query string never goes back to the browser. Rejected
a separate 'keep URL' checkbox: one field that round-trips is less to explain."
```

---

### Task 2: Pure policies for Web Data refresh and MQTT retry

**Files:**
- Create: `firmware/src/widgets/webdatawidget/WebDataPolicy.h`
- Create: `firmware/src/widgets/mqttwidget/MqttPolicy.h`
- Create: `test/test_custompages/test_main.cpp`
- Modify: `platformio.ini` (`[env:native]` `build_flags`)

**Interfaces:**
- Produces:
  - `WebDataPolicy.h`: `constexpr uint32_t WEBDATA_MIN_INTERVAL_MS = 10000; WEBDATA_DEFAULT_INTERVAL_MS = 60000; WEBDATA_TIMEOUT_MS = 5000; constexpr size_t WEBDATA_MAX_BYTES = 16 * 1024;` and `uint32_t webDataInterval(bool present, long ms)`
  - `MqttPolicy.h`: `constexpr uint32_t MQTT_RETRY_MS = 30000; constexpr int MQTT_SOCKET_TIMEOUT_S = 3;`, `bool mqttShouldAttempt(bool attemptedBefore, uint32_t now, uint32_t lastAttempt)`, `const char *mqttStateText(int state)`

- [ ] **Step 1: Write the failing tests**

Create `test/test_custompages/test_main.cpp`:

```cpp
// Host tests for WebDataPolicy.h and MqttPolicy.h:  pio test -e native
#include "MqttPolicy.h"
#include "WebDataPolicy.h"
#include <string>
#include <unity.h>

void setUp() {}
void tearDown() {}

void test_interval_missing_is_60s() {
    TEST_ASSERT_EQUAL_UINT32(60000, webDataInterval(false, 0));
}

void test_interval_clamped_to_10s() {
    TEST_ASSERT_EQUAL_UINT32(10000, webDataInterval(true, 2000));
    TEST_ASSERT_EQUAL_UINT32(10000, webDataInterval(true, 1000)); // upstream's old default
    TEST_ASSERT_EQUAL_UINT32(10000, webDataInterval(true, -5));
}

void test_interval_kept_when_long_enough() {
    TEST_ASSERT_EQUAL_UINT32(300000, webDataInterval(true, 300000));
    TEST_ASSERT_EQUAL_UINT32(10000, webDataInterval(true, 10000));
}

void test_first_attempt_is_immediate() {
    TEST_ASSERT_TRUE(mqttShouldAttempt(false, 0, 0));
    TEST_ASSERT_TRUE(mqttShouldAttempt(false, 123456, 0));
}

void test_no_attempt_within_30s() {
    TEST_ASSERT_FALSE(mqttShouldAttempt(true, 1000, 1000));
    TEST_ASSERT_FALSE(mqttShouldAttempt(true, 30999, 1000));
    TEST_ASSERT_TRUE(mqttShouldAttempt(true, 31000, 1000));
}

void test_retry_survives_millis_wrap() {
    // millis() wraps after ~49.7 days; unsigned subtraction must still work
    TEST_ASSERT_FALSE(mqttShouldAttempt(true, 10000, 0xFFFFF000u));
    TEST_ASSERT_TRUE(mqttShouldAttempt(true, 40000, 0xFFFFF000u));
}

void test_state_text() {
    TEST_ASSERT_EQUAL_STRING("connected", mqttStateText(0));
    TEST_ASSERT_EQUAL_STRING("can't reach broker", mqttStateText(-2));
    TEST_ASSERT_EQUAL_STRING("can't reach broker", mqttStateText(-4));
    TEST_ASSERT_EQUAL_STRING("connection lost", mqttStateText(-3));
    TEST_ASSERT_EQUAL_STRING("login refused", mqttStateText(4));
    TEST_ASSERT_EQUAL_STRING("login refused", mqttStateText(5));
    TEST_ASSERT_EQUAL_STRING("broker refused the connection", mqttStateText(3));
    TEST_ASSERT_EQUAL_STRING("not connected", mqttStateText(-1));
    TEST_ASSERT_EQUAL_STRING("not connected", mqttStateText(99));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_interval_missing_is_60s);
    RUN_TEST(test_interval_clamped_to_10s);
    RUN_TEST(test_interval_kept_when_long_enough);
    RUN_TEST(test_first_attempt_is_immediate);
    RUN_TEST(test_no_attempt_within_30s);
    RUN_TEST(test_retry_survives_millis_wrap);
    RUN_TEST(test_state_text);
    return UNITY_END();
}
```

In `platformio.ini`, `[env:native]`, extend `build_flags` to:

```ini
build_flags = -std=c++17 -I firmware/src/core/settings -I firmware/src/core/wifi -I firmware/src/widgets/weatherwidget -I firmware/src/widgets/webdatawidget -I firmware/src/widgets/mqttwidget
```

- [ ] **Step 2: Run the tests to verify they fail**

Run: `pio test -e native -f test_custompages`
Expected: compile error, `MqttPolicy.h: No such file or directory`.

- [ ] **Step 3: Implement**

Create `firmware/src/widgets/webdatawidget/WebDataPolicy.h`:

```cpp
#pragma once
// Web Data fetch limits. std only, so it is covered by `pio test -e native`
// (test/test_custompages).

#include <cstddef>
#include <cstdint>

// Upstream honored any "interval", default 1 s: a page left on screen fetched
// every second. Clamped so one member's JSON cannot keep the orb busy.
constexpr uint32_t WEBDATA_MIN_INTERVAL_MS = 10000;
constexpr uint32_t WEBDATA_DEFAULT_INTERVAL_MS = 60000;
constexpr uint32_t WEBDATA_TIMEOUT_MS = 5000; // connect, and each read
constexpr size_t WEBDATA_MAX_BYTES = 16 * 1024; // the SuperMini has no PSRAM

// Refresh period from the reply's "interval" (ms, as upstream)
inline uint32_t webDataInterval(bool present, long ms) {
    if (!present) {
        return WEBDATA_DEFAULT_INTERVAL_MS;
    }
    if (ms < (long)WEBDATA_MIN_INTERVAL_MS) {
        return WEBDATA_MIN_INTERVAL_MS;
    }
    return (uint32_t)ms;
}
```

Create `firmware/src/widgets/mqttwidget/MqttPolicy.h`:

```cpp
#pragma once
// MQTT connect timing. std only, so it is covered by `pio test -e native`
// (test/test_custompages).
//
// Upstream reconnect() looped `while (!connected)` with a 5 s delay: a
// mistyped broker froze the orb - no buttons, and no settings page to fix it on.

#include <cstdint>

constexpr uint32_t MQTT_RETRY_MS = 30000;
constexpr int MQTT_SOCKET_TIMEOUT_S = 3; // PubSubClient::setSocketTimeout

// First attempt at once, then no more than one per MQTT_RETRY_MS.
// Unsigned subtraction, so a millis() wrap does not stall it.
inline bool mqttShouldAttempt(bool attemptedBefore, uint32_t now, uint32_t lastAttempt) {
    return !attemptedBefore || (uint32_t)(now - lastAttempt) >= MQTT_RETRY_MS;
}

// PubSubClient::state() in words for the settings page. Numbers are
// PubSubClient.h's MQTT_* constants, written out so this compiles on the host.
inline const char *mqttStateText(int state) {
    switch (state) {
    case 0:
        return "connected";
    case -4: // MQTT_CONNECTION_TIMEOUT
    case -2: // MQTT_CONNECT_FAILED
        return "can't reach broker";
    case -3: // MQTT_CONNECTION_LOST
        return "connection lost";
    case 4: // MQTT_CONNECT_BAD_CREDENTIALS
    case 5: // MQTT_CONNECT_UNAUTHORIZED
        return "login refused";
    case 1: // MQTT_CONNECT_BAD_PROTOCOL
    case 2: // MQTT_CONNECT_BAD_CLIENT_ID
    case 3: // MQTT_CONNECT_UNAVAILABLE
        return "broker refused the connection";
    default:
        return "not connected";
    }
}
```

- [ ] **Step 4: Run all host tests**

Run: `pio test -e native`
Expected: `test_custompages`, `test_validation`, `test_roam`, `test_sunmoon`, `test_tempest` all `PASSED`.

- [ ] **Step 5: Commit**

```bash
git add firmware/src/widgets/webdatawidget/WebDataPolicy.h firmware/src/widgets/mqttwidget/MqttPolicy.h test/test_custompages/test_main.cpp platformio.ini
git commit -m "custom pages: pure refresh and retry policies with host tests

Same pattern as RoamPolicy.h. Rejected testing the timing inside the widgets:
they need WiFi and a broker, so the rule would only ever be checked on the orb."
```

---

### Task 3: Settings fields, defaults, load and save

**Files:**
- Modify: `firmware/src/core/settings/Settings.h` (struct `SettingsValues`)
- Modify: `firmware/src/core/settings/Settings.cpp` (`defaults()`, `load()`, `save()`)

**Interfaces:**
- Produces, new `SettingsValues` members (later tasks use these exact names):
  ```cpp
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
  ```
- NVS keys: `wdname wdurl mqname mqhost mqport mquser mqpass mqt1..mqt5 mql1..mql5 mqu1..mqu5 mqsetup`.

No host test: `Settings.cpp` needs Preferences. The pure parts were tested in Task 1; this task is checked by the build and by Task 9's on-orb tests.

- [ ] **Step 1: Add the fields**

In `Settings.h`, after `std::string hidden; ...` inside `struct SettingsValues`, add the block from **Interfaces** above, preceded by:

```cpp
    // Custom pages (docs/superpowers/specs/2026-10-17-custom-pages-design.md).
    // A page is registered at boot only when its URL / host is set.
```

- [ ] **Step 2: Defaults from the old defines**

In `Settings.cpp` `defaults()`, before `return d;`:

```cpp
    // Custom pages: the old build-time defines are now defaults, so a build
    // that sets them keeps working with nothing saved
    d.wdname = "Web Data";
#ifdef WEB_DATA_WIDGET_URL
    d.wdurl = WEB_DATA_WIDGET_URL;
#else
    d.wdurl = "";
#endif
    d.mqname = "MQTT";
#ifdef MQTT_WIDGET_HOST
    d.mqhost = MQTT_WIDGET_HOST;
#else
    d.mqhost = "";
#endif
#ifdef MQTT_WIDGET_PORT
    d.mqport = MQTT_WIDGET_PORT;
#else
    d.mqport = 1883;
#endif
#ifdef MQTT_WIDGET_USER
    d.mquser = MQTT_WIDGET_USER;
#else
    d.mquser = "";
#endif
#ifdef MQTT_WIDGET_PASS
    d.mqpass = MQTT_WIDGET_PASS;
#else
    d.mqpass = "";
#endif
#ifdef MQTT_SETUP_TOPIC
    d.mqsetup = MQTT_SETUP_TOPIC;
#else
    d.mqsetup = "";
#endif
    for (int i = 0; i < 5; i++) {
        d.mqt[i] = d.mql[i] = d.mqu[i] = "";
    }
```

- [ ] **Step 3: Load**

In `load()`, after `v.hidden = p.getString("hidden", ...)`:

```cpp
            // Custom pages: added after schema 1 without a bump, like tstn1
            v.wdname = p.getString("wdname", v.wdname.c_str()).c_str();
            v.wdurl = p.getString("wdurl", v.wdurl.c_str()).c_str();
            v.mqname = p.getString("mqname", v.mqname.c_str()).c_str();
            v.mqhost = p.getString("mqhost", v.mqhost.c_str()).c_str();
            v.mqport = p.getInt("mqport", v.mqport);
            v.mquser = p.getString("mquser", v.mquser.c_str()).c_str();
            v.mqpass = p.getString("mqpass", v.mqpass.c_str()).c_str();
            v.mqsetup = p.getString("mqsetup", v.mqsetup.c_str()).c_str();
            for (int i = 0; i < 5; i++) {
                char k[8];
                snprintf(k, sizeof(k), "mqt%d", i + 1);
                v.mqt[i] = p.getString(k, v.mqt[i].c_str()).c_str();
                snprintf(k, sizeof(k), "mql%d", i + 1);
                v.mql[i] = p.getString(k, v.mql[i].c_str()).c_str();
                snprintf(k, sizeof(k), "mqu%d", i + 1);
                v.mqu[i] = p.getString(k, v.mqu[i].c_str()).c_str();
            }
```

- [ ] **Step 4: Save**

In `save()`, take `p.putInt("schema", SCHEMA)` out of the `&&` chain so the chain ends with the `hidden` line:

```cpp
              p.putString("hidden", v.hidden.c_str()) == v.hidden.size();
    // put* returns bytes written; strings compare against their own length
    auto putStr = [&p](const char *k, const std::string &s) { return p.putString(k, s.c_str()) == s.size(); };
    ok = ok && putStr("wdname", v.wdname) && putStr("wdurl", v.wdurl) && putStr("mqname", v.mqname) &&
         putStr("mqhost", v.mqhost) && p.putInt("mqport", v.mqport) && putStr("mquser", v.mquser) &&
         putStr("mqpass", v.mqpass) && putStr("mqsetup", v.mqsetup);
    for (int i = 0; ok && i < 5; i++) {
        char k[8];
        snprintf(k, sizeof(k), "mqt%d", i + 1);
        ok = putStr(k, v.mqt[i]);
        snprintf(k, sizeof(k), "mql%d", i + 1);
        ok = ok && putStr(k, v.mql[i]);
        snprintf(k, sizeof(k), "mqu%d", i + 1);
        ok = ok && putStr(k, v.mqu[i]);
    }
    // schema last: a save that dies part-way leaves no schema key on a fresh
    // orb, so load() keeps using config.h
    ok = ok && p.putInt("schema", SCHEMA);
    p.end();
    return ok;
```

(Delete the old `p.end(); return ok;` and the old schema comment inside the chain so each appears once.)

- [ ] **Step 5: Build**

Run: `pio run -e esp32-s3-devkitc-1`
Expected: `SUCCESS`.

- [ ] **Step 6: Commit**

```bash
git add firmware/src/core/settings/Settings.h firmware/src/core/settings/Settings.cpp
git commit -m "settings: store the Web Data and MQTT page fields in NVS

No schema bump: absent keys fall back to the old config.h/secrets.h defines,
which become defaults. Rejected a schema 2 migration - nothing changes meaning,
and a bump would throw away every orb's saved settings for no gain."
```

---

### Task 4: Web Data — runtime name, bounded fetch, last-good fallback, status

**Files:**
- Modify: `firmware/src/widgets/webdatawidget/WebDataWidget.h`
- Modify: `firmware/src/widgets/webdatawidget/WebDataWidget.cpp`

**Interfaces:**
- Consumes: `webDataInterval`, `WEBDATA_*` from `WebDataPolicy.h` (Task 2).
- Produces:
  - `WebDataWidget(ScreenManager &manager, String url, String name)`
  - `struct WebDataStatus { bool attempted = false; bool ok = false; int code = 0; uint32_t ms = 0; uint32_t at = 0; String error; };`
  - `static const WebDataStatus &WebDataWidget::status();` — the most recent fetch of the settings-configured page

- [ ] **Step 1: Header**

Replace the class in `WebDataWidget.h` with:

```cpp
// The last fetch, for the settings page's status line (no serial on the SuperMini)
struct WebDataStatus {
    bool attempted = false;
    bool ok = false;
    int code = 0; // HTTP code, or HTTPClient's negative error
    uint32_t ms = 0; // the whole fetch
    uint32_t at = 0; // millis() when it finished
    String error; // when !ok: "HTTP 404", "bad JSON (...)", "too big", "timeout", ...
};

class WebDataWidget : public Widget {
public:
    WebDataWidget(ScreenManager &manager, String url, String name);
    ~WebDataWidget() override;
    void setup() override;
    void update(bool force = false) override;
    void draw(bool force = false) override;
    void buttonPressed(uint8_t buttonId, ButtonState state) override;
    String getName() override;
    static const WebDataStatus &status();

private:
    bool fetch(); // one bounded GET; true when the data was applied
    void showProblem(const String &reason); // center orb, only before any good data

    unsigned long m_lastUpdate = 0;
    unsigned long m_updateDelay = WEBDATA_DEFAULT_INTERVAL_MS;
    String httpRequestAddress;
    String m_name;
    bool m_hasData = false;
    bool m_showingProblem = false;
    WebDataModel m_obj[5];
    int32_t m_defaultColor = TFT_WHITE;
    int32_t m_defaultBackground = TFT_BLACK;
    static WebDataStatus s_status;
};
```

Add `#include "WebDataPolicy.h"` to the includes.

- [ ] **Step 2: A stream that stops at 16 KB**

At the top of `WebDataWidget.cpp`, after the include:

```cpp
WebDataStatus WebDataWidget::s_status;

// Hands ArduinoJson at most `limit` bytes. deserializeJson reads from a
// Stream, so the reply is never held whole in a String (upstream did
// http.getString(), unbounded). Blocking waits happen on the inner stream,
// under its own timeout; this one's is 0 so it never adds a wait of its own.
class LimitedStream : public Stream {
  public:
    LimitedStream(Stream &in, size_t limit) : m_in(in), m_left(limit) { setTimeout(0); }
    int available() override { return m_left ? m_in.available() : 0; }
    int read() override {
        if (m_left == 0) {
            m_exceeded = true;
            return -1;
        }
        char c;
        if (m_in.readBytes(&c, 1) != 1) {
            return -1;
        }
        m_left--;
        return (uint8_t)c;
    }
    int peek() override { return m_left ? m_in.peek() : -1; }
    size_t write(uint8_t) override { return 0; }
    bool exceeded() const { return m_exceeded; }

  private:
    Stream &m_in;
    size_t m_left;
    bool m_exceeded = false;
};
```

- [ ] **Step 3: Constructor, name, status**

```cpp
WebDataWidget::WebDataWidget(ScreenManager &manager, String url, String name) : Widget(manager) {
    httpRequestAddress = url;
    m_name = name;
    m_lastUpdate = 0;
    for (int i = 0; i < 5; i++) {
        m_obj[i] = WebDataModel();
    }
}

const WebDataStatus &WebDataWidget::status() {
    return s_status;
}

String WebDataWidget::getName() {
    return m_name;
}
```

- [ ] **Step 4: Update and fetch**

Replace `update()` with:

```cpp
void WebDataWidget::update(bool force) {
    if (!force && m_lastUpdate != 0 && (millis() - m_lastUpdate) < m_updateDelay) {
        return;
    }
    fetch();
    // Also after a failure: upstream left m_lastUpdate alone on error, so a
    // dead URL was retried on every pass of loop()
    m_lastUpdate = millis();
    if (m_lastUpdate == 0) {
        m_lastUpdate = 1; // 0 means "never fetched"
    }
}

bool WebDataWidget::fetch() {
    WebDataStatus st;
    st.attempted = true;
    uint32_t start = millis();
    HTTPClient http;
    http.useHTTP10(true); // no chunked encoding, so getStream() is plain JSON (as TempestSource)
    http.setConnectTimeout(WEBDATA_TIMEOUT_MS);
    http.setTimeout(WEBDATA_TIMEOUT_MS);
    http.begin(httpRequestAddress); // https without a CA check, the same path as the weather pages
    st.code = http.GET();
    if (st.code == 200) {
        int size = http.getSize(); // -1 when the server sends no Content-Length
        if (size > (int)WEBDATA_MAX_BYTES) {
            st.error = "too big";
        } else {
            LimitedStream in(http.getStream(), WEBDATA_MAX_BYTES);
            JsonDocument doc;
            DeserializationError e = deserializeJson(doc, in);
            if (in.exceeded()) {
                st.error = "too big";
            } else if (e) {
                st.error = String("bad JSON (") + e.c_str() + ")";
            } else {
                m_updateDelay = webDataInterval(doc["interval"].is<long>(), doc["interval"] | 0L);
                JsonArray array = doc["displays"].is<JsonArray>() ? doc["displays"].as<JsonArray>()
                                                                   : doc.as<JsonArray>(); // legacy: a bare array
                if (m_showingProblem) {
                    m_obj[2].setLabel("");
                    m_obj[2].setData("", m_defaultColor, m_defaultBackground);
                    m_obj[2].setInitializedStatus(false); // clear the problem text
                    m_showingProblem = false;
                }
                size_t n = array.size() < 5 ? array.size() : 5; // upstream indexed past m_obj[4]
                for (size_t i = 0; i < n; i++) {
                    m_obj[i].parseData(array[i].as<JsonObject>(), m_defaultColor, m_defaultBackground);
                }
                st.ok = true;
                m_hasData = true;
            }
        }
    } else if (st.code == HTTPC_ERROR_READ_TIMEOUT) {
        st.error = "timeout";
    } else {
        st.error = st.code > 0 ? String("HTTP ") + st.code : http.errorToString(st.code);
    }
    http.end();
    st.ms = millis() - start;
    st.at = millis();
    s_status = st;
    if (!st.ok) {
        Serial.printf("%s: %s\n", m_name.c_str(), st.error.c_str());
        if (!m_hasData) {
            showProblem(st.error);
        }
    }
    return st.ok;
}

// No good data yet: the page name and the reason on the center orb, drawn
// through the normal label/data path. Once data has arrived it stays on
// screen through later failures.
void WebDataWidget::showProblem(const String &reason) {
    m_obj[2].setLabel(m_name);
    m_obj[2].setData(reason, m_defaultColor, m_defaultBackground);
    m_showingProblem = true;
}
```

- [ ] **Step 5: Build**

Run: `pio run -e esp32-s3-devkitc-1`
Expected: build fails in `main.cpp` at `new WebDataWidget(*sm, WEB_DATA_WIDGET_URL)` only if `WEB_DATA_WIDGET_URL` is defined in your `secrets.h` (too few arguments). If it does, change that line to `new WebDataWidget(*sm, WEB_DATA_WIDGET_URL, "Web Data")` and `WEB_DATA_STOCK_WIDGET_URL`'s to `new WebDataWidget(*sm, WEB_DATA_STOCK_WIDGET_URL, "WebData")` (Task 6 replaces the first anyway). Then `SUCCESS`.

- [ ] **Step 6: Commit**

```bash
git add firmware/src/widgets/webdatawidget/WebDataWidget.h firmware/src/widgets/webdatawidget/WebDataWidget.cpp firmware/src/main.cpp
git commit -m "webdata: bounded fetch, 10 s floor, last good data kept, status

Parsed from the stream and cut at 16 KB rather than http.getString(): with no
PSRAM a large page in a String could exhaust the heap and restart the orb.
Rejected a Content-Length check alone: many servers send none."
```

---

### Task 5: MQTT — settings-driven, never blocks, row layout, background service

**Files:**
- Modify: `firmware/src/widgets/mqttwidget/MQTTWidget.h`
- Modify: `firmware/src/widgets/mqttwidget/MQTTWidget.cpp`

**Interfaces:**
- Consumes: `SettingsValues` fields from Task 3; `mqttShouldAttempt`, `mqttStateText`, `MQTT_SOCKET_TIMEOUT_S` from Task 2.
- Produces:
  - `MQTTWidget(ScreenManager &manager, const SettingsValues &s)`
  - `void MQTTWidget::service();` — call once per `loop()` pass while the page is in the rotation; never blocks longer than one connect attempt, at most once per 30 s
  - `struct MqttStatus { bool attempted = false; bool connected = false; String reason; uint32_t at = 0; uint32_t attemptMs = 0; uint32_t lastValueAt = 0; };`
  - `static const MqttStatus &MQTTWidget::status();`

Upstream drew an orb from inside the MQTT callback. Once `loop()` runs in the background that would paint MQTT values over whatever page is on screen, so values now only mark the orb dirty and `draw()` (called only for the current page) paints it.

- [ ] **Step 1: Header**

In `MQTTWidget.h`: add `#include "MqttPolicy.h"` and `#include "Settings.h"`. Add to `OrbConfig`:

```cpp
    bool autoLayout = false; // a settings-page row: label / value / unit, value fitted to the orb
    String value; // latest value shown
    bool dirty = true; // needs drawing the next time this page draws
```

Add before the class:

```cpp
// Connection state for the settings page's status line
struct MqttStatus {
    bool attempted = false;
    bool connected = false;
    String reason; // mqttStateText()
    uint32_t at = 0; // millis() of the last attempt
    uint32_t attemptMs = 0; // how long that attempt blocked
    uint32_t lastValueAt = 0; // millis() of the newest value, 0 = none yet
};
```

Change the public section to:

```cpp
    MQTTWidget(ScreenManager &manager, const SettingsValues &s);
    void setup() override;
    void update(bool force = false) override;
    void draw(bool force = false) override;
    void changeMode();
    void buttonPressed(uint8_t buttonId, ButtonState state) override;
    String getName() override;
    void service(); // keep connected and process messages; call from loop()
    static const MqttStatus &status();
```

In the private section replace `void reconnect();` with `void tryConnect();`, remove `std::map<String, String> orbDataMap;`, and add:

```cpp
    String m_name, m_user, m_pass, m_setupTopic;
    bool m_attempted = false;
    uint32_t m_lastAttempt = 0;
    bool m_wasConnected = false;
    static MqttStatus s_status;
    bool extractValue(const OrbConfig &orb, const String &message, String &out);
```

- [ ] **Step 2: Remove the build-time guard; constructor from settings**

In `MQTTWidget.cpp` delete the first line `#ifdef MQTT_WIDGET_HOST` and the final `#endif`, and un-indent the include. Replace the constructor with:

```cpp
MqttStatus MQTTWidget::s_status;

MQTTWidget::MQTTWidget(ScreenManager &manager, const SettingsValues &s)
    : Widget(manager), mqttHost(s.mqhost.c_str()), mqttPort((uint16_t)s.mqport), mqttClient(wifiClient),
      m_name(s.mqname.c_str()), m_user(s.mquser.c_str()), m_pass(s.mqpass.c_str()), m_setupTopic(s.mqsetup.c_str()) {
    instance = this;
    mqttClient.setServer(mqttHost.c_str(), mqttPort); // mqttHost is a member, so the pointer stays valid
    mqttClient.setBufferSize(2048);
    mqttClient.setSocketTimeout(MQTT_SOCKET_TIMEOUT_S);
    mqttClient.setCallback(staticCallback);

    // One orb per settings row with a topic; the others stay blank
    for (int i = 0; i < 5; i++) {
        if (s.mqt[i].empty()) {
            continue;
        }
        OrbConfig c;
        c.orbid = i;
        c.topicSrc = s.mqt[i].c_str();
        c.orbdesc = s.mql[i].c_str();
        c.orbvalunit = s.mqu[i].c_str();
        c.orbBgColor = TFT_BLACK;
        c.orbTextColor = TFT_WHITE;
        c.autoLayout = true;
        orbConfigs.push_back(c);
    }
}

const MqttStatus &MQTTWidget::status() {
    return s_status;
}
```

- [ ] **Step 3: Non-blocking connect and background service**

Replace `update()` and `reconnect()` with:

```cpp
void MQTTWidget::update(bool force) {
    service();
}

void MQTTWidget::service() {
    if (mqttClient.connected()) {
        mqttClient.loop();
        return;
    }
    if (m_wasConnected) { // dropped since the last pass
        m_wasConnected = false;
        s_status.connected = false;
        s_status.reason = mqttStateText(mqttClient.state());
    }
    uint32_t now = millis();
    if (!mqttShouldAttempt(m_attempted, now, m_lastAttempt)) {
        return;
    }
    m_attempted = true;
    m_lastAttempt = now;
    tryConnect();
}

// One attempt. Blocks for at most the TCP connect plus MQTT_SOCKET_TIMEOUT_S;
// test 5 in docs/CUSTOM-PAGES.md records the measured time (attemptMs).
void MQTTWidget::tryConnect() {
    String clientId = "info-orbs-" + String((uint32_t)ESP.getEfuseMac(), HEX); // stable, so a reconnect replaces the old session
    uint32_t start = millis();
    bool ok = m_user.length() ? mqttClient.connect(clientId.c_str(), m_user.c_str(), m_pass.length() ? m_pass.c_str() : nullptr)
                              : mqttClient.connect(clientId.c_str());
    s_status.attempted = true;
    s_status.at = millis();
    s_status.attemptMs = s_status.at - start;
    s_status.connected = ok;
    s_status.reason = mqttStateText(mqttClient.state());
    Serial.printf("MQTT %s: %s (%u ms)\n", mqttHost.c_str(), s_status.reason.c_str(), (unsigned)s_status.attemptMs);
    if (!ok) {
        return;
    }
    m_wasConnected = true;
    if (m_setupTopic.length()) {
        mqttClient.subscribe(m_setupTopic.c_str());
    }
    subscribeToOrbs(); // the rows, and after a reconnect whatever the layout topic set
}
```

- [ ] **Step 4: Callback marks orbs dirty instead of drawing**

Move the JSON-field traversal from the old `callback()` (the block from `JsonDocument dataDoc;` through the `if (!fieldValue.isNull()) { ... extractedValue = ... }` conversion) into this helper, unchanged in logic:

```cpp
// The orb's value from a message: the whole payload, or orb.jsonField from it
bool MQTTWidget::extractValue(const OrbConfig &orb, const String &message, String &out) {
    if (orb.jsonField.length() == 0) {
        out = message;
        return true;
    }
    JsonDocument dataDoc;
    if (deserializeJson(dataDoc, message)) {
        return false;
    }
    JsonVariant fieldValue = dataDoc.as<JsonVariant>();
    char path[100];
    strncpy(path, orb.jsonField.c_str(), sizeof(path) - 1);
    path[sizeof(path) - 1] = '\0'; // upstream strcpy could overrun
    char *token = strtok(path, ".");
    while (token != nullptr && !fieldValue.isNull()) {
        char *arrayBracket = strchr(token, '[');
        if (arrayBracket) {
            *arrayBracket = '\0';
            fieldValue = fieldValue[token];
            int index = atoi(arrayBracket + 1);
            if (!fieldValue.is<JsonArray>()) {
                return false;
            }
            fieldValue = fieldValue[index];
        } else {
            fieldValue = fieldValue[token];
        }
        token = strtok(nullptr, ".");
    }
    if (fieldValue.isNull()) {
        return false;
    }
    if (fieldValue.is<float>()) {
        out = String(fieldValue.as<float>(), 3);
    } else if (fieldValue.is<int>()) {
        out = String(fieldValue.as<int>());
    } else if (fieldValue.is<const char *>()) {
        out = String(fieldValue.as<const char *>());
    } else {
        out = fieldValue.as<String>();
    }
    return true;
}
```

Replace `callback()` with:

```cpp
void MQTTWidget::callback(char *topic, byte *payload, unsigned int length) {
    String message;
    message.reserve(length);
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    String receivedTopic(topic);
    if (m_setupTopic.length() && receivedTopic.equals(m_setupTopic)) {
        handleSetupMessage(message);
        return;
    }
    // Every orb on this topic (two rows may show the same value)
    for (auto &orb : orbConfigs) {
        if (!orb.topicSrc.equals(receivedTopic)) {
            continue;
        }
        String v;
        if (!extractValue(orb, message, v)) {
            Serial.println("MQTT: no value for orb " + String(orb.orbid) + " in " + receivedTopic);
            continue;
        }
        s_status.lastValueAt = millis();
        if (orb.value != v) {
            orb.value = v;
            orb.dirty = true; // drawn by draw(), only while this page is on screen
        }
    }
}
```

In `handleSetupMessage()`: delete the `orbDataMap.clear();` and `orbDataMap[config.topicSrc] = "";` lines, and replace the trailing `draw(true);` with:

```cpp
    // Replaces the settings rows until restart, as upstream. Drawn when the
    // page is next on screen, not from here.
    for (auto &o : orbConfigs) {
        o.dirty = true;
    }
```

- [ ] **Step 5: Draw only from draw(), with the row layout**

Replace `draw()` and `drawOrb()` with:

```cpp
void MQTTWidget::draw(bool force) {
    m_manager.setFont(DEFAULT_FONT);
    for (int i = 0; i < 5; i++) {
        OrbConfig *orb = nullptr;
        for (auto &o : orbConfigs) {
            if (o.orbid == i) {
                orb = &o;
                break;
            }
        }
        if (orb ? (force || orb->dirty) : force) {
            drawOrb(i);
        }
    }
}

void MQTTWidget::drawOrb(int orbid) {
    m_manager.selectScreen(orbid);
    OrbConfig *orb = nullptr;
    for (auto &o : orbConfigs) {
        if (o.orbid == orbid) {
            orb = &o;
            break;
        }
    }
    if (orb == nullptr) {
        m_manager.fillScreen(TFT_BLACK); // a row with no topic stays blank
        return;
    }
    orb->dirty = false;
    m_manager.fillScreen(orb->orbBgColor);
    m_manager.setFontColor(orb->orbTextColor, orb->orbBgColor);
    if (orb->autoLayout) {
        // Label on top, the value as large as fits in the middle, unit below.
        // "–" until the first value arrives.
        if (orb->orbdesc.length()) {
            m_manager.drawString(orb->orbdesc, ScreenCenterX, 62, 22, Align::MiddleCenter);
        }
        String v = orb->value.length() ? orb->value : String("\xE2\x80\x93");
        m_manager.drawFittedString(v, ScreenCenterX, ScreenCenterY, 190, 80, Align::MiddleCenter);
        if (orb->orbvalunit.length()) {
            m_manager.drawString(orb->orbvalunit, ScreenCenterX, 180, 22, Align::MiddleCenter);
        }
        return;
    }
    // Layout-topic orbs: upstream positions and size
    m_manager.drawString(orb->orbdesc, orb->xpostxt, orb->ypostxt, orb->orbsize, Align::MiddleCenter);
    m_manager.drawString(orb->value + orb->orbvalunit, orb->xposval, orb->yposval, orb->orbsize, Align::MiddleCenter);
}

String MQTTWidget::getName() {
    return m_name;
}
```

- [ ] **Step 6: Build**

Run: `pio run -e esp32-s3-devkitc-1`
Expected: errors only in `main.cpp` at the old `new MQTTWidget(*sm, MQTT_WIDGET_HOST, MQTT_WIDGET_PORT)` if your `config.h` defines `MQTT_WIDGET_HOST`; Task 6 replaces that line. If no MQTT define is set: `SUCCESS`. Also grep: `grep -n "MQTT_SETUP_TOPIC\|MQTT_WIDGET_USER\|MQTT_WIDGET_PASS\|orbDataMap" firmware/src/widgets/mqttwidget/MQTTWidget.cpp` must print nothing.

- [ ] **Step 7: Commit**

```bash
git add firmware/src/widgets/mqttwidget/MQTTWidget.h firmware/src/widgets/mqttwidget/MQTTWidget.cpp
git commit -m "mqtt: settings-driven, one connect attempt per 30 s, rows laid out

Replaces the while(!connected) reconnect that froze the orb on a wrong broker.
Values now mark orbs dirty and draw() paints them: rejected keeping the draw in
the callback, because loop() now runs while other pages are on screen."
```

---

### Task 6: Register the pages from settings; MQTT in the background

**Files:**
- Modify: `firmware/src/core/widget/WidgetSet.h`, `WidgetSet.cpp`
- Modify: `firmware/src/main.cpp`

**Interfaces:**
- Consumes: `WebDataWidget(ScreenManager&, String url, String name)` (Task 4), `MQTTWidget(ScreenManager&, const SettingsValues&)` and `service()` (Task 5), `SettingsValues::wdurl/wdname/mqhost` (Task 3).
- Produces: `int8_t WidgetSet::indexOf(Widget *w)` (−1 when not in the set); `MAX_WIDGETS` = 8.

- [ ] **Step 1: WidgetSet**

In `WidgetSet.h` change `#define MAX_WIDGETS 5` to:

```cpp
// Clock, Parqet, Stocks, two weather pages, Web Data, the second Web Data
// define, MQTT. At 5 an orb with two Tempest pages dropped the custom pages.
#define MAX_WIDGETS 8
```

Add under `bool shown(int8_t i);`:

```cpp
    int8_t indexOf(Widget *w); // -1 if add() refused it
```

In `WidgetSet.cpp` after `shown()`:

```cpp
int8_t WidgetSet::indexOf(Widget *w) {
    for (int8_t i = 0; i < m_widgetCount; i++) {
        if (m_widgets[i] == w) {
            return i;
        }
    }
    return -1;
}
```

- [ ] **Step 2: main.cpp includes and globals**

Replace

```cpp
#ifdef MQTT_WIDGET_HOST
    #include "mqttwidget/MQTTWidget.h"
#endif
```

with `#include "mqttwidget/MQTTWidget.h"` (keep the include list alphabetical with the others is not required; put it after `#include "icons.h"`). After `WidgetSet *widgetSet;` add:

```cpp
MQTTWidget *mqttWidget{nullptr};
bool mqttInRotation{false}; // stays connected only while its page can come up
```

- [ ] **Step 3: Registration**

Replace the three blocks from `#ifdef WEB_DATA_WIDGET_URL` through the `#ifdef MQTT_WIDGET_HOST ... #endif` block with:

```cpp
    // Custom pages: set up on /settings, registered only when configured.
    // Saving restarts the orb, so this runs again with the new values.
    const SettingsValues &cp = Settings::get();
    if (!cp.wdurl.empty()) {
        widgetSet->add(new WebDataWidget(*sm, cp.wdurl.c_str(), cp.wdname.c_str()));
    }
#ifdef WEB_DATA_STOCK_WIDGET_URL
    widgetSet->add(new WebDataWidget(*sm, WEB_DATA_STOCK_WIDGET_URL, "WebData"));
#endif
    if (!cp.mqhost.empty()) {
        mqttWidget = new MQTTWidget(*sm, cp);
        widgetSet->add(mqttWidget);
    }
```

After the line `settingsPage->setWidgets(widgetSet);` add:

```cpp
    if (mqttWidget) {
        // After every add(): shown() depends on how many pages are shown
        int8_t i = widgetSet->indexOf(mqttWidget);
        mqttInRotation = i >= 0 && widgetSet->shown(i);
    }
```

- [ ] **Step 4: Background service**

In `loop()`, right after `widgetSet->updateCurrent();`:

```cpp
        if (mqttInRotation) {
            mqttWidget->service(); // values current when the page comes up
        }
```

- [ ] **Step 5: Build, with and without a configured page**

Run: `pio run -e esp32-s3-devkitc-1`
Expected: `SUCCESS`. Note the `Flash:` line's percentage (expected near 87%, spec measurement); stop and report if it is at or above 92%.

- [ ] **Step 6: Commit**

```bash
git add firmware/src/core/widget/WidgetSet.h firmware/src/core/widget/WidgetSet.cpp firmware/src/main.cpp
git commit -m "main: register Web Data and MQTT pages from settings

Both compile into every build (measured 85 -> 87% of the OTA slot) and appear
only when a URL or broker is set. Rejected keeping the #ifdef switch: it needs
the rebuild this feature exists to remove."
```

---

### Task 7: Custom pages section on /settings

**Files:**
- Modify: `firmware/src/core/settings/SettingsPage.cpp`

**Interfaces:**
- Consumes: Task 1 parsers, Task 3 fields, `WebDataWidget::status()` (Task 4), `MQTTWidget::status()` (Task 5).
- Produces: form keys `wdname wdurl mqname mqhost mqport mquser mqpass mqpassclr mqt1..5 mql1..5 mqu1..5 mqsetup`.

- [ ] **Step 1: Includes and helpers**

Add includes after `#include "weatherwidget/TempestSource.h"`:

```cpp
#include "mqttwidget/MQTTWidget.h"
#include "webdatawidget/WebDataWidget.h"
```

After `stationRow()`'s `#endif`, add:

```cpp
static String ago(uint32_t at) {
    uint32_t mins = (millis() - at) / 60000;
    return mins == 0 ? String("under a minute ago") : mins < 120 ? String(mins) + " min ago" : String(mins / 60) + " h ago";
}

static String webDataStatusLine() {
    const WebDataStatus &st = WebDataWidget::status();
    if (!st.attempted) {
        return hint("&#9888; Not fetched since start-up (it fetches only while the page is ticked).");
    }
    String line = String(st.ok ? "&#10004; " : "&#10006; ") + "Last fetch " + ago(st.at) + ", took " + String(st.ms / 1000.0, 1) + " s";
    if (!st.ok) {
        line += " &mdash; " + esc(st.error.c_str());
    }
    return "<div class='" + String(st.ok ? "hint" : "err") + "'>" + line + "</div>";
}

static String mqttStatusLine() {
    const MqttStatus &st = MQTTWidget::status();
    if (!st.attempted) {
        return hint("&#9888; Not connected since start-up (it connects only while the page is ticked).");
    }
    String line = String(st.connected ? "&#10004; Connected" : "&#10006; ") + (st.connected ? "" : esc(st.reason.c_str()));
    line += st.lastValueAt ? ". Newest value " + ago(st.lastValueAt) : String(". No value received yet");
    if (!st.connected) {
        line += ". Last try " + ago(st.at) + ", took " + String(st.attemptMs / 1000.0, 1) + " s; retries every 30 s";
    }
    return "<div class='" + String(st.connected ? "hint" : "err") + "'>" + line + ".</div>";
}

static String plainInput(const char *key, const std::string &value, const char *placeholder) {
    return "<input type='text' id='" + String(key) + "' name='" + key + "' value='" + esc(value) + "' placeholder='" + placeholder + "'>";
}
```

- [ ] **Step 2: Form section**

In `renderForm()`, after the Tempest `#endif` that closes the Tempest fieldset and before `"<fieldset><legend>Dim hours</legend>"`, add:

```cpp
    h += "<fieldset><legend>Custom pages</legend>";
    h += hint("Pages of your own. A page appears once its address or broker is set; saving restarts the orb. Format and Home Assistant recipe: "
              "<a href='https://github.com/scottdube/hot-info-orbs/blob/main/docs/CUSTOM-PAGES.md'>CUSTOM-PAGES.md</a>.");

    h += "<h4>Web Data</h4>";
    h += textField("wdname", "Page name (16 characters)", v.wdname, d.wdname,
                   ". Its tick box above uses this name, so renaming it shows it again if it was unticked.", errors);
    h += "<label for='wdurl'>JSON address</label><input type='text' id='wdurl' name='wdurl' inputmode='url' value='" +
         esc(sv::maskUrl(v.wdurl)) + "'>";
    h += hint("Blank = no page. http:// or https://. Anything after ? is hidden here because it often holds a key: leave it as shown to keep it. "
              "Default: " + (d.wdurl.empty() ? String("none") : esc(sv::maskUrl(d.wdurl)))) +
         errorLine(errors, "wdurl");
    if (!v.wdurl.empty()) {
        h += webDataStatusLine();
    }

    h += "<h4>MQTT</h4>";
    h += textField("mqname", "Page name (16 characters)", v.mqname, d.mqname,
                   ". Its tick box above uses this name, so renaming it shows it again if it was unticked.", errors);
    h += textField("mqhost", "Broker host name or IP address", v.mqhost, d.mqhost.empty() ? std::string("none") : d.mqhost,
                   ". Blank = no page.", errors);
    h += "<label for='mqport'>Port</label><input type='number' id='mqport' name='mqport' min='1' max='65535' value='" + String(v.mqport) + "'>";
    h += hint("Default: " + String(d.mqport)) + errorLine(errors, "mqport");
    h += textField("mquser", "Username", v.mquser, d.mquser.empty() ? std::string("none") : d.mquser, ". Blank = no login.", errors);
    h += "<label for='mqpass'>Password</label><input type='password' id='mqpass' name='mqpass' value='' autocomplete='new-password'>";
    h += hint(v.mqpass.empty() ? "No password stored." : "&#10004; A password is stored. It is never shown; leave this blank to keep it.") +
         errorLine(errors, "mqpass");
    h += "<label class='chk'><input type='checkbox' name='mqpassclr' value='1'> Clear the stored password</label>";
    h += "<label>One value per orb: topic, label, unit</label>"
         "<div style='display:grid;grid-template-columns:5fr 3fr 2fr;gap:.3em'>";
    for (int i = 0; i < 5; i++) {
        char kt[8], kl[8], ku[8], ph[24];
        snprintf(kt, sizeof(kt), "mqt%d", i + 1);
        snprintf(kl, sizeof(kl), "mql%d", i + 1);
        snprintf(ku, sizeof(ku), "mqu%d", i + 1);
        snprintf(ph, sizeof(ph), "Orb %d topic", i + 1);
        h += plainInput(kt, v.mqt[i], ph) + plainInput(kl, v.mql[i], "Label") + plainInput(ku, v.mqu[i], "Unit");
    }
    h += "</div>";
    h += hint("Orbs left to right. A blank topic leaves that orb blank. No # or + wildcards. Label up to 16 characters, unit up to 8.");
    for (int i = 0; i < 5; i++) {
        char k[8];
        snprintf(k, sizeof(k), "mqt%d", i + 1);
        h += errorLine(errors, k);
        snprintf(k, sizeof(k), "mql%d", i + 1);
        h += errorLine(errors, k);
        snprintf(k, sizeof(k), "mqu%d", i + 1);
        h += errorLine(errors, k);
    }
    h += textField("mqsetup", "Layout topic (advanced)", v.mqsetup, d.mqsetup.empty() ? std::string("none") : d.mqsetup,
                   ". A JSON layout published here replaces the rows above until the next restart (mqttwidget/README.md).", errors);
    if (!v.mqhost.empty()) {
        h += mqttStatusLine();
    }
    h += "</fieldset>";
```

Also raise `h.reserve(9000);` to `h.reserve(14000);` (the section adds about 4 KB of HTML).

The errorLine for the per-orb rows needs a label to say which orb; `errorLine` prints only the message, so the parsers' messages must be prefixed in Step 3.

- [ ] **Step 3: POST handling**

In `handlePost()`, before the `// Equal hours would mean ...` comment, add:

```cpp
    // Custom pages
    if (has("wdname") && !sv::normalizeText(argStr(s, "wdname"), 16, v.wdname, err)) {
        errors["wdname"] = err;
    }
    if (has("wdurl")) {
        const std::string stored = v.wdurl; // the masked field resolves against this
        if (!sv::parseWebUrl(argStr(s, "wdurl"), stored, v.wdurl, err)) {
            errors["wdurl"] = err;
        }
    }
    if (has("mqname") && !sv::normalizeText(argStr(s, "mqname"), 16, v.mqname, err)) {
        errors["mqname"] = err;
    }
    if (has("mqhost") && !sv::parseHost(argStr(s, "mqhost"), v.mqhost, err)) {
        errors["mqhost"] = err;
    }
    if (has("mqport") && !sv::parsePort(argStr(s, "mqport"), v.mqport, err)) {
        errors["mqport"] = err;
    }
    if (has("mquser") && !sv::parseOptionalText(argStr(s, "mquser"), 64, v.mquser, err)) {
        errors["mquser"] = err;
    }
    if (has("mqpass") || has("mqpassclr")) {
        const std::string stored = v.mqpass;
        if (!sv::parsePassword(argStr(s, "mqpass"), has("mqpassclr"), stored, v.mqpass, err)) {
            errors["mqpass"] = err;
        }
    }
    for (int i = 0; i < 5; i++) {
        char k[8];
        String orb = "Orb " + String(i + 1) + " ";
        snprintf(k, sizeof(k), "mqt%d", i + 1);
        if (has(k) && !sv::parseTopic(argStr(s, k), v.mqt[i], err)) {
            errors[k] = (orb + "topic: ").c_str() + err;
        }
        snprintf(k, sizeof(k), "mql%d", i + 1);
        if (has(k) && !sv::parseOptionalText(argStr(s, k), 16, v.mql[i], err)) {
            errors[k] = (orb + "label: ").c_str() + err;
        }
        snprintf(k, sizeof(k), "mqu%d", i + 1);
        if (has(k) && !sv::parseOptionalText(argStr(s, k), 8, v.mqu[i], err)) {
            errors[k] = (orb + "unit: ").c_str() + err;
        }
    }
    if (has("mqsetup") && !sv::parseTopic(argStr(s, "mqsetup"), v.mqsetup, err)) {
        errors["mqsetup"] = err;
    }
```

- [ ] **Step 4: Build**

Run: `pio run -e esp32-s3-devkitc-1`
Expected: `SUCCESS`. Then `grep -n "mqpass" firmware/src/core/settings/SettingsPage.cpp` and confirm by reading that no line writes `v.mqpass` (or any password value) into HTML: the only `value=` on the password input is `value=''`.

- [ ] **Step 5: Commit**

```bash
git add firmware/src/core/settings/SettingsPage.cpp
git commit -m "settings page: Custom pages section for Web Data and MQTT

Password is write-only (blank keeps it, a checkbox clears it) and the URL's
query string is masked. Rejected a 'show password' toggle: the page has no
login unless OTA_PASSWORD is set, so anyone on the network could read it."
```

---

### Task 8: Docs, templates and the sample page

**Files:**
- Create: `docs/CUSTOM-PAGES.md`
- Create: `docs/custom-pages/sample.json`
- Modify: `firmware/config/config.h.template` (WEB DATA and MQTT sections)
- Modify: `firmware/config/secrets.h.template` (MQTT and web-data comments)
- Modify: `firmware/src/widgets/mqttwidget/README.md` (top)
- Modify: `docs/features.html` (the "And more" card, line ~327)
- Modify: `docs/REQUIREMENTS.md`

- [ ] **Step 1: Sample page**

Create `docs/custom-pages/sample.json` (colors from `Utils::stringToColor`; data wraps at 10 characters, so keep each `data` ≤ 10):

```json
{
  "interval": 300000,
  "displays": [
    { "label": "HOT", "data": "Info Orbs", "color": "white", "background": "navy" },
    { "label": "This is", "data": "a custom", "color": "yellow" },
    { "label": "page from", "data": "a JSON", "color": "cyan" },
    { "label": "file you", "data": "host", "color": "green" },
    { "label": "Edit me", "data": "anywhere", "color": "orange" }
  ]
}
```

Check: `python3 -m json.tool docs/custom-pages/sample.json` prints it without error.

- [ ] **Step 2: CUSTOM-PAGES.md**

Create `docs/CUSTOM-PAGES.md` with these sections, in this order:

1. **What it is** — two pages a member configures on `/settings` → Custom pages: Web Data (draws a JSON file from any http/https address) and MQTT (one live value per orb from an MQTT broker, e.g. Home Assistant). A page appears once its address/broker is set; saving restarts the orb. Untick it under Rotation → Show like any page; renaming a page re-ticks it.
2. **Web Data format** — top level `interval` (ms; at least 10000, 60000 if left out) and `displays` (up to five objects, left orb first). Per display: `label`, `data` (text, wraps at 10 characters, or an `elements` array as upstream), `color`, `labelColor`, `background`, `fullDraw`. Colors are names: list the names `Utils::stringToColor` accepts (read them from `firmware/src/core/utils/Utils.cpp` and copy the list). Limits: 16 KB reply, 5 s timeout, no certificate check on https. Paste `docs/custom-pages/sample.json` and give its URL: `https://scottdube.github.io/hot-info-orbs/custom-pages/sample.json` (served from `main` once this branch merges; before that use `https://raw.githubusercontent.com/scottdube/hot-info-orbs/custom-pages/docs/custom-pages/sample.json`). When a fetch fails the orbs keep the last good data; before any good data the center orb shows the page name and the reason.
3. **MQTT** — fields (host, port, username, write-only password, five topic/label/unit rows, advanced layout topic). Values are the topic's payload as text. Never blocks: one attempt, retried every 30 s; the status line says why it is not connected.
4. **Home Assistant recipe** — (a) MQTT Statestream:
   ```yaml
   mqtt_statestream:
     base_topic: homeassistant/statestream
     include:
       entities:
         - sensor.outdoor_temperature
   ```
   topic for an orb: `homeassistant/statestream/sensor/outdoor_temperature/state`. (b) An automation:
   ```yaml
   - alias: "Orb: garage door"
     triggers:
       - trigger: state
         entity_id: cover.garage_door
     actions:
       - action: mqtt.publish
         data:
           topic: orbs/garage
           payload: "{{ states('cover.garage_door') }}"
           retain: true
   ```
   Note: with `retain: true` the broker hands the last value to the orb as soon as it subscribes. Whether Statestream retains is filled in from on-orb test 4 (Task 9 step 6) — do not state it before then.
5. **Tested** — a table with the six on-orb tests from the spec, filled in by Task 9 (what was done, what was seen, date). No addresses or site names.

- [ ] **Step 3: Templates**

`config.h.template` — replace the WEB DATA and MQTT CONFIGURATION blocks with:

```cpp
// WEB DATA and MQTT pages
// Set these up on the settings page (http://<orb>/settings -> Custom pages);
// no rebuild needed. See docs/CUSTOM-PAGES.md. The defines below are only
// defaults for an orb that has never saved settings.
// WEB_DATA_WIDGET_URL and WEB_DATA_STOCK_WIDGET_URL live in secrets.h - a data
// URL routinely carries an API key in its query string.
//#define MQTT_WIDGET_HOST "192.168.3.40" // MQTT broker host
//#define MQTT_WIDGET_PORT 1883 // MQTT broker port
//#define MQTT_SETUP_TOPIC "info-orbs/setup/orbs" // Advanced: layout topic
// MQTT_WIDGET_USER and MQTT_WIDGET_PASS live in secrets.h
```

`secrets.h.template` — above `//#define MQTT_WIDGET_USER ""` and above `//#define WEB_DATA_WIDGET_URL ""`, add the line `// Default only: the settings page (Custom pages) can set and change this.`

`firmware/src/widgets/mqttwidget/README.md` — add at the top:

```markdown
> **HOT Info Orbs:** set the broker and one topic per orb on the settings page
> (Custom pages); see `docs/CUSTOM-PAGES.md`. The setup-topic JSON below still
> works as the "Layout topic (advanced)" field and replaces the rows until restart.
```

- [ ] **Step 4: Feature sheet and requirements**

`docs/features.html` line ~327: change the "And more" card text to: `A Parqet portfolio from upstream, switched on in <span class="num">config.h</span>.` and add a card before it:

```html
    <div class="w"><h3>Custom pages</h3><p>Your own page in the rotation, set up on the settings page: a JSON file from any web address, or one live value per orb from MQTT (Home Assistant). No rebuild.</p></div>
```

`docs/REQUIREMENTS.md`: read its structure first (`grep -n "^#" docs/REQUIREMENTS.md`), then add a requirement in the same style: custom pages set on `/settings` (Web Data URL; MQTT broker and per-orb topic/label/unit); off until configured; neither page may stop the buttons or the settings page responding; secrets (URL query, password) never sent back to the browser.

- [ ] **Step 5: Check and commit**

Run: `grep -rnE "192\.168\.|10\.[0-9]+\.|\.local\b" docs/CUSTOM-PAGES.md docs/custom-pages firmware/config/*.template` and read every hit: the only addresses allowed are upstream's `192.168.3.40` example in the template and the deliberately unreachable `10.255.255.1`. Also read the new docs once for any site, street or station name.

```bash
git add docs/CUSTOM-PAGES.md docs/custom-pages/sample.json firmware/config/config.h.template firmware/config/secrets.h.template firmware/src/widgets/mqttwidget/README.md docs/features.html docs/REQUIREMENTS.md
git commit -m "docs: custom pages guide, sample page, templates say defines are defaults

Sample served from this repo's GitHub Pages so a member can see a page before
writing one. Rejected a third-party JSON host: it can vanish, and the club
cannot fix it."
```

---

### Task 9: On-orb tests and flash measurement

Needs the orb and a laptop that reaches it on the network. If the laptop cannot reach it, stop here and say so — do not guess results.

**Files:**
- Modify: `docs/CUSTOM-PAGES.md` (the **Tested** table, Statestream sentence)

- [ ] **Step 1: Build and record flash**

Run: `pio run -e esp32-s3-devkitc-1 2>&1 | grep -E "Flash:|SUCCESS|FAILED"`
Record the `Flash:` percentage (test 6). Must be under 92%.

- [ ] **Step 2: Flash over HTTP**

Push the branch first (`git push`), then upload `.pio/build/esp32-s3-devkitc-1/firmware.bin` through `http://<orb>/update` (browser form, or `curl -F "firmware=@.pio/build/esp32-s3-devkitc-1/firmware.bin" http://<orb>/update`; the form field is `firmware`, from `OtaUpdater.cpp`). Not espota. Wait ~20 s; `/settings` must load and show the new commit.

- [ ] **Step 3: Test 1 — sample over HTTPS**

On `/settings` → Custom pages, set the Web Data address to the raw.githubusercontent.com sample URL from `docs/CUSTOM-PAGES.md`, Save. When the page comes up: all five orbs show the sample's labels and text in its colors. `/settings` shows ✔ Last fetch. Record what was seen.

- [ ] **Step 4: Tests 2 and 3 — bad and huge URLs**

Test 2: set the address to an unreachable one (`http://10.255.255.1/x.json`), Save. Expected: center orb shows the page name and the reason; `/settings` shows ✖ with the reason; buttons still change pages; `/settings` still loads. Test 3: set the address to a large page (`https://en.wikipedia.org/wiki/ESP32`). Expected: ✖ `too big` (or `bad JSON` if the page is under 16 KB — record which), no restart (uptime on `/settings` keeps counting). Record both. Then put the sample URL back.

- [ ] **Step 5: Test 5 — wrong broker**

Set the MQTT broker to a host that does not answer (`10.255.255.1`), one topic, Save. Expected: MQTT page shows "–"; `/settings` shows ✖ `can't reach broker` and the attempt time; buttons and `/settings` respond at all times, with at most a short stall every 30 s. Record the attempt time shown.

- [ ] **Step 6: Test 4 — live Home Assistant values**

Needs an MQTT user for the orb on the home broker's ACL. That is the owner's step: ask Scott to add it, and do not put the broker address or user name in the repo. With it set: one Statestream topic and one automation-published topic. Expected: values appear on their orbs; changing the entity in Home Assistant changes the orb within seconds while the page is on screen, and the new value is there when the page comes back round. Restart the orb and note whether each value appears before its next change (retained or not); write that into the Home Assistant recipe in `docs/CUSTOM-PAGES.md`. Also check the "–" glyph drew as a dash, not a box; if a box, change `"\xE2\x80\x93"` in `MQTTWidget::drawOrb` to `"-"` and re-run this step.

- [ ] **Step 7: Record and commit**

Fill the **Tested** table in `docs/CUSTOM-PAGES.md` (test, date 2026-10-xx, what was seen, ✔/✖). No addresses or site names.

```bash
git add docs/CUSTOM-PAGES.md
git commit -m "docs: custom pages on-orb test results

Recorded what was seen, not what was expected; flash N% of the OTA slot."
git push
```

---

## After the plan

- Open a PR `custom-pages` → `main`. After it merges, merge `main` into `xiao-esp32s3-plus` (memory: that branch becomes main soon).
- Video: the outline's open question on custom pages can then be closed and a line added to segment 4's tinkerer beat. Not part of this plan.
