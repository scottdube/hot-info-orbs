// Host tests for InfoPagePolicy.h:  pio test -e native
#include "InfoPagePolicy.h"
#include <string>
#include <unity.h>
#include <vector>

void setUp() {}
void tearDown() {}

using infopage::Facts;
using infopage::Kind;
using infopage::kTimeoutMs;
using infopage::Page;
using infopage::Press;
using infopage::Session;

// --- opening and closing ------------------------------------------------------
// hold(now): the middle button has been down 2 seconds and still is.
// press(middle, heldLong, now): a button was let go.

void test_the_page_is_closed_until_asked_for() {
    Session s;
    TEST_ASSERT_FALSE(s.isOpen());
    TEST_ASSERT_FALSE(s.timedOut(5 * kTimeoutMs));
    TEST_ASSERT_FALSE(s.isOpen());
}

void test_it_opens_at_two_seconds_with_the_button_still_down() {
    Session s;
    TEST_ASSERT_TRUE(s.hold(1000));
    TEST_ASSERT_TRUE(s.isOpen());
}

void test_letting_go_after_it_opened_does_not_close_it() {
    Session s;
    s.hold(1000);
    TEST_ASSERT_TRUE(s.press(true, true, 1400) == Press::Ignored);
    TEST_ASSERT_TRUE(s.isOpen());
}

void test_holding_on_says_opened_only_once() {
    Session s;
    TEST_ASSERT_TRUE(s.hold(1000));
    TEST_ASSERT_FALSE(s.hold(1010));
    TEST_ASSERT_FALSE(s.hold(5000));
    TEST_ASSERT_TRUE(s.isOpen());
}

void test_any_press_closes_it_and_does_nothing_else() {
    for (bool middle : {false, true}) {
        for (bool heldLong : {false, true}) {
            Session s;
            s.hold(1000);
            s.press(true, true, 1400); // the opening hold let go
            TEST_ASSERT_TRUE(s.press(middle, heldLong, 5000) == Press::Closed);
            TEST_ASSERT_FALSE(s.isOpen());
        }
    }
}

void test_a_second_long_hold_closes_it_on_the_release_not_at_two_seconds() {
    Session s;
    s.hold(1000);
    s.press(true, true, 1400);
    TEST_ASSERT_FALSE(s.hold(9000)); // open already: the hold itself does nothing
    TEST_ASSERT_TRUE(s.isOpen());
    TEST_ASSERT_TRUE(s.press(true, true, 9300) == Press::Closed);
}

void test_another_button_closes_it_while_the_middle_one_is_still_down() {
    Session s;
    s.hold(1000);
    TEST_ASSERT_TRUE(s.press(false, false, 1500) == Press::Closed);
    TEST_ASSERT_FALSE(s.hold(1600)); // the same hold does not open it again
    TEST_ASSERT_TRUE(s.press(true, true, 2000) == Press::Ignored); // and its release does nothing
    TEST_ASSERT_FALSE(s.isOpen());
    TEST_ASSERT_TRUE(s.hold(9000)); // a new hold does
}

void test_a_button_held_past_the_time_limit_leaves_it_closed() {
    Session s;
    s.hold(1000);
    TEST_ASSERT_TRUE(s.timedOut(1000 + kTimeoutMs));
    TEST_ASSERT_FALSE(s.hold(1000 + kTimeoutMs + 10));
    TEST_ASSERT_TRUE(s.press(true, true, 1000 + kTimeoutMs + 500) == Press::Ignored);
    TEST_ASSERT_FALSE(s.isOpen());
}

// A weather fetch can keep the loop busy for seconds. A hold that began and
// ended inside one is only seen as a finished long press: that opens it too.
void test_a_long_hold_the_loop_never_saw_opens_it_on_the_release() {
    Session s;
    TEST_ASSERT_TRUE(s.press(true, true, 1000) == Press::Opened);
    TEST_ASSERT_TRUE(s.isOpen());
    TEST_ASSERT_TRUE(s.press(true, false, 3000) == Press::Closed); // nothing to ignore: it closes
}

