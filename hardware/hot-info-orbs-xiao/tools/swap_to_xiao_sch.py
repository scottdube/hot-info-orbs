#!/usr/bin/env python3
"""One-shot: rewrite hot_info_orbs_xiao.kicad_sch from the SuperMini to the
XIAO ESP32-S3. Run once on the copied v1.1 schematic (KiCad closed).

What it does, in order:
  1. drops the SuperMini symbol instance + its lib_symbols entry + the three
     no_connects that sat on its unused pins
  2. puts a net label at the SuperMini end of every wire that reached it,
     so the display/button side keeps its wiring untouched
  3. places the XIAO symbol in free space with a stub wire + the same label
     on each edge pin, no_connect on the 11 non-edge pins
  4. adds R1/C2 (RC power-on reset for the display RST line)
Pin map: README.md in this folder.
"""
import re, sys, uuid
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
SCH = HERE / "hot_info_orbs_xiao.kicad_sch"
DEVICE_LIB = Path("/Applications/KiCad/KiCad.app/Contents/SharedSupport/symbols/Device.kicad_sym")
XIAO_LIB = HERE / "lib" / "Seeed_XIAO.kicad_sym"
SHEET = "529f227b-dda3-4d44-98c7-ef3cb8bfc72d"

s = SCH.read_text()
assert '(lib_id "_ESP32 modules:ESP32-S3-SuperMini")' in s, "already converted?"

def u(): return str(uuid.uuid4())

# ---------- 1. remove SuperMini instance, lib entry, no_connects ----------
def cut_block(text, start_marker, open_depth_start):
    """Remove the s-expr block that begins at the '(' which precedes
    start_marker; open_depth_start is the text offset of that '('."""
    i = open_depth_start
    depth = 0
    for j in range(i, len(text)):
        if text[j] == '(': depth += 1
        elif text[j] == ')':
            depth -= 1
            if depth == 0:
                return text[:i] + text[j+1:]
    raise RuntimeError("unbalanced")

k = s.index('(lib_id "_ESP32 modules:ESP32-S3-SuperMini")')
i = s.rindex('\t(symbol\n', 0, k) + 1
s = cut_block(s, None, i)

i = s.index('\t\t(symbol "_ESP32 modules:ESP32-S3-SuperMini"') + 2
s = cut_block(s, None, i)

for pos in (("96.52", "109.22"), ("96.52", "93.98"), ("96.52", "97.79")):
    m = re.search(r'\t\(no_connect\s*\(at %s %s\)\s*\(uuid "[^"]+"\)\s*\)\n' % pos, s)
    assert m, pos
    s = s[:m.start()] + s[m.end():]

# SuperMini pin 7 (SW3) ran a stub wire to a label at 133.35; both go, the
# switch-side SW3 label remains and the XIAO gets its own.
m = re.search(r'\t\(wire\n\t\t\(pts\n\t\t\t\(xy 127 116.84\) \(xy 133.35 116.84\)\n[\s\S]*?\n\t\)\n', s); assert m
s = s[:m.start()] + s[m.end():]
m = re.search(r'\t\(label "SW3"\n\t\t\(at 133.35 116.84 0\)[\s\S]*?\n\t\)\n', s); assert m
s = s[:m.start()] + s[m.end():]

# ---------- helpers ----------
def label(name, x, y, rot):
    just = {0: "left bottom", 90: "left bottom", 180: "right bottom", 270: "right bottom"}[rot]
    return ('\t(label "%s"\n\t\t(at %s %s %d)\n\t\t(effects\n\t\t\t(font\n\t\t\t\t(size 1.27 1.27)\n\t\t\t)\n'
            '\t\t\t(justify %s)\n\t\t)\n\t\t(uuid "%s")\n\t)\n') % (name, x, y, rot, just, u())

def wire(x1, y1, x2, y2):
    return ('\t(wire\n\t\t(pts\n\t\t\t(xy %s %s) (xy %s %s)\n\t\t)\n\t\t(stroke\n\t\t\t(width 0)\n\t\t\t(type default)\n\t\t)\n'
            '\t\t(uuid "%s")\n\t)\n') % (x1, y1, x2, y2, u())

def no_connect(x, y):
    return '\t(no_connect\n\t\t(at %s %s)\n\t\t(uuid "%s")\n\t)\n' % (x, y, u())

def prop(name, value, x, y, rot=0, hide=False, just=None):
    out = '\t\t(property "%s" "%s"\n\t\t\t(at %s %s %d)\n' % (name, value, x, y, rot)
    if hide: out += '\t\t\t(hide yes)\n'
    out += '\t\t\t(show_name no)\n\t\t\t(do_not_autoplace no)\n\t\t\t(effects\n\t\t\t\t(font\n\t\t\t\t\t(size 1.27 1.27)\n\t\t\t\t)\n'
    if just: out += '\t\t\t\t(justify %s)\n' % just
    out += '\t\t\t)\n\t\t)\n'
    return out

