// Host tests for WakeScript.h:  pio test -e native -f test_booteyes
#include "IrisPalette.h"
#include "LidPalette.h"
#include "NoseShape.h"
#include "WakeScript.h"
#include <cstdlib>
#include <cstring>
#include <initializer_list>
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
    TEST_ASSERT_EQUAL(9, styleIndex("Frost"));
    TEST_ASSERT_EQUAL(10, styleIndex("Night King"));
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

// --- IrisPalette.h --------------------------------------------------------------

void test_iris_indices_are_fixed() {
    const char *want[] = {"Natural", "Blue", "Brown", "Green", "Grey", "Amber", "Violet", "Red", "Ice", "Night King"};
    TEST_ASSERT_EQUAL(10, iriscolor::kCount);
    for (int i = 0; i < iriscolor::kCount; i++) {
        TEST_ASSERT_EQUAL_STRING(want[i], iriscolor::kIrises[i].name);
    }
    TEST_ASSERT_EQUAL(0, iriscolor::kDefault);
    TEST_ASSERT_TRUE(iriscolor::valid(iriscolor::kCount - 1));
    TEST_ASSERT_FALSE(iriscolor::valid(iriscolor::kCount));
    TEST_ASSERT_FALSE(iriscolor::valid(-1));
}

void test_iris_luma() {
    TEST_ASSERT_EQUAL(0, iriscolor::luma565(0x0000));
    TEST_ASSERT_TRUE(iriscolor::luma565(0xFFFF) >= 248);
    TEST_ASSERT_TRUE(iriscolor::luma565(0x07E0) > iriscolor::luma565(0xF800)); // green reads brighter than red
    TEST_ASSERT_TRUE(iriscolor::luma565(0xF800) > iriscolor::luma565(0x001F)); // red brighter than blue
}

// The texture's range maps onto the whole ramp: dark end at lo, light at hi
void test_iris_ramp_stretches_the_range() {
    uint16_t r[256];
    const iriscolor::Iris &blue = iriscolor::kIrises[1];
    iriscolor::buildRamp(blue, 40, 200, r);
    TEST_ASSERT_EQUAL_HEX16(lids::to565(blue.dark), r[0]);
    TEST_ASSERT_EQUAL_HEX16(lids::to565(blue.dark), r[40]);
    TEST_ASSERT_EQUAL_HEX16(lids::to565(blue.light), r[200]);
    TEST_ASSERT_EQUAL_HEX16(lids::to565(blue.light), r[255]);
    TEST_ASSERT_TRUE((r[120] & 0x1F) > (r[40] & 0x1F)); // bluer toward light
}

void test_iris_range_trims_two_percent() {
    uint32_t h[256] = {};
    h[5] = 1;     // a speck, under 2%
    h[60] = 500;
    h[180] = 500;
    h[250] = 1;   // a speck
    int lo = 0, hi = 0;
    iriscolor::range(h, lo, hi);
    TEST_ASSERT_EQUAL(60, lo);
    TEST_ASSERT_EQUAL(180, hi);
}

// --- Night King ------------------------------------------------------------------

// Only the Night King entries change the eye's shape; every other choice,
// Ice and Frost included, renders exactly as before
void test_night_king_is_its_own_choice() {
    for (int i = 0; i < iriscolor::kCount; i++) {
        const bool nk = strcmp(iriscolor::kIrises[i].name, "Night King") == 0;
        TEST_ASSERT_EQUAL(nk, iriscolor::kIrises[i].nightKing);
    }
    const lids::Style &frost = lids::kStyles[styleIndex("Frost")];
    TEST_ASSERT_TRUE(frost.finish == lids::Finish::Flat);
    TEST_ASSERT_TRUE(lids::kStyles[styleIndex("Night King")].finish == lids::Finish::Socket);
}

// Walk once around the star just inside its reach: count the runs of pupil
void test_night_king_star_has_seven_points() {
    namespace nk = iriscolor::nightking;
    const uint32_t raw = 127 - (nk::kTip - 6);
    int runs = 0;
    bool prev = nk::pupil(raw, 511);
    for (uint32_t a = 0; a < 512; a++) {
        const bool cur = nk::pupil(raw, a);
        if (cur && !prev) {
            runs++;
        }
        prev = cur;
    }
    TEST_ASSERT_EQUAL(7, runs);
    TEST_ASSERT_TRUE(nk::pupil(127 - (nk::kTip - 2), nk::kUp)); // a point straight up
    TEST_ASSERT_TRUE(nk::pupil(127, 0));                        // the core
    TEST_ASSERT_FALSE(nk::pupil(127 - nk::kTip, nk::kUp));      // nothing past the tips
}

