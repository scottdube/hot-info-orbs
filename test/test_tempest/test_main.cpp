// Host tests for TempestParse.h:  pio test -e native
// The fixture is made up (no real station): the repo is public.
#include "TempestParse.h"
#include <fstream>
#include <unity.h>

void setUp() {}
void tearDown() {}

void test_icons_unchanged() {
    const char *same[] = {"clear-day", "clear-night", "cloudy", "partly-cloudy-day", "partly-cloudy-night"};
    for (auto s : same) {
        TEST_ASSERT_EQUAL_STRING(s, tempestIcon(s).c_str());
    }
}

void test_icons_rain() {
    const char *rain[] = {"rainy", "possibly-rainy-day", "possibly-rainy-night", "thunderstorm",
                          "possibly-thunderstorm-day", "possibly-thunderstorm-night"};
    for (auto s : rain) {
        TEST_ASSERT_EQUAL_STRING("rain", tempestIcon(s).c_str());
    }
}

void test_icons_snow_fog_wind_unknown() {
    const char *snow[] = {"snow", "sleet", "possibly-snow-day", "possibly-snow-night", "possibly-sleet-day", "possibly-sleet-night"};
    for (auto s : snow) {
        TEST_ASSERT_EQUAL_STRING("snow", tempestIcon(s).c_str());
    }
    TEST_ASSERT_EQUAL_STRING("fog", tempestIcon("foggy").c_str());
    TEST_ASSERT_EQUAL_STRING("wind", tempestIcon("windy").c_str());
    TEST_ASSERT_EQUAL_STRING("cloudy", tempestIcon("something-new").c_str());
    TEST_ASSERT_EQUAL_STRING("cloudy", tempestIcon("").c_str());
}

void test_parse_fixture_through_filter() {
    std::ifstream in("test/fixtures/tempest_sample.json");
    TEST_ASSERT_TRUE(in.good());
    JsonDocument filter;
    tempestFilter(filter);
    JsonDocument doc;
    TEST_ASSERT_TRUE(deserializeJson(doc, in, DeserializationOption::Filter(filter)) == DeserializationError::Ok);
    TEST_ASSERT_TRUE(doc["forecast"]["hourly"].isNull()); // dropped while streaming
    TempestReading r;
    std::string err;
    TEST_ASSERT_TRUE(tempestParse(doc, r, err));
    TEST_ASSERT_EQUAL_STRING("Clear", r.conditions.c_str());
    TEST_ASSERT_EQUAL_STRING("clear-day", r.icon.c_str());
    TEST_ASSERT_EQUAL_FLOAT(50.0f, r.temp);
    TEST_ASSERT_EQUAL_FLOAT(62.0f, r.days[0].high); // daily[0] is today
    TEST_ASSERT_EQUAL_FLOAT(42.0f, r.days[0].low);
    TEST_ASSERT_EQUAL_STRING("rain", r.days[1].icon.c_str()); // translated
    TEST_ASSERT_EQUAL_STRING("rain", r.days[3].icon.c_str());
    TEST_ASSERT_EQUAL_FLOAT(51.0f, r.days[3].low);
    // What survives the filter is tiny next to the ~80 KB fixture
    TEST_ASSERT_TRUE(measureJson(doc) < 2000);
}

void test_parse_rejects_short_forecast() {
    JsonDocument doc;
    deserializeJson(doc, R"({"current_conditions":{"conditions":"Clear","icon":"clear-day","air_temperature":50},
                          "forecast":{"daily":[{"icon":"clear-day","air_temp_high":1,"air_temp_low":0}]}})");
    TempestReading r;
    std::string err;
    TEST_ASSERT_FALSE(tempestParse(doc, r, err));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, err.find("days"));
}

void test_parse_rejects_missing_current() {
    // What an error reply looks like: no current_conditions at all
    JsonDocument doc;
    deserializeJson(doc, R"({"status":{"status_code":2,"status_message":"UNAUTHORIZED"}})");
    TempestReading r;
    std::string err;
    TEST_ASSERT_FALSE(tempestParse(doc, r, err));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_icons_unchanged);
    RUN_TEST(test_icons_rain);
    RUN_TEST(test_icons_snow_fog_wind_unknown);
    RUN_TEST(test_parse_fixture_through_filter);
    RUN_TEST(test_parse_rejects_short_forecast);
    RUN_TEST(test_parse_rejects_missing_current);
    return UNITY_END();
}
