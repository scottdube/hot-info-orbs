# Web Settings Page Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** A password-protected `/settings` page on the orb that changes the v1 settings and the boot picture without reflashing.

**Architecture:** `Settings` loads NVS once at boot and falls back to `config.h`. Every read site takes its value from `Settings::get()` instead of a macro. `SettingsPage` registers handlers on the WebServer that `OtaUpdater` already runs. Saving restarts the orb. The boot picture lives in LittleFS as `/boot.jpg`, with the embedded logo as the fallback.

**Tech Stack:** Arduino-ESP32 2.0.17 (PlatformIO espressif32 7.0.1), `Preferences`, `LittleFS`, `WebServer`, TJpg_Decoder `drawFsJpg`, Unity on `platform = native` for host tests.

**Spec:** `docs/superpowers/specs/2026-09-23-web-settings-design.md`

## Global Constraints

- No brightness control on the page; dim exposes hours + on/off only; level = `DIM_BRIGHTNESS` or 128.
- An orb that never saved settings must behave exactly as today (config.h defaults).
- No stored secret is ever rendered back. Same basic auth as `/update` (`admin` / `OTA_PASSWORD`).
- Boot picture: 240×240 baseline JPEG, ≤ 64 KB; write `/boot.tmp` then rename; embedded logo on any failure.
- Save → restart. No live apply.
- Flash budget: stop and reassess above 92% of the 1,966,080-byte slot (now 83.4%).
- Deviation from spec, deliberate: `SettingsValidation` uses `std::string`, not Arduino `String`, so it compiles on the host without Arduino headers.

## File map

| File | Responsibility |
|---|---|
| `firmware/src/core/settings/SettingsValidation.h` (create) | Pure parse/normalise/validate + `urlEncode` + `rgb565` conversions + JPEG header check. std only. |
| `test/test_validation/test_main.cpp` (create) | Unity host tests for the above. |
| `firmware/src/core/settings/Settings.{h,cpp}` (create) | `SettingsValues` struct, `load()`, `get()`, `defaults()`, `save()`, `factoryReset()`. Only NVS toucher. |
| `firmware/src/core/settings/SettingsPage.{h,cpp}` (create) | HTML form, POST handling, boot-picture upload/preview/reset. |
| `firmware/src/core/ota/OtaUpdater.{h,cpp}` (modify) | Expose `server()` and `authorised()`. |
| `main.cpp`, `ScreenManager.cpp`, `WidgetSet.cpp`, `GlobalTime.{h,cpp}`, `ClockWidget.{h,cpp}`, `WeatherWidget.{h,cpp}`, `StockWidget.cpp` (modify) | Macro → `Settings::get()` at each read site in the spec table. |
| `platformio.ini` (modify) | `[env:native]` for tests; `-I firmware/src/core/settings`. |

---

### Task 1: Validation module, host-tested

**Files:** Create `firmware/src/core/settings/SettingsValidation.h`, `test/test_validation/test_main.cpp`, `test/fixtures/progressive.jpg` (generated); modify `platformio.ini`.

**Interfaces — Produces** (namespace `sv`, all `inline`):
- `bool parseCycle(const std::string &in, int &out, std::string &err)` — "0" or 5–3600.
- `bool parseHour(const std::string &in, int &out, std::string &err)` — 0–23.
- `bool normaliseTickers(const std::string &in, std::string &out, std::string &err)` — split on `,`, trim, drop empties, 1–5 items, chars `[A-Za-z0-9./&=:_^-]`, upper-case up to the first `&`, rejoin with `,`, total ≤ 128.
- `bool normaliseText(const std::string &in, size_t maxLen, std::string &out, std::string &err)` — trim, collapse runs of spaces, 1–maxLen, reject control chars.
- `std::string urlEncode(const std::string &in)` — RFC 3986 unreserved kept, rest `%XX` upper-case.
- `bool parseHexColour(const std::string &in, uint16_t &rgb565, std::string &err)` — `#rrggbb`.
- `std::string rgb565ToHex(uint16_t c)` — bit-replicated back to `#rrggbb`.
- `std::string htmlEscape(const std::string &in)` — `& < > " '`.
- `bool checkJpegHeader(const uint8_t *buf, size_t len, std::string &err)` — needs `FFD8`; walks markers; accepts SOF0 `FFC0` with height = width = 240; rejects `FFC2` ("progressive JPEG — re-save as baseline"), other SOFs, wrong size, or no SOF within `len`.

