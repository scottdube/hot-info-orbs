#!/usr/bin/env python3
"""Rev 2.3 run 3: put the DS1-DS5 designators on the BACK silk, behind each
display socket, readable on the assembled board (KiCad CLOSED, run once).

  .../python3 tools/ds_refs.py

Why (Scott, 2026-10-20): "move the display id tag ie DS_1 to the other side
so it is not off board and shows in production board ... it should be
visible for troubleshooting with noobs". As laid out, each DSn reference
sat on F.Silkscreen at y 109.03, its bottom 0.015 mm from the board edge
(109.78): the fab trims silk outside the outline, so half a letter could go,
and whatever survived was under the display module once it was plugged in.
Nobody holding a built board could read which socket is DS1.

Where it goes: B.Silkscreen, mirrored, centered on the socket's pad row,
centered at y 103.3, glyph strokes 0.8 mm above the pad copper
and 0.58 mm below the rectangle line (the "4" is the tallest glyph). On the back each socket footprint draws
an outline rectangle y 101.6..107.8 whose top band (101.6..105.13, above the
pad row) holds nothing but mask-covered tracks (RST/SCLK/CS runs at y 104.8,
the GND bus at 107.3): silk over masked copper is allowed, silk over pads is
not. Clearance is measured on the stroked glyph outline, not KiCad's text
box, which pads ~0.5 mm all round and would report 0.46 mm to the pads. Size 1.5 mm / 0.25
thick, bigger than the 1.0 mm reference text elsewhere, because the point is
reading it across a bench. Rejected the front above the socket: the pin
labels RST..VCC occupy y 101.9..105.1 there and the module covers it anyway.
Rejected 2.0 mm text: it fits (bbox 2.3 tall) but leaves 0.5 mm to the
rectangle line and 0.7 mm to the pads, and silk-to-silk 0.5 mm is where
JLCPCB's silk starts to bleed together.

The F.Fab copies (`${REFERENCE}` at y 110.85, the GC9A01_TFT value at
112.35) stay off board: Fab layers are not sent to the fab.
"""
from pathlib import Path
import pcbnew

HERE = Path(__file__).resolve().parent.parent
PCB = HERE / "hot_info_orbs_xiao_46mm.kicad_pcb"
MM, T = pcbnew.FromMM, pcbnew.ToMM
V = pcbnew.VECTOR2I

Y_REF = 103.3           # text center; pad copper starts at 105.13, rect line at 101.6
RECT_TOP = 101.7        # lower edge of the footprint's back silk rectangle line
SIZE, THICK = 1.5, 0.25
CLR = 0.5               # glyph outline to pad copper and to the rectangle line

b = pcbnew.LoadBoard(str(PCB))
log = []
for f in sorted(b.GetFootprints(), key=lambda f: f.GetReference()):
    if not f.GetReference().startswith("DS"):
        continue
    pads = list(f.Pads())
    xc = sum(T(p.GetPosition().x) for p in pads) / len(pads)
    pad_top = min(T(p.GetPosition().y) - T(p.GetSize().y) / 2 for p in pads)
    r = f.Reference()
    r.SetLayer(pcbnew.B_SilkS)
    r.SetMirrored(True)
    r.SetTextAngleDegrees(0)
    r.SetTextSize(V(MM(SIZE), MM(SIZE)))
    r.SetTextThickness(MM(THICK))
    r.SetPosition(V(MM(xc), MM(Y_REF)))
    bb = r.GetEffectiveShape().BBox()      # the strokes, not the padded text box
    top, bot = T(bb.GetTop()), T(bb.GetBottom())
    assert bot + CLR <= pad_top, (f.GetReference(), bot, pad_top)
    assert top >= RECT_TOP + CLR, (f.GetReference(), top)
    log.append(f"{f.GetReference()} -> {r.GetLayerName()} mirrored, x {xc:.2f} y {Y_REF}; "
               f"glyphs y {top:.2f}..{bot:.2f}, {pad_top - bot:.2f} mm above pad copper")

pcbnew.SaveBoard(str(PCB), b)
print("\n".join(log))
