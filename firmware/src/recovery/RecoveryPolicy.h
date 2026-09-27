#pragma once
#include <cstdint>

// Decisions the recovery app makes, kept free of ESP-IDF so they run on the
// host (test/test_recovery). The rule under all of them: never boot an app
// slot that does not verify or that the bootloader has already rolled back.

// What recovery knows about the one app slot (ota_0)
struct AppSlot {
    bool imageValid; // esp_image_verify passed: complete and checksummed
    bool markedBad;  // otadata says INVALID or ABORTED: it reset before confirming
};

enum class RecoveryAction { Stay, BootApp };

// The app sets this NVS flag before restarting into recovery to take an
// update. Recovery clears it on boot, so a power cut returns to the orbs.
#define RECOVERY_NVS_NAMESPACE "recovery"
#define RECOVERY_NVS_KEY "stay"

// Recovery answers every page with this header; tools/ota_upload.py waits for it
#define RECOVERY_HEADER "X-Orbs-Recovery"

// No upload for this long and the app is good: go back to showing the orbs,
// so an orb nobody finished updating does not sit blank
const uint32_t RECOVERY_IDLE_MS = 10UL * 60 * 1000;

inline bool bootable(const AppSlot &app) {
    return app.imageValid && !app.markedBad;
}

// requested: the app restarted into recovery on purpose to take an update.
// Without it, recovery is running because the bootloader had nothing better
// (a power cut while in recovery, a rolled-back update, an empty slot).
inline RecoveryAction onBoot(bool requested, const AppSlot &app) {
    return (!requested && bootable(app)) ? RecoveryAction::BootApp : RecoveryAction::Stay;
}

inline bool idleReturn(uint32_t idleMs, bool uploading, const AppSlot &app) {
    return idleMs >= RECOVERY_IDLE_MS && !uploading && bootable(app);
}
