# Could this run on ESPHome instead?

Evaluated 2026-08-27. **Answer: not on this hardware — ESPHome buffers each
display in RAM and five will not fit.** The idea is sound and the reasons it
fails are specific, so they are recorded rather than the conclusion alone.

## Why it was worth asking

ESPHome would provide, as built-in components rather than as work:

| Requirement | ESPHome |
|---|---|
| R2 — OTA updates | `ota:` component, native |
| R1 — remote configuration | `web_server:` component |
| Secrets handling | `secrets.yaml`, the exact pattern hand-built in `secrets.h` |
| Config format | YAML instead of C++ |

And one thing not on the requirements list that may matter more than both:
**Home Assistant integration would remove the API keys entirely.** Weather could
come from an existing HA weather entity rather than Visual Crossing; there would
be no key to register, no free-tier quota, and no key to leak. For builders new
to embedded work, YAML with no compile step is a genuinely lower wall than a C++
toolchain.

## Why it does not work

ESPHome supports GC9A01A through the `ili9xxx` display component (added March
2024). That component **allocates a full framebuffer per display**. There is an
open feature request for partial buffering on low-RAM devices
([esphome/feature-requests#3131](https://github.com/esphome/feature-requests/issues/3131)),
which is itself confirmation that full-buffer is the current behaviour.

The numbers, using the DRAM budget measured from our own build rather than a
datasheet figure:

```
DRAM budget (from `pio run`)   327,680 B
current firmware uses           53,084 B   (16.2%)
free                           274,596 B
```

| Colour depth | Per display | Five displays | |
|---|---:|---:|---|
| 16-bit | 115,200 B | 576,000 B | **2.1× the free DRAM** |
| 8-bit | 57,600 B | 288,000 B | **still over** |

Even at 8-bit colour, five buffers exceed the entire free DRAM before ESPHome's
own runtime, the WiFi stack, or anything else is accounted for.

ESPHome's own documentation says as much in passing: *"16 bit colors requires
twice the amount of RAM as 8 bit, and may not be usable unless PSRAM is
available."*

## Why the current firmware does fit

TFT_eSPI does not keep a framebuffer. It streams pixels straight out over SPI
as it draws. That is precisely how five 240×240 panels fit on a chip with 320 KB
of DRAM, and it is the architectural difference that decides this question.

## What would change the answer

**PSRAM.** An ESP32-WROVER (4–8 MB PSRAM) or an S3 variant carrying PSRAM has
room for five buffers. Note the irony: choosing ESPHome would reopen the module
question that was just closed on complexity and cost grounds, and push toward a
*more* expensive board. That is the opposite of the direction this project is
going.

Partial-buffer support landing in ESPHome would also change it, but that is an
open feature request, not a plan.

## The part worth keeping

The most attractive thing about the ESPHome route — sourcing data from Home
Assistant instead of third-party APIs — **does not require ESPHome.**

Upstream already ships an MQTT widget, and there is a Home Assistant blueprint
for it at [dreed47/info-orbs-mqtt-ha](https://github.com/dreed47/info-orbs-mqtt-ha).
An orb can subscribe to HA-published topics and display whatever HA already
knows: weather, sensors, energy, presence. No API keys, no quotas, no rewrite,
and it works on the dev-kit hardware already chosen.

**Recommendation: keep the Arduino firmware, and use the existing MQTT widget
for anything Home Assistant can supply.** That captures the real benefit of the
ESPHome idea at a fraction of the cost, and leaves the graphics work — the nixie
clock, the weather icons, the TrueType fonts — untouched rather than
reimplemented against a more limited drawing API.