void test_any_other_press_goes_on_to_the_page_while_it_is_closed() {
    Session s;
    TEST_ASSERT_TRUE(s.press(false, false, 1000) == Press::PassOn);
    TEST_ASSERT_TRUE(s.press(true, false, 1000) == Press::PassOn);
    TEST_ASSERT_TRUE(s.press(false, true, 1000) == Press::PassOn); // a long hold of a side button
    TEST_ASSERT_FALSE(s.isOpen());
}

void test_it_stays_up_two_minutes() {
    TEST_ASSERT_EQUAL_UINT32(120000, kTimeoutMs);
    Session s;
    s.hold(1000);
    TEST_ASSERT_FALSE(s.timedOut(1000 + kTimeoutMs - 1));
    TEST_ASSERT_TRUE(s.isOpen());
    TEST_ASSERT_TRUE(s.timedOut(1000 + kTimeoutMs));
    TEST_ASSERT_FALSE(s.isOpen());
    TEST_ASSERT_FALSE(s.timedOut(1000 + 2 * kTimeoutMs)); // said once, not on every pass
}

void test_the_time_limit_survives_the_millisecond_counter_wrapping() {
    Session s;
    const uint32_t opened = 0xFFFFFF00u;
    s.hold(opened);
    TEST_ASSERT_FALSE(s.timedOut(opened + kTimeoutMs - 1)); // past the wrap
    TEST_ASSERT_TRUE(s.timedOut(opened + kTimeoutMs));
}

void test_it_can_be_opened_again_with_a_fresh_time_limit() {
    Session s;
    s.hold(1000);
    s.press(true, true, 1400);
    s.press(false, false, 2000); // closed
    TEST_ASSERT_TRUE(s.hold(50000));
    TEST_ASSERT_FALSE(s.timedOut(1000 + kTimeoutMs)); // the first opening's limit is gone
    TEST_ASSERT_TRUE(s.timedOut(50000 + kTimeoutMs));
}

void test_it_is_drawn_at_once_and_then_once_a_second() {
    Session s;
    TEST_ASSERT_FALSE(s.refreshDue(500)); // closed: never
    s.hold(1000);
    TEST_ASSERT_TRUE(s.refreshDue(1000));
    TEST_ASSERT_FALSE(s.refreshDue(1500));
    TEST_ASSERT_TRUE(s.refreshDue(2000));
    TEST_ASSERT_FALSE(s.refreshDue(2100));
    s.close();
    TEST_ASSERT_FALSE(s.refreshDue(9000));
    s.press(true, true, 9100); // the opening hold let go
    s.hold(9500);
    TEST_ASSERT_TRUE(s.refreshDue(9500)); // a new opening draws at once
}

// --- wording ------------------------------------------------------------------

void test_a_time_span_reads_in_its_two_largest_units() {
    TEST_ASSERT_EQUAL_STRING("0 s", infopage::spanText(0).c_str());
    TEST_ASSERT_EQUAL_STRING("59 s", infopage::spanText(59).c_str());
    TEST_ASSERT_EQUAL_STRING("1 min", infopage::spanText(60).c_str());
    TEST_ASSERT_EQUAL_STRING("59 min", infopage::spanText(3599).c_str());
    TEST_ASSERT_EQUAL_STRING("1 h 00 min", infopage::spanText(3600).c_str());
    TEST_ASSERT_EQUAL_STRING("3 h 05 min", infopage::spanText(3 * 3600 + 5 * 60 + 59).c_str());
    TEST_ASSERT_EQUAL_STRING("1 d 00 h", infopage::spanText(86400).c_str());
    TEST_ASSERT_EQUAL_STRING("49 d 17 h", infopage::spanText(49u * 86400 + 17 * 3600 + 120).c_str());
}

