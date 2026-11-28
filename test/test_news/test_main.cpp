#include "../../firmware/src/widgets/newswidget/NewsData.h"
#include "../../firmware/src/widgets/newswidget/NewsText.h"
#include <unity.h>

void setUp() {}
void tearDown() {}

void test_combined_category_query() {
    std::string mixed[5] = {"world", "business", "business", "science", "technology"};
    TEST_ASSERT_EQUAL_STRING("world,business,science,technology", news::categoryQuery(mixed, false).c_str());
    mixed[1] = "us";
    TEST_ASSERT_TRUE(news::categoryQuery(mixed, false).empty());
    TEST_ASSERT_TRUE(news::categoryQuery(mixed, true).empty());
}

void test_parse_match_deduplicate_and_rank() {
    const char *json = R"({"status":"success","results":[
      {"id":"a","title":"A New Discovery","url":"https://example.com/a?utm_source=one",
       "published_at":"2026-11-22T15:00:00Z","category":["science"],"country":["united states of america"],
       "source":{"name":"Example","priority":20}},
      {"id":"b","title":"Different title","url":"https://example.com/a?utm_source=two"},
      {"id":"c","title":"a new discovery!","url":"https://example.com/c"},
      {"id":"d","title":"Another report","category":["world"],"source":{"name":"Other","priority":2}}
    ]})";
    JsonDocument doc;
    TEST_ASSERT_FALSE(deserializeJson(doc, json));
    std::vector<news::Article> articles;
    TEST_ASSERT_TRUE(news::parse(doc.as<JsonVariantConst>(), articles));
    TEST_ASSERT_EQUAL_UINT(2, articles.size());
    TEST_ASSERT_TRUE(news::matches(articles[0], "us"));
    TEST_ASSERT_TRUE(news::matches(articles[0], "science"));
    TEST_ASSERT_FALSE(news::matches(articles[0], "world"));
    news::rank(articles);
    TEST_ASSERT_EQUAL_STRING("d", articles[0].id.c_str());
    TEST_ASSERT_EQUAL_STRING("A New Discovery", articles[1].title.c_str());
    TEST_ASSERT_NOT_EQUAL(0, news::canonicalUrl("https://example.com/?id=1").compare(news::canonicalUrl("https://example.com/?id=2")));
}

void test_failure_does_not_replace_existing_stories() {
    JsonDocument doc;
    deserializeJson(doc, R"({"status":"error","results":[]})");
    std::vector<news::Article> articles(1);
    articles[0].title = "Keep this";
    TEST_ASSERT_FALSE(news::parse(doc.as<JsonVariantConst>(), articles));
    TEST_ASSERT_EQUAL_STRING("Keep this", articles[0].title.c_str());
}

void test_oversized_article_does_not_discard_response() {
    JsonDocument filter;
    filter["status"] = true;
    filter["results"][0]["id"] = true;
    filter["results"][0]["title"] = true;
    filter["results"][0]["category"] = true;
    JsonDocument response;
    response["status"] = "success";
    response["results"][0]["id"] = "latest";
    response["results"][0]["title"] = std::string(511, 'x') + "\xC3\xA9" + "extra";
    response["results"][0]["category"][0] = "world";
    response["results"][1]["id"] = "valid";
    response["results"][1]["title"] = "Valid story";
    response["results"][1]["category"][0] = "world";
    std::string json;
    serializeJson(response, json);
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json, DeserializationOption::Filter(filter));
    TEST_ASSERT_FALSE(error);
    std::vector<news::Article> articles;
    TEST_ASSERT_TRUE(news::parse(doc.as<JsonVariantConst>(), articles));
    TEST_ASSERT_EQUAL_UINT(2, articles.size());
    TEST_ASSERT_EQUAL_STRING("latest", news::latest(articles, "world")->id.c_str());
    TEST_ASSERT_EQUAL_STRING((std::string(511, 'x') + "...").c_str(), articles[0].title.c_str());
    TEST_ASSERT_EQUAL_STRING("valid", articles[1].id.c_str());
}

