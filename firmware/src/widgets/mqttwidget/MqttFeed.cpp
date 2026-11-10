#include "MqttFeed.h"
#include "MQTTWidget.h"

MqttStatus MqttFeed::s_status[2];

MqttFeed::MqttFeed(int slot, const std::string &host, int port, const std::string &user, const std::string &pass)
    : m_slot(slot), m_host(host.c_str()), m_port((uint16_t)port), m_user(user.c_str()), m_pass(pass.c_str()), m_client(m_wifiClient) {
    m_client.setServer(m_host.c_str(), m_port);
    m_client.setBufferSize(2048);
    m_client.setSocketTimeout(MQTT_SOCKET_TIMEOUT_S);
    m_client.setCallback([this](char *topic, uint8_t *payload, unsigned int length) { callback(topic, payload, length); });
}

void MqttFeed::addPage(MQTTWidget *page) {
    m_pages.push_back(page);
}

const MqttStatus &MqttFeed::status(int slot) {
    return s_status[slot];
}

void MqttFeed::setWanted(bool wanted) {
    m_wanted = wanted;
}

bool MqttFeed::wanted() const {
    return m_wanted;
}

bool MqttFeed::subscribe(const String &topic) {
    bool ok = m_client.subscribe(topic.c_str());
    Serial.println((ok ? "Subscribed to topic: " : "Failed to subscribe to topic: ") + topic);
    return ok;
}

void MqttFeed::service() {
    if (m_client.connected()) {
        m_client.loop();
        return;
    }
    MqttStatus &st = s_status[m_slot];
    if (m_wasConnected) { // dropped since the last pass
        m_wasConnected = false;
        st.connected = false;
        st.reason = mqttStateText(m_client.state());
    }
    uint32_t now = millis();
    if (!mqttShouldAttempt(m_attempted, now, m_lastAttempt)) {
        return;
    }
    m_attempted = true;
    m_lastAttempt = now;
    tryConnect();
}

// One attempt. Blocks for at most the TCP connect plus MQTT_SOCKET_TIMEOUT_S;
// test 5 in docs/CUSTOM-PAGES.md records the measured time (attemptMs).
void MqttFeed::tryConnect() {
    // Stable, so a reconnect replaces the old session. The second connection
    // gets its own ID: one ID twice on one broker would knock the other off.
    String clientId = "info-orbs-" + String((uint32_t)ESP.getEfuseMac(), HEX) + (m_slot ? "-2" : "");
    MqttStatus &st = s_status[m_slot];
    uint32_t start = millis();
    bool ok = m_user.length() ? m_client.connect(clientId.c_str(), m_user.c_str(), m_pass.length() ? m_pass.c_str() : nullptr)
                              : m_client.connect(clientId.c_str());
    st.attempted = true;
    st.at = millis();
    st.attemptMs = st.at - start;
    st.connected = ok;
    st.reason = mqttStateText(m_client.state());
    Serial.printf("MQTT %s: %s (%u ms)\n", m_host.c_str(), st.reason.c_str(), (unsigned)st.attemptMs);
    if (!ok) {
        return;
    }
    m_wasConnected = true;
    for (MQTTWidget *page : m_pages) {
        page->subscribeAll(); // its rows, and after a reconnect whatever the layout topic set
    }
}

// Runs from loop() while other pages may be on screen: pages only mark orbs dirty here
void MqttFeed::callback(char *topic, byte *payload, unsigned int length) {
    String message;
    message.reserve(length);
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    String receivedTopic(topic);
    bool value = false;
    for (MQTTWidget *page : m_pages) {
        value = page->onMessage(receivedTopic, message) || value;
    }
    if (value) {
        s_status[m_slot].lastValueAt = millis();
    }
}