void test_signal_strength_is_a_word() {
    TEST_ASSERT_EQUAL_STRING("strong", infopage::signalWord(-40));
    TEST_ASSERT_EQUAL_STRING("strong", infopage::signalWord(-55));
    TEST_ASSERT_EQUAL_STRING("good", infopage::signalWord(-56));
    TEST_ASSERT_EQUAL_STRING("good", infopage::signalWord(-65));
    TEST_ASSERT_EQUAL_STRING("fair", infopage::signalWord(-66)); // below -65 the orb looks for a nearer access point
    TEST_ASSERT_EQUAL_STRING("fair", infopage::signalWord(-75));
    TEST_ASSERT_EQUAL_STRING("weak", infopage::signalWord(-76));
    TEST_ASSERT_EQUAL_STRING("weak", infopage::signalWord(-95));
}

void test_free_memory_reads_in_kilobytes() {
    TEST_ASSERT_EQUAL_STRING("memory 0 KB free", infopage::memoryText(1023).c_str());
    TEST_ASSERT_EQUAL_STRING("memory 182 KB free", infopage::memoryText(182 * 1024 + 500).c_str());
}

void test_every_start_reason_has_a_short_word() {
    using infopage::Start;
    TEST_ASSERT_EQUAL_STRING("power on", infopage::startWord(Start::PowerOn));
    TEST_ASSERT_EQUAL_STRING("reset pin", infopage::startWord(Start::ResetPin));
    TEST_ASSERT_EQUAL_STRING("restart", infopage::startWord(Start::Restart));
    TEST_ASSERT_EQUAL_STRING("crash", infopage::startWord(Start::Crash));
    TEST_ASSERT_EQUAL_STRING("watchdog", infopage::startWord(Start::Watchdog));
    TEST_ASSERT_EQUAL_STRING("deep sleep", infopage::startWord(Start::DeepSleep));
    TEST_ASSERT_EQUAL_STRING("brownout", infopage::startWord(Start::Brownout));
    TEST_ASSERT_EQUAL_STRING("not known", infopage::startWord(Start::Unknown));
}

// --- the five orbs ------------------------------------------------------------

static Facts facts() {
    Facts f;
    f.version = "1.6.1";
    f.commit = "cace049";
    f.built = "2026-11-13 21:05 UTC";
    f.wifiUp = true;
    f.network = "Home";
    f.rssi = -61;
    f.channel = 11;
    f.ip = "10.0.0.50";
    f.localName = "info-orbs.local";
    f.upS = 2u * 86400 + 4 * 3600;
    f.start = infopage::Start::PowerOn;
    f.freeBytes = 182 * 1024;
    f.mqtt[0].inUse = true;
    f.mqtt[0].connected = true;
    f.mqtt[0].hasData = true;
    f.mqtt[0].dataAgeS = 12;
    f.sources.push_back({"weather", true, 3 * 60 + 20});
    f.sources.push_back({"stocks", true, 12 * 60});
    return f;
}

static void expectPage(const Page &p, const std::vector<infopage::Line> &want) {
    TEST_ASSERT_EQUAL_INT(want.size(), p.size());
    for (size_t i = 0; i < want.size(); i++) {
        TEST_ASSERT_EQUAL_STRING(want[i].text.c_str(), p[i].text.c_str());
        TEST_ASSERT_TRUE_MESSAGE(want[i].kind == p[i].kind, want[i].text.c_str());
    }
}

void test_orb_1_is_the_firmware() {
    expectPage(infopage::page(0, facts()),
               {{"Firmware", Kind::Title}, {"1.6.1", Kind::Plain}, {"commit cace049", Kind::Plain}, {"built 2026-11-13", Kind::Plain}});
}

void test_orb_2_is_the_wifi_link() {
    expectPage(infopage::page(1, facts()),
               {{"WiFi", Kind::Title}, {"Home", Kind::Network}, {"good, -61 dBm", Kind::Plain}, {"channel 11", Kind::Plain}});
}

void test_orb_2_says_no_wifi_when_the_link_is_down() {
    Facts f = facts();
    f.wifiUp = false;
    expectPage(infopage::page(1, f), {{"WiFi", Kind::Title}, {"No WiFi", Kind::Plain}});
}

