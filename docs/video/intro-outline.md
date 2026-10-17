# HOT Info Orbs — intro video outline

Target length **~3:30** (window 2:00–4:00). Eight segments; times are budgets,
not cues. Script and shot list follow from this.

**What the video is:** an introduction to the whole project for someone who
has never heard of it, Brett Turner's Info Orbs or HackerBoxes. It explains
what the orbs are, what they show, how they're built and how you use them.
The two sources are credited as inspiration and explained in a sentence each.
It is not a list of differences from them.

**Audience:** club members, on the club site (lightly password-protected).
Assume no prior knowledge of the project. Because the protection is light,
treat the video as public: no station IDs, tokens or home-network detail on
screen.

**Delivery:** narration, voice-over or a mix, decided during the shoot. Write
every line so it works as voice-over. On-camera pieces are optional inserts.

**Hardware on camera:** the **prototype**, the v1.1 carrier board (ESP32-S3
SuperMini, five 1.28" round displays, three buttons). The next board and the
enclosure appear only as renders, labeled as designs.

---

## 1. Cold open (0:00–0:15)

- Five orbs on a desk cycling through their pages. No talking for the first
  few seconds.
- Title card: **HOT Info Orbs**, Hands-On Tech logo.

## 2. What it is (0:15–0:35)

- A desk display: five small round screens on one board, driven by a tiny
  Wi-Fi computer (an ESP32).
- It shows the time, the weather and the markets, and updates itself over
  Wi-Fi.
- It's a club build, designed so any member can put one together and set it
  up from a phone.

## 3. Where it came from (0:35–0:55)

Credit, briefly. Assume the viewer knows neither source.

- **Brett Turner's Info Orbs:** an open-source project (free plans and
  software) that first put five round screens on a desk. That's the starting
  point.
- **HackerBoxes:** a monthly electronics kit. Their kit #0129 "Orbital" was a
  version of the orbs on a newer chip.
- A couple of club members built that kit (unnamed). The club liked it and
  decided to make its own version: a new board, new features, and a build
  anyone in the club could follow.

## 4. Tour of the pages (0:55–1:50) — the longest segment

Each page uses all five orbs together. Show each one full-frame on the desk,
then the buttons stepping between them.

1. **Clock:** large digits across the orbs. Three faces: standard, **nixie
   tube**, and a custom face. 12 or 24 hour, your choice of colors.
2. **Weather:** current conditions and a forecast for any town.
3. **Your own weather station:** if you have a Tempest station, its live
   readings get their own page (wind, feels-like, humidity). Up to two
   stations.
4. **Sun and moon:** sunrise and sunset times above the weather icon, and
   tonight's moon phase drawn below it.
5. **Stocks:** five tickers, one per orb, with price, percent change and the
   day's high and low. Stocks, funds or crypto.
6. **Buttons:** left and right step through the pages. On the clock, the
   middle button changes the face; hold it to switch 12/24 hour. Pages also rotate on their own on a timer.
7. **For the tinkerers, two pages you build yourself**, each shown full-frame
   on the orbs: **MQTT** (live values from Home Assistant or any MQTT source,
   one per orb) and **Web Data** (a small JSON file from any web address).
   Both set up from the phone, no new firmware.

## 5. How it's built (1:50–2:30)

- Close-up tour of the prototype board.
- **The brain:** an ESP32-S3 module about the size of a postage stamp, with
  Wi-Fi built in.
- **Five 1.28" round color displays** sharing one connection to the module.
- **Three buttons** along the board.
- **The carrier board**, designed by club member Yang (KiCad), made by
  JLCPCB. The display modules and the ESP32 plug in, so it's a socket and
  solder-the-headers build.
- **Power:** a USB cable. The whole orb draws about 1.7 W (measured).
- **Why the displays run on 5 V:** in the original design all five displays
  ran from the ESP32 module's own small 3.3 V regulator. Builders of the
  HackerBox kit reported the module too hot to touch: one measured 82 °C
  (180 °F) running only a graphics demo, Wi-Fi off. Here each display takes
  5 V and regulates its own, so the module carries only itself. Measured on
  this board with a thermal camera: **42.7 °C** at the module's hottest point,
  room 75 °F; the display backs reach 36.3 °C. Show both thermal images,
  the module and a display back (`docs/assets/thermal/`). Source and numbers: `docs/POWER-MEASURED.md`,
  `docs/VARIANT-ESP32-S3-SUPERMINI.md`.
