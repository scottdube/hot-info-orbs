// Host tests for WakeScript.h:  pio test -e native -f test_booteyes
#include "LidPalette.h"
#include "WakeScript.h"
#include <cstring>
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

// After the finale: awake, looking around and blinking until BootEyes::end()
void test_awake_loop_keyframes() {
    WakeScript s = started();
    const uint32_t C = T0 + 5000;
    s.connected(C);
    const uint32_t A = C + 2000; // awake loop start
    TEST_ASSERT_EQUAL(512, s.at(A + 1500).gazeX); // hold center
    TEST_ASSERT_EQUAL(200, s.at(A + 1800).gazeX); // glance one way
    TEST_ASSERT_EQUAL(200, s.at(A + 2600).gazeX);
    TEST_ASSERT_EQUAL(830, s.at(A + 3000).gazeX); // then the other
    TEST_ASSERT_EQUAL(830, s.at(A + 3800).gazeX);
    TEST_ASSERT_EQUAL(512, s.at(A + 4100).gazeX); // back to center
    TEST_ASSERT_EQUAL(256, s.at(A + 4700).open);
    TEST_ASSERT_EQUAL(0, s.at(A + 4800).open);   // blink
    TEST_ASSERT_EQUAL(256, s.at(A + 5000).open);
    TEST_ASSERT_TRUE(s.at(A + 4800).done);       // done = finale over
}

void test_awake_loop_repeats() {
    WakeScript s = started();
    const uint32_t A = T0 + 2600 + 2000; // early connect: finale at 2600
    s.connected(T0 + 100);
    for (uint32_t t = 0; t < 6000; t += 41) {
        EyePose a = s.at(A + t), b = s.at(A + 6000 + t), c = s.at(A + 60000 + t);
        TEST_ASSERT_EQUAL(a.open, b.open);
        TEST_ASSERT_EQUAL(a.gazeX, c.gazeX);
        TEST_ASSERT_EQUAL(a.open, c.open);
    }
}

