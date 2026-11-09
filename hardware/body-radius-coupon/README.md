# Body radius coupon: face-down print test

**Question:** printed face-down, can slicer settings make the R12 radii of the
main body good enough? Or does the body have to print on end?

Opened 2026-11-06. Nothing printed yet.

## What the coupons are

These are 40 mm long slices of the real cross-section, taken at x = 22 mm in
Fusion `HOT Projects / v0.3 / master assembly`. At that position there are no
holes, posts or button cut-outs, so the slice is the plain profile. Each coupon
is a straight extrude of that section. They were made in an unsaved Untitled
Fusion document, so `main body front` and `main body back` were not touched.
The section is identical in the `HOT Projects` copy of `master assembly`
(checked edge by edge).

| File | Size (mm) | Volume | Triangles |
|---|---|---|---|
| `front-coupon.stl` | 40 × 75.5 × 35.78 tall | 10.71 cm³ | 58,616 |
| `back-coupon.stl` | 40 × 70.46 × 5.28 tall | 5.62 cm³ | 25,820 |

- **Orientation:** both are exported already face-down, with the display face
  (front) or the back face on z = 0. Don't use lay-on-face.
- **Mesh:** fine, with 2° normal deviation, 0.005 mm surface deviation and
  1 mm maximum edge. The STL volumes match the B-rep to 0.01%, so mesh facets
  are not a factor in this test.

## Why face-down struggles (measured from the model)

Each R12 radius meets the bed at a tangent. For its first **3.5 mm** of height
it overhangs more than 45°. At 0.2 mm layers, the first layers step outward by
0.9, then 0.7, then 0.6 mm, which is wider than a 0.42 mm line, so those
perimeters print over air. At 0.08 mm layers the step is still about 0.4 mm.
The back body's visible radius ends at 45°, so all of it falls inside that
overhang band.

## Plate: 3 front coupons, one plate, one filament

- **Project file:** `coupon-plate-h2d.3mf`, built 2026-11-06 from Scott's own
  Nov 5 H2D project (same printer, process and filament list). All three
  coupons are on slot 3, Generic PLA #443089. Everything below is already set
  in the file.
- **Printer:** H2D (Scott, 2026-11-06), the printer that will run the real
  parts. One nozzle, one filament, no filament changes.
- **Why one plate:** three coupons make each layer take longer, which is closer
  to the real 249.5 mm part. A single coupon would cool better than the real
  part does.
- **Why front only (Scott, 2026-11-06):** the back body's whole visible radius
  is the same R12 arc as the front's bottom 3.5 mm. Both are tangent to the bed,
  and the back's arc ends at 45°. So the front coupon's bottom 3.5 mm prints the
  back's entire radius. **Not covered:** fuzzy skin across that overhang band
  (the old B-C). F-C keeps fuzzy skin out of the bottom 5 mm. `back-coupon.stl`
  stays here if that question comes up.

| Coupon | Variant | Settings |
|---|---|---|
| F-A | **Control** | Nov 5 slice settings: 0.20mm Standard @BBL H2D, unchanged |
| F-B | **Overhang-tuned** | Settings below |
| F-C | **Tuned + fuzzy skin** | B settings, plus fuzzy skin as described below |

**B: overhang-tuned** (setting names as in Bambu Studio):
- **Layer height:** height-range modifier from 0 to 5 mm at **0.08 mm**; the
  control layer height above that.
- **Outer wall speed:** 60 mm/s.
- **Slow down for overhang:** on. Overhang speeds: 0–25% 50, 25–50% 25,
  50–75% 15, 75–100% **10** mm/s.
- **Filament cooling: dropped (2026-11-06).** Generic PLA @BBL H2D already
  runs the part fan at 100% on every layer after the first
  (`fan_min_speed` = `fan_max_speed` = 100). The overhang-fan threshold can't
  raise it any further. It would also have needed a second project filament,
  which forced a filament swap and a prime tower on every layer. Bambu Studio
  refuses that anyway when objects have different layer heights: "The prime
  tower is only supported if all objects have the same variable layer height".
