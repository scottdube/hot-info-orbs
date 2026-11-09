# Enclosure

## Chosen style: Concept 3, "Winged Speedster" (2026-10-17)

![Concept 3 – Winged Speedster](assets/enclosure-concept-3-winged-speedster.png)

This is a concept rendering, not a model. It sets the look the design aims for:

- A Jet Age bar: dark blue-grey body, a brass ring bezel around each display,
  and finned brass end caps.
- Flat front and back, with the five 1.28" displays parallel to the PCB.
- Designed for 3D printing in several parts.

**Change from the rendering: the three buttons move to the top.** The rendering
shows them on the front, under the displays. The XIAO carrier board
(`hardware/hot-info-orbs-xiao/`, branch `xiao-esp32s3`) already puts its
right-angle switches on the top edge, so the board needs no change for this.

## Inputs already measured

| What | Value | Source |
|---|---|---|
| Bezel opening (lit area) | **~32.5 mm** dia; shows every pixel | [DISPLAY-EDGE-TEST.md](DISPLAY-EDGE-TEST.md) |
| Dark glass band outside it | ~1.5 mm, even all round, no pixels under it | same |
| Module pocket | the GC9A01 STEP model, caliper-checked by Scott | `hardware/hot-info-orbs/lib/3d/_Displays/` |

A bezel opening wider than ~32.5 mm shows the black band inside the brass ring.

## Screws: where the thread engages (2026-11-06)

Fasteners: Yang designed for **#4 self-tapping screws** and M3-0.5 socket head
(the BOM) works too (Scott, 2026-11-08). Lengths from Yang's model, with the stock
#4 size: end cap → body 8 mm (3/8") ×8; back cover → PCB → body 6.5 mm (1/4") ×4;
PCB → front body 4 mm (1/4") ×3. The thread is cut by the screw in
**main body front** only (Ø2.2 bosses, pilot for self-tapping). Every part the
screw passes through first gets an **M3 clearance hole, Ø3.2**.

| Part (Fusion, HOT Projects / HOT infor Orbs 1 / v0.3) | Hole | State |
|---|---|---|
| main body front | Ø2.2 bosses | thread engages here; unchanged |
| joint sleeve | Ø3.2 ×4 | v2: was Ø2.85 |
| winged end caps | Ø3.2 ×8 | v2: was Ø2.85 |
| main body back | Ø3.205 | already clearance |
| PCB assy | Ø3.2 | already clearance |
| display bezel, button bezel | n/a | no screws; leave alone |

Screw paths: the side screws run end cap → joint sleeve → front boss. The back
screws run back → PCB → front boss.

**Priced on McMaster, 2026-11-09 (live, not estimates), thread-forming screws for
plastic, steel pan head, zinc.** 15 per kit: 8 end-cap, 4 back-cover, 3 PCB.
Yang's model lengths and the nearest stock size of each:

| Position | Qty | M3 Torx T10 | Per pc | #4 (cheapest) | Per pc |
|---|---|---|---|---|---|
| End cap → body, 8 mm | 8 | M3 x 8, 99512A375, 100/$6.89 | $0.069 | #4 x 3/8" Phillips, 90380A108, 100/$9.98 | $0.100 |
| Back → PCB → body, 6.5 mm | 4 | M3 x 6, 99512A374, 100/$6.63 | $0.066 | #4 x 1/4" Torx, 99512A346, 100/$8.54 | $0.085 |
| PCB → front body, 4 mm | 3 | M3 x 5, 99512A373, 100/$6.43 | $0.064 | #4 x 1/4" Torx, 99512A346 | $0.085 |
| **Per kit** | 15 | | **$1.01** | | **$1.39** |

A 30-kit run: M3 = 3 packs of 8 mm + 2 of 6 mm + 1 of 5 mm = **$40.36**;
#4 = 3 packs of 3/8" + 3 of 1/4" = **$55.56**. Phillips M3 (90380A3xx) costs
$0.08-0.09 each; black oxide and 50-packs cost more per piece in every size.
McMaster ground shipping (~$8-10) is a quarter of the screw bill, so put
them on another McMaster order. The BOM's AliExpress M3 x 6 socket heads are
$0.04 each if that listing also carries 8 mm.

Two reasons M3 beats #4 beyond price: McMaster's pilot for its M3 plastic
screws is 2.25-2.44 mm, and for the #4 tri-lobe 2.64 mm, so the **Ø2.2 boss
is an M3 pilot**, tight for a #4 in PLA. And no stock #4 is shorter than 1/4"
(6.35 mm), while M3 comes in 5 mm for the 4 mm PCB-to-front position; whether
a 5 mm (or 1/4") screw bottoms in that boss is still Yang's to confirm.

**Leftover from the #4-screw design:** the sleeve and end-cap holes were driven
by the user param `number_4_screw_thread_dia` (2.85 mm). Each of those docs now
has its own `screw_clearance_dia = 3.2 mm`, and the hole dimensions point at
that. We rejected setting `number_4_screw_thread_dia` to 3.2: a param named
"thread dia" that holds a clearance value would mislead the next editor.

**Still open:**

- The hidden `rear heel:1/:2` occurrences in the master still carry Ø2.85 holes
  at the end-cap screw positions.
- The Ø2.5 holes in display standoff were not touched.

Neither has a confirmed role in the current screw path.
