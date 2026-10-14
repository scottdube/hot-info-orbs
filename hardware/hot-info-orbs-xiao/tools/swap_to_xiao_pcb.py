#!/usr/bin/env python3
"""One-shot: rewrite hot_info_orbs_xiao.kicad_pcb for the XIAO (KiCad closed).

Run with KiCad's own Python:
  /Applications/KiCad/KiCad.app/Contents/Frameworks/Python.framework/Versions/Current/bin/python3 tools/swap_to_xiao_pcb.py

  1. U2: SuperMini footprint out, XIAO-ESP32-S3-DIP in at the same spot on
     B.Cu, USB end toward the board's bottom edge (as the SuperMini was)
  2. R1 + C2 (RC reset) placed on F.Cu under the DS1/DS2 modules, next to C1
  3. every pad re-netted from the schematic netlist; footprint paths set to
     the schematic symbol uuids so "schematic parity" DRC passes
  4. all tracks and vias deleted — routing is redone by Freerouting
     (tools/route_xiao.py). Zones are kept.
"""
import re, subprocess, sys
from pathlib import Path
import pcbnew

HERE = Path(__file__).resolve().parent.parent
PCB = HERE / "hot_info_orbs_xiao.kicad_pcb"
SCH = HERE / "hot_info_orbs_xiao.kicad_sch"
NET = Path("/tmp/hot_info_orbs_xiao.net")
KICAD_CLI = "/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli"
STOCK = Path("/Applications/KiCad/KiCad.app/Contents/SharedSupport/footprints")
mm = pcbnew.FromMM

# ---------- schematic side: ref -> uuid, ref -> {pad: net} ----------
subprocess.run([KICAD_CLI, "sch", "export", "netlist", "--format", "kicadsexpr", "-o", str(NET), str(SCH)],
               check=True, capture_output=True)
t = NET.read_text()
nets = t[t.index("(nets"):]
pad_net = {}
for b in re.split(r"\(net\s+\(code", nets)[1:]:
    name = re.search(r'\(name "([^"]+)"', b).group(1)
    for ref, pin in re.findall(r'\(ref "([^"]+)"\)\s*\(pin "([^"]+)"\)', b):
        pad_net.setdefault(ref, {})[pin] = name
s = SCH.read_text()
ref_uuid = {}
for m in re.finditer(r'\t\(symbol\n\t\t\(lib_id "[^"]+"\)[\s\S]*?\(uuid "([^"]+)"\)[\s\S]*?\(property "Reference" "([^"]+)"', s):
    ref_uuid[m.group(2)] = m.group(1)
print("schematic refs:", sorted(ref_uuid))

board = pcbnew.LoadBoard(str(PCB))

# ---------- 1. U2 ----------
old = board.FindFootprintByReference("U2")
pos, layer = old.GetPosition(), old.GetLayer()
print("old U2 at", pcbnew.ToMM(pos.x), pcbnew.ToMM(pos.y), old.GetLayerName())
board.Remove(old)

sch_desc = {}
for m in re.finditer(r'\(property "Reference" "([^"]+)"[\s\S]*?\(property "Description" "([^"]*)"', s):
    sch_desc.setdefault(m.group(1), m.group(2))

def place(lib_nick, lib_dir, name, ref, value, x, y, rot, back=False):
    fp = pcbnew.FootprintLoad(str(lib_dir), name)
    assert fp, (lib_dir, name)
    fp.SetFPID(pcbnew.LIB_ID(lib_nick, name))
    fp.SetReference(ref); fp.SetValue(value)
    # SetField(FIELD_T_DESCRIPTION, ..) creates a stray user field named "5"
    # that lands on the silkscreen; edit the existing field instead.
    fp.GetField(pcbnew.FIELD_T_DESCRIPTION).SetText(sch_desc.get(ref, ""))
    board.Add(fp)
    fp.SetPosition(pcbnew.VECTOR2I(mm(x), mm(y)))
    if back:
        fp.SetLayerAndFlip(pcbnew.B_Cu)
    fp.SetOrientationDegrees(rot)
    fp.SetPath(pcbnew.KIID_PATH("/" + ref_uuid[ref]))
    for t in (fp.Reference(), fp.Value()):
        t.SetTextSize(pcbnew.VECTOR2I(mm(1.0), mm(1.0)))
        t.SetTextThickness(mm(0.15))
    return fp

# y 95.0 keeps the module's silk outline inside the board edge (107.78);
# the SuperMini sat at 95.89 but was 2 mm shorter.
u2 = place("_ESP32 modules", HERE / "lib" / "_ESP32_modules.pretty", "XIAO-ESP32-S3-DIP", "U2",
           "XIAO ESP32-S3 Plus", 131.1, 95.0, 0, back=True)
