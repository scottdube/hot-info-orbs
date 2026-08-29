# Traps

Things that cost time once. Each entry says what broke and what proved it.

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
