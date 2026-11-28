// Host tests for VillagesParse.h:  pio test -e native
#include "VillagesParse.h"
#include <unity.h>

void setUp() {}
void tearDown() {}

static const int64_t k5pmEst = 1795730400; // 2026-11-26 22:00 UTC

static std::string scan(const char *js) {
    VillagesTokenScanner s;
    for (const char *p = js; *p; p++) {
        if (s.feed(*p)) {
            break;
        }
    }
    return s.token();
}

void test_token_skips_non_assignments() {
    // the first mention is a use, as in the real script
    TEST_ASSERT_EQUAL_STRING("Basic 64f7873ca215d1eb1f72a13f",
                             scan("{Authorization:dp_AUTH_TOKEN,\"Content-Type\":1}dp_dp_AUTH_TOKEN=\"Basic 64f7873ca215d1eb1f72a13f\";let a").c_str());
}

void test_token_spaces_and_single_quotes() {
    TEST_ASSERT_EQUAL_STRING("Basic abc+/=", scan("dp_AUTH_TOKEN  =  'Basic  abc+/='").c_str());
}

void test_token_absent_or_unfinished() {
    TEST_ASSERT_EQUAL_STRING("", scan("dp_AUTH_TOKEN,\"x\" nothing here").c_str());
    TEST_ASSERT_EQUAL_STRING("", scan("dp_AUTH_TOKEN=\"Basic abc").c_str());
}

void test_iso_and_time() {
    TEST_ASSERT_EQUAL_INT64(k5pmEst, villagesParseIso("2026-11-26T22:00:00.000Z"));
    TEST_ASSERT_EQUAL_INT64(0, villagesParseIso("soon"));
    TEST_ASSERT_EQUAL_STRING("5:00 PM", villagesFormatTime(k5pmEst, -5 * 3600, false).c_str());
    TEST_ASSERT_EQUAL_STRING("17:00", villagesFormatTime(k5pmEst, -5 * 3600, true).c_str());
    TEST_ASSERT_EQUAL_STRING("12:30 AM", villagesFormatTime(k5pmEst + 7 * 3600 + 1800, -5 * 3600, false).c_str());
}

static const char *kReply = R"({"events":[
 {"title":"Late Band","location":{"title":"Sawgrass Grove"},"start":{"date":"2026-11-27T00:00:00.000Z"},"end":{"date":"2026-11-27T03:00:00.000Z"}},
 {"title":"Early Band","location":{"title":"Sawgrass Grove"},"start":{"date":"2026-11-26T20:00:00.000Z"},"end":{"date":"2026-11-26T22:00:00.000Z"}},
 {"title":"Earth Beat","location":{"title":"Spanish Springs Town Square"},"start":{"date":"2026-11-26T22:00:00.000Z"},"end":{"date":"2026-11-27T02:00:00.000Z"}},
 {"title":"Fire Rhythms","location":{"title":"Eastport"},"start":{"date":"2026-11-26T22:00:00.000Z"},"end":{"date":"2026-11-27T02:00:00.000Z"}},
 {"title":"Elsewhere","location":{"title":"The Sharon"},"start":{"date":"2026-11-26T22:00:00.000Z"},"end":{"date":"2026-11-27T02:00:00.000Z"}}
]})";

void test_parse_groups_sorts_and_drops_ended() {
    JsonDocument filter;
    villagesFilter(filter);
    JsonDocument doc;
    TEST_ASSERT_TRUE(deserializeJson(doc, kReply, DeserializationOption::Filter(filter)) == DeserializationError::Ok);
    std::vector<VillagesAct> acts[VILLAGES_VENUES];
    // 21:00Z: Early Band is still on, so both Sawgrass acts show, earliest first
    TEST_ASSERT_TRUE(villagesParse(doc, acts, k5pmEst - 3600));
    TEST_ASSERT_EQUAL(1, (int)acts[0].size());
    TEST_ASSERT_EQUAL(0, (int)acts[1].size());
    TEST_ASSERT_EQUAL(2, (int)acts[3].size());
    TEST_ASSERT_EQUAL_STRING("Early Band", acts[3][0].title.c_str());
    TEST_ASSERT_EQUAL(1, (int)acts[4].size());
    // 22:30Z: Early Band is over
    TEST_ASSERT_TRUE(villagesParse(doc, acts, k5pmEst + 1800));
    TEST_ASSERT_EQUAL(1, (int)acts[3].size());
    TEST_ASSERT_EQUAL_STRING("Late Band", acts[3][0].title.c_str());
}

void test_parse_rejects_reply_without_events() {
    JsonDocument doc;
    deserializeJson(doc, "{\"count\":0}");
    std::vector<VillagesAct> acts[VILLAGES_VENUES];
    TEST_ASSERT_FALSE(villagesParse(doc, acts, 0));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_token_skips_non_assignments);
    RUN_TEST(test_token_spaces_and_single_quotes);
    RUN_TEST(test_token_absent_or_unfinished);
    RUN_TEST(test_iso_and_time);
    RUN_TEST(test_parse_groups_sorts_and_drops_ended);
    RUN_TEST(test_parse_rejects_reply_without_events);
    return UNITY_END();
}