- **Wall order:** inner/outer, the same as the control.
- **Seam:** all three coupons are rotated so the 40 mm extrusion runs along Y,
  and seam = Back. The seam then lands on the rear cut face, a face the real
  part doesn't have.

**C: fuzzy skin:**
- **Mode:** Others → Fuzzy skin = **Contour** (outer walls only), thickness 0.3,
  point distance 0.8.
- **Front coupon:** fuzzy only from **5 mm to 34 mm** (height-range modifier).
  The bottom 5 mm is the overhang band, where fuzzy skin would push the
  perimeter further into the air. The top ~1.8 mm is the lap joint with the
  back body and must stay true.

Mark each coupon on the inside with a Sharpie as it comes off the plate. On the
plate, left to right: F-A, F-B, F-C (centres at x = 85, 175, 265 mm).

## Slice check (headless Bambu Studio CLI, 2026-11-06)

Sliced from `coupon-plate-h2d.3mf` and read back per object from the G-code:
**1 h 38 m, 36.4 g, 0 filament changes.**

| | F-A | F-B | F-C |
|---|---|---|---|
| Layers in 0–5 mm | 0.2 | 0.08 | 0.08 |
| Outer wall, max mm/s | 159 | 60 | 60 |
| Overhang wall mm/s | 10–50 | 10–50 | 10–50 |
| Outer-wall segments, 5–34 mm | 1,528 (median 1.58 mm) | 1,480 | **29,243** (median 0.84 mm): fuzzy |
| Seam, y of loop start (part spans 140–180) | 179.8 | 179.8 | 179.2–180.1 |

Outer-wall speeds below the max come from the 8 s minimum layer time.

## Results

Printed 2026-11-06 on the H2D in blue PLA. Scott numbered them 1, 2, 3 left
to right off the plate, so 1 = F-A, 2 = F-B, 3 = F-C. Scored by Scott by hand
on 2026-11-06. **Scale: 1 = worst, 5 = best, in every column.**

