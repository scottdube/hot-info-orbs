# Fab package — XIAO carrier, 46 mm display pitch, rev 2.4

**`hot_info_orbs_xiao_46mm_2.4_run1_gerbers.zip` is the package to send
(generated 2026-10-20 19:05, KiCad 10.0.3). It is named for the run and is not
to be regenerated:** if the board changes, the next run gets its own zip, and
this one stays byte-identical to what was ordered, so a returned board can be
matched to what was sent. The back silk reads `rev 2.4 · 46 mm pitch`.

`hot_info_orbs_xiao_46mm_2.3_run3_gerbers.zip` (2026-10-20 17:44) is the same
copper, mask, outline and drill (diffed: only the timestamp and rev-attribute
lines differ). It has no display marker on the front: `tools/display_marks.py`
added a Ø29.5 silk circle with the word Display at each module position so a
bare board shows which face the displays plug into, and Scott bumped the rev
to 2.4 for it. Superseded before anything was ordered; it stays as the
record, do not send it. Its STEP went to Yang by email that evening.

`hot_info_orbs_xiao_46mm_2.3_run2_gerbers.zip` (2026-10-20 15:04) is the same
copper, mask, outline and drill (diffed: only the timestamp lines differ);
it has the DS1–DS5 designators on the FRONT silk at the bottom edge, where
the fab would trim them and the display module would then cover them.
`tools/ds_refs.py` moved them to the back silk behind each socket. Superseded
before anything was ordered; it stays as the record, do not send it.

`hot_info_orbs_xiao_46mm_2.3_run1_gerbers.zip` is rev 2.3 as first laid out
the same day: back pour only, two big pour islands removed (the bare bands
Scott spotted from SW2 down to the XIAO and from DS1 to DS2). Superseded by
`tools/stitch_islands.py` (front GND pour + a stitching via in each island)
before anything was ordered; it stays as the record, do not send it.

`hot_info_orbs_xiao_46mm_2.1_run1_gerbers.zip` is rev 2.1 (2026-10-19, XIAO
4 mm further from the bottom edge, C1/C2/R1 in their old spots), superseded
by `tools/lower_xiao.py` before anything was ordered. It stays as the record
of what 2.1 was; do not send it. There was never a 2.2.

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
| T1 | 0.30 | 16 | vias (13 signal at 0.6 mm pad, 3 GND at 0.8 mm) |
| T2 | 0.80 | 6 | C1, C2, R1 leads |
| T3 | 0.99 | 6 | switch tab pads (2 per PTS645) |
| T4 | 1.00 | 49 | 5 × 7 display socket pins + 14 XIAO socket pins |
| T5 | 1.30 | 6 | switch signal pins |

The seven Ø3.20 mm mounting holes are `Edge.Cuts` circles the fab routes, not
drills, as on v1.1. Signal vias are 0.6 mm pad on 0.3 mm drill; GND vias
(the net class "Power Low Curent") are 0.8 on 0.3 (see the v1.1 fab notes in
git history, `6c64b43`, for the JLCPCB annular-ring caveat).

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
