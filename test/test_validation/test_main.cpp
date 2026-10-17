// Host tests for SettingsValidation.h:  pio test -e native
#include "SettingsValidation.h"
#include <cstdio>
#include <unity.h>
#include <vector>

using namespace sv;

void setUp() {}
void tearDown() {}

static std::vector<uint8_t> readFile(const char *path) {
    std::vector<uint8_t> data;
    FILE *f = fopen(path, "rb");
    if (!f) {
        return data;
    }
    int c;
    while ((c = fgetc(f)) != EOF) {
        data.push_back((uint8_t)c);
    }
    fclose(f);
    return data;
}

// --- cycle -------------------------------------------------------------------

void test_cycle() {
    int out = -1;
    std::string err;
    TEST_ASSERT_TRUE(parseCycle("0", out, err));
    TEST_ASSERT_EQUAL(0, out);
    TEST_ASSERT_TRUE(parseCycle("15", out, err));
    TEST_ASSERT_EQUAL(15, out);
    TEST_ASSERT_TRUE(parseCycle(" 30 ", out, err));
    TEST_ASSERT_EQUAL(30, out);
    TEST_ASSERT_FALSE(parseCycle("4", out, err));
    TEST_ASSERT_FALSE(parseCycle("3601", out, err));
    TEST_ASSERT_FALSE(parseCycle("abc", out, err));
    TEST_ASSERT_FALSE(parseCycle("", out, err));
    TEST_ASSERT_FALSE(err.empty());
}

// --- hour --------------------------------------------------------------------

void test_hour() {
    int out = -1;
    std::string err;
    TEST_ASSERT_TRUE(parseHour("0", out, err));
    TEST_ASSERT_EQUAL(0, out);
    TEST_ASSERT_TRUE(parseHour("23", out, err));
    TEST_ASSERT_EQUAL(23, out);
    TEST_ASSERT_FALSE(parseHour("24", out, err));
    TEST_ASSERT_FALSE(parseHour("-1", out, err));
    TEST_ASSERT_FALSE(parseHour("", out, err));
}

void test_hour_range() {
    TEST_ASSERT_TRUE(inHourRange(18, 18, 22));
    TEST_ASSERT_TRUE(inHourRange(21, 18, 22));
    TEST_ASSERT_FALSE(inHourRange(22, 18, 22)); // end hour is excluded
    TEST_ASSERT_FALSE(inHourRange(17, 18, 22));
    TEST_ASSERT_TRUE(inHourRange(23, 23, 6)); // wraps midnight
    TEST_ASSERT_TRUE(inHourRange(0, 23, 6));
    TEST_ASSERT_TRUE(inHourRange(5, 23, 6));
    TEST_ASSERT_FALSE(inHourRange(6, 23, 6));
    TEST_ASSERT_FALSE(inHourRange(12, 23, 6));
    TEST_ASSERT_FALSE(inHourRange(7, 7, 7)); // same hour = empty
}

void test_list_contains() {
    TEST_ASSERT_TRUE(listContains("Clock", "Clock"));
    TEST_ASSERT_TRUE(listContains("Clock\nWeather Home", "Weather Home"));
    TEST_ASSERT_FALSE(listContains("Clock\nWeather Home", "Weather"));
    TEST_ASSERT_FALSE(listContains("WebData 2", "WebData"));
    TEST_ASSERT_FALSE(listContains("", "Clock"));
    TEST_ASSERT_FALSE(listContains("Clock\n", ""));
}

// --- tickers -----------------------------------------------------------------

void test_tickers() {
    std::string out, err;
    TEST_ASSERT_TRUE(normalizeTickers("spy, qqq ,AAPL", out, err));
    TEST_ASSERT_EQUAL_STRING("SPY,QQQ,AAPL", out.c_str());
    TEST_ASSERT_TRUE(normalizeTickers("BTC/USD", out, err));
    TEST_ASSERT_EQUAL_STRING("BTC/USD", out.c_str());
    TEST_ASSERT_TRUE(normalizeTickers("shop&country=Canada", out, err));
    TEST_ASSERT_EQUAL_STRING("SHOP&country=Canada", out.c_str());
    TEST_ASSERT_FALSE(normalizeTickers("A,B,C,D,E,F", out, err));
    TEST_ASSERT_FALSE(normalizeTickers("", out, err));
    TEST_ASSERT_FALSE(normalizeTickers(",,", out, err));
    TEST_ASSERT_FALSE(normalizeTickers("SP Y", out, err));
}

// --- text --------------------------------------------------------------------

