#!/usr/bin/env python3
"""Redraw the display wiring as net labels in a boxed block (KiCad CLOSED, run once).

  python3 tools/display_block.py            then check the netlist is unchanged

Why: every display pin was wired to a column of labels in mid-sheet, so 60-odd
wires crossed the page to say what one label at the pin says (Scott,
2026-10-24: "no need to bring the net wires down from the displays like
that"). Each display pin now carries a short stub and its net label, the
displays sit in a dashed box, and the mid-sheet label column is gone. SW1/SW2
get labels at the switch like SW3 already had, and the switch common bus gets
a 3V3 label instead of its run up to the column. C1 keeps only its +5V/GND
power symbols: a +5V or GND label joins the power net by name (checked: same
pin groups with no label at C1), so the rail into C1 had no electrical job.

Rejected: a hierarchical sheet for the displays. Scott chose a boxed group on
the same page; a sub-sheet would also change every display's symbol path and
need the PCB re-linked.

Only wires, junctions, labels and two graphics change; no symbol moves, so the
PCB is untouched. Verify with the kicadxml netlist: same pins on the same nets.
"""

import math
import re
import sys
import uuid
from pathlib import Path

SCH = Path(sys.argv[1] if len(sys.argv) > 1 else
           Path(__file__).resolve().parent.parent / "hot_info_orbs_xiao_46mm.kicad_sch")

DISPLAY_NET = {"VCC": "+5V", "GND": "GND", "SCL": "SCLK", "SDA": "MOSI",
               "DC": "DC", "RST": "RST"}  # CS -> CS<n> from the reference
STUB = 5.08            # display pin stub length, mm (2 grid steps)
C1_X_MIN = 168.0       # rail wires right of this are C1's own; kept


def top_items(s):
    depth, start, in_str, i = 0, None, False, 0
    while i < len(s):
        c = s[i]
        if in_str:
            if c == "\\":
                i += 2
                continue
            in_str = c != '"'
        elif c == '"':
            in_str = True
        elif c == "(":
            depth += 1
            if depth == 2:
                start = i
        elif c == ")":
            if depth == 2:
                yield start, i + 1, s[start:i + 1]
            depth -= 1
        i += 1


def r(v):
    return round(v, 2)


def lib_pins(lib, lib_id):
    i = lib.find(f'(symbol "{lib_id}"')
    d, j = 0, i
    while True:
        d += {"(": 1, ")": -1}.get(lib[j], 0)
        if d == 0 and lib[j] == ")":
            break
        j += 1
    return [(float(m[1]), float(m[2]), m[4], m[3]) for m in re.finditer(
        r'\(pin \w+ \w+\s+\(at ([\d.\-]+) ([\d.\-]+) \d+\)\s+\(length [\d.]+\)'
        r'.*?\(name "([^"]*)".*?\(number "([^"]*)"', lib[i:j], re.S)]


def on_seg(p, a, b):
    (x, y), (x1, y1), (x2, y2) = p, a, b
    return ((x1 == x2 == x and min(y1, y2) <= y <= max(y1, y2)) or
            (y1 == y2 == y and min(x1, x2) <= x <= max(x1, x2)))


def wire(a, b):
    return (f'\t(wire\n\t\t(pts\n\t\t\t(xy {a[0]:g} {a[1]:g}) (xy {b[0]:g} {b[1]:g})\n\t\t)\n'
            f'\t\t(stroke\n\t\t\t(width 0)\n\t\t\t(type default)\n\t\t)\n'
            f'\t\t(uuid "{uuid.uuid4()}")\n\t)\n')


def label(name, p, angle):
    just = "right bottom" if angle in (180, 270) else "left bottom"
    return (f'\t(label "{name}"\n\t\t(at {p[0]:g} {p[1]:g} {angle})\n'
            f'\t\t(effects\n\t\t\t(font\n\t\t\t\t(size 1.27 1.27)\n\t\t\t)\n'
            f'\t\t\t(justify {just})\n\t\t)\n\t\t(uuid "{uuid.uuid4()}")\n\t)\n')


