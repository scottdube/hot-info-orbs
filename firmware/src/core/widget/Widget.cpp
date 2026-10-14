#include "Widget.h"

Widget::Widget(ScreenManager &manager) : m_manager(manager) {}

// BUSY_ON / BUSY_OFF come from config.h when the indicator is active-low (the
// XIAO's on-board LED); the SuperMini board drives an external LED high.
#ifndef BUSY_ON
    #define BUSY_ON HIGH
    #define BUSY_OFF LOW
#endif

void Widget::setBusy(bool busy) {
    if (busy) {
        digitalWrite(BUSY_PIN, BUSY_ON);
    } else {
        digitalWrite(BUSY_PIN, BUSY_OFF);
    }
}
