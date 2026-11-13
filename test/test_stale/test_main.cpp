// Host tests for StalePolicy.h (issue #32):  pio test -e native -f test_stale
#include "StalePolicy.h"
#include <unity.h>

void setUp() {}
void tearDown() {}

static const uint32_t MIN = 60000;
static const uint32_t WEATHER = 10 * MIN; // WeatherWidget's refresh

void test_page_is_fresh_after_a_good_refresh() {
    stale::Tracker t;
    t.attempt(5000, true, WEATHER);
    TEST_ASSERT_FALSE(t.stale());
}

// A roam or a router restart costs one refresh: that must not mark the page
void test_one_missed_refresh_does_not_mark_the_page() {
    stale::Tracker t;
    t.attempt(0, true, WEATHER);
    t.attempt(10 * MIN + 2000, false, WEATHER);
    TEST_ASSERT_FALSE(t.stale());
}

void test_two_missed_refreshes_in_a_row_mark_the_page() {
    stale::Tracker t;
    t.attempt(0, true, WEATHER);
    t.attempt(10 * MIN + 2000, false, WEATHER);
    t.attempt(20 * MIN + 4000, false, WEATHER);
    TEST_ASSERT_TRUE(t.stale());
}

void test_a_good_refresh_clears_the_mark() {
    stale::Tracker t;
    t.attempt(0, true, WEATHER);
    t.attempt(10 * MIN, false, WEATHER);
    t.attempt(20 * MIN, false, WEATHER);
    t.attempt(30 * MIN, true, WEATHER);
    TEST_ASSERT_FALSE(t.stale());
}

// Only the page on show refreshes. One that was off screen for an hour comes
// back with hour-old data: if its first refresh fails, that is stale now.
void test_old_data_is_stale_on_its_first_missed_refresh() {
    stale::Tracker t;
    t.attempt(0, true, WEATHER);
    t.attempt(60 * MIN, false, WEATHER);
    TEST_ASSERT_TRUE(t.stale());
}

// No data since power-on: the page shows its own error. The clock starts at
// the first try, so the same two-refresh rule applies.
void test_page_that_never_had_data_follows_the_same_rule() {
    stale::Tracker t;
    t.attempt(1000, false, WEATHER);
    TEST_ASSERT_FALSE(t.stale());
    t.attempt(1000 + 10 * MIN, false, WEATHER);
    TEST_ASSERT_FALSE(t.stale());
    t.attempt(1000 + 20 * MIN, false, WEATHER);
    TEST_ASSERT_TRUE(t.stale());
}

// A Web Data page may refresh every 10 s. Two misses there is 20 s, less than
// a roam takes, so nothing is marked inside the three-minute floor.
void test_fast_pages_are_not_marked_inside_three_minutes() {
    stale::Tracker t;
    t.attempt(0, true, 10000);
    t.attempt(1 * MIN, false, 10000);
    t.attempt(2 * MIN, false, 10000);
    TEST_ASSERT_FALSE(t.stale());
    t.attempt(3 * MIN, false, 10000);
    TEST_ASSERT_TRUE(t.stale());
}

// An MQTT page has no refresh time: its broker connection is up or down, and
// is asked on every pass of loop()
void test_a_connection_down_three_minutes_is_stale() {
    stale::Tracker t;
    t.attempt(0, true, 0);
    t.attempt(179000, false, 0);
    TEST_ASSERT_FALSE(t.stale());
    t.attempt(180000, false, 0);
    TEST_ASSERT_TRUE(t.stale());
    t.attempt(181000, true, 0);
    TEST_ASSERT_FALSE(t.stale());
}

// millis() wraps after 49.7 days
void test_age_is_right_across_the_millis_wrap() {
    stale::Tracker t;
    const uint32_t beforeWrap = 0xFFFFFFFFu - 5 * MIN;
    t.attempt(beforeWrap, true, WEATHER);
    t.attempt(beforeWrap + 10 * MIN, false, WEATHER); // 5 min past the wrap
    TEST_ASSERT_FALSE(t.stale());
    t.attempt(beforeWrap + 20 * MIN, false, WEATHER);
    TEST_ASSERT_TRUE(t.stale());
}

void test_the_limit_is_two_refreshes_with_a_three_minute_floor() {
    TEST_ASSERT_EQUAL_UINT32(20 * MIN, stale::limitMs(WEATHER));
    TEST_ASSERT_EQUAL_UINT32(30 * MIN, stale::limitMs(15 * MIN)); // stocks
    TEST_ASSERT_EQUAL_UINT32(3 * MIN, stale::limitMs(10000));
    TEST_ASSERT_EQUAL_UINT32(3 * MIN, stale::limitMs(0));
}

