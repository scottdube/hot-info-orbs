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

**Antenna:** the XIAO has a U.FL pigtail and a stick-on FPC antenna (Seeed
A-02, inventory part 102), and **the antenna is not mounted on this board**
(Scott, 2026-10-16), so the board has no copper keepout for it. The
2026-10-14 `ANTENNA_PATCH` rule area is removed; `tools/antenna_patch.py` is
retired and kept as history.

**Physical layout (2026-10-16, `tools/physical_rework.py`, NOT yet routed):**
- Displays plug in: each GC9A01 keeps its male header and seats in a 1x07
  2.54 mm female socket (footprint `_Displays:GC9A01_SOCKET`, same pads as
  `GC9A01_PINS`, display lifted 8.5 mm in 3D). Pad row at y 106.01, as low as
  the 0.5 mm copper-to-edge rule allows.
- Board top edge raised 17.28 mm to y 62.0, even with the tops of the
  displays (measured off a render); top mounting holes and the GND pour
  moved up with it. Board is now 198.8 x 45.78 mm.
- C1, C2, R1 and SW1-3 are on the BACK, the XIAO's side (Scott: "all on
  the same side as the xiao").
- Buttons: right-angle 6 x 6 x 8 mm tact switches (uxcell, Amazon) on the TOP
  edge, actuators out past it, centres x 128.68 / 150.78 / 172.87 = equal
  spacing across the centre third. Footprint is KiCad's stock
  `SW_Tactile_SPST_Angled_PTS645Vx39-2LFS` - **unverified for the uxcell
  part**: the listing says 6.5 mm and 4.5 mm pin pitch, the footprint has
  4.5 mm (switch pins) and 7.0 mm (mounting tabs). Measure one.
- R1 between DS1 and DS2, C2 + C1 between DS3 and DS4, all on the bottom edge.
- Board texts moved to the top band (under the display overhang).

## Status — PLACED, NOT ROUTED (2026-10-16)

The 2026-10-14 status below describes the routed board before the physical
rework; tracks were stripped for the re-route. Current DRC: 46 unconnected
(no tracks), 0 parity, 19 silk warnings (14 = Seeed's U2 outline, 5 = the
socket tab boxes at the bottom edge).

### 2026-10-14 (superseded)

Schematic ERC 0; board autorouted, DRC 0 errors / 0 unconnected / 0 parity
(14 silk warnings = Seeed's module outline over its own pads, accepted).
Firmware env `xiao-esp32s3` compiles. Nothing ordered, nothing flashed.
3D: every footprint resolves its model (Seeed's XIAO ESP32S3 PLUS STEP on U2,
the GC9A01 STEP on DS1–DS5, both project-local under `lib/3d/`), and the board
assembly is exported to `3d-assembly/hot_info_orbs_xiao_assembly.step`
(2026-10-14). `docs/HANDOFF.md` has the step log and the traps;
`../../docs/VARIANT-XIAO-ESP32S3.md` has the reasoning.

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
| `lib/3d/_ESP32 modules/XIAO-ESP32S3-PLUS-v4.step` | U2's 3D model (Seeed assembly "XIAO ESP32S3 PLUS_V1.0_241025"), attached by `tools/xiao_model.py` with offset (-1.778, -7.62, 0) / rotate (-90, 0, 0), found by rendering. Also stands in for the plain S3 (same outline, same USB end). | Seeed product page download, 2026-10-14 |

The SuperMini symbol/footprint are still in `lib/` but unused.