void test_orb_3_is_the_address() {
    expectPage(infopage::page(2, facts()),
               {{"Address", Kind::Title}, {"10.0.0.50", Kind::Address}, {"info-orbs.local", Kind::Address}});
}

void test_orb_3_has_no_address_without_wifi() {
    Facts f = facts();
    f.wifiUp = false;
    expectPage(infopage::page(2, f), {{"Address", Kind::Title}, {"none, no WiFi", Kind::Plain}});
}

void test_orb_4_is_the_health() {
    expectPage(infopage::page(3, facts()),
               {{"Health", Kind::Title}, {"up 2 d 04 h", Kind::Plain}, {"last start: power on", Kind::Plain}, {"memory 182 KB free", Kind::Plain}});
}

// Orb 5: one line for each kind of data page in the rotation, then MQTT.

void test_orb_5_lists_each_kind_of_data_and_when_it_last_refreshed() {
    expectPage(infopage::page(4, facts()), {{"Data", Kind::Title},
                                            {"weather 3 min ago", Kind::Plain},
                                            {"stocks 12 min ago", Kind::Plain},
                                            {"MQTT 12 s ago", Kind::Plain}});
}

void test_orb_5_without_mqtt_still_shows_the_other_data() {
    Facts f = facts();
    f.mqtt[0] = infopage::Mqtt{};
    expectPage(infopage::page(4, f), {{"Data", Kind::Title}, {"weather 3 min ago", Kind::Plain}, {"stocks 12 min ago", Kind::Plain}});
}

void test_orb_5_with_no_data_pages_at_all_says_so() {
    Facts f = facts();
    f.sources.clear();
    f.mqtt[0] = infopage::Mqtt{};
    expectPage(infopage::page(4, f), {{"Data", Kind::Title}, {"no data pages", Kind::Plain}});
}

void test_orb_5_a_page_with_nothing_fetched_yet_says_so() {
    Facts f = facts();
    f.sources[1].hasData = false;
    expectPage(infopage::page(4, f), {{"Data", Kind::Title},
                                      {"weather 3 min ago", Kind::Plain},
                                      {"stocks no data yet", Kind::Plain},
                                      {"MQTT 12 s ago", Kind::Plain}});
}

void test_orb_5_two_pages_of_one_kind_share_a_line_with_the_oldest_age() {
    Facts f = facts();
    f.sources.push_back({"weather", true, 25 * 60}); // a second weather page, older
    f.sources.push_back({"web data", true, 40});
    expectPage(infopage::page(4, f), {{"Data", Kind::Title},
                                      {"weather 25 min ago", Kind::Plain},
                                      {"stocks 12 min ago", Kind::Plain},
                                      {"web data 40 s ago", Kind::Plain},
                                      {"MQTT 12 s ago", Kind::Plain}});
}

void test_orb_5_one_page_of_a_kind_without_data_wins_over_its_age() {
    Facts f = facts();
    f.sources.push_back({"weather", false, 0});
    const Page p = infopage::page(4, f);
    TEST_ASSERT_EQUAL_STRING("weather no data yet", p[1].text.c_str());
}

void test_orb_5_mqtt_connected_with_nothing_received_yet() {
    Facts f = facts();
    f.mqtt[0].hasData = false;
    const Page p = infopage::page(4, f);
    TEST_ASSERT_EQUAL_STRING("MQTT no data yet", p.back().text.c_str());
}

void test_orb_5_mqtt_not_connected_says_so() {
    Facts f = facts();
    f.mqtt[0].connected = false;
    const Page p = infopage::page(4, f);
    TEST_ASSERT_EQUAL_STRING("MQTT not connected", p.back().text.c_str());
}

void test_orb_5_two_brokers_share_a_line_with_the_oldest_data() {
    Facts f = facts();
    f.mqtt[1] = {true, true, true, 90};
    const Page p = infopage::page(4, f);
    TEST_ASSERT_EQUAL_INT(4, p.size());
    TEST_ASSERT_EQUAL_STRING("MQTT 1 min ago", p.back().text.c_str());
}

