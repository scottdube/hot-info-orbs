#!/usr/bin/env python3
"""Attach Seeed's XIAO ESP32S3 PLUS STEP to U2 (board + library footprint),
and point the five display footprints on the board at the project-local
GC9A01 model. KiCad CLOSED. Run with KiCad's Python:

  .../python3 tools/xiao_model.py [ox oy oz rx ry rz]   (no args = the fit below)

Why: the AMz3D footprint XIAO-ESP32-S3-DIP carries five ${AMZPATH} models
(nRF52840, SAMD21, RP2040, ESP32-C3, ESP32S3) and AMZPATH is not defined on
this machine, so the 3D view and STEP export showed no module at all. The
board's DS1-DS5 instances still referenced ${KICAD_USER_3DMODEL_DIR} from the
v1.1 project while the library footprint already said ${KIPRJMOD}; same
symptom. Scott's 2026-10-14 export log listed all six as "Could not add 3D
model".

Model: Seeed "XIAO ESP32S3 PLUS v4.step" (assembly 'XIAO ESP32S3
PLUS_V1.0_241025', downloaded 2026-10-14), copied to
lib/3d/_ESP32 modules/XIAO-ESP32S3-PLUS-v4.step. One model, the Plus: the
plain S3 is the same outline (21 x 17.8 mm, same USB end) so the picture is
right for either. Offset/rotation were found by rendering, see README.
"""
import sys
from pathlib import Path
import pcbnew

HERE = Path(__file__).resolve().parent.parent
PCB = HERE / "hot_info_orbs_xiao_46mm.kicad_pcb"
MOD = HERE / "lib/_ESP32_modules.pretty/XIAO-ESP32-S3-DIP.kicad_mod"
MODEL = "${KIPRJMOD}/lib/3d/_ESP32 modules/XIAO-ESP32S3-PLUS-v4.step"
DISP_OLD = "${KICAD_USER_3DMODEL_DIR}/_Displays/"
DISP_NEW = "${KIPRJMOD}/lib/3d/_Displays/"

# Fit found by rendering (kicad-cli pcb render --side bottom --zoom 9, pivot on
# U2) 2026-10-14: Seeed's STEP has the module standing on edge, so -90 about X
# lays it component-side out with the USB-C at the board edge; -7.62 in y puts
# its castellation columns on the footprint's holes (it was one column off);
# -1.778 in x lines the seven pads up along the column. z 0 seats it on the
# copper.
FIT = [-1.778, -7.62, 0.0, -90.0, 0.0, 0.0]   # DIP-era fit; 1.49 mm off the holes. The SOCKET footprint carries the measured (-1.651, -6.13, 11).
ox, oy, oz, rx, ry, rz = (list(map(float, sys.argv[1:7])) + FIT[len(sys.argv) - 1:])[:6]

def xiao_model():
    m = pcbnew.FP_3DMODEL()
    m.m_Filename = MODEL
    m.m_Offset = pcbnew.VECTOR3D(ox, oy, oz)
    m.m_Rotation = pcbnew.VECTOR3D(rx, ry, rz)
    m.m_Scale = pcbnew.VECTOR3D(1, 1, 1)
    m.m_Show = True
    return m

# board
b = pcbnew.LoadBoard(str(PCB))
for fp in b.GetFootprints():
    if fp.GetReference() == "U2":
        models = fp.Models()
        models.clear()
        models.push_back(xiao_model())
    elif fp.GetReference().startswith("DS"):
        # editing m_Filename on the iterated element does not stick (SWIG
        # hands back copies): rebuild the list
        models = fp.Models()
        fresh = []
        for m in models:
            n = pcbnew.FP_3DMODEL()
            n.m_Filename = (DISP_NEW + m.m_Filename[len(DISP_OLD):]) if m.m_Filename.startswith(DISP_OLD) else m.m_Filename
            n.m_Offset, n.m_Rotation, n.m_Scale, n.m_Show = m.m_Offset, m.m_Rotation, m.m_Scale, m.m_Show
            fresh.append(n)
        models.clear()
        for n in fresh:
            models.push_back(n)
pcbnew.SaveBoard(str(PCB), b)

# library footprint: text edit, keep everything before the first (model
txt = MOD.read_text()
head = txt[: txt.index("\t(model ")]
block = ('\t(model "%s"\n\t\t(offset\n\t\t\t(xyz %g %g %g)\n\t\t)\n\t\t(scale\n\t\t\t(xyz 1 1 1)\n\t\t)\n'
         '\t\t(rotate\n\t\t\t(xyz %g %g %g)\n\t\t)\n\t)\n)\n' % (MODEL, ox, oy, oz, rx, ry, rz))
MOD.write_text(head + block)
print("U2 model set: offset (%g %g %g) rotate (%g %g %g); DS models -> %s" % (ox, oy, oz, rx, ry, rz, DISP_NEW))
