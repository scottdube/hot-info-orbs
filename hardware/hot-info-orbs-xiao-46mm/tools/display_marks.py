#!/usr/bin/env python3
"""Rev 2.4: a front-silk circle at each display position with the word
Display in its center, and the rev bump 2.3 -> 2.4 (KiCad CLOSED, run once).

  .../python3 tools/display_marks.py

Why (Scott, 2026-10-20): "add circles indicating the display goes on that
side, in the middle put Display". The front of a bare board shows five
7-pin socket rows along the bottom edge and nothing else; with the XIAO
marked "GOES ON OTHER SIDE" on the front, a first-time builder has no cue
which face the displays plug into or where they sit. Now each display's
footprint is drawn where the module will be.

Where: F.Silkscreen, one circle per DSn at the module's 3D-model origin as
placed by the footprint (x = fp.x + 1.7, y = fp.y - 21.5 for the 180-degree
socket: 58.74 + n*46, y 81.0). Radius 14.75 mm (Ø29.5), inside the lit area
(Ø32.5, docs/DISPLAY-EDGE-TEST.md). Not the module rim (r 19.05): at DS5 the
rim lands 0.7 mm PAST the right board edge. Not the lit area (r 16.25)
either, which was the first try: at DS3 that circle crosses the front
annular rings of C1 pad 2 (16.1..17.7 from the center) and XIAO pad 8
(15.0..16.7), two DRC "clipped by solder mask" warnings, and the fab would
cut two gaps into the ring. 14.75 is the largest radius that clears both
with a quarter millimetre to spare. Text "Display" 2.0 mm / 0.3 thick at
the center, over mask-covered F.Cu pour only (no via within 9 mm of any
center). Line 0.2 mm, above JLCPCB's 0.15 minimum.

Known overlap: the DS3 circle crosses the F.Silk "XIAO GOES ON OTHER SIDE"
rectangle (x 121.05..141.15, y 88.45..109.33); the DS2 circle ends 1.6 mm
short of it. That is the truth of the layout (display 3 sits over the back
of the XIAO), and silk-over-silk is not a DRC item, so both stay.

The enclosure's bezel centres (docs/ENCLOSURE.md, board y 80.665) sit
0.34 mm above the model origin used here; the circle is a marker, not a
datum.
"""
from pathlib import Path
import pcbnew

HERE = Path(__file__).resolve().parent.parent
PCB = HERE / "hot_info_orbs_xiao_46mm.kicad_pcb"
MM, T = pcbnew.FromMM, pcbnew.ToMM
V = pcbnew.VECTOR2I

R = 14.75               # see docstring: largest radius clear of the C1/U2 front annular rings
LINE = 0.2
SIZE, THICK = 2.0, 0.3
LABEL = "Display"
REV_OLD, REV_NEW = "rev 2.3", "rev 2.4"
X0, X1 = 37.015, 261.065  # board outline
EDGE_CLR = 0.5

b = pcbnew.LoadBoard(str(PCB))
log = []
for f in sorted(b.GetFootprints(), key=lambda f: f.GetReference()):
    if not f.GetReference().startswith("DS"):
        continue
    assert abs(f.GetOrientationDegrees()) == 180.0, f.GetOrientationDegrees()
    m = f.Models()[0]
    assert "GC9A01" in m.m_Filename, m.m_Filename
    # 180-degree footprint: local offset (-1.7, -21.5) -> board (+1.7, -21.5) with KiCad's up-positive model y
    cx = T(f.GetPosition().x) - m.m_Offset.x
    cy = T(f.GetPosition().y) + m.m_Offset.y
    assert cx - R >= X0 + EDGE_CLR and cx + R <= X1 - EDGE_CLR, (f.GetReference(), cx)

    c = pcbnew.PCB_SHAPE(b, pcbnew.SHAPE_T_CIRCLE)
    c.SetLayer(pcbnew.F_SilkS)
    c.SetCenter(V(MM(cx), MM(cy)))
    c.SetEnd(V(MM(cx + R), MM(cy)))
    c.SetWidth(MM(LINE))
    c.SetFilled(False)
    b.Add(c)

    t = pcbnew.PCB_TEXT(b)
    t.SetLayer(pcbnew.F_SilkS)
    t.SetText(LABEL)
    t.SetTextSize(V(MM(SIZE), MM(SIZE)))
    t.SetTextThickness(MM(THICK))
    t.SetPosition(V(MM(cx), MM(cy)))
    t.SetHorizJustify(pcbnew.GR_TEXT_H_ALIGN_CENTER)
    t.SetVertJustify(pcbnew.GR_TEXT_V_ALIGN_CENTER)
    b.Add(t)
    log.append(f"{f.GetReference()}: circle r {R} at ({cx:.2f}, {cy:.2f}), '{LABEL}' {SIZE} mm")

n = 0
for d in b.GetDrawings():
    if isinstance(d, pcbnew.PCB_TEXT) and REV_OLD in d.GetText():
        d.SetText(d.GetText().replace(REV_OLD, REV_NEW)); n += 1
        log.append(f"rev silk: {d.GetText()}")
assert n == 1, n
b.GetTitleBlock().SetRevision(REV_NEW.split()[1])
pcbnew.SaveBoard(str(PCB), b)
print("\n".join(log))
