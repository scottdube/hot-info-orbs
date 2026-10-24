#pragma once
// MQTT connect timing. std only, so it is covered by `pio test -e native`
// (test/test_custompages).
//
// Upstream reconnect() looped `while (!connected)` with a 5 s delay: a
// mistyped broker froze the orb - no buttons, and no settings page to fix it on.

#include <cstdint>
#include <string>

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

// Which MQTT pages register (settings page, Custom pages). A page needs a
// broker and at least one topic; page 1 also counts its layout topic, so an
// orb set up by layout topic alone keeps its page. Page 2 either shares page
// 1's broker (one connection for both) or names its own (a second one).
struct MqttPlan {
    bool page1;
    bool page2;
    bool page2OwnBroker; // page 2 registers on a connection of its own
};

inline bool mqttAnyTopic(const std::string (&topics)[5]) {
    for (const auto &t : topics) {
        if (!t.empty()) {
            return true;
        }
    }
    return false;
}

inline MqttPlan mqttPlan(const std::string &host1, const std::string (&topics1)[5], const std::string &layout, bool page2SameBroker,
                         const std::string &host2, const std::string (&topics2)[5]) {
    MqttPlan p{};
    p.page1 = !host1.empty() && (mqttAnyTopic(topics1) || !layout.empty());
    p.page2 = !(page2SameBroker ? host1 : host2).empty() && mqttAnyTopic(topics2);
    p.page2OwnBroker = p.page2 && !page2SameBroker;
    return p;
}
