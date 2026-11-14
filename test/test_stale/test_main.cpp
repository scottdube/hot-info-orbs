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
    TEST_ASSERT_EQUAL_UINT32(30 * MIN, stale::limitMs(15 * MIN));
    TEST_ASSERT_EQUAL_UINT32(3 * MIN, stale::limitMs(10000));
    TEST_ASSERT_EQUAL_UINT32(3 * MIN, stale::limitMs(0));
}

// Scott, 2026-11-13: weather and stocks ring at the same age. Stocks refresh
// every 15 min, so two misses came to 30.
static const uint32_t STOCKS = 15 * MIN; // StockWidget's refresh

void test_the_fetch_limit_is_the_weather_pages_two_refreshes() {
    TEST_ASSERT_EQUAL_UINT32(20 * MIN, stale::kFetchLimitMs);
    TEST_ASSERT_EQUAL_UINT32(stale::limitMs(WEATHER), stale::kFetchLimitMs);
}

void test_a_good_refresh_is_followed_by_the_normal_wait() {
    TEST_ASSERT_EQUAL_UINT32(STOCKS, stale::nextWaitMs(STOCKS, stale::kFetchLimitMs, 0));
    TEST_ASSERT_EQUAL_UINT32(WEATHER, stale::nextWaitMs(WEATHER, stale::kFetchLimitMs, 0));
}

void test_stocks_try_again_early_after_the_first_miss() {
    TEST_ASSERT_EQUAL_UINT32(5 * MIN, stale::nextWaitMs(STOCKS, stale::kFetchLimitMs, 1));
}

// One early try per outage: the price service's key is shared by every builder
void test_stocks_go_back_to_the_normal_wait_after_the_second_miss() {
    TEST_ASSERT_EQUAL_UINT32(STOCKS, stale::nextWaitMs(STOCKS, stale::kFetchLimitMs, 2));
    TEST_ASSERT_EQUAL_UINT32(STOCKS, stale::nextWaitMs(STOCKS, stale::kFetchLimitMs, 9));
}

void test_a_page_whose_second_refresh_already_lands_on_the_limit_waits_as_usual() {
    TEST_ASSERT_EQUAL_UINT32(WEATHER, stale::nextWaitMs(WEATHER, stale::kFetchLimitMs, 1));
    TEST_ASSERT_EQUAL_UINT32(30 * MIN, stale::nextWaitMs(30 * MIN, stale::kFetchLimitMs, 1)); // one miss is past the limit
    TEST_ASSERT_EQUAL_UINT32(5 * MIN, stale::nextWaitMs(5 * MIN, stale::kFetchLimitMs, 1)); // never longer than normal
}

void test_stocks_ring_at_twenty_minutes_like_weather() {
    stale::Tracker t;
    uint32_t now = 1000;
    t.attemptWithin(now, true, stale::kFetchLimitMs);
    now += stale::nextWaitMs(STOCKS, stale::kFetchLimitMs, 0) + 2000; // a try starts a little late
    t.attemptWithin(now, false, stale::kFetchLimitMs); // 15 min: first miss
    TEST_ASSERT_FALSE(t.stale());
    now += stale::nextWaitMs(STOCKS, stale::kFetchLimitMs, 1) + 2000;
    t.attemptWithin(now, false, stale::kFetchLimitMs); // 20 min: second miss
    TEST_ASSERT_TRUE(t.stale());
    now += stale::nextWaitMs(STOCKS, stale::kFetchLimitMs, 2) + 2000;
    t.attemptWithin(now, true, stale::kFetchLimitMs);
    TEST_ASSERT_FALSE(t.stale());
}

// mark(page stale, WiFi out, page fetches data). "WiFi out" = down for longer
// than the grace below.
void test_fresh_page_with_wifi_up_gets_no_mark() {
    TEST_ASSERT_TRUE(stale::Mark::None == stale::mark(false, false, true));
}

// Scott on the rig, 2026-11-12: the orb knows at once that WiFi is gone, so it
// should say so at once, not three minutes later when the data is old
void test_wifi_out_says_so_before_the_data_is_old() {
    TEST_ASSERT_TRUE(stale::Mark::LinkDown == stale::mark(false, true, true));
}

