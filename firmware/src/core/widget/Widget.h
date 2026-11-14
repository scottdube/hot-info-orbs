#ifndef WIDGET_H
#define WIDGET_H

#include "Button.h"
#include "ScreenManager.h"
#include "StalePolicy.h"
#include "config_helper.h"

class Widget {
public:
    Widget(ScreenManager &manager);
    virtual ~Widget() = default;
    virtual void setup() = 0;
    virtual void update(bool force = false) = 0;
    virtual void draw(bool force = false) = 0;
    virtual void buttonPressed(uint8_t buttonId, ButtonState state) = 0;
    virtual String getName() = 0;
    virtual String showLabel() { return getName(); } // its Show tick box; the saved key stays getName()
    // Issue #32 (StalePolicy.h). true = this page's refreshes have been
    // failing and what it shows is old. A page that fetches nothing (the
    // clock) never is.
    virtual bool dataStale() { return false; }
    // For the info page on the orbs (InfoPagePolicy.h): the kind of data this
    // page fetches, "weather", and when it last got it. nullptr = it has no
    // line there: it fetches nothing, or MQTT, which has its own line.
    virtual const char *dataKind() { return nullptr; }
    virtual stale::LastGood lastGood() { return {false, 0}; }
    // The orbs that show that data, bit 0 = orb 1. Those get the ring. 0 =
    // the page fetches nothing, and is not told about WiFi either.
    virtual uint8_t dataOrbs() { return 0x1F; }
    // Where "No WiFi" goes on orb 2 of this page
    virtual stale::Plate stalePlate() { return stale::kPlate; }
    void setBusy(bool busy);

protected:
    ScreenManager &m_manager;
};
#endif // WIDGET_H