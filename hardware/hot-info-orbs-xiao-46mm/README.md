# HoT Info Orbs — XIAO carrier at 46 mm display pitch (KiCad project)

**Copy of `hardware/hot-info-orbs-xiao/` made 2026-10-19 (from branch
`enclosure-front-mockup`, 318e8b0) with one physical change: the five
displays sit 46.0 mm centre to centre instead of ~38.98 mm**, to match the
enclosure front in Fusion (`main_body_front`: bezel centres at
32.75 + n × 46.0; see `docs/ENCLOSURE.md`). The enclosure grows out of this
board; this board is the one going to the fab. The original project is
untouched.

What `tools/pitch_46.py` changed (run once, then `tools/route_xiao.py`):

- DS1–DS5 at x = 57.04 / 103.04 / 149.04 / 195.04 / 241.04 (DS3 unchanged
  and now the board centre). y unchanged.
- Board 224.0 × 47.78 mm (was 198.82 wide): edges x 37.04 … 261.04, 20 mm
  from the outer display centres. y 62.0 … 109.78 as before.
- Seven Ø3.2 holes re-derived by the same rules: corners 5 mm in, mid holes
  at centre ± 19.5 (129.54 / 168.54, top row, plus 168.54 bottom).
- SW1–SW3 at thirds of the new width (centres 111.71 / 149.04 / 186.37), the
  `button` labels with them.
- Back silk: title block keeps its offset from the low-x edge, logo and club
  text keep theirs from the high-x edge; rev text `rev X0.2 · 46 mm pitch`
  (now 2.1, below).
- U2, C1, R1, C2 and the XIAO notes did not move. Schematic unchanged.
- Re-routed from bare copper with Freerouting 2.4.1 (46 nets, 0 unrouted,
  0 violations, 20 passes), GND pour refilled. **DRC 0 errors / 0 warnings /
  0 unconnected / 0 parity** (2026-10-19).

Outputs of that pass: `fab/hot_info_orbs_xiao_46mm_X0.2_run1_gerbers.zip`
(superseded the same day, see `fab/README.md`).

## Grown 5 mm at the top (2026-10-19, `tools/grow_top.py`, rev 2.1)

Placed in the Fusion `overall assembly`, the X0.2 board's top mounting holes
sat 5 mm too low for a standoff post at either end of the top row: the
enclosure's top lip was in the way, so the ends hung from corner hangers
instead. The board cannot shift on the enclosure (the five displays are
pinned by the bezel holes) and the sockets fix the bottom edge, so the TOP
edge was grown 5 mm instead. Run once after `pitch_46.py`, then
`tools/route_xiao.py 20` again:

- Board **224.0 × 52.78 mm**, y 57.0 … 109.78 (was 62.0 … 109.78).
- Top-row holes up 5 mm to y 62.0 (x 42.04 / 129.54 / 168.54 / 256.04);
  bottom row unchanged. All seven still 5 mm from an edge.
- SW1–SW3 up 5 mm (y 59.59), so the actuators clear the enclosure's top lip
  by less: 12.9 mm plunger gap, was 17.9. The `button` labels went with them.
- GND pour rebuilt to the new edge. Displays, XIAO, passives, logo and title
  block did not move.
- Rev silk **`rev 2.1 · 46 mm pitch`** (Scott set the number 2026-10-19; it
  read X0.3 for a few minutes, never fabbed).
- Re-routed from bare copper: 46 nets, 0 unrouted, 0 violations, 300
  tracks+vias (6 vias). **DRC 0 errors / 0 warnings / 0 unconnected /
  0 parity.**

Outputs: `fab/hot_info_orbs_xiao_46mm_2.1_run1_gerbers.zip` (see
`fab/README.md`), `fab/46mm_top.png` / `fab/46mm_bottom.png`, and
`3d-assembly/hot_info_orbs_xiao_46mm_assembly.step` (same export recipe as
below; all six models resolve). That STEP is what `main_body_front` v4 and
`overall assembly` v4 were built against (`docs/ENCLOSURE.md`); rev 2.3
below overwrote it.

