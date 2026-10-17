#ifndef MQTT_WIDGET_H
#define MQTT_WIDGET_H

#include "MqttPolicy.h"
#include "Settings.h"
#include "Utils.h"
#include "Widget.h"
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <map>
#include <vector>

// Structure to hold individual orb configurations
struct OrbConfig {
    int orbid; // Orb identifier
    String orbdesc; // Orb description/title
    uint16_t orbBgColor; // Background color of the orb
    uint16_t orbTextColor; // Text color within the orb
    String topicSrc; // MQTT topic source for data
    int xpostxt; // x axis label
    int ypostxt; // y axis label
    int xposval; // x axis value
    int yposval; // y axis value
    String orbvalunit; // value unit
    int orbsize; // font size
    String jsonField; // JSON field to extract
    std::map<String, String> lastValuesMap; // Store last values for fields
    bool autoLayout = false; // a settings-page row: label / value / unit, value fitted to the orb
    String value; // latest value shown
    bool dirty = true; // needs drawing the next time this page draws
};

// Connection state for the settings page's status line
struct MqttStatus {
    bool attempted = false;
    bool connected = false;
    String reason; // mqttStateText()
    uint32_t at = 0; // millis() of the last attempt
    uint32_t attemptMs = 0; // how long that attempt blocked
    uint32_t lastValueAt = 0; // millis() of the newest value, 0 = none yet
};

class MQTTWidget : public Widget {
public:
    MQTTWidget(ScreenManager &manager, const SettingsValues &s);
    void setup() override;
    void update(bool force = false) override;
    void draw(bool force = false) override;
    void changeMode();
    void buttonPressed(uint8_t buttonId, ButtonState state) override;
    String getName() override;
    void service(); // keep connected and process messages; call from loop()
    static const MqttStatus &status();

private:
    // MQTT-related members
    String mqttHost; // MQTT broker host
    uint16_t mqttPort; // MQTT broker port
    WiFiClient wifiClient; // Wi-Fi client for MQTT
    PubSubClient mqttClient; // MQTT client

    // Configuration from setup topic
    std::vector<OrbConfig> orbConfigs; // Vector of orb configurations

    // Static callback proxy
    static void staticCallback(char *topic, byte *payload, unsigned int length);

    // Static instance pointer
    static MQTTWidget *instance;

    // Helper functions
    void tryConnect(); // One connect attempt
    void callback(char *topic, byte *payload, unsigned int length); // MQTT message callback
    void handleSetupMessage(const String &message); // Process setup JSON
    void subscribeToOrbs(); // Subscribe to all configured orb topics
    uint16_t getColorFromString(const String &colorStr); // Convert color string to uint16_t
    void drawOrb(int orbid); // Draw a single orb based on orbid

    String m_name, m_user, m_pass, m_setupTopic;
    bool m_attempted = false;
    uint32_t m_lastAttempt = 0;
    bool m_wasConnected = false;
    static MqttStatus s_status;
    bool extractValue(const OrbConfig &orb, const String &message, String &out);
};

#endif // MQTT_WIDGET_H
