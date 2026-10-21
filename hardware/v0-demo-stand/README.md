# v0 demo stand — throwaway end feet for the v1.1 run1 board

Two identical printed L-feet that hold the first-run SuperMini board
(`hardware/hot-info-orbs/` at tag `v1.1-run1`, 198.8 x 28.5 mm, four
Ø3.2 mm corner holes 3 mm in from each edge) upright for the demo.
Disposable: the XIAO board (`hardware/hot-info-orbs-xiao/`) gets a proper
enclosure and this folder goes with the v1.1 board when it is retired.

**Why the green legs failed:** they grip the board at the board plane, and the
displays on sockets put the combined center of gravity ~8 mm in front of
that plane, so the board tips forward unless the cable is weighed down.
These feet put 40 mm of base in front of the board face and screw the
board to a tab behind it.

**Why a screw and not a slot:** a 10° recline moves the CG back only ~2.5 mm
of the ~8 mm it sits forward; a screwless slot stand would need ~30° of
recline plus a front lip, and the displays come within 1.5 mm of the
board's bottom edge, so there is no room for a lip under them anyway.

**Why the board sits 20 mm up (v2):** the SuperMini's USB-C is on the back
of the board at mid-length and points at the bottom edge; the connector
face overhangs that edge by 1.3 mm. The first cut of these feet rested the
board edge 4 mm off the table, which left no room for a 90° plug. Now a
16 mm riser under the tab holds the edge 20 mm up (connector face 18 mm
off the table, measured in the Fusion assembly), so the plug body hangs
below the board and the cable runs out under it toward the heel. The
riser is 2 mm thicker than the tab, so the board edge rests on that step
and the screw only clamps, it does not carry the board.

| | |
|---|---|
| Foot | 60 x 16 x 36 mm, base 4 mm, riser 6 mm thick x 16 high, tab 4 mm thick x 16 above the step, all leaning back 10° |
| Toe / heel | 40 mm ahead of the board face, 20 mm behind |
| Seat | board bottom edge rests on the riser step, 20 mm above the table |
| Hole | Ø2.6 mm through the tab, 3 mm above the step, centered on the width — M3 x 8 self-taps |
| Board end | sits 5 mm inside the foot's outer edge (hole is 3 mm in from the board end, foot is 16 wide) |

**Print:** 2 off, flat on the base, any PLA/PETG, no supports (the 10°
tab is well under the overhang limit). ~6 g each.

**Fit:** board bottom edge on the step, back face against the tab, one
M3 x 8 through each tab into the bottom corner hole from behind. Plug the
90° USB-C in pointing down; the cable exits under the board.

Files: `v0-demo-stand-foot.stl` / `.3mf` (mm), `foot-iso.png`,
`assy-side.png` (side view of the Fusion assembly, board lying face-up).
Fusion source: **HOT Projects / info-orbs-v0-demo-stand** (v3; parameters
BaseT, TabT, TabH, SeatH, StepD, Toe, Heel, FootW, Lean, HoleD, HoleUp
document the numbers and drive the extrude width and hole; the side
profile itself is drawn to numbers, not to those parameters — it is a
throwaway). Assembly with the board model:
**HOT Projects / info-orbs-v0-demo-stand-assy** (v3; the board model shows
the displays unsocketed, 8.5 mm lower than the real board, which does not
affect the feet).
