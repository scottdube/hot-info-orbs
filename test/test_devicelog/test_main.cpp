// Host tests for DeviceLog.h:  pio test -e native
#include "DeviceLog.h"
#include <string>
#include <unity.h>

void setUp() {}
void tearDown() {}

using devlog::kLinesPerHour;
using devlog::kMsgMax;

static const int64_t kNoClock = 0;
static const int64_t kMorning = 1794567138; // 2026-11-13 10:52:18, already local

static std::string msgN(int n) {
    return "event " + std::to_string(n);
}

void test_a_line_carries_the_clock_the_uptime_and_the_event() {
    TEST_ASSERT_EQUAL_STRING("2026-11-13 10:52:18 | 0d 00:16:42 | wifi down, reason 202\n",
                             devlog::line(kMorning, 1002, "wifi down, reason 202").c_str());
}

void test_before_the_clock_is_set_the_date_is_dashes_of_the_same_width() {
    const std::string known = devlog::line(kMorning, 5, "boot");
    const std::string unknown = devlog::line(kNoClock, 5, "boot");
    TEST_ASSERT_EQUAL_STRING("---------- --:--:-- | 0d 00:00:05 | boot\n", unknown.c_str());
    TEST_ASSERT_EQUAL(known.size(), unknown.size());
}

void test_uptime_rolls_into_days() {
    TEST_ASSERT_EQUAL_STRING("0d 00:00:00", devlog::upText(0).c_str());
    TEST_ASSERT_EQUAL_STRING("0d 23:59:59", devlog::upText(86399).c_str());
    TEST_ASSERT_EQUAL_STRING("12d 03:04:05", devlog::upText(12 * 86400 + 3 * 3600 + 4 * 60 + 5).c_str());
}

void test_an_event_is_kept_to_one_plain_line() {
    // A line break inside an event would let one event pass for two
    TEST_ASSERT_EQUAL_STRING("a b c", devlog::clean("a\nb\rc").c_str());
    TEST_ASSERT_EQUAL_STRING("tab here", devlog::clean("tab\there").c_str());
    TEST_ASSERT_EQUAL_STRING("caf??", devlog::clean("caf\xC3\xA9").c_str()); // plain ASCII only
}

void test_a_long_event_is_cut_at_the_limit() {
    const std::string cut = devlog::clean(std::string(300, 'x'));
    TEST_ASSERT_EQUAL(kMsgMax, cut.size());
    TEST_ASSERT_EQUAL_STRING("...", cut.substr(kMsgMax - 3).c_str());
}

void test_the_first_event_is_written_at_once() {
    devlog::Writer w;
    TEST_ASSERT_EQUAL_STRING("2026-11-13 10:52:18 | 0d 00:16:42 | wifi down, reason 202\n",
                             w.add(kMorning, 1002, "wifi down, reason 202").c_str());
    TEST_ASSERT_EQUAL_STRING("", w.pending().c_str());
}

void test_the_same_event_again_is_counted_not_written() {
    devlog::Writer w;
    w.add(kMorning, 100, "weather refresh failed");
    TEST_ASSERT_EQUAL_STRING("", w.add(kMorning + 600, 700, "weather refresh failed").c_str());
    TEST_ASSERT_EQUAL_STRING("", w.add(kMorning + 1200, 1300, "weather refresh failed").c_str());
}

void test_the_count_is_written_when_something_else_happens() {
    devlog::Writer w;
    w.add(kMorning, 100, "weather refresh failed");
    w.add(kMorning + 600, 700, "weather refresh failed");
    w.add(kMorning + 1200, 1300, "weather refresh failed");
    TEST_ASSERT_EQUAL_STRING("2026-11-13 11:12:18 | 0d 00:21:40 | same as the line above, 2 more times\n"
                             "2026-11-13 11:13:18 | 0d 00:22:40 | weather fresh again\n",
                             w.add(kMorning + 1260, 1360, "weather fresh again").c_str());
}

void test_a_count_not_yet_written_shows_as_pending() {
    devlog::Writer w;
    w.add(kMorning, 100, "weather refresh failed");
    w.add(kMorning + 600, 700, "weather refresh failed");
    TEST_ASSERT_EQUAL_STRING("2026-11-13 11:02:18 | 0d 00:11:40 | same as the line above, 1 more time\n", w.pending().c_str());
}

void test_flush_hands_over_the_pending_count_once() {
    devlog::Writer w;
    w.add(kMorning, 100, "weather refresh failed");
    w.add(kMorning + 600, 700, "weather refresh failed");
    TEST_ASSERT_EQUAL_STRING("2026-11-13 11:02:18 | 0d 00:11:40 | same as the line above, 1 more time\n", w.flush().c_str());
    TEST_ASSERT_EQUAL_STRING("", w.flush().c_str());
    TEST_ASSERT_EQUAL_STRING("", w.pending().c_str());
}

