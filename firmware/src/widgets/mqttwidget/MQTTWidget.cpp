#include "MQTTWidget.h"

MQTTWidget::MQTTWidget(ScreenManager &manager, MqttFeed &feed, const std::string &name, const std::string (&topics)[5],
                       const std::string (&labels)[5], const std::string (&units)[5], const std::string &setupTopic)
    : Widget(manager), m_feed(feed), m_name(name.c_str()), m_setupTopic(setupTopic.c_str()) {
    // One orb per settings row with a topic; the others stay blank
    for (int i = 0; i < 5; i++) {
        if (topics[i].empty()) {
            continue;
        }
        OrbConfig c{}; // value-initialized: the layout-topic fields stay zero
        c.orbid = i;
        c.topicSrc = topics[i].c_str();
        c.orbdesc = labels[i].c_str();
        c.orbvalunit = units[i].c_str();
        c.orbBgColor = TFT_BLACK;
        c.orbTextColor = TFT_WHITE;
        c.autoLayout = true;
        orbConfigs.push_back(c);
    }
    feed.addPage(this);
}

MqttFeed &MQTTWidget::feed() {
    return m_feed;
}

// Helper function to map color strings to uint16_t color values
uint16_t MQTTWidget::getColorFromString(const String &colorStr) {
    if (colorStr.equalsIgnoreCase("TFT_BLACK")) {
        return TFT_BLACK;
    } else if (colorStr.equalsIgnoreCase("TFT_NAVY")) {
        return TFT_NAVY;
    } else if (colorStr.equalsIgnoreCase("TFT_DARKGREEN")) {
        return TFT_DARKGREEN;
    } else if (colorStr.equalsIgnoreCase("TFT_DARKCYAN")) {
        return TFT_DARKCYAN;
    } else if (colorStr.equalsIgnoreCase("TFT_MAROON")) {
        return TFT_MAROON;
    } else if (colorStr.equalsIgnoreCase("TFT_PURPLE")) {
        return TFT_PURPLE;
    } else if (colorStr.equalsIgnoreCase("TFT_OLIVE")) {
        return TFT_OLIVE;
    } else if (colorStr.equalsIgnoreCase("TFT_LIGHTGREY")) {
        return TFT_LIGHTGREY;
    } else if (colorStr.equalsIgnoreCase("TFT_DARKGREY")) {
        return TFT_DARKGREY;
    } else if (colorStr.equalsIgnoreCase("TFT_BLUE")) {
        return TFT_BLUE;
    } else if (colorStr.equalsIgnoreCase("TFT_GREEN")) {
        return TFT_GREEN;
    } else if (colorStr.equalsIgnoreCase("TFT_CYAN")) {
        return TFT_CYAN;
    } else if (colorStr.equalsIgnoreCase("TFT_RED")) {
        return TFT_RED;
    } else if (colorStr.equalsIgnoreCase("TFT_MAGENTA")) {
        return TFT_MAGENTA;
    } else if (colorStr.equalsIgnoreCase("TFT_YELLOW")) {
        return TFT_YELLOW;
    } else if (colorStr.equalsIgnoreCase("TFT_WHITE")) {
        return TFT_WHITE;
    } else if (colorStr.equalsIgnoreCase("TFT_ORANGE")) {
        return TFT_ORANGE;
    } else if (colorStr.equalsIgnoreCase("TFT_GREENYELLOW")) {
        return TFT_GREENYELLOW;
    } else if (colorStr.equalsIgnoreCase("TFT_PINK")) {
        return TFT_PINK;
    } else if (colorStr.equalsIgnoreCase("TFT_BROWN")) {
        return TFT_BROWN;
    } else if (colorStr.equalsIgnoreCase("TFT_GOLD")) {
        return TFT_GOLD;
    } else if (colorStr.equalsIgnoreCase("TFT_SILVER")) {
        return TFT_SILVER;
    } else if (colorStr.equalsIgnoreCase("TFT_SKYBLUE")) {
        return TFT_SKYBLUE;
    } else if (colorStr.equalsIgnoreCase("TFT_VIOLET")) {
        return TFT_VIOLET;
    }
    // Add more color mappings as needed

    return TFT_BLACK; // Default color if unknown
}

