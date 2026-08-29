# Fab package

## FIRST PRODUCTION RUN — v1.1

**`hot_info_orbs_v1.1_run1_gerbers.zip` is the package the first boards were
ordered from.** Generated 2026-08-29 from the board tagged `v1.1-run1`.

Do not regenerate this file to "refresh" it. If the board changes, the next run
gets its own zip and its own tag, and this one stays exactly as ordered — the
whole point is that when a board comes back, there is no question what was sent.

Verified before generating: ERC 0, DRC 0 unconnected pads, 0 schematic parity
issues. One silkscreen-overlap warning on B.Silkscreen near U2, accepted as
cosmetic.

Drill tools cross-checked against the footprint count:

| Tool | Holes | |
|---|---:|---|
| 0.30 mm | 18 | vias |
| 0.80 mm | 2 | C1 |
| 1.00 mm | 53 | 5×7 display headers + 18 module pins |
| 1.10 mm | 12 | 3 buttons × 4 pads |
| | **85** | total |

The four Ø3.20 mm mounting holes are deliberately absent from the drill file —
they are `Edge.Cuts` circles the fab routes, not drills.

### Via geometry — RESOLVED, no issue

Earlier drafts flagged the 0.6 mm / 0.3 mm vias as a possible problem: a
0.150 mm annular ring against a "0.18 mm minimum annular ring" figure on
JLCPCB's capabilities page. **That was the wrong number.**

JLCPCB's order form states the rule as a *diameter* difference, not a ring:

> "Via diameter should be 0.1mm (0.15mm preferred) larger than Via hole size."

For a 0.3 mm hole that means a 0.4 mm diameter minimum, 0.45 mm preferred. These
vias are **0.6 mm on a 0.3 mm hole — 0.3 mm larger, three times the minimum and
twice the preferred.** Not close to a limit.

The same tooltip also settles cost:

> "No additional charge when the via hole ≥0.3mm, and via diameter ≥0.4mm."

0.3 mm hole and 0.6 mm diameter meet both conditions, so there is no via
surcharge on this order.

**Lesson worth keeping:** the capabilities page and the order form express the
same constraint differently, and the capabilities page's figures are not
internally consistent. When a number decides something, take it from the order
form.

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
