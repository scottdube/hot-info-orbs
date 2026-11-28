# Info Orbs User Guide

For orbs that are already running. If yours are not on WiFi yet, do the
[Quick start](../README.md#quick-start-no-software-to-install-on-your-pc)
first. If something looks wrong, go to
[Troubleshooting](TROUBLESHOOTING.md).

In this guide the five displays are **orb 1 to orb 5, left to right**, and
"the orbs" means the whole set.

**Contents**

- [The three buttons](#the-three-buttons)
- [The pages](#the-pages)
- [What you see at start-up](#what-you-see-at-start-up)
- [Finding the orbs' address](#finding-the-orbs-address)
- [The settings page](#the-settings-page)
- [The info page](#the-info-page)
- [What the amber and red marks mean](#what-the-amber-and-red-marks-mean)
- [When WiFi goes down](#when-wifi-goes-down)
- [Changing the WiFi network](#changing-the-wifi-network)
- [Running from a phone hotspot](#running-from-a-phone-hotspot)
- [Weather from your own station](#weather-from-your-own-station)
- [Dimming and switching off at night](#dimming-and-switching-off-at-night)
- [The log](#the-log)
- [The password](#the-password)
- [Starting over](#starting-over)

---

## The three buttons

Every press is a **tap** or a **hold**. A tap is any press you let go of
within 2 seconds, quick or slow. A hold is 2 seconds or more.

| Button | Tap | Hold 2 seconds |
|---|---|---|
| **Left** | previous page | nothing |
| **Right** | next page | nothing |
| **Middle** | the page's action: see the table below | the [info page](#the-info-page) |
| **Left and right together** | | lock the orbs on this page; the same again unlocks: see [Locking a page](#locking-a-page) |
| **Right** while plugging in power | | forget the WiFi network, after a 5-second countdown on the middle orb: see [Changing the WiFi network](#changing-the-wifi-network) |

The buttons keep their places when the orbs are set to **Upside down**: on an
upside-down set, "left" and "right" here are the buttons that are on the left
and right when the orbs are the right way up.

Left and right move only between the pages ticked under **Rotation > Show** on
the settings page.

A tap on the middle button on each page:

| Page | Middle tap |
|---|---|
| Clock | next clock face |
| Weather | orb 5 switches between the three-day highs and lows |
| Stocks | fetch the prices now |
| Earthquakes | fetch the feed now |
| MQTT | draw the page again (MQTT values arrive by themselves, so there is nothing to fetch) |
| Web Data | fetch the data now |

The orbs keep a clock face picked with the button through a restart. They
save it a few seconds after your last tap, so the settings page shows the
same face. 12-hour or 24-hour is set on the settings page only.

**During screens-off hours** the first press of any button only wakes the
screens for a minute. Press again to do something.

## The pages

Each page uses all five orbs.

| Page | What the orbs show |
|---|---|
| **Clock** | Hours on orbs 1 and 2, minutes on orbs 4 and 5, the colon and the seconds on orb 3. Normal (LED digits) or Nixie-tube face. |
| **Weather** | Orb 1: time, date and weekday. Orb 2: the conditions in words. Orb 3: the weather picture, sunrise and sunset, and the moon's phase. Orb 4: the temperature now, with today's high and low. Orb 5: the next three days. |
| **Stocks** | One ticker on each orb: price, change today, and the 52-week high and low. |
| **Earthquakes** | The five most recent earthquakes at or above a magnitude you choose, one per orb: the magnitude large, a short location, how long ago it was, and its depth. The ring is colored by magnitude (green, yellow, orange, red). From the public USGS feed; no key needed. |
| **Web Data** | A page of your own, drawn from a JSON file at a web address. |
| **MQTT** (up to two) | A page of your own: one live value on each orb from an MQTT broker, such as Home Assistant. |

Weather has one page for your town (Visual Crossing) and one for each
[weather station](#weather-from-your-own-station) you add (Tempest or
Ambient Weather). Web Data
and MQTT pages are set up on the settings page under **Advanced**;
[CUSTOM-PAGES.md](CUSTOM-PAGES.md) has the format and Home Assistant recipes.

**Staying on one page, or rotating.** New orbs stay on one page until you
press a button. To rotate by themselves, set **Rotation > Seconds per page**
on the settings page.

### Locking a page

**Hold left and right together for 2 seconds** to keep the orbs on the page
they show: the rotation stops. The middle orb shows **Locked** for a moment,
and a small padlock stays at the bottom of it while the lock is on. Do the
same again to unlock: the middle orb shows **Unlocked** and the padlock goes.

- Letting go of the two buttons does nothing else: it does not change the page.
- Left and right still change pages while locked, and the lock stays on the
  page you move to.
- **Clock only when WiFi is out** still takes over during an outage; the
  locked page comes back after it.
- A restart clears the lock.
- **Don't hold both buttons while plugging the orbs in.** Holding right at
  power-up starts the countdown to forget the WiFi network (let go to keep
  it). A hold carried over from power-up never locks.

**How often the data refreshes.** Weather every 10 minutes, stocks every
15 minutes, and earthquakes every minute, **while their page is on screen**. A page that is not on screen
does not fetch; it gets fresh data as soon as it comes up again. So with the
clock kept on screen, the info page can show weather that is hours old, and
that is normal. MQTT values change as the broker sends them, on screen or
not.

## What you see at start-up

With the waking eyes on, which is how new orbs come:

1. Eyes on orbs 2 and 4 wake up while the orbs join WiFi, with a nose on
   orb 3.
2. When WiFi joins, orb 5 shows **IP Address** and four numbers with dots,
   such as `192.168.1.57`, for 5 seconds. That is the orbs' address.
3. The eyes look around while the data loads. Orb 5 names each page as it
   loads.
4. Your start-up picture shows alone for 2 seconds, then the first page comes
   up.

With the eyes switched off, the orbs show the picture on orb 3 and plain
text: **Welcome** on orb 1 with how full the firmware space is, the version,
**Connecting**, then **Success** on orb 4 and the address on orb 5.

Joining takes up to about 45 seconds: three tries of 15 seconds. If the orbs
cannot join, orbs 4 and 5 say why. See
[WiFi setup](TROUBLESHOOTING.md#wifi-setup) in Troubleshooting.

## Finding the orbs' address

Everything you change, you change from a web browser on a phone or computer
**on the same WiFi as the orbs**. For that you need their address.

- **At start-up:** orb 5 shows it for 5 seconds. Unplug the orbs and plug
  them back in to see it again.
- **On the info page**: hold the middle button 2 seconds and read
  orb 3.
- **From your router:** its list of connected devices shows the orbs as
  `info-orbs`.

Then browse to:

| Address | What it is |
|---|---|
| `http://<address>/settings` | the [settings page](#the-settings-page) |
| `http://<address>/update` | install a new firmware version ([README](../README.md#updating-later-over-wifi-no-cable)); links to the newest one |
| `http://<address>/log` | the [log](#the-log) |

For example, `http://192.168.1.57/settings`.

`http://info-orbs.local/settings` often works too, with no numbers to
remember. Use the numbers if it does not. With more than one set, give each
its own name under **Name on the network** on the settings page: until you
do, every set answers to `info-orbs` and only one of them can be reached by
it.

The address can change after the router restarts. If a bookmark stops
working, read the address again.

## The settings page

`http://<address>/settings`

The top of the page shows the firmware version, how long the orbs have been
running, and the WiFi signal as a mark and a number in dBm: ✔ for -65 or
better, ⚠ to -75, ✖ below that (closer to zero is stronger; -60 is better
than -80).

Under the version, the page says whether a newer one is out:

| The line | What it means |
|---|---|
| **⬆ Version 1.9.0 is available.** Update page | A newer version is out. **Update page** takes you to `/update`, which has the download link. |
| ✔ This is the newest version. | Nothing to do. |
| Local build, not a release file. Newest release: 1.8.0. Update page | The orbs run software built from the source, not a downloaded release. Normal if you build your own. |
| This build is newer than the newest release, 1.8.0. | The same, with a version number ahead of the newest release. |
| Could not check for a newer version. | No answer within 10 seconds. See [Troubleshooting](TROUBLESHOOTING.md#updating). The orbs are fine. |

While it asks, the line reads **Checking for a newer version…**.

The phone or computer you are reading on does the asking, not the orbs: it
asks GitHub, where the releases are, each time the page loads. So the line
needs internet on that device, and the orbs never check by themselves.

Change what you want, then press **Save and restart** at the bottom. The orbs
restart, which takes about 15 seconds, and the page reloads by itself. If a
value is not allowed, the page says so beside that field and saves nothing.

Your settings stay through restarts, power cuts and
[WiFi updates](../README.md#updating-later-over-wifi-no-cable).

### Setup: once per set

| Section | What is in it |
|---|---|
| **Name on the network** | The set's name: `info-orbs` until you change it. Letters, digits and dashes. The set answers at `http://<name>.local/settings`, and your router lists it by this name. With more than one set, give each its own. After a save that changes the name, the page goes on to the new address by itself. |
| **API keys** | Your free [Visual Crossing](https://www.visualcrossing.com/sign-up) weather key, and your station's keys if you have one (a Tempest token, or Ambient Weather's two keys). Once saved, a key shows as stars. Leave the box alone to keep the key, or paste a new one to replace it. |
| **Time zone and location** | **Time zone**: pick from the list. Daylight saving is automatic. **Weather location**: your town, such as `Orlando, FL`. The page cannot check the town; a wrong one shows up as an empty weather page. |
| **Weather stations** | See [Weather from your own station](#weather-from-your-own-station). |
| **Mounting** | **Upside down** turns the screens over and reverses the order of the orbs, for a set mounted the other way up. The buttons do not move: left is still the button that is on the left when the orbs are the right way up. |

### Display: what the orbs show

| Section | What is in it |
|---|---|
| **Rotation** | **Seconds per page**: 0 stays on one page; 5 to 3600 moves to the next page after that many seconds. **Show**: tick the pages you want. Tick one to keep it on screen, or several to rotate through them. **Clock only when WiFi is out**: Off keeps the page up with its marks; On switches to the clock while WiFi is down. See [the marks](#what-the-amber-and-red-marks-mean). |
| **Clock** | The clock face (a middle tap on the clock page changes it too), 12-hour or 24-hour, the AM/PM mark, the digit color and the color of the unlit segments. |
| **Weather** | °F and mph, or °C and km/h. Light or dark screens. |
| **Stocks** | One to five tickers, with commas between them, one per orb. Crypto is written `BTC/USD`. To pick an exchange, add the country: `APC&country=Germany`. |
| **Earthquakes** | **Minimum magnitude**: the smallest earthquake to show, 0.0 to 10.0. 4.5 keeps the world's many small quakes off the orbs. Turn the page itself on with its tick box under **Rotation > Show**. |
| **Dim times**, **Screens-off times** | See [Dimming and switching off at night](#dimming-and-switching-off-at-night). |
| **Start-up eyes** | **Waking eyes** on or off, the eye lid style, eyelashes, and the iris color. **Demo: show each in turn** plays a wake-up in every style, with its name on orb 5, and keeps playing until you pick a style. While it plays, no pages show, the buttons do nothing and the start-up picture is skipped; the settings page still works. |
| **Start-up picture** | Pick any photo. The page crops it to a square and shows the circle the orb will display. **Upload and restart** puts it on orb 3 at start-up. **Use the built-in logo** takes it off again. |

### Advanced: pages of your own

The **Web Data page** and the two **MQTT pages**. See
[CUSTOM-PAGES.md](CUSTOM-PAGES.md).

### Reset to built-in values

The button beside **Save and restart**. It forgets every setting saved from
the page, your keys included, and asks before it does. WiFi and the start-up
picture are not touched.

## The info page

**Hold the middle button for 2 seconds.** The info page appears while you
are still holding. It works from any page. Press any button to close it, or
it closes by itself after 2 minutes. The numbers update once a second.

It tells you, without a computer, what you would otherwise look up on the
settings page. It is the first thing to read when something looks wrong.

| Orb | Title | What it shows |
|---|---|---|
| 1 | **Firmware** | The version, the commit, and the date it was built. |
| 2 | **WiFi** | The network name, the signal, and the channel. Or **No WiFi**. |
| 3 | **Address** | The orbs' address and their `.local` name, or **none, no WiFi**. |
| 4 | **Health** | How long the orbs have been running, why they last started, and free memory. |
| 5 | **Data** | How long ago each kind of page last got fresh data. |

**What the words mean**

| You see | It means |
|---|---|
| `commit 82de822` | Exactly which build of the software this is. Give it with the version when you ask for help. |
| `strong`, `good`, `fair`, `weak` | The WiFi signal at the orbs. The number beside it is in dBm: closer to zero is stronger. Strong is -55 or better, good to -65, fair to -75, weak below that. Weak signal is the usual cause of pages going stale. |
| `channel 6` | Which WiFi channel the orbs are on. Only useful to whoever looks after the network. |
| `up 3 d 04 h` | The orbs have run 3 days and 4 hours since they last started. A small number you did not expect means they restarted. |
| `last start: power on` | They started because they were plugged in. The normal one. |
| `last start: restart` | The software restarted itself: after **Save and restart**, after an update, or to get back on WiFi. Normal. |
| `last start: reset pin` | Someone pressed the small **R** button on the XIAO. |
| `last start: crash` or `watchdog` | The software stopped and restarted itself. Once is nothing. If it keeps happening, [send the log](TROUBLESHOOTING.md#getting-help). |
| `last start: brownout` | The power dipped. Try another USB cable, or a supply rated 2 A. |
| `last start: deep sleep` or `not known` | Rare. `not known` can follow a reset from the USB side. Once is nothing; if it keeps happening, [send the log](TROUBLESHOOTING.md#getting-help). |
| `memory 142 KB free` | Working memory the software has spare. It moves around. It only matters to whoever is helping you. |
| `weather 3 min ago` | The weather page last got fresh data 3 minutes ago. With more than one weather page, this is the oldest of them. |
| `stocks no data yet` | That page has not had data since the orbs started. |
| `MQTT not connected` | The orbs cannot reach the MQTT broker. With two MQTT pages and only one broker out of reach, it says which: `MQTT 1 not connected` or `MQTT 2 not connected`. |
| `MQTT no data yet`, `MQTT 4 min ago` | Connected; no value has arrived yet, or the oldest value arrived that long ago. |
| `no data pages` | Only the clock is ticked under **Show**. |

Times here are never in seconds: `<1 min`, `12 min`, `2 h 05 min`,
`3 d 04 h`.

## What the amber and red marks mean

The orbs keep showing the last data they got when they cannot get new data.
The marks tell you when what you are looking at is old.

The words and the shape of the ring say what is wrong. The color only says
which kind of page is marked: **amber** on the pages that show data, **red**
on the clock.

### On the weather, stocks and other data pages: amber

| You see | It means |
|---|---|
| **No WiFi** in black on an amber plate, near the top of orb 2, and no ring | WiFi is down. The data on screen is still recent. |
| **No WiFi** on orb 2, and a **broken** (dashed) amber ring around the orbs | WiFi is down, and the data inside the ring is old. |
| A **solid** amber ring, and no **No WiFi** | WiFi is fine, but new data is not arriving: the internet is down, or the weather or stock service is. |

- **No WiFi** appears once WiFi has been down for 10 seconds.
- The ring goes around the orbs holding old data. Weather and stocks count as
  old after 20 minutes without a refresh. An MQTT page counts as old once its
  broker has been out of reach for 3 minutes. A Web Data page counts as old
  at twice its own refresh time, and never sooner than 3 minutes: 3 minutes
  at the default of 60 seconds.
- The clock gets none of these marks. It keeps time without WiFi.
- When WiFi comes back, the marks go away, and the page on screen refreshes
  at once if its data was old. Other pages refresh when they next come up.

### On the clock: red, with "Clock only when WiFi is out"

Some people would rather see a clock they can trust than weather that is
getting old. The settings page has a switch for that:
**Rotation > Clock only when WiFi is out**. It is **Off** on new orbs, and
then everything above is all you will see.

With it **On**, once WiFi has been down for 10 seconds:

| You see | It means |
|---|---|
| All five orbs show the clock, each with a **broken** (dashed) red ring, and **No WiFi** in black on a red plate on orb 3 | WiFi is down. The time is right; the orbs have put the other pages away until WiFi is back. |

- The clock comes up even if it is not ticked under **Rotation > Show**.
- The page rotation waits.
- The seconds tick around the middle orb stops, and on the Nixie face the
  colon stays lit instead of blinking. The time itself is not affected.
- A button still moves to another page. That page carries the amber marks
  above. The next rotation turn goes back to the clock; with
  **Seconds per page** at 0, the page you picked stays.
- When WiFi comes back, the rings come off and the page that was up before
  the outage returns, refreshed at once if its data is old. If you moved to
  another page by button in the meantime, that page stays.

## When WiFi goes down

You do not need to do anything. The orbs get themselves back on.

**While they are running**:

| When | What the orbs do |
|---|---|
| WiFi drops | Keep showing the last data. The clock keeps running. |
| 10 seconds | Show **No WiFi** on orb 2. With **Clock only when WiFi is out** On, switch to the clock with its red rings instead. |
| every 30 seconds | Try to rejoin. |
| the router is off or out of range | Keep trying for as long as it takes, and never restart. Every 10 minutes they switch their radio off and on. |
| the router is there but will not let them in | Switch their radio off and on after 2½ minutes, and restart after 5 minutes. |
| WiFi returns | Rejoin, refresh the page on screen if its data is old (the others when they next come up), and clear the marks. If they had switched to the clock, go back to the page that was up. |

So after a power cut or a router restart, give the orbs a few minutes after
the router is back.

**At start-up**, the orbs need WiFi before they show any page. If they cannot
join, orbs 4 and 5 say why and how to fix it. They also keep trying by
themselves:

| Orbs 4 and 5 say | What the orbs do by themselves |
|---|---|
| **WiFi not found** | Listen for the network, first after 1 minute and then every 2 minutes, and restart to join as soon as they hear it. |
| **WiFi didn't connect** | Restart and try again after 5 minutes, then 10, then 20, then every 30 minutes. |
| **Wrong password** | If this password has worked before: the same retries as above. If it has never worked: wait for you to fix it. |

They hold off while a phone or computer is joined to their setup network, so
they never restart under you while you are typing a password.

## Changing the WiFi network

The orbs remember **one** WiFi network. To move them to another:

1. Unplug the orbs.
2. Hold the **right** button and plug them back in. The middle orb (orb 3)
   shows `Keep holding to forget WiFi` and counts down 3, 2, 1 over 5 seconds. Keep
   holding until it shows `WiFi forgotten`, then let go. If you let go during
   the count, it shows `WiFi kept` and nothing changes, so a thumb resting on
   the button while you plug in can't wipe the network by accident. Only the
   WiFi network and its password are forgotten: every setting on the settings
   page stays as it was. On a set mounted
   upside down, this is the button on the left as you look at it: the buttons
   do not move with the **Upside down** setting.
3. The orbs start their own setup network. Orb 5 shows its name:
   `Info-Orbs_` and two characters, such as `Info-Orbs_84`. Yours will have
   its own two characters.
4. On a phone or computer, join that network. A setup page opens by itself;
   if it does not, browse to `http://192.168.4.1`.
5. Choose **Configure WiFi**, pick the network, type its password, and press
   **Save**.

Your settings, keys and start-up picture are kept. Only the network changes.

The network must be **2.4 GHz**. The orbs cannot see a 5 GHz network.

## Running from a phone hotspot

For showing the orbs away from home. Joining the hotspot replaces your home
network, so try it at home before the day.

1. **Turn on the hotspot, on 2.4 GHz.**
   - iPhone: Settings > Personal Hotspot, turn on **Maximize Compatibility**.
     Without it, recent iPhones use 5 GHz and the orbs never find the
     hotspot. Stay on that screen until the orbs join: an iPhone stops
     offering its hotspot when nothing has connected for a while.
   - Android: in the hotspot settings, set the band to 2.4 GHz.
2. Follow [Changing the WiFi network](#changing-the-wifi-network), and do
   step 4 **from a second device**, a laptop or another phone. That keeps the
   hotspot phone a hotspot the whole time.
3. The orbs join and fetch their data over the phone's mobile data. The
   settings page is at `http://<address>/settings` from a device joined to
   the hotspot.
4. **Back home**, change the network back the same way.

Pages that read from something on your home network, such as an MQTT broker,
show no data while the orbs are on the hotspot. Untick them under
**Rotation > Show** for the day.

## Weather from your own station

If you own a WeatherFlow Tempest or an Ambient Weather station, the orbs can
show its weather: one weather page per station, up to two, of either kind.
If a Visual Crossing key is also set, your town keeps its own page as well.
Without that key only the station pages show.

Under **Weather stations**, pick each station's **type** first; the page then
asks for that type's ID.

**Tempest**

1. Get a token: tempestwx.com > Settings > Data Authorizations > Create
   Token. On the settings page, paste it under **API keys > Tempest token**.
2. Find the station's ID: it is the number in the station's tempestwx.com
   address. Enter it as the **Station ID** with a short label, 16 characters
   at most. The label shows in place of the town name.
3. Press **Save and restart**.

**Ambient Weather**

An Ambient station sends its readings but no forecast, so its page takes the
temperature and the readings (feels like, humidity, wind) from the station,
and the words, the picture, today's high and low, sunrise and sunset and the
next three days from **Visual Crossing**, at the station's own location. Set
the Visual Crossing key as well; without it the page shows the temperature
only, and the settings page says so.

1. Make two keys on **ambientweather.net** (the `.net` site, not `.com`),
   on your account page: an **API Key** and an **Application Key**. They are
   two different kinds; two API Keys do not work. Paste both under
   **API keys**.
2. Give the station a short label, 16 characters at most. Leave **Station
   MAC** blank if the account has one station: the page uses it, and after
   the first fetch the settings page shows its name and MAC. With several
   stations on the account, the settings page lists them; copy the MAC of
   the one you want.
3. Press **Save and restart**.

To remove an Ambient page, blank both the MAC and the label.

Afterwards each station shows ✔ or ✖ on the settings page for its last
fetch, how long ago that was, and the error if it failed.

## Dimming and switching off at night

Two separate schedules on the settings page, both off until you turn them on.

- **Dim times** darkens the colors between two times. The screens stay on.
- **Screens-off times** blanks the screens between two times. Any button
  wakes them for a minute.

Type times as 24-hour (`22:00`, `7:30`) or with am and pm (`10pm`,
`7:30am`). A schedule can run past midnight, such as from `22:00` until
`7:00`. Where the two overlap, off wins.

## The log

The orbs keep a record of what happens to them: each start and why, WiFi
going down and coming back, a page that stops refreshing and when it
recovers. Each line has the date and time; lines from before the orbs had
set their clock show dashes for the date.

It is at the top of the settings page: **Log: view · download**. **Download**
saves it as `info-orbs-log.txt`, which is the file to send when you ask for
help.

- It records events only. **It never holds your network name, passwords, API
  keys or station IDs.**
- It keeps roughly the last few hundred events, and it survives restarts and
  WiFi updates.

The file ends its header with a short key to the WiFi numbers it uses, so
whoever reads it does not need to look them up.

## The password

Orbs installed from the release file have no password: anyone on your WiFi
can open the settings page. The page says so near the top. On a home network
that is how most sets run.

A password is built into the software, so setting one means building the
software yourself: [SETUP step 8](SETUP.md#8-later-updates-no-cable-needed).
The browser then asks for the user name `admin` and your password on the
settings, update and log pages.

## Starting over

| You want to | Do this |
|---|---|
| put the settings back, keep the WiFi | **Reset to built-in values** on the settings page |
| change the WiFi, keep the settings | [Changing the WiFi network](#changing-the-wifi-network) |
| wipe everything | the [Quick start](../README.md#quick-start-no-software-to-install-on-your-pc) again. Its file erases the whole board, so WiFi, the weather key and your start-up picture go back in afterward, and the log starts empty. |
