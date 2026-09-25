# Web settings page — design

**Status:** approved by Scott 2026-09-23. Implements R1 in `docs/REQUIREMENTS.md`
(partially — see Scope). Builds on the OTA web server merged the same day.

## Why our own page rather than upstream's

`upstream/dev` has a finished settings page (`ConfigManager`, 1.71 MB built for
our S3, fits). It was rejected for three reasons. It is unreleased contributor
work that never reached upstream `main`. Adopting it means moving this fork across 571
commits. And it offers a brightness control that cannot work on these
displays. This page covers only the settings in the table below and lives on
the server that OTA already runs.

## Scope

### v1 — on the page

| Setting | Key | Type / range | `config.h` default today | Read at |
|---|---|---|---|---|
| Rotation time, s per page | `cycle` | int, 0 (off) or 5–3600 | `WIDGET_CYCLE_DELAY` | `main.cpp:28` |
| Stock tickers | `tickers` | 1–5 symbols, comma-separated | `STOCK_TICKER_LIST` | `StockWidget.cpp:11` |
| Weather location | `wxloc` | text, 1–64 chars | `WEATHER_LOCATION` | `WeatherWidget.h:60` |
| Weather units | `wxmetric` | bool | `WEATHER_UNITS_METRIC` defined? | `WeatherWidget.h:61`, `GlobalTime.cpp:117` |
| Weather screen | `wxdark` | bool (dark / light) | `WEATHER_SCREEN_MODE` | `WeatherWidget.cpp:41` |
| Timezone | `tz` | text, IANA name, 1–48 chars | `TIMEZONE_API_LOCATION` | `GlobalTime.cpp:135` |
| 24-hour clock | `h24` | bool | `FORMAT_24_HOUR` | `GlobalTime.h:82`, `ClockWidget.cpp:50,93` |
| AM/PM indicator | `ampm` | bool | `SHOW_AM_PM_INDICATOR` | `ClockWidget.cpp:50` |
| Starting clock face | `face` | Normal / Nixie (Custom only if built with `USE_CLOCK_CUSTOM`) | `DEFAULT_CLOCK` | `ClockWidget.h:99` |
| Clock color | `clkcol` | RGB565 via color picker | `CLOCK_COLOR` | `ClockWidget.cpp:23,27,31,211` |
| Clock shadow color | `shdcol` | RGB565 via color picker | `CLOCK_SHADOW_COLOR` | `ClockWidget.cpp:43,168,185` |
| Night dim | `dim` | bool | `DIM_START_HOUR` defined? | `WidgetSet.cpp:96` |
| Dim start / end hour | `dimstart`, `dimend` | int 0–23, may wrap midnight | `DIM_START_HOUR`, `DIM_END_HOUR` | `WidgetSet.cpp:99–104` |
| Orbs upside down | `invert` | bool | `INVERTED_ORBS` | `ScreenManager.cpp:13,87` |
| Boot picture | file `/boot.jpg` | 240×240 baseline JPEG, ≤ 64 KB | embedded `images/logo.jpg` | `main.cpp:107` |

