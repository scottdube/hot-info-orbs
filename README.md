# HoT Info Orbs

Five round 240×240 displays on one SPI bus, driven by a Seeed **XIAO ESP32-S3
or XIAO ESP32-S3 Plus** (builder's choice), on a custom carrier board. Clock, weather, stocks — and whatever else gets written.

## Start here

**Never used GitHub?** You don't need an account. The list of folders above is
the project's files; everything you need is one of these links:

- **What it is and what it does:** the
  [feature sheet](https://scottdube.github.io/hot-info-orbs/features.html),
  an ordinary web page.
- **Getting one:** it's a Hands-On Tech Club build,
  [handsontech.org](https://handsontech.org).
- **The microcontroller:** a Seeed **XIAO ESP32-S3 or XIAO ESP32-S3 Plus**,
  whichever you prefer. Same board, same pins, same firmware; the Plus has more
  flash.
- **You have a built one and want it running:** the
  [setup guide](https://github.com/scottdube/hot-info-orbs/blob/xiao-esp32s3-plus/docs/SETUP.md), written for someone who has never
  done this.
- **You want the files:**
  [download them as a ZIP](https://github.com/scottdube/hot-info-orbs/archive/refs/heads/xiao-esp32s3-plus.zip). The board is in
  `hardware/hot-info-orbs-xiao/`, the software in `firmware/`.

The XIAO board and its firmware are on the
[`xiao-esp32s3-plus` branch](https://github.com/scottdube/hot-info-orbs/tree/xiao-esp32s3-plus) until they move here, which is why the links
above point there. The files on `main` today are the earlier v1.1
prototype, kept for the record; it is not part of the club build.

Built as a club project, which shapes the whole thing: it is assembled by people
at a wide range of skill levels, so the build has to be hard to get wrong and
cheap to get into.

## Credit where it belongs

**This is a derivative of [Info Orbs by Brett Turner](https://github.com/brettdottech/info-orbs)**
— his project, his firmware, his idea. The upstream repository, its
[contributors](https://github.com/brettdottech/info-orbs/graphs/contributors),
and Brett's [assembly and flashing video](https://link.brett.tech/orbsYT) are
where this came from, and his boards can be
[bought as a dev kit](https://brett.tech/collections/electronics-projects/products/info-orbs-full-dev-kit)
if you want the original rather than our variant.

The firmware here is his, with local changes. The board is not: it is a new
design for a different microcontroller.

Licensed **AGPL-3.0**, inherited from upstream — see `LICENSE.txt`. Sharing a
built board with someone means they are entitled to the source, which is why
this repository is public.

## What is different here

| | Upstream | This |
|---|---|---|
| Microcontroller | ESP32 dev kit (30/38-pin) | **Seeed XIAO ESP32-S3 or XIAO ESP32-S3 Plus** |
| Display supply | 3.3 V, off the module's regulator | **5 V, each module regulates its own** |
| API keys | compiled into `config.h`, shipped with live keys in the template | **`secrets.h`, gitignored, build fails without it** |
| Board | upstream's 2-layer design | **new carrier board** for the XIAO |
| Firmware updates | USB cable every time | **over WiFi** — browser page or `pio run -e ota` |
| Boot splash | "Info Orbs by brett.tech", Brett's logo | **"HOT Info Orbs by HOT-Team"**, Yang's avatar for now; the Hands-On Tech hand (`images/hot-logo.jpg`) replaces it once Yang has this version |
| Settings | edit `config.h`, rebuild, reflash | **a page in the browser**: `http://<orb's IP>/settings` |
| Boot picture | fixed at build time | **upload your own** from the settings page |
| Weather | one Visual Crossing town | also **your own Tempest stations**, one page each (see SETUP step 10) |

**Upgrading a board flashed before 2026-09-23? Flash it once more over USB.**
The WiFi updates need a new flash layout (two firmware slots instead of one), and
only a USB flash writes that layout. Every update after that can go over WiFi:
open `http://<orb's IP>/update`, choose `.pio/build/esp32-s3-devkitc-1/firmware.bin`
and upload. The page also shows which commit is running. Set `OTA_PASSWORD` in
`secrets.h`, or anyone on your network can flash your orbs. Details in
[docs/SETUP.md](docs/SETUP.md#8-later-updates-no-cable-needed).

The 5 V change is the significant one. Upstream powers all five displays from
the ESP32 module's onboard regulator, which is why builders report the module
running at 82 °C with the radio off. The GC9A01 modules carry their own
regulators, so feeding them 5 V moves that load off the microcontroller
entirely. **Check the module you buy actually has one** — they vary, and it
looks like a small chip with capacitors either side on the back of the board.

## Getting started

**[docs/SETUP.md](docs/SETUP.md)** goes from an empty machine to a running orb,
assuming no PlatformIO experience, and ends in a troubleshooting table keyed by
the symptom you actually see.

The short version: copy **both** templates in `firmware/config/`, then build.

| Copy | To | Holds |
|---|---|---|
| `config.h.template` | `config.h` | preferences — timezone, units, tickers |
| `secrets.h.template` | `secrets.h` | your own API keys |

Both are gitignored; the templates are what's tracked. **The build refuses to
compile without both**, and refuses again if a key is left empty — deliberately,
because the alternative is a blank orb on the bench and no idea why.

Two free API keys are needed, about a minute each:
[Visual Crossing](https://www.visualcrossing.com/sign-up) for weather and
[TimeZoneDB](https://timezonedb.com/register) for DST changeovers. Register your
own — a shared key burns one free-tier quota and can be revoked out from under
everyone using it.

To have the orbs rotate Clock → Stocks → Weather on their own, set
`WIDGET_CYCLE_DELAY` in `config.h` to the number of seconds per page (15 works
well; 0 turns it off). Pressing a button restarts the countdown.

**After the first flash, most of `config.h` can be changed from a phone.** Open
`http://<orb's IP>/settings` to change the rotation time, tickers, weather
location and units, timezone, clock face and colors, night-dim hours,
upside-down mounting, and the start-up picture. The orbs restart to apply the
changes. The values in `config.h` become the defaults, and the page's reset button returns
to them. See [docs/SETUP.md](docs/SETUP.md#9-change-settings-from-a-browser).

## Hardware

**The club board is the XIAO carrier**, for a XIAO ESP32-S3 or XIAO ESP32-S3
Plus: [`hardware/hot-info-orbs-xiao/` on the `xiao-esp32s3-plus` branch](https://github.com/scottdube/hot-info-orbs/tree/xiao-esp32s3-plus/hardware/hot-info-orbs-xiao).

What follows describes the **v1.1 prototype** on this branch, built for an
earlier module that the project no longer uses. Its KiCad 10 project is in
**[hardware/hot-info-orbs/](hardware/hot-info-orbs/)**.
Symbol and footprint libraries are project-local, so cloning and opening is all
that is required — nothing to install or copy into a global library.

**Board v1.1 — 10 boards ordered 2026-08-29**, tagged
[`v1.1-run1`](../../releases/tag/v1.1-run1). The exact gerbers sent to the fab
are committed at `hardware/hot-info-orbs/fab/`, and that file is never
regenerated: if a board comes back wrong, there has to be no question what was
sent. See [hardware/hot-info-orbs/fab/README.md](hardware/hot-info-orbs/fab/README.md)
for the order parameters and the drill-tool cross-check.

## Documentation

| | |
|---|---|
| [docs/SETUP.md](docs/SETUP.md) | empty machine to running orb |
| [docs/TRAPS.md](docs/TRAPS.md) | things that cost time once, so they cost it once |
| [docs/REQUIREMENTS.md](docs/REQUIREMENTS.md) | web control panel and OTA, with the traps that will bite whoever builds them |
| [docs/VARIANT-ESP32-S3-SUPERMINI.md](docs/VARIANT-ESP32-S3-SUPERMINI.md) | the prototype's module choice, since replaced by the XIAO ESP32-S3 / S3 Plus; kept for the arguments that reversed |
| [docs/ESPHOME-EVALUATION.md](docs/ESPHOME-EVALUATION.md) | why this is not an ESPHome project |

The variant and ESPHome documents keep the reasoning that changed rather than
only the conclusions. A decision with its argument attached can be revisited;
one without it gets re-litigated from scratch.

## Contributing

**Fork it and open a pull request.** Anyone can clone, build, fork and propose
changes — no permission needed for any of that, and a fork is yours to take in
whatever direction you like.

**Push access is by invitation**, and deliberately limited to a couple of
people. That is not gatekeeping the project, it is keeping the board files safe:
GitHub gives a personal repository no read-only collaborator tier, so adding
someone grants full write to everything including the KiCad files, which cannot
be merged if two people change them at once.

If you are building one of these and something is wrong or missing — a step in
the setup guide, a trap worth recording — [open an issue](../../issues) or send
a PR. Both are open to everybody, and a build that went sideways is worth
reporting even if you already worked around it: that is how the traps file gets
written.

## For collaborators

```bash
git pull --rebase     # before you start
# ...work...
git commit -am "what changed and why"
git push
```

`git config pull.rebase true` once, and a plain `git pull` does the right thing.

**KiCad files cannot be merged.** If two people edit `hot_info_orbs.kicad_pcb`,
git cannot reconcile them and somebody's work is lost. No tool fixes this, so it
is handled by saying so out loud: *"I've got the board"* / *"it's yours"*.
Everything else — firmware, docs, config — merges normally and needs no
ceremony.

Do not edit the KiCad project while it is open in KiCad, including with scripts.
KiCad holds its own copy in memory and writes it back on save, silently
discarding changes made underneath it. That has already cost a merge here once.

MegaLinter runs on every push and is configured **not** to commit anything back.
Upstream had it auto-committing fixes, which meant a bot pushed after every push
and rejected the next one — so "the remote has moved" stopped meaning "the other
person did something".
