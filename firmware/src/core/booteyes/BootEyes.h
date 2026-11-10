#pragma once
// Waking eyes on orbs 2 and 4 from WiFi connect until the first page
// (docs/superpowers/specs/2026-11-10-boot-eyes-design.md). The eye task and
// the start-up text on orb 5 share one SPI bus: until end(), anything drawing
// must hold a ScreenGuard. Before finish() nothing else draws at all.
#include "ScreenManager.h"
#include <TFT_eSPI.h>

namespace BootEyes {
#ifdef BOOT_EYES
// Starts the eye task. false = could not (out of memory); nothing was drawn.
bool start(ScreenManager &sm, TFT_eSPI &tft);
// connected = true: play the finale, then keep looking around until end().
// Returns at once. connected = false: stop at once and blank orbs 2 and 4.
// No-op if not started.
void finish(bool connected);
// Stops the eyes for the first page, after the finale has played (gives up
// after 6 s). Leaves the last frame up: the first page clears the screens.
// No-op if not running.
void end();
// Take turns on the bus with the eye task. No-op when the eyes are not running.
void lockScreens();
void unlockScreens();
#else
inline void end() {}
inline void lockScreens() {}
inline void unlockScreens() {}
#endif

struct ScreenGuard {
    ScreenGuard() { lockScreens(); }
    ~ScreenGuard() { unlockScreens(); }
    ScreenGuard(const ScreenGuard &) = delete;
    ScreenGuard &operator=(const ScreenGuard &) = delete;
};
} // namespace BootEyes