void test_stale_page_without_wifi_gets_the_broken_ring() {
    TEST_ASSERT_TRUE(stale::Mark::NoWifi == stale::mark(true, true, true));
}

void test_stale_page_with_wifi_up_gets_the_solid_ring() {
    TEST_ASSERT_TRUE(stale::Mark::Old == stale::mark(true, false, true));
}

// The clock keeps time without WiFi
void test_a_page_that_fetches_nothing_is_never_marked() {
    TEST_ASSERT_TRUE(stale::Mark::None == stale::mark(false, true, false));
    TEST_ASSERT_TRUE(stale::Mark::None == stale::mark(true, true, false));
}

// ---- LinkGrace: WiFi counts as out once it has been down ten seconds
void test_a_short_drop_is_not_wifi_out() {
    stale::LinkGrace g;
    TEST_ASSERT_FALSE(g.out(1000, true));
    TEST_ASSERT_FALSE(g.out(2000, false)); // a roam between access points
    TEST_ASSERT_FALSE(g.out(2000 + stale::kLinkGraceMs - 1, false));
}

void test_wifi_is_out_after_ten_seconds_down() {
    stale::LinkGrace g;
    g.out(2000, false);
    TEST_ASSERT_TRUE(g.out(2000 + stale::kLinkGraceMs, false));
    TEST_ASSERT_TRUE(g.out(2000 + 5 * stale::kLinkGraceMs, false));
}

void test_wifi_is_back_at_once() {
    stale::LinkGrace g;
    g.out(2000, false);
    g.out(60000, false);
    TEST_ASSERT_FALSE(g.out(60001, true));
}

void test_a_second_drop_gets_its_own_ten_seconds() {
    stale::LinkGrace g;
    g.out(2000, false);
    g.out(60000, false);
    g.out(61000, true);
    TEST_ASSERT_FALSE(g.out(62000, false));
    TEST_ASSERT_FALSE(g.out(62000 + stale::kLinkGraceMs - 1, false));
    TEST_ASSERT_TRUE(g.out(62000 + stale::kLinkGraceMs, false));
}

void test_the_grace_is_right_across_the_millis_wrap() {
    stale::LinkGrace g;
    const uint32_t start = 0xFFFFFFFFu - 3000;
    g.out(start, false);
    TEST_ASSERT_FALSE(g.out(start + 5000, false)); // wrapped, 5 s down
    TEST_ASSERT_TRUE(g.out(start + stale::kLinkGraceMs, false));
}

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

// "No WiFi" alone: the words on orb 2, no ring, the data is still good
void test_wifi_out_puts_only_the_words_on_orb_2() {
    stale::Overlay o;
    const stale::Paint p = o.step(0, stale::Mark::LinkDown, 0x1E);
    TEST_ASSERT_FALSE(p.repaint);
    TEST_ASSERT_EQUAL_HEX8(0x02, p.orbs);
}

// Minutes later the data is old: the rings go on over the page, and orb 2 is
// painted again because it now needs a ring under its words
void test_the_ring_joins_the_words_without_a_repaint() {
    stale::Overlay o;
    o.step(0, stale::Mark::LinkDown, 0x1E);
    const stale::Paint p = o.step(0, stale::Mark::NoWifi, 0x1E);
    TEST_ASSERT_FALSE(p.repaint);
    TEST_ASSERT_EQUAL_HEX8(0x1E, p.orbs);
    const stale::Paint next = o.step(0, stale::Mark::NoWifi, 0x1E);
    TEST_ASSERT_EQUAL_HEX8(0, next.orbs); // and then it is left alone
}

void test_the_words_come_off_when_wifi_is_back() {
    stale::Overlay o;
    o.step(0, stale::Mark::LinkDown, 0x1E);
    const stale::Paint p = o.step(0, stale::Mark::None, 0x1E);
    TEST_ASSERT_TRUE(p.repaint);
    TEST_ASSERT_EQUAL_HEX8(0, p.orbs);
}

