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
3. [~] PCB: `tools/swap_to_xiao_pcb.py` (run once against e6ea813) places
   U2/R1/C2, re-nets every pad, rips tracks, fixes silk. Then
   `kicad-cli pcb drc --schematic-parity --severity-all --refill-zones
   --save-board` → 0 errors, 0 parity, 45 unconnected (expected, unrouted),
   14 warnings = Seeed's own footprint outline crossing its castellation
   pads (cosmetic, fab clips silk off pads; accepted). **Routing still to
   do**: `tools/route_xiao.py` (Freerouting) is next.
   Trap: `FOOTPRINT.SetField(FIELD_T_DESCRIPTION, txt)` creates a stray user
   field named "5" that prints on silk — use `GetField(..).SetText()`.
   Trap: board texts are left/top justified; SetPosition moves the corner.
4. [ ] Firmware: `xiao-esp32s3` PlatformIO env + pin block in
   config.h.template (board json `seeed_xiao_esp32s3`; it sets USB CDC on
   boot — that is the XIAO's normal console, the TRAPS.md warning was about
   the SuperMini's build).
5. [ ] Docs: features.html / VARIANT doc mention; 3D step not regenerated.
