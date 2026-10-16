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
   regeneration recipe, library table). 3D STEP regenerated in step 8.

6. [x] XIAO-only (Scott, 2026-10-14: "this branch is xiao only", S3 and S3
   Plus interchangeable). SuperMini env, pin block and `hardware/hot-info-orbs/`
   removed from the branch (all on `main`, tag `v1.1-run1`). Default envs are
   now `xiao-esp32s3, ota`; no `BOARD_XIAO_ESP32S3` define any more. Docs
   (README, SETUP, TRAPS, TEMPEST-API, features.html Basic = S3 / Advanced =
   Plus) rewritten; VARIANT-ESP32-S3-SUPERMINI.md kept as history.
   Interchangeability verified against the Seeed wiki (same D0-D10 map, LED,
   size, PSRAM; flash 8 vs 16 MB) - build stays 8 MB so one binary fits both.

7. [x] Antenna patch (2026-10-14, Scott: "fix that"). The XIAO has a U.FL
   and a stick-on FPC antenna (Seeed A-02, inventory part 102), not a PCB
   antenna, so the inherited "Antenna" keepouts/silk were wrong.
   `tools/antenna_patch.py` (run on the e6ea813-derived board BEFORE
   `route_xiao.py`): removes the two SuperMini keepouts and both "Antenna"
   texts, adds rule area `ANTENNA_PATCH` on F.Cu + B.Cu (no pour/tracks/vias)
   at x 143.5–183.5, y 99.5–107.0 = **40 × 7.5 mm** on the bottom strip
   right of the XIAO, plus a B.Silk box and label "STICK FPC ANTENNA HERE".
   Re-routed: 0 errors, 0 unconnected, 0 parity, 14 silk warnings (same),
   268 tracks+vias, GND pour has no fill inside the patch.
   Height was measured against the router, not chosen: 10.5 mm left 3 nets
   unrouted, 9.0 mm left CS2 unrouted, 7.5 mm routes; narrowing instead
   (x from 150 or 155) did not help, x 160.5–183.5 routes at 10.5 mm but is
   only 23 mm long. The bottom strip under DS3 is the bus's second channel
   and needs ~4 mm. **The patch size is an assumption** — Seeed publishes no
   A-02 dimensions; measure one and rerun with `PATCH=x0,x1,y0,y1`.
   Traps: `ZONE.Outline()` returns a bare SwigPyObject (no NewOutline) —
   build the outline with `z.AppendCorner(VECTOR2I, -1)`; Freerouting can
   write a **0-byte SES** when nets are left unrouted, which `route_xiao.py`
   then failed to import with no hint — it now asserts size > 0 and passes
   `-mt 1` (2.4.1 warns its multi-threaded optimizer is broken).

8. [x] 3D models (2026-10-14, Scott: "add the xiao 3d model"). Scott's own
   STEP export that morning logged six "Could not add 3D model": U2's
   AMz3D footprint carried five `${AMZPATH}` models (AMZPATH undefined here)
   and DS1–DS5 on the board still said `${KICAD_USER_3DMODEL_DIR}` from v1.1
   while the library footprint already said `${KIPRJMOD}`. `tools/xiao_model.py`
   (KiCad closed) puts Seeed's "XIAO ESP32S3 PLUS v4.step" (assembly
   'XIAO ESP32S3 PLUS_V1.0_241025', from Downloads, copied to
   `lib/3d/_ESP32 modules/XIAO-ESP32S3-PLUS-v4.step`) on U2 in the board
   and the library footprint, and rewrites the five DS model paths to
   `${KIPRJMOD}/lib/3d/_Displays/`. Fit found by rendering
   (`kicad-cli pcb render --side bottom --zoom 9 --pivot "-1.97,0,0"`):
   offset (-1.778, -7.62, 0), rotate (-90, 0, 0) — Seeed's STEP stands the
   module on edge, and at zero offset it sat one castellation column over.
   Then `kicad-cli pcb export step --subst-models --include-silkscreen
   --include-soldermask --grid-origin --min-distance=0.001mm` →
   `3d-assembly/hot_info_orbs_xiao_assembly.step`, no missing models.
   One model only, the Plus: the plain S3 has the same outline and USB end.
   Traps: `FOOTPRINT.Models()` iterates copies — setting `m_Filename` on
   them changes nothing; rebuild the list with `clear()` + `push_back()`.
   KiCad's STEP export dialog defaults the output name to the project's
   `~….kicad_pro.lck` lock file; Scott's export landed as
   `~hot_info_orbs_xiao.kicad_pro.lck.step` in the project folder (removed).
   A raw CARTESIAN_POINT bounding box of an assembly STEP is meaningless
   (sub-part local frames): render, don't parse.