def main():
    s = SCH.read_text()
    items = list(top_items(s))
    lib = next(t for _, _, t in items if t.startswith("(lib_symbols"))

    pins = {}  # (x, y) -> (ref, pin name)
    for _, _, t in items:
        if not t.startswith("(symbol"):
            continue
        lid = re.search(r'\(lib_id "([^"]+)"', t)[1]
        x0, y0, rot = map(float, re.search(
            r'\(at ([\d.\-]+) ([\d.\-]+) ([\d.\-]+)\)', t).groups())
        ref = re.search(r'\(property "Reference" "([^"]+)"', t)[1]
        th = math.radians(-rot)
        for px, py, _num, name in lib_pins(lib, lid):
            vx, vy = px, -py
            pins[(r(x0 + vx * math.cos(th) - vy * math.sin(th)),
                  r(y0 + vx * math.sin(th) + vy * math.cos(th)))] = (ref, name)

    wires, labels, juncs = [], [], []
    for a, b, t in items:
        if t.startswith("(wire"):
            p = [(r(float(x)), r(float(y))) for x, y in
                 re.findall(r'\(xy ([\d.\-]+) ([\d.\-]+)\)', t)]
            wires.append(((a, b), p[0], p[1]))
        elif t.startswith("(label"):
            x, y = re.search(r'\(at ([\d.\-]+) ([\d.\-]+)', t).groups()
            labels.append(((a, b), re.search(r'\(label "([^"]+)"', t)[1],
                           (r(float(x)), r(float(y)))))
        elif t.startswith("(junction"):
            x, y = re.search(r'\(at ([\d.\-]+) ([\d.\-]+)', t).groups()
            juncs.append(((a, b), (r(float(x)), r(float(y)))))

    # Group wires into connected runs.
    parent = list(range(len(wires)))

    def find(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i
    for i, (_, a1, b1) in enumerate(wires):
        for j in range(i + 1, len(wires)):
            _, a2, b2 = wires[j]
            if any(on_seg(p, a2, b2) for p in (a1, b1)) or \
               any(on_seg(p, a1, b1) for p in (a2, b2)):
                parent[find(i)] = find(j)
    runs = {}
    for i in range(len(wires)):
        runs.setdefault(find(i), []).append(i)

    def touches(idx, p):
        return any(on_seg(p, wires[i][1], wires[i][2]) for i in idx)

    drop = set()  # spans to delete
    rail_ends = {}  # net -> kept wire points of a display rail at C1
    for idx in runs.values():
        refs = {ref for p, (ref, _) in pins.items() if touches(idx, p)}
        if any(ref.startswith("DS") for ref in refs):
            keep_c1 = "C1" in refs
            kept = []
            for i in idx:
                _, a, b = wires[i]
                if keep_c1 and min(a[0], b[0]) >= C1_X_MIN:
                    kept += [a, b]
                else:
                    drop.add(wires[i][0])
            if kept:
                rail_ends[next(n for sp, n, p in labels if touches(idx, p))] = kept
        elif refs & {"SW1", "SW2"} and not refs & {"SW3"}:
            drop.update(wires[i][0] for i in idx)          # SW1/SW2 runs to the column
        elif refs == {"SW1", "SW2", "SW3"}:
            bus_x = min(max(a[0], b[0]) for _, a, b in (wires[i] for i in idx)
                        if a[0] == b[0])                       # the vertical common bus
            for i in idx:
                _, a, b = wires[i]
                if max(a[0], b[0]) > bus_x:
                    drop.add(wires[i][0])                  # its run up to the column
            bus_top = min((p for i in idx for p in wires[i][1:] if p[0] == bus_x),
                          key=lambda p: p[1])
    # The rail taps left at C1 end in mid-air once the rail is gone; trim them.
    c1_pts = {p for pts in rail_ends.values() for p in pts}
    while True:
        kept_wires = [(sp, a, b) for sp, a, b in wires if sp not in drop]
        loose = [sp for sp, a, b in kept_wires if (a in c1_pts or b in c1_pts) and any(
            end not in pins and sum(on_seg(end, a2, b2) for _, a2, b2 in kept_wires) < 2
            for end in (a, b))]
        if not loose:
            break
        drop.update(loose)
    kept_wires = [(a, b) for sp, a, b in wires if sp not in drop]
    for sp, name, p in labels:
        if not any(on_seg(p, a, b) for a, b in kept_wires) and p not in pins:
            drop.add(sp)
    for sp, p in juncs:   # a junction marks a tee: 3+ wires, or one passing through
        on = [(a, b) for a, b in kept_wires if on_seg(p, a, b)]
        if len(on) < 3 and not any(p not in (a, b) for a, b in on):
            drop.add(sp)

    add = []
    xs = []
    for (x, y), (ref, name) in sorted(pins.items()):
        if ref.startswith("DS"):
            net = f"CS{ref[2:]}" if name == "CS" else DISPLAY_NET[name]
            end = (x, r(y + STUB))
            add += [wire((x, y), end), label(net, end, 270)]
            xs.append(x)
        elif ref in ("SW1", "SW2") and name == "1":
            end = (r(x - 6.35), y)                           # as SW3's label sits
            add += [wire(end, (x, y)), label(ref, end, 0)]
    top = (bus_top[0], r(bus_top[1] - 2.54))
    add += [wire(top, bus_top), label("3V3", top, 90)]
    x0, x1, y0, y1 = min(xs) - 7.62, max(xs) + 7.62, 15.24, 63.5
    add.append(
        f'\t(rectangle\n\t\t(start {x0:g} {y0:g})\n\t\t(end {x1:g} {y1:g})\n'
        f'\t\t(stroke\n\t\t\t(width 0)\n\t\t\t(type dash)\n\t\t)\n'
        f'\t\t(fill\n\t\t\t(type none)\n\t\t)\n\t\t(uuid "{uuid.uuid4()}")\n\t)\n')
    add.append(
        f'\t(text "DISPLAYS  DS1-DS5: GC9A01 round TFT, each in a 1x7 socket"\n'
        f'\t\t(exclude_from_sim no)\n\t\t(at {x0 + 1.27:g} {y1 - 1.27:g} 0)\n'
        f'\t\t(effects\n\t\t\t(font\n\t\t\t\t(size 1.27 1.27)\n\t\t\t)\n'
        f'\t\t\t(justify left bottom)\n\t\t)\n\t\t(uuid "{uuid.uuid4()}")\n\t)\n')

    out, pos = [], 0
    for a, b in sorted(drop):
        out.append(s[pos:a].rstrip("\t"))
        pos = b
        while pos < len(s) and s[pos] in "\n":
            pos += 1
    out.append(s[pos:])
    s2 = "".join(out)
    k = s2.rfind("\t(sheet_instances")
    s2 = s2[:k] + "".join(add) + s2[k:]
    SCH.write_text(s2)
    print(f"dropped {len(drop)} items, added {len(add)}")


if __name__ == "__main__":
    main()
