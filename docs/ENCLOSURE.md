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

**Priced on McMaster, 2026-11-09 (live, not estimates): Phillips pan head
thread-forming screws for plastic, zinc-plated steel.** Phillips because every
builder has a driver (Scott, 2026-11-09); these sizes take a **No. 1 Phillips**,
not the common No. 2. 15 per kit: 8 end-cap, 4 back-cover, 3 PCB. Yang's model
lengths and the nearest stock size of each:

| Position | Qty | M3 Phillips | Per pc | #4 Phillips | Per pc |
|---|---|---|---|---|---|
| End cap → body, 8 mm | 8 | M3 x 8, 90380A349, 100/$8.38 | $0.084 | #4 x 3/8", 90380A108, 100/$9.98 | $0.100 |
| Back → PCB → body, 6.5 mm | 4 | M3 x 6, 90380A348, 100/$8.22 | $0.082 | #4 x 1/4", 90380A106, 100/$9.54 | $0.095 |
| PCB → front body, 4 mm | 3 | M3 x 5, 90380A347, 100/$5.92 | $0.059 | #4 x 1/4", 90380A106 | $0.095 |
| **Per kit** | 15 | | **$1.18** | | **$1.47** |

The run is **40 kits (Scott, 2026-11-09) = 600 screws**: 320 x 8 mm, 160 x
6.5 mm, 120 x 4 mm. M3 = 4 packs of 8 mm + 2 of 6 mm + 2 of 5 mm = **$61.80**
($1.55/kit as bought, 120 spare); #4 = 4 packs of 3/8" + 3 of 1/4" =
**$68.54** ($1.71/kit, 100 spare). Torx (99512A3xx) is $0.01-0.02
a screw cheaper and was rejected for the driver; black oxide and 50-packs
cost more per piece in every size. McMaster ground shipping (~$8-10) is a
fifth of the screw bill, so put them on another McMaster order. The BOM's
AliExpress M3 x 6 socket heads are $0.04 each if that listing also carries
8 mm, but they need a hex key.

