// Host tests for RoamPolicy.h:  pio test -e native
#include "RoamPolicy.h"
#include <unity.h>

void setUp() {}
void tearDown() {}

static const uint8_t HERE[6] = {1, 1, 1, 1, 1, 1};
static ApSeen ap(const char *ssid, uint8_t id, int rssi) {
    ApSeen a{ssid, {id, id, id, id, id, id}, rssi, 6};
    return a;
}

void test_stays_when_nothing_better() {
    std::vector<ApSeen> seen = {ap("home", 1, -80), ap("home", 2, -75)}; // only 5 dB better
    TEST_ASSERT_EQUAL(-1, pickRoamTarget("home", HERE, -80, seen));
    TEST_ASSERT_EQUAL(-1, pickRoamTarget("home", HERE, -80, {}));
}

void test_moves_to_clearly_stronger_ap() {
    std::vector<ApSeen> seen = {ap("home", 1, -80), ap("home", 2, -72)}; // exactly the margin
    TEST_ASSERT_EQUAL(1, pickRoamTarget("home", HERE, -80, seen));
}

void test_picks_the_strongest() {
    std::vector<ApSeen> seen = {ap("home", 2, -60), ap("home", 3, -45), ap("home", 4, -55)};
    TEST_ASSERT_EQUAL(1, pickRoamTarget("home", HERE, -80, seen));
}

void test_ignores_other_networks_and_itself() {
    std::vector<ApSeen> seen = {ap("neighbor", 2, -40), ap("home", 1, -40)}; // current AP, stronger reading
    TEST_ASSERT_EQUAL(-1, pickRoamTarget("home", HERE, -80, seen));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_stays_when_nothing_better);
    RUN_TEST(test_moves_to_clearly_stronger_ap);
    RUN_TEST(test_picks_the_strongest);
    RUN_TEST(test_ignores_other_networks_and_itself);
    return UNITY_END();
}
