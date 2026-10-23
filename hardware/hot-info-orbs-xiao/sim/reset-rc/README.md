# Display reset (R1/C2) simulation, 2026-10-23

The XIAO board has no GPIO left for the panels' RST, so all five share an RC
power-on reset: **R1 10 kΩ from the XIAO's 3V3, C2 1 µF to GND** (τ = 10 ms).
This folder asks whether that RC holds RST low long enough.

- `rst_rc.kicad_sch` is a standalone sheet holding only the reset network. Open
  `rst_rc.kicad_pro` in KiCad, then Inspect › Simulator, and plot `V(RST)`,
  `V(IOVCC)` and `V(RST_ASSERTED)`. Edit the `.param` line to change ramps.
- `make_sheet.py` regenerates the sheet from KiCad's own symbol libraries.
- `run_sweep.py` runs every case below in batch. It exports the sheet's netlist
  with `kicad-cli` (so the sheet is the only copy of the circuit) and drives the
  `libngspice` inside the KiCad app bundle. `kicad-cli` has no simulate command
  and there is no standalone ngspice on this Mac. `python3 run_sweep.py 10u diode` tries the
  fix below.

## What the sheet models

| Node | Is | Source |
|---|---|---|
| `3V3` | XIAO `3V3_OUT`, the R1 pull-up | board netlist |
| `IOVCC` | the display module's own 3.3 V. GC9A01 `VCC` is on **+5V** and each module regulates it | board netlist; `docs/POWER-MEASURED.md` |
| `I1` 5 µA | 5 panels × IIH max 1 µA, pulling RST down (the worst case for release) | GC9A01A datasheet DC table |
| `RST_ASSERTED` | 1 while IOVCC ≥ 1.65 V **and** RST < VIL = 0.3·IOVCC | same table: IOVCC 1.65–3.3 V, VIL 0.3·IOVCC, VIH 0.7·IOVCC |

**Not modelled:** the panels' input clamp diodes; any pull-up or RC on the
module itself (not checked); real rail shapes. Rails are linear ramps from
ideal sources. During a replug, an ideal source discharges C2 through R1 faster
than a real rail decaying under load would, so the replug rows are optimistic.

## What the datasheet does and doesn't say

GC9A01A datasheet (Waveshare mirror, `files.waveshare.com/upload/5/5e/GC9A01A.pdf`):

- RESX: *"must be applied to properly initialize the chip."* **No minimum
  pulse width and no reset-to-command time are published.** Related controllers
  in the ST7789 class specify 10 µs and 5/120 ms, but those are borrowed numbers,
  not GC9A01A specs.
- **The command list (§6.1) starts at 04h. There is no 01h software reset.**
  The firmware's fallback for `TFT_RST -1` is TFT_eSPI's
  `writecommand(TFT_SWRST)`, which is 0x01. Whether the chip acts on an
  undocumented 0x01 is **not known**. If it ignores it, this RC is the only
  reset the panels ever get.

## Results as drawn (1 µF, no diode)

`low` is how long the panels see RESX asserted. `release` is when RST rises
through VIH. `min ratio` is the lowest RST/IOVCC seen while IOVCC is valid.

| Case | low (ms) | release (ms) | min ratio |
|---|---|---|---|
| both rails rise in 0.1–5 ms | 3.8–3.9 | 12.6–15.2 | −0.03…0.09 |
| both rails 10 ms | 3.2 | 18.0 | 0.18 |
| both rails 15 ms | 1.0 | 21.0 | 0.27 |
| both rails 17 ms | 0.12 | 22.3 | 0.30 |
| **both rails ≥ 18 ms** | **0** | 22.9+ | ≥ 0.31 |
| IOVCC 1 ms, 3V3 30 ms (display rail first) | 17.3 | 31.1 | −0.03 |
| IOVCC 1 ms late, both 5 ms | 2.9 | 15.2 | 0.19 |
| **IOVCC 2 ms late, both 5 ms** | **0** | 15.2 | 0.32 |
| **IOVCC 30 ms, 3V3 1 ms** | **0** | 15.0 | 0.93 |
| **replug, power off ≤ 10 ms** | **0** | — | ≥ 0.35 |
| replug, off 15 / 50 ms | 1.4 / 3.7 | 25.8 / 63.0 | 0.23 / 0.01 |

