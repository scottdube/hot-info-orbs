#!/usr/bin/env python3
"""Rev 2.3: drop the XIAO 4 mm toward the bottom edge and pull the passives
in behind its 5V/GND pins (KiCad CLOSED, run once, then tools/route_xiao.py).

  .../python3 tools/lower_xiao.py        then   tools/route_xiao.py 20

Why (Scott, 2026-10-20): the USB-C jack should sit as close to the enclosure
wall as the board allows, without growing the board or putting the socket
pins too close to the edge. He eyeballed 2-4 mm; the numbers say 4 mm is
the top of that range and still inside every rule:

  - lowest XIAO socket pads 102.62 -> 106.62 (pad copper 2.31 mm from the
    edge; the display socket pads sit 2.95 mm from it; the rule is 0.5)
  - socket housing ends 1.89 mm inside the edge
  - module body ends 0.92 mm inside the edge; the USB-C jack's face, 1.5 mm
    proud of the body, lands 0.6 mm PAST the edge, at the wall
  - the footprint's own USB-C silk and the F.Silk "module here" rectangle
    would cross the edge: the rectangle is clipped 0.5 mm inside it, the
    footprint silk is left (the fab trims silk outside the outline)

The passives follow as far as they can. C1 (5V bulk) cannot drop with the
XIAO: DS3's back-side pin labels sit at y 101.9 and C1's courtyard was
already 0.4 mm above them on 2.1 (a 2.4 mm drop put its 5V pad on the "DC"
label: 3 silk warnings). So C1 keeps its y and moves 3.2 mm SIDEWAYS,
courtyard 0.5 mm from the XIAO socket's, giving up the 3 mm "castellation
pad" margin of 2026-10-17 (speculative; Scott's ask is the shortest 5V run).
Pad to pad the 5V run is 9.3 mm, the same as 2.1: the XIAO moved away from
C1 as much as C1 moved toward it. C2 and R1 swap: C2 (reset cap, its GND
beside C1's GND) stacks straight above C1 in the same column, R1 above that
with its RST pad adjacent to C2's RST pad, so the long run is the 3V3 feed
(no current) and not the reset node. Courtyard gaps 0.5 mm. The previous
two-column R1/C2 pair put the cap 15 mm up.

Tracks are removed here (route_xiao.py starts from bare copper anyway).
Rev text 2.1 -> 2.3 on the back silk and in the title block; 2.2 was never
made. The 2.3 STEP went into overall assembly v7 on 2026-10-20.
main_body_front v4 has no USB cutout yet: this docstring once said one was
"4 mm off", written before anyone looked at the enclosure timeline.
"""
from pathlib import Path
import pcbnew

HERE = Path(__file__).resolve().parent.parent
PCB = HERE / "hot_info_orbs_xiao_46mm.kicad_pcb"
MM, T = pcbnew.FromMM, pcbnew.ToMM
V = pcbnew.VECTOR2I

DROP = 4.0                 # XIAO
Y1 = 109.78                # bottom board edge
EDGE_CLR = 0.5             # min_copper_edge_clearance (.kicad_pro)
CRTYD_GAP = 0.5
REV_OLD, REV_NEW = "rev 2.1", "rev 2.3"

b = pcbnew.LoadBoard(str(PCB))
fps = {f.GetReference(): f for f in b.GetFootprints()}
U2, C1, C2, R1, DS3 = (fps[r] for r in ("U2", "C1", "C2", "R1", "DS3"))
log = []


def crtyd(f):
    bb = f.GetCourtyard(pcbnew.B_CrtYd).BBox()
    return T(bb.GetTop()), T(bb.GetBottom())


def place_bottom_at(f, x, bottom, rot=None):
    """Move f so its B courtyard bottom sits at `bottom` (mm), centred on x."""
    if rot is not None:
        f.SetOrientationDegrees(rot)
    f.SetPosition(V(MM(x), MM(0)))
    _, b0 = crtyd(f)                       # courtyard bottom with centre at y=0
    f.SetPosition(V(MM(x), MM(bottom - b0)))


# 1. XIAO down DROP, with the free-standing silk that marks it
U2.Move(V(0, MM(DROP)))
low = max(T(p.GetPosition().y) + T(p.GetSize().y) / 2 for p in U2.Pads())
assert low <= Y1 - EDGE_CLR, low
log.append(f"U2 at y {T(U2.GetPosition().y):.2f}; lowest pad copper {Y1 - low:.2f} mm from edge")

