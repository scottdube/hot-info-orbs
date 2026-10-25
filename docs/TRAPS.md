# Traps

Things that cost time once. Each entry says what broke and what proved it.

## A privacy grep that names what it looks for is the leak

2026-09-24, branch `tempest`. The plan's check that a test fixture held no
real station data was a grep whose pattern listed the real station IDs and
name fragments. So the check itself put them in a public repo. It was caught
after the push and removed from the branch history.

Keep private values out of anything committed, including commands that
search for them. Read them from the git-ignored `config.h` at run time, and
grep the staged diff before pushing, not after.

## `config_helper.h` is force-included into C files, not just C++

`platformio.ini` passes `-include "firmware/config/config_helper.h"`, so that
header lands in **every** translation unit — including plain C from vendored
libraries (`tjpgd.c`, `sd_diskio_crc.c`). Anything C++-only in it breaks the
build inside someone else's library, with an error pointing at our header.

Cost a build cycle when a `static_assert` guarding the API keys was added
unguarded. Now gated on `defined(__cplusplus)`.

**Anything added to that header must be valid C**, or be inside a
`#if defined(__cplusplus)` block.

## One build failure can mask another

The first `pio run` died at `bootloader.bin` on a missing `intelhex` module.
That looked like the only problem — but it aborted the build *before* the C
library files compiled, hiding the `static_assert` breakage entirely. The
second failure only appeared once the first was fixed.

**A build that fails early has not verified the code it never reached.** Fix
and re-run before concluding anything compiles.

## `clang -fsyntax-only` is not a substitute for a real `pio run`

The header guards were checked with `clang -fsyntax-only` on a C++ translation
unit. All three states passed. The real build then failed, because the check
never compiled a C file and never touched the vendored libraries.

Useful for fast preprocessor logic checks. Not evidence the firmware builds.

## PlatformIO's Python environment is replaced by VS Code updates

A VS Code update can rebuild `~/.platformio/penv` from scratch, and the fresh
environment came up missing `intelhex`, a dependency of esptool 4.11. Symptom
is a machine that built ESP32 firmware fine for months suddenly failing at
`bootloader.bin`.

Fix: `~/.platformio/penv/bin/python -m pip install intelhex`

Diagnostic that settles it fast: `stat -f '%SB' ~/.platformio/penv` — if the
creation date is today, the environment is new, and prior successes were on a
different one.

## `git add -A` near an open KiCad project commits its session state

KiCad writes `~<project>.kicad_*.lck` lock files while a project is open, and
the editor keeps a `.history/` sidecar that is **its own git repository**. A
blanket `git add -A` swept all four into a commit here — the lock files as
ordinary files, and `.history` as a gitlink pointing at a repo that exists only
on one machine. A clone would get a broken submodule reference.

Now covered by `.gitignore` (`~*.lck`, `**/.history/`, `*-backups/`,
`*.kicad_prl`). The general lesson stands regardless: **check `git status`
before `git add -A` in a directory someone has open in a GUI tool.**

Related: the presence of `~*.lck` is also the reliable test for whether KiCad
has a project open, which matters because writing to a project directory while
KiCad holds it corrupts state.

## Strapping pins keep turning up on user-facing connections

Two independent boards for this project both route an MCU strapping pin
somewhere a user can drive it:

**Upstream, GPIO12 on the `U1` expansion header.** `U1` is a 1x6 sensor
breakout carrying 5V, 3V3, GND, GPIO34, GPIO35, GPIO12. GPIO34/35 are a good
choice — input-only and on ADC1, which still works while WiFi is active. GPIO12
is not: it is MTDI, the strapping pin that selects flash voltage at boot. Held
high at reset on a 3.3V-flash module, the board may not boot. It is also ADC2,
so it cannot do analog while the radio is up. The header offers 3V3 two pins
away from it.

**HackerBox 0129, GPIO3 as `BUTTON_RIGHT`.** GPIO3 is an ESP32-S3 strapping pin
(JTAG source select), and Info Orbs wires buttons to VCC with `INPUT_PULLDOWN` —
so a button held during reset drives it high.

