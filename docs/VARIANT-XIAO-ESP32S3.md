# Variant: Seeed XIAO ESP32-S3 Plus

Status: **DESIGNED, NOT BUILT.** Branch `xiao-esp32s3-plus`, 2026-10-14.
Schematic and board are done and DRC-clean, the firmware env compiles, and
no board has been ordered or flashed. The SuperMini carrier
(`hardware/hot-info-orbs`, v1.1) is untouched and stays the club board.

Date: 2026-10-14

---

## The question

The features sheet proposed the XIAO ESP32-S3 Plus as the "Advanced" module,
reached through an adapter that plugs into the v1.1 board's SuperMini socket.
Scott asked instead for the carrier itself reworked to take the XIAO, with the
old board project kept. This records what that took.

## Decision: a second carrier, not an adapter

`hardware/hot-info-orbs-xiao/` is a copy of the v1.1 project with U2 swapped.
Everything else — the five display positions, three buttons, the 5 V display
supply, the ground pour, the outline — is the v1.1 board. Two projects rather
than one with a configurable U2, because KiCad has no clean way to carry two
footprints for one symbol and the two boards will be ordered separately
anyway.

## Mounting: edge pins only

Scott's choice (asked 2026-10-14). The XIAO has 14 castellated edge pins and,
on the Plus, nine more pads on its underside. Only the edge pins are used, so
the module goes on in either of two ways a member can do with an iron:
2.54 mm headers (socket or pin), or soldered flat on its castellations. Each
pin on the carrier is one oval through-hole pad spanning hole and
castellation edge, so the same copper serves both. Consequences:

- any XIAO ESP32-S3 (plain, Sense, Plus) fits — they share the edge pins;
- the Plus's back pads, and the extra GPIO on them, are given up;
- 11 GPIO on the edge against the SuperMini board's 12.

## The twelfth pin

The SuperMini board drives the displays' shared RST from a GPIO. On the XIAO
there is no pin left for it, so RST is an RC power-on reset (R1 10 kΩ to
3V3, C2 1 µF to GND) and the firmware builds with `TFT_RST -1`, which makes
TFT_eSPI issue the panel's software reset command (`TFT_SWRST`) in `init()`.
The GC9A01 accepts that; the hardware line only has to be high by the time
the MCU is out of reset, and a 10 ms RC against the ESP32-S3's boot time is.

The BUSY indicator, which on the SuperMini board is an external LED on
GPIO15, becomes the XIAO's own user LED on GPIO21. That LED is wired to 3V3,
so it is active-low; `BUSY_ON`/`BUSY_OFF` macros in `config.h` carry the
polarity and `Widget::setBusy` uses them, defaulting to the old HIGH/LOW so
the SuperMini build is unchanged.

## Pin assignment and why

| Signal | XIAO | GPIO | Reason it went there |
|---|---|---|---|
| SW1 / SW2 / SW3 | D0 / D1 / D5 | 1 / 2 / 6 | buttons kept off the UART pins: a button held through a reset would short 3V3 into GPIO43 while the ROM drives it |
| DS1..DS3 CS | D2 / D3 / D4 | 3 / 4 / 5 | GPIO3 is a strapping pin, but a CS is an MCU-driven output that idles high — its default level |
| DS4 / DS5 CS | D6 / D7 | 43 / 44 | UART0 TX/RX; the ROM boot log toggles TX before init, harmless on a chip select |
| SCLK / DC / MOSI | D8 / D9 / D10 | 7 / 8 / 9 | |
| 5V / GND / 3V3 | | | displays run from 5 V through their own regulators, as on v1.1 |

Full map with notes: `hardware/hot-info-orbs-xiao/README.md`.

## Firmware

`pio run -e xiao-esp32s3` (board `seeed_xiao_esp32s3`, `-D BOARD_XIAO_ESP32S3`);
`xiao-ota` for espota. `config.h.template` has the XIAO pin block under
`#if defined(BOARD_XIAO_ESP32S3)` with the SuperMini block verbatim in the
`#else`. The XIAO board definition turns on USB CDC on boot itself: the
XIAO's console *is* the native USB (no bridge chip), so the TRAPS.md warning
against those flags — written for the SuperMini — does not apply.

Flash: the generic XIAO board json declares 8 MB; the Plus has 16 MB. The
build keeps v1.1's `partitions.csv` (two 1.875 MB app slots, laid out for
4 MB) so both boards run one OTA layout. Using the rest means
`board_upload.flash_size = 16MB` and a new partition table — not done, not
needed yet. Measured 2026-10-14: the XIAO build fills 84.0 % of an app slot.

Trap: `config.h` reaches the compiler via `-include config_helper.h`, which
SCons does not track as a dependency. After editing `config.h` an incremental
build produced a binary byte-identical to the SuperMini one. Clean the env
first.

## Board work — what was tried and rejected

- Freerouting with Seeed's footprint as shipped (a PTH pad *and* a
  castellation SMD pad per pin) reported 14 "violations", put a stub on every
  pin and left one net unrouted on every run. Hand-routing that net moved the
  failure to the next net — rejected. Merging each pair into one oval THT pad
  (`tools/merge_xiao_pads.py`) gave 0 violations and a complete route, and is
  the footprint the two mounting methods want anyway.
- Exporting the DSN with the GND pour present makes Freerouting treat GND as
  a plane and route nothing for it; the new tracks then slice the pour into
  islands (3 starved thermals, 2 unconnected). The pour is exported out and
  poured back on top of routed GND tracks.
- Result: 0 DRC errors, 0 unconnected, 0 schematic-parity issues, 14 silk
  warnings that are Seeed's own module outline crossing its pads (accepted;
  fabs clip silk off copper). Pipeline and traps: `hardware/hot-info-orbs-xiao/docs/HANDOFF.md`.

## Not done

- No board ordered, nothing flashed; the RC reset timing and the active-low
  LED are reasoned, not measured.
- 3D STEP not regenerated for the XIAO board.
- `docs/features.html` still lists the module price from 24 Sept 2026.
