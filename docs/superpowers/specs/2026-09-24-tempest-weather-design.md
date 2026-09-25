# Tempest weather — design

**Date:** 2026-09-24 · **Branch:** `tempest` · **Status:** approved in chat, spec awaiting review

## What and why

Scott has a WeatherFlow Tempest station at each site (SLN and LRD). He wants
the orbs to show weather from those stations, **one weather page per station**
in the rotation. Today the orbs show a single Visual Crossing page for one
town.

Decisions made in chat, with the options rejected:

| Question | Chosen | Rejected |
|---|---|---|
| How do two sites appear? | Two full weather pages in the rotation: clock → SLN → LRD → stocks | One combined page (loses the 3-day forecast); one page alternating sites (only one site per pass) |
| What does a Tempest page show? | The same five orbs as today, fed from Tempest | Swapping orb 1 for a wind/rain/humidity readout. That is a later round; a new screen design now would cost flash and an eye check |
| How is it built? | A: a data-source split inside `WeatherWidget`, one widget instance per station | B: a separate Tempest widget (duplicates the drawing code: flash, and every fix made twice). C: read the stations through Home Assistant (ties the orbs to Scott's HA; the club couldn't use it) |
| Where does it live? | Branch `tempest`, built so Tempest is **compiled out** unless `TEMPEST_TOKEN` is defined | Mixing it into main now. Whether `tempest` ever merges is Scott's later call; compiled-out means a merge would cost Yang's build nothing |

Measured API facts are in `docs/TEMPEST-API.md`. Station IDs are not in this
repo, because it is public and a station amounts to a home address.

## Global constraints

- Flash stays **below 92%** of a 1,966,080-byte slot (87.0% before this work).
  Measure after every task that adds code.
- The SuperMini has **no PSRAM**. Never hold a whole Tempest reply (~96 KB) in
  RAM.
- The Tempest token lives only in `firmware/config/secrets.h` (git-ignored),
  never on the settings page and never in the repo. The page must never show
  it.
- With `TEMPEST_TOKEN` undefined, the build and the orb behave **exactly** as
  on main.
- Every settings-page state keeps a glyph as well as a color (✖/✔/⚠).

## Design

### 1. Data source split

`WeatherWidget` keeps all its drawing code and the five-orb layout. Fetching
and parsing move behind an interface:

```cpp
class WeatherSource {
  public:
    virtual ~WeatherSource() = default;
    virtual bool fetch(WeatherDataModel &model) = 0; // true = model filled
};
```

- `VisualCrossingSource`: today's `getWeatherData()` body moved as is, with
  its URL built from Settings as now.
- `TempestSource(stationId, label)`, only when `TEMPEST_TOKEN` is defined.
- `WeatherWidget(ScreenManager&, WeatherSource*)`. The existing
  `update()`/retry/10-minute logic calls `m_source->fetch(model)`.

### 2. Tempest fetch

`GET https://swd.weatherflow.com/swd/rest/better_forecast?station_id=<id>&units_temp=<f|c>&token=<TEMPEST_TOKEN>`
(`c` when Settings `wxmetric`).

- `http.useHTTP10(true)`, so the reply is not chunked, then
  `deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter))`.
  The filter keeps only:
  - `current_conditions.conditions`, `.icon` and `.air_temperature`
  - `forecast.daily[*].icon`, `.air_temp_high` and `.air_temp_low`
  - the whole of `hourly` is discarded as it streams
- Model mapping:
  - city name = the station's **label** (e.g. "SLN")
  - text = `current_conditions.conditions`
  - icon = the translated `current_conditions.icon`
  - temperature = `air_temperature`
  - today high/low = `daily[0]`
  - the 3 forecast days = `daily[1..3]`
- Failure (HTTP error, parse error, fewer than 4 days): return false and keep
  the previous data. The existing retry and 10-minute cycle apply unchanged.
- The label shows on orb 1 before the first successful fetch.

### 3. Icon translation

Pure function `tempestIcon(const std::string&) -> std::string`, std-only in a
header so the native tests cover it:

| Tempest | Orb icon |
|---|---|
| `clear-day`, `clear-night`, `cloudy`, `partly-cloudy-day`, `partly-cloudy-night` | unchanged |
| `rainy`, `possibly-rainy-day/night`, `thunderstorm`, `possibly-thunderstorm-day/night` | `rain` (the orb has no storm icon) |
| `snow`, `sleet`, `possibly-snow-day/night`, `possibly-sleet-day/night` | `snow` |
| `foggy` | `fog` |
| `windy` | `wind` |
| anything else | `cloudy` |

### 4. Settings

New `SettingsValues` fields: `tstn1`, `tlbl1`, `tstn2`, `tlbl2`.
- Station ID: 0 = unused. The page accepts 1 to 9 digits. `sv::parseInt` stops
  at 6 characters, so a new `sv::parseStationId` handles this.
- Label: `normalizeText`, 1–8 characters.
- Defaults come from new optional `config.h` macros `TEMPEST_STATION_1`,
  `TEMPEST_LABEL_1`, `TEMPEST_STATION_2` and `TEMPEST_LABEL_2`, and fall back
  to 0 and "".
- NVS keys are added without a schema bump; an absent key falls back to its
  default, as it does for every existing key.
- **Settings page:** a "Tempest stations" section appears only when
  `TEMPEST_TOKEN` is compiled in. Two rows, each a station ID and a label, with
  the config.h default shown as a hint. The token itself is never shown or
  mentioned beyond "token set in secrets.h".
- Each station row also shows its **last fetch result**, e.g. "✔ 14:32, 4.1 s"
  or "✖ 14:32 HTTP 401". `TempestSource` keeps the time, duration and
  code of its last attempt. There is no serial on the SuperMini, so this is the
  only way to see a fetch fail without looking at the orbs.

### 5. Start-up wiring (`main.cpp`)

When `TEMPEST_TOKEN` is defined and at least one station ID is non-zero, add
one `WeatherWidget(new TempestSource(id, label))` per set station, in order 1
then 2, where the single Visual Crossing widget is added today. Otherwise add
the Visual Crossing widget as now.

## Testing

- **Native (`pio test -e native`):**
  - `tempestIcon` against every row of the table.
  - `parseStationId` edge cases.
  - The filter and parse step against a **made-up** sample reply in
    `test/fixtures/tempest_sample.json`, shaped like the real one (including a
    long `hourly` array) but with no real station, name or coordinates.
  - The parse test needs ArduinoJson on the native env (it is header-only).
- **On the orb, from the laptop:**
  - Both stations show "✔" as their last fetch result on the settings page.
  - Record the fetch duration the page reports: the 96 KB download blocks the loop.
  - Free heap before and after a fetch.
  - Flash percentage.
- **By eye (Scott):** SLN and LRD pages in the rotation, with correct labels,
  temperatures, icons and forecast.
- Build **without** `TEMPEST_TOKEN`: it must match main's flash size to within
  a few hundred bytes. That proves the feature compiles out.

## Out of scope

- A wind/rain/humidity/pressure/lightning readout: the next round, and it needs
  its own screen design.
- More than two stations.
- The local UDP broadcast from the hub, which only reaches the station on the
  same LAN.
- Putting the token on the page. It would make a password mandatory (see R1).