void test_text() {
    std::string out, err;
    TEST_ASSERT_TRUE(normalizeText("  The   Villages,  FL ", 64, out, err));
    TEST_ASSERT_EQUAL_STRING("The Villages, FL", out.c_str());
    TEST_ASSERT_FALSE(normalizeText(std::string(65, 'a'), 64, out, err));
    TEST_ASSERT_TRUE(normalizeText(std::string(64, 'a'), 64, out, err));
    TEST_ASSERT_FALSE(normalizeText("a\tb", 64, out, err));
    TEST_ASSERT_FALSE(normalizeText("   ", 64, out, err));
}

// --- urlEncode ---------------------------------------------------------------

void test_url_encode() {
    TEST_ASSERT_EQUAL_STRING("The%20Villages%2C%20FL", urlEncode("The Villages, FL").c_str());
    TEST_ASSERT_EQUAL_STRING("America%2FNew_York", urlEncode("America/New_York").c_str());
    TEST_ASSERT_EQUAL_STRING("A-z_0.~", urlEncode("A-z_0.~").c_str());
}

// --- colors -----------------------------------------------------------------

void test_color() {
    uint16_t c = 0;
    std::string err;
    TEST_ASSERT_TRUE(parseHexColor("#fc8000", c, err));
    TEST_ASSERT_EQUAL_HEX16(0xFC00, c);
    TEST_ASSERT_TRUE(parseHexColor("#FFFFFF", c, err));
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, c);
    TEST_ASSERT_FALSE(parseHexColor("fc8000", c, err));
    TEST_ASSERT_FALSE(parseHexColor("#zzzzzz", c, err));
    TEST_ASSERT_EQUAL_STRING("#ffffff", rgb565ToHex(0xFFFF).c_str());
    TEST_ASSERT_EQUAL_STRING("#000000", rgb565ToHex(0x0000).c_str());
    // Round trip is stable once a color has been through RGB565
    TEST_ASSERT_TRUE(parseHexColor(rgb565ToHex(0xfc80), c, err));
    TEST_ASSERT_EQUAL_HEX16(0xfc80, c);
}

// --- htmlEscape --------------------------------------------------------------

void test_html_escape() {
    TEST_ASSERT_EQUAL_STRING("&lt;a href=&quot;x&quot;&gt;&amp;&#39;", htmlEscape("<a href=\"x\">&'").c_str());
}

// --- JPEG header -------------------------------------------------------------

void test_jpeg_baseline_logo_passes() {
    std::vector<uint8_t> d = readFile("images/logo.jpg");
    TEST_ASSERT_TRUE_MESSAGE(d.size() > 0, "images/logo.jpg not found - run from the project root");
    std::string err;
    TEST_ASSERT_TRUE_MESSAGE(checkJpegHeader(d.data(), d.size(), err), err.c_str());
}

void test_jpeg_progressive_rejected() {
    std::vector<uint8_t> d = readFile("test/fixtures/progressive.jpg");
    TEST_ASSERT_TRUE(d.size() > 0);
    std::string err;
    TEST_ASSERT_FALSE(checkJpegHeader(d.data(), d.size(), err));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, err.find("progressive"));
}

void test_jpeg_garbage_rejected() {
    std::string err;
    const uint8_t noSof[] = {0xFF, 0xD8, 0xFF, 0xD9};
    TEST_ASSERT_FALSE(checkJpegHeader(noSof, sizeof(noSof), err));
    const uint8_t png[] = {0x89, 'P', 'N', 'G'};
    TEST_ASSERT_FALSE(checkJpegHeader(png, sizeof(png), err));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, err.find("not a JPEG"));
    TEST_ASSERT_FALSE(checkJpegHeader(png, 0, err));
}

void test_jpeg_wrong_size_rejected() {
    // Minimal SOI + SOF0 declaring 100x100
    const uint8_t small[] = {0xFF, 0xD8, 0xFF, 0xC0, 0x00, 0x11, 0x08, 0x00, 0x64, 0x00, 0x64, 0x03};
    std::string err;
    TEST_ASSERT_FALSE(checkJpegHeader(small, sizeof(small), err));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, err.find("240"));
}

void test_station_id() {
    uint32_t id = 99;
    std::string err;
    TEST_ASSERT_TRUE(parseStationId("", id, err));
    TEST_ASSERT_EQUAL_UINT32(0, id); // blank = unused
    TEST_ASSERT_TRUE(parseStationId(" 123456 ", id, err));
    TEST_ASSERT_EQUAL_UINT32(123456, id);
    TEST_ASSERT_TRUE(parseStationId("123456789", id, err));
    TEST_ASSERT_FALSE(parseStationId("1234567890", id, err)); // 10 digits
    TEST_ASSERT_FALSE(parseStationId("12a", id, err));
    TEST_ASSERT_FALSE(parseStationId("-5", id, err));
}