## Rev 2.3: XIAO 4 mm toward the bottom edge (2026-10-20, `tools/lower_xiao.py`)

Scott wants the USB-C jack as close to the enclosure wall as the board allows
without growing the board or crowding the socket pins against the edge; he
eyeballed 2-4 mm and the numbers allow the top of that range. Run once after
`grow_top.py`, then `tools/route_xiao.py 20` again:

- **U2 down 4.0 mm** to y 99.0. Lowest socket pads at y 106.62: pad copper
  2.31 mm from the edge (the display socket pads are 2.95 mm from it; the
  rule is 0.5). Socket housings end 1.89 mm inside the edge, the module body
  0.92 mm inside it, and the USB-C face (1.5 mm proud of the body) lands
  0.6 mm **past** the edge, at the wall. The F.Silk "XIAO goes on other
  side" rectangle moved with it and is clipped 0.5 mm inside the edge; the
  footprint's own USB-C silk crosses the edge and is left alone (3 DRC
  `silk_edge_clearance` warnings; the fab trims silk outside the outline).
- **C1 could not follow it down**: DS3's back-side pin labels sit at y 101.9
  and C1's courtyard was already 0.4 mm above them on 2.1 (a 2.4 mm drop put
  the 5V pad on the "DC" label, 3 silk warnings). So C1 keeps y 99.02 and
  moves **3.2 mm sideways to x 143.99**, courtyard 0.5 mm from the XIAO
  socket's, giving up the 3 mm "castellation pad" margin of 2026-10-17.
  Pad to pad the 5V run is 9.25 mm, the same as 2.1: the XIAO moved away
  from C1 by as much as C1 moved toward it. Anything shorter needs DS3's
  back-silk labels moved or C1 under the module (cap height unverified).
