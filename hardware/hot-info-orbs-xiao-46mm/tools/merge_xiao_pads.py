#!/usr/bin/env python3
"""One-shot: merge each XIAO pin's PTH + castellation SMD pad into ONE oval
through-hole pad in lib/_ESP32_modules.pretty/XIAO-ESP32-S3-DIP.kicad_mod.

Seeed's footprint draws every pin twice (a 1.524 circle on 0.889 drill at
y ±7.62 and a 2.432 x 1.524 SMD at y ±8.455). Two pads with one number
confuse Freerouting: it reports 14 "violations" (the overlapping pairs),
adds a stub between them on every pin, and then leaves one net unrouted
each run. One oval pad spanning both (y 6.858 .. 9.671, hole kept at
7.62 via drill offset) is also what the "headers OR castellation solder"
build wants — one piece of copper either way.
Run once with plain python3; the PCB swap script loads the result.
"""
import re
from pathlib import Path

MOD = Path(__file__).resolve().parent.parent / "lib/_ESP32_modules.pretty/XIAO-ESP32-S3-DIP.kicad_mod"
s = MOD.read_text()
assert s.count("(pad ") == 28, "already merged?"

# drop the 14 SMD pads
s, n = re.subn(r'\t\(pad "\d+" smd roundrect\n(?:\t\t.*\n)*?\t\)\n', "", s)
assert n == 14, n

def oval(m):
    num, x, y, rot = m.group(1), float(m.group(2)), float(m.group(3)), m.group(4)
    sign = 1 if y > 0 else -1
    cy = sign * 8.2645                       # copper center: (6.858 + 9.671) / 2
    # (at ..) is the HOLE; (offset ..) moves the copper shape away from it
    off = cy - y
    return ('\t(pad "%s" thru_hole oval\n\t\t(at %s %s 0)\n\t\t(size 1.524 2.813)\n'
            '\t\t(drill 0.889\n\t\t\t(offset 0 %.4f)\n\t\t)\n\t\t(layers "*.Cu" "*.Mask")\n'
            '\t\t(remove_unused_layers no)\n' % (num, m.group(2), m.group(3), off))

s, n = re.subn(r'\t\(pad "(\d+)" thru_hole circle\n\t\t\(at (-?[\d.]+) (-?[\d.]+) (\d+)\)\n\t\t\(size 1\.524 1\.524\)\n'
               r'\t\t\(drill 0\.889\)\n\t\t\(layers "\*\.Cu" "\*\.Mask"\)\n\t\t\(remove_unused_layers no\)\n', oval, s)
assert n == 14, n
MOD.write_text(s)
print("merged 28 -> 14 pads in", MOD.name)