// Connecting is the feed's job: setup() runs on every page switch and must not block
void MQTTWidget::setup() {}

void MQTTWidget::update(bool force) {
    m_feed.service();
}

void MQTTWidget::subscribeAll() {
    if (m_setupTopic.length()) {
        m_feed.subscribe(m_setupTopic);
    }
    subscribeToOrbs();
}

// Draw method: only the current page's draw() is called, so this is the only place orbs are painted
void MQTTWidget::draw(bool force) {
    m_manager.setFont(DEFAULT_FONT);
    for (int i = 0; i < 5; i++) {
        OrbConfig *orb = nullptr;
        for (auto &o : orbConfigs) {
            if (o.orbid == i) {
                orb = &o;
                break;
            }
        }
        if (orb ? (force || orb->dirty) : force) {
            drawOrb(i);
        }
    }
}

// ChangeMode method
void MQTTWidget::changeMode() {
    // Implement mode changes if applicable
    draw(true);
}

// The orb's value from a message: the whole payload, or orb.jsonField from it
bool MQTTWidget::extractValue(const OrbConfig &orb, const String &message, String &out) {
    if (orb.jsonField.length() == 0) {
        out = message;
        return true;
    }
    JsonDocument dataDoc;
    if (deserializeJson(dataDoc, message)) {
        return false;
    }
    JsonVariant fieldValue = dataDoc.as<JsonVariant>();
    char path[100];
    strncpy(path, orb.jsonField.c_str(), sizeof(path) - 1);
    path[sizeof(path) - 1] = '\0'; // upstream strcpy could overrun
    char *token = strtok(path, ".");
    while (token != nullptr && !fieldValue.isNull()) {
        char *arrayBracket = strchr(token, '[');
        if (arrayBracket) {
            *arrayBracket = '\0';
            fieldValue = fieldValue[token];
            int index = atoi(arrayBracket + 1);
            if (!fieldValue.is<JsonArray>()) {
                return false;
            }
            fieldValue = fieldValue[index];
        } else {
            fieldValue = fieldValue[token];
        }
        token = strtok(nullptr, ".");
    }
    if (fieldValue.isNull()) {
        return false;
    }
    if (fieldValue.is<float>()) {
        out = String(fieldValue.as<float>(), 3);
    } else if (fieldValue.is<int>()) {
        out = String(fieldValue.as<int>());
    } else if (fieldValue.is<const char *>()) {
        out = String(fieldValue.as<const char *>());
    } else {
        out = fieldValue.as<String>();
    }
    return true;
}

// Marks orbs dirty; never draws (loop() runs this while other pages may be on screen)
bool MQTTWidget::onMessage(const String &receivedTopic, const String &message) {
    if (m_setupTopic.length() && receivedTopic.equals(m_setupTopic)) {
        handleSetupMessage(message);
        return false;
    }
    bool value = false;
    // Every orb on this topic (two rows may show the same value)
    for (auto &orb : orbConfigs) {
        if (!orb.topicSrc.equals(receivedTopic)) {
            continue;
        }
        String v;
        if (!extractValue(orb, message, v)) {
            Serial.println("MQTT: no value for orb " + String(orb.orbid) + " in " + receivedTopic);
            continue;
        }
        value = true;
        if (orb.value != v) {
            orb.value = v;
            orb.dirty = true; // drawn by draw(), only while this page is on screen
        }
    }
    return value;
}

