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
