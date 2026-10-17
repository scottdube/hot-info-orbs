# HOT Info Orbs — intro video outline

Target length **~3:00** (window 2:00–4:00). Six segments; times are budgets,
not cues. Script and shot list follow from this.

Shot on the **prototype**: the v1.1 carrier board (ESP32-S3 SuperMini, five
GC9A01 displays, three buttons). The XIAO board and the enclosure appear only
as renders, labeled as designed and not yet built.

---

## 1. Cold open — what it is (0:00–0:15)

- Five orbs on a desk, cycling Clock → Weather → Stocks. No talking for the
  first few seconds.
- One line: "Five round screens that show the time, the weather and the
  markets, set up from your phone."
- Title card: **HOT Info Orbs**, Hands-On Tech club logo.

## 2. Where it came from (0:15–0:45)

- Brett Turner's open-source **Info Orbs** (ESP32, five displays). Credit him
  by name.
- **HackerBox 0129 "Orbital"** (2026) shipped a kit version on the ESP32-S3.
- Club members built the HackerBox kit → the club looked at it and decided to
  redesign it as a club build: easy to get right, cheap to get into.
- Beat to land: "a derivative, not a copy" — what follows is what the club
  changed.

## 3. The hardware (0:45–1:25)

- Close-up tour of the prototype: SuperMini in sockets, five displays on one
  SPI bus, three buttons.
- **Yang's carrier board** (KiCad, libraries in the project so it opens with
  nothing to install).
- Design changes to call out (pick 3, keep each to one sentence):
  - Each display runs on 5 V through its own regulator, so the ESP32's small
    regulator only feeds the ESP32. Whole orb measured **1.71 W**.
  - Fixed in review before fab: fifth display's DC line connected, a button
    moved off a strapping pin (GPIO3 on the HackerBox), ground pour added.
  - Ordered from JLCPCB; 10 boards.

## 4. The software, in use (1:25–2:30) — the longest segment

Show each on the phone screen next to the orb reacting. In priority order;
cut from the bottom if long.

1. **Settings page** in a browser: rotation time, tickers, town, 12/24 h,
   colors, which pages show. Save → orb restarts with it. *(Was: edit a file,
   rebuild, reflash over USB.)*
2. **Firmware updates over Wi-Fi** at `/update`, with automatic rollback if
   the new image can't reach Wi-Fi. *(Was: USB cable every time.)*
3. **Tempest personal weather station** page, plus sunrise/sunset and the moon
   phase drawn on the orb.
4. **Night mode**: separate dim hours and screens-off hours; any button wakes.
5. **Your own boot picture**, uploaded from the phone.
6. Under the hood, one line each over B-roll: Wi-Fi roaming to the strongest
   access point, API keys kept out of the code, unit tests in CI.

## 5. What's next (2:30–2:50)

- **XIAO ESP32-S3 carrier board**: routed and DRC-clean, not yet
  fabricated. Buttons on the top edge. Show the KiCad 3D render.
- **Enclosure, "Winged Speedster"**: brass bezels, finned end caps,
  multi-part print. Show the concept render.
- Say plainly these are designs, not built.

## 6. Close (2:50–3:05)

- Where to get it: GitHub `scottdube/hot-info-orbs` and the feature sheet
  (scottdube.github.io/hot-info-orbs/features.html).
- Credits: Brett Turner (original), HackerBoxes (0129 Orbital), Yang (board),
  the club members who built the kit.
- End card: Hands-On Tech logo, orbs running.

---

## Before the shoot

- **Scott's orb is running the display-edge test firmware**, not the orb
  firmware. It needs a USB reflash with a SuperMini `config.h` (the local one
  is the XIAO pin map) before anything in segments 1, 3 or 4 can be filmed.
- Re-tick all pages on `/settings` (the orb was left Clock-only for testing).
- Set the boot picture to the HOT hand for the title beat.
- Phone screen recording for segment 4; the orb on camera at the same time.

## Open questions

- Which club members built the HackerBox kit, and should they be named?
- Audience: club members (assume they know the kit) or the public (explain
  HackerBox in a line)?
- Narrator on camera, or voice-over only?
- Show the Stocks page? It needs a ticker that's moving on shoot day.
