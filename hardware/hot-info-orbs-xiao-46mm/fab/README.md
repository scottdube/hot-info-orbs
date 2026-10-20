# Fab package — XIAO carrier, 46 mm display pitch, rev 2.1

**`hot_info_orbs_xiao_46mm_2.1_run1_gerbers.zip` is the package for the first
run of this board (generated 2026-10-19, KiCad 10.0.3). It is named for the run
and is not to be regenerated:** if the board changes, the next run gets its own
zip, and this one stays byte-identical to what was ordered, so a returned board
can be matched to what was sent. The back silk reads `rev 2.1 · 46 mm pitch`.

`hot_info_orbs_xiao_46mm_X0.2_run1_gerbers.zip` is the 47.78 mm-tall board
from earlier the same day, superseded by the 5 mm top-edge grow
(`tools/grow_top.py`) before anything was ordered. It stays as the record of
what X0.2 was; do not send it.

Seven layers plus the drill file, the same set the v1.1 run used:
F.Cu, B.Cu, F.Mask, B.Mask, F.Silkscreen, B.Silkscreen, Edge.Cuts, Excellon
drill in mm. Nothing else (no courtyard/fab/paste/user layers).

Drill tools cross-checked against the footprints:

| Tool | Drill | Hits | What |
|---|---|---|---|
| T1 | 0.30 | 6 | vias |
| T2 | 0.80 | 6 | C1, C2, R1 leads |
| T3 | 0.99 | 6 | switch tab pads (2 per PTS645) |
| T4 | 1.00 | 49 | 5 × 7 display socket pins + 14 XIAO socket pins |
| T5 | 1.30 | 6 | switch signal pins |

The seven Ø3.20 mm mounting holes are `Edge.Cuts` circles the fab routes, not
drills, as on v1.1. Vias are still 0.6 mm pad on 0.3 mm drill (see the v1.1
fab notes in git history, `6c64b43`, for the JLCPCB annular-ring caveat).

Regenerate (a new run, new zip name):

```bash
KC=/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli
"$KC" pcb export gerbers --output fab/gerbers \
  --layers "F.Cu,B.Cu,F.Mask,B.Mask,F.Silkscreen,B.Silkscreen,Edge.Cuts" \
  hot_info_orbs_xiao_46mm.kicad_pcb
"$KC" pcb export drill --output fab/gerbers --format excellon --excellon-units mm \
  hot_info_orbs_xiao_46mm.kicad_pcb
cd fab/gerbers && zip ../hot_info_orbs_xiao_46mm_<rev>_run<n>_gerbers.zip *.gbl *.gbs *.gbo *.gm1 *.gtl *.gts *.gto *.drl
```
