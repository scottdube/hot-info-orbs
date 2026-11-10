# Setting Up Info Orbs From Scratch

> **Just want it running?** You do not need anything on this page. The
> [Quick start](../README.md#quick-start-no-software-to-install-on-your-pc) flashes a
> ready-made file from Chrome or Edge in about 10 minutes, and you paste your
> free weather API key on the settings page afterwards. This guide is for building the
> software yourself.

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
> [Troubleshooting](#troubleshooting).

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

**The password is optional.** Set `OTA_PASSWORD` in `secrets.h` and the browser
asks for user `admin` and that password on both the update and settings pages.
Without one, anyone on your network can flash the orbs or change their
settings, and both pages show a ⚠ warning saying so.

A failed or interrupted update keeps the old firmware. A new firmware that
installs but cannot get back on WiFi is rolled back when it next restarts.

### Flashing the release files over USB

The [Quick start](../README.md#quick-start-no-software-to-install-on-your-pc)
keeps novices to two safe moves: the factory reset file at `0x0`, and updates
over WiFi. If you flash the [release files](https://github.com/scottdube/hot-info-orbs/releases)
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

## 9. Change settings from a browser

Open `http://<orb's IP>/settings` (or `http://info-orbs.local/settings`) on a
phone or computer on the same network. It uses the same optional `admin` /
`OTA_PASSWORD` login as the update page. The page never shows your API keys.

- **Save and restart** stores the changes and restarts the orbs, about 15 s.
  The page reloads by itself. A value that fails its check is shown beside
  that field, and nothing is saved.
- **Reset to built-in values** forgets everything saved from the page and goes
  back to what `config.h` says.
- **Start-up picture:** pick any photo. The page crops it to a centered square
  and shows the circle the orb will display. **Upload and restart** puts it on the
  middle orb at start-up. **Use the built-in logo** takes it off again.
- **Waking eyes** (Start-up): on by default. While the orb joins WiFi, orbs 2
  and 4 show a pair of eyes waking up, with the bridge of a circuit-board nose
  on the middle orb; once it connects they open fully and look around while the
  data loads (orb 5 names what is loading). Then the start-up picture shows
  alone for 2 seconds, and the first page comes up. Off gives the plain
  start-up text, with the picture on the middle orb from the start.

Settings survive restarts and firmware updates. A USB flash with **Erase
Flash** first clears them.
Timezone and weather location cannot be checked until the orb uses them. If
you mistype one, the clock or weather orb shows the error.

## 10. Optional: weather from your own Tempest stations

If you own a WeatherFlow Tempest station, the orbs can show its weather, with
one weather page per station (up to two). If a Visual Crossing key is also
set, the location under **Weather** keeps its page as well, so you can follow
a third place; without that key only the Tempest pages show.

Nothing to build: this works on the ready-made release file too.

1. Get a token: tempestwx.com > Settings > Data Authorizations > Create Token.
   Paste it on the settings page under **API keys** > **Tempest token**.
2. Find each station's ID: the number in the station's tempestwx.com address.
   Enter it under **Tempest stations** with a short label (16 characters at
   most, shown in place of the town name), then press **Save and restart**.
3. Each station now shows ✔ or ✖ for its last fetch, how long ago it was, and
   the error if it failed. A station with no token says so instead.

The page never shows the token once saved; leave the field blank to keep it.
Building it yourself, you can instead put the token in `secrets.h`
(`#define TEMPEST_TOKEN "..."`) and the stations in `config.h` (the commented
`TEMPEST_STATION_1` lines in the template). Those are only defaults: whatever
the page saves wins. In firmware 1.2.0 and earlier the token could only come from
`secrets.h`, and without it the Tempest code was not compiled in at all.

## 11. Optional: run it from a phone hotspot

For a demo away from home. The orbs remember only **one** WiFi network, so
joining the hotspot replaces your home network. Try it at home before the day.

1. **Turn on the hotspot, on 2.4 GHz.** The ESP32 cannot see 5 GHz.
   - iPhone: Settings > Personal Hotspot, turn on **Maximize Compatibility**.
     Without it, recent iPhones use 5 GHz and the orbs never find the hotspot.
     Stay on that screen until the orbs join: an iPhone stops advertising its
     hotspot when nothing has connected for a while.
   - Android: in the hotspot settings, set the band to 2.4 GHz.
2. **Hold the right button while plugging in power.** The orbs show
   `Wifi Settings reset`, then `Connect phone or PC to WiFi network:`, a name
   like `Info-Orbs_84` (your two characters differ; see step 7), and
   `192.168.4.1`.
3. **From a second device** (a laptop or another phone), join that
   `Info-Orbs_..` network. A second device keeps the hotspot phone a hotspot
   the whole time. The setup page opens by itself; if it does not, browse to
   `http://192.168.4.1`. Choose **Configure WiFi**, pick the hotspot, enter its
   password, **Save**.
4. The orbs show `Success`, then their IP address, and start fetching over the
   phone's mobile data. The settings page is `http://<that IP>/settings` from a
   device joined to the hotspot.
5. **Back home**, repeat steps 2 and 3 with your home network.

Custom pages that read from a server or MQTT broker on your home network show
no data while the orbs are on the hotspot. Untick them on the settings page
for the demo, or point them at something reachable from the internet.

---

## Troubleshooting

| Symptom                                           | Cause and fix                                                                                                                                                                                |
|---------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `Copy firmware/config/secrets.h.template…`        | You missed step 3. Copy both templates.                                                                                                                                                      |
| `WEATHER_API_KEY is empty…`                       | `secrets.h` exists but a key is still `""`. See step 4.                                                                                                                                      |
| `ModuleNotFoundError: No module named 'intelhex'` | PlatformIO bootstrapped a fresh Python environment (often after a VS Code update) and missed a dependency of its flashing tool. Fix: `~/.platformio/penv/bin/python -m pip install intelhex` |
| No serial port listed                             | Suspect the USB cable first — many are charge-only. The XIAO uses the ESP32-S3's own USB, so no CP2102/CH340 driver is needed. If the orbs still answer on WiFi, they are fine: enter download mode (step 6) and reflash. |
| Upload starts then fails                          | Enter download mode by hand: hold **B** while plugging the cable in, then Upload (step 6).                                                                                                |
| Clock right, weather orb blank                    | The weather key is wrong or its quota is spent. A bad key fails silently at runtime — it cannot be caught at build time.                                                                     |
| Never finds your WiFi                             | The network is 5 GHz. The ESP32 only sees 2.4 GHz.                                                                                                                                           |
| Builds broke on a machine that worked before      | A VS Code update replaced PlatformIO's Python environment. Reopen the project folder in VS Code and let the bootstrap finish, then see the `intelhex` row above.                             |
| Time is an hour off after a DST change            | The timezone key is wrong or out of quota — that is the service that supplies the changeover.                                                                                                |
