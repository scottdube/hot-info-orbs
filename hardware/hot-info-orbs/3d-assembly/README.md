# 3D assembly model

`hot_info_orbs_v1.1_assembly.step` — KiCad STEP export of the whole assembly:
PCB, ESP32-S3 SuperMini, five GC9A01 modules, three buttons, C1. For enclosure
design and fit checking.

Exported from Pcbnew **2026-09-09 14:04**, sent by Yang.

**Assembled stack is roughly 40 mm tall** (Z −13.2 to +26.7 mm): the module hangs
below the board, the displays stand above it. That is the dimension an enclosure
has to clear.

## Verify this matches your boards before designing around it

The boards in hand were ordered **2026-08-29** from tag `v1.1-run1`. This model
was exported **eleven days later**. If the board moved in between, the model
describes hardware nobody has. Confirm before cutting an enclosure to it.

## Why it arrived as a .bin

It was saved out of KiCad with no filename — the STEP header records the name as
literally `.step`, no stem. Messages had nothing to work with, so it substituted
a send-timestamp and a generic `.bin` extension. Renamed here; contents
unaltered (md5 `20c54433dd50a71e634d8743aad6edbf`).

Displays and module are shown as if soldered flat. Sockets will stand both
further off the board than this model suggests — Yang's note, 2026-08-29.
