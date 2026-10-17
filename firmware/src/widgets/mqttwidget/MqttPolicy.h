#pragma once
// MQTT connect timing. std only, so it is covered by `pio test -e native`
// (test/test_custompages).
//
// Upstream reconnect() looped `while (!connected)` with a 5 s delay: a
// mistyped broker froze the orb - no buttons, and no settings page to fix it on.

#include <cstdint>

constexpr uint32_t MQTT_RETRY_MS = 30000;
constexpr int MQTT_SOCKET_TIMEOUT_S = 3; // PubSubClient::setSocketTimeout

// First attempt at once, then no more than one per MQTT_RETRY_MS.
// Unsigned subtraction, so a millis() wrap does not stall it.
inline bool mqttShouldAttempt(bool attemptedBefore, uint32_t now, uint32_t lastAttempt) {
    return !attemptedBefore || (uint32_t)(now - lastAttempt) >= MQTT_RETRY_MS;
}

// PubSubClient::state() in words for the settings page. Numbers are
// PubSubClient.h's MQTT_* constants, written out so this compiles on the host.
inline const char *mqttStateText(int state) {
    switch (state) {
    case 0:
        return "connected";
    case -4: // MQTT_CONNECTION_TIMEOUT
    case -2: // MQTT_CONNECT_FAILED
        return "can't reach broker";
    case -3: // MQTT_CONNECTION_LOST
        return "connection lost";
    case 4: // MQTT_CONNECT_BAD_CREDENTIALS
    case 5: // MQTT_CONNECT_UNAUTHORIZED
        return "login refused";
    case 1: // MQTT_CONNECT_BAD_PROTOCOL
    case 2: // MQTT_CONNECT_BAD_CLIENT_ID
    case 3: // MQTT_CONNECT_UNAVAILABLE
        return "broker refused the connection";
    default:
        return "not connected";
    }
}
