#!/usr/bin/env python3
"""Physical rework of the XIAO board, 2026-10-16 (Scott). Placement only:
strips every track and via and unfills the pour, because the board is
rerouted AFTER Scott has checked the 3D. KiCad CLOSED. KiCad's Python:

  .../python3 tools/physical_rework.py

Runs on the committed 1d4623f board (routed, with the antenna patch). What
Scott asked for, and how each was read:

1. No antenna exclusion: the FPC antenna will not be stuck on this board.
   Rule area ANTENNA_PATCH, its B.Silk box and label go; tools/antenna_patch.py
   is retired.
2. Displays as far down the board as possible, on pin headers plugged into a
   female socket. New footprint _Displays:GC9A01_SOCKET: same 7 pads as
   GC9A01_PINS, plus KiCad's PinSocket_1x07_P2.54mm_Vertical model, and the
   display lifted 8.5 mm (socket body). Pad row at y = PIN_ROW_Y: the lowest
   it goes with the 1.7 mm pads 0.5 mm (min_copper_edge_clearance) inside the
   edge and the socket body still on the board.
3. Buttons on the TOP edge (Scott corrected "bottom" to top; see 5.), right-angle,
   spaced equally across the centre third of the board. The part is uxcell's
   6 x 6 x 8 mm right-angle tact switch (Amazon); footprint is KiCad's stock
   PTS645Vx39 (C&K PTS645VL39, 8.05 mm overall from body back to actuator tip
   - the closest of the four PTS645 right-angle variants to "8 mm"). The
   listing gives "pin pitch 6.5 mm, 4.5 mm"; the footprint has 4.5 mm between
   the switch pins and 7.0 mm between the mounting tabs, so the tab pitch is
   UNVERIFIED until one is measured. Equal spacing = each switch centred in
   its own third of the centre third (centres at 1/6, 1/2, 5/6 of it). Body
   front face on the board edge, so the actuator stands 1.26 mm proud.
4. C1, C2, R1 to the bottom edge, in the gaps between display sockets:
   R1 between DS1 and DS2, C2 + C1 between DS3 and DS4. Not between DS2 and
   DS3: the XIAO sits there on the back and THT leads would hit it.
5. (Later 2026-10-16, Scott) "bottom of the board" meant the SIDE: C1, C2,
   R1 and SW1-3 all go on the back, "the same side as the xiao". The
   switches stay on the top edge; C1/C2/R1 keep their bottom-edge spots
   (the XIAO is not there). Flipped left-right, so a switch's pad 1 lands
   CX left of its centre instead of right.
6. Top board edge brought up even with the tops of the displays:
   DISPLAY_TOP_Y, measured off an orthographic kicad-cli top render of the
   placed board against a bare-board render at the same camera (19.4 px/mm,
   tallest display at y 61.99). The two top mounting holes and the B.Cu GND
   pour's top corners move up by the same amount; the holes keep their 3 mm
   corner inset.

Footprint swaps keep the symbol path so parity still matches, and the
schematic's Footprint fields are rewritten to the new names at the end.
"""
from pathlib import Path
import pcbnew

HERE = Path(__file__).resolve().parent.parent
PCB = HERE / "hot_info_orbs_xiao.kicad_pcb"
STOCK = Path("/Applications/KiCad/KiCad.app/Contents/SharedSupport/footprints")
MM = pcbnew.FromMM

b = pcbnew.LoadBoard(str(PCB))
edge = b.GetBoardEdgesBoundingBox()
X0, X1 = pcbnew.ToMM(edge.GetLeft()), pcbnew.ToMM(edge.GetRight())
Y0, Y1 = pcbnew.ToMM(edge.GetTop()), pcbnew.ToMM(edge.GetBottom())
DISPLAY_TOP_Y = 62.0   # render-measured 61.99, see 6. above
RISE = Y0 - DISPLAY_TOP_Y

# TRAP: after the first b.Remove() in a process, SWIG stops typing return
# values - b.Zones()/GetDrawings()/GetTracks() AND methods on handles taken
# earlier (fp.GetPosition()) come back as bare SwigPyObjects. HANDOFF had
# only seen it on Zones(). So: read and move everything first, Add the new
# footprints, and do every Remove last, just before Save.
zones = list(b.Zones())
drawings = list(b.GetDrawings())
tracks = list(b.GetTracks())
fps = {f.GetReference(): f for f in b.GetFootprints()}
doomed = []

# --- 1. antenna patch -------------------------------------------------------
for z in zones:
    if z.GetIsRuleArea() and z.GetZoneName() == "ANTENNA_PATCH":
        doomed.append(z)
    else:
        z.UnFill()  # the pour is refilled after the reroute