// Every speck sits wholly inside the iris and clear of the star's reach
void test_night_king_specks_in_the_iris() {
    namespace nk = iriscolor::nightking;
    const int tipR = 79 * nk::kTip / 127; // the star's reach in half pixels
    for (int i = 0; i < nk::kSpeckCount; i++) {
        const nk::Speck &k = nk::kSpecks[i];
        const int R = k.r + nk::kRing;
        const int c2 = k.x * k.x + k.y * k.y;
        TEST_ASSERT_TRUE(c2 < (79 - R) * (79 - R));
        TEST_ASSERT_TRUE(c2 > (tipR + R) * (tipR + R));
        TEST_ASSERT_TRUE(k.r >= 4); // smaller comes out square on the panel
        TEST_ASSERT_EQUAL(2, nk::speck(k.x, k.y));
        TEST_ASSERT_EQUAL(1, nk::speck(k.x + k.r + 1, k.y));
    }
    TEST_ASSERT_EQUAL(0, nk::speck(0, 0));
}

// The sockets are dark at the lid edge and fade out to Frost's skin
void test_night_king_sockets_fade_to_frost() {
    const lids::Style &s = lids::kStyles[styleIndex("Night King")];
    const lids::Style &frost = lids::kStyles[styleIndex("Frost")];
    const lids::Rgb edge = lids::body(s, lids::kLashW, 0);
    const lids::Rgb deep = lids::body(s, 255, 0);
    TEST_ASSERT_TRUE(edge.r + edge.g + edge.b < 60);
    TEST_ASSERT_EQUAL(frost.skin.r, deep.r);
    TEST_ASSERT_EQUAL(frost.skin.g, deep.g);
    TEST_ASSERT_EQUAL(frost.skin.b, deep.b);
}

// ---- The nose on orb 3 (issue #24): drawn in the selected lid style ----

static int noseLuma(lids::Rgb c) { return (c.r * 30 + c.g * 59 + c.b * 11) / 100; }
static bool noseNear(lids::Rgb a, lids::Rgb b, int tol) {
    return abs(a.r - b.r) <= tol && abs(a.g - b.g) <= tol && abs(a.b - b.b) <= tol;
}
static const lids::Style &lidNamed(const char *name) {
    for (int i = 0; i < lids::kCount; i++) {
        if (strcmp(lids::kStyles[i].name, name) == 0) return lids::kStyles[i];
    }
    TEST_FAIL_MESSAGE(name);
    return lids::kStyles[0];
}
// Deep inside one wing, clear of the nostrils and of both highlights
static const int kDeepX = 150, kDeepY = 172;

void test_nose_is_black_around_the_outline() {
    const lids::Style &s = lidNamed("Skin");
    const int pts[][2] = {{5, 120}, {120, 10}, {120, 232}, {30, 60}, {210, 200}, {119, 40}};
    for (const auto &p : pts) {
        TEST_ASSERT_FALSE(nose::inside(p[0], p[1]));
        TEST_ASSERT_EQUAL(0, noseLuma(nose::pixel(s, p[0], p[1])));
    }
    TEST_ASSERT_TRUE(nose::inside(120, 166));
}

void test_nose_outline_is_symmetric_and_inside_its_box() {
    int count = 0;
    for (int y = 0; y < 240; y++) {
        for (int x = 0; x < 240; x++) {
            const bool in = nose::inside(x, y);
            TEST_ASSERT_EQUAL(in, nose::inside(239 - x, y));
            if (in) {
                count++;
                // the renderer skips everything outside this box
                TEST_ASSERT_TRUE(x >= nose::kLeft && x < nose::kRight && y >= nose::kTop && y < nose::kBottom);
                // and the box is well clear of the round panel's edge
                TEST_ASSERT_TRUE((x - 120) * (x - 120) + (y - 120) * (y - 120) < 108 * 108);
            }
        }
    }
    TEST_ASSERT_TRUE(count > 9000); // a nose, not a dot: about a fifth of the panel
}

void test_nose_takes_the_lid_skin_and_shadow() {
    for (const char *name : {"Taupe", "Skin", "Tan", "Robot", "Stone", "Ash", "Frost"}) {
        const lids::Style &s = lidNamed(name);
        // deep inside: between shadow and skin, nearer the skin
        const lids::Rgb deep = nose::pixel(s, kDeepX, kDeepY);
        TEST_ASSERT_TRUE_MESSAGE(deep.r >= s.shadow.r && deep.r <= s.skin.r, name);
        TEST_ASSERT_TRUE_MESSAGE(abs(deep.r - s.skin.r) < abs(deep.r - s.shadow.r), name);
        TEST_ASSERT_TRUE_MESSAGE(abs(deep.b - s.skin.b) < abs(deep.b - s.shadow.b), name);
        // the first pixel inside the outline is the lid's shadow color
        int x = 0;
        while (!nose::inside(x, 180)) x++;
        TEST_ASSERT_TRUE_MESSAGE(noseNear(nose::pixel(s, x, 180), s.shadow, 4), name);
    }
}

void test_nose_nostrils_are_the_lash_color() {
    const lids::Style &s = lidNamed("Skin");
    TEST_ASSERT_TRUE(noseNear(nose::pixel(s, 120 - 21, 198), s.lash, 2));
    TEST_ASSERT_TRUE(noseNear(nose::pixel(s, 119 + 21, 198), s.lash, 2));
    // and between them it is skin again
    TEST_ASSERT_TRUE(noseLuma(nose::pixel(s, 120, 196)) > 100);
}