**Verdict, as drawn: ⚠️ conditional.** When the reset happens, it lasts 1–4 ms,
far longer than any plausible minimum pulse. But it happens only if all three of
these hold:

1. both rails reach 3.3 V in under **~17 ms**;
2. the display's own regulator is no more than **~1 ms behind** the XIAO's 3V3;
3. a replug leaves power off for at least **~15 ms**.

Outside those limits the panels never see RESX at all, and nothing reports it.
**None of the three has been measured.** The XIAO board isn't built yet, and
the v1.1 boards drive RST from GPIO13, so they can't answer the question.

## The fix that passes every case simulated

| Variant | Cold start, rails 0.1–50 ms | IOVCC 2–5 ms late | IOVCC 30 ms late | Replug, off 1–10 ms | Release |
|---|---|---|---|---|---|
| 1 µF (as drawn) | fails at ≥ 18 ms | fails | fails | fails | 13–35 ms |
| 4.7 µF | 13.8–18.6 ms low | 12.8–15.8 ms | fails | fails | 59–86 ms |
| 10 µF | 37.9–38.9 ms | 32.9–35.9 ms | 23.4 ms | fails | 126–152 ms |
| **10 µF + diode across R1** (cathode to 3V3) | 37.9–38.9 ms | 32.9–35.9 ms | 23.4 ms | **18.0–22.7 ms** | 107–180 ms |

The diode dumps C2 into the rail as the rail collapses, so a quick replug starts
from 0 V. The model is a generic small-signal diode. Any small-signal or Schottky
part should behave the same; that wasn't simulated.

**The cost of 10 µF is a late release (up to ~180 ms after power-up).** If the
firmware sends its first panel command before RST rises, the panels lose the
init sequence. How soon after power-on `tft.init()` runs on the XIAO has not
been measured. Before adopting this, measure that time or add a guard
(`while (millis() < 250) {}` before the display init).

## Bench rig: one XIAO, one display, R1/C2 on a breadboard

This is valid for some questions and not others, because the board shares 5 V
and RST across five displays:

| Question | Valid with 1 display? | Why |
|---|---|---|
| Does the panel start with RESX held high (internal POR or 0x01)? | **Yes** | It's a property of the chip. |
| Skew between the XIAO's 3V3 and a display regulator | **Mostly** | Same parts, but the 5 V edge they both follow is shaped by the load. |
| 5 V and rail rise times | **No, optimistic** | Five modules' input caps plus C1 slow the 5 V edge; one display charges faster. Fit C1 100 µF; use all five displays if any are free. |
| Replug off-time | **No** | Rails collapse faster under five displays' load. |
| RST loading | **Only after an ohmmeter check** | If each module carries its own RST pull-up, five in parallel fight R1 five times harder than one does. Measure RST→VCC and RST→GND on an unpowered bare module first. |

Use the orb's real USB supply and cable, and the BOM C2 (KEMET C330C105K5R5, 50 V,
so DC-bias loss at 3.3 V is small). Scope channels: 5 V (trigger, rising,
single), XIAO 3V3, the display module's regulator output, RST; ~20 ms/div.
Repeat the plug-in ~10 times: USB contact bounce is part of the real case.

## The measurement that settles it

Scope at plug-in, with the 5 V supply the orb really uses: XIAO 3V3, one display
module's regulator output, and RST. That gives the rise times, the skew and the
RESX low time directly. Then feed the rise times back in with
`.param tr3=… tri=… tdi=…`. A second cheap test, on any board: power up with
RESX held high the whole time. If the panels still initialize, the chip has an
internal power-on reset or honors 0x01, and this RC is a backup rather than the
only reset.
