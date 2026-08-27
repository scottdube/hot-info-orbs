# Variant: ESP32-S3 SuperMini

Status: **evaluation only.** Nothing here is committed to. This records what the
switch would actually cost, so the decision is made against facts rather than
against how small the board looks.

Date: 2026-08-27

---

## The question

Replace the 30-pin ESP32 devkit with an ESP32-S3 SuperMini. Does it have enough
pins, and what does the change drag along with it?

**Short answer: pins are not the problem. The carrier board is.**

---

## 1. Pin budget — it fits, barely

Counted from `firmware/config/config.h.template`, not estimated:

| Function | Pins | Notes |
|---|---:|---|
| SPI MOSI + SCLK | 2 | `TFT_MISO` is `-1`; displays are write-only |
| Display DC + RST | 2 | shared across all five |
| Per-screen chip select | 5 | one per display, the unavoidable cost of 5 panels on one bus |
| Buttons | 3 | |
| **Hard requirement** | **12** | |

Two more are soft. `BUSY_PIN` is a real output (`Widget.cpp:7`) but only drives an
activity indicator and can be dropped. `TFT_CS` appears solely in a
`Serial.println` in `ScreenManager.cpp:28` — very likely reclaimable, but confirm
TFT_eSPI isn't consuming it internally before counting on that.

**Against the candidates:**

| Board | Usable GPIO | Verdict |
|---|---|---|
| ESP32-C3 SuperMini | ~11 | **Does not fit.** Short before the optional pins. |
| ESP32-S3 SuperMini | 32 broken out; conservative "safe" set is IO1, IO2, IO4–8, IO15–18, IO21 = **exactly 12** | Fits, with zero margin on the safe set |

The safe set landing on exactly 12 is worth sitting with. It works, but any
future addition — a rotary encoder, a light sensor, an I²C bus — means reasoning
about strapping and boot-state pins rather than picking a free one.

**Unverified:** the S3 SuperMini pin classification above is from vendor
documentation, not from the board on the shelf. The specific AITRIP unit's flash
size and whether it carries PSRAM are **not confirmed**. Both must be read off
the actual module before any footprint is drawn.

## 2. Retracting the flash-size argument

An earlier claim that the S3's 4MB flash relieves the current squeeze **is
wrong**, and the correction matters because it removes one of the two upsides.

`partitions.csv` already maps a 4MB device:

```
app0,     app,  ota_0,   0x10000, 0x200000     <- 2MB app partition
spiffs,   data, spiffs,  0x210000,0x1E0000
```

The build reports 77.2% of `0x200000`. That is a **partition layout** constraint,
not a chip constraint. The classic board already has 4MB. An S3 SuperMini with
4MB therefore gives nothing here — and if the app partition needs to grow, that
can be done on the existing hardware by editing this file, no board change.

What genuinely remains on the S3 side: native USB (less fiddly flashing, no
BOOT-button dance), more GPIO headroom in principle, and physical size.

## 3. The carrier board is the real work — and it is needed either way

Read from `PCBFiles+Schematics/Main PCB/info_orbs.kicad_pro` and `.kicad_pcb`:

- The upstream board is **2-layer** (F.Cu + B.Cu, no inner layers)
- Its track width defaults are `[0.0, 0.2, 0.6]` mm — **0.2 mm tracks**
- `min_track_width` and `min_clearance` are both `0.0`, i.e. no floor set

This shop mills **single-sided with no plated vias**, and the milling floor is
**0.4 mm** track and clearance. So:

> **Upstream's PCB cannot be milled here as-is, regardless of which MCU is
> chosen.** Its traces are half the minimum width and it relies on two layers.

That reframes the decision. "Keep the devkit" does not mean "use the existing
board on the mill" — it means order upstream's board from a fab house, or
redesign for milling anyway. The SuperMini's PCB cost is therefore **less
incremental than it first appears**.

### Routing risk, specific to this circuit

Five displays share MOSI, SCLK, DC and RST while each needs its own CS. On a
single copper layer that is a bus fanning out to five connectors with five
individual returns crossing it. Expect jumpers. Before committing to milling,
the honest test from the shop guide applies: if it will not route on one layer
with a ground pour and a couple of jumpers, **order it** rather than fighting
the layout.

Given the fan-out, ordering from JLCPCB deserves serious consideration here even
though the mill is sitting right there. Two layers makes this circuit easy and
milling makes it hard.

## 4. Shop constraints the carrier must respect

Values traced to source, not quoted from memory:

| Constraint | Value | Source |
|---|---|---|
| Annular ring minimum | **0.25 mm**, checked *after* drill snapping | `pcbmill/constants.py:45`, `toollib.py:156` |
| Owned drill bits | 0.032″ / 0.0465″ / 0.0625″ / 0.125″ (0.813 / 1.181 / 1.588 / 3.175 mm) | `pcbmill/toollib.py:24-31` |
| Drill snapping | to **nearest** owned bit, not up | `toollib.py:128-138` |