Both produce the same failure signature: **an intermittent no-boot that nobody
connects to a button or a sensor lead**, because it only happens when something
is held at the moment of reset.

**When drawing a pin map, check the strapping list for that exact part first.**
It differs between ESP32 variants — classic is GPIO0/2/5/12/15, S3 is
GPIO0/3/45/46 — so a map ported between them silently changes which pins are
hazardous.

## Changing the build contract breaks CI silently

The secrets split made `secrets.h` a required file and added a guard that
rejects an unfilled key. Both are deliberate. But `.github/workflows/platformio.yml`
created only `config.h`, so every CI build failed from that commit onward —
with our own `#error`, on a repo nobody was watching the checks on.

It went unnoticed for several commits because the repo was private and nothing
surfaces a red check unless you look. **If you change what the build requires,
change CI in the same commit.** The build guard is the point; CI has to satisfy
it like any other builder.

CI now writes both files and injects `CI_BUILD_ONLY` placeholder keys — it
compiles, it never calls the APIs. Note the injection is done in Python rather
than `sed`: the matrix runs ubuntu, macos and windows, and `sed -i` differs
between GNU and BSD and does not exist in the default Windows shell.

## ESP32-S3 native USB: a serial monitor that connects, says nothing, and drops

Symptom on a freshly ported S3 build: `pio device monitor` attaches, prints no
output, then loops `Disconnected (read failed: [Errno 6] Device not configured)`
/ `Reconnecting... Connected!` forever.

It looks exactly like a boot loop and is not. **Check whether the board is
actually up before diagnosing the firmware** — 20 pings with 0% loss settles it
in ten seconds, and that is what happened here: the firmware was running fine
and on the network the whole time.

The cause is that Arduino's `Serial` defaults to **UART0 on GPIO43/44**, not to
the USB port, so nothing services a CDC endpoint and macOS keeps losing it.

**The obvious fix makes it worse on this board. Tried and reverted 2026-09-10:**

```ini
; DO NOT USE on the SuperMini without testing on hardware first
-D ARDUINO_USB_MODE=1
-D ARDUINO_USB_CDC_ON_BOOT=1
```

> **XIAO (2026-10-14):** this trap is the v1.1 prototype's (ESP32-S3 SuperMini). The
> `seeed_xiao_esp32s3` board definition sets exactly these two flags itself,
> and on the XIAO they are right: its USB-C is wired to the ESP32-S3's native
> USB with no bridge chip, so the application *must* own the CDC endpoint for
> there to be a console at all. Do not remove them here. Untested on hardware
> until a XIAO board exists.

With those set, the *application* takes ownership of the USB peripheral instead
of leaving it to the ROM. It then fails to enumerate, and the board disappears
from the host completely — no `/dev/cu.usbmodem*`, nothing in
`system_profiler SPUSBDataType`. It still boots, still joins WiFi, still answers
pings. There is simply no port left to flash over.

Recovery is manual bootloader entry: hold BOOT while plugging the USB cable in,
then reflash. Which is unpleasant on a board whose BOOT and RST silkscreen is
unreadable — the recovery attempt is also how you find out which button is which.

**The diagnostic that keeps this cheap:** a board that has vanished from USB but
answers pings is not broken, and the firmware is not the thing to investigate.
Ping first. Both times this came up, the firmware was running correctly and only
the host link was in question.

Serial output on this board therefore needs a different route — UART0 on
GPIO43/44 with an external adapter — or you wait for displays, which report the
same failures visually.
The classic ESP32 does not need this — it has a separate USB-serial chip, so
`Serial` reaches the host without any build flag. This is specific to parts
where the MCU provides USB itself, and it is easy to miss when porting because
nothing about the build fails.

After enabling CDC the first upload may need the manual bootloader entry (hold
BOOT, tap RST, release BOOT), because the USB device presents differently.


## `info-orbs.local` resolves but curl to it stalls — use the IP (2026-09-23)

