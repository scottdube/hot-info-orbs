// Host tests for RejoinPolicy.h:  pio test -e native
#include "DeviceLog.h"
#include "RejoinPolicy.h"
#include <string>
#include <unity.h>
#include <vector>

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

// ---- The ladder: what to do after each failed join (rejoin::Ladder) ----

using rejoin::Action;
using rejoin::kHeardResetMs;
using rejoin::kHeardRestartMs;
using rejoin::kUnheardResetMs;

void test_a_reason_other_than_no_access_point_means_the_network_was_heard() {
    TEST_ASSERT_FALSE(rejoin::heard(201)); // NO_AP_FOUND: the scan before the join found nothing
    TEST_ASSERT_TRUE(rejoin::heard(2)); // AUTH_EXPIRE
    TEST_ASSERT_TRUE(rejoin::heard(8)); // our own leave, and no failure since: the join is in progress
    TEST_ASSERT_TRUE(rejoin::heard(15)); // 4WAY_HANDSHAKE_TIMEOUT
    TEST_ASSERT_TRUE(rejoin::heard(202)); // AUTH_FAIL
}

// One failed try every kEveryMs from `from`, all with the same answer; the actions asked for
static std::vector<Action> tries(rejoin::Ladder &l, uint32_t from, uint32_t forMs, bool heardIt) {
    std::vector<Action> out;
    for (uint32_t t = from; t - from <= forMs; t += kEveryMs) {
        out.push_back(l.afterTry(t, heardIt));
    }
    return out;
}

static int count(const std::vector<Action> &a, Action what) {
    int n = 0;
    for (Action x : a) {
        n += x == what ? 1 : 0;
    }
    return n;
}

void test_an_outage_never_restarts_the_orb() {
    // The network is off the air: the clock and "No WiFi" stay up (Scott, 2026-11-13)
    rejoin::Ladder l;
    const std::vector<Action> a = tries(l, 30000, 6UL * 3600 * 1000, false); // six hours
    TEST_ASSERT_EQUAL(0, count(a, Action::Restart));
}

void test_an_outage_resets_the_radio_every_ten_minutes() {
    // A radio gone deaf never "hears" the network; off-and-on costs nothing on screen
    rejoin::Ladder l;
    TEST_ASSERT_EQUAL(0, count(tries(l, 30000, kUnheardResetMs - kEveryMs, false), Action::ResetRadio));
    TEST_ASSERT_EQUAL(Action::ResetRadio, l.afterTry(30000 + kUnheardResetMs, false));
    TEST_ASSERT_EQUAL(Action::None, l.afterTry(30000 + kUnheardResetMs + kEveryMs, false));
    rejoin::Ladder hour;
    TEST_ASSERT_EQUAL(6, count(tries(hour, 30000, 3600000, false), Action::ResetRadio));
}

void test_heard_but_not_joining_resets_the_radio_half_way() {
    rejoin::Ladder l;
    const uint32_t first = 30000;
    TEST_ASSERT_EQUAL(Action::None, l.afterTry(first, true));
    TEST_ASSERT_EQUAL(0, count(tries(l, first + kEveryMs, kHeardResetMs - 2 * kEveryMs, true), Action::ResetRadio));
    TEST_ASSERT_EQUAL(Action::ResetRadio, l.afterTry(first + kHeardResetMs, true));
    TEST_ASSERT_EQUAL(Action::None, l.afterTry(first + kHeardResetMs + kEveryMs, true)); // once, not on every try
}

void test_heard_but_not_joining_restarts_after_five_minutes() {
    rejoin::Ladder l;
    const uint32_t first = 30000;
    const std::vector<Action> before = tries(l, first, kHeardRestartMs - kEveryMs, true);
    TEST_ASSERT_EQUAL(0, count(before, Action::Restart));
    TEST_ASSERT_EQUAL(1, count(before, Action::ResetRadio));
    TEST_ASSERT_EQUAL(Action::Restart, l.afterTry(first + kHeardRestartMs, true));
}

void test_the_five_minutes_count_from_when_the_network_is_first_heard() {
    // Two hours off the air, then on the air and refusing: not an instant restart
    rejoin::Ladder l;
    const uint32_t outage = 2UL * 3600 * 1000;
    tries(l, 30000, outage, false);
    const uint32_t first = 30000 + outage + kEveryMs;
    TEST_ASSERT_EQUAL(Action::None, l.afterTry(first, true));
    TEST_ASSERT_EQUAL(0, count(tries(l, first + kEveryMs, kHeardRestartMs - 2 * kEveryMs, true), Action::Restart));
    TEST_ASSERT_EQUAL(Action::Restart, l.afterTry(first + kHeardRestartMs, true));
}

