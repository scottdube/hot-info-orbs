#!/usr/bin/env python3
"""Put each circuit group in a dashed, titled box like DISPLAYS (KiCad CLOSED).

  python3 tools/group_boxes.py              then check the netlist is unchanged

Why: Scott, 2026-10-24, after display_block.py: "put each discreet circuit
group in its own box like you did with displays and title it". Each box is
sized from what KiCad will draw for the group: the symbols' body graphics,
pin ends, visible fields, and the wires and labels hanging off its pins,
padded and snapped to the 1.27 mm grid. The title sits inside the bottom-left
corner, as on DISPLAYS. Re-running replaces the boxes this script drew
(matched by title prefix), so it can follow a later layout change.

Graphics only: no symbol, wire or label is touched.
"""

import math
import re
import sys
import uuid
from pathlib import Path

SCH = Path(sys.argv[1] if len(sys.argv) > 1 else
           Path(__file__).resolve().parent.parent / "hot_info_orbs_xiao_46mm.kicad_sch")

GROUPS = [  # (refs, title)
    ({"U2"}, "MCU  U2: XIAO ESP32-S3 or S3 Plus"),
    ({"SW1", "SW2", "SW3"}, "BUTTONS  SW1-SW3: tact switches to 3V3"),
    ({"R1", "C2"}, "DISPLAY RESET  R1/C2: RC power-on"),
    ({"C1", "#PWR01", "#PWR02"}, "5V BULK  C1"),
    ({"#PWR03", "#PWR04", "#FLG01", "#FLG02"}, "POWER FLAGS  ERC only"),
]
PAD = 2.54
TITLE_H = 2.54          # room under the contents for the title line
CHAR_W = 1.1            # mm per character at 1.27 mm text (estimate)


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


def block(text, head):
    """The balanced (...) starting at text.find(head)."""
    i = text.find(head)
    d, j = 0, i
    while True:
        d += {"(": 1, ")": -1}.get(text[j], 0)
        if d == 0 and text[j] == ")":
            return text[i:j + 1]
        j += 1


def on_seg(p, a, b):
    (x, y), (x1, y1), (x2, y2) = p, a, b
    return ((abs(x1 - x) < 1e-3 and abs(x2 - x) < 1e-3 and min(y1, y2) <= y <= max(y1, y2)) or
            (abs(y1 - y) < 1e-3 and abs(y2 - y) < 1e-3 and min(x1, x2) <= x <= max(x1, x2)))


def text_box(x, y, angle, n, centered=False, just=""):
    """Rough extent of n characters of 1.27 mm text anchored at (x, y).

    A label's text runs away from its anchor: 0 right, 90 up, 180 left,
    270 down. A field (centered) spreads both ways along its angle.
    """
    w, h = n * CHAR_W, 1.27
    if centered and not just:
        lo, hi = -w / 2, w / 2
    elif just:
        lo, hi = (0, w) if just == "left" else (-w, 0)
    else:
        lo, hi = (0, w) if angle in (0, 270) else (-w, 0)
    if angle in (90, 270):
        return [(x - h, y + lo), (x + h, y + hi)]
    return [(x + lo, y - h), (x + hi, y + h)]


def snap(v, up):
    g = 1.27
    return round((math.ceil(v / g) if up else math.floor(v / g)) * g, 2)