void test_station_label() {
    std::string out, err;
    TEST_ASSERT_TRUE(parseStationLabel("", 0, out, err)); // unused station, no label needed
    TEST_ASSERT_EQUAL_STRING("", out.c_str());
    TEST_ASSERT_FALSE(parseStationLabel("  ", 123, out, err)); // a station needs a label
    TEST_ASSERT_TRUE(parseStationLabel(" SLN ", 123, out, err));
    TEST_ASSERT_EQUAL_STRING("SLN", out.c_str());
    TEST_ASSERT_TRUE(parseStationLabel("Salt Lake City", 123, out, err));
    TEST_ASSERT_FALSE(parseStationLabel("SEVENTEEN-LETTERS", 123, out, err)); // 17 > 16
}

// --- custom pages ------------------------------------------------------------

void test_mask_url() {
    TEST_ASSERT_EQUAL_STRING("https://example.com/a.json", maskUrl("https://example.com/a.json").c_str());
    TEST_ASSERT_EQUAL_STRING("https://example.com/a.json?\xE2\x80\xA2\xE2\x80\xA2\xE2\x80\xA2",
                             maskUrl("https://example.com/a.json?key=SECRET&x=1").c_str());
    TEST_ASSERT_EQUAL_STRING("", maskUrl("").c_str());
}

void test_web_url() {
    std::string out, err;
    const std::string stored = "https://example.com/a.json?key=SECRET";
    // blank clears
    TEST_ASSERT_TRUE(parseWebUrl("  ", stored, out, err));
    TEST_ASSERT_EQUAL_STRING("", out.c_str());
    // the masked value sent back unchanged keeps the stored URL, secret included
    TEST_ASSERT_TRUE(parseWebUrl(maskUrl(stored), stored, out, err));
    TEST_ASSERT_EQUAL_STRING(stored.c_str(), out.c_str());
    // a new URL replaces it
    TEST_ASSERT_TRUE(parseWebUrl(" http://example.org/b ", stored, out, err));
    TEST_ASSERT_EQUAL_STRING("http://example.org/b", out.c_str());
    TEST_ASSERT_TRUE(parseWebUrl("HTTPS://example.org/b", stored, out, err));
    // an edited path that kept the mask cannot be rebuilt
    out = "unchanged";
    TEST_ASSERT_FALSE(parseWebUrl("https://example.com/other.json?\xE2\x80\xA2\xE2\x80\xA2\xE2\x80\xA2", stored, out, err));
    TEST_ASSERT_EQUAL_STRING("unchanged", out.c_str()); // out untouched on failure
    TEST_ASSERT_FALSE(err.empty());
    TEST_ASSERT_FALSE(parseWebUrl("ftp://example.com/a", stored, out, err));
    TEST_ASSERT_FALSE(parseWebUrl("example.com/a.json", stored, out, err));
    TEST_ASSERT_FALSE(parseWebUrl("https://", stored, out, err)); // no host
    TEST_ASSERT_FALSE(parseWebUrl("https://exa mple.com/", stored, out, err)); // space
    TEST_ASSERT_FALSE(parseWebUrl("https://example.com/" + std::string(250, 'a'), stored, out, err)); // 270 > 256
}

void test_host() {
    std::string out, err;
    TEST_ASSERT_TRUE(parseHost("", out, err));
    TEST_ASSERT_EQUAL_STRING("", out.c_str());
    TEST_ASSERT_TRUE(parseHost(" broker.local ", out, err));
    TEST_ASSERT_EQUAL_STRING("broker.local", out.c_str());
    TEST_ASSERT_TRUE(parseHost("10.0.0.5", out, err));
    TEST_ASSERT_TRUE(parseHost("my-broker", out, err));
    TEST_ASSERT_FALSE(parseHost("mqtt://broker", out, err)); // scheme typed in
    TEST_ASSERT_FALSE(parseHost("broker:1883", out, err)); // port goes in its own field
    TEST_ASSERT_FALSE(parseHost("-broker", out, err));
    TEST_ASSERT_FALSE(parseHost("broker.", out, err));
    TEST_ASSERT_FALSE(parseHost(std::string(65, 'a'), out, err));
}