Two reasons M3 beats #4 beyond price: McMaster's pilot for its M3 plastic
screws is 2.25-2.44 mm, and for the #4 tri-lobe 2.64 mm, so the **Ø2.2 boss
is an M3 pilot**, tight for a #4 in PLA. And no stock #4 is shorter than 1/4"
(6.35 mm), while M3 comes in 5 mm for the 4 mm PCB-to-front position; whether
a 5 mm (or 1/4") screw bottoms in that boss is still Yang's to confirm.

**Better source than McMaster (checked live 2026-11-09): Bolt Depot.** Scott
asked whether this commodity has a cheaper home. Amazon is mostly assortment
kits and the 100-packs are 304 stainless at $0.06-0.07 (no cheaper than it
is worth), and AliExpress lead time rules it out (Scott, 2026-11-09).
Fastenal carries the exact part, #4-20 Phillips pan zinc **Hi-Lo** thread-
rolling screws for plastic, at $0.04 each, but only as 500/1000 packs
(924847104 1/4" 500/$21.65; 925206223 3/8" 1000/$41.30 = $62.95 for 1,500),
so no saving at this run. **Bolt Depot** (Hingham MA, no minimum order) sells
plain #4 zinc Phillips pan sheet-metal screws, ASME B18.6.3 type A/AB,
**No. 1 Phillips**, by the 100:

| Position | Qty for 40 kits | Bolt Depot | Bags | Cost |
|---|---|---|---|---|
| End cap → body, 8 mm | 320 | #4 x 3/8", prod 2469, 100/$2.21 | 4 | $8.84 |
| Back and PCB, 6.5 + 4 mm | 280 | #4 x 1/4", prod 7370, 100/$2.07 | 3 | $6.21 |
| **40 kits** | 600 | | 7 | **$15.05** + $12.13 Economy (USPS, 3-7 days) + $1.90 FL tax = **$29.08 landed** (cart estimate to the LRD zip, 2026-11-09; UPS Ground $17.15). **Superseded the same day: all 3/8", see the Fusion check below.** |

That is **$0.38/kit** ($0.73 landed) against $1.55 at McMaster before shipping, with 100 spares. The cart is built (300 x 7370, 400 x 2469) and not checked out. Trade-offs:
these are sheet-metal threads, not the Plastite/Hi-Lo profile cut for
plastic, so they displace more PLA in the Ø2.2 boss; McMaster's own pilot
for a #4 plastic screw is 2.64 mm. **Print one front body and drive a #4 x
3/8" into a Ø2.2 boss before ordering 600** (a split boss means open the
hole to ~2.4-2.5 mm in the model, which is Yang's). Bolt Depot has no M3
sheet-metal screws, so this choice is also the #4-over-M3 decision. **No
black #4 at Bolt Depot either** (Scott asked, 2026-11-09): their black Phillips
pan self-tappers are stainless, black oxide from #6 up and painted-black #8
and #10 only. Zinc is the only #4 finish there; black means McMaster. Fastenal
is the fallback if the sheet-metal thread cracks bosses and the hole can't
move: same $0.04 each in the proper Hi-Lo form, bought 1,500 at a time.

**Test stock on hand (2026-11-09):** Scott's 625-piece "PA Self-Tapping Micro
Screws" kit has 50 each of M3 x 6 and M3 x 8. PA = pan head, sharp point
(PB is the blunt-tail sibling): a ~2.9-3.0 mm, ~1.05 mm pitch, 60° thread,
i.e. ST2.9 / type AB sheet-metal form, not Plastite or Hi-Lo. That is within
0.15 mm and 0.01 mm pitch of a #4 AB, so the boss-split test above can be
run with these before anything is ordered. Identification is from the label
convention, not a measured thread.

**Boss depth checked in Fusion (2026-11-09, read-only, nothing saved).** Scott
asked whether each screw length has enough boss. Measured in the open v0.4
files (main body front v2, main body back v2, wing right/left v1, joint sleeve
v1, master assembly v2), not the v0.3 the table above cites. All distances are
in the body's frame, y = 0 at the front face, y along the screw:

| Path | Stack the screw passes (mm) | Boss hole | #4 x 1/4" (6.35) | #4 x 3/8" (9.525) |
|---|---|---|---|---|
| End cap → front boss ×8 | wing 5.0 (head on the outer face, no counterbore) | Ø2.2, 7.0 long, **through** into the open interior | — | 4.5 engaged, tip 2.5 short of the interior; cannot bottom |
| Back → PCB → front boss ×4 | back cover 2.0 + PCB 1.51 | Ø2.2, 10.0 deep, blind, 5.95 solid under it | **2.7 engaged**, 7.3 spare | 5.9 engaged, 4.1 spare |
| PCB → front boss ×3 | PCB 1.51 | same | 4.8 engaged, 5.2 spare | 8.0 engaged, 2.0 spare |

So neither stock length bottoms anywhere, and the ordered split (3/8" end
cap, 1/4" the rest) fits. The weak spot is not depth but **engagement at the
back cover: 2.7 mm, under three pitches of a #4 AB** (Yang's 6.5 mm nominal
gives 2.9). A 3/8" there engages 5.9 mm with 4.1 mm to spare, and 3/8" in
the PCB position still leaves 2.0 mm, so **one length, 600 x 3/8" (6 bags of
prod 2469, $13.26), is cheaper than the two-length cart and engages more
everywhere.** **Decided 2026-11-09 (Scott): one length.** The Bolt Depot cart
is now 600 x #4 x 3/8" (prod 2469, 6 bags, $13.26); the 1/4" (prod 7370) is
out. Estimate to the LRD zip: $8.41 Economy + $1.52 tax = **$23.19 landed**
($0.58/kit; UPS Ground $32.54). **Ordered 2026-11-09 (Scott): Bolt Depot order
2651204, PO "Info Orbs", 600 x 2469, $13.26 + $8.41 Economy + $1.52 tax =
$23.19, Economy to LRD.** The boss-split test with the PA screws is now a check on the printed
part, not a gate on the order. Economy dropped from
$12.13 to $8.41 while the cart's estimated weight stayed at 1.0 lb both
times (Scott, 2026-11-09), so Bolt Depot's Economy rate carries a
per-line or per-bag surcharge (~$3.72 here), not a weight step: on
small orders, fewer lines is worth real money. The screw table above is the as-designed lengths;
the kit ships 15 x 3/8". Also found:

- The eight Ø2.2 bosses are at x = -108.7, -21.2, 17.8, 105.3 (body frame),
  z = 13.635 / 56.415, top at y = 15.95, boss OD ~5.0, so the **wall around a
  #4 is 1.4 mm** - the split test above stays necessary.
- The PCB (`hot_info_orbs_xiao_46mm_assy final` in the master) sits on the
  boss tops and has 7 Ø3.2 holes on 7 of the 8 boss positions; the back
  cover's 4 Ø3.25 holes are the outer four (x = -108.7 / 105.3). The three
  PCB-only screws therefore take 17.8/13.635, -21.2/56.415 and 17.8/56.415;
  the boss at -21.2/13.635 has no PCB hole. **The KiCad board on this branch
  has no mounting holes at all** (no MountingHole footprint, no NPTH); the
  holes exist only in Yang's Fusion PCB, which is 1.51 mm thick against the
  board's 1.6.
- **The v0.4 joint sleeve has no screw holes**: a 4 mm wide, 1 mm wall ring
  straddling the body/wing seam, 2 mm on each side, outside the screw path.
  The "end cap → joint sleeve → boss" path and its Ø3.2 x4 in the table are
  v0.3.
- Wing left mirrors wing right: 4 Ø3.25 x 5.0 at y = 12 / 28, z = 5.5 / 70.

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