void test_a_chatty_hour_is_capped() {
    // A source nobody foresaw must not wear the flash or push the history out
    devlog::Writer w;
    for (int i = 0; i < kLinesPerHour; i++) {
        TEST_ASSERT_TRUE(w.add(kMorning + i, 10 + i, msgN(i)).size() > 0);
    }
    TEST_ASSERT_EQUAL_STRING("", w.add(kMorning + 100, 110, "one too many").c_str());
    TEST_ASSERT_EQUAL_STRING("", w.add(kMorning + 101, 111, "two too many").c_str());
}

void test_the_next_hour_says_how_many_were_dropped() {
    devlog::Writer w;
    for (int i = 0; i < kLinesPerHour; i++) {
        w.add(kMorning + i, 10 + i, msgN(i));
    }
    w.add(kMorning + 100, 110, "one too many");
    w.add(kMorning + 101, 111, "two too many");
    TEST_ASSERT_EQUAL_STRING("2026-11-13 11:52:28 | 0d 01:00:10 | 2 events not logged: more than 60 in an hour\n"
                             "2026-11-13 11:52:28 | 0d 01:00:10 | back to normal\n",
                             w.add(kMorning + 3610, 3610, "back to normal").c_str());
}

void test_repeats_do_not_use_up_the_hour() {
    devlog::Writer w;
    w.add(kMorning, 10, "stuck");
    for (int i = 0; i < 500; i++) {
        w.add(kMorning + i, 10 + i, "stuck");
    }
    TEST_ASSERT_TRUE(w.add(kMorning + 600, 610, "moved on").size() > 0);
}

void test_a_file_rolls_over_when_the_next_text_would_pass_the_cap() {
    TEST_ASSERT_FALSE(devlog::needsRotate(1000, 80));
    TEST_ASSERT_FALSE(devlog::needsRotate(devlog::kFileCap - 80, 80)); // lands exactly on the cap
    TEST_ASSERT_TRUE(devlog::needsRotate(devlog::kFileCap - 79, 80));
}

void test_an_empty_file_never_rolls_over() {
    // or one oversized text would rotate on every write and wipe both files
    TEST_ASSERT_FALSE(devlog::needsRotate(0, devlog::kFileCap + 1));
}

void test_a_long_outage_reports_at_widening_intervals() {
    devlog::Milestones m;
    TEST_ASSERT_FALSE(m.due(59));
    TEST_ASSERT_TRUE(m.due(60));
    TEST_ASSERT_FALSE(m.due(61)); // once each
    TEST_ASSERT_FALSE(m.due(299));
    TEST_ASSERT_TRUE(m.due(300));
    TEST_ASSERT_TRUE(m.due(900));
    TEST_ASSERT_TRUE(m.due(3600));
    TEST_ASSERT_FALSE(m.due(7199));
    TEST_ASSERT_TRUE(m.due(7200)); // then hourly
    TEST_ASSERT_TRUE(m.due(10800));
}

void test_a_late_first_look_reports_once_not_once_per_missed_step() {
    devlog::Milestones m;
    TEST_ASSERT_TRUE(m.due(1000)); // 60, 300 and 900 all passed
    TEST_ASSERT_FALSE(m.due(1001));
    TEST_ASSERT_TRUE(m.due(3600));
}

void test_milestones_start_over_for_the_next_outage() {
    devlog::Milestones m;
    m.due(4000);
    m.reset();
    TEST_ASSERT_FALSE(m.due(30));
    TEST_ASSERT_TRUE(m.due(60));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_a_line_carries_the_clock_the_uptime_and_the_event);
    RUN_TEST(test_before_the_clock_is_set_the_date_is_dashes_of_the_same_width);
    RUN_TEST(test_uptime_rolls_into_days);
    RUN_TEST(test_an_event_is_kept_to_one_plain_line);
    RUN_TEST(test_a_long_event_is_cut_at_the_limit);
    RUN_TEST(test_the_first_event_is_written_at_once);
    RUN_TEST(test_the_same_event_again_is_counted_not_written);
    RUN_TEST(test_the_count_is_written_when_something_else_happens);
    RUN_TEST(test_a_count_not_yet_written_shows_as_pending);
    RUN_TEST(test_flush_hands_over_the_pending_count_once);
    RUN_TEST(test_a_chatty_hour_is_capped);
    RUN_TEST(test_the_next_hour_says_how_many_were_dropped);
    RUN_TEST(test_repeats_do_not_use_up_the_hour);
    RUN_TEST(test_a_file_rolls_over_when_the_next_text_would_pass_the_cap);
    RUN_TEST(test_an_empty_file_never_rolls_over);
    RUN_TEST(test_a_long_outage_reports_at_widening_intervals);
    RUN_TEST(test_a_late_first_look_reports_once_not_once_per_missed_step);
    RUN_TEST(test_milestones_start_over_for_the_next_outage);
    return UNITY_END();
}