| Coupon | Radius quality | Droop at bed edge | Lap edge | Notes |
|---|---|---|---|---|
| F-A (#1) | **3** | **4** | 5 | About the same as the Nov 4 body |
| F-B (#2) | 1–2 | 1 | 5 | Worse than the control |
| F-C (#3) | 1–2 | 1 | 5 | Fuzzy skin did not hide it |

**What was measured:** the stock profile beat both tuned variants on both
radius quality and droop. The lap edge was fine on all three.

**What it rules out:**
- **Mesh faceting.** F-A (1° facets) looks about the same as the Nov 4 body
  (9° facets from Fusion's 3D Print STL), so the faceting was not the problem.
- **The B settings as a group.** 0.08 mm layers, outer wall 60 mm/s and
  inner/outer order together made both quality and droop worse.

**Cause of B's droop: not established.** B changed layer height and outer-wall
speed at the same time, so this plate can't say which one did it. One untested
explanation is that a slower outer wall puts more heat into an unsupported
perimeter. Isolating it needs a coupon that changes only one of the two.

**Where face-down stands:** with stock settings, droop is acceptable (4), but
the radius quality stays at 3, the same as the real part.

## Which file was the poor print? (settled 2026-11-06)

**The job:** Bambu Handy shows the poor face-down print started **Nov 4 at
17:14**. Both bodies came from **Fusion's 3D Print hand-off**, which writes an
STL with Fusion's default mesh to `$TMPDIR/Neutron/` and opens Bambu Studio:
- `main body front.stl`, written 16:52:11 (11,006 triangles). Bambu Studio
  session 21232 started at 16:52:02, so Fusion launched the slicer.
- `main body back.stl`, written 16:58:44 (5,848 triangles).

Both meshes are closed. The R12 arcs are flats of about **9° each**: 10 per 90°
on the front, the same step on the back's 45° arc. Each flat is 1.9 mm wide and
37 µm deep. A copy of both files is in the session scratchpad. They are not
committed here, because they are Yang's enclosure and this repo is public.

**Still unknown:** the print settings. That session's autosave was cleaned up and
the logs are encrypted. The job's `.gcode.3mf` holds them, in Bambu Studio →
Device → Storage on the printer named in Handy.

**Ruled out:**
- `~/Downloads/main body front.3mf` (Oct 31, Gmail, `v15`) did not print. It has
  17 flats per 90°.
- `~/Downloads/main body front.stl` (Nov 5 12:59) was a later re-slice.

**A later slice, for reference:** on Nov 5 at 13:00 the H2D sliced
`~/Downloads/main body front.stl` (written 12:59) on plate 2, with these settings:
- 0.20mm Standard @BBL H2D, Generic PLA #443089, textured PEI, no supports.
- 69.6 g, 2 h 27 m.
- Outer wall 200 mm/s; overhang speeds 0/50/30/10.
- For that filament: overhang fan 100%, overhang fan threshold 50%.
- Inner/outer wall order, aligned seam.

This record is in the Nov 5 autosave
(`bamboo_model/Thu_Nov_05/09_35_15#18743#51`). Use it for control coupon A only
if Scott confirms those were his usual settings.

**R12 faceting in each file** (distinct mesh positions on the arc; both meshes
are closed):

| File | Triangles | R12 facets per 90° | Facet width | Sagitta |
|---|---|---|---|---|
| **Nov 4 printed front** (Fusion 3D Print) | 11,006 | 10 (9°) | 1.88 mm | 37 µm |
| **Nov 4 printed back** (Fusion 3D Print) | 5,848 | ~5 over its 45° (8.7°) | 1.82 mm | 35 µm |
| Nov 5 STL | 11,010 | 10 (9°) | 1.88 mm | 37 µm |
| Oct 31 3MF (v15) | 13,102 | 17 (5.3°) | 1.11 mm | 13 µm |
| Coupons | 58,616 | 90 (1°) | 0.21 mm | <1 µm |

A 37 µm flat is smaller than a 0.2 mm layer step, but the facet edges run the
full 249.5 mm length as straight lines, which can show on a glossy filament.
**Answered 2026-11-06:** F-A (1° facets) is about the same as the Nov 4 body
(9° facets), so faceting was not part of the problem. A finer export is still
cheap insurance.

## If B and C both fail: print on end

Stand the part on an end face, with the 249.5 mm length vertical. The radii then
print as in-plane curves.

- **Height:** fits the H2D. The X1C has only 6.5 mm of headroom.
- **Supports, front body:**
  - Tree supports under the **7 PCB standoffs** (Ø5 × 13 mm, now horizontal).
    They sit in line along the length, so each tree can land on the standoff
    below it; the tallest is about 90 mm.
  - Support enforcers at the **top of each display hole** (Ø34.2 in a 2 mm
    wall). The bezel opening is Ø35.7, so 0.75 mm of the hole edge shows inside
    the bezel and the Ø35.5 crystal rests on it.
- **Supports, back body:** tree supports under the **4 bosses**
  (Ø11.5 × 20 mm). The far pair are about 214 mm up with nothing below them but
  the near pair. That is the risky part.
- **No support needed:** button holes, the bezel pin holes and the end-cap
  screw bosses.
- **Brim:** 8–10 mm. The back body stands on only about 150 mm² per end.

### Yang's on-end print (photo via Scott, 2026-11-06)

This is a front body printed standing on end in dual-color silk PLA, with tree
supports inside it. The slicer settings are unknown and the photo is soft.
What it shows:
- **Support scars on the top arc of each display hole.** The trees land there,
  and the edge is ragged and torn. That is the edge the bezel opening shows,
  about 0.75 mm of it, and the crystal rests on it.
- **The bezel pin holes print as horizontal holes.** Several are misshapen at
  the top, and the ones touched by supports are torn.
- **The display face is now a layered wall,** not a bed face. Layer lines run
  across it, with visible banding. Some of the banding may be the silk's color
  shift; that is not established.
- **The supports sit inside the shell.** They have to come out through the
  Ø34 holes, and the contact points under the standoffs can't be seen.

**Net:** on end trades the radius stepping for damage on the display face,
which is the face that matters most. Face-down at stock settings (radius 3,
droop 4) remains the better trade so far.