void test_fresh_page_gets_no_mark_whatever_the_wifi_does() {
    TEST_ASSERT_TRUE(stale::Mark::None == stale::mark(false, true));
    TEST_ASSERT_TRUE(stale::Mark::None == stale::mark(false, false));
}

void test_stale_page_without_wifi_gets_the_broken_ring() {
    TEST_ASSERT_TRUE(stale::Mark::NoWifi == stale::mark(true, false));
}

void test_stale_page_with_wifi_up_gets_the_solid_ring() {
    TEST_ASSERT_TRUE(stale::Mark::Old == stale::mark(true, true));
}

// The broken ring: 12 dashes, in TFT_eSPI's degrees (0 at 6 o'clock, clockwise)
void test_dashes_stay_in_one_turn_and_never_touch() {
    int prevEnd = -1;
    for (int i = 0; i < stale::kDashes; i++) {
        const stale::Arc a = stale::dash(i);
        TEST_ASSERT_TRUE(a.start > prevEnd);
        TEST_ASSERT_TRUE(a.end > a.start);
        TEST_ASSERT_TRUE(a.end < 360);
        prevEnd = a.end;
    }
    TEST_ASSERT_TRUE(stale::dash(0).start > 0); // a gap at 6 o'clock, not a dash cut in two by the 0/360 seam
}

void test_dashes_and_gaps_are_near_equal() {
    for (int i = 0; i < stale::kDashes; i++) {
        const stale::Arc a = stale::dash(i);
        const int nextStart = i + 1 < stale::kDashes ? stale::dash(i + 1).start : stale::dash(0).start + 360;
        TEST_ASSERT_INT_WITHIN(2, 15, a.end - a.start);
        TEST_ASSERT_INT_WITHIN(2, 15, nextStart - a.end);
    }
}

// Left and right halves match, so the ring does not look rotated
void test_dashes_mirror_left_to_right() {
    bool lit[360] = {};
    for (int i = 0; i < stale::kDashes; i++) {
        const stale::Arc a = stale::dash(i);
        for (int d = a.start; d < a.end; d++) {
            lit[d] = true;
        }
    }
    for (int d = 0; d < 360; d++) {
        TEST_ASSERT_EQUAL(lit[d], lit[359 - d]);
    }
}

// When WiFi comes back the pages refresh at once rather than at their next
// 10 or 15 minute turn
void test_link_coming_back_is_reported_once() {
    stale::LinkWatch w;
    TEST_ASSERT_FALSE(w.cameBack(true)); // up since start-up: nothing came back
    TEST_ASSERT_FALSE(w.cameBack(false));
    TEST_ASSERT_FALSE(w.cameBack(false));
    TEST_ASSERT_TRUE(w.cameBack(true));
    TEST_ASSERT_FALSE(w.cameBack(true));
}

// ---- Overlay: which orbs to paint on each pass. Weather's data is on orbs
// 2 to 5 (0x1E); orb 2 (bit 1) also carries the "No WiFi" plate.

void test_nothing_is_drawn_while_the_page_is_fresh() {
    stale::Overlay o;
    const stale::Paint p = o.step(0x1F, stale::Mark::None, 0x1E);
    TEST_ASSERT_FALSE(p.repaint);
    TEST_ASSERT_EQUAL_HEX8(0, p.orbs);
}

void test_the_mark_goes_on_over_the_page_without_a_repaint() {
    stale::Overlay o;
    o.step(0x1F, stale::Mark::None, 0x1E);
    const stale::Paint p = o.step(0, stale::Mark::Old, 0x1E);
    TEST_ASSERT_FALSE(p.repaint); // no flicker when the mark comes up
    TEST_ASSERT_EQUAL_HEX8(0x1E, p.orbs);
}

void test_the_mark_is_drawn_once_not_on_every_pass() {
    stale::Overlay o;
    o.step(0, stale::Mark::Old, 0x1E);
    const stale::Paint p = o.step(0, stale::Mark::Old, 0x1E);
    TEST_ASSERT_FALSE(p.repaint);
    TEST_ASSERT_EQUAL_HEX8(0, p.orbs);
}

void test_an_orb_the_page_wiped_gets_its_mark_back() {
    stale::Overlay o;
    o.step(0, stale::Mark::Old, 0x1E);
    const stale::Paint p = o.step(0x04, stale::Mark::Old, 0x1E); // the page redrew orb 3
    TEST_ASSERT_FALSE(p.repaint);
    TEST_ASSERT_EQUAL_HEX8(0x04, p.orbs);
}

