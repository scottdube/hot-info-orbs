# HoT Info Orbs — KiCad project

ESP32-S3 SuperMini carrier for five GC9A01 displays. Original board by Yangmin
Shen, KiCad 10, received 2026-08-28.

## Libraries are project-local, on purpose

The custom symbols and footprints live in `lib/` and are wired up through
`sym-lib-table` and `fp-lib-table` using `${KIPRJMOD}`. **Do not move them into
your global KiCad libraries.** Clone the repo, open the project, and everything
resolves with no setup.

This is deliberate. Upstream's board (`PCBFiles+Schematics/Main PCB`) references
libraries named `A Local Libs` and `New folder` that exist only on its author's
machine — the footprints render because KiCad caches them inside the board file,
but nobody else can place a new instance or update one. Project-local tables
avoid that permanently.

| Nickname                 | Provides                     |
|--------------------------|------------------------------|
| `_AZDelivery components` | `GC9A01_TFT` symbol          |
| `_ESP32 modules`         | SuperMini symbol + footprint |
| `_Displays`              | `GC9A01_PINS` footprint      |
| `_Logos`                 | `Hot_Logo_5x5mm` footprint   |

**`_Logos` was not in the zip.** It was recovered by extracting the embedded
footprint from `hot_info_orbs.kicad_pcb`, since KiCad caches full definitions in
the board file. It is byte-equivalent to what the board already places, but it
has not been round-tripped through the footprint editor — if it misbehaves, ask
Yang for the original.

`.kicad_prl` is deliberately not committed: it is per-user editor state
(open tabs, zoom, last-used layer) and only creates conflicts.
