// Host tests for LoopWatch.h:  pio test -e native
// Issue #52: the clock's colon stops blinking while the second tick keeps
// going. These watches put the time sync and slow loop passes in the device
// log, so the next freeze says what held the loop.
#include "LoopWatch.h"
#include <string>
#include <unity.h>

void setUp() {}
void tearDown() {}

// ---- time sync ----

void test_a_sync_that_was_not_due_is_not_news() {
    devlog::SyncWatch w;
    TEST_ASSERT_EQUAL_STRING("", w.step(100, false, false, 0).c_str());
}

void test_a_quick_good_sync_is_not_news() {
    devlog::SyncWatch w;
    TEST_ASSERT_EQUAL_STRING("", w.step(100, true, true, 40).c_str());
}

void test_a_slow_good_sync_is_news() {
    devlog::SyncWatch w;
    TEST_ASSERT_EQUAL_STRING("time sync slow: 640 ms", w.step(100, true, true, 640).c_str());
}

void test_the_first_failure_says_how_long_it_held_the_loop() {
    devlog::SyncWatch w;
    TEST_ASSERT_EQUAL_STRING("time sync failed (1012 ms), retrying every pass", w.step(100, true, false, 1012).c_str());
}

void test_failures_after_the_first_are_counted_not_logged() {
    devlog::SyncWatch w;
    w.step(100, true, false, 1012);
    TEST_ASSERT_EQUAL_STRING("", w.step(102, true, false, 2030).c_str());
    TEST_ASSERT_EQUAL_STRING("", w.step(104, true, false, 2030).c_str());
}

void test_recovery_says_how_many_failures_and_how_long_they_held_the_loop() {
    devlog::SyncWatch w;
    w.step(100, true, false, 1000);
    w.step(102, true, false, 2000);
    w.step(104, true, false, 2000);
    TEST_ASSERT_EQUAL_STRING("time sync ok again after 3 failures in 6 s, 5000 ms blocked",
                             w.step(106, true, true, 50).c_str());
    // and the next failure is news again
    TEST_ASSERT_EQUAL_STRING("", w.step(170, true, true, 50).c_str());
    TEST_ASSERT_EQUAL_STRING("time sync failed (1001 ms), retrying every pass", w.step(230, true, false, 1001).c_str());
}

// ---- slow loop passes ----

static const devlog::PassParts kNtpHeld{2011, 3, 0, 18};
static const devlog::PassParts kQuiet{0, 2, 0, 15};

void test_a_normal_pass_is_not_news() {
    devlog::SlowWatch w;
    TEST_ASSERT_EQUAL_STRING("", w.step(100, 300, kQuiet).c_str());
}

void test_one_or_two_slow_passes_are_not_news() {
    // a page switch or one data fetch: the colon misses a beat at most
    devlog::SlowWatch w;
    TEST_ASSERT_EQUAL_STRING("", w.step(100, 2040, kNtpHeld).c_str());
    TEST_ASSERT_EQUAL_STRING("", w.step(102, 2040, kNtpHeld).c_str());
    TEST_ASSERT_EQUAL_STRING("", w.step(104, 300, kQuiet).c_str());
}

void test_the_third_slow_pass_in_a_row_says_what_held_it() {
    devlog::SlowWatch w;
    w.step(100, 2040, kNtpHeld);
    w.step(102, 2040, kNtpHeld);
    TEST_ASSERT_EQUAL_STRING("slow loop: 3 passes over 1 s, last 2032 ms (ntp 2011 page 3 mqtt 0 draw 18)",
                             w.step(104, 2032, kNtpHeld).c_str());
    TEST_ASSERT_EQUAL_STRING("", w.step(106, 2040, kNtpHeld).c_str());
}

void test_the_end_of_a_run_gives_its_length_and_its_longest_pass() {
    devlog::SlowWatch w;
    w.step(100, 2040, kNtpHeld);
    w.step(102, 2100, devlog::PassParts{2080, 2, 0, 18});
    w.step(104, 2032, kNtpHeld);
    w.step(106, 2040, kNtpHeld);
    TEST_ASSERT_EQUAL_STRING("slow loop end: 4 passes/8 s, max 2100 ms (ntp 2080 page 2 mqtt 0 draw 18)",
                             w.step(108, 300, kQuiet).c_str());
    TEST_ASSERT_EQUAL_STRING("", w.step(109, 300, kQuiet).c_str());
}

void test_a_pass_of_exactly_one_second_counts_as_slow() {
    // two seconds of the clock in one pass is what freezes the colon; a pass
    // of a whole second already lets the clock miss one
    devlog::SlowWatch w;
    w.step(100, 1000, kQuiet);
    w.step(101, 1000, kQuiet);
    TEST_ASSERT_NOT_EQUAL(0, w.step(102, 1000, kQuiet).size());
}

void test_lines_fit_the_log() {
    devlog::SlowWatch w;
    const devlog::PassParts big{99999, 99999, 99999, 99999};
    w.step(100, 399996, big);
    w.step(500, 399996, big);
    const std::string start = w.step(900, 399996, big);
    const std::string end = w.step(99999, 300, kQuiet);
    TEST_ASSERT_TRUE(start.size() <= devlog::kMsgMax);
    TEST_ASSERT_TRUE(end.size() <= devlog::kMsgMax);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_a_sync_that_was_not_due_is_not_news);
    RUN_TEST(test_a_quick_good_sync_is_not_news);
    RUN_TEST(test_a_slow_good_sync_is_news);
    RUN_TEST(test_the_first_failure_says_how_long_it_held_the_loop);
    RUN_TEST(test_failures_after_the_first_are_counted_not_logged);
    RUN_TEST(test_recovery_says_how_many_failures_and_how_long_they_held_the_loop);
    RUN_TEST(test_a_normal_pass_is_not_news);
    RUN_TEST(test_one_or_two_slow_passes_are_not_news);
    RUN_TEST(test_the_third_slow_pass_in_a_row_says_what_held_it);
    RUN_TEST(test_the_end_of_a_run_gives_its_length_and_its_longest_pass);
    RUN_TEST(test_a_pass_of_exactly_one_second_counts_as_slow);
    RUN_TEST(test_lines_fit_the_log);
    return UNITY_END();
}
