#!/usr/bin/env python3
"""Spread the five displays to a 46.0 mm pitch (KiCad CLOSED, run once).

  .../python3 tools/pitch_46.py        then   tools/route_xiao.py

Why: the enclosure front (Fusion "main_body_front", 2026-10-19) puts the
bezel centres at x = 32.75 + n*46.0 (R-pattern uDistance 92 over four gaps),
and the carrier it grew from has them ~38.98 mm apart. Nothing electrical
changes, so the schematic is untouched; this is a placement edit on the copy
of the XIAO carrier, followed by a fresh autoroute.

Placement rules kept from physical_rework.py (same numbers, re-derived):
  - DS3 stays at x 149.04 and becomes the board centre; 20 mm from DS1/DS5
    centre to the edge (was 19.65 / 23.23) -> board 224.0 mm wide.
  - corner holes 5 mm in, mid holes at centre +-19.5 (top row + one bottom).
  - switches at thirds of the width, their "button" labels follow.
  - back-silk title block keeps its distance from the low-x edge, the logo
    and club text keep theirs from the high-x edge; the XIAO group (U2, C1,
    R1, C2, both XIAO notes, the F.Silk outline) does not move.
  - GND pour re-drawn 1 mm inside the new outline.
Rev text X0.1 -> X0.2 so a fab-returned board can be told from the narrow one.

pcbnew SWIG: everything here is Move()/SetText() on live objects, no Remove(),
so the untyped-return trap does not bite. Zone outline: RemoveAllContours +
AppendCorner (no SetCornerPosition in 10.0).
"""
from pathlib import Path
import pcbnew

HERE = Path(__file__).resolve().parent.parent
PCB = HERE / "hot_info_orbs_xiao_46mm.kicad_pcb"
MM, T = pcbnew.FromMM, pcbnew.ToMM

PITCH, MARGIN, CX = 46.0, 20.0, 149.04
X0, X1 = CX - 2 * PITCH - MARGIN, CX + 2 * PITCH + MARGIN     # 37.04 .. 261.04
OLD_X0, OLD_X1, OLD_CX = 51.366, 250.184, (51.366 + 250.184) / 2
dX0, dX1, dCX = X0 - OLD_X0, X1 - OLD_X1, CX - OLD_CX
HOLE_IN, MID = 5.0, 19.5
SW_CX = 2.25                      # switch origin sits 2.25 mm right of its centre
REV_OLD, REV_NEW = "rev X0.1", "rev X0.2 · 46 mm pitch"

b = pcbnew.LoadBoard(str(PCB))
fps = {f.GetReference(): f for f in b.GetFootprints()}
moved = []

def mv(item, dx, label):
    item.Move(pcbnew.VECTOR2I(MM(dx), 0))
    moved.append((label, round(dx, 3)))

# displays
for i in range(5):
    f = fps[f"DS{i + 1}"]
    target = CX + (i - 2) * PITCH
    mv(f, target - T(f.GetPosition().x), f"DS{i + 1} -> {target:.2f}")

# switches at thirds + their labels (labels sorted by x, like the switches)
third = (X1 - X0) / 3
centres = [X0 + third, X0 + 1.5 * third, X0 + 2 * third]
labels = sorted((d for d in b.GetDrawings()
                 if isinstance(d, pcbnew.PCB_TEXT) and d.GetText().strip() == "button"),
                key=lambda d: d.GetPosition().x)
for i, (cx, lab) in enumerate(zip(centres, labels)):
    f = fps[f"SW{i + 1}"]
    dx = (cx + SW_CX) - T(f.GetPosition().x)
    mv(f, dx, f"SW{i + 1} -> centre {cx:.2f}")
    mv(lab, dx, f"label SW{i + 1}")

# edge, holes, silk
for d in b.GetDrawings():
    L = d.GetLayerName()
    if L == "Edge.Cuts" and not isinstance(d, pcbnew.PCB_TEXT):
        if d.GetShape() == pcbnew.SHAPE_T_RECT:
            s, e = d.GetStart(), d.GetEnd()
            d.SetStart(pcbnew.VECTOR2I(MM(X0), s.y))
            d.SetEnd(pcbnew.VECTOR2I(MM(X1), e.y))
            moved.append(("edge rect", (X0, X1)))
        elif d.GetShape() == pcbnew.SHAPE_T_CIRCLE:
            x = T(d.GetCenter().x)
            if x < OLD_X0 + 10:
                mv(d, (X0 + HOLE_IN) - x, "hole L")
            elif x > OLD_X1 - 10:
                mv(d, (X1 - HOLE_IN) - x, "hole R")
            elif x < OLD_CX:
                mv(d, (CX - MID) - x, "hole mid-L")
            else:
                mv(d, (CX + MID) - x, "hole mid-R")
    elif L == "B.Silkscreen":
        if isinstance(d, pcbnew.PCB_TEXT) and (d.GetText().strip() == "button" or "XIAO" in d.GetText() and "SIDE" in d.GetText()):
            continue
        bb = d.GetBoundingBox()
        mid = T(bb.GetLeft() + bb.GetRight()) / 2
        if mid < 120:
            mv(d, dX0, f"B.Silk left {d.GetText()[:20]!r}" if isinstance(d, pcbnew.PCB_TEXT) else "B.Silk left rule")
        elif mid > 180:
            mv(d, dX1, f"B.Silk right {d.GetText()[:20]!r}")
        if isinstance(d, pcbnew.PCB_TEXT) and REV_OLD in d.GetText():
            d.SetText(d.GetText().replace(REV_OLD, REV_NEW))
            moved.append(("rev", REV_NEW))

mv(fps["G***"], dX1, "logo")

# pour: 1 mm inside the outline
for z in b.Zones():
    if z.GetIsRuleArea():
        continue
    y0, y1 = 62.0 + 1.0, 109.78 - 1.0
    z.Outline().RemoveAllContours()
    for x, y in ((X0 + 1, y0), (X0 + 1, y1), (X1 - 1, y1), (X1 - 1, y0)):
        z.AppendCorner(pcbnew.VECTOR2I(MM(x), MM(y)), -1)
    z.UnFill()
    moved.append(("pour", z.GetNetname()))

pcbnew.SaveBoard(str(PCB), b)
for m in moved:
    print(*m)