Measured from the MacBook right after the first OTA-layout flash: `dns-sd -G v4
info-orbs.local` answered at once with 192.168.30.208, but `curl
http://info-orbs.local/update` did not answer in a 3 s timeout for over a
minute, while `curl http://192.168.30.208/update` returned 200 first try. The cause is
not established. Whether a browser has the same problem was not tested. When
the name stalls, test the IP before concluding the orb or its web server is
down. The orbs show the IP when they connect.

**Re-measured 2026-09-24:** the same again. The name resolves at once, and
`curl http://info-orbs.local/settings` times out at 5 s, twice. On the SLN
network, use **`http://info-orb-sln.internal/settings`** instead. That is the
router's DNS name for the orb, not something the firmware sets: 200 in 0.19 s,
forward and reverse both point at 192.168.30.208. Another orb or another
network will not have it.

## `__DATE__`/`__TIME__` do not date the image (2026-09-23)

The OTA page first printed `built __DATE__ __TIME__` from `OtaUpdater.cpp`. After
a change to `main.cpp` only, the orb reported the *previous* build's time,
because that file had not recompiled. That is a version report that lies. Fixed with
`tools/build_id.py`, which writes the git commit into a generated
`firmware/include/build_id.h`. The ELF SHA-256 in the app descriptor was
checked as an alternative: this build leaves it zeroed.

## A failed `pio run -e ota -t upload` reboots the orb — and the next try hits the reboot (2026-09-23)

Measured: from the MacBook at 192.168.1.203, espota to the orb at 192.168.30.208
failed twice with `No response from device`. Each time the orb's port 80 then
refused connections for ~10–20 s and came back on the old firmware. That is by
design, not a crash: the invitation reaches the orb and fires `onStart`, the
orb cannot open the data connection back to the laptop, and `onError` restarts
it after 3 s (`OtaUpdater.cpp`). A `curl` upload fired straight after landed in
that reboot and was refused, which reads as "the orb is down".

espota needs the orb to connect **back** to the laptop; the browser/curl path
needs only laptop → orb. Across subnets (here .1.x → .30.x, TTL 63) only the
second was shown to work: `curl -F "firmware=@.pio/build/ota/firmware.bin"
http://<ip>/update` → `Update OK` in 11 s. Why the connect-back fails (routing,
firewall, macOS application firewall) was not established. After a failed
espota, wait for `/update` to answer 200 before trying anything else.

## Finding the orb on 192.168.30.x, and a URL that "doesn't work" (2026-09-23)

- **Several other devices on 192.168.30.x answer `/update` with a bare "200
  OK".** A port-80 sweep for `/update` does not find the orb; check that
  `/settings` contains "Running 1.1.0".
- **A trailing `.` pasted after the URL** (`…/settings.`) stops the page loading
  by itself. Rule it out first when someone says the page "doesn't work".
- **An orb that has gone completely dark on the network may simply be
  unplugged.** On 2026-09-23 it was an accidental unplug, not firmware. Ask
  before debugging. A 4 h watch at 30 s intervals afterwards (480 checks, 20:25–00:26) saw no drop.

## A `native` test env gets built by a bare `pio run` — and cannot compile the firmware (2026-09-24)

The web-settings merge added `[env:native]` for host unit tests. CI runs a bare
`pio run`, which builds **every** env, so it tried to compile `firmware/src`
for the host: `'Arduino.h' file not found`. It failed on all three OSes from
983c037 until the fix. Locally everything was green, because every local command
named its env (`-e esp32-s3-devkitc-1` — now `xiao-esp32s3` on this branch —, `-e ota`, `pio test -e native`).

Fix: `default_envs = <firmware env>, ota` under `[platformio]`, plus an
explicit `pio test -e native` step in CI. This is the "change the build
contract, change CI in the same commit" trap above, a second time. **Before a
merge, run the exact CI command (`pio run`, no `-e`) locally.**

## One display lit but blank — cleared by a reset, cause unknown (2026-10-17)