def main():
    s = SCH.read_text()
    items = list(top_items(s))
    lib = next(t for _, _, t in items if t.startswith("(lib_symbols"))

    # Drop boxes and titles a previous run drew.
    titles = tuple(t.split("  ")[0] + "  " for _, t in GROUPS)
    old = [(a, b) for a, b, t in items if t.startswith("(text") and
           re.search(r'\(text "([^"]*)"', t)[1].startswith(titles)]
    old_rects = []
    for a, b in old:
        tx, ty = map(float, re.search(r'\(at ([\d.\-]+) ([\d.\-]+)', s[a:b]).groups())
        for a2, b2, t2 in items:
            if t2.startswith("(rectangle"):
                (x0, y0), (x1, y1) = [tuple(map(float, m)) for m in re.findall(
                    r'\((?:start|end) ([\d.\-]+) ([\d.\-]+)\)', t2)]
                if abs(tx - (x0 + 1.27)) < 0.01 and abs(ty - (y1 - 1.27)) < 0.01:
                    old_rects.append((a2, b2))

    syms, pins = {}, {}
    for _, _, t in items:
        if not t.startswith("(symbol"):
            continue
        ref = re.search(r'\(property "Reference" "([^"]+)"', t)[1]
        lid = re.search(r'\(lib_id "([^"]+)"', t)[1]
        x0, y0, rot = map(float, re.search(
            r'\(at ([\d.\-]+) ([\d.\-]+) ([\d.\-]+)\)', t).groups())
        th = math.radians(-rot)

        def tf(px, py):
            vx, vy = px, -py
            return (x0 + vx * math.cos(th) - vy * math.sin(th),
                    y0 + vx * math.sin(th) + vy * math.cos(th))
        body = block(lib, f'(symbol "{lid}"')
        pts = []
        for kind, xs, ys in re.findall(r'\((xy|start|end|mid|center|at) ([\d.\-]+) ([\d.\-]+)', body):
            pts.append(tf(float(xs), float(ys)))
        for m in re.finditer(r'\(pin \w+ \w+\s+\(at ([\d.\-]+) ([\d.\-]+) \d+\)', body):
            pins[tuple(round(v, 2) for v in tf(float(m[1]), float(m[2])))] = ref
        for m in re.finditer(r'\(property "([^"]+)" "([^"]*)"\s+\(at ([\d.\-]+) ([\d.\-]+) ([\d.\-]+)\)(.*?)\n\t\t\)', t, re.S):
            if "(hide yes)" in m[6] or not m[2]:
                continue
            j = re.search(r'\(justify (left|right)', m[6])
            pts += text_box(float(m[3]), float(m[4]), int(float(m[5])) % 180 and 90 or 0,
                            len(m[2]), centered=True, just=j[1] if j else "")
        syms[ref] = pts

    wires = [tuple((round(float(x), 2), round(float(y), 2)) for x, y in
                   re.findall(r'\(xy ([\d.\-]+) ([\d.\-]+)\)', t))
             for _, _, t in items if t.startswith("(wire")]
    labels = []
    for _, _, t in items:
        if t.startswith("(label"):
            x, y, ang = re.search(r'\(at ([\d.\-]+) ([\d.\-]+) ([\d.\-]+)', t).groups()
            labels.append((re.search(r'\(label "([^"]+)"', t)[1],
                           (round(float(x), 2), round(float(y), 2)), int(float(ang))))

    add, report = [], []
    for refs, title in GROUPS:
        pts = [p for r in refs for p in syms[r]]
        gp = [p for p, r in pins.items() if r in refs]
        # Wires reachable from the group's pins, without crossing another symbol's pin.
        net_pts, mine, grew = set(gp), set(), True
        while grew:
            grew = False
            for i, (a, b) in enumerate(wires):
                if i not in mine and any(on_seg(p, a, b) or p in (a, b) for p in net_pts) and \
                        not any(pins.get(q) and pins[q] not in refs for q in (a, b)):
                    mine.add(i)
                    net_pts |= {a, b}
                    grew = True
        for i in mine:
            pts += list(wires[i])
        for name, p, ang in labels:
            if p in net_pts or any(on_seg(p, *wires[i]) for i in mine):
                pts += text_box(p[0], p[1], ang, len(name) + 1)
        xs, ys = [p[0] for p in pts], [p[1] for p in pts]
        x0, y0 = snap(min(xs) - PAD, False), snap(min(ys) - PAD, False)
        x1 = snap(max(max(xs) + PAD, x0 + 1.27 + len(title) * CHAR_W + 1.27), True)
        y1 = snap(max(ys) + PAD + TITLE_H, True)
        report.append((title.split("  ")[0], x0, y0, x1, y1))
        add.append(
            f'\t(rectangle\n\t\t(start {x0:g} {y0:g})\n\t\t(end {x1:g} {y1:g})\n'
            f'\t\t(stroke\n\t\t\t(width 0)\n\t\t\t(type dash)\n\t\t)\n'
            f'\t\t(fill\n\t\t\t(type none)\n\t\t)\n\t\t(uuid "{uuid.uuid4()}")\n\t)\n')
        add.append(
            f'\t(text "{title}"\n\t\t(exclude_from_sim no)\n\t\t(at {x0 + 1.27:g} {y1 - 1.27:g} 0)\n'
            f'\t\t(effects\n\t\t\t(font\n\t\t\t\t(size 1.27 1.27)\n\t\t\t)\n'
            f'\t\t\t(justify left bottom)\n\t\t)\n\t\t(uuid "{uuid.uuid4()}")\n\t)\n')

    out, pos = [], 0
    for a, b in sorted(old + old_rects):
        out.append(s[pos:a].rstrip("\t"))
        pos = b
        while pos < len(s) and s[pos] == "\n":
            pos += 1
    out.append(s[pos:])
    s2 = "".join(out)
    k = s2.rfind("\t(sheet_instances")
    SCH.write_text(s2[:k] + "".join(add) + s2[k:])
    for name, *box in report:
        print(f"{name:14} {box}")
    # Overlap check, including the DISPLAYS box drawn by display_block.py.
    rects = [(n, *b) for n, *b in report]
    for _, _, t in items:
        if t.startswith("(rectangle"):
            (a0, b0), (a1, b1) = [tuple(map(float, m)) for m in re.findall(
                r'\((?:start|end) ([\d.\-]+) ([\d.\-]+)\)', t)]
            if (a0, b0, a1, b1) not in [tuple(r[1:]) for r in rects]:
                rects.append(("(existing)", a0, b0, a1, b1))
    for i, (n1, *r1) in enumerate(rects):
        for n2, *r2 in rects[i + 1:]:
            if r1[0] < r2[2] and r2[0] < r1[2] and r1[1] < r2[3] and r2[1] < r1[3]:
                print(f"OVERLAP {n1} / {n2}")


if __name__ == "__main__":
    main()
