#ifndef CLOCKWIDGET_H
#define CLOCKWIDGET_H

#include "GlobalTime.h"
#include "Widget.h"
#include <TJpg_Decoder.h>

#ifndef USE_CLOCK_NIXIE
    #define USE_CLOCK_NIXIE true
#endif

#ifndef USE_CLOCK_CUSTOM
    #define USE_CLOCK_CUSTOM false
#endif

#ifndef USE_CLOCK_VFD
    #define USE_CLOCK_VFD true
#endif

#ifndef USE_CLOCK_DOTMATRIX
    #define USE_CLOCK_DOTMATRIX true
#endif

#ifndef USE_CLOCK_STEAMPUNK
    #define USE_CLOCK_STEAMPUNK true
#endif

// Optional blank images for empty digit positions (e.g., leading space in "9:00")
// Set to true and add the blank.jpg image to use a separate blank instead of colon_off
#ifndef USE_CLOCK_VFD_BLANK
    #define USE_CLOCK_VFD_BLANK true
#endif

#ifndef USE_CLOCK_STEAMPUNK_BLANK
    #define USE_CLOCK_STEAMPUNK_BLANK true
#endif

#if USE_CLOCK_NIXIE
    #include "nixie.h"
#endif

#if USE_CLOCK_CUSTOM
    #include "clock-custom.h"
#endif

#if USE_CLOCK_VFD
    #include "vfd.h"
#endif

#if USE_CLOCK_DOTMATRIX
    #include "dotmatrix.h"
#endif

#if USE_CLOCK_STEAMPUNK
    #include "steampunk.h"
#endif

#ifndef CLOCK_FONT
    #define CLOCK_FONT DSEG7
#endif

#ifndef CLOCK_FONT_SIZE
    #define CLOCK_FONT_SIZE 200
#endif

// Some fonts are not really centered, use this to compensate
#ifndef CLOCK_OFFSET_X_DIGITS
    #define CLOCK_OFFSET_X_DIGITS -10
#endif

#ifndef CLOCK_OFFSET_X_COLON
    #define CLOCK_OFFSET_X_COLON 0
#endif

// Not all digits in DSEG7/14 are aligned identically, therefore we need to offset them
#ifndef CLOCK_DIGITS_OFFSET
    #define CLOCK_DIGITS_OFFSET \
        {                       \
            {0, 0}, {1, -5}, {0, 0}, {1, 0}, {0, -5}, {0, 0}, {0, 0}, {0, -5}, {0, 0}, {0, 0}}
// Use zero offset for other fonts (or adjust)
// #define CLOCK_DIGITS_OFFSET { {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} }
#endif

#ifndef CLOCK_COLOR
    #define CLOCK_COLOR FOREGROUND_COLOR
#endif

#ifndef CLOCK_SHADOW_COLOR
    #define CLOCK_SHADOW_COLOR BG_COLOR
#endif

#ifndef CLOCK_SHADOWING
    #define CLOCK_SHADOWING SHADOWING
#endif

#ifndef DEFAULT_CLOCK
    #define DEFAULT_CLOCK ClockType::NORMAL
#endif

struct DigitOffset {
    int x;
    int y;
};

enum class ClockType {
    NORMAL,
    NIXIE,
    CUSTOM,
    VFD,
    DOTMATRIX,
    STEAMPUNK
};

class ClockWidget : public Widget {
public:
    ClockWidget(ScreenManager &manager);
    ~ClockWidget() override;
    void setup() override;
    void update(bool force = false) override;
    void draw(bool force = false) override;
    void middleTap() override;
    String getName() override;
    uint8_t dataOrbs() override { return 0; } // keeps time without WiFi: no data to mark
    // The red ring and "No WiFi" of "Clock only when WiFi is out". The
    // center orb carries both, so while they are on nothing may repaint
    // that orb whole or draw in the ring's band: see draw().
    uint8_t clockOnlyOrbs() override { return 0x1F; }
    void setClockOnly(bool on) override { m_ringed = on; }

private:
    void displayDigit(int displayIndex, const String &lastDigit, const String &digit, uint32_t color, bool shadowing);
    void displayDigit(int displayIndex, const String &lastDigit, const String &digit, uint32_t color);
    void displaySeconds(int displayIndex, int seconds, int color);
    void displayAmPm(String &amPm, uint32_t color);
    DigitOffset getOffsetForDigit(const String &digit);
    void displayImage(int displayIndex, const String &digit);
    void displayNixie(int displayIndex, const String &digit);
    void displayCustom(int displayIndex, const String &digit);
    void displayVfd(int displayIndex, const String &digit);
    void displayDotmatrix(int displayIndex, const String &digit);
    void displaySteampunk(int displayIndex, const String &digit);
    void changeClockType();
    void drawNoTime();

    // No real time yet (GlobalTime::timeValid): dashes and "No time", never a
    // made-up time (Scott on the rig, 2026-11-18)
    bool m_noTime = false;
    bool m_noTimeChanged = true; // the first draw paints whichever face is right

    ClockType m_type = DEFAULT_CLOCK; // replaced from Settings in the constructor
    bool m_ringed = false; // the red ring is on (setClockOnly)
    bool pictureFace() const { return m_type != ClockType::NORMAL; }
    bool m_tickDrawn = false;
    uint32_t m_color = CLOCK_COLOR;
    uint32_t m_shadow = CLOCK_SHADOW_COLOR;

    time_t m_unixEpoch;
    int m_timeZoneOffset;

    // Delays for setting how often certain screens/functions are refreshed/checked. These include both the frequency which they need to be checked and a varibale to store the last checked value.
    unsigned long m_secondTimer = 2000; // This time is used to refressh/check the clock every second.
    unsigned long m_secondTimerPrev = 0;

    int m_minuteSingle;
    int m_hourSingle;
    int m_secondSingle;

    int m_lastMinuteSingle{-1};
    int m_lastHourSingle{-1};
    int m_lastSecondSingle{-1};

    // Digits
    String m_display1Digit;
    String m_lastDisplay1Digit{""};
    String m_display2Digit;
    String m_lastDisplay2Digit{""};
    // Display 3 is :
    String m_display4Digit;
    String m_lastDisplay4Digit{""};
    String m_display5Digit;
    String m_lastDisplay5Digit{""};

    String m_amPm;
    String m_lastAmPm{""};

    DigitOffset m_digitOffsets[10] = CLOCK_DIGITS_OFFSET;
};
#endif // CLOCKWIDGET_H
