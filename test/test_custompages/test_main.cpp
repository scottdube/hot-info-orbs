// Host tests for WebDataPolicy.h and MqttPolicy.h:  pio test -e native
#include "MqttPolicy.h"
#include "WebDataPolicy.h"
#include <string>
#include <unity.h>

void setUp() {}
void tearDown() {}

void test_interval_missing_is_60s() {
    TEST_ASSERT_EQUAL_UINT32(60000, webDataInterval(false, 0));
}

void test_interval_clamped_to_10s() {
    TEST_ASSERT_EQUAL_UINT32(10000, webDataInterval(true, 2000));
    TEST_ASSERT_EQUAL_UINT32(10000, webDataInterval(true, 1000)); // upstream's old default
    TEST_ASSERT_EQUAL_UINT32(10000, webDataInterval(true, -5));
}

void test_interval_kept_when_long_enough() {
    TEST_ASSERT_EQUAL_UINT32(300000, webDataInterval(true, 300000));
    TEST_ASSERT_EQUAL_UINT32(10000, webDataInterval(true, 10000));
}

void test_first_attempt_is_immediate() {
    TEST_ASSERT_TRUE(mqttShouldAttempt(false, 0, 0));
    TEST_ASSERT_TRUE(mqttShouldAttempt(false, 123456, 0));
}

void test_no_attempt_within_30s() {
    TEST_ASSERT_FALSE(mqttShouldAttempt(true, 1000, 1000));
    TEST_ASSERT_FALSE(mqttShouldAttempt(true, 30999, 1000));
    TEST_ASSERT_TRUE(mqttShouldAttempt(true, 31000, 1000));
}

void test_retry_survives_millis_wrap() {
    // millis() wraps after ~49.7 days; unsigned subtraction must still work
    TEST_ASSERT_FALSE(mqttShouldAttempt(true, 10000, 0xFFFFF000u));
    TEST_ASSERT_TRUE(mqttShouldAttempt(true, 40000, 0xFFFFF000u));
}

void test_state_text() {
    TEST_ASSERT_EQUAL_STRING("connected", mqttStateText(0));
    TEST_ASSERT_EQUAL_STRING("can't reach broker", mqttStateText(-2));
    TEST_ASSERT_EQUAL_STRING("can't reach broker", mqttStateText(-4));
    TEST_ASSERT_EQUAL_STRING("connection lost", mqttStateText(-3));
    TEST_ASSERT_EQUAL_STRING("login refused", mqttStateText(4));
    TEST_ASSERT_EQUAL_STRING("login refused", mqttStateText(5));
    TEST_ASSERT_EQUAL_STRING("broker refused the connection", mqttStateText(3));
    TEST_ASSERT_EQUAL_STRING("not connected", mqttStateText(-1));
    TEST_ASSERT_EQUAL_STRING("not connected", mqttStateText(99));
}

void test_connect_error_by_elapsed_time() {
    // -1 is refusal, DNS failure and connect timeout alike: only the clock tells them apart
    TEST_ASSERT_EQUAL_STRING("can't connect", webDataConnectError(-1, 40));
    TEST_ASSERT_EQUAL_STRING("can't connect", webDataConnectError(-1, 4700));
    TEST_ASSERT_EQUAL_STRING("timeout", webDataConnectError(-1, 4750)); // within the slack
    TEST_ASSERT_EQUAL_STRING("timeout", webDataConnectError(-1, 5000)); // measured: 5.0 s
    TEST_ASSERT_EQUAL_STRING("timeout", webDataConnectError(-1, 9000));
}

void test_read_timeout_is_timeout_at_any_time() {
    TEST_ASSERT_EQUAL_STRING("timeout", webDataConnectError(-11, 0));
    TEST_ASSERT_EQUAL_STRING("timeout", webDataConnectError(-11, 5000));
}

void test_other_codes_are_left_to_the_caller() {
    TEST_ASSERT_NULL(webDataConnectError(200, 5000));
    TEST_ASSERT_NULL(webDataConnectError(404, 20));
    TEST_ASSERT_NULL(webDataConnectError(-4, 5000)); // e.g. HTTPC_ERROR_NOT_CONNECTED keeps errorToString
}

void test_stalled_body_counts_as_timeout() {
    // IncompleteInput after the fetch ran the full timeout; a quick truncation stays "bad JSON"
    TEST_ASSERT_TRUE(webDataRanToTimeout(5000));
    TEST_ASSERT_TRUE(webDataRanToTimeout(4750));
    TEST_ASSERT_FALSE(webDataRanToTimeout(4749));
    TEST_ASSERT_FALSE(webDataRanToTimeout(300));
}

void test_retry_waits_at_least_a_minute_after_failure() {
    TEST_ASSERT_EQUAL_UINT32(60000, webDataRetryDelay(10000)); // a good reply asked for 10 s
    TEST_ASSERT_EQUAL_UINT32(60000, webDataRetryDelay(0));
    TEST_ASSERT_EQUAL_UINT32(60000, webDataRetryDelay(60000));
    TEST_ASSERT_EQUAL_UINT32(300000, webDataRetryDelay(300000)); // never shortens a longer wait
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_interval_missing_is_60s);
    RUN_TEST(test_interval_clamped_to_10s);
    RUN_TEST(test_interval_kept_when_long_enough);
    RUN_TEST(test_first_attempt_is_immediate);
    RUN_TEST(test_no_attempt_within_30s);
    RUN_TEST(test_retry_survives_millis_wrap);
    RUN_TEST(test_state_text);
    RUN_TEST(test_connect_error_by_elapsed_time);
    RUN_TEST(test_read_timeout_is_timeout_at_any_time);
    RUN_TEST(test_other_codes_are_left_to_the_caller);
    RUN_TEST(test_stalled_body_counts_as_timeout);
    RUN_TEST(test_retry_waits_at_least_a_minute_after_failure);
    return UNITY_END();
}