- [ ] **Step 1:** Add to `platformio.ini`:
  ```ini
  [env:native]
  platform = native
  test_framework = unity
  build_flags = -std=c++17 -I firmware/src/core/settings
  ```
  and add `-I firmware/src/core/settings` to the S3 env's `build_flags`.
- [ ] **Step 2:** Generate the fixture: `python3 -c "from PIL import Image; Image.open('images/logo.jpg').save('test/fixtures/progressive.jpg', progressive=True)"`
- [ ] **Step 3:** Write the tests (all cases below), with a header-only stub so the file compiles and the tests FAIL:
  - cycle: `"0"`→0 ok; `"15"`→15; `"4"` err; `"3601"` err; `"abc"` err; `" 30 "`→30.
  - hour: `"0"`,`"23"` ok; `"24"`,`"-1"`,`""` err.
  - tickers: `"spy, qqq ,AAPL"`→`"SPY,QQQ,AAPL"`; `"BTC/USD"` ok; `"shop&country=Canada"`→`"SHOP&country=Canada"`; 6 items err; `""` err; `",,"` err; `"SP Y"` err.
  - text: `"  Dover,   NH "`→`"Dover, NH"`; 65 chars with max 64 err; `"a\tb"` err.
  - urlEncode: `"Dover, NH"`→`"Dover%2C%20NH"`; `"America/New_York"`→`"America%2FNew_York"`; `"A-z_0.~"` unchanged.
  - colour: `"#fc8000"`→ `0xFC00`; `"fc8000"` err; `"#zzzzzz"` err; `rgb565ToHex(0xFFFF)`→`"#ffffff"`, `rgb565ToHex(0)`→`"#000000"`.
  - htmlEscape: `"<a href=\"x\">&"` → `"&lt;a href=&quot;x&quot;&gt;&amp;"`.
  - jpeg: `images/logo.jpg` (read with fopen) passes; `test/fixtures/progressive.jpg` fails with "progressive"; a 4-byte `{0xFF,0xD8,0xFF,0xD9}` fails; `{0x89,'P','N','G'}` fails "not a JPEG".
- [ ] **Step 4:** `~/.platformio/penv/bin/pio test -e native` → expect failures.
- [ ] **Step 5:** Implement `SettingsValidation.h`.
- [ ] **Step 6:** `pio test -e native` → all pass. `pio run -e esp32-s3-devkitc-1` still builds.
- [ ] **Step 7:** Commit.

### Task 2: Settings store + read sites switched

**Files:** Create `Settings.{h,cpp}`; modify the read-site files listed in the file map.

**Interfaces — Produces:**
```cpp
struct SettingsValues {
    int cycle; std::string tickers; std::string wxloc; bool wxmetric; bool wxdark;
    std::string tz; bool h24; bool ampm; int face; uint16_t clkcol; uint16_t shdcol;
    bool dim; int dimstart; int dimend; bool invert;
};
class Settings {
  public:
    static void load();                          // once, first thing in setup()
    static const SettingsValues &get();
    static SettingsValues defaults();            // from config.h macros
    static bool save(const SettingsValues &v);   // writes all keys + schema=1
    static bool factoryReset();                  // clears namespace "orbs"
    static bool hasStored();                     // schema key present
};
```
Defaults: `cycle`=`WIDGET_CYCLE_DELAY` or 0; `tickers`=`STOCK_TICKER_LIST` or ""; `wxmetric`=defined(`WEATHER_UNITS_METRIC`); `wxdark`=`WEATHER_SCREEN_MODE == Dark` (default Dark); `face`=`(int)DEFAULT_CLOCK`; `clkcol`=`CLOCK_COLOR`; `shdcol`=`CLOCK_SHADOW_COLOR`; `dim`=defined(`DIM_START_HOUR`)&&defined(`DIM_END_HOUR`); `dimstart`/`dimend` = those or 22/7; `invert`=`INVERTED_ORBS`.

