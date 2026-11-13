#ifndef MQTT_WIDGET_H
#define MQTT_WIDGET_H

#include "MqttFeed.h"
#include "MqttText.h"
#include "Settings.h"
#include "Utils.h"
#include "Widget.h"
#include <ArduinoJson.h>
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
    String fitFor; // the text fitSize was measured for
    int fitSize = 0; // largest font size at which fitFor fits this orb (MqttText.h)
};

// One MQTT page: five orbs, one per settings row. The broker connection is
// the MqttFeed it is given, which may be shared with the other MQTT page.
class MQTTWidget : public Widget {
public:
    // topics/labels/units: the page's five settings rows. setupTopic is the
    // advanced layout topic ("" = none), page 1 only. sameSize: every orb's value
    // at one font size, the largest that fits all five. fixedSize: 0 = automatic
    // (sameSize decides), otherwise every value at that size, cut to fit.
    MQTTWidget(ScreenManager &manager, MqttFeed &feed, int page, const std::string &name, const std::string (&topics)[5],
               const std::string (&labels)[5], const std::string (&units)[5], const std::string &setupTopic,
               bool sameSize, int fixedSize);
    void setup() override;
    void update(bool force = false) override;
    void draw(bool force = false) override;
    void changeMode();
    void buttonPressed(uint8_t buttonId, ButtonState state) override;
    String getName() override;
    String showLabel() override;
    MqttFeed &feed();
    bool dataStale() override { return m_feed.stale(); }
    uint8_t dataOrbs() override; // the orbs with a topic

    // Called by the feed
    void subscribeAll(); // layout topic and every orb topic, after each connect
    bool onMessage(const String &topic, const String &message); // true = a value for one of its orbs

private:
    MqttFeed &m_feed;
    std::vector<OrbConfig> orbConfigs; // Vector of orb configurations

    void handleSetupMessage(const String &message); // Process setup JSON
    void subscribeToOrbs(); // Subscribe to all configured orb topics
    uint16_t getColorFromString(const String &colorStr); // Convert color string to uint16_t
    void drawOrb(int orbid); // Draw a single orb based on orbid

    int m_page; // 1 or 2, for its Show label
    bool m_sameSize;
    int m_fixedSize; // 0 = automatic; otherwise the size of every value on the page (mqttCutToFit)
    int m_sharedSize = 0; // the size every orb's value is drawn at when m_sameSize; 0 = none yet
    String valueText(const OrbConfig &orb); // the value, or a hyphen until one arrives
    int bestSize(OrbConfig &orb); // cached in orb.fitFor / fitSize
    void valueBox(const OrbConfig &orb, int &top, int &bottom);
    String m_name, m_setupTopic;
    bool extractValue(const OrbConfig &orb, const String &message, String &out);
};

#endif // MQTT_WIDGET_H