// Rings cannot be taken off without a repaint; the words then go back on
void test_rings_that_go_while_wifi_is_still_out_repaint_and_keep_the_words() {
    stale::Overlay o;
    o.step(0, stale::Mark::NoWifi, 0x1E);
    const stale::Paint p = o.step(0, stale::Mark::LinkDown, 0x1E);
    TEST_ASSERT_TRUE(p.repaint);
    TEST_ASSERT_EQUAL_HEX8(0x02, p.orbs);
}

// --- when the data last refreshed well, for the info page ----------------------

void test_no_good_refresh_yet_before_any_try() {
    stale::Tracker t;
    TEST_ASSERT_FALSE(t.lastGood().any);
}

void test_a_failed_first_try_is_not_a_good_refresh() {
    stale::Tracker t;
    t.attempt(1000, false, WEATHER);
    TEST_ASSERT_FALSE(t.lastGood().any);
}

void test_the_last_good_refresh_is_remembered_through_failures() {
    stale::Tracker t;
    t.attempt(5000, true, WEATHER);
    TEST_ASSERT_TRUE(t.lastGood().any);
    TEST_ASSERT_EQUAL_UINT32(5000, t.lastGood().at);
    t.attempt(5000 + 10 * MIN, false, WEATHER);
    t.attempt(5000 + 20 * MIN, false, WEATHER);
    TEST_ASSERT_TRUE(t.lastGood().any);
    TEST_ASSERT_EQUAL_UINT32(5000, t.lastGood().at);
    t.attempt(5000 + 30 * MIN, true, WEATHER);
    TEST_ASSERT_EQUAL_UINT32(5000 + 30 * MIN, t.lastGood().at);
}

void test_a_good_refresh_after_a_failed_first_try_counts() {
    stale::Tracker t;
    t.attempt(1000, false, WEATHER);
    t.attempt(1000 + 10 * MIN, true, WEATHER);
    TEST_ASSERT_TRUE(t.lastGood().any);
    TEST_ASSERT_EQUAL_UINT32(1000 + 10 * MIN, t.lastGood().at);
}

// ---- Clock only when WiFi is out (the setting; Yang's request, 2026-11-14) ----
static const int8_t CLOCK = 0, WEATHER_PAGE = 3, STOCKS_PAGE = 2;

void test_clock_only_does_nothing_while_the_setting_is_off() {
    stale::ClockOnly c;
    TEST_ASSERT_EQUAL_INT8(stale::kStay, c.step(false, true, WEATHER_PAGE, CLOCK));
    TEST_ASSERT_FALSE(c.active());
}

void test_clock_only_does_nothing_while_wifi_is_up() {
    stale::ClockOnly c;
    TEST_ASSERT_EQUAL_INT8(stale::kStay, c.step(true, false, WEATHER_PAGE, CLOCK));
    TEST_ASSERT_FALSE(c.active());
}

void test_wifi_out_switches_to_the_clock() {
    stale::ClockOnly c;
    TEST_ASSERT_EQUAL_INT8(CLOCK, c.step(true, true, WEATHER_PAGE, CLOCK));
    TEST_ASSERT_TRUE(c.active());
}

void test_an_outage_that_starts_on_the_clock_stays_there() {
    stale::ClockOnly c;
    TEST_ASSERT_EQUAL_INT8(stale::kStay, c.step(true, true, CLOCK, CLOCK));
    TEST_ASSERT_TRUE(c.active());
}

// One switch, not one on every pass: a button may move off the clock
void test_the_switch_to_the_clock_happens_once() {
    stale::ClockOnly c;
    c.step(true, true, WEATHER_PAGE, CLOCK);
    TEST_ASSERT_EQUAL_INT8(stale::kStay, c.step(true, true, CLOCK, CLOCK));
    TEST_ASSERT_EQUAL_INT8(stale::kStay, c.step(true, true, STOCKS_PAGE, CLOCK));
    TEST_ASSERT_TRUE(c.active());
}

void test_wifi_back_returns_to_the_page_that_was_up() {
    stale::ClockOnly c;
    c.step(true, true, WEATHER_PAGE, CLOCK);
    c.step(true, true, CLOCK, CLOCK);
    TEST_ASSERT_EQUAL_INT8(WEATHER_PAGE, c.step(true, false, CLOCK, CLOCK));
    TEST_ASSERT_FALSE(c.active());
}