for d in b.GetDrawings():
    if isinstance(d, pcbnew.PCB_TEXT):
        if d.GetText().startswith("XIAO") and "SIDE" in d.GetText():
            d.Move(V(0, MM(DROP)))
            log.append(f"silk text '{d.GetText().split()[0]}...' on {d.GetLayerName()} down {DROP}")
    elif d.GetLayerName() == "F.Silkscreen" and d.GetShape() == pcbnew.SHAPE_T_RECT:
        s, e = d.GetStart(), d.GetEnd()
        if abs(T(s.x) - 121.1) < 0.01 and abs(T(e.x) - 141.1) < 0.01:
            d.Move(V(0, MM(DROP)))
            s, e = d.GetStart(), d.GetEnd()
            cap = MM(Y1 - EDGE_CLR)
            if s.y > cap: d.SetStart(V(s.x, cap))
            if e.y > cap: d.SetEnd(V(e.x, cap))
            log.append(f"F.Silk module rect now y {T(d.GetStart().y):.2f}..{T(d.GetEnd().y):.2f} (clipped {EDGE_CLR} inside edge)")

# 2. C1 sideways: courtyard CRTYD_GAP from the XIAO socket's, y unchanged
u2_right = T(U2.GetCourtyard(pcbnew.B_CrtYd).BBox().GetRight())
c1_bb = C1.GetCourtyard(pcbnew.B_CrtYd).BBox()
c1_half = (T(c1_bb.GetRight()) - T(c1_bb.GetLeft())) / 2
COL_X = round(u2_right + CRTYD_GAP + c1_half, 2)
C1.SetPosition(V(MM(COL_X), C1.GetPosition().y))
p5, p14 = C1.FindPadByNumber("1").GetPosition(), U2.FindPadByNumber("14").GetPosition()
log.append(f"C1 to x {COL_X} (was 147.22), y {T(C1.GetPosition().y):.2f} unchanged; "
           f"5V pad to XIAO 5V pad {((T(p5.x)-T(p14.x))**2 + (T(p5.y)-T(p14.y))**2)**0.5:.2f} mm")

# 3. C2 straight above C1 (GND pad lowest), R1 above C2 (RST pad lowest)
top, _ = crtyd(C1)
place_bottom_at(C2, COL_X, top - CRTYD_GAP, rot=-90)
pads = {p.GetNumber(): T(p.GetPosition().y) for p in C2.Pads()}
assert pads["2"] > pads["1"], pads          # pad 2 = GND is the lower one
log.append(f"C2 at y {T(C2.GetPosition().y):.2f}: RST pad {pads['1']:.2f}, GND pad {pads['2']:.2f}")

top, _ = crtyd(C2)
place_bottom_at(R1, COL_X, top - CRTYD_GAP, rot=-90)
pads = {p.GetNumber(): T(p.GetPosition().y) for p in R1.Pads()}
assert pads["2"] > pads["1"], pads          # pad 2 = RST is the lower one
log.append(f"R1 at y {T(R1.GetPosition().y):.2f}: 3V3 pad {pads['1']:.2f}, RST pad {pads['2']:.2f}")

# reference labels in one column to the right of the parts, like C1's
for f in (C1, C2, R1):
    t, bt = crtyd(f)
    f.Reference().SetPosition(V(MM(COL_X + 4.6), MM((t + bt) / 2)))
    f.Reference().SetTextAngleDegrees(0)

# 4. rev
for d in b.GetDrawings():
    if isinstance(d, pcbnew.PCB_TEXT) and REV_OLD in d.GetText():
        d.SetText(d.GetText().replace(REV_OLD, REV_NEW))
        log.append(f"rev silk: {d.GetText()}")
b.GetTitleBlock().SetRevision(REV_NEW.split()[1])

# 5. bare copper for the router. Zones BEFORE tracks: once tracks are
# removed, Zones() hands back bare SwigPyObjects (route_xiao.py hit the same).
for z in b.Zones():
    z.UnFill()
n = len(b.GetTracks())
for tr in list(b.GetTracks()):
    b.Remove(tr)
log.append(f"{n} tracks/vias removed, pour unfilled")

pcbnew.SaveBoard(str(PCB), b)
print("\n".join(log))
