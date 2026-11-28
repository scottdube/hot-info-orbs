// Host tests for QuakePolicy.h (Earthquake Monitor):  pio test -e native -f test_quake
#include "QuakePolicy.h"
#include <string>
#include <unity.h>

void setUp() {}
void tearDown() {}

void test_band_for_magnitude() {
    TEST_ASSERT_TRUE(quake::bandFor(3.9f) == quake::Band::Green);
    TEST_ASSERT_TRUE(quake::bandFor(4.0f) == quake::Band::Yellow);
    TEST_ASSERT_TRUE(quake::bandFor(4.9f) == quake::Band::Yellow);
    TEST_ASSERT_TRUE(quake::bandFor(5.0f) == quake::Band::Orange);
    TEST_ASSERT_TRUE(quake::bandFor(5.9f) == quake::Band::Orange);
    TEST_ASSERT_TRUE(quake::bandFor(6.0f) == quake::Band::Red);
    TEST_ASSERT_TRUE(quake::bandFor(7.4f) == quake::Band::Red);
}

void test_short_place() {
    // The region after the last comma, upper-cased
    TEST_ASSERT_EQUAL_STRING("JAPAN", quake::shortPlace("92km SSW of Example, Japan").c_str());
    TEST_ASSERT_EQUAL_STRING("CHILE", quake::shortPlace("near the coast of Chile", 10).c_str());
    // "... of <region>" without a comma uses the region
    TEST_ASSERT_EQUAL_STRING("JAPAN", quake::shortPlace("Near the coast of Japan").c_str());
    // No comma and no " of ": the whole string, cut to length
    TEST_ASSERT_EQUAL_STRING("CENTRAL C", quake::shortPlace("Central California", 9).c_str());
    // A trailing space after the cut is removed
    TEST_ASSERT_EQUAL_STRING("ONE", quake::shortPlace("one two", 4).c_str());
    TEST_ASSERT_EQUAL_STRING("", quake::shortPlace("   ").c_str());
}

void test_age_text() {
    TEST_ASSERT_EQUAL_STRING("now", quake::ageText(1000, 1000).c_str());
    TEST_ASSERT_EQUAL_STRING("now", quake::ageText(1000, 1030).c_str()); // under a minute
    TEST_ASSERT_EQUAL_STRING("8m", quake::ageText(0, 8 * 60).c_str());
    TEST_ASSERT_EQUAL_STRING("2h", quake::ageText(0, 2 * 3600).c_str());
    TEST_ASSERT_EQUAL_STRING("3d", quake::ageText(0, 3 * 86400).c_str());
    TEST_ASSERT_EQUAL_STRING("now", quake::ageText(2000, 1000).c_str()); // future reads as now
}

void test_mag_text() {
    TEST_ASSERT_EQUAL_STRING("5.4", quake::magText(5.4f).c_str());
    TEST_ASSERT_EQUAL_STRING("6.0", quake::magText(5.99f).c_str()); // rounds to the tenth
    TEST_ASSERT_EQUAL_STRING("4.2", quake::magText(4.15f).c_str());
    TEST_ASSERT_EQUAL_STRING("0.0", quake::magText(-1.0f).c_str()); // clamped
}

void test_feed_url() {
    TEST_ASSERT_TRUE(std::string(quake::feedUrl(4.5f)).find("4.5_day") != std::string::npos);
    TEST_ASSERT_TRUE(std::string(quake::feedUrl(5.0f)).find("4.5_day") != std::string::npos);
    TEST_ASSERT_TRUE(std::string(quake::feedUrl(2.5f)).find("2.5_day") != std::string::npos);
    TEST_ASSERT_TRUE(std::string(quake::feedUrl(1.0f)).find("1.0_day") != std::string::npos);
    TEST_ASSERT_TRUE(std::string(quake::feedUrl(0.0f)).find("all_day") != std::string::npos);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_band_for_magnitude);
    RUN_TEST(test_short_place);
    RUN_TEST(test_age_text);
    RUN_TEST(test_mag_text);
    RUN_TEST(test_feed_url);
    return UNITY_END();
}