for d in drawings:
    if d.GetLayerName() != "B.Silkscreen":
        continue
    bb = d.GetBoundingBox()
    if (143.0 <= pcbnew.ToMM(bb.GetLeft()) and pcbnew.ToMM(bb.GetRight()) <= 184.0
            and 99.0 <= pcbnew.ToMM(bb.GetTop()) and pcbnew.ToMM(bb.GetBottom()) <= 107.5):
        doomed.append(d)

# Leftover F.Silk frames: the SuperMini's antenna box (v1.1, never meaningful
# on the XIAO, now under SW1) and the three frames round the old switches.
STALE_RECTS = [(125.27, 82.97), (182.995, 99.23), (197.015, 99.23), (210.985, 99.2)]
for d in drawings:
    if d.GetLayerName() == "F.Silkscreen" and not isinstance(d, pcbnew.PCB_TEXT):
        s = d.GetStart()
        if any(abs(pcbnew.ToMM(s.x) - x) < 0.01 and abs(pcbnew.ToMM(s.y) - y) < 0.01
               for x, y in STALE_RECTS):
            doomed.append(d)

# --- 6. top edge up to the display tops ---------------------------------------
for d in drawings:
    if d.GetLayerName() != "Edge.Cuts":
        continue
    if d.GetShape() == pcbnew.SHAPE_T_RECT:
        s = d.GetStart()
        d.SetStart(pcbnew.VECTOR2I(s.x, MM(DISPLAY_TOP_Y)))
    elif d.GetShape() == pcbnew.SHAPE_T_CIRCLE and pcbnew.ToMM(d.GetCenter().y) < (Y0 + Y1) / 2:
        d.Move(pcbnew.VECTOR2I(0, -MM(RISE)))
for z in zones:
    if z.GetIsRuleArea():
        continue
    # ZONE has no SetCornerPosition: rebuild the single outline instead
    pts = [z.GetCornerPosition(i) for i in range(z.GetNumCorners())]
    pts = [pcbnew.VECTOR2I(c.x, c.y - MM(RISE)) if pcbnew.ToMM(c.y) < (Y0 + Y1) / 2
           else pcbnew.VECTOR2I(c.x, c.y) for c in pts]
    z.RemoveAllContours()
    for c in pts:
        z.AppendCorner(c, -1)
Y0 = DISPLAY_TOP_Y

# --- tracks out: reroute comes after the 3D check ---------------------------
doomed += tracks

def swap(old, lib, name, pos, rot_deg, back=False):
    """Add lib:name in place of `old`, keeping identity and nets; old is
    queued for removal."""
    new = pcbnew.FootprintLoad(str(lib), name)
    new.SetParent(b)
    new.SetPath(old.GetPath())
    new.SetSheetname(old.GetSheetname())
    new.SetSheetfile(old.GetSheetfile())
    new.SetReference(old.GetReference())
    new.SetValue(old.GetValue())
    for fid in (pcbnew.FIELD_T_DATASHEET, pcbnew.FIELD_T_DESCRIPTION):
        # GetField().SetText, never SetField: that makes a stray user field "5"
        new.GetField(fid).SetText(old.GetField(fid).GetText())
    new.SetFPIDAsString(f"{Path(str(lib)).stem}:{name}")
    nets = {p.GetNumber(): p.GetNet() for p in old.Pads()}
    for p in new.Pads():
        if p.GetNumber() in nets:
            p.SetNet(nets[p.GetNumber()])
    new.SetOrientationDegrees(rot_deg)
    new.SetPosition(pos)
    if back:
        new.Flip(pos, pcbnew.FLIP_DIRECTION_LEFT_RIGHT)
    b.Add(new)
    doomed.append(old)
    return new

# --- 2. displays on sockets, pad row low --------------------------------------
PAD_R = 0.85
PIN_ROW_Y = round(Y1 - 0.5 - PAD_R - 0.45, 2)  # 0.45 mm margin over the rule
ds_pads = {}
for ref in ("DS1", "DS2", "DS3", "DS4", "DS5"):
    old = fps[ref]
    x = old.GetPosition().x
    ds_pads[ref] = [pcbnew.ToMM(p.GetPosition().x) for p in old.Pads()]
    # footprint origin is 3.48 mm above the pad row once rotated 180
    swap(old, HERE / "lib/_Displays.pretty", "GC9A01_SOCKET",
         pcbnew.VECTOR2I(x, MM(PIN_ROW_Y - 3.48)), old.GetOrientationDegrees())

