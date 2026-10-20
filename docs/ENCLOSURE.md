# Enclosure

## Chosen style: Concept 3, "Winged Speedster" (2026-10-17)

![Concept 3 – Winged Speedster](assets/enclosure-concept-3-winged-speedster.png)

This is a concept rendering, not a model. It sets the look the design aims for:

- A Jet Age bar: dark blue-grey body, a brass ring bezel around each display,
  and finned brass end caps.
- Flat front and back, with the five 1.28" displays parallel to the PCB.
- Designed for 3D printing in several parts.

**Change from the rendering: the three buttons move to the top.** The rendering
shows them on the front, under the displays. The XIAO carrier board
(`hardware/hot-info-orbs-xiao/`, branch `xiao-esp32s3-plus`) already puts its
right-angle switches on the top edge, so the board needs no change for this.

## Inputs already measured

| What | Value | Source |
|---|---|---|
| Bezel opening (lit area) | **~32.5 mm** dia; shows every pixel | [DISPLAY-EDGE-TEST.md](DISPLAY-EDGE-TEST.md) |
| Dark glass band outside it | ~1.5 mm, even all round, no pixels under it | same |
| Module pocket | the GC9A01 STEP model, caliper-checked by Scott | `hardware/hot-info-orbs/lib/3d/_Displays/` |

A bezel opening wider than ~32.5 mm shows the black band inside the brass ring.

## Front mock-up v1 (2026-10-19)

A printable front that drops over the real board: a face plate plus five
separate bezel rings (37 mm OD, 32.5 mm window). Its top and bottom margins are
equal, 29.135 mm. The interference check against the board STEP is clean. See
[hardware/enclosure/front-mockup-v1/](../hardware/enclosure/front-mockup-v1/README.md).
