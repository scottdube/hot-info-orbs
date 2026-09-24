# Tempest (WeatherFlow) API — measured, 2026-09-24

Measured from the laptop against an account that owns two stations (SLN and LRD). The
token came from Home Assistant's `weatherflow_cloud` integration. It is never
committed; the firmware reads it from `secrets.h`.

Station IDs, names and coordinates are **not recorded here**: this repo is
public, and a station name plus coordinates is a home address. They go in each
orb's own settings.

`GET https://swd.weatherflow.com/swd/rest/stations?token=…` lists them.

## The forecast call

`GET https://swd.weatherflow.com/swd/rest/better_forecast?station_id=<id>&units_temp=f&token=…`
(`units_temp=c` for metric)

- **~96 KB per reply** (95–97 KB for both stations). About 100 KB of that is
  `forecast.hourly`: 231 entries nobody on the orbs needs. `http.getString()`,
  which the Visual Crossing code uses, would hold all of it in RAM. The
  SuperMini has no PSRAM, so the reply has to be **streamed through an
  ArduinoJson filter**.
- **Field order in the raw stream:** `current_conditions` at byte 1,
  `forecast.daily` at byte 906, `forecast.hourly` at byte 3,584, and `station`
  and `status` in the last ~450 bytes. Everything the page needs is in the
  first ~3.6 KB.
- `current_conditions`: `conditions` (text, e.g. "Partly Cloudy"), `icon`,
  `air_temperature`, `feels_like`, `relative_humidity`, `wind_avg`, and more.
- `forecast.daily`: 10 days. **`daily[0]` is today** (`day_num` = today's
  date): `conditions`, `icon`, `air_temp_high`, `air_temp_low`. Visual
  Crossing's `days[0]` is also today, and the page shows `days[1..3]`.

## Icons seen

Both stations, one day, current + daily + hourly: `clear-day`, `clear-night`,
`cloudy`, `partly-cloudy-day`, `partly-cloudy-night`, `possibly-rainy-day`,
`possibly-rainy-night`, `rainy`, `possibly-thunderstorm-day`,
`possibly-thunderstorm-night`, `thunderstorm`.

The orb's icon set (`WeatherWidget::drawWeatherIcon`) knows
`partly-cloudy-day/night`, `clear-day/night`, `snow`, `rain`,
`fog`/`wind`/`cloudy`. Tempest's names need a translation table. The names
Tempest documents but that were not seen on this day (`foggy`, `windy`,
`sleet`, `snow`, `possibly-snow-*`, `possibly-sleet-*`) are unmeasured.
