#!/usr/bin/env python3
"""Rev 2.5: three probe pads, 3V3 / GND / 5V, for debugging other people's
builds (KiCad CLOSED, run once, then tools/route_xiao.py).

  /Applications/KiCad/KiCad.app/Contents/Frameworks/Python.framework/Versions/Current/bin/python3 tools/test_points.py

Why (Scott, 2026-10-25): "on the debug test ports just add gnd 3.3v and 5v".
Suggested and dropped on his call: RST, SCLK, MOSI, DC and CS1-CS5 pads.

Schematic: TP1 3V3, TP2 GND, TP3 +5V (Connector:TestPoint) in a dashed
"TEST POINTS" box in the bottom housekeeping row, right of POWER FLAGS, tops
aligned at y 160.02, wired the way POWER FLAGS is (power symbol and TP on the
same point; 3V3 by local label). Excluded from the BOM: a test pad is a bare
plated hole, there is nothing to buy, so the kit cost does not move.

Board: a 2.0 mm pad on a 1.00 mm drill, the drill size the sockets already
use, so no new drill tool. Footprint lib/_TestPoints.pretty/
TestPoint_THTPad_D2.0mm_Drill1.0mm_P2.54 is the stock TestPoint one trimmed
to stack at 2.54 mm: courtyard r 1.5 -> 1.25 (stock overlapped its
neighbour, 2 DRC errors), silk circle removed, reference on F.Fab (on silk
it sat on the next pad). Editing the stock footprint in the board instead
gave 3 lib_footprint_mismatch warnings forever. One column at
x 173.74, midway between the DS3 and DS4 lit-area circles (150.74 and
196.74), 2.54 mm pitch centered on y 81: a 1x3 header or a 3-pin clip fits
if someone wants one. GND in the middle so the black probe stays put and the
red probe reaches either rail. Plated through, so reachable from either face.
The stock silk circles (r 1.2) would touch at 2.54 pitch, so: one rectangle around the column instead, front and back, with
the rail names beside each pad on both faces.

Rejected: a 1x8 row with the bus signals (Scott's call above); the column
next to DS3 at x ~153, where the pads clipped DS3's front silk circle.
"""
import re
import uuid
from pathlib import Path
import pcbnew

HERE = Path(__file__).resolve().parent.parent
SCH = HERE / "hot_info_orbs_xiao_46mm.kicad_sch"
PCB = HERE / "hot_info_orbs_xiao_46mm.kicad_pcb"
SHEET = "529f227b-dda3-4d44-98c7-ef3cb8bfc72d"
SYMLIB = Path("/Applications/KiCad/KiCad.app/Contents/SharedSupport/symbols/Connector.kicad_sym")
FPLIB = HERE / "lib/_TestPoints.pretty"
FPNAME = "TestPoint_THTPad_D2.0mm_Drill1.0mm_P2.54"
REV = "2.5"

# ref, value, net, rail symbol, sch x, board y
TPS = [("TP1", "3V3", "/3V3", None, 63.5, 78.46),
       ("TP2", "GND", "GND", "power:GND", 76.2, 81.00),
       ("TP3", "5V", "+5V", "power:+5V", 88.9, 83.54)]
SY = 170.18          # schematic pin point row
BX = 173.74          # board column x
U = lambda: str(uuid.uuid4())


def block(s, i):
    d = 0
    for j in range(i, len(s)):
        if s[j] == "(":
            d += 1
        elif s[j] == ")":
            d -= 1
            if d == 0:
                return s[i:j + 1]


def prop(name, val, x, y, hide=False):
    h = "\n\t\t\t(hide yes)" if hide else ""
    return (f'\t\t(property "{name}" "{val}"\n\t\t\t(at {x:g} {y:g} 0){h}\n'
            f'\t\t\t(show_name no)\n\t\t\t(do_not_autoplace no)\n'
            f'\t\t\t(effects\n\t\t\t\t(font\n\t\t\t\t\t(size 1.27 1.27)\n\t\t\t\t)\n\t\t\t)\n\t\t)\n')


def symbol(lib_id, ref, val, x, y, rot, fields, footprint="", in_bom=True, desc=""):
    """fields: (ref_xy, val_xy, ref_hidden)"""
    (rx, ry), (vx, vy), rhide = fields
    yn = "yes" if in_bom else "no"
    return (f'\t(symbol\n\t\t(lib_id "{lib_id}")\n\t\t(at {x:g} {y:g} {rot})\n\t\t(unit 1)\n'
            f'\t\t(body_style 1)\n\t\t(exclude_from_sim no)\n\t\t(in_bom {yn})\n\t\t(on_board yes)\n'
            f'\t\t(in_pos_files {yn})\n\t\t(dnp no)\n\t\t(uuid "{SYM_UUID.get(ref) or U()}")\n'
            + prop("Reference", ref, rx, ry, rhide) + prop("Value", val, vx, vy)
            + prop("Footprint", footprint, x, y, True) + prop("Datasheet", "", x, y, True)
            + prop("Description", desc, x, y, True)
            + f'\t\t(pin "1"\n\t\t\t(uuid "{U()}")\n\t\t)\n'
            f'\t\t(instances\n\t\t\t(project "hot_info_orbs_xiao_46mm"\n\t\t\t\t(path "/{SHEET}"\n'
            f'\t\t\t\t\t(reference "{ref}")\n\t\t\t\t\t(unit 1)\n\t\t\t\t)\n\t\t\t)\n\t\t)\n\t)\n')


# ---------------------------------------------------------------- schematic
s = SCH.read_text()
assert '"TP1"' not in s, "already applied"
SYM_UUID = {ref: U() for ref, *_ in TPS}

