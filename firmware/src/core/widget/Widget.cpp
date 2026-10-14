#include "Widget.h"

Widget::Widget(ScreenManager &manager) : m_manager(manager) {}

// BUSY_ON / BUSY_OFF come from config.h: the XIAO's on-board LED is active-low.
// The fallback keeps an older config.h (external LED, driven high) building.
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