void test_wifi_back_after_an_outage_that_began_on_the_clock_stays() {
    stale::ClockOnly c;
    c.step(true, true, CLOCK, CLOCK);
    TEST_ASSERT_EQUAL_INT8(stale::kStay, c.step(true, false, CLOCK, CLOCK));
    TEST_ASSERT_FALSE(c.active());
}

// Someone pressed a button during the outage and is reading that page
void test_wifi_back_leaves_a_page_picked_by_button_alone() {
    stale::ClockOnly c;
    c.step(true, true, WEATHER_PAGE, CLOCK);
    TEST_ASSERT_EQUAL_INT8(stale::kStay, c.step(true, false, STOCKS_PAGE, CLOCK));
    TEST_ASSERT_FALSE(c.active());
}

void test_switching_the_setting_off_in_an_outage_returns_too() {
    stale::ClockOnly c;
    c.step(true, true, WEATHER_PAGE, CLOCK);
    TEST_ASSERT_EQUAL_INT8(WEATHER_PAGE, c.step(false, true, CLOCK, CLOCK));
    TEST_ASSERT_FALSE(c.active());
}

void test_a_second_outage_remembers_its_own_page() {
    stale::ClockOnly c;
    c.step(true, true, WEATHER_PAGE, CLOCK);
    c.step(true, false, CLOCK, CLOCK);
    c.step(true, true, STOCKS_PAGE, CLOCK);
    TEST_ASSERT_EQUAL_INT8(STOCKS_PAGE, c.step(true, false, CLOCK, CLOCK));
}

void test_clock_only_needs_a_clock_page() {
    stale::ClockOnly c;
    TEST_ASSERT_EQUAL_INT8(stale::kStay, c.step(true, true, WEATHER_PAGE, -1));
    TEST_ASSERT_FALSE(c.active());
}

void test_the_clock_gets_its_own_mark_in_clock_only() {
    stale::ClockOnly c;
    c.step(true, true, WEATHER_PAGE, CLOCK);
    TEST_ASSERT_TRUE(stale::Mark::ClockOnly == c.mark(CLOCK, stale::Mark::None));
}

// A page reached by button in the outage keeps the marks it has today
void test_other_pages_keep_their_own_mark_in_clock_only() {
    stale::ClockOnly c;
    c.step(true, true, WEATHER_PAGE, CLOCK);
    TEST_ASSERT_TRUE(stale::Mark::NoWifi == c.mark(WEATHER_PAGE, stale::Mark::NoWifi));
    TEST_ASSERT_TRUE(stale::Mark::LinkDown == c.mark(STOCKS_PAGE, stale::Mark::LinkDown));
}

void test_the_clock_is_not_marked_outside_clock_only() {
    stale::ClockOnly c;
    TEST_ASSERT_TRUE(stale::Mark::None == c.mark(CLOCK, stale::Mark::None));
}

void test_the_clock_mark_is_a_broken_ring_with_words() {
    TEST_ASSERT_TRUE(stale::hasRing(stale::Mark::ClockOnly));
    TEST_ASSERT_TRUE(stale::hasWords(stale::Mark::ClockOnly)); // a ring alone did not say why
    TEST_ASSERT_TRUE(stale::broken(stale::Mark::ClockOnly));
}

// Orb 2 holds a digit on the clock. The colon orb has room above the colon.
void test_the_clock_label_goes_on_the_center_orb() {
    TEST_ASSERT_EQUAL(2, stale::plateOrb(stale::Mark::ClockOnly));
    TEST_ASSERT_EQUAL(1, stale::plateOrb(stale::Mark::NoWifi));
    TEST_ASSERT_EQUAL(1, stale::plateOrb(stale::Mark::LinkDown));
}

void test_the_label_takes_the_color_of_its_ring() {
    TEST_ASSERT_EQUAL_HEX16(stale::kRed, stale::plateColor(stale::Mark::ClockOnly));
    TEST_ASSERT_EQUAL_HEX16(stale::kAmber, stale::plateColor(stale::Mark::NoWifi));
    TEST_ASSERT_EQUAL_HEX16(stale::kAmber, stale::plateColor(stale::Mark::LinkDown));
}

