// Host tests for RejoinPolicy.h:  pio test -e native
#include "RejoinPolicy.h"
#include <unity.h>

void setUp() {}
void tearDown() {}

using rejoin::kEveryMs;

void test_no_join_while_the_link_is_up() {
    rejoin::Watch w;
    for (uint32_t t = 0; t < 10 * kEveryMs; t += 1000) {
        TEST_ASSERT_FALSE(w.step(t, true).join);
    }
}

void test_no_join_until_the_link_has_been_down_a_while() {
    rejoin::Watch w;
    w.step(1000, true);
    TEST_ASSERT_FALSE(w.step(2000, false).join); // just dropped
    TEST_ASSERT_FALSE(w.step(2000 + kEveryMs - 1, false).join);
}

void test_joins_once_the_wait_is_over() {
    rejoin::Watch w;
    w.step(1000, true);
    w.step(2000, false);
    TEST_ASSERT_TRUE(w.step(2000 + kEveryMs, false).join);
    TEST_ASSERT_FALSE(w.step(2000 + kEveryMs + 1, false).join); // one call, not one per loop pass
}

void test_keeps_trying_at_the_same_interval() {
    rejoin::Watch w;
    w.step(0, false);
    int joins = 0;
    for (uint32_t t = 100; t <= 10 * kEveryMs; t += 100) { // five minutes down
        joins += w.step(t, false).join ? 1 : 0;
    }
    TEST_ASSERT_EQUAL(10, joins);
}

void test_a_new_drop_waits_from_its_own_start() {
    rejoin::Watch w;
    w.step(0, false);
    w.step(kEveryMs - 1, false);
    w.step(kEveryMs, true); // back just before a try was due
    w.step(kEveryMs + 10, false); // and down again
    TEST_ASSERT_FALSE(w.step(kEveryMs + 20, false).join);
    TEST_ASSERT_FALSE(w.step(2 * kEveryMs + 9, false).join);
    TEST_ASSERT_TRUE(w.step(2 * kEveryMs + 10, false).join);
}

void test_works_across_the_millis_wrap() {
    rejoin::Watch w;
    const uint32_t start = 0xFFFFFFFFu - 5000; // 5 s before millis() wraps
    w.step(start, false);
    TEST_ASSERT_FALSE(w.step(start + 4000, false).join);
    TEST_ASSERT_FALSE(w.step(start + kEveryMs - 1, false).join); // past the wrap
    TEST_ASSERT_TRUE(w.step(start + kEveryMs, false).join);
}

void test_reports_the_outage_once_when_the_link_returns() {
    rejoin::Watch w;
    w.step(500, true);
    w.step(1000, false);
    for (uint32_t t = 1100; t < 1000 + 95000; t += 100) {
        TEST_ASSERT_FALSE(w.step(t, false).ended);
    }
    const rejoin::Step back = w.step(1000 + 95000, true);
    TEST_ASSERT_TRUE(back.ended);
    TEST_ASSERT_FALSE(back.join);
    TEST_ASSERT_EQUAL_UINT32(95000, back.outMs);
    TEST_ASSERT_EQUAL(3, back.tries); // at 30, 60 and 90 s
    TEST_ASSERT_FALSE(w.step(1000 + 95100, true).ended);
}

void test_a_short_blip_ends_with_no_tries() {
    rejoin::Watch w;
    w.step(0, true);
    w.step(100, false);
    const rejoin::Step back = w.step(1600, true); // a roam between access points
    TEST_ASSERT_TRUE(back.ended);
    TEST_ASSERT_EQUAL_UINT32(1500, back.outMs);
    TEST_ASSERT_EQUAL(0, back.tries);
}

void test_start_up_with_the_link_up_reports_no_outage() {
    rejoin::Watch w;
    TEST_ASSERT_FALSE(w.step(12345, true).ended);
}

void test_while_down_it_says_for_how_long_and_how_many_tries() {
    // for the "still down" lines in the device log
    rejoin::Watch w;
    w.step(1000, true);
    TEST_ASSERT_FALSE(w.down());
    w.step(2000, false);
    w.step(2000 + kEveryMs, false);
    w.step(2000 + 2 * kEveryMs + 500, false);
    TEST_ASSERT_TRUE(w.down());
    TEST_ASSERT_EQUAL(2 * kEveryMs + 500, w.downMs(2000 + 2 * kEveryMs + 500));
    TEST_ASSERT_EQUAL(2, w.tries());
    w.step(2000 + 3 * kEveryMs, true);
    TEST_ASSERT_FALSE(w.down());
}

void test_the_core_gives_up_on_a_refusal_but_retries_a_lost_beacon() {
    // Read from arduino-esp32 2.0.17 WiFiGeneric.cpp, _isReconnectableReason()
    TEST_ASSERT_TRUE(rejoin::coreRetries(200)); // BEACON_TIMEOUT
    TEST_ASSERT_TRUE(rejoin::coreRetries(201)); // NO_AP_FOUND
    TEST_ASSERT_TRUE(rejoin::coreRetries(2)); // AUTH_EXPIRE
    TEST_ASSERT_TRUE(rejoin::coreRetries(15)); // 4WAY_HANDSHAKE_TIMEOUT
    TEST_ASSERT_FALSE(rejoin::coreRetries(202)); // AUTH_FAIL
    TEST_ASSERT_FALSE(rejoin::coreRetries(8)); // ASSOC_LEAVE
    TEST_ASSERT_FALSE(rejoin::coreRetries(12)); // BSS_TRANSITION_DISASSOC
    TEST_ASSERT_FALSE(rejoin::coreRetries(30)); // NOT_AUTHORIZED_THIS_LOCATION
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_no_join_while_the_link_is_up);
    RUN_TEST(test_no_join_until_the_link_has_been_down_a_while);
    RUN_TEST(test_joins_once_the_wait_is_over);
    RUN_TEST(test_keeps_trying_at_the_same_interval);
    RUN_TEST(test_a_new_drop_waits_from_its_own_start);
    RUN_TEST(test_works_across_the_millis_wrap);
    RUN_TEST(test_reports_the_outage_once_when_the_link_returns);
    RUN_TEST(test_a_short_blip_ends_with_no_tries);
    RUN_TEST(test_start_up_with_the_link_up_reports_no_outage);
    RUN_TEST(test_while_down_it_says_for_how_long_and_how_many_tries);
    RUN_TEST(test_the_core_gives_up_on_a_refusal_but_retries_a_lost_beacon);
    return UNITY_END();
}
