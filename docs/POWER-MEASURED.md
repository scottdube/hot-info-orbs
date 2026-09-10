# Power: measured, 2026-09-10

Every power figure in this project was an estimate until this. Measured on the
first assembled board — five GC9A01 displays, ESP32-S3 SuperMini, running the
ported firmware on WiFi.

**Instrument:** Nordic PPK II, source meter mode, 5.000 V, 10 kSa/s, 10 s window.

| | |
|---|---|
| Average | **341.79 mA** → **1.71 W** |
| Max (10 s window) | 399.87 mA → 2.00 W |
| Boot inrush | **>1 A for a few ms** — see the caveat below |

## What it settles

The estimate this project ran on was ~380 mA (five displays at 40–80 mA plus an
ESP32 averaging ~80 mA). Measured 342 mA, so **the estimate was 11% high** —
close enough that everything built on it stands.

**The 5 V decision is vindicated with numbers.** Upstream powers all five
displays from the module's own 3.3 V regulator. Had this load gone that way:

```
(5.0 − 3.3) × 342 mA = 581 mW in a SOT-23-5
  → junction 112–170 °C depending on copper
  → LDO thermal shutdown is 150–165 °C
```

At or past shutdown. That is what the field reports were describing — one
builder measured 82 °C **with the radio off, running only a graphics demo**.

As built, each display regulates its own 5 V and the module's LDO carries only
the ESP32: roughly 80 mA average, ~136 mW, ~52 °C. The part that was cooking is
no longer in the path.

## Caveat on the inrush figure

**The PPK II source meter tops out at 1 A, so ">1 A" is clipped, not measured.**
The true peak is unknown and at least 1 A. It lasts a few milliseconds — five
display regulators and their capacitors starting together.

Consequence worth designing around: a marginal USB supply can sag at power-on,
giving an orb that starts on one charger and not another, with nothing wrong
with the orb. The 220 µF bulk capacitor is doing real work there. If a unit ever
refuses to start on a particular supply, this is the first thing to suspect
rather than the firmware.

Measuring the true inrush needs an instrument with more headroom — a current
probe on a scope, or a sense resistor.

## For pricing and supply choice

1.71 W typical. Any USB source should be rated comfortably above the inrush
rather than the average: **2 A is a sensible floor**, and that costs nothing.