void test_nose_bridge_fades_into_the_forehead() {
    const lids::Style &s = lidNamed("Skin");
    TEST_ASSERT_TRUE(nose::inside(120, 60));
    TEST_ASSERT_EQUAL(0, noseLuma(nose::pixel(s, 120, 60)));
    const int a = noseLuma(nose::pixel(s, 120, 75)), b = noseLuma(nose::pixel(s, 120, 92)), c = noseLuma(nose::pixel(s, 120, 114));
    TEST_ASSERT_TRUE(a > 0 && a < b && b < c);
}

void test_nose_metal_follows_the_lid_ramp() {
    const lids::Style &chrome = lidNamed("Chrome"), &steel = lidNamed("Stainless");
    int chromeMax = 0, chromeMin = 255, steelMax = 0;
    for (int y = 130; y < 200; y++) { // below the fade
        for (int x = 0; x < 240; x++) {
            if (!nose::inside(x, y)) continue;
            const int c = noseLuma(nose::pixel(chrome, x, y));
            chromeMax = c > chromeMax ? c : chromeMax;
            chromeMin = c < chromeMin ? c : chromeMin;
            const int st = noseLuma(nose::pixel(steel, x, y));
            steelMax = st > steelMax ? st : steelMax;
        }
    }
    TEST_ASSERT_TRUE(chromeMax >= 240); // the hard highlight band inside the rim
    TEST_ASSERT_TRUE(chromeMin <= 90);  // the rim, the horizon band or a nostril
    TEST_ASSERT_TRUE(steelMax >= 190 && steelMax <= 230); // softer, never white
    // Stainless is brushed: the same depth on two rows is not always the same grey
    bool brushed = false;
    for (int y = 150; y < 170 && !brushed; y += 2) {
        brushed = noseLuma(nose::pixel(steel, 120, y)) != noseLuma(nose::pixel(steel, 120, y + 2));
    }
    TEST_ASSERT_TRUE(brushed);
}

void test_nose_night_king_is_frost_not_socket_dark() {
    // The socket ramp is near black for a long way in; used as it is on the
    // eye lids it left the nose invisible (design mock, 2026-11-12)
    const lids::Rgb deep = nose::pixel(lidNamed("Night King"), kDeepX, kDeepY);
    TEST_ASSERT_TRUE(noseLuma(deep) > 100);
    TEST_ASSERT_TRUE(deep.b > deep.r); // blue-grey, like Frost
}

void test_nose_black_lids_draw_an_outline() {
    const lids::Style &s = lidNamed("Black");
    TEST_ASSERT_EQUAL(0, noseLuma(nose::pixel(s, kDeepX, kDeepY))); // black inside, as the lids are
    int lit = 0;
    for (int y = 0; y < 240; y++) {
        for (int x = 0; x < 240; x++) lit += noseLuma(nose::pixel(s, x, y)) >= 30;
    }
    TEST_ASSERT_TRUE(lit >= 200); // a ring of rim light, or orb 3 looks dead
}

void test_nose_shows_in_every_style() {
    for (int i = 0; i < lids::kCount; i++) {
        int lit = 0;
        for (int y = 0; y < 240; y++) {
            for (int x = 0; x < 240; x++) lit += noseLuma(nose::pixel(lids::kStyles[i], x, y)) >= 30;
        }
        TEST_ASSERT_TRUE_MESSAGE(lit >= 200, lids::kStyles[i].name);
    }
}

void test_nose_style_index_out_of_range_is_the_default() {
    TEST_ASSERT_EQUAL_PTR(&lids::kStyles[lids::kDefault], &nose::style(-1));
    TEST_ASSERT_EQUAL_PTR(&lids::kStyles[lids::kDefault], &nose::style(lids::kCount));
    TEST_ASSERT_EQUAL_PTR(&lids::kStyles[lids::kDefault], &nose::style(lids::kDemo));
    TEST_ASSERT_EQUAL_PTR(&lids::kStyles[10], &nose::style(10));
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
    RUN_TEST(test_iris_indices_are_fixed);
    RUN_TEST(test_iris_luma);
    RUN_TEST(test_iris_ramp_stretches_the_range);
    RUN_TEST(test_iris_range_trims_two_percent);
    RUN_TEST(test_night_king_is_its_own_choice);
    RUN_TEST(test_night_king_star_has_seven_points);
    RUN_TEST(test_night_king_specks_in_the_iris);
    RUN_TEST(test_night_king_sockets_fade_to_frost);
    RUN_TEST(test_nose_is_black_around_the_outline);
    RUN_TEST(test_nose_outline_is_symmetric_and_inside_its_box);
    RUN_TEST(test_nose_takes_the_lid_skin_and_shadow);
    RUN_TEST(test_nose_nostrils_are_the_lash_color);
    RUN_TEST(test_nose_bridge_fades_into_the_forehead);
    RUN_TEST(test_nose_metal_follows_the_lid_ramp);
    RUN_TEST(test_nose_night_king_is_frost_not_socket_dark);
    RUN_TEST(test_nose_black_lids_draw_an_outline);
    RUN_TEST(test_nose_shows_in_every_style);
    RUN_TEST(test_nose_style_index_out_of_range_is_the_default);
    return UNITY_END();
}