// Handle setup message to configure orbs
void MQTTWidget::handleSetupMessage(const String &message) {
    //    Serial.println("Handling setup message...");

    // Parse JSON configuration
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);

    if (error) {
        Serial.print("Failed to parse setup JSON: ");
        Serial.println(error.c_str());
        return;
    }

    // Clear existing configurations and data
    orbConfigs.clear();

    // Parse "orbs" array
    JsonArray orbs = doc["orbs"].as<JsonArray>();
    for (JsonObject orbObj : orbs) {
        OrbConfig config;
        config.orbid = orbObj["orbid"].as<int>();
        config.topicSrc = orbObj["topicsrc"].as<String>();
        config.orbvalunit = orbObj["orbvalunit"].as<String>();
        config.orbdesc = orbObj["orbdesc"].as<String>();
        config.xpostxt = orbObj["xpostxt"].as<int>();
        config.ypostxt = orbObj["ypostxt"].as<int>();
        config.xposval = orbObj["xposval"].as<int>();
        config.yposval = orbObj["yposval"].as<int>();
        config.orbsize = orbObj["orbsize"].as<int>();
        String bgColorStr = orbObj["orb-bg"].as<String>();
        String textColorStr = orbObj["orb-textcol"].as<String>();

        // Parse "jsonfield" (optional)
        if (orbObj["jsonfield"].is<String>()) {
            config.jsonField = orbObj["jsonfield"].as<String>();
        } else {
            config.jsonField = ""; // Default to empty if not provided
        }

        // Convert color strings to actual color values using helper function
        config.orbBgColor = getColorFromString(bgColorStr);
        config.orbTextColor = getColorFromString(textColorStr);

        orbConfigs.push_back(config);
        Serial.println("Configured Orb: " + String(config.orbid) + " -> " + config.orbdesc);
    }

    // Subscribe to all configured topics
    subscribeToOrbs();

    // Replaces the settings rows until restart, as upstream. Drawn when the
    // page is next on screen, not from here.
    for (auto &o : orbConfigs) {
        o.dirty = true;
    }
}

// Subscribe to all orb topics
void MQTTWidget::subscribeToOrbs() {
    for (const auto &orb : orbConfigs) {
        m_feed.subscribe(orb.topicSrc);
    }
}

void MQTTWidget::drawOrb(int orbid) {
    m_manager.selectScreen(orbid);
    OrbConfig *orb = nullptr;
    for (auto &o : orbConfigs) {
        if (o.orbid == orbid) {
            orb = &o;
            break;
        }
    }
    if (orb == nullptr) {
        m_manager.fillScreen(TFT_BLACK); // a row with no topic stays blank
        return;
    }
    orb->dirty = false;
    m_manager.fillScreen(orb->orbBgColor);
    m_manager.setFontColor(orb->orbTextColor, orb->orbBgColor);
    if (orb->autoLayout) {
        // Label on top, the value as large as fits in the middle, unit below.
        // A hyphen until the first value arrives (the font has no en dash: it drew a box).
        if (orb->orbdesc.length()) {
            m_manager.drawString(orb->orbdesc, ScreenCenterX, 62, 22, Align::MiddleCenter);
        }
        String v = orb->value.length() ? orb->value : String("-");
        m_manager.drawFittedString(v, ScreenCenterX, ScreenCenterY, 190, 80, Align::MiddleCenter);
        if (orb->orbvalunit.length()) {
            m_manager.drawString(orb->orbvalunit, ScreenCenterX, 180, 22, Align::MiddleCenter);
        }
        return;
    }
    // Layout-topic orbs: upstream positions and size
    m_manager.drawString(orb->orbdesc, orb->xpostxt, orb->ypostxt, orb->orbsize, Align::MiddleCenter);
    m_manager.drawString(orb->value + orb->orbvalunit, orb->xposval, orb->yposval, orb->orbsize, Align::MiddleCenter);
}

String MQTTWidget::getName() {
    return m_name;
}

void MQTTWidget::buttonPressed(uint8_t buttonId, ButtonState state) {
    if (buttonId == BUTTON_OK && state == BTN_SHORT)
        changeMode();
}