void test_latest_story_per_category() {
    JsonDocument doc;
    deserializeJson(doc, R"({"status":"success","results":[
      {"id":"world-new","title":"World newest","category":["world"]},
      {"id":"science-new","title":"Science newest","category":["science"],"country":["united states of america"]},
      {"id":"world-old","title":"World older","category":["world"]},
      {"id":"science-old","title":"Science older","category":["science"]}
    ]})");
    std::vector<news::Article> articles;
    TEST_ASSERT_TRUE(news::parse(doc.as<JsonVariantConst>(), articles));
    TEST_ASSERT_EQUAL_STRING("world-new", news::latest(articles, "world")->id.c_str());
    TEST_ASSERT_EQUAL_STRING("science-new", news::latest(articles, "science")->id.c_str());
    TEST_ASSERT_EQUAL_STRING("science-new", news::latest(articles, "us")->id.c_str());
    TEST_ASSERT_NULL(news::latest(articles, "sports"));
    articles.erase(articles.begin());
    TEST_ASSERT_EQUAL_STRING("world-old", news::latest(articles, "world")->id.c_str());
}

void test_display_text_filters_unsupported_characters() {
    auto hasGlyph = [](uint32_t cp) { return cp < 128 || cp == 0xE9; };
    const std::string input =
        "Caf\xC3\xA9 \xE2\x80\x9Cnews\xE2\x80\x9D \xE2\x80\x94 now"
        "\xE2\x80\x8B \xF0\x9F\x98\x80\nnext\x01";
    TEST_ASSERT_EQUAL_STRING("Caf\xC3\xA9 \"news\" - now ? next",
                             news::displayText(input, hasGlyph).c_str());
    TEST_ASSERT_EQUAL_STRING("bad ?( ok",
                             news::displayText("bad \xC3( ok", hasGlyph).c_str());
}

void test_headline_fits_one_screen_and_marks_truncation() {
    auto measure = [](const std::string &text, int size) {
        return TextExtent{static_cast<int>(text.size()) * size / 2, size};
    };
    news::HeadlineLayout shortText = news::fitHeadline("Short headline", measure);
    TEST_ASSERT_EQUAL_INT(news::kHeadlineMaxSize, shortText.size);
    TEST_ASSERT_TRUE(mqttFits("Short headline", shortText.size, news::kHeadlineWidth,
                              news::kHeadlineBottom - news::kHeadlineTop, measure));

    const std::string medium = "A headline with enough words to require a smaller font size on the display";
    news::HeadlineLayout fitted = news::fitHeadline(medium, measure);
    TEST_ASSERT_LESS_THAN_INT(news::kHeadlineMaxSize, fitted.size);
    TEST_ASSERT_GREATER_OR_EQUAL_INT(news::kHeadlineMinSize, fitted.size);
    TEST_ASSERT_TRUE(mqttFits(medium, fitted.size, news::kHeadlineWidth,
                              news::kHeadlineBottom - news::kHeadlineTop, measure));

    for (const std::string &text : {std::string(200, 'a'),
                                    std::string("A long headline ") + std::string(180, 'x')}) {
        news::HeadlineLayout cut = news::fitHeadline(text, measure);
        TEST_ASSERT_EQUAL_INT(news::kHeadlineMinSize, cut.size);
        TEST_ASSERT_LESS_OR_EQUAL_UINT(3, cut.lines.size());
        TEST_ASSERT_EQUAL_STRING("...", cut.lines.back().substr(cut.lines.back().size() - 3).c_str());
        for (const auto &line : cut.lines)
            TEST_ASSERT_LESS_OR_EQUAL_INT(news::kHeadlineWidth, measure(line, cut.size).w);
    }
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_combined_category_query);
    RUN_TEST(test_parse_match_deduplicate_and_rank);
    RUN_TEST(test_failure_does_not_replace_existing_stories);
    RUN_TEST(test_oversized_article_does_not_discard_response);
    RUN_TEST(test_latest_story_per_category);
    RUN_TEST(test_display_text_filters_unsupported_characters);
    RUN_TEST(test_headline_fits_one_screen_and_marks_truncation);
    return UNITY_END();
}
