// Host tests for WakeScript.h:  pio test -e native -f test_booteyes
#include "WakeScript.h"
#include <unity.h>

void setUp() {}
void tearDown() {}

static const uint32_t T0 = 100000; // begin() time; not zero, to catch "since start" bugs

static WakeScript started() {
    WakeScript s;
    s.begin(T0);
    return s;
}

void test_intro_starts_shut_and_looking_down() {
    WakeScript s = started();
    EyePose p = s.at(T0);
    TEST_ASSERT_EQUAL(0, p.open);
    TEST_ASSERT_EQUAL(512, p.gazeX);
    TEST_ASSERT_EQUAL(380, p.gazeY);
    TEST_ASSERT_FALSE(p.done);
}

void test_intro_keyframes() {
    WakeScript s = started();
    TEST_ASSERT_EQUAL(0, s.at(T0 + 1500).open);
    TEST_ASSERT_EQUAL(70, s.at(T0 + 2600).open); // first crack open
    TEST_ASSERT_EQUAL(60, s.at(T0 + 3200).open);
    TEST_ASSERT_EQUAL(0, s.at(T0 + 3500).open);
    TEST_ASSERT_EQUAL(0, s.at(T0 + 3900).open);
    TEST_ASSERT_EQUAL(400, s.at(T0 + 3900).gazeY);
}

void test_loop_keyframes_and_period() {
    WakeScript s = started();
    const uint32_t L = T0 + 3900; // loop start
    TEST_ASSERT_EQUAL(110, s.at(L + 1000).open);
    TEST_ASSERT_EQUAL(90, s.at(L + 1700).open);
    TEST_ASSERT_EQUAL(420, s.at(L + 1700).gazeY);
    TEST_ASSERT_EQUAL(150, s.at(L + 2100).open);
    TEST_ASSERT_EQUAL(40, s.at(L + 2700).open);
    for (uint32_t t = 0; t < 3000; t += 37) {
        EyePose a = s.at(L + t), b = s.at(L + 3000 + t), c = s.at(L + 30000 + t);
        TEST_ASSERT_EQUAL(a.open, b.open);
        TEST_ASSERT_EQUAL(a.open, c.open);
        TEST_ASSERT_EQUAL(a.gazeY, c.gazeY);
    }
}

void test_never_done_without_connect() {
    WakeScript s = started();
    TEST_ASSERT_FALSE(s.at(T0 + 600000).done);
}

void test_early_connect_waits_for_first_crack_open() {
    WakeScript s = started();
    s.connected(T0 + 1000);
    TEST_ASSERT_EQUAL(WakeScript::groggy(2599).open, s.at(T0 + 2599).open); // still groggy
    EyePose p = s.at(T0 + 2600 + 300);                                      // finale t = 300
    TEST_ASSERT_EQUAL(170, p.open);
    TEST_ASSERT_EQUAL(512, p.gazeX);
    TEST_ASSERT_EQUAL(512, p.gazeY);
    TEST_ASSERT_TRUE(s.at(T0 + 2600 + 2000).done);
    TEST_ASSERT_FALSE(s.at(T0 + 2600 + 1999).done);
}

void test_finale_keyframes_and_end() {
    WakeScript s = started();
    const uint32_t C = T0 + 5000;
    s.connected(C);
    TEST_ASSERT_EQUAL(180, s.at(C + 700).gazeX);  // look one way
    TEST_ASSERT_EQUAL(850, s.at(C + 1300).gazeX); // then the other
    TEST_ASSERT_EQUAL(0, s.at(C + 1600).open);    // quick blink
    EyePose end = s.at(C + 2000);
    TEST_ASSERT_TRUE(end.done);
    TEST_ASSERT_EQUAL(256, end.open);
    TEST_ASSERT_EQUAL(512, end.gazeX);
    TEST_ASSERT_EQUAL(512, end.gazeY);
    TEST_ASSERT_TRUE(s.at(C + 60000).done); // stays done
}

void test_handoff_is_continuous() {
    const uint32_t connects[] = {500, 2600, 3000, 4500, 5200, 6800, 9000, 12345};
    for (uint32_t c : connects) {
        WakeScript s = started();
        s.connected(T0 + c);
        const uint32_t f = c > 2600 ? c : 2600; // finale start, ms since begin
        EyePose g = WakeScript::groggy(f), p = s.at(T0 + f);
        TEST_ASSERT_INT_WITHIN(1, g.open, p.open);
        TEST_ASSERT_INT_WITHIN(1, g.gazeX, p.gazeX);
        TEST_ASSERT_INT_WITHIN(1, g.gazeY, p.gazeY);
    }
}

void test_second_connect_is_ignored() {
    WakeScript s = started();
    s.connected(T0 + 5000);
    s.connected(T0 + 9000);
    TEST_ASSERT_TRUE(s.at(T0 + 7000).done);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_intro_starts_shut_and_looking_down);
    RUN_TEST(test_intro_keyframes);
    RUN_TEST(test_loop_keyframes_and_period);
    RUN_TEST(test_never_done_without_connect);
    RUN_TEST(test_early_connect_waits_for_first_crack_open);
    RUN_TEST(test_finale_keyframes_and_end);
    RUN_TEST(test_handoff_is_continuous);
    RUN_TEST(test_second_connect_is_ignored);
    return UNITY_END();
}
