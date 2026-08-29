# Fab package

> **THE GENERATED FILES HAVE BEEN REMOVED ON PURPOSE.** They were built from the
> board before Yang's v2 reroute and are wrong for the current design. A stale
> gerber zip that looks ready is exactly what gets uploaded by accident.
> Regenerate with the command below once the board is final — the zones need
> filling and the GPIO3 button move is still outstanding.

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

## Design rules

Set 2026-08-28 from JLCPCB's published capabilities for standard 2-layer, 1 oz.
They were all `0.0` before, so DRC was checking nothing.

| Rule | Value |
|---|---|
| Min clearance | 0.10 mm |
| Min track width | 0.10 mm |
| Min through-hole | 0.15 mm |
| Min via diameter | 0.25 mm |
| Min annular width | **0.15 mm — see below** |
| Min copper-to-edge | 0.5 mm (JLC allows 0.2; the board already passes at 0.5) |

The board uses 0.2 and 0.35 mm tracks, so it has 2–3.5× margin on width.

### The annular ring number is the one to confirm

JLCPCB's capabilities page states a minimum annular ring of 0.18 mm,
"recommended 0.25 mm or above". This board's 17 smaller vias are 0.6 mm pad on a
0.3 mm drill, giving **0.150 mm** — under that figure.

Two reasons it is set to 0.15 here rather than 0.18:

1. **The quoted 0.18 is not internally consistent** with the rest of that page,
   which also gives a 0.15 mm minimum drill and a 0.25 mm minimum via diameter —
   0.15 + 2×0.18 would be 0.51 mm, not 0.25. The figures appear to span
   different capability tiers, so 0.18 should not be treated as settled.
2. **0.6/0.3 is KiCad's default via** and is what an enormous number of boards
   are fabricated with at JLC every day.

**Enlarging the vias was tried and made things worse.** Taking the 17 vias to
0.8 mm gave the required ring but produced 24 clearance violations, because the
larger pads crowd neighbouring copper. Fixing it properly means moving traces —
real layout work on someone else's board, not a settings change.

**If certainty is wanted before ordering, ask JLC directly whether 0.6/0.3 vias
are acceptable on a standard 2-layer order.** Nothing else on the board is close
to a limit.