// Broken = no WiFi, solid = WiFi up and the service is not answering
void test_the_shape_says_whether_wifi_is_out() {
    TEST_ASSERT_TRUE(stale::broken(stale::Mark::NoWifi));
    TEST_ASSERT_FALSE(stale::broken(stale::Mark::Old));
}

// Red only ever on the clock, amber only on data pages
void test_the_clock_ring_is_red_and_data_rings_are_amber() {
    // Full red with a little blue and no green. Pure red (0xF800) read as
    // orange on the panels, next to the amber (Scott, on the rig, 2026-11-14)
    TEST_ASSERT_EQUAL(31, stale::kRed >> 11);
    TEST_ASSERT_EQUAL(0, (stale::kRed >> 5) & 0x3F);
    TEST_ASSERT_TRUE((stale::kRed & 0x1F) >= 4);
    TEST_ASSERT_TRUE((stale::kRed & 0x1F) <= 10);
    TEST_ASSERT_TRUE(stale::kAmber != stale::kRed);
    TEST_ASSERT_TRUE(stale::kAmber != 0);
    TEST_ASSERT_EQUAL_HEX16(stale::kRed, stale::ringColor(stale::Mark::ClockOnly));
    TEST_ASSERT_EQUAL_HEX16(stale::kAmber, stale::ringColor(stale::Mark::NoWifi));
    TEST_ASSERT_EQUAL_HEX16(stale::kAmber, stale::ringColor(stale::Mark::Old));
}

void test_the_clock_ring_goes_on_every_clock_orb_without_a_repaint() {
    stale::Overlay o;
    const stale::Paint p = o.step(0, stale::Mark::ClockOnly, 0x1F);
    TEST_ASSERT_FALSE(p.repaint);
    TEST_ASSERT_EQUAL_HEX8(0x1F, p.orbs);
}

// The picture clock repaints its colon orb every second: no ring there
// The center orb is painted for its label even if it were to take no ring
void test_the_clock_label_is_painted_on_the_center_orb() {
    stale::Overlay o;
    const stale::Paint p = o.step(0, stale::Mark::ClockOnly, 0x1B);
    TEST_ASSERT_EQUAL_HEX8(0x1F, p.orbs);
}

void test_the_data_label_still_goes_on_orb_2() {
    stale::Overlay o;
    const stale::Paint p = o.step(0, stale::Mark::LinkDown, 0x10);
    TEST_ASSERT_EQUAL_HEX8(0x02, p.orbs);
}

void test_a_clock_orb_that_was_repainted_gets_its_ring_back() {
    stale::Overlay o;
    o.step(0, stale::Mark::ClockOnly, 0x1F);
    const stale::Paint p = o.step(0x10, stale::Mark::ClockOnly, 0x1F); // the minute changed on orb 5
    TEST_ASSERT_FALSE(p.repaint);
    TEST_ASSERT_EQUAL_HEX8(0x10, p.orbs);
}

void test_the_clock_ring_comes_off_by_repainting_the_clock() {
    stale::Overlay o;
    o.step(0, stale::Mark::ClockOnly, 0x1F);
    const stale::Paint p = o.step(0, stale::Mark::None, 0);
    TEST_ASSERT_TRUE(p.repaint);
    TEST_ASSERT_EQUAL_HEX8(0, p.orbs);
}

