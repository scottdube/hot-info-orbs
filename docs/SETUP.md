# Setting Up Info Orbs From Scratch

Written for someone who has never used PlatformIO. If you have, skip to
[step 3](#3-create-your-two-config-files) — the only thing that has changed from
upstream is that there are now **two** files to copy, not one.

Budget about 45 minutes the first time. Most of that is downloads.

---

## What you need before you start

- The assembled orbs hardware (ESP32-S3 SuperMini + five round displays + three buttons)
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

## 4. Get your two API keys

Both are free, both take about a minute, and **you need your own**. A key shared
across several orbs burns one free-tier quota and gets rate-limited, and whoever
owns it can revoke it without warning.

| Key                | Register at                                                          | What it does                                                                                                     |
|--------------------|----------------------------------------------------------------------|------------------------------------------------------------------------------------------------------------------|
| `WEATHER_API_KEY`  | [visualcrossing.com/sign-up](https://www.visualcrossing.com/sign-up) | Current conditions and forecast for the weather orb. Free tier is ~1000 records/day, far more than one orb uses. |
| `TIMEZONE_API_KEY` | [timezonedb.com/register](https://timezonedb.com/register)           | Your UTC offset and the next daylight-saving changeover.                                                         |

A note on the second one, because the name misleads: TimeZoneDB is **not** where
the orb gets the time. The time comes from NTP (`pool.ntp.org`). TimeZoneDB
supplies only the offset and the DST transition date — which is how the clock
fixes itself every March and November without anyone reflashing it.

Paste each key between the quotes in `secrets.h`:

```c
#define WEATHER_API_KEY "your-key-here"
#define TIMEZONE_API_KEY "your-key-here"
```

Leaving one empty fails the build with a message naming the key. It will not let
you flash a half-configured orb.

## 5. Set your preferences in `config.h`

Open `firmware/config/config.h`. Everything above the
`END OF USER CONFIGURATION` banner is meant to be edited; below it is hardware
wiring you should leave alone unless you built a variant board.

The four settings most people want to change:

```c
#define TIMEZONE_API_LOCATION "America/New_York"  // from https://timezonedb.com/time-zones
#define WEATHER_LOCATION "The Villages, FL"       // city, state
//#define WEATHER_UNITS_METRIC                    // uncomment for celsius
#define STOCK_TICKER_LIST "SPY,QQQ,AAPL,MSFT,BTC/USD"  // exactly 5
```

Worth knowing about the ticker list: it takes five symbols. Crypto and forex
need a currency pair (`BTC/USD`). For a symbol that trades on more than one
exchange, append the country (`APC&country=Germany`).

Other things you can turn on here: nighttime dimming, a Nixie-tube clock face,
24-hour time, widget auto-cycling. Each is commented in the file.

## 6. Build and upload

1. Plug the orbs into USB.
2. Click the **PlatformIO icon** (the alien head) in the left bar.
3. Under **Project Tasks → esp32-s3-devkitc-1 → General**, click **Upload**.

The first build downloads the ESP32 platform and compiler — several hundred MB,
several minutes, once. Later builds take about 15 seconds.

When it starts, one orb shows **Welcome** and how full the firmware slot is,
e.g. `Flash 87%`. The project keeps that under 92% so updates always fit.

If upload cannot find the board, put it in download mode by hand: unplug the
USB cable, hold the **BOOT** button on the SuperMini, plug the cable back in,
release, then click Upload. (The BOOT and RST labels can be hard to read. If
no new port appears, try the other button: RST only restarts the board.)

## 7. Connect it to WiFi

The orbs create their own WiFi network on first boot. On a phone or laptop, join
the network the orbs advertise, and a configuration page opens automatically.
Pick your home network from the list and enter its password.

This is stored on the device, not in your files. You do not reflash to change
WiFi later — hold the buttons per the on-screen prompts to reopen the portal.

## 8. Later updates: no cable needed

Once the orbs are on your WiFi, new firmware goes over the network. The **first**
flash has to be over USB (step 6), because it also writes the partition layout
that makes network updates possible.

- **From a browser:** build as usual, then open `http://info-orbs.local/update`,
  choose `.pio/build/esp32-s3-devkitc-1/firmware.bin` and press Upload. If
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

## 9. Change settings from a browser

Open `http://<orb's IP>/settings` (or `http://info-orbs.local/settings`) on a
phone or computer on the same network. It uses the same optional `admin` /
`OTA_PASSWORD` login as the update page. The page never shows your API keys.

- **Save and restart** stores the changes and restarts the orbs, about 15 s.
  The page reloads by itself. A value that fails its check is shown beside
  that field, and nothing is saved.
- **Reset to built-in values** forgets everything saved from the page and goes
  back to what `config.h` says.
- **Start-up picture:** pick any photo. The page crops it to a centred square
  and shows the circle the orb will display. **Upload and restart** puts it on the
  middle orb at start-up. **Use the built-in logo** takes it off again.

Settings survive restarts and firmware updates. A USB flash with **Erase
Flash** first clears them.
Timezone and weather location cannot be checked until the orb uses them. If
you mistype one, the clock or weather orb shows the error.

---

## Troubleshooting

| Symptom                                           | Cause and fix                                                                                                                                                                                |
|---------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `Copy firmware/config/secrets.h.template…`        | You missed step 3. Copy both templates.                                                                                                                                                      |
| `WEATHER_API_KEY is empty…`                       | `secrets.h` exists but a key is still `""`. See step 4.                                                                                                                                      |
| `ModuleNotFoundError: No module named 'intelhex'` | PlatformIO bootstrapped a fresh Python environment (often after a VS Code update) and missed a dependency of its flashing tool. Fix: `~/.platformio/penv/bin/python -m pip install intelhex` |
| No serial port listed                             | Suspect the USB cable first — many are charge-only. The SuperMini uses the ESP32-S3's own USB, so no CP2102/CH340 driver is needed. If the orbs still answer on WiFi, they are fine: enter download mode (step 6) and reflash. |
| Upload starts then fails                          | Enter download mode by hand: hold **BOOT** while plugging the cable in, then Upload (step 6).                                                                                                |
| Clock right, weather orb blank                    | The weather key is wrong or its quota is spent. A bad key fails silently at runtime — it cannot be caught at build time.                                                                     |
| Never finds your WiFi                             | The network is 5 GHz. The ESP32 only sees 2.4 GHz.                                                                                                                                           |
| Builds broke on a machine that worked before      | A VS Code update replaced PlatformIO's Python environment. Reopen the project folder in VS Code and let the bootstrap finish, then see the `intelhex` row above.                             |
| Time is an hour off after a DST change            | The timezone key is wrong or out of quota — that is the service that supplies the changeover.                                                                                                |
