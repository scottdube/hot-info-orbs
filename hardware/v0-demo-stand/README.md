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

**Why screws and not a slot:** a 10° recline moves the CG back only ~2.5 mm
of the ~8 mm it sits forward; a screwless slot stand would need ~30° of
recline plus a front lip, and the displays come within 1.5 mm of the
board's bottom edge, so there is no room for a lip under them anyway.

**Why the board sits 16 mm up:** the SuperMini's USB-C is on the back of
the board at mid-length and points at the bottom edge. With a 90° plug in,
the bottom of the connector is 13 mm below the board edge (measured
2026-10-21). The riser under the tab holds the edge 16 mm off the table,
3 mm clear, and the cable runs out under the board toward the heel. The
riser is 2 mm thicker than the tab, so the board edge rests on that step
and the screws only clamp, they do not carry the board.

**Why 6 mm wide and flush (v3):** the foot's outer face is flush with the
board end and the tab runs to the board's top edge, so the stand reads as
part of the board rather than a clamp on it. The corner holes are 3 mm in
from the end, so a single symmetric part that is flush must be 6 mm wide.
A wider foot would need a mirrored left/right pair; rejected for a
throwaway. The nearest display socket tails start 6 mm inboard of the foot.

| | |
|---|---|
| Foot | 60 x 6 x 44 mm: base 4 mm, riser 6 thick x 12 high, tab 4 thick to the board's top edge, all leaning back 10° |
| Toe / heel | 40 mm ahead of the board face, 20 mm behind |
| Seat | board bottom edge rests on the riser step, 16 mm above the table |
| Holes | two Ø2.6 mm through the tab, 3 and 25.5 mm above the step measured along the face, centered on the width — M3 x 8 self-taps |
| Board end | flush with the foot's outer face |

**Print:** 2 off, flat on the base, any PLA/PETG, no supports (the 10°
tab is well under the overhang limit). ~5 g each.

**Fit:** board bottom edge on the step, back face against the tab, two
M3 x 8 through each tab into the corner holes from behind. Plug the
90° USB-C in pointing down; the cable exits under the board.

Files: `v0-demo-stand-foot.stl` / `.3mf` (mm), `foot-iso.png`.
Fusion source: **HOT Projects / info-orbs-v0-demo-stand** (v7, a clean
rebuild; parameters BaseT, TabT, TabH, SeatH, StepD, Toe, Heel, FootW,
Lean, HoleD, HoleUp document the numbers and drive the extrude width and
hole diameter; the side profile and hole points are drawn to numbers, not
to those parameters — it is a throwaway). Assembly with the board model:
**HOT Projects / info-orbs-v0-demo-stand-assy** (v9; all four foot holes
verified on the board's corner holes; the board model shows the displays
unsocketed, 8.5 mm lower than the real board, which does not affect the
feet).

**Trap (2026-10-21):** editing the foot in place from the assembly while
an MCP session was also driving Fusion lost the edits: every MCP call
switches the active document tab, which cancels an open dialog. Finish
and save hand edits before any agent call, or edit in the part document
with the agent idle.
