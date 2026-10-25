#!/usr/bin/env python3
"""Rev 2.5 run 2: "Test" beside the probe-pad column (KiCad CLOSED, run once).
Silk only, so no reroute: DRC, then the fab export.

  /Applications/KiCad/KiCad.app/Contents/Frameworks/Python.framework/Versions/Current/bin/python3 tools/test_label.py

Why (Scott, 2026-10-25): "add "Test" on the opposite side of the voltage
labels same side of board". The rail names sit right of the column on the
front and left of it on the back (tools/test_points.py), so "Test" goes on
the other side on each face. It is set vertically, centered on the GND pad,
so one word labels all three pads rather than reading as a fourth row beside
GND only.

Superseded on the board: Scott rotated both labels to horizontal at the GND
row by hand in KiCad (run 3, front x 170, back x 177). Rerunning this
script would refuse anyway ("already applied").
"""
from pathlib import Path
import pcbnew

HERE = Path(__file__).resolve().parent.parent
PCB = HERE / "hot_info_orbs_xiao_46mm.kicad_pcb"
BX, CY = 173.74, 81.00    # pad column x, GND pad y (column center)
DX = 2.3                  # rectangle edge is BX +/- 1.5; 1.0 mm text leaves 0.3 mm

MM, V = pcbnew.FromMM, pcbnew.VECTOR2I
b = pcbnew.LoadBoard(str(PCB))
assert not any(isinstance(d, pcbnew.PCB_TEXT) and d.GetText() == "Test"
               for d in b.GetDrawings()), "already applied"
for layer, dx, mirror in ((pcbnew.F_SilkS, -DX, False), (pcbnew.B_SilkS, DX, True)):
    t = pcbnew.PCB_TEXT(b)
    t.SetText("Test")
    t.SetLayer(layer)
    t.SetTextSize(V(MM(1.0), MM(1.0)))
    t.SetTextThickness(MM(0.15))
    t.SetHorizJustify(pcbnew.GR_TEXT_H_ALIGN_CENTER)
    t.SetVertJustify(pcbnew.GR_TEXT_V_ALIGN_CENTER)
    t.SetTextAngle(pcbnew.EDA_ANGLE(90, pcbnew.DEGREES_T))
    t.SetMirrored(mirror)
    t.SetPosition(V(MM(BX + dx), MM(CY)))
    b.Add(t)
pcbnew.SaveBoard(str(PCB), b)
print("board written; next: DRC, fab export")
