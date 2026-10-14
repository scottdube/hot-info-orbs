# HoT Info Orbs — XIAO ESP32-S3 variant (KiCad project)

Carrier for five GC9A01 displays and three buttons, taking a **Seeed Studio
XIAO ESP32-S3 or XIAO ESP32-S3 Plus** (SKU 102010671) in place of the ESP32-S3
SuperMini. Copied on 2026-10-14 from the v1.1 SuperMini board (`main` branch,
`hardware/hot-info-orbs/`, tag `v1.1-run1`, the boards ordered 2026-08-29);
this branch is XIAO-only and no longer carries that project.

**S3 and S3 Plus are interchangeable here** (Seeed wiki, checked 2026-10-14):
identical D0–D10 → GPIO map, user LED GPIO21 on both, 21 × 17.8 mm both,
U.FL antenna both, 8 MB PSRAM both. Flash 8 MB vs 16 MB; the firmware builds
for 8 MB and uses 4 MB, so one binary runs on either. The Plus's extra
D11–D19 are on its back and are not used by this board.

**Antenna:** the XIAO has no PCB antenna — it ships with a U.FL pigtail and
a stick-on 2.4 GHz FPC antenna (Seeed A-02, inventory part 102). An FPC
antenna detunes over copper, and the whole back of this board is a GND pour,
so the board carries a copper-free patch for it: rule area `ANTENNA_PATCH`
(no pour, tracks or vias on either layer) at x 143.5–183.5, y 99.5–107.0,
**40 × 7.5 mm**, on the back right of the XIAO, outlined and labelled on
B.Silk. The SuperMini's "Antenna" keepouts and silk are gone.
`tools/antenna_patch.py` makes it; its docstring records why 7.5 mm (taller
patches leave nets unrouted). **The size is an assumption** — Seeed publishes
no A-02 dimensions. Measure one before ordering boards and rerun with
`PATCH=x0,x1,y0,y1` if it does not fit.

## Status — DESIGNED, not fabricated (2026-10-14)

Schematic ERC 0; board autorouted, DRC 0 errors / 0 unconnected / 0 parity
(14 silk warnings = Seeed's module outline over its own pads, accepted).
Firmware env `xiao-esp32s3` compiles. Nothing ordered, nothing flashed; the
3D STEP has not been regenerated. `docs/HANDOFF.md` has the step log and the
traps; `../../docs/VARIANT-XIAO-ESP32S3.md` has the reasoning.

## Regenerating the board

All with KiCad CLOSED. `tools/swap_to_xiao_sch.py` and `tools/merge_xiao_pads.py`
were run once and their results are committed; do not rerun them. The board
itself is reproducible from the v1.1 copy of the .kicad_pcb (commit e6ea813):
`tools/swap_to_xiao_pcb.py` (KiCad's Python) then `tools/route_xiao.py`
(Freerouting headless, ~5 s, ends with a DRC + parity count). Each script's
docstring says why it does what it does.

## Module fit

Only the 14 castellated edge pins are used, so the footprint
(`_ESP32 modules:XIAO-ESP32-S3-DIP`, Seeed's own, CC BY-SA 4.0, with each pin's
PTH + castellation pad pair merged into one oval through-hole pad — see
`tools/merge_xiao_pads.py`) takes the module on 2.54 mm headers **or** soldered
flat on its castellations. Any XIAO ESP32-S3
(plain, Sense, Plus) has the same 14 edge pins and fits; the Plus's nine extra
pads under the module are not used.

## Pin map (decided 2026-10-14; same for S3 and S3 Plus)

11 GPIOs are available on the edge; the SuperMini board used 12, so the
display RST line moves off the MCU onto an RC power-on reset (R1 10 kΩ to
3V3, C2 1 µF to GND) and the firmware uses TFT_eSPI's software reset
(`TFT_RST -1`). The BUSY indicator goes to the XIAO's on-board user LED
(GPIO21, active LOW), which is not an edge pin.

| XIAO pin | GPIO | Signal | Note |
|---|---|---|---|
| D0 | 1 | SW1 (left) | same GPIO as the SuperMini board |
| D1 | 2 | SW2 (OK) | same GPIO as the SuperMini board |
| D2 | 3 | DS1 CS | strapping pin (JTAG select); MCU-driven output, no external pull, so harmless |
| D3 | 4 | DS2 CS | |
| D4 | 5 | DS3 CS | |
| D5 | 6 | SW3 (right) | |
| D6 | 43 | DS4 CS | UART0 TX: ROM boot log toggles it before init, harmless on a CS |
| D7 | 44 | DS5 CS | UART0 RX: pulled up at boot, so CS idles high |
| D8 | 7 | SCLK | |
| D9 | 8 | DC | |
| D10 | 9 | MOSI | |
| 5V | — | +5V | displays run from 5 V through their own regulators |
| GND | — | GND | |
| 3V3 | — | 3V3 | button pull side + RST pull-up |

Buttons stay on D0/D1/D5 rather than the UART pins: a button held through a
reset would short 3V3 into GPIO43 while the ROM is driving it.

## Libraries

Project-local, as in the parent project. Added here:

| Nickname | Provides | Source |
|---|---|---|
| `_Seeed XIAO` | `XIAO-ESP32-S3-edge` symbol (Seeed's 25-pin symbol trimmed to the 14 edge pins, so PCB parity has no pad-less pins) | Seeed `OPL_Kicad_Library`, CC BY-SA 4.0 |
| `_ESP32 modules` | `XIAO-ESP32-S3-DIP` footprint, 14 oval THT pads (hole at the header position, copper out to the castellation edge) | same, modified |

The SuperMini symbol/footprint are still in `lib/` but unused.
