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

// --- events waiting to be written ---------------------------------------------

void test_the_inbox_hands_events_back_in_order() {
    devlog::Inbox in;
    TEST_ASSERT_TRUE(in.push(10, "first"));
    TEST_ASSERT_TRUE(in.push(11, "second"));
    devlog::Entry e;
    TEST_ASSERT_TRUE(in.pop(e));
    TEST_ASSERT_EQUAL(10, e.up);
    TEST_ASSERT_EQUAL_STRING("first", e.msg);
    TEST_ASSERT_TRUE(in.pop(e));
    TEST_ASSERT_EQUAL_STRING("second", e.msg);
    TEST_ASSERT_FALSE(in.pop(e));
}

void test_a_full_inbox_keeps_the_oldest_and_counts_the_rest() {
    // The WiFi task can post faster than the main loop writes: what came
    // first says how the trouble began
    devlog::Inbox in;
    for (int i = 0; i < devlog::kInboxSize; i++) {
        TEST_ASSERT_TRUE(in.push(i, msgN(i).c_str()));
    }
    TEST_ASSERT_FALSE(in.push(99, "no room"));
    TEST_ASSERT_FALSE(in.push(99, "no room"));
    TEST_ASSERT_EQUAL(2, in.takeLost());
    TEST_ASSERT_EQUAL(0, in.takeLost()); // reported once
    devlog::Entry e;
    TEST_ASSERT_TRUE(in.pop(e));
    TEST_ASSERT_EQUAL_STRING("event 0", e.msg);
}

void test_the_inbox_cuts_a_long_event_to_fit() {
    devlog::Inbox in;
    in.push(1, std::string(300, 'x').c_str());
    devlog::Entry e;
    in.pop(e);
    TEST_ASSERT_EQUAL(kMsgMax, std::string(e.msg).size());
}

void test_the_inbox_is_reusable_past_its_size() {
    devlog::Inbox in;
    devlog::Entry e;
    for (int i = 0; i < devlog::kInboxSize * 3; i++) {
        TEST_ASSERT_TRUE(in.push(i, msgN(i).c_str()));
        TEST_ASSERT_TRUE(in.pop(e));
        TEST_ASSERT_EQUAL_STRING(msgN(i).c_str(), e.msg);
    }
    TEST_ASSERT_EQUAL(0, in.takeLost());
}

// --- the time of an event that waited -----------------------------------------

void test_an_event_is_dated_when_it_happened_not_when_it_was_written() {
    // clock read at 500 s of uptime; the event was at 490 s
    TEST_ASSERT_EQUAL(kMorning - 10, devlog::epochAt(kMorning, 500, 490));
    // and one after the last clock reading (the line before a restart)
    TEST_ASSERT_EQUAL(kMorning + 7, devlog::epochAt(kMorning, 500, 507));
}

void test_without_a_clock_an_event_has_no_date() {
    TEST_ASSERT_EQUAL(0, devlog::epochAt(kNoClock, 500, 490));
}

void test_the_clock_counts_as_set_only_with_a_real_date() {
    // Before the first time sync the clock library counts up from zero, plus
    // or minus the time zone
    TEST_ASSERT_FALSE(devlog::clockSet(0));
    TEST_ASSERT_FALSE(devlog::clockSet(42));
    TEST_ASSERT_FALSE(devlog::clockSet(-18000 + 42));
    TEST_ASSERT_TRUE(devlog::clockSet(kMorning));
}

// --- room on the filesystem ----------------------------------------------------

void test_a_small_filesystem_gets_smaller_files() {
    // 384 KB on the 8 MB board, 128 KB on a 4 MB one: the start-up picture
    // lives there too
    TEST_ASSERT_EQUAL(16384, devlog::fileCap(384 * 1024));
    TEST_ASSERT_EQUAL(8192, devlog::fileCap(128 * 1024));
    TEST_ASSERT_FALSE(devlog::needsRotate(8000, 80, devlog::fileCap(128 * 1024)));
    TEST_ASSERT_TRUE(devlog::needsRotate(8150, 80, devlog::fileCap(128 * 1024)));
}