void test_finale_to_awake_is_continuous() {
    WakeScript s = started();
    const uint32_t C = T0 + 5000;
    s.connected(C);
    EyePose a = s.at(C + 1999), b = s.at(C + 2000), c = s.at(C + 2001);
    TEST_ASSERT_INT_WITHIN(2, a.open, b.open);
    TEST_ASSERT_INT_WITHIN(2, b.gazeX, c.gazeX);
    TEST_ASSERT_EQUAL(512, b.gazeY);
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


// --- LidPalette.h ---------------------------------------------------------------

static int styleIndex(const char *name) {
    for (int i = 0; i < lids::kCount; i++) {
        if (strcmp(lids::kStyles[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

// Stored as an index: these may never move (append only)
void test_lid_indices_are_fixed() {
    TEST_ASSERT_EQUAL(0, styleIndex("Taupe"));
    TEST_ASSERT_EQUAL(1, styleIndex("Skin"));
    TEST_ASSERT_EQUAL(2, styleIndex("Tan"));
    TEST_ASSERT_EQUAL(3, styleIndex("Robot"));
    TEST_ASSERT_EQUAL(4, styleIndex("Stone"));
    TEST_ASSERT_EQUAL(5, styleIndex("Ash"));
    TEST_ASSERT_EQUAL(6, styleIndex("Black"));
    TEST_ASSERT_EQUAL(7, styleIndex("Chrome"));
    TEST_ASSERT_EQUAL(8, styleIndex("Stainless"));
    TEST_ASSERT_EQUAL(0, lids::kDefault);
    TEST_ASSERT_TRUE(lids::kDemo >= lids::kCount);
}

void test_lid_valid() {
    TEST_ASSERT_TRUE(lids::valid(0));
    TEST_ASSERT_TRUE(lids::valid(lids::kCount - 1));
    TEST_ASSERT_TRUE(lids::valid(lids::kDemo));
    TEST_ASSERT_FALSE(lids::valid(-1));
    TEST_ASSERT_FALSE(lids::valid(lids::kCount));
}

// Same packing as TFT_eSPI::color565
void test_lid_to565() {
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, lids::to565({255, 255, 255}));
    TEST_ASSERT_EQUAL_HEX16(0x0000, lids::to565({0, 0, 0}));
    TEST_ASSERT_EQUAL_HEX16(0xF800, lids::to565({255, 0, 0}));
    TEST_ASSERT_EQUAL_HEX16(0x07E0, lids::to565({0, 255, 0}));
    TEST_ASSERT_EQUAL_HEX16(0x001F, lids::to565({0, 0, 255}));
}

// Taupe draws exactly as 1.3.0 did: shadow band to margin 20, then skin
void test_lid_taupe_unchanged() {
    const lids::Style &t = lids::kStyles[0];
    const lids::Rgb a = lids::body(t, 6, 0), b = lids::body(t, 19, 3), c = lids::body(t, 20, 1);
    TEST_ASSERT_EQUAL(128, a.r);
    TEST_ASSERT_EQUAL(106, a.g);
    TEST_ASSERT_EQUAL(95, a.b);
    TEST_ASSERT_EQUAL(128, b.r);
    TEST_ASSERT_EQUAL(170, c.r);
    TEST_ASSERT_EQUAL(143, c.g);
    TEST_ASSERT_EQUAL(129, c.b);
    TEST_ASSERT_TRUE(t.strands);
}

// Chrome: bright highlight near the edge, dark band after it, light again deep
void test_lid_chrome_bands() {
    const lids::Style &c = lids::kStyles[styleIndex("Chrome")];
    const int hi = lids::body(c, 12, 0).r, band = lids::body(c, 38, 0).r, deep = lids::body(c, 200, 0).r;
    TEST_ASSERT_TRUE(hi > 240);
    TEST_ASSERT_TRUE(band < 80);
    TEST_ASSERT_TRUE(deep > 200);
    TEST_ASSERT_EQUAL(lids::body(c, 255, 0).r, lids::body(c, 999, 0).r); // clamps past the map
    TEST_ASSERT_EQUAL(lids::body(c, 38, 0).r, lids::body(c, 38, 3).r);  // not brushed
    TEST_ASSERT_FALSE(c.strands);
}

// Stainless: brushed, so rows differ by a few steps, and stays mid-grey
void test_lid_stainless_brushed() {
    const lids::Style &s = lids::kStyles[styleIndex("Stainless")];
    TEST_ASSERT_EQUAL(20, lids::body(s, 60, 3).r - lids::body(s, 60, 0).r);
    bool seen[lids::kBrushLevels] = {};
    for (int row = 0; row < 128; row++) {
        const int l = lids::brushLevel(row);
        TEST_ASSERT_TRUE(l >= 0 && l < lids::kBrushLevels);
        seen[l] = true;
    }
    for (bool b : seen) {
        TEST_ASSERT_TRUE(b); // every step shows up somewhere
    }
    TEST_ASSERT_EQUAL(lids::brushLevel(17), lids::brushLevel(17)); // stable frame to frame
}

void test_lid_demo_cycles_every_style() {
    TEST_ASSERT_EQUAL(0, lids::demoStyle(0));
    TEST_ASSERT_EQUAL(0, lids::demoStyle(lids::kDemoCycleMs - 1));
    TEST_ASSERT_EQUAL(1, lids::demoStyle(lids::kDemoCycleMs));
    TEST_ASSERT_EQUAL(lids::kCount - 1, lids::demoStyle(lids::kDemoCycleMs * (lids::kCount - 1)));
    TEST_ASSERT_EQUAL(0, lids::demoStyle(lids::kDemoCycleMs * lids::kCount)); // wraps
    TEST_ASSERT_EQUAL(500, lids::demoCycleTime(lids::kDemoCycleMs * 3 + 500));
}

// Each demo cycle is one whole wake: shut at the start, finale done and awake
// by the end, wide open just before the next style cuts in
void test_lid_demo_cycle_is_a_whole_wake() {
    WakeScript w;
    w.begin(0);
    w.connected(lids::kDemoConnectMs);
    TEST_ASSERT_EQUAL(0, w.at(0).open);
    TEST_ASSERT_FALSE(w.at(lids::kDemoConnectMs + wake::kFinaleMs - 1).done);
    TEST_ASSERT_TRUE(w.at(lids::kDemoConnectMs + wake::kFinaleMs).done);
    TEST_ASSERT_EQUAL(lids::kDemoCycleMs, lids::kDemoConnectMs + wake::kFinaleMs + wake::kAwakeMs);
    TEST_ASSERT_EQUAL(256, w.at(lids::kDemoCycleMs - 1).open);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_intro_starts_shut_and_looking_down);
    RUN_TEST(test_intro_keyframes);
    RUN_TEST(test_loop_keyframes_and_period);
    RUN_TEST(test_never_done_without_connect);
    RUN_TEST(test_early_connect_waits_for_first_crack_open);
    RUN_TEST(test_finale_keyframes_and_end);
    RUN_TEST(test_awake_loop_keyframes);
    RUN_TEST(test_awake_loop_repeats);
    RUN_TEST(test_finale_to_awake_is_continuous);
    RUN_TEST(test_handoff_is_continuous);
    RUN_TEST(test_second_connect_is_ignored);
    RUN_TEST(test_lid_indices_are_fixed);
    RUN_TEST(test_lid_valid);
    RUN_TEST(test_lid_to565);
    RUN_TEST(test_lid_taupe_unchanged);
    RUN_TEST(test_lid_chrome_bands);
    RUN_TEST(test_lid_stainless_brushed);
    RUN_TEST(test_lid_demo_cycles_every_style);
    RUN_TEST(test_lid_demo_cycle_is_a_whole_wake);
    return UNITY_END();
}
