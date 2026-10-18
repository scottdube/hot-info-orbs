#!/usr/bin/env python3
"""One-shot: replace the SuperMini antenna keepout with an FPC-antenna patch
(KiCad CLOSED). Run with KiCad's Python, then tools/route_xiao.py to re-route.

Why: the SuperMini has a PCB antenna at its end, so v1.1 kept copper away
from a 12 x 9.6 mm area above the module and wrote "Antenna" there. The XIAO
ESP32-S3 / S3 Plus has NO PCB antenna - a U.FL connector and a stick-on 2.4 GHz
FPC antenna (Seeed 318020968 "A-02", the one in inventory as part 102). That
antenna has to be stuck onto something with no copper under it, or it detunes;
the whole back of this board is a GND pour. So:

  - the two "Antenna Keep Out" rule areas and both "Antenna" silk texts go;
  - one new rule area ANTENNA_PATCH on F.Cu + B.Cu (no pour, tracks or vias)
    sits on the bottom strip right of the XIAO, x 143.5..183.5, y 99.5..107.0
    = 40 x 7.5 mm, on the back, clear of DS3's pin row (ends y 95.3) and SW1
    (starts x 187.4), with the XIAO's U.FL a few cm away;
  - a B.Silk outline + label says what the patch is for.

Why y starts at 99.5 and not 96.5: the bottom strip under DS3 is the SPI
bus's second route to DS4/DS5 and the switches. Measured against Freerouting
(20 passes, -mt 1), 2026-10-14, all at x 143.5..183.5:
    y 96.5..107 (10.5 mm)  3 nets unrouted
    y 98.0..107 ( 9.0 mm)  1 net unrouted (CS2)
    y 99.5..107 ( 7.5 mm)  0 unrouted, DRC clean      <- this
Narrower patches at the same heights (x 150.. or 155..) still left a net
unrouted; x 160.5..183.5 at 10.5 mm routes but gives only 23 mm of length.
4.2 mm of channel under the DS3 pins is what the router needs.

SIZE IS AN ASSUMPTION. Seeed publishes no dimensions for the A-02 (product
page, Pakronics, RS datasheet all checked 2026-10-14). 40 x 7.5 mm is sized
to the room, not to the part: measure an A-02 and its pigtail (5 at SLN
B3-R3C4, or the one in the XIAO box) and resize or move the patch before fab.
PATCH=x0,x1,y0,y1 in the environment overrides the rectangle for trials.
To re-run: git checkout the .kicad_pcb first (the script expects the two old
keepouts), then this script, then tools/route_xiao.py.
"""
import os
import pcbnew
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
PCB = HERE / "hot_info_orbs_xiao.kicad_pcb"
mm = pcbnew.FromMM
# PATCH=x0,x1,y0,y1 overrides, for trying sizes against the router
X0, X1, Y0, Y1 = map(float, os.environ.get("PATCH", "143.5,183.5,99.5,107.0").split(","))

b = pcbnew.LoadBoard(str(PCB))

# 1. old keepouts (take the list before touching anything - Zones() is fragile)
old = [z for z in b.Zones() if z.GetIsRuleArea() and "Antenna" in z.GetZoneName()]
assert len(old) == 2, [z.GetZoneName() for z in old]
for z in old:
    b.Remove(z)

# 2. old silk texts
for d in list(b.GetDrawings()):
    if d.GetClass() == "PCB_TEXT" and d.GetText().strip() == "Antenna":
        b.Remove(d)

# 3. new rule area, both copper layers
z = pcbnew.ZONE(b)
z.SetIsRuleArea(True)
z.SetZoneName("ANTENNA_PATCH")
ls = pcbnew.LSET(); ls.addLayer(pcbnew.F_Cu); ls.addLayer(pcbnew.B_Cu)
z.SetLayerSet(ls)
z.SetDoNotAllowZoneFills(True)
z.SetDoNotAllowTracks(True)
z.SetDoNotAllowVias(True)
z.SetDoNotAllowPads(True)
z.SetDoNotAllowFootprints(False)
# z.Outline() hands back a bare shared_ptr SwigPyObject (no NewOutline/Append);
# AppendCorner(pt, -1) builds the outline through the ZONE itself.
for x, y in ((X0, Y0), (X1, Y0), (X1, Y1), (X0, Y1)):
    z.AppendCorner(pcbnew.VECTOR2I(mm(x), mm(y)), -1)
b.Add(z)

# 4. silk on the side the antenna is stuck to (B = XIAO side)
r = pcbnew.PCB_SHAPE(b, pcbnew.SHAPE_T_RECT)
r.SetLayer(pcbnew.B_SilkS)
r.SetStart(pcbnew.VECTOR2I(mm(X0), mm(Y0))); r.SetEnd(pcbnew.VECTOR2I(mm(X1), mm(Y1)))
r.SetWidth(mm(0.15))
b.Add(r)
t = pcbnew.PCB_TEXT(b)
t.SetLayer(pcbnew.B_SilkS)
t.SetText("STICK FPC ANTENNA HERE\nno copper under this box")
t.SetTextSize(pcbnew.VECTOR2I(mm(1.0), mm(1.0))); t.SetTextThickness(mm(0.15))
t.SetMirrored(True)
t.SetHorizJustify(pcbnew.GR_TEXT_H_ALIGN_CENTER); t.SetVertJustify(pcbnew.GR_TEXT_V_ALIGN_CENTER)
t.SetPosition(pcbnew.VECTOR2I(mm((X0 + X1) / 2), mm((Y0 + Y1) / 2)))
b.Add(t)

pcbnew.SaveBoard(str(PCB), b)
print("saved: antenna patch %.1f x %.1f mm at x %.1f..%.1f y %.1f..%.1f" % (X1 - X0, Y1 - Y0, X0, X1, Y0, Y1))