Read-site changes (each one line or a few):
- `main.cpp`: `Settings::load()` first in `setup()`; `m_widgetCycleDelay = Settings::get().cycle * 1000UL` set in `setup()`; `widgetSet->add(new StockWidget)` only when `!Settings::get().tickers.empty()` (still inside `#ifdef STOCK_TICKER_LIST` for the include).
- `ScreenManager.cpp:13,87`: `Settings::get().invert`.
- `WidgetSet.cpp:96-111`: runtime `if (!s.dim) return;` using `dimstart/dimend`; level `DIM_BRIGHTNESS` or 128.
- `GlobalTime.h:82`: `m_format24hour{Settings::get().h24}`; `GlobalTime.cpp:117` `#ifdef WEATHER_UNITS_METRIC` → `if (Settings::get().wxmetric)`; `:135` zone → `urlEncode(tz)`.
- `ClockWidget`: `CLOCK_COLOR`/`CLOCK_SHADOW_COLOR` uses → `m_colour`/`m_shadow` members set in ctor from settings; `FORMAT_24_HOUR`→`Settings::get().h24`; `SHOW_AM_PM_INDICATOR`→`.ampm`; `m_type` from `.face`, clamped to NORMAL if the face's images are not compiled in.
- `WeatherWidget.h`: `weatherLocation`, `weatherUnits`, `httpRequestAddress` become non-const members built in the ctor, location through `urlEncode`; `WeatherWidget.cpp:41` `m_screenMode = s.wxdark ? Dark : Light`.
- `StockWidget.cpp:10-25`: parse from `Settings::get().tickers`; loop guard fixed to `m_stockCount < MAX_STOCKS` before writing (existing code writes index 5 before breaking).

- [ ] **Step 1:** Write `Settings.{h,cpp}`.
- [ ] **Step 2:** Switch each read site. Build.
- [ ] **Step 3:** OTA-flash; the orb must look exactly as before (clock face, colours, rotation 15 s, tickers, weather). Check the weather orb shows real data; record whether the URL-encoding changed anything.
- [ ] **Step 4:** Commit.

### Task 3: Settings page

**Files:** Create `SettingsPage.{h,cpp}`; modify `OtaUpdater.{h,cpp}`, `main.cpp`.

**Interfaces — Consumes:** `Settings`, `sv::*`. **Produces:** `class SettingsPage { public: explicit SettingsPage(OtaUpdater &ota); void begin(); };` (idempotent, call after `otaUpdater->begin()`). `OtaUpdater` gains `WebServer &server();` and `bool authorised();` (returns false after sending the 401 challenge).

- [ ] **Step 1:** OtaUpdater accessors; `/update` handlers use `authorised()`.
- [ ] **Step 2:** `GET /settings`: form with every v1 field, current value, config.h default as hint, colour pickers, hour selects, face select (Nixie/Custom only if compiled in), running commit + uptime, links to `/update`. All values through `htmlEscape`. Boot-picture section placeholder for Task 4.
- [ ] **Step 3:** `POST /settings`: validate every field; collect all errors; on any error re-render with messages, save nothing; else `Settings::save`, "Saved, restarting", restart after 1 s. `POST /settings/reset`: `factoryReset` + restart.
- [ ] **Step 4:** Build, OTA-flash, and use curl (by IP, basic auth) to test: GET renders; POST with `cycle=4` returns the error and nothing saved; POST with `cycle=20` → restart, and GET shows 20 afterwards; reset → back to 15.
- [ ] **Step 5:** Commit.

### Task 4: Boot picture

**Files:** modify `SettingsPage.cpp`, `main.cpp`.

- [ ] **Step 1:** `LittleFS.begin(true)` in `setup()` after `Settings::load()`; log result.
- [ ] **Step 2:** `main.cpp:107`: if `/boot.jpg` exists and `TJpgDec.drawFsJpg(0,0,"/boot.jpg",LittleFS) == JDR_OK`, done; else draw embedded logo.
- [ ] **Step 3:** `POST /settings/boot`: stream chunks into `/boot.tmp`, abort past 64 KB; on end read the first 4 KB, `sv::checkJpegHeader`; pass → remove old, rename; fail → delete tmp, 400 with reason. `POST /settings/boot/reset`: remove `/boot.jpg`. `GET /boot.jpg`: stored file, or embedded bytes.
- [ ] **Step 4:** Page JS: file input → canvas 240×240 centre-crop → circle preview → `toBlob('image/jpeg', q)` stepping q from 0.85 by 0.1 until ≤ 64 KB → `fetch` POST FormData → show server reply.
- [ ] **Step 5:** Build, OTA-flash. curl-upload the progressive fixture and expect it rejected; curl-upload `images/logo.jpg` and expect it accepted. Then restart and check the splash, reset, and restart again. Scott tests a real phone photo through the page.
- [ ] **Step 6:** Commit.

### Task 5: Verify on hardware + docs

- [ ] Each v1 setting changed once and seen on the orb (Scott's eyes where it is visual); power-cycle persistence; factory reset.
- [ ] Flash % recorded.
- [ ] README (settings page row + how to reach it), SETUP step 9, REQUIREMENTS R1 status, TRAPS for anything learned.
- [ ] Commit + push; memory updated.
