# HoT Info Orbs — XIAO ESP32-S3 variant (KiCad project)

Carrier for five GC9A01 displays and three buttons, taking a
**Seeed Studio XIAO ESP32-S3 Plus** (SKU 102010671) in place of the
ESP32-S3 SuperMini. Copied from `../hot-info-orbs` (v1.1, the board that was
ordered 2026-08-29) on 2026-10-14; that project is untouched and stays the
SuperMini board.

## Status — IN PROGRESS, not fabricated

See `docs/HANDOFF.md` in this folder for exactly where the work stands.

## Module fit

Only the 14 castellated edge pins are used, so the footprint
(`_ESP32 modules:XIAO-ESP32-S3-DIP`, Seeed's own, CC BY-SA 4.0) takes the module on
2.54 mm headers **or** soldered flat on its castellations. Any XIAO ESP32-S3
(plain, Sense, Plus) has the same 14 edge pins and fits; the Plus's nine extra
pads under the module are not used.

## Pin map (decided 2026-10-14)

11 GPIOs are available on the edge; the SuperMini board used 12, so the
display RST line moves off the MCU onto an RC power-on reset (R1 10 kΩ to
3V3, C2 1 µF to GND) and the firmware uses TFT_eSPI's software reset
(`TFT_RST -1`). The BUSY indicator goes to the XIAO's on-board user LED
(GPIO21, active LOW), which is not an edge pin.

| XIAO pin | GPIO | Signal | Note |
|---|---|---|---|
| D0 | 1 | SW1 (left) | same GPIO as the SuperMini board |
| D1 | 2 | SW2 (OK) | same GPIO as the SuperMini board |
| D2 | 3 | DS1 CS | strapping pin (JTAG select); MCU-driven output, no external pull, so harmless |
| D3 | 4 | DS2 CS | |
| D4 | 5 | DS3 CS | |
| D5 | 6 | SW3 (right) | |
| D6 | 43 | DS4 CS | UART0 TX: ROM boot log toggles it before init, harmless on a CS |
| D7 | 44 | DS5 CS | UART0 RX: pulled up at boot, so CS idles high |
| D8 | 7 | SCLK | |
| D9 | 8 | DC | |
| D10 | 9 | MOSI | |
| 5V | — | +5V | displays run from 5 V through their own regulators |
| GND | — | GND | |
| 3V3 | — | 3V3 | button pull side + RST pull-up |

Buttons stay on D0/D1/D5 rather than the UART pins: a button held through a
reset would short 3V3 into GPIO43 while the ROM is driving it.

## Libraries

Project-local, as in the parent project. Added here:

| Nickname | Provides | Source |
|---|---|---|
| `_Seeed XIAO` | `XIAO-ESP32-S3-SMD` symbol (14-pin) | Seeed `OPL_Kicad_Library`, CC BY-SA 4.0 |
| `_ESP32 modules` | `XIAO-ESP32-S3-DIP` footprint (THT + castellation pads) | same |

The SuperMini symbol/footprint are still in `lib/` but unused.