# lib symbol: the stock one, renamed to its library-qualified id
lib = SYMLIB.read_text()
i = lib.find('\t(symbol "TestPoint"\n')
tp = block(lib, i + 1)
tp = tp.replace('(symbol "TestPoint"', '(symbol "Connector:TestPoint"', 1)
tp = "\t\t" + tp.replace("\n", "\n\t")
anchor = s.find('\t\t(symbol "Device:C"')
s = s[:anchor] + tp.lstrip("\t").join(["\t\t", "\n"]) + s[anchor:]

new = []
pwr = {"power:GND": "#PWR05", "power:+5V": "#PWR06"}
for ref, val, net, rail, x, _ in TPS:
    if ref == "TP3":   # TP hangs below the +5V arrow, as #FLG01 does
        new.append(symbol("Connector:TestPoint", ref, val, x, SY, 180,
                          ((x, SY + 7.62), (x, SY + 5.842), False), f"_TestPoints:{FPNAME}", False,
                          "test point"))
    else:
        new.append(symbol("Connector:TestPoint", ref, val, x, SY, 0,
                          ((x, SY - 6.858), (x, SY - 5.08), False), f"_TestPoints:{FPNAME}", False,
                          "test point"))
    if rail == "power:GND":
        new.append(symbol(rail, pwr[rail], "GND", x, SY, 0, ((x, SY + 6.35), (x, SY + 5.08), True),
                          desc='Power symbol creates a global label with name \\"GND\\" , ground'))
    elif rail == "power:+5V":
        new.append(symbol(rail, pwr[rail], "+5V", x, SY, 0, ((x, SY + 3.81), (x, SY - 3.556), True),
                          desc='Power symbol creates a global label with name \\"+5V\\"'))
    else:
        new.append(f'\t(label "3V3"\n\t\t(at {x:g} {SY:g} 270)\n\t\t(effects\n\t\t\t(font\n'
                   f'\t\t\t\t(size 1.27 1.27)\n\t\t\t)\n\t\t\t(justify right bottom)\n\t\t)\n'
                   f'\t\t(uuid "{U()}")\n\t)\n')
new.append(f'\t(rectangle\n\t\t(start 55.88 160.02)\n\t\t(end 96.52 182.88)\n\t\t(stroke\n'
           f'\t\t\t(width 0)\n\t\t\t(type dash)\n\t\t)\n\t\t(fill\n\t\t\t(type none)\n\t\t)\n'
           f'\t\t(uuid "{U()}")\n\t)\n')
new.append(f'\t(text "TEST POINTS  TP1-TP3: probe pads"\n\t\t(exclude_from_sim no)\n'
           f'\t\t(at 57.15 181.61 0)\n\t\t(effects\n\t\t\t(font\n\t\t\t\t(size 1.27 1.27)\n'
           f'\t\t\t)\n\t\t\t(justify left bottom)\n\t\t)\n\t\t(uuid "{U()}")\n\t)\n')
anchor = s.rfind("\t(sheet_instances")
s = s[:anchor] + "".join(new) + s[anchor:]
s = s.replace('(rev "2.4")', f'(rev "{REV}")', 1)
SCH.write_text(s)

# ---------------------------------------------------------------- board
MM, V = pcbnew.FromMM, pcbnew.VECTOR2I
b = pcbnew.LoadBoard(str(PCB))
nets = b.GetNetsByName()
for ref, val, net, _, _, y in TPS:
    fp = pcbnew.FootprintLoad(str(FPLIB), FPNAME)
    fp.SetFPID(pcbnew.LIB_ID("_TestPoints", FPNAME))
    fp.SetReference(ref)
    fp.SetValue(val)
    b.Add(fp)
    fp.SetPosition(V(MM(BX), MM(y)))
    fp.SetPath(pcbnew.KIID_PATH("/" + SYM_UUID[ref]))
    fp.SetSheetname("/")
    fp.SetSheetfile("hot_info_orbs_xiao_46mm.kicad_sch")
    for p in fp.Pads():
        p.SetNet(nets[net])
    for layer, dx, mirror in ((pcbnew.F_SilkS, 2.0, False), (pcbnew.B_SilkS, -2.0, True)):
        t = pcbnew.PCB_TEXT(b)
        t.SetText(val)
        t.SetLayer(layer)
        t.SetTextSize(V(MM(1.0), MM(1.0)))
        t.SetTextThickness(MM(0.15))
        t.SetHorizJustify(pcbnew.GR_TEXT_H_ALIGN_LEFT)
        t.SetVertJustify(pcbnew.GR_TEXT_V_ALIGN_CENTER)
        t.SetMirrored(mirror)
        t.SetPosition(V(MM(BX + dx), MM(y)))
        b.Add(t)
for layer in (pcbnew.F_SilkS, pcbnew.B_SilkS):
    r = pcbnew.PCB_SHAPE(b)
    r.SetShape(pcbnew.SHAPE_T_RECT)
    r.SetStart(V(MM(BX - 1.5), MM(TPS[0][5] - 1.5)))
    r.SetEnd(V(MM(BX + 1.5), MM(TPS[-1][5] + 1.5)))
    r.SetLayer(layer)
    r.SetWidth(MM(0.12))
    b.Add(r)
for d in b.GetDrawings():
    if isinstance(d, pcbnew.PCB_TEXT) and "rev 2.4" in d.GetText():
        d.SetText(d.GetText().replace("rev 2.4", f"rev {REV}"))
tb = b.GetTitleBlock()
tb.SetRevision(REV)
b.SetTitleBlock(tb)
pcbnew.SaveBoard(str(PCB), b)
print("schematic + board written; next: tools/route_xiao.py")