void test_no_wifi_also_marks_orb_2_when_it_holds_no_data() {
    stale::Overlay o;
    const stale::Paint p = o.step(0, stale::Mark::NoWifi, 0x11); // data on orbs 1 and 5 only
    TEST_ASSERT_EQUAL_HEX8(0x13, p.orbs); // orb 2 for the words
}

void test_the_mark_comes_off_by_repainting_the_page() {
    stale::Overlay o;
    o.step(0, stale::Mark::Old, 0x1E);
    const stale::Paint p = o.step(0, stale::Mark::None, 0x1E);
    TEST_ASSERT_TRUE(p.repaint);
    TEST_ASSERT_EQUAL_HEX8(0, p.orbs);
    const stale::Paint next = o.step(0, stale::Mark::None, 0x1E);
    TEST_ASSERT_FALSE(next.repaint); // once
}

void test_a_change_of_shape_repaints_then_draws_the_new_one() {
    stale::Overlay o;
    o.step(0, stale::Mark::NoWifi, 0x1E);
    const stale::Paint p = o.step(0, stale::Mark::Old, 0x1E); // WiFi back, data still old
    TEST_ASSERT_TRUE(p.repaint); // the plate and the gaps have to go
    TEST_ASSERT_EQUAL_HEX8(0x1E, p.orbs);
}

void test_a_page_switch_needs_no_extra_repaint() {
    stale::Overlay o;
    o.step(0, stale::Mark::NoWifi, 0x1E);
    // The switch wiped all five orbs and drew a fresh page
    const stale::Paint p = o.step(0x1F, stale::Mark::None, 0x1F);
    TEST_ASSERT_FALSE(p.repaint);
    TEST_ASSERT_EQUAL_HEX8(0, p.orbs);
}

void test_a_switch_to_another_stale_page_marks_its_orbs() {
    stale::Overlay o;
    o.step(0, stale::Mark::NoWifi, 0x1E);
    const stale::Paint p = o.step(0x1F, stale::Mark::NoWifi, 0x07); // three stocks
    TEST_ASSERT_FALSE(p.repaint);
    TEST_ASSERT_EQUAL_HEX8(0x07, p.orbs);
}

void test_an_orb_that_stops_holding_data_loses_its_ring() {
    stale::Overlay o;
    o.step(0, stale::Mark::Old, 0x1F);
    const stale::Paint p = o.step(0, stale::Mark::Old, 0x0F); // orb 5's topic was removed
    TEST_ASSERT_TRUE(p.repaint);
    TEST_ASSERT_EQUAL_HEX8(0x0F, p.orbs);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_page_is_fresh_after_a_good_refresh);
    RUN_TEST(test_one_missed_refresh_does_not_mark_the_page);
    RUN_TEST(test_two_missed_refreshes_in_a_row_mark_the_page);
    RUN_TEST(test_a_good_refresh_clears_the_mark);
    RUN_TEST(test_old_data_is_stale_on_its_first_missed_refresh);
    RUN_TEST(test_page_that_never_had_data_follows_the_same_rule);
    RUN_TEST(test_fast_pages_are_not_marked_inside_three_minutes);
    RUN_TEST(test_a_connection_down_three_minutes_is_stale);
    RUN_TEST(test_age_is_right_across_the_millis_wrap);
    RUN_TEST(test_the_limit_is_two_refreshes_with_a_three_minute_floor);
    RUN_TEST(test_fresh_page_gets_no_mark_whatever_the_wifi_does);
    RUN_TEST(test_stale_page_without_wifi_gets_the_broken_ring);
    RUN_TEST(test_stale_page_with_wifi_up_gets_the_solid_ring);
    RUN_TEST(test_dashes_stay_in_one_turn_and_never_touch);
    RUN_TEST(test_dashes_and_gaps_are_near_equal);
    RUN_TEST(test_dashes_mirror_left_to_right);
    RUN_TEST(test_link_coming_back_is_reported_once);
    RUN_TEST(test_nothing_is_drawn_while_the_page_is_fresh);
    RUN_TEST(test_the_mark_goes_on_over_the_page_without_a_repaint);
    RUN_TEST(test_the_mark_is_drawn_once_not_on_every_pass);
    RUN_TEST(test_an_orb_the_page_wiped_gets_its_mark_back);
    RUN_TEST(test_no_wifi_also_marks_orb_2_when_it_holds_no_data);
    RUN_TEST(test_the_mark_comes_off_by_repainting_the_page);
    RUN_TEST(test_a_change_of_shape_repaints_then_draws_the_new_one);
    RUN_TEST(test_a_page_switch_needs_no_extra_repaint);
    RUN_TEST(test_a_switch_to_another_stale_page_marks_its_orbs);
    RUN_TEST(test_an_orb_that_stops_holding_data_loses_its_ring);
    return UNITY_END();
}
