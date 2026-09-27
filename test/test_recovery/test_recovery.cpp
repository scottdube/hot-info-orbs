// Host tests for RecoveryPolicy.h:  pio test -e native
#include "RecoveryPolicy.h"
#include <unity.h>

void setUp() {}
void tearDown() {}

static AppSlot good() { return AppSlot{true, false}; }

void test_hands_over_to_a_good_app() {
    // A power cut while in recovery lands here: nobody asked, the app is fine
    TEST_ASSERT_TRUE(onBoot(false, good()) == RecoveryAction::BootApp);
}

void test_stays_when_asked() {
    TEST_ASSERT_TRUE(onBoot(true, good()) == RecoveryAction::Stay);
}

void test_never_boots_an_image_that_does_not_verify() {
    // Empty slot (recovery flashed alone) or an upload cut off part way
    AppSlot empty{false, false};
    TEST_ASSERT_TRUE(onBoot(false, empty) == RecoveryAction::Stay);
    TEST_ASSERT_FALSE(bootable(empty));
}

void test_stays_after_a_rollback() {
    // The image verifies but reset before confirming; the bootloader marked it
    AppSlot rolledBack{true, true};
    TEST_ASSERT_TRUE(onBoot(false, rolledBack) == RecoveryAction::Stay);
    TEST_ASSERT_FALSE(bootable(rolledBack));
}

void test_idle_timeout_returns_only_to_a_good_app() {
    TEST_ASSERT_FALSE(idleReturn(RECOVERY_IDLE_MS - 1, false, good()));
    TEST_ASSERT_TRUE(idleReturn(RECOVERY_IDLE_MS, false, good()));
    TEST_ASSERT_FALSE(idleReturn(RECOVERY_IDLE_MS, true, good())); // upload in progress
    TEST_ASSERT_FALSE(idleReturn(RECOVERY_IDLE_MS * 10, false, AppSlot{true, true}));
    TEST_ASSERT_FALSE(idleReturn(RECOVERY_IDLE_MS * 10, false, AppSlot{false, false}));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_hands_over_to_a_good_app);
    RUN_TEST(test_stays_when_asked);
    RUN_TEST(test_never_boots_an_image_that_does_not_verify);
    RUN_TEST(test_stays_after_a_rollback);
    RUN_TEST(test_idle_timeout_returns_only_to_a_good_app);
    return UNITY_END();
}
