# Variant: ESP32-S3 SuperMini

Status: **CLOSED — declined 2026-08-27.** Staying on the ESP32 dev kit. See
[Decision](#decision-declined) at the end. The evaluation is kept because the
measurements in it apply to the dev-kit board too. This records what the
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

## 3. The carrier board — reassessed for 2-layer

**Updated 2026-08-27: two-layer boards are available.** This reverses most of
what this section originally argued. Recorded rather than deleted, because the
reversal is the useful part.

### What the original argument was

Read from `PCBFiles+Schematics/Main PCB/info_orbs.kicad_pro` and `.kicad_pcb`:

- Upstream's board is **2-layer**, track width defaults `[0.0, 0.2, 0.6]` mm
- In-house milling is **single-sided, no plated vias, 0.4 mm floor**

So upstream's board could not be milled here, and any board — SuperMini or
devkit — meant a single-sided redesign with jumpers. That made the SuperMini's
PCB cost look nearly free, since a redesign was happening anyway, and it made
ordering look attractive purely to escape the one-layer fan-out problem.

### What changes with 2 layers on the table

Every one of those premises drops:

| | Was (single-sided milled) | Now (2-layer ordered) |
|---|---|---|
| Five CS lines crossing a shared bus | jumper exercise, real routing risk | trivial, vias handle it |
| Min track / clearance | 0.4 mm floor | ~0.15 mm |
| Upstream's existing board | unusable | **usable as-is, or as a starting point** |
| Turnaround | hours | weeks |

**The important consequence: upstream's PCB becomes an asset instead of a dead
end.** For the classic devkit it can be ordered as drawn — zero PCB work. For
the SuperMini, the display and button sections of the existing KiCad project are
reusable; only the module footprint and its fan-out need redrawing.

### Which means the SuperMini's cost went back up

Not to "from scratch", but no longer to "free". Honestly stated:

- **Classic devkit, ordered:** no PCB work at all. Send upstream's Gerbers.
- **SuperMini, ordered:** new module footprint, re-layout of the module area and
  its fan-out, then a full verification pass. Days, not weeks — but not zero,
  and it is work that buys size and native USB rather than function.

The single-sided constraint was doing a lot of the arguing in favour of the
SuperMini. With it gone, the case rests on what it was always really about:
physical size and nicer flashing.

### The new cost is schedule

Ordered boards run weeks, not hours. That is the constraint to plan around now —
and it argues for settling the pin map on flying leads first, because a mistake
discovered after ordering costs another full turnaround.

## 4. Constraints on the carrier

**Applies only if this is milled in-house.** For an ordered 2-layer board the
drill-snapping and annular-ring gates below are irrelevant — the fab plates the
holes and stock KiCad footprints are fine. Kept because the milled path is still
open, and because the socket-pad trap is easy to carry into the wrong context.

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
**True on any board, milled or ordered:**

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

## 6. Power — the regulator is the real constraint

**Correction:** an earlier draft of this section said the displays run off 5 V.
They do not. The netlist is explicit — display connector pin 7 is
`Net-(U1-3.3v)` and pin 6 is GND. All five displays hang off the **module's
onboard 3.3 V regulator**, which makes the regulator comparison decisive rather
than incidental.

| | DevKit V1 (DOIT) | S3 SuperMini |
|---|---|---|
| Regulator | AMS1117-3.3, SOT-223 | ME6211-series, SOT-23-5 |
| Datasheet max | 1 A | 500 mA |
| Practical sustained | ~500-600 mA before thermal droop | 500 mA, and needs >=4.3 V in |

**The SuperMini has about half the headroom in a much smaller package.**

Estimated load, not measured: a GC9A01 1.28in module draws roughly 40-80 mA at
3.3 V with the backlight up. Five is 200-400 mA. The ESP32 itself spikes to
350-500 mA on WiFi transmit. Peak lands near **550-900 mA** — marginal on the
AMS1117, over the limit on a 500 mA part.

**Two things to verify physically, both quick:**

1. **Read the regulator marking** on one of the AITRIP boards. The ME6211 is
   documented on the C3 SuperMini and the S3 shares the SOT-23-5 footprint, but
   this specific board is unconfirmed.
2. **Measure one GC9A01 at full brightness** (part #70 is on the shelf) and
   multiply by five. That replaces the whole estimated range with a number.

### The design answer that removes the question

Give the display rail **its own 3.3 V regulator** on the carrier board, fed from
the 5 V input, rather than hanging five displays off the module's LDO. A 1 A LDO
is about a dollar, and the board then does not care which module is fitted.

Since a new board is being drawn anyway, this is likely correct regardless of
how the SuperMini decision goes — and it retires the single biggest electrical
risk in the variant.

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
4. **Lay out the carrier** as a modification of upstream's KiCad project, not
   from scratch — the display and button sections carry over.

Ordering runs weeks per turn, so steps 1-3 are not optional preliminaries. A pin
map error found after the boards arrive costs another full turnaround.

Steps 1–3 cost parts already on the shelf and no money. Step 4 is where the
commitment starts.

## Open questions

- How does the board mount in the assembly? (blocks layout — see §5)
- ~~Milled single-sided, or ordered 2-layer?~~ **Resolved 2026-08-27: 2 layers
  available.** See §3 for what that reversed.
- Does the variant replace the classic build or sit alongside it? That decides
  whether `config.h.template` forks or grows conditionals.
- Is the SuperMini socketed or soldered down? Socketing is friendlier to
  reflashing and rework but drags in the 1.78 mm pad requirement and doubles the
  height.

---

# Appendix: Is upstream's board poorly laid out?

Measured 2026-08-27 from `PCBFiles+Schematics/Main PCB/info_orbs.kicad_pcb`.
Every number here came from parsing that file, not from inspection by eye.

## What is on the board

| | |
|---|---|
| Footprints | 16 (3 are graphics with no pads) |
| Pads | 135, **all through-hole**, no SMD |
| Nets | 36 |
| Track segments | 277 — 155 on F.Cu, 122 on B.Cu |
| Copper length | 204 cm front, 38 cm back |
| Vias | 56, every one 0.6 mm pad / 0.3 mm drill |
| **Copper pours** | **zero** |

## Three findings that explain the 56 vias

**1. There is no ground plane.** Not on either layer. Every ground and power
connection is an individually routed trace. Ground touches 14 pads across the
board and is the highest-fanout net on it; a bottom-layer pour would absorb
nearly all of those with no vias at all. The 14-via net `Net-(J1-Pin_1)` is
ground, stitched across the board one hop at a time.

**2. The board carries TWO ESP32 module footprints, both fully wired.** `V1`
(`New folder:38 Pin Board`) and `V2` (`A Local Libs:38 Pin Board thin`) — 38
pads and 36 nets each, connected in parallel so either module variant can be
fitted. That is a deliberate choice, not an error, but it roughly doubles the
routing burden. Every shared bus net reaches seven pads instead of five: five
displays plus both module positions.

**3. The back layer is used for stubs, not for a strategy.** 204 cm of copper on
the front against 38 cm on the back, yet 56 vias to service that back layer.
122 back-layer segments carrying 38 cm total means the pattern is hop over an
obstacle and hop straight back.

Supporting evidence of hurry rather than intent: two *different* 38-pin board
footprints, a library named `New folder`, a footprint named `Untitled`, and no
power symbols in the schematic — the nets are auto-named `Net-(U1-3.3v)` and
`Net-(J1-Pin_1)` rather than `+3V3` and `GND`, which is *why* nothing could be
poured. You cannot pour to a net the schematic never named.

None of this makes the board wrong. It works, and hundreds of people have built
it. It was drawn for a fab house where vias are free and there is no reason to
count them.

## How many connections actually need the other side?

Tested rather than estimated: the netlist was built into a graph — one node per
pad, net connections as edges, each component's pin rows locked into their
physical order, routing under through-hole parts allowed — and checked for
planarity. A planar graph can be drawn with no edge crossings, which is the
graph-theory statement of "routable on one layer".

| Scenario | Net edges | Planar? | Crossings required |
|---|---:|---|---:|
| One module, nothing poured | 59 | **yes** | **0** |
| One module, GND poured | 49 | **yes** | **0** |
| One module, GND + 3V3 poured | 37 | **yes** | **0** |

**The circuit is inherently single-layer routable.** Upstream's 56 vias are a
property of their layout, not of this design. Drop the duplicate module
footprint, pour ground, and place the five display connectors in a row so the
shared bus runs along it, and the topology has no forced crossings at all.

### What this result does not prove

Planarity is about topology; it ignores geometry. Specifically:

- It assumes traces can be made as thin as needed. At the 0.4 mm milling floor
  they cannot.
- **It ignores that you cannot thread a trace between 0.1″ header pads.** The
  gap between 1.7 mm pads at 2.54 mm pitch is about 0.84 mm — too tight to
  isolate reliably on the mill. With 5×7-pin connectors, routing *around* rather
  than *between* is where real crossings will appear.
- Display placement is not actually free; the enclosure dictates where five orbs
  sit.

So the honest reading: **zero crossings are forced by the circuit, and whatever
crossings appear will be driven by clearance geometry, not topology.** That is a
much better starting position than 56 suggests, and it means a small number of
jumpers, not a stitched board.

## Consequence: a milled prototype looks feasible after all

This retracts the earlier claim in §3 that a single-layer version would be a
jumper exercise. It would not. A single-sided milled board with a ground pour
and a handful of jumpers is a realistic prototype path, which matters because
production is going to JLCPCB regardless — the mill only has to get a testable
board onto the bench, not produce the final article.

The remaining unknown is geometric, not topological, and the way to settle it is
to attempt the layout rather than to reason further about it.

---

# Appendix B: Attempting the layout — where geometry actually bites

The planarity result in Appendix A says zero crossings are forced by the
circuit. Planarity ignores geometry. This is what happens when the geometry is
put back in.

## The pinout is the whole problem

All five display connectors are identical 1x7 headers:

| Pin | Net | |
|---|---|---|
| 1 | IO18 — RST | shared by all five |
| **2** | **IO13 / IO33 / IO32 / IO25 / IO21** | **unique per display** |
| 3 | IO19 — DC | shared |
| 4 | IO17 — MOSI | shared |
| 5 | IO23 — SCLK | shared |
| 6 | GND | shared |
| 7 | 3V3 | shared |

**The one signal that differs per display sits at pin 2 — fenced in between two
shared bus lanes.** Every other pin can be served by a straight lane running the
length of the row. Pin 2 cannot, because each connector's pin 2 goes somewhere
different.

## The hard constraint, computed

Header pads are 1.70 mm on 2.54 mm pitch:

```
pad-to-pad gap    0.84 mm
one track needs   1.20 mm   (0.40 trace + 0.40 clearance each side)
tracks that fit   0
```

**Nothing routes between adjacent header pads at the milling floor.** Not one
track. Every connection either arrives from outside the pin column or does not
arrive.

## The layout that follows

Place the five connectors in a row, pin columns parallel, pins perpendicular to
the row. Then:

- **Pins 1, 3, 4, 5, 7 become straight horizontal lanes**, each at its own pin
  height, hopping connector to connector. No crossings, no detours.
- **Pin 6 (GND) disappears into the pour** and costs nothing.
- **Pin 2 is the exception.** Each CS line must reach a pad whose neighbours on
  both sides are occupied bus lanes.

A CS line heading for a distant connector must route around the intervening pin
columns — there is no path between the pads. Sending it above the row (past pin
1) into a clear channel works, and 4 such tracks need only 3.6 mm of channel,
which is nothing on a board this size. But coming back *down* to pin 2 means
crossing the pin-1 RST lane.

**That crossing is the irreducible cost, and it is one per CS line that has to
get past another connector.**

## The number

| Module placement | Jumpers |
|---|---:|
| Module at one end of the row | **4** |
| Module in the middle of the row | **3** |

With the module at the left, CS for the nearest display runs alongside the RST
lane without crossing it; the other four each cross once. Moving the module into
the middle of the row splits the fan-out in both directions and saves one.

Routing the RST lane above the CS channel instead does not help — then RST has
to come down to each pin 1 and crosses the CS tracks instead. The count is
symmetric because the topology is.

## Verdict

**Three or four jumper wires on a single-sided milled board.** Not 56, not
zero. That is an entirely reasonable prototype — a handful of short wire links
soldered on the copper face.

The path is open: mill a single-sided prototype in-house with a GND pour and
3–4 jumpers, prove the firmware and the pin map on real hardware, then send the
proper 2-layer design to JLCPCB for production where the jumpers become vias and
cost nothing.

## One trap to fix before milling

The stock 1x7 header footprint is **1.70 mm pads on a 1.00 mm drill**. Milled:

```
drill 1.00 snaps to nearest owned bit 1.181 (UP)
annulus = (1.70 - 1.181)/2 = 0.2595 mm
gate = 0.25 mm  ->  passes by 9.5 microns
```

It passes, with no margin worth the name — this is exactly the socket-row case
the shop guide flags. **Grow the header pads to 1.78 mm** before generating CAM,
which takes the annulus to 0.299 mm and a 49 µm margin. Same change applies to
all five display connectors and the module row if it is socketed.

Note this is a *milling* concern only. Sent to JLCPCB, the holes are plated and
the stock footprint is fine.

---

# Appendix C: HackerBox 0129 "Orbital" — someone already built this

Found 2026-08-27. HackerBoxes shipped a monthly kit that is this exact variant:
ESP32-S3 SuperMini, five GC9A01 modules, three tactile buttons, custom PCB.
Their own description: *"originally inspired by the very cool Info Orbs from
Brett Turner... employs a different microcontroller (ESP32-S3 vs ESP32) and thus
has a different PCB layout and different IO pin assignments."*

Sources: [product page](https://hackerboxes.com/collections/past-hackerboxes/products/hackerbox-0129-orbital),
[build guide](https://www.instructables.com/HackerBox-0129-Orbital/) (published
2026-07-29, 11 people report building it).

## What this hands us for free

**A working pin map for the S3, published:**

```c
#define TFT_MOSI 11
#define TFT_MISO -1
#define TFT_SCLK 12
#define TFT_CS   10
#define TFT_DC    9
#define TFT_RST  13
#define USE_HSPI_PORT

#define SCREEN_1_CS 4
#define SCREEN_2_CS 5
#define SCREEN_3_CS 6
#define SCREEN_4_CS 7
#define SCREEN_5_CS 8

#define BUTTON_OK    1
#define BUTTON_LEFT  2
#define BUTTON_RIGHT 3
```

**And the PlatformIO target:**

```ini
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
board_upload.flash_size = 4MB
```

Three things this settles that were open in §1 and §7:

1. **The firmware port is a config edit, not a project.** `config.h` pin
   defines plus a new env block. §7 estimated more work than that.
2. **13 GPIO, not 12.** They kept `TFT_CS` (pin 10) rather than reclaiming it,
   so the real budget is 13.
3. **My "exactly 12 safe pins" caution in §1 was over-conservative.** They use
   IO1 through IO13 contiguously, including IO3 and IO9-13 which the
   conservative classification lists as restricted. Those restrictions apply to
   variants with octal PSRAM; on this board they are usable in practice, and 11
   builders confirm it.

**Flash is confirmed 4MB** (`board_upload.flash_size = 4MB`), which supports §2:
no headroom gain over the classic board.

## What it does not fix

- **No regulator for the display rail.** The kit contents are PCB, five female
  1x7 headers, three buttons, one right-angle male header. No LDO, no bulk
  capacitor. Five displays hang directly off the SuperMini's onboard regulator,
  exactly the arrangement §6 flags as the electrical risk. It evidently works
  well enough to ship — but "works" here is untested against brownout under
  simultaneous WiFi transmit and full backlight.
- **It runs stock Info Orbs firmware**, so every configuration and secrets
  problem this repo just fixed is still present in theirs.
- **The PCB is not published as source.** The Instructable attaches an `.ino`
  demo sketch and a 3-D printed stand `.stl`. No Gerbers, no KiCad project. It
  cannot be forked, only bought — and it is a past box, sold while supplies
  last.

## One thing worth checking on their pin map

**GPIO3 is a strapping pin on the ESP32-S3** (JTAG signal source select), and
they assign it to `BUTTON_RIGHT`. Info Orbs wires buttons to VCC with
`INPUT_PULLDOWN`, so a button held down during reset drives a strapping pin
high at boot. Likely harmless in the default eFuse configuration, but it is the
kind of thing that produces an intermittent "sometimes won't boot" that nobody
connects to a button. **If a pin map is being drawn fresh anyway, move that
button off IO3.**

## Assessment: better as a starting point, not as a project

**Take from it:** the pin map, the PlatformIO target, and the empirical proof
that one SuperMini drives five GC9A01s. That is genuine de-risking and it costs
nothing.

**Do not take:** the power architecture. The dedicated display-rail LDO in §6
remains the right call, and their omission of it is not evidence against —
they were building to a kit price with no PCB revisions planned.

From a planning standpoint it is *worse*, not better: a discontinued monthly box
with an unpublished board, no maintenance path, and no way to fork the hardware.
Its value here is as a reference design that proves the electrical concept, not
as something to depend on.

## Appendix C.1: Field reports confirm the thermal problem

Builders in the Instructable comments report the SuperMini running hot. One
quantified it:

> "my ESP32 gets 'very' hot. Painful to touch. Close up thermal image shows
> 180 F." — wheagy1, on the HackerBox 0129 Instructable

**180 °F is 82 °C, not 180 °C.** Still a problem, and the qualifier in the same
comment is what makes it serious: *"I've only loaded the graphics test so far."*
**That temperature is with the radio off.**

Working back from a 57 °C rise over ambient, through a SOT-23-5 LDO dropping
5 V to 3.3 V:

| θ_JA assumption | Dissipation | Implied current |
|---|---|---|
| 250 °C/W (still air) | 229 mW | 135 mA |
| 150 °C/W (on copper) | 381 mW | 224 mA |

Now add the radio, which Info Orbs uses constantly:

| WiFi TX burst | Total current | Junction temp |
|---|---|---|
| +150 mA | 374 mA | ~120 °C |
| +250 mA | 474 mA | ~146 °C |
| +350 mA | 574 mA | **~171 °C — past thermal shutdown** |

LDO thermal shutdown is typically 150–165 °C, and the ME6211-class part is
rated 500 mA absolute maximum. **The margin is already spent on the graphics
demo.**

This is field confirmation of §6 rather than new analysis, and it settles the
open question there: the dedicated display-rail LDO is not a nicety, and
HackerBoxes' omission of one is a defect in their board, not a demonstration
that it is unnecessary.

### Two corrections from the same comment thread

**GPIO count returns to 12.** A builder traced the PCB with a multimeter and
lists **GP10 as "Unused?"** — so `TFT_CS 10` appears in HackerBoxes' config but
is not physically connected. Appendix C revised the budget up to 13 on the
strength of their config file; that was wrong. `TFT_CS` is vestigial, as §1
originally suspected, and the requirement is 12.

**A maintained fork exists.** [JoeAWagner/orbital-orbs](https://github.com/JoeAWagner/orbital-orbs)
— Info Orbs for the ESP32-S3 with a web control panel and OTA updates. That is
the runtime-configuration approach considered and deferred during the secrets
work, already implemented by someone else. Worth reading before building it.

**Sourcing caveat:** reply threads on that Instructable are login-gated. Only
top-level comments were read directly; reports of additional heat complaints are
secondhand.


---

# Decision: declined

**2026-08-27 — staying with the ESP32 dev kit. The SuperMini variant is closed.**

The evaluation answered its own question. Pins were never the constraint, and
the two things that would have justified the switch did not survive contact with
the measurements:

- **Flash headroom was a mirage.** `partitions.csv` already maps 4 MB with a
  2 MB app partition, and the SuperMini is also 4 MB (§2).
- **Power got worse, not better.** The dev kit's AMS1117-3.3 is 1 A in a SOT-223
  with a real thermal tab; the SuperMini carries a 500 mA part in SOT-23-5.
  Field reports put the SuperMini at 82 °C *with the radio off* (§C.1).

What remained was physical size and native USB — real, but not worth what they
cost here.

**The deciding factor was complexity and cost, not the electronics.** This is
built as a learning project by people at a range of skill levels. The dev-kit
path needs no new carrier board, no added regulator, no new footprint, and no
divergence from the upstream assembly documentation that builders will follow.
The SuperMini path needs all four before anyone sees a working orb.

On the dev kit the power problem largely goes away: roughly twice the current
headroom in a package that can shed the heat. The dedicated display-rail
regulator discussed in §6 becomes an option rather than a requirement — still
worth doing on any new board, no longer load-bearing.

**Consequences:**

- InvenTree BO-0016 already allocates a classic dev kit (#62), so the parts
  claim is unchanged and needs no correction.
- The four AITRIP SuperMini boards stay as general spare stock.
- The two-layer JLCPCB question stands on its own merits — upstream's board can
  be ordered as drawn (§3), which is now the zero-work option.
- Everything measured here about the displays, the pinout, the planarity result
  and the 3–4 jumper count applies unchanged to a dev-kit board.

**What would reopen this:** a display rail with its own regulator making the
module's LDO irrelevant, plus a reason to care about size. Neither is true today.