# --- 3. right-angle switches on the top edge, centre third --------------------
SW = "SW_Tactile_SPST_Angled_PTS645Vx39-2LFS"
BODY_FRONT = 2.59      # body front face is 2.59 mm above pad 1 (footprint F.Fab)
CX = 2.25              # switch centre is 2.25 mm right of pad 1
third = (X1 - X0) / 3
c0 = X0 + third
centres = [c0 + third * k / 6 for k in (1, 3, 5)]
sw_y = Y0 + BODY_FRONT
for ref, cx in zip(("SW1", "SW2", "SW3"), centres):
    # rotation 0: actuator toward -y = the top edge. Flipped to the back the
    # footprint mirrors in x, so its centre is CX LEFT of the origin
    swap(fps[ref], STOCK / "Button_Switch_THT.pretty", SW,
         pcbnew.VECTOR2I(MM(cx + CX), MM(sw_y)), 0, back=True)

def place_box(d, left, top):
    """Put a text's bounding box at (left, top). Board texts here carry mixed
    justifications (centred, bottom-anchored multi-line), so SetPosition on
    the anchor lands them anywhere - move by the bbox delta instead."""
    bb = d.GetBoundingBox()
    dx = MM(left) - bb.GetLeft()
    dy = MM(top) - bb.GetTop()
    p = d.GetPosition()
    d.SetPosition(pcbnew.VECTOR2I(p.x + dx, p.y + dy))

# "button" silk labels follow their switch, just below the body
labels = sorted((d for d in drawings
                 if isinstance(d, pcbnew.PCB_TEXT) and d.GetText().strip() == "button"),
                key=lambda d: d.GetPosition().x)
for d, cx in zip(labels, centres):
    w = pcbnew.ToMM(d.GetBoundingBox().GetWidth())
    d.SetLayer(pcbnew.B_SilkS)   # labels follow the switches to the back
    d.SetMirrored(True)
    place_box(d, cx - w / 2, sw_y + 2.49 + 1.05 + 0.4)  # below the tab pads

# --- 4. R1, C2, C1 to the bottom edge -----------------------------------------
g12 = (max(ds_pads["DS1"]) + min(ds_pads["DS2"])) / 2
g34 = (max(ds_pads["DS3"]) + min(ds_pads["DS4"])) / 2
ROW = PIN_ROW_Y  # axial / disc parts' pad row level with the sockets'
fps["R1"].SetPosition(pcbnew.VECTOR2I(MM(g12 - 10.16 / 2), MM(ROW)))
fps["C2"].SetPosition(pcbnew.VECTOR2I(MM(g34 - 7.0), MM(ROW)))
# C1 (radial, rotated 180: pad 1 right, pad 2 2.5 mm left); bottom of its
# 6.3 mm body + silk sits on the same 0.5 mm edge margin
fps["C1"].SetPosition(pcbnew.VECTOR2I(MM(g34 + 7.0), MM(Y1 - 0.5 - 3.42)))
# its reference sat below the body, off the board: put it above
fps["C1"].Reference().SetPosition(pcbnew.VECTOR2I(MM(g34 + 7.0 - 1.25), MM(Y1 - 0.5 - 6.84 - 1.2)))
for ref in ("R1", "C2", "C1"):   # 5.: on the XIAO's side
    # flipping mirrors about the origin = pad 1, which swung R1 into DS1's
    # socket pads; put the pad midpoint back where it was
    f = fps[ref]
    mid = lambda: sum(pd.GetPosition().x for pd in f.Pads()) // len(list(f.Pads()))
    before = mid()
    f.Flip(f.GetPosition(), pcbnew.FLIP_DIRECTION_LEFT_RIGHT)
    f.Move(pcbnew.VECTOR2I(before - mid(), 0))

# --- board texts that now sit under a socket move to the top band -------------
for d in drawings:
    if not isinstance(d, pcbnew.PCB_TEXT) or d.GetLayerName() != "F.Silkscreen":
        continue
    txt = d.GetText()
    if txt.startswith("The Villages"):
        place_box(d, X0 + 7.0, Y0 + 1.5)
    elif txt.startswith("HoT Info Orbs"):
        w = pcbnew.ToMM(d.GetBoundingBox().GetWidth())
        place_box(d, X1 - 7.0 - w, Y0 + 1.5)

summary = (f"pin row y {PIN_ROW_Y}, switch pad-1 row y {sw_y:.3f}, "
           f"centres {[round(c, 2) for c in centres]}, gaps {g12:.2f} {g34:.2f}, "
           f"removing {len(doomed)} items")
for item in doomed:
    b.Remove(item)
b.Save(str(PCB))

SCH = HERE / "hot_info_orbs_xiao.kicad_sch"
s = SCH.read_text()
s = s.replace('"Footprint" "_Displays:GC9A01_PINS"', '"Footprint" "_Displays:GC9A01_SOCKET"')
s = s.replace('"Footprint" "Button_Switch_THT:SW_PUSH_6mm_H5mm"',
              f'"Footprint" "Button_Switch_THT:{SW}"')
SCH.write_text(s)
print(summary)
