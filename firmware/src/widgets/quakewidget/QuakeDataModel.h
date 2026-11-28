#ifndef QUAKE_DATA_MODEL_H
#define QUAKE_DATA_MODEL_H

#include <Arduino.h>

// One earthquake, as the orb needs it. The feed carries more (coordinates,
// status, tsunami); only what a single orb shows is kept.
class QuakeDataModel {
public:
    void setId(const String &id) { m_id = id; }
    String getId() const { return m_id; }
    void setMagnitude(float mag) { m_magnitude = mag; }
    float getMagnitude() const { return m_magnitude; }
    void setPlace(const String &place) { m_place = place; }
    String getPlace() const { return m_place; }
    void setDepthKm(float depth) { m_depthKm = depth; }
    float getDepthKm() const { return m_depthKm; }
    // Event time, UTC seconds since the epoch (the feed gives milliseconds)
    void setEventEpoch(long epoch) { m_eventEpoch = epoch; }
    long getEventEpoch() const { return m_eventEpoch; }
    void setTsunami(bool tsunami) { m_tsunami = tsunami; }
    bool getTsunami() const { return m_tsunami; }
    // true until USGS has reviewed it; shown as provisional
    void setProvisional(bool provisional) { m_provisional = provisional; }
    bool getProvisional() const { return m_provisional; }

    bool isChanged() const { return m_changed; }
    void setChangedStatus(bool changed) { m_changed = changed; }

private:
    String m_id = "";
    float m_magnitude = 0.0f;
    String m_place = "";
    float m_depthKm = 0.0f;
    long m_eventEpoch = 0;
    bool m_tsunami = false;
    bool m_provisional = false;
    bool m_changed = false;
};

#endif // QUAKE_DATA_MODEL_H
