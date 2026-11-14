#pragma once
// The info page on the orbs themselves: hold the middle button 2 seconds. The
// rules (when it opens and closes, the wording of every line) are in
// InfoPagePolicy.h, where they are tested on a computer; this file reads the
// values off the device and draws them.
#include "InfoPagePolicy.h"
#include "ScreenManager.h"

class InfoPage {
public:
    explicit InfoPage(ScreenManager &manager);

    bool isOpen() const { return m_session.isOpen(); }

    // Every finished button press goes through here first, see infopage::Press
    infopage::Press press(bool okHeldLong);
    void close();

    // Every loop pass while the page is open. Draws when a second has gone by.
    // False on the pass where the minute ran out and the page closed itself.
    // mqttInUse: which of the two brokers has a page in the rotation.
    bool show(bool linkUp, const bool mqttInUse[2]);

private:
    infopage::Facts facts(bool linkUp, const bool mqttInUse[2]) const;
    void drawLine(const infopage::Page &page, int index);

    ScreenManager &m_manager;
    infopage::Session m_session;
    infopage::Page m_shown[infopage::kOrbs]; // what each orb has on it now
};