void test_orb_5_names_the_broker_that_is_not_connected() {
    Facts f = facts();
    f.mqtt[1] = {true, false, false, 0};
    TEST_ASSERT_EQUAL_STRING("MQTT 2 not connected", infopage::page(4, f).back().text.c_str());
    f.mqtt[0].connected = false;
    TEST_ASSERT_EQUAL_STRING("MQTT not connected", infopage::page(4, f).back().text.c_str()); // both
}

void test_orb_5_only_the_second_broker_in_use() {
    Facts f = facts();
    f.mqtt[0] = infopage::Mqtt{};
    f.mqtt[1] = {true, true, true, 7};
    TEST_ASSERT_EQUAL_STRING("MQTT 7 s ago", infopage::page(4, f).back().text.c_str());
}

void test_every_orb_has_a_title_and_at_most_five_lines() {
    Facts f = facts();
    f.mqtt[1].inUse = true;
    for (const char *kind : {"web data", "a fourth kind", "a fifth kind"}) {
        f.sources.push_back({kind, true, 5});
    }
    for (int orb = 0; orb < infopage::kOrbs; orb++) {
        const Page p = infopage::page(orb, f);
        TEST_ASSERT_TRUE(p.size() >= 2 && p.size() <= 5);
        TEST_ASSERT_TRUE(p[0].kind == Kind::Title);
    }
    // MQTT keeps its line when the kinds run over: it is the last one
    TEST_ASSERT_EQUAL_STRING("MQTT 2 not connected", infopage::page(4, f).back().text.c_str());
}

// --- drawing only what changed ------------------------------------------------

void test_nothing_is_redrawn_when_nothing_changed() {
    const Page p = infopage::page(3, facts());
    TEST_ASSERT_EQUAL_INT(0, infopage::linesToRedraw(p, p).size());
}

void test_only_the_changed_line_is_redrawn() {
    Facts f = facts();
    const Page before = infopage::page(3, f);
    f.upS += 3600;
    const std::vector<int> r = infopage::linesToRedraw(before, infopage::page(3, f));
    TEST_ASSERT_EQUAL_INT(1, r.size());
    TEST_ASSERT_EQUAL_INT(1, r[0]);
}

void test_the_whole_orb_is_redrawn_when_the_line_count_changes() {
    Facts f = facts();
    const Page before = infopage::page(1, f);
    f.wifiUp = false;
    const Page after = infopage::page(1, f);
    const std::vector<int> r = infopage::linesToRedraw(before, after);
    TEST_ASSERT_EQUAL_INT(infopage::kWholeOrb, r.size() == 1 ? r[0] : 0);
    const std::vector<int> first = infopage::linesToRedraw({}, after); // nothing drawn yet
    TEST_ASSERT_EQUAL_INT(infopage::kWholeOrb, first.size() == 1 ? first[0] : 0);
}

// --- where the lines go on a round panel --------------------------------------

void test_lines_are_centered_on_the_panel() {
    TEST_ASSERT_EQUAL_INT(120, infopage::lineY(1, 0));
    TEST_ASSERT_EQUAL_INT(240, infopage::lineY(4, 0) + infopage::lineY(4, 3));
    TEST_ASSERT_EQUAL_INT(240, infopage::lineY(5, 0) + infopage::lineY(5, 4));
    TEST_ASSERT_EQUAL_INT(120, infopage::lineY(5, 2));
    TEST_ASSERT_TRUE(infopage::lineY(5, 1) - infopage::lineY(5, 0) == infopage::lineY(5, 4) - infopage::lineY(5, 3));
}

