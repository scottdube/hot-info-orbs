# Info Orbs Troubleshooting

Find what you see, then do what it says. The five displays are **orb 1 to
orb 5, left to right**.

Almost nothing here can damage the orbs. The worst case is always the same
fix: do the
[Quick start](../README.md#quick-start-no-software-to-install-on-your-pc)
again.

> **Parts marked (1.7.0)** are in the next firmware release. Firmware 1.6.1
> does not have them.

- [Installing](#installing)
- [WiFi setup](#wifi-setup)
- [Running](#running)
- [The settings page](#the-settings-page)
- [Updating](#updating)
- [Building from source](#building-from-source)
- [Getting help](#getting-help)

---

## Installing

Problems during the
[Quick start](../README.md#quick-start-no-software-to-install-on-your-pc).

| What you see | What to do |
|---|---|
| ESPConnect will not connect at all, or says the browser is not supported | Use Google Chrome or Microsoft Edge on a computer. Safari, Firefox and phones cannot talk to USB devices. |
| **Connect** lists no port for the orbs | 1. Suspect the cable first: many USB cables carry power only. Try one you know moves data, such as a phone's own cable. 2. Put the XIAO in download mode: unplug, hold the **B** button on the XIAO, plug back in, let go, click **Connect** again. The XIAO's two small buttons are marked B and R; R only restarts it. 3. No driver is needed on Windows or Mac. |
| Flashing starts, then stops with an error | Put the XIAO in download mode (the row above) and flash again. If it still fails, try another cable or another USB port on the computer. |
| Flashing finished, but the orbs stay dark | Unplug the USB cable and plug it back in. If they are still dark, flash again and check the two boxes: the file is `info_orbs_fw_combo.bin` and **Flash offset** is `0x0`. The smaller `firmware.bin` is for WiFi updates only; flashed over USB it leaves the orbs dark until the combo file goes in. |
| The orbs show **Set up WiFi** | Nothing is wrong. That is step 6 of the Quick start. |

## WiFi setup

Orbs 4 and 5 tell you what happened. A red **X** above the words means a join
failed.

| What you see | What it means, and what to do |
|---|---|
| **Set up WiFi**, a network name starting `Info-Orbs_`, and `192.168.4.1` | The orbs have no WiFi network saved. On a phone, join the network named on the screen. A setup page opens; choose **Configure WiFi**, pick your network, type its password, press **Save**. |
| You joined `Info-Orbs_..` but no page opened | Browse to `http://192.168.4.1`. If the phone says the network has no internet, that is expected: stay on it. If the page still does not load, turn off the phone's mobile data for a minute, or use a laptop. |
| Your network is not in the setup page's list | The orbs see only **2.4 GHz** WiFi. A network that is 5 GHz only never appears. On a phone hotspot, see [Running from a phone hotspot](USER-GUIDE.md#running-from-a-phone-hotspot). Otherwise move the orbs closer to the router and press **Refresh**. |
| Red X, **WiFi not found**, and your network's name | The orbs cannot hear the network they were given. Is the router on? Are the orbs in range? Was the name typed by hand and misspelled? The orbs keep listening and join by themselves when they hear it (1.7.0). To give them a different network, join `Info-Orbs_..` and open `http://192.168.4.1`. |
| Red X, **Wrong password**, and your network's name | The router refused the password. Join `Info-Orbs_..`, open `http://192.168.4.1`, and type it again. Capitals matter. |
| **Wrong password**, and you are sure it is right | A router that has **blocked** the orbs refuses them the same way. Look in the router's settings for a block list or access control. On some routers, after unblocking you must also remove ("forget") the orbs from the router's device list before it lets them back in. |
| Red X, **WiFi didn't connect**, and your network's name | The orbs reached the network but did not finish joining. Unplug them and plug them back in to try again at once. Left alone, they retry after 5 minutes, then 10, then 20, then every 30 minutes (1.7.0). |
| The orbs are on a failure screen and you cannot find `Info-Orbs_..` on your phone | Stand near the orbs and refresh the phone's WiFi list. If it never shows, unplug the orbs, hold the **right** button, and plug them back in: they show `Wifi Settings reset` and start the setup network fresh. |
| Two sets show the same `Info-Orbs_..` name | It can happen: there are only 256 endings. Set them up one at a time, with the other unplugged. |
| You want to move the orbs to another network | [Changing the WiFi network](USER-GUIDE.md#changing-the-wifi-network). |

## Running

### Red marks (1.7.0)

| What you see | What it means, and what to do |
|---|---|
| **No WiFi** on a red plate on orb 2 | WiFi is down. Wait: the orbs try to rejoin every 30 seconds. If the router restarted, give it a few minutes. [When WiFi goes down](USER-GUIDE.md#when-wifi-goes-down) has the timings. |
| **No WiFi** that comes and goes through the day | The signal is weak where the orbs sit. Hold the middle button 2 seconds and read orb 2: `fair` or `weak` is the cause. Move the orbs or the router closer together. |
| **No WiFi** that never clears, though phones are online | Unplug the orbs and plug them back in. If they cannot join, orbs 4 and 5 say why: see [WiFi setup](#wifi-setup). |
| A **broken** red ring around some orbs | WiFi has been down long enough that the data inside the ring is old. It clears when WiFi returns. |
| A **solid** red ring, no **No WiFi** | WiFi is up, but fresh data is not arriving. Check that the internet works on your phone, on the same WiFi. If only the weather page has the ring, see "Weather worked, then stopped" below. |

### Pages

| What you see | What it means, and what to do |
|---|---|
| Weather page is empty, the clock is right | Open the settings page. If **API keys** says `No key yet`, paste your Visual Crossing key and press **Save and restart**. |
| A key is saved and the weather page is still empty | Paste the key again, and check **Weather location** under **Time zone and location**: write the town and state, such as `Orlando, FL`. |
| Weather worked, then stopped, and the internet is fine | The free key has a daily allowance. One set uses a small part of it; a key shared between people can run out. It starts again the next day. |
| A Tempest page is missing | A station gets a page only when a token is saved. On the settings page, **Tempest stations** shows ✔ or ✖ and the error for each station. |
| Stocks page shows nothing for one orb | Check that ticker's spelling under **Stocks**. Crypto is written `BTC/USD`. |
| A page you set up does not appear when you press the buttons | It is not ticked. Settings page, **Rotation > Show**. |
| The orbs stay on one page | **Seconds per page** is 0, or only one page is ticked. Settings page, **Rotation**. |
| An MQTT or Web Data page is empty | The settings page shows each one's last result under **Advanced**. See [CUSTOM-PAGES.md](CUSTOM-PAGES.md). |

### Time

| What you see | What it means, and what to do |
|---|---|
| The time is wrong by whole hours | The time zone is not yours: new orbs start on New York. Settings page, **Time zone and location > Time zone**, then **Save and restart**. Daylight saving is automatic and needs no key. |
| Your time zone is not in the list | Ask at the club, or [open an issue](https://github.com/scottdube/hot-info-orbs/issues), and it gets added. |

### Screens and buttons

| What you see | What it means, and what to do |
|---|---|
| All five orbs are dark | Press any button. If they light up, it is **Screens-off times** on the settings page doing its job. If not, check the cable and the power supply. |
| A button press does nothing | During screens-off hours the first press only wakes the screens. The press that closes the info page does nothing else. Left and right move only between ticked pages. |
| One orb is lit but blank, the others are fine | Unplug the orbs and plug them back in. This has been seen once and a restart cleared it; the cause is not known. If it comes back, note which orb and whether it is the same one each time, and [tell us](#getting-help). |
| The orbs are upside down, or in the wrong order | Settings page, **Mounting**. |
| The eyes play over and over at start-up | **Eye lids** is set to **Demo: show each in turn**. Pick a style under **Start-up eyes**. |
| The orbs restart by themselves | Hold the middle button 2 seconds and read **last start** on orb 4 (1.7.0). `restart` after WiFi trouble is the orbs getting themselves back on, on purpose. `brownout` means the power dipped: try another cable or power supply. `crash` or `watchdog` more than once: [send the log](#getting-help). |

## The settings page

| What you see | What it means, and what to do |
|---|---|
| The browser cannot open `http://<address>/settings` | 1. The phone or computer must be on the **same WiFi** as the orbs. A guest network usually cannot reach them. 2. Read the address again: it can change. Orb 5 shows it at start-up, and orb 3 of the info page shows it any time (1.7.0). 3. Type `http://` at the front. The orbs do not answer `https://`. |
| `http://info-orbs.local/settings` does not open | Use the numbers instead. The name does not work on every network or every phone, and with more than one set only one of them answers to it. |
| The browser asks for a user name and password | Whoever built the software set one. The user name is `admin`. |
| **Save and restart** shows a message beside a field | That value is not allowed, and nothing was saved. Fix the field and save again. |
| `Not saved: paste the key again` under a key | Another field had an error, so the key you pasted was not kept. Fix the error, paste the key again, and save. |

## Updating

Problems with a
[WiFi update](../README.md#updating-later-over-wifi-no-cable).

| What you see | What it means, and what to do |
|---|---|
| The orbs show **Update failed, old firmware kept** | Nothing changed; the orbs keep running the old version. Check that you chose `firmware.bin` (not the combo file, which is for USB) and upload again. |
| The page still shows the old version after an update | Look too soon and the old version is still answering: the orbs take several seconds to restart. Wait until the orbs show a page again, reload, and read the first line. |
| The orbs do not come back after an update | Unplug them and plug them back in. A new version that cannot get back on WiFi is undone at the next restart, and the old one runs again. |
| Still dark | Do the [Quick start](../README.md#quick-start-no-software-to-install-on-your-pc) again. It wipes the board, so WiFi and the weather key go back in afterwards. |

## Building from source

Only if you build the software yourself with the
[build guide](SETUP.md).

| What you see | What it means, and what to do |
|---|---|
| `Copy firmware/config/secrets.h.template…` | You missed [step 3](SETUP.md#3-create-your-two-config-files). Copy both templates. |
| `WEATHER_API_KEY is empty…` | `secrets.h` exists but the key is still `""`. See [step 4](SETUP.md#4-get-your-api-key). |
| `ModuleNotFoundError: No module named 'intelhex'` | PlatformIO set up a fresh Python environment, often after a VS Code update, and missed a part of its flashing tool. Fix: `~/.platformio/penv/bin/python -m pip install intelhex` |
| Builds broke on a machine that worked before | A VS Code update replaced PlatformIO's Python environment. Reopen the project folder in VS Code, let the set-up notice at the bottom right finish, then see the `intelhex` row above. |
| No serial port listed | Suspect the USB cable first: many carry power only. The XIAO uses the ESP32-S3's own USB, so no CP2102 or CH340 driver is needed. If the orbs still answer on WiFi they are fine: enter download mode ([step 6](SETUP.md#6-build-and-upload)) and upload again. |
| Upload starts, then fails | Enter download mode by hand: hold **B** while plugging the cable in, then Upload. |
| `pio run -e ota -t upload` says `No response from device` | The orbs have to connect back to your computer, which fails across subnets, and a failed try restarts them. Wait until the update page loads again and upload from the browser. |
| Opening a serial monitor restarts the orbs | That is the XIAO's USB port. To see what the orbs have been doing, read `http://<address>/log` instead (1.7.0). |

[TRAPS.md](TRAPS.md) has the longer stories behind these, for developers.

## Getting help

Tell someone at the club, or
[open an issue](https://github.com/scottdube/hot-info-orbs/issues) (a free
GitHub account is needed). Three things make it quick to help you:

1. **What you see, and on which orb.** A phone photo of the five orbs is
   best.
2. **The version.** Hold the middle button 2 seconds and read orb 1: the
   version and the commit (1.7.0). On 1.6.1, it is the first line of the
   settings page.
3. **The log** (1.7.0). On the settings page, **Log: download** saves
   `info-orbs-log.txt`. Attach it. It holds events only: no network name, no
   passwords, no keys, no station IDs.

If the orbs cannot get on WiFi at all, the photo and what orbs 4 and 5 say
are enough to start.
