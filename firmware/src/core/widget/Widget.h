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
    // The orbs that show that data, bit 0 = orb 1. Those get the ring.
    virtual uint8_t dataOrbs() { return 0x1F; }
    // Where "No WiFi" goes on orb 2 of this page
    virtual stale::Plate stalePlate() { return stale::kPlate; }
    void setBusy(bool busy);

protected:
    ScreenManager &m_manager;
};
#endif // WIDGET_H