// Weather with "No WiFi" and rings up, then the switch (which wipes all five)
void test_a_switch_from_a_marked_page_to_the_clock_needs_no_extra_repaint() {
    stale::Overlay o;
    o.step(0, stale::Mark::NoWifi, 0x1E);
    const stale::Paint p = o.step(0x1F, stale::Mark::ClockOnly, 0x1F);
    TEST_ASSERT_FALSE(p.repaint);
    TEST_ASSERT_EQUAL_HEX8(0x1F, p.orbs);
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
    RUN_TEST(test_the_fetch_limit_is_the_weather_pages_two_refreshes);
    RUN_TEST(test_a_good_refresh_is_followed_by_the_normal_wait);
    RUN_TEST(test_stocks_try_again_early_after_the_first_miss);
    RUN_TEST(test_stocks_go_back_to_the_normal_wait_after_the_second_miss);
    RUN_TEST(test_a_page_whose_second_refresh_already_lands_on_the_limit_waits_as_usual);
    RUN_TEST(test_stocks_ring_at_twenty_minutes_like_weather);
    RUN_TEST(test_fresh_page_with_wifi_up_gets_no_mark);
    RUN_TEST(test_wifi_out_says_so_before_the_data_is_old);
    RUN_TEST(test_stale_page_without_wifi_gets_the_broken_ring);
    RUN_TEST(test_stale_page_with_wifi_up_gets_the_solid_ring);
    RUN_TEST(test_a_page_that_fetches_nothing_is_never_marked);
    RUN_TEST(test_a_short_drop_is_not_wifi_out);
    RUN_TEST(test_wifi_is_out_after_ten_seconds_down);
    RUN_TEST(test_wifi_is_back_at_once);
    RUN_TEST(test_a_second_drop_gets_its_own_ten_seconds);
    RUN_TEST(test_the_grace_is_right_across_the_millis_wrap);
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
    RUN_TEST(test_wifi_out_puts_only_the_words_on_orb_2);
    RUN_TEST(test_the_ring_joins_the_words_without_a_repaint);
    RUN_TEST(test_the_words_come_off_when_wifi_is_back);
    RUN_TEST(test_rings_that_go_while_wifi_is_still_out_repaint_and_keep_the_words);
    RUN_TEST(test_no_good_refresh_yet_before_any_try);
    RUN_TEST(test_a_failed_first_try_is_not_a_good_refresh);
    RUN_TEST(test_the_last_good_refresh_is_remembered_through_failures);
    RUN_TEST(test_a_good_refresh_after_a_failed_first_try_counts);
    RUN_TEST(test_clock_only_does_nothing_while_the_setting_is_off);
    RUN_TEST(test_clock_only_does_nothing_while_wifi_is_up);
    RUN_TEST(test_wifi_out_switches_to_the_clock);
    RUN_TEST(test_an_outage_that_starts_on_the_clock_stays_there);
    RUN_TEST(test_the_switch_to_the_clock_happens_once);
    RUN_TEST(test_wifi_back_returns_to_the_page_that_was_up);
    RUN_TEST(test_wifi_back_after_an_outage_that_began_on_the_clock_stays);
    RUN_TEST(test_wifi_back_leaves_a_page_picked_by_button_alone);
    RUN_TEST(test_switching_the_setting_off_in_an_outage_returns_too);
    RUN_TEST(test_a_second_outage_remembers_its_own_page);
    RUN_TEST(test_clock_only_needs_a_clock_page);
    RUN_TEST(test_the_clock_gets_its_own_mark_in_clock_only);
    RUN_TEST(test_other_pages_keep_their_own_mark_in_clock_only);
    RUN_TEST(test_the_clock_is_not_marked_outside_clock_only);
    RUN_TEST(test_the_clock_mark_is_a_broken_ring_with_words);
    RUN_TEST(test_the_clock_label_goes_on_the_center_orb);
    RUN_TEST(test_the_label_takes_the_color_of_its_ring);
    RUN_TEST(test_the_shape_says_whether_wifi_is_out);
    RUN_TEST(test_the_clock_ring_is_red_and_data_rings_are_amber);
    RUN_TEST(test_the_clock_ring_goes_on_every_clock_orb_without_a_repaint);
    RUN_TEST(test_the_clock_label_is_painted_on_the_center_orb);
    RUN_TEST(test_the_data_label_still_goes_on_orb_2);
    RUN_TEST(test_a_clock_orb_that_was_repainted_gets_its_ring_back);
    RUN_TEST(test_the_clock_ring_comes_off_by_repainting_the_clock);
    RUN_TEST(test_a_switch_from_a_marked_page_to_the_clock_needs_no_extra_repaint);
    return UNITY_END();
}
