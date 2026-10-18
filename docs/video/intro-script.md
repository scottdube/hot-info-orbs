# HOT Info Orbs — intro video script

Follows [intro-outline.md](intro-outline.md). Every line works as voice-over;
any line can be said on camera instead. About **620 words** (it was about 500
when first written and grew with the custom pages, the 5 V point and the
credits). The practice read on 2026-10-18 ran **174 words a minute: 3:38 of
speech**, and **4:29** once each shot holds until its line ends. Line-by-line
times: [vo-practice-2026-10-18.md](vo-practice-2026-10-18.md).

**[brackets]** = picture. Plain text = narration.

---

## 1. Cold open (0:00–0:15)

**[Five orbs on a desk, cycling Clock → Weather → Stocks. Hold 5–6 s with no
narration, room tone only.]**

This is the HOT Info Orbs.

**[Title card: HOT Info Orbs, Hands-On Tech logo. The orbs keep running
behind it.]**

## 2. What it is (0:15–0:35)

**[Slow push-in on the orbs; cut to the board from above.]**

Five little round screens on one board, run by a Wi-Fi chip about the size of
a postage stamp. They show the time, the weather and the markets, and they
keep themselves up to date.

It's a Hands-On Tech club build, so anyone in the club can make one and set
it up from a phone or computer.

## 3. Where it came from (0:35–0:55)

**[Still or screenshot of Brett Turner's Info Orbs project page, credited on
screen.]**

The idea isn't ours. Brett Turner designed the original Info Orbs and shared
the plans and software for free.

**[HackerBoxes #0129 "Orbital" product image, credited on screen.]**

Later, HackerBoxes, a monthly electronics kit, sent out a version of it
called Orbital.

**[Donald Champion's photos of his built HackerBox kit, credited on
screen.]**

A couple of our members built that kit. We liked it enough to make our own:
a new board, new features, and a build anyone in the club can follow.

## 4. Tour of the pages (0:55–1:50)

**[Clock page, standard face, full frame.]**

The clock spreads across all five orbs.

**[Middle button press → nixie face.]**

Press the middle button for a nixie tube face. You can also build in a
custom face of your own. Twelve or twenty-four hour, in any colors you like.

**[Right button → Weather page.]**

The weather page covers any town: what it's doing now, and the next three
days.

**[Tempest page.]**

If you own a Tempest weather station, it gets a page of its own, with live
wind, humidity and feels-like temperature. You can add two.

**[Close-up on the weather icon orb.]**

Sunrise and sunset sit above the weather icon, and tonight's moon is drawn
below it.

**[Right button → Stocks page.]**

And there are five tickers, one per orb, with the price, the change and the
day's range. Stocks, funds or crypto.

**[Hand on the buttons, stepping left and right.]**

Left and right step through the pages. Leave it alone and it rotates on its
own.

**[Right button → MQTT page: five live Home Assistant values, one per orb.]**

And for the tinkerers, two pages you build yourself. This one shows live
values from Home Assistant, or anything else that speaks MQTT.

**[Right button → Web Data page: the sample, five colored orbs.]**

This one draws a small JSON file from any web address. You set both up from
your phone or computer, with no new firmware.

## 5. How it's built (1:50–2:30)

**[Close-up: the ESP32-S3 SuperMini in its socket.]**

Here's what's inside. The brain is an ESP32-S3, with Wi-Fi built in. It sits
in a socket, so it lifts out.

**[Pan across the five displays, then the three buttons.]**

Five round color displays, an inch and a quarter across, share one
connection to it. Three buttons do the rest.

**[Bare carrier board, then KiCad view of it.]**

Club member Yang and I designed the board that ties it together, and JLCPCB makes
it.

**[USB cable plugging in; the orbs light up on the boot screen.]**

It runs off a USB cable and draws under two watts.

**[Thermal image, color: the ESP32 module, 43.6 °C on its hottest point.]**

The displays run on five volts, each with its own regulator. On the original
kit they all ran off the ESP's regulator, and builders found it too hot to
touch.

**[Thermal image, color: the backs of three displays, 37.9 °C hottest.]**

The displays carry their own share now, and stay warm, not hot.

**[GitHub repo page.]**

The board files and the software are all open, on GitHub.

## 6. Setting it up and living with it (2:30–3:05)

**[Laptop, USB cable to the orb, the upload progress.]**

You load the software once, over USB. After that, there's no cable.

**[Split screen: phone on the settings page | the orb. Scroll past the
Tempest station fields.]**

Settings live on a web page you open from your phone or computer: your town, your
tickers, time zone, colors. Save, and the orb restarts with them.

**[Phone: untick Stocks, Save; the orb's rotation skips it.]**

And you pick the pages. Not interested in the markets? Untick Stocks, and it
drops out of the rotation. Tick just one page, and that page stays put.

**[Phone on the update page; the orb showing "Updating nn%".]**

Software updates come over Wi-Fi too. If one goes wrong, the orb falls back
to the version that worked.

**[Orbs dimming; then a hand pressing a button to wake dark screens.]**

It dims in the evening and can switch its screens off at bedtime. Any button
wakes it.

**[Boot screen showing the Hands-On Tech hand.]**

And it starts up with any picture you like.

## 7. What's next (3:05–3:20)

**[KiCad 3D render of the XIAO board.]**

Two things are coming. A new board for an even smaller chip, with the
buttons on top. It's being made now.

**[Winged Speedster enclosure concept render.]**

And a case: brass rings around each screen and finned end caps, all 3D
printed. That one is still a design.

## 8. Close (3:20–3:30)

**[On-screen: github.com/scottdube/hot-info-orbs and the feature-sheet URL.]**

Everything's on GitHub, at the link on screen.

**[Credits card: Brett Turner — Info Orbs · HackerBoxes — #0129 Orbital ·
the members who built the first kit ·
HackerBox build photos — Donald Champion.]**

Thanks to Brett Turner, to HackerBoxes, and to the members who built the first kit.

**[End card: Hands-On Tech logo over the orbs running. Hold 3 s.]**

---

## Notes for the shoot

- **Custom clock face** is off by default (`USE_CLOCK_CUSTOM false`) and
  needs images built into the firmware. The script only says you *can* build
  one in. If the reflashed orb has it enabled, it can be shown; otherwise
  don't.
- **Screens off is not black.** The backlight stays on, so the panels glow.
  Film the dim beat; if screens-off is shown, film it in a lit room and don't
  call it dark.
- **"Under two watts"** is the measured 1.71 W at 5 V (features.html).
- **"Too hot to touch"** is a HackerBox 0129 builder's report: 82 °C (180 °F)
  on the module running only a graphics demo (`docs/VARIANT-ESP32-S3-SUPERMINI.md`).
- **"About forty-three degrees"** is the thermal-camera readings of 41.8–43.6 °C
  at the module's hottest point, room 75 °F, after hours of running and again
  ~20 minutes after a power cycle
  (`docs/POWER-MEASURED.md`, 2026-10-17). Surface reading, not junction.
- **"Inch and a quarter"** = the 1.28" GC9A01 panel.
- Weather is three days ahead: `threeDayWeather(4)` in WeatherWidget.cpp.
- Button behavior checked against `main.cpp` (left/right = previous/next
  page) and `ClockWidget.cpp` (middle = face; hold = 12/24 hour, not
  scripted).
