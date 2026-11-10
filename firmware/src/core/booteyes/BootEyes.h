#pragma once
// Waking eyes on orbs 2 and 4 while WiFi connects
// (docs/superpowers/specs/2026-11-10-boot-eyes-design.md). Between start() and
// finish() the eye task is the ONLY thing drawing; the caller must not touch
// the screens in between. That is why TFT_eSPI needs no lock.
#include "ScreenManager.h"
#include <TFT_eSPI.h>

namespace BootEyes {
// Starts the eye task. false = could not (out of memory); nothing was drawn.
bool start(ScreenManager &sm, TFT_eSPI &tft);
// connected = true: play the finale and wait for it (gives up after 6 s).
// connected = false: stop at once and blank orbs 2 and 4. No-op if not started.
void finish(bool connected);
} // namespace BootEyes