- **C2 and R1 swapped**, one column at x 143.99: C2 (1 µF reset cap) straight
  above C1 with its GND pad lowest (y 92.73, beside C1's GND at 96.52), R1
  above C2 with its RST pad lowest (85.04, adjacent to C2's RST at 87.73),
  3V3 pad at 74.88. The long run is the 3V3 feed, not the reset node.
  Courtyard gaps 0.5 mm; reference labels in a column 4.6 mm to the right.
- Rev silk **`rev 2.3 · 46 mm pitch`**; title-block rev 2.3 in both the
  schematic and the board (they read X0.1 / 1.1 before). **There is no 2.2**;
  Scott named this 2.3.
- Re-routed from bare copper: 46 nets, 0 unrouted, 0 violations, 284
  tracks+vias (14 vias). **DRC 0 errors / 3 warnings (the USB-C silk over
  the edge, above) / 0 unconnected / 0 parity.** `route_xiao.py` now reruns
  Freerouting when a pass ends "stage interrupted" with a non-empty SES
  (seen once: 44 unrouted after 0.14 s, the same DSN routed clean by hand).

**Run 2, same day (`tools/stitch_islands.py`):** Scott spotted two wide bare
bands in the back pour, SW2 down past R1 to the XIAO and DS1 to DS2 along
the bottom edge. Measured (fill redone in memory with island removal off):
removed islands of 283 and 211 mm², copper boxed in by the SW2/SW3 pair and
the 3V3 feed, and by the DS1 CS1/SCLK/MOSI bundle and the shared RST track.
Removing them is right (floating copper beside SPI lines), so the fix is a
connection: **the GND zone now pours F.Cu as well**, and a 0.8/0.3 GND via
sits in each island (68.70,101.46 and 143.10,68.13). Vias alone did nothing:
with no front copper the island plus via was still floating and KiCad still
removed it. DS5's GND pin gets 45° thermal spokes on the front (the +5V front
track along the bottom blocks its downward spoke; the rule wants two). DRC
0 / 3 / 0 / 0 as before; 16 vias. **ERC 0 / 0**: the six
`lib_symbol_mismatch` warnings were the project libraries still naming the
PINS/DIP footprints as the symbols' default while the schematic's embedded
copies (what the board was built from) name the SOCKET ones; the library
files now say SOCKET, the schematic is untouched.

**Run 3 (`tools/ds_refs.py`, 2026-10-20 17:44): the DS1–DS5 designators are
on the back silk**, 1.5 mm text mirrored behind each display socket (y 103.3,
glyphs 0.8 mm above the pad copper, 0.58 mm below the footprint's back
outline). They had been on the front silk at y 109.03 with their bottom
0.015 mm from the board edge: the fab trims silk outside the outline and
the display module covers that strip once plugged in, so a built board
had no readable socket name. Scott's ask: visible for troubleshooting with
beginners. Nothing else moved: run 3's copper, mask, outline and drill
differ from run 2 only by their timestamp lines. DRC unchanged (0 / 3 / 0 / 0).

Outputs: `fab/hot_info_orbs_xiao_46mm_2.3_run3_gerbers.zip` (runs 1 and 2
are superseded, see `fab/README.md`),
`fab/46mm_top.png` / `fab/46mm_bottom.png`, and
`3d-assembly/hot_info_orbs_xiao_46mm_assembly.step` (run 3 silk). `overall
assembly` v7 (2026-10-20) holds the run 2 STEP: the label move changed
silk bodies only, so it was not re-imported. `main_body_front` v4 has **no USB cutout yet** (an
earlier line here said one was "4 mm off"; it never existed); the jack's
place in the enclosure is measured in `docs/ENCLOSURE.md`.

The rest of this README is inherited from the parent project. Its y
coordinates and the pin map still hold; its x coordinates and board width
describe the narrow board.

---

# HoT Info Orbs — XIAO ESP32-S3 variant (KiCad project)

Carrier for five GC9A01 displays and three buttons, taking a **Seeed Studio
XIAO ESP32-S3 or XIAO ESP32-S3 Plus** (SKU 102010671), the builder's choice; the two
share the edge pins. This is the project's only board. It was copied on
2026-10-14 from the v1.1 prototype board, built for an ESP32-S3 SuperMini
(`hardware/hot-info-orbs/` at tag `v1.1-run1`, the boards ordered
2026-08-29); that module is not part of the project.

**S3 and S3 Plus are interchangeable here** (Seeed wiki, checked 2026-10-14):
identical D0–D10 → GPIO map, user LED GPIO21 on both, 21 × 17.8 mm both,
U.FL antenna both, 8 MB PSRAM both. Flash 8 MB vs 16 MB; the firmware builds
for 8 MB and uses all 8 MB, so one binary runs on either. The Plus's extra
D11–D19 are on its back and are not used by this board.

**Antenna:** the XIAO has a U.FL pigtail and a stick-on FPC antenna (Seeed
A-02, inventory part 102), and **the antenna is not mounted on this board**
(Scott, 2026-10-16), so the board has no copper keepout for it. The
2026-10-14 `ANTENNA_PATCH` rule area is removed; `tools/antenna_patch.py` is
retired and kept as history.

**Physical layout (2026-10-16/17, `tools/physical_rework.py`; routed 2026-10-17):**
- Displays plug in: each GC9A01 keeps its male header and seats in a 1x07
  2.54 mm female socket (footprint `_Displays:GC9A01_SOCKET`, same pads as
  `GC9A01_PINS`, display lifted 8.5 mm in 3D). Pad row at y 105.98, as low as
  the 0.5 mm copper-to-edge rule allows.
- Board top edge raised 17.28 mm to y 62.0, even with the tops of the
  displays (measured off a render); top mounting holes and the GND pour
  moved up with it. Bottom edge 2 mm lower (2026-10-17) to route under the
  display pin rows; the bottom holes followed it, so all seven are 5 mm in.
  Board is now 198.82 x 47.78 mm.
- Back silk: HOT hand logo, "The Villages / Hands-On Tech Club" and
  handsontech.org (the club's domain; it 301s to the weebly host).
- C1, C2, R1 and SW1-3 are on the BACK, the XIAO's side (Scott: "all on
  the same side as the xiao").
- Buttons: right-angle 6 x 6 x 8 mm tact switches (uxcell, Amazon) on the TOP
  edge, actuators out past it, centred on the board's 1/3 line, its 2/3
  line and midway: x 117.64 / 150.77 / 183.91. Footprint is KiCad's stock
  `SW_Tactile_SPST_Angled_PTS645Vx39-2LFS` - **unverified for the uxcell
  part**: the listing says 6.5 mm and 4.5 mm pin pitch, the footprint has
  4.5 mm (switch pins) and 7.0 mm (mounting tabs). Measure one.
- XIAO plugs in like the displays: male headers into two 1x07 female
  sockets (footprint `_ESP32 modules:XIAO-ESP32-S3-SOCKET`, holes at the
  header positions +-7.62 mm, 1.0 mm drill; module 11 mm off the board in 3D,
  with the male headers modelled under it, upside down, plastic on the sockets).
- C1 (+5V bulk for the displays) right beside the XIAO's 5V/GND pins;
  R1 + C2 (display RC reset) in the same column above it, R1 off the 3V3
  pin. Pass 1 had them in the bottom-edge gaps between sockets, ~36 mm
  from the 5V pin. The column sits 8.5 mm out from the pins, leaving 3 mm
  beside the XIAO free for castellation pads if we ever solder it down.
- White solder mask, black silkscreen (stackup colors; order the same at the fab).
- Board ID is on the back: a 28 mm HOT hand logo (`_Logos:Hot_Logo_28mm`,
  the 5 mm logo's polygons scaled) with "The Villages / Hands-On Tech Club"
  at one end; "HoT Info Orbs", "XIAO ESP32-S3 · rev X0.1" and the
  brettdottech credit at the other, in Comic Sans as on v1.1.
- Mounting holes: the four corner holes moved 2 mm in on each axis (now
  5 mm from each edge: x 56.37 / 245.18, y 67.0 / 102.78), plus two
  mid-board holes on the top row at x 131.28 and 170.27 (centre +-19.5 mm,
  in the gaps above DS2/DS3 and DS3/DS4; 22.5 mm from each display centre
  against a 16.2 mm display radius). Not on the bottom row: the XIAO, its
  sockets and the display socket row are there. A seventh hole on the
  bottom row straight below the right mid hole (170.27, 102.78), DS3/DS4
  gap, about 6 mm from each display tab; its left twin would hit the XIAO.

## Status — ROUTED (2026-10-17)

`tools/route_xiao.py`, one Freerouting pass, then DRC with refilled zones:
0 errors, 0 warnings, 0 unconnected, 0 parity (re-routed after the club URL,
bottom holes and upright DS labels: 314 tracks+vias). STEP in `3d-assembly/` now
carries tracks, pads and the pour. BOM: `bom/hot_info_orbs_xiao_bom.xlsx`
(and `.csv`), generated by `tools/make_bom.py`; C1 is 100 uF 16 V (set 2026-10-17).
BOM carries Link 1 prices and a build-cost total. Nothing ordered, nothing flashed.

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
| `lib/3d/_ESP32 modules/XIAO-ESP32S3-PLUS-v4.step` | U2's 3D model (Seeed assembly "XIAO ESP32S3 PLUS_V1.0_241025"), attached by `tools/xiao_model.py` (offset (-1.778, -7.62, 0) / rotate (-90, 0, 0), found by rendering against the DIP footprint). In the SOCKET footprint the offset is (-1.651, -6.13, 11): the old fit left the module's holes 1.49 mm off the header pins, measured in Fusion 2026-10-16. Also stands in for the plain S3 (same outline, same USB end). | Seeed product page download, 2026-10-14 |

The SuperMini symbol/footprint are still in `lib/` but unused.
