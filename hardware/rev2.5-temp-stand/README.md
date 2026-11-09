# rev 2.5 temp stand — end feet for the 46 mm carrier

Two printed L-feet that hold the rev 2.5 board (`hardware/hot-info-orbs-xiao-46mm/`,
224.0 x 52.78 mm, XIAO ESP32-S3 on the back) upright until it has an
enclosure. Temporary, like the v0 demo stand they are copied from.

**What carried over from v0** (`info-orbs-v0-demo-stand` v9): the base, toe and heel (40 mm ahead of the board face, 20 mm behind),
the 10° lean, the 20 mm seat and the 2 mm step the board edge rests on, the
12 mm width with the board end flush with the foot's outer face, and M3 x 8
self-taps from behind. The v0 README has the reasoning for each (CG in
front of the board plane, screws not a slot, 90° USB-C plug under the
edge). The XIAO's USB-C on rev 2.5 also points down and sits only 0.6 mm
past the bottom edge, so 20 mm still clears a 90° plug.

**What changed — the upright and the holes:**

- The tab runs the full board height, 52.78 mm along the face (v0: 28.5).
- Holes are where the rev 2.5 end holes are: **5 mm from the board end,
  5 mm from the top and bottom edges, 42.78 mm apart** (v0: 3 mm, 22.5 mm
  apart). Measured from the KiCad file on main: end holes at x 42.04 and
  256.04 on a 37.04…261.04 outline, rows at y 62.0 and 104.78 on
  57.0…109.78.
- **A 3 x 8 mm relief in the tab's inboard edge**, from 9 mm in to the
  12 mm edge and 8 mm up the face from the step. DS5's socket pins are
  10.6 mm from the right board end and 2.9–4.7 mm above the bottom edge,
  so their solder tails on the back would land on a solid 12 mm tab. The
  part is mirrored, so the left foot carries the relief too; DS1's pins
  are 14.2 mm from the left end, clear of the foot either way. Rejected
  narrowing the foot to 9 mm instead: it moves the board end off the
  foot's center and takes width from the base that keeps it from tipping.

| | |
|---|---|
| Foot | 60 x 12 x 72 mm: flat base 4 mm, riser to the 20 mm step, tab 4 thick and 52.78 long along the face, leaning back 10° |
| Toe / heel | 40 mm ahead of the board face, 20 mm behind |
| Seat | board bottom edge on the step, 20 mm above the table |
| Holes | two Ø2.6 mm through the tab, 5 and 47.78 mm above the step measured along the face, 5 mm in from the outer (flush) face — M3 x 8 self-taps |
| Relief | 3 mm (9–12 mm in from the outer face) x 8 mm up the face from the step, through the tab |
| Volume | 6426 mm³ each (Fusion and the exported mesh agree), ~8 g solid PLA |

**Print:** one `-left` and one `-right` (as seen from the front, displays
toward you), flat on the base, PLA/PETG, no supports. The relief's roof is
a 3 mm overhang off the tab; it may droop a little and nothing rests on it.

**Fit:** board bottom edge on the step, back face against the tab, two
M3 x 8 through each tab into the end holes from behind. Plug the 90° USB-C
in pointing down.

**Not yet checked on the real board:** the solder-tail relief is from the
footprint, not a measured board, and no assembly interference check was run
this time. Print one right foot and offer it up before printing the pair.

Files: `rev2.5-temp-stand-foot-left.stl` / `.3mf` and `-right.stl` / `.3mf`
(mm), `foot-iso.png` (left foot). Fusion source: **HOT Projects /
info-orbs-rev2.5-temp-stand** (Body1 = left, Body2 = its Fusion mirror, so
both files come straight from Fusion, not a flipped mesh). Parameters BaseT,
TabT, TabH, Toe, Heel, FootW, Lean, SeatH, StepD, HoleD, HoleIn, HoleUp1,
HoleUp2, ReliefIn, ReliefH document the numbers; FootW, ReliefIn and TabT
drive features, the side profile and hole circles are drawn to numbers.

## Screws for the kit — McMaster, priced live 2026-11-09

Four per kit (two per foot). Thread engagement is in the 4 mm PLA tab only:
the board's end holes are Ø3.2 (M3 clearance), so the screw passes through
the board whichever way it goes in. Thread-forming screws for plastic, steel
pan head, read off mcmaster.com in Chrome (not estimates, no +40%):

| Screw | McMaster | Pack | Per piece | Per kit (4) | McMaster hole |
|---|---|---|---|---|---|
| **M3 x 8 Torx T10, zinc** | 99512A375 | 100 / $6.89 | $0.069 | **$0.28** | 41 Ga. (2.44 mm) |
| M3 x 8 Phillips #1, zinc | 90380A349 | 100 / $8.38 | $0.084 | $0.34 | 2.25 mm |
| #4 x 5/16" Phillips #1, black oxide | 90417A117 | 100 / $7.67 | $0.077 | $0.31 | 37 Ga. (2.64 mm) |
| #4 x 5/16" Phillips #1, zinc | 90380A107 | 50 / $6.12 | $0.122 | $0.49 | 37 Ga. (2.64 mm) |
| #4 x 5/16" Torx T10, zinc | 99512A621 | 100 / $11.67 | $0.117 | $0.47 | 37 Ga. (2.64 mm) |

Cheapest is the M3 x 8 Torx at $0.28/kit; the cheapest #4 is 3¢/kit behind.
A 100-pack covers 25 kits. McMaster ground shipping (~$8–10) is more than
the screws, so ride them on another McMaster order.

**Hole-size note:** the foot's Ø2.6 tab hole is what McMaster lists for the
#4 tri-lobe (2.64 mm) and is 0.15–0.35 mm over what it lists for the M3
round-shank screws (2.25–2.44 mm). FDM holes print undersize, so the M3 may
bite fine as drawn; if it strips, shrink `HoleD` to 2.4 rather than switch
screws. The 5 mm M3 Torx (99512A373, $6.43/100) is the same price class if
the proud tip past the tab is unwanted.