# USB end (pads 1 D0 and 14 5V) must face the bottom board edge (+y)
def pad_y(fp, n):
    return [pcbnew.ToMM(p.GetPosition().y) for p in fp.Pads() if p.GetNumber() == n and p.GetAttribute() == pcbnew.PAD_ATTRIB_PTH][0]
for rot in (0, 90, 180, 270):
    u2.SetOrientationDegrees(rot)
    if pad_y(u2, "1") > pad_y(u2, "7") and abs(pad_y(u2, "1") - pad_y(u2, "14")) < 0.01:
        break
else:
    sys.exit("no orientation puts the USB end at +y")
print("U2 rot", rot, "pad1 y", pad_y(u2, "1"), "pad7 y", pad_y(u2, "7"))
# reference and the "goes on this side" note sit inside the module outline,
# clear of the pad columns (x 124.2..138.0) and of DS2 which ends at x 121.0
u2.Reference().SetPosition(pcbnew.VECTOR2I(mm(131.1), mm(90.5)))
u2.Reference().SetTextAngleDegrees(0)
u2.Value().SetVisible(False)

# ---------- 2. R1, C2 on the display side, under DS1/DS2, beside C1 ----------
place("Resistor_THT", STOCK / "Resistor_THT.pretty", "R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal", "R1", "10k", 97.0, 101.5, 0)
place("Capacitor_THT", STOCK / "Capacitor_THT.pretty", "C_Disc_D5.0mm_W2.5mm_P5.00mm", "C2", "1u", 86.0, 101.5, 0)

# ---------- 2b. silkscreen that described the SuperMini ----------
for d in list(board.GetDrawings()):
    if "Silk" not in d.GetLayerName():
        continue
    bb = d.GetBoundingBox()
    x0, y0 = pcbnew.ToMM(bb.GetX()), pcbnew.ToMM(bb.GetY())
    if d.GetClass() == "PCB_TEXT":
        txt = d.GetText()
        if txt.startswith("ESP32"):
            d.SetText(txt.replace("ESP32", "XIAO"))
            # the originals are left/top justified, so SetPosition would set the
            # corner; center them so a 9 mm text fits the 13.7 mm between pad rows
            d.SetHorizJustify(pcbnew.GR_TEXT_H_ALIGN_CENTER)
            d.SetVertJustify(pcbnew.GR_TEXT_V_ALIGN_CENTER)
            if d.GetLayerName() == "B.Silkscreen":   # was under the module outline
                d.SetPosition(pcbnew.VECTOR2I(mm(131.1), mm(96.5)))
            else:                                    # F.Silk copy: keep inside the outline rect
                d.SetPosition(pcbnew.VECTOR2I(mm(131.1), mm(96.5)))
        elif txt == "Antenna":                       # now overlaps the longer module; edge is y 79.26
            d.SetPosition(pcbnew.VECTOR2I(d.GetPosition().x, mm(82.0)))
        elif txt.startswith("HoT Info Orbs"):
            d.SetText("HoT Info Orbs\nXIAO X0.1")
    elif d.GetClass() == "PCB_SHAPE" and d.GetShapeStr() == "Rect":
        if d.GetLayerName() == "B.Silkscreen" and 125 < x0 < 126 and 82 < y0 < 83:
            board.Remove(d)                          # keepout marker, collides with U2 silk; rule area stays
        elif d.GetLayerName() == "F.Silkscreen" and 122 < x0 < 123 and 83 < y0 < 85:
            d.SetStart(pcbnew.VECTOR2I(mm(122.2), mm(84.5)))   # module outline as seen from the front
            d.SetEnd(pcbnew.VECTOR2I(mm(140.0), mm(105.5)))

# ---------- 3. nets + paths for every footprint ----------
def net(name):
    n = board.FindNet(name)
    if n is None:
        n = pcbnew.NETINFO_ITEM(board, name)
        board.Add(n)
    return n

for fp in board.GetFootprints():
    ref = fp.GetReference()
    if ref not in pad_net and ref not in ref_uuid:
        print("  (no schematic symbol:", ref, fp.GetFPIDAsString(), ")")
        continue
    fp.SetPath(pcbnew.KIID_PATH("/" + ref_uuid[ref]))
    for pad in fp.Pads():
        nm = pad_net.get(ref, {}).get(pad.GetNumber())
        if nm is None:
            pad.SetNet(net("unconnected-(%s-Pad%s)" % (ref, pad.GetNumber())) if pad.GetNumber() else None)
            continue
        pad.SetNet(net(nm))

# ---------- 4. rip up ----------
for tr in list(board.GetTracks()):
    board.Remove(tr)
board.BuildListOfNets()
pcbnew.SaveBoard(str(PCB), board)
print("saved", PCB)