Scott's v1.1 prototype orb (ESP32-S3 SuperMini, MAC e0:72:a1:ea:ab:84) showed display 1 with its
backlight on and no content; displays 2–5 were not checked before the fix. Three
USB resets happened while probing — opening the USB-JTAG port resets the S3, and
so does `esptool flash_id` — and afterwards all five displays showed data.

**Measured:** the board boots normally and the app never logs over USB (Serial is
on UART0, see above), so the USB port tells you nothing about the displays. A
lit backlight proves only that 5 V reaches the module: the backlight is wired to
VCC. **Not established:** why display 1 missed its content. One candidate is that
DS1 (CS GPIO4) missed initialization at power-up, but that is a hypothesis. If it
comes back: note which displays are blank, and **before resetting anything**,
swap DS1 with DS2. If the blank moves with the module, the module is at fault; if
it stays in position 1, the cause is the board, the GPIO4 path or that socket.

## A 3D model fitted by eye can sit a castellation column off the holes (2026-10-16)

Branch `xiao-esp32s3`, carrier board. On 2026-10-14 the Seeed XIAO
STEP was fitted by rendering against the DIP footprint's copper. It looked
right. When the module went onto headers, Scott saw that its holes missed
the pins. Fusion `find_geometry` on the hole faces measured the error at
1.49 mm across the rows and 0.127 mm along them.

Fit a model by measuring its holes against the pad positions. A render
can't show this: the module is 11 mm up, so parallax hides it.

The fix had a sign trap. On a back-side (flipped) footprint, the model
offset's local y went to -x on the board while the pads' local y went to
+x. Make one move, re-measure, and only then trust the direction.


## KiCad draws 180-degree silk upright; the STEP export does not (2026-10-17)

The XIAO board's display footprints sit at 180 degrees, and their DS1-DS5
reference text rode along at 180. In KiCad's editor and its 3D viewer the
labels read upright, so the board looked right. The STEP export
(`kicad-cli pcb export step --include-silkscreen`) does not apply that
correction: in Fusion every DS label read flipped, and Scott caught it, not
a check. Fix: set the text to 0 degrees absolute
(`Reference().SetTextAngleDegrees(0)` in `physical_rework.py`), which is
upright in both places. **Review silk in the STEP, not in KiCad, whenever
a footprint is rotated 180.**

## Freerouting's empty SES is an exit race, not unrouted nets (2026-10-17)

Branch `xiao-esp32s3`, `tools/route_xiao.py`. The script asserted that
a 0-byte SES meant "nets left unrouted" and stopped there. That reading was
wrong. A run whose log said `0 unrouted and 0 violations` wrote 0 bytes, and
an identical rerun wrote the file. Runs with 1 unrouted connection wrote
21 KB. Freerouting 2.4.1 logs `Saving ...` and sometimes exits before the
write lands.

Fix: take the unrouted count from the log's `final score` line, and rerun on
an empty file. The script does both now.

Same session, two more: the DSN exports the mounting-hole Edge.Cuts circles
as keepouts at the bare hole radius. Freerouting keeps only its 0.2 mm track
clearance from them, so KiCad flagged `copper_edge_clearance` (0.5 mm). The
script now grows them by 0.4 mm. And reordering the nets in the DSN changes
nothing: 12 shuffles gave a byte-identical result, because Freerouting
orders them itself.

## Swapping `config.h` does not rebuild the pins; a wrong pin map can wipe Wi-Fi (2026-10-20)

**What happened.** A demo build for the v1.1 SuperMini prototype was made in
a fresh worktree with the XIAO `config.h` copied in by mistake. After the OTA,
screens 1-3 went black. Screens 4-5 kept their old picture, because a panel
holds its last frame through a soft restart, so that picture proved nothing.
Copying the prototype `config.h` over it and rebuilding still produced the
XIAO pins. The orb's own boot log showed it: `TFT_MOSI:9`, `SCREEN_1_CS:3`.

**Why.** `config.h` reaches every file only through
`-include firmware/config/config_helper.h` in `build_flags`. PlatformIO's
dependency scan does not follow a force-included header. Editing or replacing
`config.h` leaves the old objects in place, and the build reports SUCCESS with
the old pins.