def instance(lib_id, x, y, rot, ref, value, fp, desc, pins, ref_at, val_at):
    out = '\t(symbol\n\t\t(lib_id "%s")\n\t\t(at %s %s %d)\n\t\t(unit 1)\n\t\t(body_style 1)\n' % (lib_id, x, y, rot)
    out += '\t\t(exclude_from_sim no)\n\t\t(in_bom yes)\n\t\t(on_board yes)\n\t\t(in_pos_files yes)\n\t\t(dnp no)\n'
    out += '\t\t(uuid "%s")\n' % u()
    out += prop("Reference", ref, *ref_at, just="left")
    out += prop("Value", value, *val_at, just="left")
    out += prop("Footprint", fp, x, y, hide=True)
    out += prop("Datasheet", "", x, y, hide=True)
    out += prop("Description", desc, x, y, hide=True)
    for p in pins:
        out += '\t\t(pin "%s"\n\t\t\t(uuid "%s")\n\t\t)\n' % (p, u())
    out += ('\t\t(instances\n\t\t\t(project "hot_info_orbs"\n\t\t\t\t(path "/%s"\n\t\t\t\t\t(reference "%s")\n'
            '\t\t\t\t\t(unit 1)\n\t\t\t\t)\n\t\t\t)\n\t\t)\n\t)\n') % (SHEET, ref)
    return out

# ---------- 2. labels at the old SuperMini pin ends ----------
# (x, y, rot, net) — positions measured from the v1.1 schematic, pin 7 (SW3)
# already carries a label further along its wire and is left alone.
OLD = [
    (127.0, 93.98, 0, "+5V"), (127.0, 97.79, 0, "GND"), (127.0, 101.6, 0, "3V3"),
    (127.0, 105.41, 0, "RST"), (127.0, 109.22, 0, "SCLK"), (127.0, 113.03, 0, "MOSI"),
    (127.0, 120.65, 0, "DC"), (127.0, 124.46, 0, "CS5"),
    (96.52, 124.46, 180, "CS4"), (96.52, 120.65, 180, "CS3"), (96.52, 116.84, 180, "CS2"),
    (96.52, 113.03, 180, "CS1"), (96.52, 105.41, 180, "SW2"), (96.52, 101.6, 180, "SW1"),
]
add = "".join(label(n, x, y, r) for x, y, r, n in OLD)

# ---------- 3. XIAO ----------
SX, SY = 170.18, 160.02
XIAO_PINS = {  # number: (px, py, rot, net or None)
    1: (-21.59, 1.27, 0, "SW1"), 2: (-21.59, -2.54, 0, "SW2"), 3: (-21.59, -6.35, 0, "CS1"),
    4: (-21.59, -10.16, 0, "CS2"), 5: (-21.59, -13.97, 0, "CS3"), 6: (-21.59, -17.78, 0, "SW3"),
    7: (-21.59, -21.59, 0, "CS4"), 8: (21.59, -21.59, 180, "CS5"), 9: (21.59, -17.78, 180, "SCLK"),
    10: (21.59, -13.97, 180, "DC"), 11: (21.59, -10.16, 180, "MOSI"), 12: (21.59, -6.35, 180, "3V3"),
    13: (21.59, -2.54, 180, "GND"), 14: (21.59, 1.27, 180, "+5V"),
    15: (-13.97, 13.97, 270, None), 16: (-10.16, 13.97, 270, None), 17: (-6.35, 13.97, 270, None),
    18: (-2.54, 13.97, 270, None), 19: (1.27, 13.97, 270, None), 20: (5.08, 13.97, 270, None),
    21: (8.89, 13.97, 270, None), 22: (12.7, 13.97, 270, None),
    23: (-2.54, -30.48, 90, None), 24: (1.27, -30.48, 90, None), 25: (5.08, -30.48, 90, None),
}
# Pins 15-25 (JTAG, USB, EN, VBAT, back pad) are not on the edge-pin
# footprint, so the symbol is trimmed to 14 pins rather than carrying eleven
# no_connects that the PCB parity check would then flag as pad-less.
for n, (px, py, rot, net) in XIAO_PINS.items():
    if net is None:
        continue
    x, y = round(SX + px, 2), round(SY - py, 2)
    if rot == 0:
        add += wire(x, y, round(x - 5.08, 2), y) + label(net, round(x - 5.08, 2), y, 180)
    else:
        add += wire(x, y, round(x + 5.08, 2), y) + label(net, round(x + 5.08, 2), y, 0)