Consequences for this board specifically:

- **Socket rows need 1.78 mm pads, not KiCad's stock 1.70 mm.** The stock
  `PinSocket_1x15_P2.54mm_Vertical` clears the 0.25 mm gate by about 9 microns.
  That is not margin. If the SuperMini is socketed rather than soldered down,
  this applies to every pin in both rows.
- **Machined-pin (turned) sockets, or direct-solder the module.** Never stamped
  female headers — a defective one cost a full day in this shop and mimicked a
  dead flash chip perfectly.
- **Antenna keepout** at the SuperMini's antenna end. Non-negotiable, and easy
  to forget on a board this small where every mm is contested.
- **Bulk cap near the module supply pin** — 220 µF electrolytic plus 0.1 µF
  ceramic, ceramic closest. Five displays plus WiFi transmit bursts is exactly
  the load that browns out a module fed by a long trace.
- **Flash/PSRAM pin keepouts must be re-derived for the S3.** The familiar
  "GPIO6–11 are the flash bus" rule is an **ESP32-classic** fact and does not
  transfer. Whichever pins this variant reserves need NPTH mechanical holes —
  hole, no pad — so the pins pass through touching nothing.

## 5. Mounting — unanswered, and it blocks layout

**How does the board physically mount inside the orbs assembly?** Standoffs,
panel screws, or floating on the display connectors? This is not yet known, and
a board has reached Gerber export in this shop with no mounting holes because
nobody asked.

On the mill, mounting holes are free — same op, same G54. Retrofitting means
hand-drilling a soldered, masked board next to a pour, with no registration.
Decide before placing the first component. Default is M3 drilled 3.175 mm, NPTH,
with a ~6.5 mm keepout and centre ≥ 3.2 mm from the board edge.

## 6. Power — measure, do not model

Five GC9A01 modules run their backlights off the 5 V rail. On a SuperMini that
rail is USB VBUS passthrough, not a regulator output with headroom.

**This is the cheapest thing to check and the one most likely to kill the
variant.** Put a meter on the existing five-display build and measure actual
draw at full brightness before any layout work. A number here either clears the
approach or ends it in five minutes.

## 7. Firmware port

Lower risk than the hardware, but not free:

- New PlatformIO environment (S3 target, not `esp32doit-devkit-v1`)
- Pin map rewrite in `config.h.template` — and the pin numbers become part of
  the variant's identity, so the two builds can no longer share one config
- TFT_eSPI on S3 needs its own verification pass; the GC9A01 driver is the same
  but the SPI peripheral is not
- `platformio.ini` currently declares `platform = espressif32` unpinned, which
  is a separate reproducibility question that a second board makes worse

## 8. Cost that is not technical

Upstream's assembly video, wiring diagram and dev kit all assume the classic
board. Anyone following those with a SuperMini in hand gets stuck. If more than
one person is building these, a board change means the written setup path in
`docs/SETUP.md` forks — or stops matching reality for half the builders.

---

## What is on the shelf

- **AITRIP Type-C SuperMini ESP32-S3** — InvenTree part #60, 4 on hand: 2 at
  B3-R6C3 (SLN), 2 at LRD. Recorded as spare stock, bought for the voice
  assistant and wrong for it. Flash/PSRAM variant **not verified**.
- Classic devkits: #62 (ESP-WROOM-32, 2 free at B3-R6C3) and #58 (38-pin CP2102,
  2 free). The current build BO-0016 allocates one of #62.

## Recommended sequence, if this proceeds

Ordered so the cheapest disqualifier runs first:

1. **Measure the 5 V draw** of five displays at full brightness. Five minutes,
   and it can end the discussion.
2. **Verify the actual module** — read the silk, confirm the flash/PSRAM variant
   and the true usable-GPIO set. "SuperMini" is not a specification.
3. **Port the firmware on flying leads.** Prove the S3 drives five GC9A01s
   before any copper is drawn. No PCB risk taken until this works.
4. **Then decide milled vs ordered**, and only then lay out the carrier.

Steps 1–3 cost parts already on the shelf and no money. Step 4 is where the
commitment starts.

## Open questions

- How does the board mount in the assembly? (blocks layout — see §5)
- Milled single-sided, or ordered 2-layer? (§3 argues ordering is stronger than
  it first looks, for this circuit)
- Does the variant replace the classic build or sit alongside it? That decides
  whether `config.h.template` forks or grows conditionals.
- Is the SuperMini socketed or soldered down? Socketing is friendlier to
  reflashing and rework but drags in the 1.78 mm pad requirement and doubles the
  height.
