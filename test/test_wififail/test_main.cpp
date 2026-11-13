// Host tests for WifiFailPolicy.h:  pio test -e native -f test_wififail
#include "WifiFailPolicy.h"
#include <unity.h>

void setUp() {}
void tearDown() {}

using namespace wififail;

void test_no_saved_network_is_first_setup() {
    // whatever the radio said, nothing saved means nothing was tried
    TEST_ASSERT_TRUE(Case::NeverSetUp == classify(false, wl::NoSsidAvail));
    TEST_ASSERT_TRUE(Case::NeverSetUp == classify(false, wl::Disconnected));
}

void test_saved_network_not_heard() {
    TEST_ASSERT_TRUE(Case::NotFound == classify(true, wl::NoSsidAvail));
}

void test_wrong_password() {
    // WiFiManager's own code 7, from the AUTH_FAIL / AUTH_EXPIRE disconnect reason
    TEST_ASSERT_TRUE(Case::WrongPassword == classify(true, wl::WrongPassword));
}

void test_anything_else_is_other() {
    TEST_ASSERT_TRUE(Case::Other == classify(true, wl::ConnectFailed));
    TEST_ASSERT_TRUE(Case::Other == classify(true, wl::ConnectionLost));
    TEST_ASSERT_TRUE(Case::Other == classify(true, wl::Disconnected));
    TEST_ASSERT_TRUE(Case::Other == classify(true, 0));
}

void test_password_refusal_reasons() {
    using namespace wififail::reason;
    TEST_ASSERT_TRUE(wififail::passwordRefused(FourWayHandshakeTimeout)); // WPA2 wrong password
    TEST_ASSERT_TRUE(wififail::passwordRefused(AuthFail));
    TEST_ASSERT_TRUE(wififail::passwordRefused(AuthExpire));
    TEST_ASSERT_TRUE(wififail::passwordRefused(HandshakeTimeout));
    TEST_ASSERT_FALSE(wififail::passwordRefused(NoApFound));
    TEST_ASSERT_FALSE(wififail::passwordRefused(AssocLeave)); // our own disconnect
}

void test_only_not_found_keeps_looking() {
    // a wrong password or an unknown fault would restart the orb forever
    TEST_ASSERT_TRUE(keepsLooking(Case::NotFound));
    TEST_ASSERT_FALSE(keepsLooking(Case::NeverSetUp));
    TEST_ASSERT_FALSE(keepsLooking(Case::WrongPassword));
    TEST_ASSERT_FALSE(keepsLooking(Case::Other));
}

static bool has(const std::vector<Line> &lines, const std::string &text, Kind kind) {
    for (const Line &l : lines) {
        if (l.text == text && l.kind == kind) {
            return true;
        }
    }
    return false;
}

void test_first_setup_screens_unchanged() {
    const Screens s = screens(Case::NeverSetUp, "home", "Info-Orbs_AB");
    TEST_ASSERT_FALSE(s.cross);
    TEST_ASSERT_TRUE(has(s.steps, "Connect", Kind::Plain));
    TEST_ASSERT_TRUE(has(s.steps, "Info-Orbs_AB", Kind::Network));
    TEST_ASSERT_TRUE(has(s.steps, "192.168.4.1", Kind::Address));
    TEST_ASSERT_FALSE(has(s.headline, "home", Kind::Network)); // nothing saved to name
}

void test_failure_screens_name_the_saved_network() {
    for (Case c : {Case::NotFound, Case::WrongPassword, Case::Other}) {
        const Screens s = screens(c, "home", "Info-Orbs_AB");
        TEST_ASSERT_TRUE(s.cross); // the X glyph: never color alone
        TEST_ASSERT_TRUE(has(s.headline, "home", Kind::Network));
        TEST_ASSERT_TRUE(has(s.steps, "Info-Orbs_AB", Kind::Network));
        TEST_ASSERT_TRUE(has(s.steps, "192.168.4.1", Kind::Address));
        TEST_ASSERT_TRUE(s.steps.size() <= 5); // five lines fit a round orb
        TEST_ASSERT_TRUE(s.headline.size() <= 3);
    }
}