void test_one_missed_scan_does_not_start_the_five_minutes_over() {
    // A scan can miss an access point that is there
    rejoin::Ladder l;
    const uint32_t first = 30000;
    tries(l, first, 4 * kEveryMs, true); // heard for 2 minutes
    TEST_ASSERT_EQUAL(Action::None, l.afterTry(first + 5 * kEveryMs, false)); // one miss; 2.5 min mark passes unheard
    const std::vector<Action> rest = tries(l, first + 6 * kEveryMs, kHeardRestartMs - 6 * kEveryMs, true);
    TEST_ASSERT_EQUAL(Action::Restart, rest.back());
    TEST_ASSERT_EQUAL(1, count(rest, Action::Restart));
}

void test_two_misses_in_a_row_start_the_five_minutes_over() {
    rejoin::Ladder l;
    const uint32_t first = 30000;
    tries(l, first, 8 * kEveryMs, true); // heard for 4 minutes
    l.afterTry(first + 9 * kEveryMs, false);
    l.afterTry(first + 10 * kEveryMs, false); // off the air after all
    const uint32_t again = first + 11 * kEveryMs;
    const std::vector<Action> a = tries(l, again, kHeardRestartMs - kEveryMs, true);
    TEST_ASSERT_EQUAL(0, count(a, Action::Restart));
    TEST_ASSERT_EQUAL(Action::Restart, l.afterTry(again + kHeardRestartMs, true));
}

void test_the_link_coming_back_clears_the_ladder() {
    rejoin::Ladder l;
    const uint32_t first = 30000;
    tries(l, first, kHeardRestartMs - kEveryMs, true); // one try short of a restart
    l.linkUp();
    const uint32_t later = first + kHeardRestartMs + 3600000;
    TEST_ASSERT_EQUAL(Action::None, l.afterTry(later, true));
    TEST_ASSERT_EQUAL(0, count(tries(l, later + kEveryMs, kHeardRestartMs - 2 * kEveryMs, true), Action::Restart));
}

void test_the_ladder_works_across_the_millis_wrap() {
    rejoin::Ladder l;
    const uint32_t first = 0xFFFFFFFFu - 60000; // a minute before millis() wraps
    const std::vector<Action> a = tries(l, first, kHeardRestartMs - kEveryMs, true);
    TEST_ASSERT_EQUAL(0, count(a, Action::Restart));
    TEST_ASSERT_EQUAL(Action::Restart, l.afterTry(first + kHeardRestartMs, true));
}

void test_the_ladder_says_whether_it_counts_the_network_as_heard() {
    // The device log gets a line when this changes: without USB the tries are not visible
    rejoin::Ladder l;
    TEST_ASSERT_FALSE(l.hearing());
    l.afterTry(30000, true);
    TEST_ASSERT_TRUE(l.hearing());
    l.afterTry(60000, false); // one miss: still counted as heard
    TEST_ASSERT_TRUE(l.hearing());
    l.afterTry(90000, false); // two in a row
    TEST_ASSERT_FALSE(l.hearing());
    l.afterTry(120000, true);
    l.linkUp();
    TEST_ASSERT_FALSE(l.hearing());
}

void test_the_still_down_line_reads_in_plain_words() {
    TEST_ASSERT_EQUAL_STRING("wifi down 300 s, reason 201: state 1, 118 drops, 9 tries", rejoin::stillDownLine(300, 1, 118, 9, 201).c_str());
}

void test_the_still_down_line_keeps_its_reason_with_the_largest_numbers() {
    // Seen 2026-11-13 on the rig: the longer wording lost "last reason N" to the log's cut at 300 s
    const std::string line = rejoin::stillDownLine(0xFFFFFFFFu / 1000, 255, 0xFFFFFFFFu, 0xFFFFFFFFu, 255);
    TEST_ASSERT_TRUE(line.size() <= devlog::kMsgMax); // nothing is cut
    TEST_ASSERT_TRUE(line.find(", reason 255:") != std::string::npos);
    TEST_ASSERT_EQUAL_STRING("4294967295 tries", line.substr(line.size() - 16).c_str());
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
    RUN_TEST(test_a_reason_other_than_no_access_point_means_the_network_was_heard);
    RUN_TEST(test_an_outage_never_restarts_the_orb);
    RUN_TEST(test_an_outage_resets_the_radio_every_ten_minutes);
    RUN_TEST(test_heard_but_not_joining_resets_the_radio_half_way);
    RUN_TEST(test_heard_but_not_joining_restarts_after_five_minutes);
    RUN_TEST(test_the_five_minutes_count_from_when_the_network_is_first_heard);
    RUN_TEST(test_one_missed_scan_does_not_start_the_five_minutes_over);
    RUN_TEST(test_two_misses_in_a_row_start_the_five_minutes_over);
    RUN_TEST(test_the_link_coming_back_clears_the_ladder);
    RUN_TEST(test_the_ladder_works_across_the_millis_wrap);
    RUN_TEST(test_the_ladder_says_whether_it_counts_the_network_as_heard);
    RUN_TEST(test_the_still_down_line_reads_in_plain_words);
    RUN_TEST(test_the_still_down_line_keeps_its_reason_with_the_largest_numbers);
    return UNITY_END();
}