**Do instead.**
- After changing `config.h`, delete the env's build dir
  (`rm -rf .pio/build/<env>`) or run `pio run -e <env> -t clean` first.
- Prove the pins from the boot log over USB serial. The `TFT_*:` and
  `SCREEN_n_CS:` lines are printed at start-up.
- Before flashing, check which board the orb is: `esptool.py flash_id` says
  "Embedded Flash 4MB" for the prototype, 8 or 16 MB for a XIAO.

**Wi-Fi wiped.** The orb came back with "No wifi saved" and opened the setup
portal. `WifiWidget` calls `resetSettings()` when `BUTTON_RIGHT` reads pressed
at power-up. In the XIAO map that is GPIO 6, and on the prototype GPIO 6 is
Screen 3's CS line, so a wrong-board build can read a "held" button. This is a
hypothesis, not measured. The likely cause fits, but the orb had joined Wi-Fi
on the first boot of that same image. Re-join through the `Info-Orbs_XX`
portal.

## fusion-essentials: the active document flips between calls (2026-10-19)

With five documents open (two of them named `main_body_front`), the active
document switched back to `overall assembly` *between* tool calls, and more
than once switched the other way. Reads (`find_geometry`, `design_get`,
`sketch_get`, `view_screenshot` without `expect_document`) have no guard and
silently answer about whichever document is up: a `find_geometry` for the
new posts returned a capacitor on the board, a "main_body_front" screenshot
captured the assembly. Writes with `expect_document` are refused instead
(`active_document_changed`), which is the only reason this was noticed.

Rules that held: pass `expect_document` as the lineage **URN** (a bare name
is refused as `ambiguous_document_name` when two open docs share it); put
`doc_open(file_id=<URN>, force_api_open=true)` or `doc_activate(<URN>)` in
the **same message** as the calls that need it; and read `active_document`
at the bottom of every read result before believing it. `design_get` with
`tree` on the assembly is ~81k characters; narrow with `name_filter` /
`max_depth`.

## Fusion assembly: the board is pinned by the displays, so grow the edge (2026-10-19)

The X0.2 board's top mounting holes sat under the enclosure's top lip, so
the ends hung from hangers. Shifting the board up is not an option: the
five displays are located by the bezel holes, and sliding the board slides
them. Growing the board's top edge 5 mm moves the holes without moving the
displays. The sockets fix the bottom edge the same way.

Smaller ones from the same build: grounding an occurrence resets a free
`assembly_move`, so capture the position and leave it free. Interference
checks want **leaf** occurrence paths (`...+hot_info_orbs_xiao_46mm_PCB:1`),
not the imported assembly's top node, and the GC9A01 vendor STEP reports a
0.0001 cm³ overlap with itself. A sketch on the plate's inner face
auto-projects 39 face edges, and a projected edge splits each bottom-row
post circle into two profiles, so read profile indices from `sketch_get`
by centroid and extrude both halves.

## KiCad text: set it through pcbnew, not sed (2026-10-19)

`gr_text` in a `.kicad_pcb` carries a `render_cache` of the outline-font
polygons. Editing the string with sed leaves the old glyphs in the cache,
and the gerber/STEP exports plot the cache. `PCB_TEXT.SetText()` +
`SaveBoard()` regenerates it (the cache line changed to `rev 2.1`). Also:
`kicad-cli pcb export drill` takes `--excellon-units mm`; `--units` prints
the help and exits 0, so the zip step then fails on a missing `.drl`.

## fusion-essentials decals: a vendor STEP's glass face cannot be named (2026-10-19)