9. [x] Physical rework, NOT routed (2026-10-16, Scott: no antenna on the
   board; displays as low as possible on headers into female sockets;
   right-angle switches on the TOP edge, equally spaced in the centre third;
   C1/C2/R1 to the bottom). `tools/physical_rework.py` runs on the committed
   1d4623f board (`git checkout` the .kicad_pcb/.kicad_sch first), KiCad
   closed. Removes ANTENNA_PATCH + its B.Silk, the SuperMini antenna F.Silk
   box and three old button frames, strips all 268 tracks/vias and unfills
   the pour; swaps DS1–5 to new `_Displays:GC9A01_SOCKET` (GC9A01_PINS pads +
   PinSocket_1x07 model, display lifted 8.5 mm) with the pad row at y 106.01,
   and SW1–3 to stock `SW_Tactile_SPST_Angled_PTS645Vx39-2LFS` at centres
   x 128.68 / 150.78 / 172.87, body front on the top edge. R1 in the DS1/DS2
   gap, C2 + C1 in the DS3/DS4 gap; NOT the DS2/DS3 gap because the XIAO is
   on the back there. Club/title texts moved to the top band. Schematic
   Footprint fields rewritten; DRC 0 parity, 46 unconnected (no tracks),
   5 silk_edge + 19 silk_over_copper warnings. STEP re-exported.
   Choice of Vx39: the 8.05 mm overall depth is the PTS645 right-angle
   variant nearest the uxcell "6x6x8". Its tab pitch (7.0 mm) is
   UNVERIFIED against the listing's "6.5 mm" — measure a switch.
   Socket model needed rotate z **-90** (+90 put it beside the pins).
   Traps: after the first `b.Remove()` in a process SWIG stops typing ALL
   return values, not just `Zones()` — `GetFootprints()`, `GetTracks()` and
   methods on handles taken earlier (`fp.GetPosition()`) come back as bare
   SwigPyObjects. Read and move everything first, Add, then Remove last.
   Board texts carry mixed justifications, so `SetPosition` lands them off
   the board — move by bounding-box delta. Copy Datasheet/Description onto
   swapped footprints or parity flags `footprint_symbol_field_mismatch`.
   Same day, second pass (script extended, still one run from 1d4623f):
   Scott — "bottom of the board" meant the SIDE, "all on the same side as
   the xiao": C1, C2, R1, SW1-3 flipped to the back (switches stay on the top
   edge); and "bring the board edge up even with the top of the displays":
   top edge y 79.28 -> 62.0, measured from an orthographic top render of the
   placed board against a footprint-free copy at the same camera (19.4
   px/mm, tallest display top at y 61.99). Top mounting holes and the GND
   pour's top corners moved up 17.28 mm too. DRC: 0 parity, 46 unconnected,
   19 silk warnings (the SW1-over-XIAO overlap is gone — the switches are
   17 mm higher now).
   Traps: a left-right `Flip()` mirrors about the footprint origin = pad 1,
   so R1 swung into DS1's socket pads (4 shorts) — re-centre on the pre-flip
   pad midpoint. `ZONE` has no `SetCornerPosition`: rebuild with
   `RemoveAllContours()` + `AppendCorner(v, -1)`.
   Third pass (Scott, same day): switches on the THIRDS — one on the 1/3
   line, one on the 2/3 line, one midway (x 117.63 / 150.78 / 183.92); pass
   1 had centred them in three equal slots of the centre third, which is not
   what he meant. XIAO "on the same pin arrangement as the displays": new
   `_ESP32 modules:XIAO-ESP32-S3-SOCKET` (Seeed DIP with plain 1.7 mm pads /
   1.0 mm drills at the header holes, two PinSocket_1x07 models, module z
   11.0 = 8.5 socket + 2.5 header plastic; header pins themselves not
   modelled). Board ID silk to the back with the HOT logo scaled to 28 mm;
   orphan B.Silk "VCC"/"GND" removed (they marked nothing on 1d4623f
   either). DRC: 0 parity, 46 unconnected, 5 silk_edge (socket tab boxes);
   the 14 Seeed silk_over_copper warnings went with the smaller pads.
   Not a trap, a near-miss: the DIP pads' `(drill … (offset …))` moves the
   COPPER, not the hole — the holes were at the header positions all along.
   Fourth pass (Scott, same day): corner holes 2 mm in on each axis, plus
   two holes "near the middle for support" on the top row at x 131.28 /
   170.27 (y 67.0) — the top row because the bottom middle is the XIAO
   and its sockets. Trap: `GetBoardEdgesBoundingBox()` includes half the
   0.05 mm edge line, so every extent was 0.025 mm off; extents now come
   from the Edge.Cuts rect's start/end. That moved the pin row to 105.98
   and the switches to 117.64 / 150.77 / 183.91, and cleared the 5
   silk_edge warnings: DRC 0 violations, 46 unconnected, 0 parity.
   Fifth pass (Scott: "shouldn't the caps be closer to the esp?"): yes for
   C1, which is the +5V bulk cap and the displays run off +5V; it was ~36 mm
   from the 5V pin. Now 5.5 mm right of the XIAO's 5V/GND pins, 3.6 mm up so
   its silk clears DS3's B.Silk outline box (first try sat 1.3 mm up and
   hit it). R1/C2 are the display RC reset, not decoupling; grouped above C1
   because R1 takes 3V3 from the XIAO. DRC 0 / 46 unconnected / 0 parity.
   The zoomed kicad-cli render makes R1 look under the module (the module
   is 11 mm up); courtyards say 1.1 mm clear.
   Sixth pass (Scott): XIAO "on male headers into the females" - the
   sockets were already there, but the headers were not modelled, so in
   Fusion the module floated. Added two PinHeader_1x07 models, upside down.
   Trap: rotate (180, 0, -90), the sockets' -90 plus the flip, ran the
   rows 15 mm out past the module end, because the X flip reverses the row
   before the Z turn. (180, 0, 90) is right; checked with a `--side left`
   render. Also a seventh hole at (170.27, 102.78), straight below the right
   mid hole. DRC 0 / 46 unconnected / 0 parity.
   **Routing waits for Scott's review of the STEP.** Then `route_xiao.py`.
   `tools/antenna_patch.py` is retired.

## Open
- Scott reviews the 2026-10-16 physicals in the STEP, then re-route
  (`route_xiao.py`) and refill the pour.
- Measure a uxcell right-angle switch: tab pitch 6.5 vs 7.0 mm decides
  whether the stock PTS645 footprint is usable.
- Order / build / flash a board; measure the RC reset and LED polarity.
- Branch has no upstream until pushed (`git push -u origin xiao-esp32s3-plus`).
