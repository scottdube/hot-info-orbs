# v0 demo stand — throwaway end feet for the v1.1 run1 board

Two identical printed L-feet that hold the first-run SuperMini board
(`hardware/hot-info-orbs/` at tag `v1.1-run1`, 198.8 x 28.5 mm, four
Ø3.2 mm corner holes 3 mm in from each edge) upright for the demo.
Disposable: the XIAO board (`hardware/hot-info-orbs-xiao/`) gets a proper
enclosure and this folder goes with the v1.1 board when it is retired.

**Why the green legs failed:** they grip the board at the board plane, and the
displays on sockets put the combined centre of gravity ~8 mm in front of
that plane, so the board tips forward unless the cable is weighed down.
These feet put 40 mm of base in front of the board face and screw the
board to a tab behind it.

**Why a screw and not a slot:** a 10° recline moves the CG back only ~2.5 mm
of the ~8 mm it sits forward; a screwless slot stand would need ~30° of
recline plus a front lip, and the displays come within 1.5 mm of the
board's bottom edge, so there is no room for a lip under them anyway.

| | |
|---|---|
| Foot | 60 x 16 x 20 mm, base 4 mm, tab 4 mm, tab leans back 10° |
| Toe / heel | 40 mm ahead of the board face, 20 mm behind |
| Hole | Ø2.6 mm through the tab, 3 mm above the seat, centred on the width — M3 x 8 self-taps |
| Board end | sits 5 mm inside the foot's outer edge (hole is 3 mm in from the board end, foot is 16 wide) |

**Print:** 2 off, flat on the base, any PLA/PETG, no supports (the 10°
tab is well under the overhang limit). ~4 g each.

**Fit:** board bottom edge on the base, back face against the tab, one
M3 x 8 through each tab into the bottom corner hole from behind. The
SuperMini and its USB-C are mid-board, nothing is near the ends.

Files: `v0-demo-stand-foot.stl` / `.3mf` (mm), `foot-iso.png`.
Fusion source: **HOT Projects / info-orbs-v0-demo-stand** (parameters
BaseT, TabT, TabH, Toe, Heel, FootW, Lean, HoleD, HoleUp drive the
extrude width and hole; the side profile itself is drawn to numbers, not
to those parameters — it is a throwaway).
