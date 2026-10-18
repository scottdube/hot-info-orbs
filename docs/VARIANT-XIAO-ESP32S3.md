# Variant: Seeed XIAO ESP32-S3 / ESP32-S3 Plus

Status: **DESIGNED, NOT BUILT.** Started on branch `xiao-esp32s3-plus`
2026-10-14, **merged into `main` 2026-10-18.** Schematic and board are done and
DRC-clean, the firmware compiles, and no board has been ordered or flashed.

**The project uses this module only: a Seeed XIAO ESP32-S3 or XIAO ESP32-S3
Plus, the builder's choice** (Scott, 2026-10-18). The earlier ESP32-S3
SuperMini prototype is not part of it; its env, pin block and hardware folder
were removed, and survive at tags `v1.1-run1` (board) and
`v1.1-prototype-firmware` (last firmware for the prototype boards).

The merge brought across what `main` gained after the split: the `displaytest`
env and `firmware/src/displaytest/` (moved to the XIAO pins, not yet run on a
XIAO), `docs/ENCLOSURE.md` with its concept image, `docs/DISPLAY-EDGE-TEST.md`,
the custom web-data and MQTT pages, and the 2026-10-17 entry in `docs/TRAPS.md`.

Date: 2026-10-14

---

## The question

The features sheet proposed the XIAO ESP32-S3 Plus as the "Advanced" module,
reached through an adapter that plugs into the v1.1 board's SuperMini socket.
Scott asked instead for the carrier itself reworked to take the XIAO, with the
old board project kept. This records what that took.

## Decision: a second carrier, not an adapter

`hardware/hot-info-orbs-xiao/` is a copy of the v1.1 project with U2 swapped.
Everything else — the five display positions, three buttons, the 5 V display
supply, the ground pour, the outline — is the v1.1 board. Two projects rather
than one with a configurable U2, because KiCad has no clean way to carry two
footprints for one symbol and the two boards will be ordered separately
anyway.

## S3 and S3 Plus: interchangeable

Scott wants either module to drop in. Checked against the Seeed wiki
(`wiki.seeedstudio.com/xiao_esp32s3_getting_started/`, 2026-10-14):

| | XIAO ESP32-S3 | XIAO ESP32-S3 Plus |
|---|---|---|
| D0–D10 → GPIO | 1 2 3 4 5 6 43 44 7 8 9 | identical |
| User LED | GPIO21 | GPIO21 |
| Size | 21 × 17.8 mm | 21 × 17.8 mm |
| Antenna | U.FL, external | U.FL, external |
| PSRAM | 8 MB | 8 MB |
| Flash | 8 MB | **16 MB** |
| GPIO | 11 | 18 (D11–D19 on the back, not reachable from this board) |

So the same board and the same binary serve both, provided the binary is
built for 8 MB flash — which it is (`board_upload.flash_size = 8MB`, 8 MB
partition layout). A 16 MB build would gain the Plus's extra flash and stop
running on the plain S3; rejected until there is something to put in it.
Not verified on hardware: no XIAO of either kind has run this firmware.

## Mounting: edge pins only

Scott's choice (asked 2026-10-14). The XIAO has 14 castellated edge pins and,
on the Plus, nine more pads on its underside. Only the edge pins are used, so
the module goes on in either of two ways a member can do with an iron:
2.54 mm headers (socket or pin), or soldered flat on its castellations. Each
pin on the carrier is one oval through-hole pad spanning hole and
castellation edge, so the same copper serves both. Consequences:

- any XIAO ESP32-S3 (plain, Sense, Plus) fits — they share the edge pins;
- the Plus's back pads, and the extra GPIO on them, are given up;
- 11 GPIO on the edge against the SuperMini board's 12.

## The twelfth pin

The SuperMini board drives the displays' shared RST from a GPIO. On the XIAO
there is no pin left for it, so RST is an RC power-on reset (R1 10 kΩ to
3V3, C2 1 µF to GND) and the firmware builds with `TFT_RST -1`, which makes
TFT_eSPI issue the panel's software reset command (`TFT_SWRST`) in `init()`.
The GC9A01 accepts that; the hardware line only has to be high by the time
the MCU is out of reset, and a 10 ms RC against the ESP32-S3's boot time is.

The BUSY indicator, which on the SuperMini board is an external LED on
GPIO15, becomes the XIAO's own user LED on GPIO21. That LED is wired to 3V3,
so it is active-low; `BUSY_ON`/`BUSY_OFF` macros in `config.h` carry the
polarity and `Widget::setBusy` uses them, defaulting to HIGH/LOW for a board
with an external LED.

## Pin assignment and why

| Signal | XIAO | GPIO | Reason it went there |
|---|---|---|---|
| SW1 / SW2 / SW3 | D0 / D1 / D5 | 1 / 2 / 6 | buttons kept off the UART pins: a button held through a reset would short 3V3 into GPIO43 while the ROM drives it |
| DS1..DS3 CS | D2 / D3 / D4 | 3 / 4 / 5 | GPIO3 is a strapping pin, but a CS is an MCU-driven output that idles high — its default level |
| DS4 / DS5 CS | D6 / D7 | 43 / 44 | UART0 TX/RX; the ROM boot log toggles TX before init, harmless on a chip select |
| SCLK / DC / MOSI | D8 / D9 / D10 | 7 / 8 / 9 | |
| 5V / GND / 3V3 | | | displays run from 5 V through their own regulators, as on v1.1 |

