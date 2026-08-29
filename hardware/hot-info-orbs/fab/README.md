# Fab package

Generated from `hot_info_orbs.kicad_pcb` with `kicad-cli` (KiCad 10.0.3).
**Upload `hot_info_orbs_gerbers.zip` to JLCPCB as-is.**

Regenerate with:

```bash
KC=/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli
"$KC" pcb export gerbers --output fab/gerbers \
  --layers "F.Cu,B.Cu,F.Mask,B.Mask,F.Silkscreen,B.Silkscreen,Edge.Cuts" \
  hot_info_orbs.kicad_pcb
"$KC" pcb export drill --output fab/gerbers --format excellon --excellon-units mm \
  hot_info_orbs.kicad_pcb
cd fab/gerbers && zip ../hot_info_orbs_gerbers.zip *.gbl *.gbs *.gbo *.gm1 *.gtl *.gts *.gto *.drl
```

**Only those seven layers plus the drill file.** KiCad's default export also
emits Courtyard, Fab, Adhesive, Paste and User_1-9. None of it is used for a
bare board, and extra layers are how fab queries start.

## Order parameters

| | |
|---|---|
| Layers | 2 |
| Dimensions | 198.8 × 28.5 mm |
| Thickness | 1.6 mm |
| Min quantity | 5 (price per board drops steeply above that) |

Board exceeds 100 × 100 mm, so it does not fall in the cheapest tier.

## What the drill file should contain

Cross-check after any regeneration — the counts come from the footprints, so a
mismatch means something was added or lost:

| Tool | Holes | |
|---|---:|---|
| 0.30 mm | 21 | vias |
| 0.80 mm | 2 | C1 |
| 1.00 mm | 53 | 5×7 display headers + 18 SuperMini pins |
| 1.10 mm | 12 | 3 buttons × 4 pads |

**The four mounting holes are deliberately not in the drill file.** They are
Ø3.20 mm circles on `Edge.Cuts`, so the fab routes them as cutouts rather than
drilling them. If they ever appear in the drill file, somebody converted them to
a footprint and that changes how they are made.

## Verified before generating

ERC 0, DRC 0, 0 unconnected pads, 0 schematic parity issues.
