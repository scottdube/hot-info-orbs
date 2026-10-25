#!/usr/bin/env python3
"""Grow the board's TOP edge by 5 mm and carry the top-edge parts with it
(KiCad CLOSED, run once).

  .../python3 tools/grow_top.py        then   tools/route_xiao.py

Why (Scott, 2026-10-19, looking at the board in the Fusion enclosure): the
two top corner holes sat under the outer displays' rims (hole-to-display
centre 21.6 / 19.1 mm against a 19.05 mm module radius + 3 mm post), so the
enclosure had to hang those corners from its top lip instead of standing
posts on them. Another 5 mm of board above the displays puts the holes at
25.0 / 22.9 mm from the display centres, so all seven holes get posts, and
the edge-mounted switches ride up with the edge, 5 mm nearer the lip.

The board cannot shift as a whole: the display sockets are 2 mm from the
bottom edge. So only the top edge moves (y 62.0 -> 57.0, board 224.0 x
52.78 mm), and with it:
  - the three top-row holes (y 67.0 -> 62.0, still 5 mm in)
  - SW1-SW3 and their "button" labels (actuators stay past the edge)
  - the GND pour's top side (1 mm inside the edge)
Displays, XIAO, passives, logo and title block do not move. Tracks are
removed here; route_xiao.py starts from bare copper anyway.
Rev text X0.2 -> 2.1 (Scott set the number 2026-10-19; it read X0.3 for a few
minutes, never fabbed): a fab-returned board must be tellable from the 47.78
mm one.
"""
from pathlib import Path
import pcbnew

HERE = Path(__file__).resolve().parent.parent
PCB = HERE / "hot_info_orbs_xiao_46mm.kicad_pcb"
MM, T = pcbnew.FromMM, pcbnew.ToMM

GROW = 5.0
OLD_Y0, Y1 = 62.0, 109.78
Y0 = OLD_Y0 - GROW
REV_OLD, REV_NEW = "rev X0.2", "rev 2.1"

b = pcbnew.LoadBoard(str(PCB))
fps = {f.GetReference(): f for f in b.GetFootprints()}
moved = []

def up(item, label):
    item.Move(pcbnew.VECTOR2I(0, MM(-GROW)))
    moved.append((label, -GROW))

for i in (1, 2, 3):
    up(fps[f"SW{i}"], f"SW{i}")

for d in b.GetDrawings():
    L = d.GetLayerName()
    if L == "Edge.Cuts" and not isinstance(d, pcbnew.PCB_TEXT):
        if d.GetShape() == pcbnew.SHAPE_T_RECT:
            s, e = d.GetStart(), d.GetEnd()
            top = min(T(s.y), T(e.y))
            assert abs(top - OLD_Y0) < 0.01, top
            if T(s.y) < T(e.y):
                d.SetStart(pcbnew.VECTOR2I(s.x, MM(Y0)))
            else:
                d.SetEnd(pcbnew.VECTOR2I(e.x, MM(Y0)))
            moved.append(("edge top", Y0))
        elif d.GetShape() == pcbnew.SHAPE_T_CIRCLE and T(d.GetCenter().y) < 80:
            up(d, f"hole top x={T(d.GetCenter().x):.2f}")
    elif L == "B.Silkscreen" and isinstance(d, pcbnew.PCB_TEXT):
        if d.GetText().strip() == "button":
            up(d, "label button")
        if REV_OLD in d.GetText():
            d.SetText(d.GetText().replace(REV_OLD, REV_NEW))
            moved.append(("rev", d.GetText()))

for z in b.Zones():
    if z.GetIsRuleArea():
        continue
    o = z.Outline()
    xs = sorted(T(o.CVertex(i).x) for i in range(o.VertexCount()))
    x0, x1 = xs[0], xs[-1]
    o.RemoveAllContours()
    for x, y in ((x0, Y0 + 1), (x0, Y1 - 1), (x1, Y1 - 1), (x1, Y0 + 1)):
        z.AppendCorner(pcbnew.VECTOR2I(MM(x), MM(y)), -1)
    z.UnFill()
    moved.append(("pour top", Y0 + 1))

for tr in list(b.GetTracks()):
    b.Remove(tr)
moved.append(("tracks removed", "route_xiao.py next"))

pcbnew.SaveBoard(str(PCB), b)
for m in moved:
    print(*m)