Full map with notes: `hardware/hot-info-orbs-xiao/README.md`.

## Firmware

`pio run` (default envs `xiao-esp32s3`, board `seeed_xiao_esp32s3`, and
`ota` for espota). `config.h.template` carries only the XIAO pin block; the
prototype's block is at tag `v1.1-prototype-firmware`. The XIAO board definition turns on USB CDC on boot itself: the
XIAO's console *is* the native USB (no bridge chip), so the TRAPS.md warning
against those flags — written for the prototype's module — does not apply.

Flash: the generic XIAO board json declares 8 MB; the Plus has 16 MB. Until
2026-10-18 the build kept v1.1's `partitions.csv` (two 1.875 MB app slots,
laid out for 4 MB), where it filled 84.0 % of a slot (2026-10-14) and 85.9 %
(2026-10-18). `partitions.csv` is now laid out for 8 MB — two 3.75 MB app
slots, 384 KB filesystem — so both boards still run one OTA layout and the same
build fills 43.0 %. Using the Plus's other 8 MB means
`board_upload.flash_size = 16MB` and a 16 MB table, which would not boot on the
plain S3 — not done, not needed. A changed table is only written by a USB
flash; an OTA update keeps whatever table the module already has.

Trap: `config.h` reaches the compiler via `-include config_helper.h`, which
SCons does not track as a dependency. After editing `config.h` an incremental
build produced a binary byte-identical to the SuperMini one. Clean the env
first.

## Board work — what was tried and rejected

- Freerouting with Seeed's footprint as shipped (a PTH pad *and* a
  castellation SMD pad per pin) reported 14 "violations", put a stub on every
  pin and left one net unrouted on every run. Hand-routing that net moved the
  failure to the next net — rejected. Merging each pair into one oval THT pad
  (`tools/merge_xiao_pads.py`) gave 0 violations and a complete route, and is
  the footprint the two mounting methods want anyway.
- Exporting the DSN with the GND pour present makes Freerouting treat GND as
  a plane and route nothing for it; the new tracks then slice the pour into
  islands (3 starved thermals, 2 unconnected). The pour is exported out and
  poured back on top of routed GND tracks.
- Result: 0 DRC errors, 0 unconnected, 0 schematic-parity issues, 14 silk
  warnings that are Seeed's own module outline crossing its pads (accepted;
  fabs clip silk off copper). Pipeline and traps: `hardware/hot-info-orbs-xiao/docs/HANDOFF.md`.

## Antenna

**Superseded 2026-10-16:** the FPC antenna will not be mounted on the board
(Scott), so the patch below was removed by
`hardware/hot-info-orbs-xiao/tools/physical_rework.py`. Kept as history.

The SuperMini has a PCB antenna and the v1.1 board keeps copper away from
it. The XIAO has none: a U.FL connector and a stick-on 2.4 GHz FPC antenna
(Seeed A-02, inventory part 102) on a short pigtail. A flex antenna stuck
over copper detunes, and the back of this board is one GND pour, so the
board now has a copper-free patch for it: rule area `ANTENNA_PATCH` on both
copper layers, **40 × 7.5 mm** at x 143.5–183.5, y 99.5–107.0 — the bottom
strip right of the XIAO, a few cm from its U.FL — outlined and labelled
"STICK FPC ANTENNA HERE" on B.Silk. The SuperMini keepouts and "Antenna"
silk are removed (`hardware/hot-info-orbs-xiao/tools/antenna_patch.py`).

The height was set by the router, not the antenna. That strip is also the
SPI bus's second channel to DS4/DS5 and the switches: at 10.5 mm tall
Freerouting left 3 nets unrouted, at 9 mm one (CS2), at 7.5 mm none, and
narrowing the patch instead did not help. **40 × 7.5 mm is an assumption**:
Seeed publishes no A-02 dimensions (product page, distributors checked
2026-10-14). Measure an A-02 and its pigtail before ordering boards; the
script takes `PATCH=x0,x1,y0,y1` to move or resize it, then re-route.

## Not done

- No board ordered, nothing flashed; the RC reset timing and the active-low
  LED are reasoned, not measured.
- 3D: done 2026-10-14 — Seeed's XIAO ESP32S3 PLUS STEP on U2 (fit by
  rendering, `tools/xiao_model.py`), display models re-pointed, assembly
  exported to `hardware/hot-info-orbs-xiao/3d-assembly/`.
- Board is placed but NOT routed since the 2026-10-16 physical rework
  (sockets, board edge up to the display tops, switches + passives on the
  XIAO's side, switches on the top edge at the 1/3, 1/2, 2/3 lines, XIAO
  on female sockets, board ID + HOT logo on the back, corner holes 2 mm
  in plus two mid-board holes); right-angle switch
  footprint unverified against the uxcell part.
- Plain XIAO ESP32-S3 price not checked (Plus: $7.90 at Seeed, 2026-10-14).