void test_a_line_is_narrower_the_further_it_is_from_the_middle() {
    TEST_ASSERT_TRUE(infopage::lineWidthAt(120) >= 200);
    TEST_ASSERT_TRUE(infopage::lineWidthAt(120) <= 240);
    int last = infopage::lineWidthAt(120);
    for (int y = 121; y <= 240; y++) {
        const int w = infopage::lineWidthAt(y);
        TEST_ASSERT_TRUE(w <= last);
        TEST_ASSERT_TRUE(w >= 0);
        TEST_ASSERT_EQUAL_INT(w, infopage::lineWidthAt(240 - y)); // the same above the middle
        last = w;
    }
    // The outermost of five lines still has room for a short line
    TEST_ASSERT_TRUE(infopage::lineWidthAt(infopage::lineY(5, 0)) >= 140);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_the_page_is_closed_until_asked_for);
    RUN_TEST(test_it_opens_at_two_seconds_with_the_button_still_down);
    RUN_TEST(test_letting_go_after_it_opened_does_not_close_it);
    RUN_TEST(test_holding_on_says_opened_only_once);
    RUN_TEST(test_any_press_closes_it_and_does_nothing_else);
    RUN_TEST(test_a_second_long_hold_closes_it_on_the_release_not_at_two_seconds);
    RUN_TEST(test_another_button_closes_it_while_the_middle_one_is_still_down);
    RUN_TEST(test_a_button_held_past_the_time_limit_leaves_it_closed);
    RUN_TEST(test_a_long_hold_the_loop_never_saw_opens_it_on_the_release);
    RUN_TEST(test_any_other_press_goes_on_to_the_page_while_it_is_closed);
    RUN_TEST(test_it_stays_up_two_minutes);
    RUN_TEST(test_the_time_limit_survives_the_millisecond_counter_wrapping);
    RUN_TEST(test_it_can_be_opened_again_with_a_fresh_time_limit);
    RUN_TEST(test_it_is_drawn_at_once_and_then_once_a_second);
    RUN_TEST(test_a_time_span_reads_in_its_two_largest_units);
    RUN_TEST(test_signal_strength_is_a_word);
    RUN_TEST(test_free_memory_reads_in_kilobytes);
    RUN_TEST(test_every_start_reason_has_a_short_word);
    RUN_TEST(test_orb_1_is_the_firmware);
    RUN_TEST(test_orb_2_is_the_wifi_link);
    RUN_TEST(test_orb_2_says_no_wifi_when_the_link_is_down);
    RUN_TEST(test_orb_3_is_the_address);
    RUN_TEST(test_orb_3_has_no_address_without_wifi);
    RUN_TEST(test_orb_4_is_the_health);
    RUN_TEST(test_orb_5_lists_each_kind_of_data_and_when_it_last_refreshed);
    RUN_TEST(test_orb_5_without_mqtt_still_shows_the_other_data);
    RUN_TEST(test_orb_5_with_no_data_pages_at_all_says_so);
    RUN_TEST(test_orb_5_a_page_with_nothing_fetched_yet_says_so);
    RUN_TEST(test_orb_5_two_pages_of_one_kind_share_a_line_with_the_oldest_age);
    RUN_TEST(test_orb_5_one_page_of_a_kind_without_data_wins_over_its_age);
    RUN_TEST(test_orb_5_mqtt_connected_with_nothing_received_yet);
    RUN_TEST(test_orb_5_mqtt_not_connected_says_so);
    RUN_TEST(test_orb_5_two_brokers_share_a_line_with_the_oldest_data);
    RUN_TEST(test_orb_5_names_the_broker_that_is_not_connected);
    RUN_TEST(test_orb_5_only_the_second_broker_in_use);
    RUN_TEST(test_every_orb_has_a_title_and_at_most_five_lines);
    RUN_TEST(test_nothing_is_redrawn_when_nothing_changed);
    RUN_TEST(test_only_the_changed_line_is_redrawn);
    RUN_TEST(test_the_whole_orb_is_redrawn_when_the_line_count_changes);
    RUN_TEST(test_lines_are_centered_on_the_panel);
    RUN_TEST(test_a_line_is_narrower_the_further_it_is_from_the_middle);
    return UNITY_END();
}
