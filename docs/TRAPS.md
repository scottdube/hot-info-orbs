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
named its env (`-e esp32-s3-devkitc-1`, `-e ota`, `pio test -e native`).

Fix: `default_envs = esp32-s3-devkitc-1, ota` under `[platformio]`, plus an
explicit `pio test -e native` step in CI. This is the "change the build
contract, change CI in the same commit" trap above, a second time. **Before a
merge, run the exact CI command (`pio run`, no `-e`) locally.**