- Open source: the board files and firmware are on GitHub.

## 6. Setting it up and living with it (2:30–3:05)

Show the phone screen next to the orb reacting.

- **First setup:** load the firmware once over USB. After that, no cable.
- **Settings from a phone browser:** town, tickers, time zone, colors,
  rotation speed, upside-down mounting. Save, and the orb restarts with the
  new settings.
- **Pick your pages:** untick any page you don't want (Yang's example: he
  doesn't care about stocks). Tick one and it stays on screen. Show it
  unticking Stocks.
- **Updates over Wi-Fi:** new firmware uploads from the browser. If an
  update fails, the orb goes back to the version that worked.
- **Night mode:** dim hours and screens-off hours. Any button wakes it.
- **Your own boot picture:** upload one from your phone.
- One line over B-roll: it moves to the strongest Wi-Fi access point on its
  own, and your API keys stay out of the shared code.

## 7. What's next (3:05–3:20)

- **The next board** for the Seeed XIAO ESP32-S3: buttons on the top edge,
  everything plugs in. Expected to be in manufacturing by the shoot (Scott,
  2026-10-17). KiCad 3D render; if bare boards are in hand by then, film one
  instead. Confirm the status on the day and say only that.
- **An enclosure**, "Winged Speedster": brass bezels around each display,
  finned end caps, 3D-printed. Concept render.
- Say plainly where each stands: the board in manufacturing, the enclosure a
  design.

## 8. Close (3:20–3:30)

- Where to get it: GitHub `scottdube/hot-info-orbs` and the feature sheet
  (scottdube.github.io/hot-info-orbs/features.html).
- Credits on screen: Brett Turner (Info Orbs), HackerBoxes (#0129 Orbital),
  Yang (board design), the club members who built the first kit.
- End card: Hands-On Tech logo, orbs running.

---

## Before the shoot

- **Orb firmware reflashed over USB 2026-10-17** (from the display-edge test
  back to main). That happened off camera, so shot S22 has to be staged with
  the same command: `cd ~/code/hot-info-orbs-main && pio run -e
  esp32-s3-devkitc-1 -t upload`.
- Check every page is ticked on `/settings` (the orb cycles pages again since
  the 2026-10-17 reflash). **Web Data and MQTT are unticked on purpose** until
  the shoot, because they have no live data: tick them both before filming.
- **Web Data:** confirm `/settings` shows ✔ for the sample
  (`https://scottdube.github.io/hot-info-orbs/custom-pages/sample.json`).
- **MQTT:** fill all five rows (topic, label, unit) with generic labels, no
  room or place names. The page shows a dash (-) on every orb until a value arrives.
  Before filming, either publish a value from Home Assistant to the orb's
  topic, or set up a sensor that publishes on its own (an automation with
  `retain: true`, see `docs/CUSTOM-PAGES.md` on main). Check `/settings`
  reads ✔ Connected with a recent value. Keep the broker address and topic
  names off screen.
- Set the boot picture to the HOT hand for the title beat.
- Shoot the Stocks page on a weekday during market hours, or the stock
  tickers show the last close and don't move. BTC/USD moves around the clock.
- Phone screen recording for segment 6, with the orb on camera at the same
  time. Scroll past the Tempest station fields: the station IDs are private.
- Check the custom clock face has images loaded before filming segment 4.

## Decided (2026-10-17)

- **Something for every skill level.** Members range from first-time builders
  to experienced tinkerers, so the video carries both: the plain tour for
  newcomers, and the custom pages (MQTT, Web Data) shown on the orbs for the
  tinkerers, not just mentioned.
- Introduces the whole project to newcomers; Brett Turner and HackerBoxes are
  credited as inspiration and explained, not assumed known.
- Stocks is part of the page tour.
- HackerBox builders (Donald and others) are not named in the video.
- Audience: club members, on the lightly password-protected club site.
- Delivery: narration, voice-over or a mix, decided during the shoot.

- No parts cost in the video for now.

## Open questions

None open. (Custom pages: settled 2026-10-17, on main and set up from
`/settings`; each gets a full-frame shot in segment 4.)