void test_port() {
    int out = 0;
    std::string err;
    TEST_ASSERT_TRUE(parsePort("1883", out, err));
    TEST_ASSERT_EQUAL(1883, out);
    TEST_ASSERT_TRUE(parsePort("1", out, err));
    TEST_ASSERT_TRUE(parsePort("65535", out, err));
    TEST_ASSERT_FALSE(parsePort("0", out, err));
    TEST_ASSERT_FALSE(parsePort("65536", out, err));
    TEST_ASSERT_FALSE(parsePort("", out, err));
    TEST_ASSERT_FALSE(parsePort("18a3", out, err));
}

void test_optional_text() {
    std::string out, err;
    TEST_ASSERT_TRUE(parseOptionalText("   ", 8, out, err));
    TEST_ASSERT_EQUAL_STRING("", out.c_str());
    TEST_ASSERT_TRUE(parseOptionalText(" kWh ", 8, out, err));
    TEST_ASSERT_EQUAL_STRING("kWh", out.c_str());
    TEST_ASSERT_FALSE(parseOptionalText("123456789", 8, out, err)); // 9 > 8
}

void test_topic() {
    std::string out, err;
    TEST_ASSERT_TRUE(parseTopic("", out, err));
    TEST_ASSERT_EQUAL_STRING("", out.c_str());
    TEST_ASSERT_TRUE(parseTopic(" homeassistant/statestream/sensor/outdoor_temp/state ", out, err));
    TEST_ASSERT_EQUAL_STRING("homeassistant/statestream/sensor/outdoor_temp/state", out.c_str());
    TEST_ASSERT_FALSE(parseTopic("home/#", out, err));
    TEST_ASSERT_FALSE(parseTopic("home/+/temp", out, err));
    TEST_ASSERT_FALSE(parseTopic(std::string(129, 'a'), out, err));
    TEST_ASSERT_FALSE(parseTopic("a\tb", out, err));
}

void test_password() {
    std::string out, err;
    TEST_ASSERT_TRUE(parsePassword("", false, "old", out, err)); // blank keeps
    TEST_ASSERT_EQUAL_STRING("old", out.c_str());
    TEST_ASSERT_TRUE(parsePassword("new pass", false, "old", out, err)); // spaces kept, not trimmed
    TEST_ASSERT_EQUAL_STRING("new pass", out.c_str());
    TEST_ASSERT_TRUE(parsePassword("ignored", true, "old", out, err)); // clear wins
    TEST_ASSERT_EQUAL_STRING("", out.c_str());
    TEST_ASSERT_FALSE(parsePassword(std::string(65, 'p'), false, "old", out, err));
    TEST_ASSERT_FALSE(parsePassword("a\nb", false, "old", out, err));
}

void test_password_hint() {
    // Untouched: the old wording
    TEST_ASSERT_EQUAL_STRING("No password stored.", passwordHint("", ""));
    TEST_ASSERT_NOT_NULL(strstr(passwordHint("old", "old"), "A password is stored"));
    // Rejected save with a newly typed password: must NOT claim the stored one is kept as typed
    TEST_ASSERT_NOT_NULL(strstr(passwordHint("new", "old"), "Not saved: type the password again"));
    TEST_ASSERT_NOT_NULL(strstr(passwordHint("new", ""), "Not saved: type the password again"));
    TEST_ASSERT_NULL(strstr(passwordHint("new", "old"), "A password is stored"));
    // Rejected save with Clear ticked
    TEST_ASSERT_NOT_NULL(strstr(passwordHint("", "old"), "Not saved: tick the box"));
    // Neither text may carry either password
    TEST_ASSERT_NULL(strstr(passwordHint("hunter2", "swordfish"), "hunter2"));
    TEST_ASSERT_NULL(strstr(passwordHint("hunter2", "swordfish"), "swordfish"));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_cycle);
    RUN_TEST(test_hour);
    RUN_TEST(test_hour_range);
    RUN_TEST(test_list_contains);
    RUN_TEST(test_tickers);
    RUN_TEST(test_text);
    RUN_TEST(test_url_encode);
    RUN_TEST(test_color);
    RUN_TEST(test_html_escape);
    RUN_TEST(test_jpeg_baseline_logo_passes);
    RUN_TEST(test_jpeg_progressive_rejected);
    RUN_TEST(test_jpeg_garbage_rejected);
    RUN_TEST(test_jpeg_wrong_size_rejected);
    RUN_TEST(test_station_id);
    RUN_TEST(test_station_label);
    RUN_TEST(test_mask_url);
    RUN_TEST(test_web_url);
    RUN_TEST(test_host);
    RUN_TEST(test_port);
    RUN_TEST(test_optional_text);
    RUN_TEST(test_topic);
    RUN_TEST(test_password);
    RUN_TEST(test_password_hint);
    return UNITY_END();
}
