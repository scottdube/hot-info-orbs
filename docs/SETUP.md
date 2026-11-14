# Building the Info Orbs Software Yourself

> **Just want it running?** You do not need anything on this page. The
> [Quick start](../README.md#quick-start-no-software-to-install-on-your-pc) flashes a
> ready-made file from Chrome or Edge in about 10 minutes, and you paste your
> free weather API key on the settings page afterwards. This guide is for building the
> software yourself.
>
> **Already running?** Using the orbs is in the [user guide](USER-GUIDE.md).
> When something goes wrong, see [Troubleshooting](TROUBLESHOOTING.md).

Written for someone who has never used PlatformIO. If you have, skip to
[step 3](#3-create-your-two-config-files) — the only thing that has changed from
upstream is that there are now **two** files to copy, not one.

Budget about 45 minutes the first time. Most of that is downloads.

---

## What you need before you start

- The assembled orbs hardware (Seeed XIAO ESP32-S3 or XIAO ESP32-S3 Plus + five round displays + three buttons)
- A **USB data cable**. Many cheap USB cables carry power only. If the board
  never shows up as a port, suspect the cable before anything else.
- A 2.4 GHz WiFi network. The ESP32 cannot see 5 GHz networks at all.
- About 1 GB of free disk for the toolchain.

---

## 1. Install VS Code and PlatformIO

1. Install [Visual Studio Code](https://code.visualstudio.com/).
2. Open VS Code, click the **Extensions** icon in the left bar (four squares).
3. Search for `PlatformIO IDE` and install it.
4. **Restart VS Code, then wait.** On first launch PlatformIO downloads and
   builds its own Python environment. This takes several minutes and shows a
   progress notice in the bottom-right. Do not start a build until it finishes.

> PlatformIO redoes this bootstrap after some VS Code updates. If builds
> suddenly break on a machine that used to work, that is usually why — see
> [Troubleshooting](TROUBLESHOOTING.md#building-from-source).

## 2. Get the code

If you use git:

```bash
git clone https://github.com/scottdube/hot-info-orbs.git
cd hot-info-orbs
```

If you don't: click the green **Code** button on the repository page, choose
**Download ZIP**, and unzip it somewhere you can find again. Avoid a path with
spaces or accents in it — some toolchain steps still trip on those.

Then in VS Code: **File → Open Folder**, and pick the `hot-info-orbs` folder
(from a ZIP it may be `hot-info-orbs-main`). Opening
the folder (not a single file) is what makes PlatformIO recognize the project.

## 3. Create your two config files

In `firmware/config/` you will find two `.template` files. Make a copy of each,
in the same folder, with the `.template` dropped from the name:

| Copy this            | To this     | What it holds                                |
|----------------------|-------------|----------------------------------------------|
| `config.h.template`  | `config.h`  | your preferences — timezone, units, tickers  |
| `secrets.h.template` | `secrets.h` | your API keys, and WiFi/MQTT if you use them |

```bash
cd firmware/config
cp config.h.template config.h
cp secrets.h.template secrets.h
```

**Both are required.** The build refuses to start without them and tells you
which one is missing. That is deliberate — better than a blank orb on the bench.

**Why two files:** `config.h` and `secrets.h` are both ignored by git, so nothing
personal ever gets committed. The `.template` files are the ones tracked in git,
which is why they must never contain real values. If you ever edit a `.template`
file, put placeholders in it, not your own keys.

## 4. Get your API key

One key, free, about a minute, and **you need your own**. A key shared across
several orbs burns one free-tier quota and gets rate-limited, and whoever owns
it can revoke it without warning.

| Key               | Register at                                                          | What it does                                                                                                     |
|-------------------|----------------------------------------------------------------------|------------------------------------------------------------------------------------------------------------------|
| `WEATHER_API_KEY` | [visualcrossing.com/sign-up](https://www.visualcrossing.com/sign-up) | Current conditions and forecast for the weather orb. Free tier is ~1000 records/day, far more than one orb uses. |

Paste it between the quotes in `secrets.h`:

```c
#define WEATHER_API_KEY "your-key-here"
```

Leaving it empty fails the build with a message naming the key. It will not let
you flash a half-configured orb.

**No time zone key.** Until firmware 1.2.0 the orb asked TimeZoneDB for its
offset, which took a second key. Now the time comes from NTP
(`pool.ntp.org`) as before, and the firmware turns it into local time from a
table of daylight-saving rules built into it
(`firmware/src/core/settings/TimeZones.h`). An old `secrets.h` that still has
`TIMEZONE_API_KEY` builds fine; the line is ignored.

## 5. Set your preferences in `config.h`

Open `firmware/config/config.h`. Everything above the
`END OF USER CONFIGURATION` banner is meant to be edited; below it is hardware
wiring you should leave alone unless you built a variant board.

The four settings most people want to change:

```c
#define TIMEZONE_API_LOCATION "America/New_York"  // a name from TimeZones.h
#define WEATHER_LOCATION "The Villages, FL"       // city, state
//#define WEATHER_UNITS_METRIC                    // uncomment for celsius
#define STOCK_TICKER_LIST "SPY,QQQ,AAPL,MSFT,BTC/USD"  // exactly 5
```

Worth knowing about the ticker list: it takes five symbols. Crypto and forex
need a currency pair (`BTC/USD`). For a symbol that trades on more than one
exchange, append the country (`APC&country=Germany`).

Other things you can turn on here: dim hours and screens-off hours (separate schedules), a Nixie-tube clock face,
24-hour time, widget auto-cycling. Each is commented in the file.

## 6. Build and upload

1. Plug the orbs into USB.
2. Click the **PlatformIO icon** (the alien head) in the left bar.
3. Under **Project Tasks → xiao-esp32s3 → General**, click **Upload**.

The first build downloads the ESP32 platform and compiler — several hundred MB,
several minutes, once. Later builds take about 15 seconds.

When it starts, one orb shows **Welcome** and how full the firmware slot is,
e.g. `Flash 43%`. The project keeps that under 92% so updates always fit.

If upload cannot find the board, put it in download mode by hand: unplug the
USB cable, hold the **B** (boot) button on the XIAO, plug the cable back in,
release, then click Upload. (The XIAO's two buttons are marked B and R. If
no new port appears, try the other button: R only restarts the board.)

## 7. Connect it to WiFi

The orbs create their own WiFi network on first boot, and the screens show its
name: `Info-Orbs_` followed by two characters, for example `Info-Orbs_84`.
On a phone or computer, join that network, and a configuration page opens
automatically (if it does not, browse to `http://192.168.4.1`). Pick your home
network from the list and enter its password.

**The two characters are different on every orb.** They are the last two
characters of that orb's WiFi MAC address, so yours will not match the `84` in
these docs or in the video. Read the name off your own screens. This keeps
two orbs being set up in the same room from advertising the same network;
with only 256 possible endings two orbs can still match, so set them up one
at a time.

This is stored on the device, not in your files. You do not reflash to change
WiFi later: hold the **right** button while plugging in power. The orbs show
`Wifi Settings reset`, forget the network, and open the setup network again.
The orbs remember only one network at a time.

If the orbs cannot join, orbs 4 and 5 say why. The user guide covers
[what they do by themselves when WiFi fails](USER-GUIDE.md#when-wifi-goes-down),
and Troubleshooting has [each screen and its fix](TROUBLESHOOTING.md#wifi-setup).

## 8. Later updates: no cable needed

Once the orbs are on your WiFi, new firmware goes over the network. The **first**
flash has to be over USB (step 6), because it also writes the partition layout
that makes network updates possible.

- **From a browser:** build as usual, then open `http://info-orbs.local/update`,
  choose `.pio/build/xiao-esp32s3/firmware.bin` and press Upload. If
  `.local` names do not resolve on your computer, use the IP address shown on
  the orbs when they connect.
- **From PlatformIO:** `pio run -e ota -t upload`. This needs the orb to connect
  *back* to your computer, which fails when the two are on different subnets,
  and a failed attempt restarts the orb. If it says `No response from device`,
  wait until the update page loads again and use the browser.

**The password is optional.** Without one, anyone on your network can flash
the orbs or change their settings, and the settings and update pages show a
⚠ warning saying so. To set one: in `secrets.h`, find the line
`//#define OTA_PASSWORD ""`, delete the `//` at its start, type a password
between the quotes, save, and upload again (step 6). The browser then asks
for user `admin` and that password on the update, settings and log pages.
A password is part of the build, so the ready-made release file has none.

A failed or interrupted update keeps the old firmware. A new firmware that
installs but cannot get back on WiFi is rolled back when it next restarts.

### Flashing the release files over USB

The README keeps novices to two safe moves: the
[Quick start](../README.md#quick-start-no-software-to-install-on-your-pc) file
at `0x0`, and [updates over WiFi](../README.md#updating-later-over-wifi-no-cable). If you flash the [release files](https://github.com/scottdube/hot-info-orbs/releases)
by hand instead, know the layout (`partitions.csv`):

| Offset     | What is there                                            |
|------------|----------------------------------------------------------|
| `0x0`      | bootloader (S3: not `0x1000` as on the classic ESP32)    |
| `0x8000`   | partition table                                          |
| `0x9000`   | NVS: WiFi, settings, the weather key                     |
| `0xE000`   | `otadata`: which app slot boots                          |
| `0x10000`  | `app0`                                                   |
| `0x3D0000` | `app1`                                                   |
| `0x790000` | filesystem: the uploaded start-up picture                |

- **`info_orbs_fw_combo.bin` goes at `0x0` only.** It is the whole 8 MB chip,
  blank everywhere past the app, so it is also a factory reset.
- **`firmware.bin` never goes at `0x0`.** That overwrites the bootloader and
  the orbs stay dark until the combo file is flashed.
- **`firmware.bin` at `0x10000` may not take.** WiFi updates alternate between
  the two app slots, and `otadata` records which one boots. After an odd number
  of WiFi updates the orb runs from `app1`, so writing `app0` alone changes
  nothing: it reboots into the old firmware. Erase `otadata` in the same
  session so the bootloader falls back to `app0`:

  ```bash
  esptool.py --chip esp32s3 write_flash 0x10000 firmware.bin
  esptool.py --chip esp32s3 erase_region 0xe000 0x2000
  ```

  `pio run -t upload` never hits this: it writes `boot_app0.bin` to `0xE000`
  with every upload. The orbs' `/update` page never hits it either, which is
  why the Quick start uses it.

## Building more than one set

Many members build several. Each set keeps its own WiFi network, settings-page
choices and start-up picture inside itself, so you set each one up on its own.
You can use the same API keys and the same `config.h` and `secrets.h` for all
of your own sets. Each set uses the keys' free daily allowance separately.

**WiFi: one at a time, or not at all.** Each new set starts its own setup
network, `Info-Orbs_` plus two characters. Two sets can end up with the same
two characters, so set them up one at a time. To skip the setup network
altogether, put your WiFi name and password in `secrets.h` before you flash:
find `//#define WIFI_SSID` and `//#define WIFI_PASS`, delete the `//` from
both, and fill them in. Every set you flash then joins that network by itself.

**Names: give each set its own.** Every set is called `info-orbs` unless you
change it, and only one of them can answer to `info-orbs.local`. Before you
flash a set, open `config.h`, find `//#define OTA_HOSTNAME "info-orbs"`,
delete the `//`, and change the name, for example to `"orb-office"`. That set
is then at `http://orb-office.local/settings`, and your router lists it under
that name. The IP address shown on its screen always works too.

**The name is built into the software file, so watch it when updating.** The
name in `config.h` when you build is the name the set gets. If you build once
and upload the same file to two sets, they both end up with the same name.
To update several sets, change the name in `config.h`, build, upload to that
set, and repeat for the next one. Sets installed from the ready-made release
file are all called `info-orbs`; reach those by IP address.

**Optional: update from PlatformIO by name.** Settings that belong to your
computer, such as which USB port to upload to or which set to update over
WiFi, go in a file named `platformio_local.ini` next to `platformio.ini`.
Create it only if you need it; it stays on your computer and is never uploaded
to GitHub. For example, so that *Upload* in the `ota` task updates
`orb-office`:

```ini
[env:ota]
upload_port = orb-office.local
```

---

*Sections 9 to 11 and the troubleshooting table moved out of this guide in
November 2026. The headings stay so that older links still land.*

## 9. Change settings from a browser

Moved to the user guide: [The settings page](USER-GUIDE.md#the-settings-page),
[The log](USER-GUIDE.md#the-log-170) and
[What you see at start-up](USER-GUIDE.md#what-you-see-at-start-up).

For a build of your own, two things differ from the ready-made file:

- The built-in values are the ones in your `config.h`. They are only the
  starting point: whatever the settings page saves wins, and **Reset to
  built-in values** goes back to what `config.h` says.
- Settings survive restarts and firmware updates. A USB flash with **Erase
  Flash** first clears them.

## 10. Optional: weather from your own Tempest stations

Moved to the user guide:
[Weather from your own Tempest stations](USER-GUIDE.md#weather-from-your-own-tempest-stations).
Nothing to build: it works on the ready-made release file too.

Building it yourself, you can instead put the token in `secrets.h`
(`#define TEMPEST_TOKEN "..."`) and the stations in `config.h` (the commented
`TEMPEST_STATION_1` lines in the template). Those are only defaults: whatever
the page saves wins. In firmware 1.2.0 and earlier the token could only come from
`secrets.h`, and without it the Tempest code was not compiled in at all.

## 11. Optional: run it from a phone hotspot

Moved to the user guide:
[Running from a phone hotspot](USER-GUIDE.md#running-from-a-phone-hotspot).

## Troubleshooting

Moved to [TROUBLESHOOTING.md](TROUBLESHOOTING.md), organized by what you see.
Build errors are under
[Building from source](TROUBLESHOOTING.md#building-from-source).