void test_headlines_say_what_went_wrong() {
    TEST_ASSERT_TRUE(has(screens(Case::NotFound, "h", "a").headline, "not found", Kind::Plain));
    TEST_ASSERT_TRUE(has(screens(Case::WrongPassword, "h", "a").headline, "Wrong password", Kind::Plain));
    TEST_ASSERT_TRUE(has(screens(Case::NotFound, "h", "a").steps, "To change, join", Kind::Plain));
    TEST_ASSERT_FALSE(has(screens(Case::NotFound, "h", "a").steps, "Will retry.", Kind::Plain));
}

// ---- Setup screen, network heard but the join failed: retry by restarting ----

void test_only_a_heard_network_that_did_not_join_is_retried_by_restart() {
    TEST_ASSERT_TRUE(wififail::retriesByRestart(wififail::Case::Other));
    TEST_ASSERT_FALSE(wififail::retriesByRestart(wififail::Case::NotFound)); // keepsLooking() scans for that one
    TEST_ASSERT_FALSE(wififail::retriesByRestart(wififail::Case::WrongPassword)); // the same password gets the same answer
    TEST_ASSERT_FALSE(wififail::retriesByRestart(wififail::Case::NeverSetUp));
}

void test_retries_wait_5_10_20_then_30_minutes() {
    const uint32_t minute = 60000;
    TEST_ASSERT_EQUAL_UINT32(5 * minute, wififail::retryAfterMs(0));
    TEST_ASSERT_EQUAL_UINT32(10 * minute, wififail::retryAfterMs(1));
    TEST_ASSERT_EQUAL_UINT32(20 * minute, wififail::retryAfterMs(2));
    TEST_ASSERT_EQUAL_UINT32(30 * minute, wififail::retryAfterMs(3));
    TEST_ASSERT_EQUAL_UINT32(30 * minute, wififail::retryAfterMs(4));
    TEST_ASSERT_EQUAL_UINT32(30 * minute, wififail::retryAfterMs(100000)); // no end (Scott, 2026-11-13)
}

void test_the_retry_count_survives_a_restart_but_not_a_power_cycle() {
    // The count lives in memory a restart keeps and a power cycle scrambles
    wififail::RetryMemory fresh{0xDEADBEEF, 0x12345678}; // whatever was there at power-on
    TEST_ASSERT_EQUAL_UINT32(0, wififail::retriesDone(fresh));
    wififail::noteRetry(fresh);
    TEST_ASSERT_EQUAL_UINT32(1, wififail::retriesDone(fresh));
    wififail::noteRetry(fresh);
    wififail::noteRetry(fresh);
    TEST_ASSERT_EQUAL_UINT32(3, wififail::retriesDone(fresh));
    wififail::clearRetries(fresh); // joined
    TEST_ASSERT_EQUAL_UINT32(0, wififail::retriesDone(fresh));
}

void test_a_scrambled_count_reads_as_none() {
    wififail::RetryMemory m{0, 0};
    wififail::noteRetry(m);
    m.check ^= 1; // one bit off
    TEST_ASSERT_EQUAL_UINT32(0, wififail::retriesDone(m));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_no_saved_network_is_first_setup);
    RUN_TEST(test_saved_network_not_heard);
    RUN_TEST(test_wrong_password);
    RUN_TEST(test_anything_else_is_other);
    RUN_TEST(test_password_refusal_reasons);
    RUN_TEST(test_only_not_found_keeps_looking);
    RUN_TEST(test_first_setup_screens_unchanged);
    RUN_TEST(test_failure_screens_name_the_saved_network);
    RUN_TEST(test_headlines_say_what_went_wrong);
    RUN_TEST(test_only_a_heard_network_that_did_not_join_is_retried_by_restart);
    RUN_TEST(test_retries_wait_5_10_20_then_30_minutes);
    RUN_TEST(test_the_retry_count_survives_a_restart_but_not_a_power_cycle);
    RUN_TEST(test_a_scrambled_count_reads_as_none);
    return UNITY_END();
}