Night dim exposes **hours only**. The dim *level* stays at `DIM_BRIGHTNESS`
from `config.h`, or 128 (the template's commented value) when that is undefined. The page has no brightness control. Upstream's dimming
darkens colors. The backlight is hard-wired on (REQUIREMENTS R1).

### Later — needs restructuring first

- **API keys on the page.** It removes the build-time empty-key guard, so it
  first needs per-key status: set, unset, last call failed.
- **Second-hand ticks, language.** `#if` blocks select code at compile time
  (`ClockWidget.cpp:45`, `GlobalTime.h:15`).
- **Enabling and disabling widgets.** `main.cpp` includes and adds widgets under `#ifdef`.
- **Clock font, custom clock face upload, MQTT/Parqet/WebData widgets.** These
  are compile-time selections. A custom face is 12 images, about 200 KB at
  Nixie-digit size, which does not fit the 128 KB filesystem.

### Never on the page

The OTA password, and any stored secret displayed back. WiFi credentials stay with
WiFiManager's setup portal.

## Architecture

Three units, each usable without reading the others' internals.

**`Settings`** (`firmware/src/core/settings/Settings.{h,cpp}`) is the only code that
touches NVS. It uses the `Preferences` namespace `orbs`. It has typed
getters that fall back to the `config.h` value when a key is absent, so an orb
that never opened the page behaves exactly as today. It also has `save(values)` and
`factoryReset()`, which clears the namespace. A `schema` key (= 1) is written with every save,
so a later layout change can detect old data. Loaded once in `setup()` before
any widget is constructed.

**`SettingsValidation`** (`firmware/src/core/settings/SettingsValidation.h`) is
pure functions, with no Arduino types beyond `String`. For each field it either
normalizes the raw form value (trims, upper-cases tickers, collapses spaces) or
returns a message saying what is wrong. Kept separate so it can be unit-tested
on the host.

**`SettingsPage`** (`firmware/src/core/settings/SettingsPage.{h,cpp}`) holds HTTP
handlers registered on the `WebServer` that `OtaUpdater` owns. `OtaUpdater`
gains `WebServer &server()` and `bool authorized()` so that both pages share one
server and one password check rather than each binding port 80.

- `GET /settings` returns the form, filled with current values. Every field
  shows its `config.h` default as a hint.
- `POST /settings` runs the validation. On any error it re-renders the form
  with messages beside the fields and saves nothing. On success it saves,
  shows "Saved, restarting", and calls `ESP.restart()` after one second.
- `POST /settings/reset` clears the stored settings and restarts.
- `GET /boot.jpg` returns the stored picture, or the embedded one, for the preview.
- `POST /settings/boot` uploads a picture. `POST /settings/boot/reset`
  deletes `/boot.jpg`.

The page links to `/update` and back, and shows the running commit and uptime.

### Why restart on save

Every v1 setting is read at construction or in `setup()`, as a `const` member
or at a single initialization point. Applying settings live would mean a setter on every widget
and invalidating what is on screen. A restart costs about 14 s, measured
during OTA, and settings change rarely. **Rejected: live apply.**

### Consumers

Each read site in the table swaps its macro for a `Settings` getter. Settings
that today are tested with `#ifdef` (`WEATHER_UNITS_METRIC`,
`DIM_START_HOUR`) become runtime `if`s on a bool. The macro stays in
`config.h` only as the default. `GlobalTime`'s strftime locale strings,
chosen by `#if LOCALE`, are untouched.

**Weather location is URL-encoded** where the request is built
(`WeatherWidget.h:69`). Today the configured town goes into the URL raw with its spaces and
comma. Upstream issue #337 reports that locations containing a space fail. Whether
weather data currently displays for a two-word town has **not been verified** —
check it on the orb before and after this change.

## Boot picture

Upload path: in the browser, the page draws the chosen file onto a 240×240
canvas, center-cropped to a square, and previews it inside a circle. The circle
is what the orb shows. The browser then sends `canvas.toBlob("image/jpeg", q)`. The browser always
produces a **baseline** JPEG, which TJpg_Decoder requires, since it cannot decode
progressive ones. Quality starts at 0.85 and steps down until the file is at
most 64 KB.

The device does not trust the browser. On receipt it checks the SOI marker
`FFD8`, a baseline SOF0 `FFC0` (rejecting progressive `FFC2`), and 240×240 in
the SOF header. It then writes `/boot.tmp` and renames it to `/boot.jpg`, so an interrupted
upload never leaves a half-written picture in place.

Filesystem: `LittleFS.begin(true)` in `setup()`, which formats on first mount.
Nothing calls `begin()` today, so the 128 KB partition has never been
formatted. First boot after this firmware formats it, which takes a few
seconds once.

Draw path (`main.cpp:107`): if `/boot.jpg` exists, draw it from the file. If
the file is missing, or decoding returns an error, draw the embedded logo
instead. A bad file must never leave screen 2 blank.

## Security

Same credentials as `/update`: HTTP basic auth, user `admin`, `OTA_PASSWORD`.
If `OTA_PASSWORD` is unset both pages are open, as `/update` already is. v1
holds no secrets (no keys, no WiFi), and the update page, which can install
arbitrary firmware, is the larger exposure. REQUIREMENTS' security floor ("at
minimum a password") binds when keys move onto the page. The
"Later" item above must make the password mandatory then.

## Error handling

| Failure | Behavior |
|---|---|
| Field fails validation | Form re-shown with the message beside that field; nothing saved |
| NVS write fails | Page says so; no restart; old values still in effect |
| Unknown timezone or weather location | Accepted, because it cannot be checked offline. The existing on-screen error from the API call is what the user sees. Noted beside those fields on the page |
| Boot picture fails the checks | Rejected with the reason (not JPEG / progressive / wrong size / too big) |
| Filesystem full or mount fails | Upload rejected; embedded logo continues |
| Stored `/boot.jpg` will not decode | Embedded logo drawn; the page shows "stored picture unreadable — reset it" |

## Testing

- **Host unit tests** for `SettingsValidation`: a `[env:native]` PlatformIO
  environment with Unity, run via `pio test -e native`. It covers the ticker list
  (1–5, symbols like `BTC/USD` and `SHOP&country=Canada` that the ticker
  comment in `config.h` allows), hour wrap, cycle bounds, empty and over-long text, and
  color hex to RGB565.
- **On the orb**, over OTA. First confirm that a board which never opened the page shows
  exactly what it does today. Then change each v1 setting and check the result on the
  orb, one at a time, reading back from the device and not from the form.
  Then factory reset, and upload a progressive JPEG directly with curl to confirm the device check
  rejects it. Finally upload a real photo through the page, and power-cycle to confirm it persists.
- Flash budget: app is 1,639,797 bytes (83.4% of a slot) before this work.
  Report the new figure. Stop and reassess above 92%.

## Not in this spec

The later-list items; a JSON API for Home Assistant; mDNS naming per orb.
