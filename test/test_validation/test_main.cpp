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

// --- tickers -----------------------------------------------------------------

void test_tickers() {
    std::string out, err;
    TEST_ASSERT_TRUE(normaliseTickers("spy, qqq ,AAPL", out, err));
    TEST_ASSERT_EQUAL_STRING("SPY,QQQ,AAPL", out.c_str());
    TEST_ASSERT_TRUE(normaliseTickers("BTC/USD", out, err));
    TEST_ASSERT_EQUAL_STRING("BTC/USD", out.c_str());
    TEST_ASSERT_TRUE(normaliseTickers("shop&country=Canada", out, err));
    TEST_ASSERT_EQUAL_STRING("SHOP&country=Canada", out.c_str());
    TEST_ASSERT_FALSE(normaliseTickers("A,B,C,D,E,F", out, err));
    TEST_ASSERT_FALSE(normaliseTickers("", out, err));
    TEST_ASSERT_FALSE(normaliseTickers(",,", out, err));
    TEST_ASSERT_FALSE(normaliseTickers("SP Y", out, err));
}

// --- text --------------------------------------------------------------------

void test_text() {
    std::string out, err;
    TEST_ASSERT_TRUE(normaliseText("  Dover,   NH ", 64, out, err));
    TEST_ASSERT_EQUAL_STRING("Dover, NH", out.c_str());
    TEST_ASSERT_FALSE(normaliseText(std::string(65, 'a'), 64, out, err));
    TEST_ASSERT_TRUE(normaliseText(std::string(64, 'a'), 64, out, err));
    TEST_ASSERT_FALSE(normaliseText("a\tb", 64, out, err));
    TEST_ASSERT_FALSE(normaliseText("   ", 64, out, err));
}

// --- urlEncode ---------------------------------------------------------------

void test_url_encode() {
    TEST_ASSERT_EQUAL_STRING("Dover%2C%20NH", urlEncode("Dover, NH").c_str());
    TEST_ASSERT_EQUAL_STRING("America%2FNew_York", urlEncode("America/New_York").c_str());
    TEST_ASSERT_EQUAL_STRING("A-z_0.~", urlEncode("A-z_0.~").c_str());
}

// --- colours -----------------------------------------------------------------

void test_colour() {
    uint16_t c = 0;
    std::string err;
    TEST_ASSERT_TRUE(parseHexColour("#fc8000", c, err));
    TEST_ASSERT_EQUAL_HEX16(0xFC00, c);
    TEST_ASSERT_TRUE(parseHexColour("#FFFFFF", c, err));
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, c);
    TEST_ASSERT_FALSE(parseHexColour("fc8000", c, err));
    TEST_ASSERT_FALSE(parseHexColour("#zzzzzz", c, err));
    TEST_ASSERT_EQUAL_STRING("#ffffff", rgb565ToHex(0xFFFF).c_str());
    TEST_ASSERT_EQUAL_STRING("#000000", rgb565ToHex(0x0000).c_str());
    // Round trip is stable once a colour has been through RGB565
    TEST_ASSERT_TRUE(parseHexColour(rgb565ToHex(0xfc80), c, err));
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

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_cycle);
    RUN_TEST(test_hour);
    RUN_TEST(test_tickers);
    RUN_TEST(test_text);
    RUN_TEST(test_url_encode);
    RUN_TEST(test_colour);
    RUN_TEST(test_html_escape);
    RUN_TEST(test_jpeg_baseline_logo_passes);
    RUN_TEST(test_jpeg_progressive_rejected);
    RUN_TEST(test_jpeg_garbage_rejected);
    RUN_TEST(test_jpeg_wrong_size_rejected);
    return UNITY_END();
}