XIAO_DESC = "Seeed Studio XIAO ESP32-S3 Plus, edge pins only (headers or castellations)"
add += instance("_Seeed XIAO:XIAO-ESP32-S3-edge", SX, SY, 0, "U2", "XIAO ESP32-S3 Plus",
                "_ESP32 modules:XIAO-ESP32-S3-DIP", XIAO_DESC,
                [str(i) for i in range(1, 15)], (SX - 5.08, SY - 14.0), (SX - 11.43, SY - 11.0))

# ---------- 4. RC reset: 3V3 -R1- RST -C2- GND ----------
RX, RY = 215.9, 165.1
CX, CY = 215.9, 180.34
add += wire(RX, RY - 7.62, RX, RY - 3.81) + label("3V3", RX, RY - 7.62, 90)
add += wire(RX, RY + 3.81, RX, RY + 7.62) + wire(CX, CY - 7.62, CX, CY - 3.81) + label("RST", RX, RY + 7.62, 0)
add += wire(CX, CY + 3.81, CX, CY + 7.62) + label("GND", CX, CY + 7.62, 270)
add += instance("Device:R", RX, RY, 0, "R1", "10k",
                "Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal",
                "Display RST pull-up (RC power-on reset)", ["1", "2"], (RX + 2.54, RY - 1.27), (RX + 2.54, RY + 1.27))
add += instance("Device:C", CX, CY, 0, "C2", "1u",
                "Capacitor_THT:C_Disc_D5.0mm_W2.5mm_P5.00mm",
                "Display RST hold-off (RC power-on reset), ceramic", ["1", "2"], (CX + 2.54, CY - 1.27), (CX + 2.54, CY + 1.27))

# ---------- lib_symbols: XIAO, Device:R, Device:C ----------
def lib_entry(path, name, new_name):
    t = path.read_text()
    i = t.index('\t(symbol "%s"\n' % name)
    j = t.index('\n\t(symbol "', i + 5) if '\n\t(symbol "' in t[i+5:] else t.rindex('\n)')
    blk = t[i:j].rstrip("\n")
    blk = blk.replace('\t(symbol "%s"\n' % name, '\t(symbol "%s"\n' % new_name, 1)
    # indent one level deeper to sit inside (lib_symbols ...)
    return "\n".join("\t" + ln for ln in blk.split("\n")) + "\n"

def trim_pins(blk, keep):
    """Drop every (pin ...) block whose number is not in keep."""
    out = blk
    while True:
        found = None
        for m in re.finditer(r'\(pin \w+ \w+\s*\(at [^)]*\)[\s\S]*?\(number "(\d+)"', out):
            if int(m.group(1)) not in keep:
                found = m; break
        if not found:
            return out
        i = found.start(); depth = 0
        for j in range(i, len(out)):
            if out[j] == '(': depth += 1
            elif out[j] == ')':
                depth -= 1
                if depth == 0: break
        # also eat the leading tabs/newline before the block
        k = out.rfind("\n", 0, i)
        out = out[:k] + out[j+1:]

# Rewrite the project lib with the trimmed, renamed symbol so the schematic's
# cached copy and the library agree (Description set so parity is exact).
raw = XIAO_LIB.read_text()
i0 = raw.index('\t(symbol "XIAO-ESP32-S3-SMD"\n')
sym = trim_pins(raw[i0:raw.rindex("\n)")], set(range(1, 15)))
sym = sym.replace('"XIAO-ESP32-S3-SMD"', '"XIAO-ESP32-S3-edge"').replace('"XIAO-ESP32-S3-SMD_', '"XIAO-ESP32-S3-edge_')
sym = sym.replace('(property "Value" "XIAO-ESP32-S3-SMD"', '(property "Value" "XIAO-ESP32-S3-edge"')
sym = re.sub(r'\(property "Description" ""', '(property "Description" "%s"' % XIAO_DESC, sym, 1)
XIAO_LIB.write_text(raw[:i0] + sym + "\n)\n")

libs = (lib_entry(XIAO_LIB, "XIAO-ESP32-S3-edge", "_Seeed XIAO:XIAO-ESP32-S3-edge")
        + lib_entry(DEVICE_LIB, "R", "Device:R") + lib_entry(DEVICE_LIB, "C", "Device:C"))
s = s.replace("\t(lib_symbols\n", "\t(lib_symbols\n" + libs, 1)

# ---------- title + insert ----------
s = s.replace('\t(title_block\n\t\t(rev "1.1")\n\t)',
              '\t(title_block\n\t\t(title "HoT Info Orbs - XIAO ESP32-S3 variant")\n\t\t(rev "X0.1")\n\t)', 1)
s = s.replace("\t(sheet_instances\n", add + "\t(sheet_instances\n", 1)
SCH.write_text(s)
print("ok", SCH)