`design_edit_reference_image` refused the display glass with "2 of them sit
at the recorded position … co-located candidates": the GC9A01 module STEP has
two coincident faces at the glass top, and the locator will not pick one. The
tool also wants root-local faces. Work around both with a thin disc in the
root component at the glass plane and decal that (ENCLOSURE.md, "Populated
display faces"). Other things learned on the same job:

- `find_geometry nearest_to` walks the whole design (5127 planar faces here)
  and times out at 30 s about two calls in five; a timed-out call that "may
  still commit" is a read, so just re-ask.
- The action to change an existing decal is `place` (create/place/style/
  delete; there is no `edit`), and `kind` is required on every action.
- A 240 px square image lands at 16 × 16 mm; `scale` is a multiplier on that,
  so [2, 2] = 32 mm. `size` in the read-back is the placed size.
- `view_screenshot` views are front/back/top/bottom/left/right and
  iso-top-right etc., not `iso`.

## The USB cutout that was "4 mm off" never existed (2026-10-20)

Rev 2.3 of the 46 mm board moved the XIAO 4 mm down, and three places
(ENCLOSURE.md, the board README, the `lower_xiao.py` docstring) recorded
that "the enclosure's USB cutout is now 4 mm off". Scott then asked to
"replace the board and move the cutout". `main_body_front` v4's timeline is
plate, walls, fillet, button holes, posts, pins: no cutout, and the other
`main_body_front` file in the project has even less. The sentence had been
written from the board change, not from the enclosure model, and once it was
in three files it read as a fact. Before writing "X in the model is now
wrong", read the model's timeline (`design_get include=['timeline']`) and
name the feature. Same job: a 12.7 MB STEP `doc_insert_import` times out on
the MCP side but still completes in Fusion (Group3 of 125 features appeared
about a minute later); the next calls time out until it finishes, then the
active document has flipped, so `doc_activate` before reading.

## A package built with `git archive` drops uncommitted KiCad edits (2026-10-23)

The zip and schematic PDF sent to Yang on 2026-10-23 were built at 17:32 by
`git archive 8dda9b9`, the last commit (2026-10-20). Scott's schematic tidy-up
was saved in this worktree but never committed, so it was silently left out.
Before 21:30 the working tree differed from 8dda9b9 in three ways:

- **Schematic layout only.** Every symbol had moved, one wire and one junction
  were added, and `sch_revision` went from 1.1 to 2.4. The XML netlists match:
  same 12 parts, same 15 nets, same pins.
- **Board:** one /SW3 dogleg near (123.5, 92.8) became a straight run, and
  the pour was refilled.
- **Checks:** ERC 0; DRC shows the 3 known USB-C silk warnings, 0 unconnected,
  0 parity.

**Before building any package:** run `git status` in the project folder. Then
either commit first, or build from the working tree and say in the package
that it is uncommitted.

## `min_resolved_spokes (min 1)` is silently ignored; write `min_resolved_spokes 1` (2026-10-25)

A one-pad `.kicad_dru` rule to accept one thermal spoke on C2 pad 2 (rev 2.5,
46 mm board) did nothing with `(constraint min_resolved_spokes (min 1))`. DRC
still said "zone min spoke count 2", and kicad-cli printed no parse error.
Changing the condition did not help, even `A.Type == 'Pad'`. What works is
the bare number, `(constraint min_resolved_spokes 1)`, with the condition
`A.memberOfFootprint('C2') && A.Pad_Number == '2'`. **Test any custom rule
in both directions:** aimed at pad 1 the same rule let the error back, which
proves it matches the one pad and nothing wider.

DRC exclusions in `.kicad_pro` were tried first and dropped. The key holds
the marker position and the item UUIDs, and three guesses at the
`starved_thermal` format did not match. A reroute would move the marker
anyway. `pcbnew.WriteDRCReport` cannot be used to read the real key: from a
script it asserts in `Pgm()`.

## A reroute from bare copper drops the stitching vias (2026-10-25)

`tools/route_xiao.py` removes every track **and via** before importing the
SES. On the rev 2.5 reroute the three GND vias went: Freerouting's own one
and the two that `tools/stitch_islands.py` had added. Nothing reports this.
DRC is clean whether or not an island got removed. **After any reroute,
fill with island removal off and on and compare the island lists** before
deciding whether to rerun the stitcher. For rev 2.5 nothing ≥100 mm² was
removed, so no via was needed.
