# Feature requirements

Requirements for this build beyond what upstream provides. Each says what it
must do and *why*, because the why is what survives when the how changes.

Context: this is built as a learning project by people at a range of skill
levels, several of whom will be new to embedded development. **Complexity and
cost are constraints, not preferences.** A feature that adds a build step, a
part, or a failure mode has to earn it.

---

## R1 — Web control panel

**Settings must be changeable without rebuilding or reflashing the firmware.**

Today every preference lives in `config.h` and is compiled in. Changing a stock
ticker, a weather location, or a timezone means opening VS Code, editing a C
header, rebuilding, and connecting a USB cable. For someone who assembled the
hardware but does not write code, that is a wall.

**Must:**
- Serve a configuration page from the device over the local network
- Cover the settings people actually change: timezone, weather location, units,
  ticker list, clock face, and the scheduled dim hours (see the note below —
  this is not brightness control)
- Persist to NVS so settings survive a reboot and a firmware update
- Be reachable without special software — a browser on a phone is enough

**Should:**
- Show the device's current state: WiFi signal, uptime, last successful API call
- Allow API keys to be entered here rather than compiled in

**Interaction with the secrets work — read this before implementing.** Keys are
currently compile-time constants in a gitignored `secrets.h`, guarded so an
empty key fails the build. Moving keys to runtime NVS storage weakens that
guarantee: a device can then be flashed and running with no keys at all, failing
silently at the first API call. If keys move to the panel, the panel must show
their status plainly — set, unset, or last call failed — because the build-time
error that currently catches this goes away.

**There is no brightness control, and there cannot be on this hardware.** The
7-pin GC9A01 modules do not break out the backlight pin — BL is tied on at the
module, so the backlight runs at full output whenever the board is powered.
Upstream's "dimming" is a firmware effect: during the configured hours it
desaturates the colours so the display *looks* dimmer. It draws exactly the same
current.

So the panel can expose the dim *hours*, and must not offer a brightness slider.
A slider that does nothing is worse than no slider — someone will report the
orbs as too bright at night and be told to adjust a control that was never
connected to anything.

Real dimming needs different hardware: a display module that exposes BL, wired
to a PWM-capable GPIO. Worth weighing when the next batch of modules is chosen,
because it also cuts the dominant continuous load — the backlight, not the
radio, is what makes these boards warm.

**Screens off (added 2026-09-24).** The night hours can instead send the
GC9A01's display-off and sleep-in commands (0x28, 0x10) to all five panels. The
HOT meeting that day asked for it, and the controller datasheet lists the
commands. Scott's first look in daylight: "looks pretty good". **Not yet
measured:** how much the backlight still glows in a dark room, and the current
draw. The LED is wired to VCC on the module, not to the controller, so it
probably still draws full current. Until that's checked, call this "blanks the
panels", not "turns the backlight off".

**Security floor:** the panel holds WiFi credentials and API keys. It needs at
minimum a password, and it must never display stored secrets back in plaintext.
An unauthenticated page on the LAN that reveals WiFi credentials is not
acceptable, however friendly the network.

**Upstream already built this — found 2026-09-23.** `upstream/dev` (not
`main`, which this fork is based on; 571 commits apart) has
`firmware/src/core/configmanager/ConfigManager.*`: a settings page on
WiFiManager, stored in NVS via `Preferences`, with sections, an "advanced"
toggle and i18n. Per-widget settings register themselves; MainHelper.cpp
covers timezone, language, widget cycle delay, NTP server, orb rotation, and dim
hours. Measured by building `upstream/dev` against our S3 board and
partitions.csv: **1,712,081 bytes, 87.1% of one OTA slot**, no OTA code yet. So it
fits. Two conflicts with decisions here: it offers a `tftBrightness` setting,
which does nothing on these displays (see above), and its key handling has to
be reconciled with the `secrets.h` split.

**Status 2026-09-23: partly implemented — our own page, not upstream's.**
`http://<orb>/settings` covers rotation time, tickers, weather location,
units and screen mode, timezone, 12/24 h, AM/PM, starting clock face, clock
and shadow colours, night-dim hours, upside-down mounting, and an uploadable
boot picture. It saves to NVS and restarts. It has no brightness control.
It uses the same password as `/update`, and holds no secrets.
Tested from a phone on 2026-09-23: a 310×372 transparent PNG went through the
page's crop-and-shrink and arrived as a 16.7 KB baseline 240×240 JPEG, which the
orb stored and serves back. Not yet checked by eye on the displays.
Design and reasons: `docs/superpowers/specs/2026-09-23-web-settings-design.md`.
Not done yet: API keys on the page (needs the per-key status above first), WiFi
signal and last-API-call status, and enabling/disabling widgets. Flash is
87.0% of a slot, and the stop line is 92%.

## R2 — Over-the-air firmware updates

**Firmware updates must not require a USB cable.**

**Status 2026-09-23: implemented and tested on the SuperMini.** USB flash of the
new layout, then a browser-style multipart upload to `/update` by IP: 11 s
upload, back on the network 14 s later. otadata read back over USB afterwards:
highest sequence 2 → running `app1`, state `VALID`, i.e. the new image reached
WiFi and confirmed itself. Not yet exercised: `pio run -e ota` (espota), a
password, an interrupted upload, and an actual rollback.
`firmware/src/core/ota/OtaUpdater.*`, browser upload at `/update` plus
ArduinoOTA (`pio run -e ota -t upload`). Partition table reworked to two
1.875 MB slots (app at 83.4%) with the filesystem cut to 128 KB. A new image is
confirmed only after it reaches WiFi and starts listening for the next update,
using the core's `verifyRollbackLater()` hook; the prebuilt bootloader has
rollback enabled. Version shows on the boot screen and the update page. Still
open: the *Should* — devices do not check for updates themselves.

The orbs sit on a desk or shelf, often in a case. Fixing a bug currently means
retrieving the device, opening it if necessary, finding a data-capable USB
cable, and running a toolchain the owner may not have installed. In practice
that means fixes do not reach people.

**Must:**
- Accept a firmware image over the network and apply it
- Fall back safely on a failed or interrupted update — a half-written image
  must not brick the device
- Report the running firmware version somewhere visible

**Should:**
- Check for updates rather than requiring a push to each device

**Constraint worth checking early:** the current build occupies 77.2% of a 2 MB
app partition. Conventional OTA wants two app partitions so the old image
survives until the new one is verified. Two 2 MB partitions do not fit in 4 MB
alongside NVS and SPIFFS. **The partition table in `partitions.csv` will need
reworking, and that is a bigger change than it sounds** — it is worth confirming
the layout before building against the current one.

## Prior art

[JoeAWagner/orbital-orbs](https://github.com/JoeAWagner/orbital-orbs) — Info
Orbs firmware with a web control panel and OTA updates, written for the ESP32-S3
variant. Both requirements here, already implemented by someone else. Read it
before writing anything: it may be adoptable directly, and where it is not, it
will still show where the partition and NVS problems actually bite.

## Open question — ESPHome

Whether these requirements are better met by reimplementing on ESPHome rather
than extending the Arduino firmware. ESPHome provides OTA, a web server, and
`secrets.yaml` natively rather than as features to build. Evaluated separately;
see `docs/ESPHOME-EVALUATION.md`.
