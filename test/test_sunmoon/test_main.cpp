// Host tests for SunMoon.h:  pio test -e native
#include "SunMoon.h"
#include <unity.h>

void setUp() {}
void tearDown() {}

void test_moon_known_new_and_full() {
    // 2024-04-08 18:21 UTC new moon (the eclipse); 2024-04-23 23:49 UTC full
    double atNew = moonAgeDays(1712600460);
    TEST_ASSERT_TRUE(atNew < 1.0 || atNew > 28.53);
    TEST_ASSERT_EQUAL_STRING("New moon", moonPhaseName(atNew));
    TEST_ASSERT_FLOAT_WITHIN(1.0, 14.77, moonAgeDays(1713916140));
    TEST_ASSERT_EQUAL_STRING("Full moon", moonPhaseName(moonAgeDays(1713916140)));
}

void test_moon_matches_visual_crossing() {
    // Visual Crossing gave moonphase 0.43 for 2026-09-24 (measured that day)
    double frac = moonAgeDays(1790265600) / 29.530588853; // 2026-09-24 14:00 UTC
    TEST_ASSERT_FLOAT_WITHIN(0.04, 0.43, frac);
    TEST_ASSERT_EQUAL_STRING("Waxing gibbous", moonPhaseName(moonAgeDays(1790265600)));
}

void test_moon_names_cover_the_cycle() {
    TEST_ASSERT_EQUAL_STRING("New moon", moonPhaseName(0.0));
    TEST_ASSERT_EQUAL_STRING("Waxing crescent", moonPhaseName(3.7));
    TEST_ASSERT_EQUAL_STRING("First quarter", moonPhaseName(7.4));
    TEST_ASSERT_EQUAL_STRING("Waning crescent", moonPhaseName(25.8));
    TEST_ASSERT_EQUAL_STRING("New moon", moonPhaseName(29.4)); // wraps
}

void test_sun_clock() {
    // 1700000000 = 2023-11-14 22:13:20 UTC
    TEST_ASSERT_EQUAL_STRING("22:13", sunClock(1700000000, 0, true).c_str());
    TEST_ASSERT_EQUAL_STRING("10:13", sunClock(1700000000, 0, false).c_str());
    TEST_ASSERT_EQUAL_STRING("17:13", sunClock(1700000000, -5 * 3600, true).c_str()); // EST
    TEST_ASSERT_EQUAL_STRING("12:13", sunClock(1700000000 + 2 * 3600, 0, false).c_str()); // noon hour
    TEST_ASSERT_EQUAL_STRING("12:13", sunClock(1700000000 + 2 * 3600 + 12 * 3600, 0, false).c_str()); // midnight hour
    TEST_ASSERT_EQUAL_STRING("", sunClock(0, 0, true).c_str());
}

void test_moon_lit_span() {
    double a, b;
    moonLitSpan(0.0, 10, a, b); // new: nothing lit
    TEST_ASSERT_TRUE(b - a < 0.01);
    moonLitSpan(29.530588853 / 4, 10, a, b); // first quarter: right half
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0, a);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 10, b);
    moonLitSpan(29.530588853 / 2, 10, a, b); // full: whole row
    TEST_ASSERT_FLOAT_WITHIN(0.01, -10, a);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 10, b);
    moonLitSpan(29.530588853 * 3 / 4, 10, a, b); // last quarter: left half
    TEST_ASSERT_FLOAT_WITHIN(0.01, -10, a);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0, b);
    moonLitSpan(29.530588853 * 0.43, 10, a, b); // today, waxing gibbous: past half, lit on the right
    TEST_ASSERT_TRUE(a < -5 && b == 10);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_moon_known_new_and_full);
    RUN_TEST(test_moon_matches_visual_crossing);
    RUN_TEST(test_moon_names_cover_the_cycle);
    RUN_TEST(test_sun_clock);
    RUN_TEST(test_moon_lit_span);
    return UNITY_END();
}