// --- WiFi disconnect reasons ---------------------------------------------------

void test_each_disconnect_reason_is_news_once_per_outage() {
    // A join every 30 s can fail with two reasons in turn for hours
    devlog::ReasonsSeen seen;
    TEST_ASSERT_TRUE(seen.firstTime(8));
    TEST_ASSERT_TRUE(seen.firstTime(201));
    TEST_ASSERT_FALSE(seen.firstTime(8));
    TEST_ASSERT_FALSE(seen.firstTime(201));
    TEST_ASSERT_TRUE(seen.firstTime(0));
    TEST_ASSERT_TRUE(seen.firstTime(255));
    TEST_ASSERT_FALSE(seen.firstTime(255));
}

void test_the_next_outage_reports_its_reasons_again() {
    devlog::ReasonsSeen seen;
    seen.firstTime(8);
    seen.reset();
    TEST_ASSERT_TRUE(seen.firstTime(8));
}

// --- a page's refreshes ---------------------------------------------------------

void test_good_refreshes_are_not_news() {
    devlog::PageWatch w;
    TEST_ASSERT_EQUAL_STRING("", w.step(100, true, false).c_str());
    TEST_ASSERT_EQUAL_STRING("", w.step(700, true, false).c_str());
}

void test_the_first_failed_refresh_is_news_the_next_is_not() {
    devlog::PageWatch w;
    w.step(100, true, false);
    TEST_ASSERT_EQUAL_STRING("refresh failed", w.step(700, false, false).c_str());
    TEST_ASSERT_EQUAL_STRING("", w.step(1300, false, false).c_str());
}

void test_going_stale_is_news_once() {
    devlog::PageWatch w;
    w.step(700, false, false);
    TEST_ASSERT_EQUAL_STRING("data now stale", w.step(1300, false, true).c_str());
    TEST_ASSERT_EQUAL_STRING("", w.step(1900, false, true).c_str());
}

void test_a_first_miss_that_is_already_stale_says_both() {
    // a page that was off screen for an hour
    devlog::PageWatch w;
    w.step(100, true, false);
    TEST_ASSERT_EQUAL_STRING("refresh failed, data now stale", w.step(4000, false, true).c_str());
}

void test_recovery_says_how_long_the_trouble_lasted() {
    devlog::PageWatch w;
    w.step(100, true, false);
    w.step(700, false, false);
    w.step(1300, false, true);
    TEST_ASSERT_EQUAL_STRING("ok again after 1200 s", w.step(1900, true, false).c_str()); // first failure at 700
    TEST_ASSERT_EQUAL_STRING("", w.step(2500, true, false).c_str());
    // and the next trouble is news again
    TEST_ASSERT_EQUAL_STRING("refresh failed", w.step(3100, false, false).c_str());
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
    RUN_TEST(test_the_inbox_hands_events_back_in_order);
    RUN_TEST(test_a_full_inbox_keeps_the_oldest_and_counts_the_rest);
    RUN_TEST(test_the_inbox_cuts_a_long_event_to_fit);
    RUN_TEST(test_the_inbox_is_reusable_past_its_size);
    RUN_TEST(test_an_event_is_dated_when_it_happened_not_when_it_was_written);
    RUN_TEST(test_without_a_clock_an_event_has_no_date);
    RUN_TEST(test_the_clock_counts_as_set_only_with_a_real_date);
    RUN_TEST(test_a_small_filesystem_gets_smaller_files);
    RUN_TEST(test_each_disconnect_reason_is_news_once_per_outage);
    RUN_TEST(test_the_next_outage_reports_its_reasons_again);
    RUN_TEST(test_good_refreshes_are_not_news);
    RUN_TEST(test_the_first_failed_refresh_is_news_the_next_is_not);
    RUN_TEST(test_going_stale_is_news_once);
    RUN_TEST(test_a_first_miss_that_is_already_stale_says_both);
    RUN_TEST(test_recovery_says_how_long_the_trouble_lasted);
    return UNITY_END();
}
