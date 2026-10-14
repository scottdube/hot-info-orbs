# Handoff — XIAO variant build log

Branch `xiao-esp32s3-plus`. Written so a fresh session (any model) can
continue without re-deriving anything. Newest at the bottom.

## Decisions (Scott, 2026-10-14)
- Edge pins only (headers or flat castellation solder). Not the Plus's
  back pads. Reason: members can build it with an iron.
- Pin map: see README.md in this folder. Display RST → RC power-on reset,
  firmware `TFT_RST -1`. BUSY → GPIO21 on-board LED.
- Tooling: KiCad 10 at /Applications/KiCad, its Python at
  `/Applications/KiCad/KiCad.app/Contents/Frameworks/Python.framework/Versions/Current/bin/python3`
  (pcbnew import works, ignore the fontconfig noise). Freerouting 2.4.1 jar at
  `~/Documents/KiCad/10.0/3rdparty/plugins/app_freerouting_kicad-plugin/jar/`,
  Java at `/opt/homebrew/opt/openjdk/bin/java` (brew-installed 2026-10-14).
  No kicad-cli on PATH: use
  `/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli`.
- KiCad must be CLOSED while scripts write the project files.
- Seeed's library is CC BY-SA 4.0 (not MIT — first draft said MIT, corrected).

## Steps
1. [x] Copy project to `hardware/hot-info-orbs-xiao/`, rename files, add
   Seeed symbol + DIP footprint, register libs.
2. [x] Schematic: `tools/swap_to_xiao_sch.py` (run once against e6ea813).
   ERC 0. Symbol trimmed to the 14 edge pins and renamed
   `XIAO-ESP32-S3-edge` so PCB parity has no pad-less pins.
3. [x] PCB. Pipeline, each run from the e6ea813 copy of the .kicad_pcb:
   `tools/merge_xiao_pads.py` (once, edits the footprint in lib/) →
   `tools/swap_to_xiao_pcb.py` → `tools/route_xiao.py` (Freerouting,
   headless, ~5 s). Result: 0 errors, 0 unconnected, 0 parity, 14 warnings
   = Seeed's own B.Silk outline crossing its pads (cosmetic, fab clips
   silk off pads; accepted). 276 tracks+vias, GND routed as tracks with
   the pour on top.
   Traps found on the way (all written into the scripts' docstrings):
   - `FOOTPRINT.SetField(FIELD_T_DESCRIPTION, txt)` creates a stray user
     field "5" that prints on silk — use `GetField(..).SetText()`.
   - board texts are left/top justified; SetPosition moves the corner.
   - a second `pcbnew.LoadBoard()` in one interpreter, and the board after
     `ImportSpecctraSES`, come back as bare SwigPyObjects — one board per
     process. `Zones()` breaks after `Remove(track)`: take zones first.
   - with the GND pour in the DSN Freerouting routes nothing for GND and
     the tracks cut the pour into islands — export without the pour.
   - Seeed's two pads per pin (PTH + castellation SMD) gave Freerouting 14
     "violations", a stub per pin and one unrouted net per run; merged
     into one oval THT pad per pin (hole at ±7.62, copper to ±9.671).
     Hand-routing the leftover net was tried first and just moved the
     failure to the next net — rejected.
4. [x] Firmware (commit after 4ef5421): `[env:xiao-esp32s3]` + `[env:xiao-ota]`
   in platformio.ini (board `seeed_xiao_esp32s3`, 8 MB, same partitions.csv,
   `-D BOARD_XIAO_ESP32S3`). config.h.template: XIAO pin block under
   `#if defined(BOARD_XIAO_ESP32S3)`, SuperMini block verbatim in the
   `#else`. TFT_RST -1 (RC reset + TFT_eSPI's TFT_SWRST). BUSY_PIN 21 with
   `BUSY_ON LOW / BUSY_OFF HIGH`; Widget.cpp `setBusy` uses the macros and
   defaults them to HIGH/LOW so the SuperMini build is unchanged. Both envs
   compile (xiao 84.0 % flash of the 1.9 MB app slot, devkitc 85.0 %).
   The board json sets USB CDC on boot — that is the XIAO's normal console,
   the TRAPS.md warning was about the SuperMini's build. Not flashed: no
   XIAO board exists yet.
   Trap: config.h reaches the build through `-include config_helper.h`, and
   SCons does not track that, so after editing config.h an incremental
   `pio run` rebuilt nothing (flash size byte-identical to the SuperMini
   build). `pio run -e xiao-esp32s3 -t clean` first.
5. [x] Docs: `docs/VARIANT-XIAO-ESP32S3.md` (new), README.md row + intro,
   features.html Advanced card rewritten from "adapter, proposed" to
   "second carrier, designed not built", this folder's README (status,
   regeneration recipe, library table). 3D STEP NOT regenerated.

6. [x] XIAO-only (Scott, 2026-10-14: "this branch is xiao only", S3 and S3
   Plus interchangeable). SuperMini env, pin block and `hardware/hot-info-orbs/`
   removed from the branch (all on `main`, tag `v1.1-run1`). Default envs are
   now `xiao-esp32s3, ota`; no `BOARD_XIAO_ESP32S3` define any more. Docs
   (README, SETUP, TRAPS, TEMPEST-API, features.html Basic = S3 / Advanced =
   Plus) rewritten; VARIANT-ESP32-S3-SUPERMINI.md kept as history.
   Interchangeability verified against the Seeed wiki (same D0-D10 map, LED,
   size, PSRAM; flash 8 vs 16 MB) - build stays 8 MB so one binary fits both.

## Open
- Antenna: the XIAO uses a U.FL external antenna, not a PCB antenna. The
  board's "Antenna" silk + keepouts are SuperMini leftovers; decide where the
  pigtail antenna goes.
- Order / build / flash a board; measure the RC reset and LED polarity.
- Regenerate the 3D STEP for the XIAO board.
- Branch has no upstream until pushed (`git push -u origin xiao-esp32s3-plus`).
