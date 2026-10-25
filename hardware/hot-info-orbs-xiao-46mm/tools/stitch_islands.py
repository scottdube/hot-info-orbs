#!/usr/bin/env python3
"""Rev 2.3 follow-up: GND pour on the front too, and one stitching via in
each large back-pour island (KiCad CLOSED, run once, then the DRC/refill
below).

  .../python3 tools/stitch_islands.py
  kicad-cli pcb drc --schematic-parity --severity-all --refill-zones --save-board ...

Why (Scott, 2026-10-20): the back-side GND pour showed two wide bare bands,
one from SW2 down past R1 to the XIAO, one from DS1 to DS2 along the bottom
edge. Measured with the fill redone in memory and island removal OFF: they
are removed islands, 283 mm2 (x 124-146, y 62-107, boxed in by the SW2/SW3
pair and the 3V3 feed to R1) and 211 mm2 (x 61-104, y 99-105, between the
DS1 CS1/SCLK/MOSI bundle and the shared RST track along the edge). Tracks
close a loop around copper with no GND pad inside it, so the "remove
islands: always" setting deletes it, correctly: floating copper beside SPI
lines is worse than none. A via to the GND net inside each loop gives the
copper a connection and the fill comes back.

Tried first with the vias alone: KiCad still removed both islands and DRC
called the vias dangling. Correct again: the board had no front pour, so a
via out of an island landed on bare laminate and the island plus via was
still floating. A stitching via needs copper on the other side. So the GND
zone now covers F.Cu as well as B.Cu (one zone, two layers), the front pour
reaches the display sockets' GND pins, and the vias tie the two back
islands to it. Rejected the alternative of a front-side track from each via
to a socket GND pin: it would have to skirt the socket pin rows, and a front
pour is the normal thing on a two-layer board anyway.

The via goes at the point inside the island farthest from every track and
pad, so it adds no clearance violation; size and drill copy the GND via
Freerouting already placed (0.8/0.3, the GND net class; signal vias are
0.6/0.3). Islands under 100 mm2 are
left alone: they are slivers between socket pins, not pour.
"""
from pathlib import Path
import pcbnew, math

HERE = Path(__file__).resolve().parent.parent
PCB = HERE / "hot_info_orbs_xiao_46mm.kicad_pcb"
MM, T, V = pcbnew.FromMM, pcbnew.ToMM, pcbnew.VECTOR2I
MIN_ISLAND = 100.0          # mm2

b = pcbnew.LoadBoard(str(PCB))
z = list(b.Zones())[0]
assert z.GetNetname() == "GND", z.GetNetname()
L = pcbnew.B_Cu
gnd = b.FindNet("GND")
# the net class comes back as a bare SwigPyObject (no GetViaDiameter); copy
# the size of the vias already on the board instead, all 0.6/0.3
ref_via = next(t for t in b.GetTracks() if t.GetClass() == "PCB_VIA" and t.GetNetname() == "GND")
via_d, via_drill = T(ref_via.GetWidth(pcbnew.B_Cu)), T(ref_via.GetDrillValue())

# fill with islands kept, in memory only
z.SetIslandRemovalMode(pcbnew.ISLAND_REMOVAL_MODE_NEVER)
pcbnew.ZONE_FILLER(b).Fill(b.Zones())
poly = z.GetFilledPolysList(L)
big = [i for i in range(poly.OutlineCount()) if abs(poly.Outline(i).Area()) / 1e12 >= MIN_ISLAND
       and abs(poly.Outline(i).Area()) / 1e12 < 2000]      # the main pour is >2000
assert len(big) == 2, big

obstacles = [(T(t.GetStart().x), T(t.GetStart().y), T(t.GetEnd().x), T(t.GetEnd().y)) for t in b.GetTracks()]
pads = [(T(p.GetPosition().x), T(p.GetPosition().y), max(T(p.GetSize().x), T(p.GetSize().y)) / 2)
        for f in b.GetFootprints() for p in f.Pads()]

def seg_dist(px, py, x0, y0, x1, y1):
    dx, dy = x1 - x0, y1 - y0
    L2 = dx * dx + dy * dy
    u = 0 if L2 == 0 else max(0, min(1, ((px - x0) * dx + (py - y0) * dy) / L2))
    return math.hypot(px - (x0 + u * dx), py - (y0 + u * dy))

def clearance_at(px, py):
    d = min(seg_dist(px, py, *o) for o in obstacles)
    return min(d, min(math.hypot(px - x, py - y) - r for x, y, r in pads))

log = []
for i in big:
    o = poly.Outline(i)
    bb = o.BBox()
    best = None
    x = T(bb.GetLeft())
    while x <= T(bb.GetRight()):
        y = T(bb.GetTop())
        while y <= T(bb.GetBottom()):
            if o.PointInside(V(MM(x), MM(y))):
                c = clearance_at(x, y)
                if best is None or c > best[0]:
                    best = (c, x, y)
            y += 0.25
        x += 0.25
    c, x, y = best
    via = pcbnew.PCB_VIA(b)
    via.SetPosition(V(MM(x), MM(y)))
    via.SetViaType(pcbnew.VIATYPE_THROUGH)
    via.SetWidth(MM(via_d))
    via.SetDrill(MM(via_drill))
    via.SetNet(gnd)
    b.Add(via)
    log.append(f"island {abs(o.Area())/1e12:.0f} mm2 (x {T(bb.GetLeft()):.1f}..{T(bb.GetRight()):.1f}, "
               f"y {T(bb.GetTop()):.1f}..{T(bb.GetBottom()):.1f}): GND via {via_d}/{via_drill} at ({x:.2f}, {y:.2f}), "
               f"{c:.2f} mm from the nearest track/pad edge")

# DS5 pad 2: diagonal thermal spokes, see the docstring
p = b.FindFootprintByReference("DS5").FindPadByNumber("2")
p.SetThermalSpokeAngle(pcbnew.EDA_ANGLE(45, pcbnew.DEGREES_T))
log.append("DS5 pad 2 thermal spoke angle 45 deg")

# front pour too, then back to the real setting; kicad-cli --refill-zones fills
ls = pcbnew.LSET()
ls.AddLayer(pcbnew.F_Cu)
ls.AddLayer(pcbnew.B_Cu)
z.SetLayerSet(ls)
log.append("GND zone now on " + ", ".join(b.GetLayerName(l) for l in z.GetLayerSet().Seq()))
z.SetIslandRemovalMode(pcbnew.ISLAND_REMOVAL_MODE_ALWAYS)
z.UnFill()
pcbnew.SaveBoard(str(PCB), b)
print("\n".join(log))
