#ifndef MQTT_FEED_H
#define MQTT_FEED_H

#include "MqttPolicy.h"
#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <string>
#include <vector>

class MQTTWidget;

// Connection state for the settings page's status line
struct MqttStatus {
    bool attempted = false;
    bool connected = false;
    String reason; // mqttStateText()
    uint32_t at = 0; // millis() of the last attempt
    uint32_t attemptMs = 0; // how long that attempt blocked
    uint32_t lastValueAt = 0; // millis() of the newest value, 0 = none yet
};

// One broker connection. Both MQTT pages share one when page 2 uses page 1's
// broker; page 2 gets a second only when it names a broker of its own.
// Messages go to every page on this connection; a page ignores topics it
// does not show.
class MqttFeed {
public:
    // slot 0 = page 1's broker, 1 = page 2's own broker
    MqttFeed(int slot, const std::string &host, int port, const std::string &user, const std::string &pass);
    void addPage(MQTTWidget *page); // MQTTWidget's constructor calls this
    void service(); // keep connected and process messages; call from loop()
    bool subscribe(const String &topic);
    void setWanted(bool wanted); // one of its pages is in the rotation
    bool wanted() const;
    static const MqttStatus &status(int slot);

private:
    void tryConnect(); // One connect attempt
    void callback(char *topic, byte *payload, unsigned int length);

    int m_slot;
    String m_host; // a member, so the pointer given to setServer stays valid
    uint16_t m_port;
    String m_user, m_pass;
    WiFiClient m_wifiClient;
    PubSubClient m_client;
    std::vector<MQTTWidget *> m_pages;
    bool m_attempted = false;
    uint32_t m_lastAttempt = 0;
    bool m_wasConnected = false;
    bool m_wanted = false;
    static MqttStatus s_status[2];
};

#endif // MQTT_FEED_H
