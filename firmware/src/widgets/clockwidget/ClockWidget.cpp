#include "ClockWidget.h"

#include "Settings.h"
#include "StalePolicy.h" // kAmber
#include "config_helper.h"

ClockWidget::ClockWidget(ScreenManager &manager) : Widget(manager) {
    const SettingsValues &s = Settings::get();
    m_color = s.clkcol;
    m_shadow = s.shdcol;
    // A face whose images were not compiled in would draw nothing
    m_type = ClockType::NORMAL;
    if (s.face == (int)ClockType::NIXIE && USE_CLOCK_NIXIE) {
        m_type = ClockType::NIXIE;
    } else if (s.face == (int)ClockType::CUSTOM && USE_CLOCK_CUSTOM) {
        m_type = ClockType::CUSTOM;
    } else if (s.face == (int)ClockType::VFD && USE_CLOCK_VFD) {
        m_type = ClockType::VFD;
    } else if (s.face == (int)ClockType::DOTMATRIX && USE_CLOCK_DOTMATRIX) {
        m_type = ClockType::DOTMATRIX;
    } else if (s.face == (int)ClockType::STEAMPUNK && USE_CLOCK_STEAMPUNK) {
        m_type = ClockType::STEAMPUNK;
    }
}

ClockWidget::~ClockWidget() {
}

void ClockWidget::setup() {
    m_lastDisplay1Digit = "";
    m_lastDisplay2Digit = "";
    m_lastDisplay4Digit = "";
    m_lastDisplay5Digit = "";
}

void ClockWidget::draw(bool force) {
    m_manager.setFont(CLOCK_FONT);
    GlobalTime *time = GlobalTime::getInstance();

    if (m_noTimeChanged) {
        // Into or out of the no-time face: start from black orbs
        m_noTimeChanged = false;
        m_manager.clearAllScreens();
        force = true;
    }
    if (m_noTime) {
        if (force) {
            drawNoTime();
        }
        return;
    }

    if (m_lastDisplay1Digit != m_display1Digit || force) {
        displayDigit(0, m_lastDisplay1Digit, m_display1Digit, m_color);
        m_lastDisplay1Digit = m_display1Digit;
    }
    if (m_lastDisplay2Digit != m_display2Digit || force) {
        displayDigit(1, m_lastDisplay2Digit, m_display2Digit, m_color);
        m_lastDisplay2Digit = m_display2Digit;
    }
    if (m_lastDisplay4Digit != m_display4Digit || force) {
        displayDigit(3, m_lastDisplay4Digit, m_display4Digit, m_color);
        m_lastDisplay4Digit = m_display4Digit;
    }
    if (m_lastDisplay5Digit != m_display5Digit || force) {
        displayDigit(4, m_lastDisplay5Digit, m_display5Digit, m_color);
        m_lastDisplay5Digit = m_display5Digit;
    }

    if (m_secondSingle != m_lastSecondSingle || force) {
        if (m_ringed && pictureFace()) {
            // A picture face blinks its colon by painting the whole orb,
            // which would take the red ring and "No WiFi" off once a second.
            // While they are on, the colon stays lit.
            if (force) {
                displayDigit(2, "", ":", m_color, false);
            }
        } else if (m_secondSingle % 2 == 0) {
            displayDigit(2, "", ":", m_color, false);
        } else {
            displayDigit(2, "", ":", m_shadow, false);
        }
#if SHOW_SECOND_TICKS == true
        // The tick runs in the same band as the red ring of "Clock only when
        // WiFi is out" (110..120 against 113..120), and wiping the old tick
        // would cut a notch in the ring every second. While the ring is on
        // the tick stays off.
        if (m_tickDrawn) {
            displaySeconds(2, m_lastSecondSingle, TFT_BLACK);
            m_tickDrawn = false;
        }
        if (!m_ringed) {
            displaySeconds(2, m_secondSingle, m_color);
            m_tickDrawn = true;
        }
#endif
        m_lastSecondSingle = m_secondSingle;
        if (!time->getFormat24Hour() && Settings::get().ampm && m_type != ClockType::NIXIE) {
            if (m_amPm != m_lastAmPm) {
                // Clear old AM/PM
                displayAmPm(m_lastAmPm, TFT_BLACK);
                m_lastAmPm = m_amPm;
            }
            displayAmPm(m_amPm, m_color);
        }
    }
}

void ClockWidget::displayAmPm(String &amPm, uint32_t color) {
    m_manager.selectScreen(2);
    m_manager.setFontColor(color, TFT_BLACK);
    // Workaround for 12h AM/PM problem
    // The colon is slightly offset and that's a problem because to remove them, we paint over them
    // I think this is related to the TTF cache
    // The problem disappears if we reload the font here
    if (CLOCK_FONT == TTF_Font::DSEG7) {
        // We set a new font anyway
        m_manager.setFont(TTF_Font::DSEG14);
    } else {
        // Force reloading the font
        m_manager.setFont(TTF_Font::NONE);
        m_manager.setFont(CLOCK_FONT);
    }
    m_manager.drawString(amPm, SCREEN_SIZE / 5 * 4, SCREEN_SIZE / 2, 25, Align::MiddleCenter);
}

void ClockWidget::update(bool force) {
    if (millis() - m_secondTimerPrev < m_secondTimer && !force) {
        return;
    }

    GlobalTime *time = GlobalTime::getInstance();

    const bool noTime = !time->timeValid();
    if (noTime != m_noTime) {
        m_noTime = noTime;
        m_noTimeChanged = true;
        force = true; // the digits are worked out afresh when the time arrives
    }
    if (m_noTime) {
        return;
    }

    m_hourSingle = time->getHour();
    m_minuteSingle = time->getMinute();
    m_secondSingle = time->getSecond();
    m_amPm = time->isPM() ? "PM" : "AM";

    if (m_lastHourSingle != m_hourSingle || force) {
        if (m_hourSingle < 10) {
            if (time->getFormat24Hour()) {
                m_display1Digit = "0";
            } else {
                m_display1Digit = " ";
            }
        } else {
            m_display1Digit = int(m_hourSingle / 10);
        }
        m_display2Digit = m_hourSingle % 10;

        m_lastHourSingle = m_hourSingle;
    }

    if (m_lastMinuteSingle != m_minuteSingle || force) {
        String currentMinutePadded = String(m_minuteSingle).length() == 1 ? "0" + String(m_minuteSingle) : String(m_minuteSingle);

        m_display4Digit = currentMinutePadded.substring(0, 1);
        m_display5Digit = currentMinutePadded.substring(1, 2);

        m_lastMinuteSingle = m_minuteSingle;
    }
}

// Every face, Nixie and custom too: their pictures have no dash. The words
// are amber like "No WiFi" on the data pages (red stays the clock's "Clock
// only" ring), and the dashes say the same thing without the color.
void ClockWidget::drawNoTime() {
    const int digitOrbs[] = {0, 1, 3, 4};
    for (int orb : digitOrbs) {
        m_manager.selectScreen(orb);
        m_manager.fillScreen(TFT_BLACK);
        m_manager.setFont(CLOCK_FONT);
        m_manager.setFontColor(m_color, TFT_BLACK);
        m_manager.drawString("-", SCREEN_SIZE / 2 + CLOCK_OFFSET_X_DIGITS, SCREEN_SIZE / 2, CLOCK_FONT_SIZE, Align::MiddleCenter);
    }
    m_manager.selectScreen(2);
    m_manager.fillScreen(TFT_BLACK);
    m_manager.setFont(DEFAULT_FONT);
    m_manager.setFontColor(stale::kAmber, TFT_BLACK);
    m_manager.drawString("No time", SCREEN_SIZE / 2, SCREEN_SIZE / 2 - 14, 30, Align::MiddleCenter);
    m_manager.setFontColor(TFT_WHITE, TFT_BLACK);
    m_manager.drawString("waiting to sync", SCREEN_SIZE / 2, SCREEN_SIZE / 2 + 22, 16, Align::MiddleCenter);
    m_manager.setFont(CLOCK_FONT);
}

void ClockWidget::changeClockType() {
    switch (m_type) {
    case ClockType::NORMAL:
        if (USE_CLOCK_NIXIE) {
            m_type = ClockType::NIXIE;
            break;
        }
        // fall through

    case ClockType::NIXIE:
        if (USE_CLOCK_CUSTOM) {
            m_type = ClockType::CUSTOM;
            break;
        }
        // fall through

    case ClockType::CUSTOM:
        if (USE_CLOCK_VFD) {
            m_type = ClockType::VFD;
            break;
        }
        // fall through

    case ClockType::VFD:
        if (USE_CLOCK_DOTMATRIX) {
            m_type = ClockType::DOTMATRIX;
            break;
        }
        // fall through

    case ClockType::DOTMATRIX:
        if (USE_CLOCK_STEAMPUNK) {
            m_type = ClockType::STEAMPUNK;
            break;
        }
        // fall through

    default:
        m_type = ClockType::NORMAL;
        break;
    }
    m_manager.clearAllScreens();
    draw(true);
}

// Next face, kept across a restart (Settings::saveFaceSoon). 12/24 h is on
// the settings page only since #60: it was the half-second press here.
void ClockWidget::middleTap() {
    changeClockType();
    Settings::saveFaceSoon((int)m_type);
}

DigitOffset ClockWidget::getOffsetForDigit(const String &digit) {
    if (digit.length() > 0) {
        char c = digit.charAt(0);
        if (c >= '0' && c <= '9') {
            // get digit offsets
            return m_digitOffsets[c - '0'];
        }
    }
    // not a valid digit
    return {0, 0};
}

void ClockWidget::displayDigit(int displayIndex, const String &lastDigit, const String &digit, uint32_t color, bool shadowing) {
    if (pictureFace()) {
        if (digit == ":" && color == m_shadow) {
            // Show colon off
            displayImage(displayIndex, " ");
        } else {
            displayImage(displayIndex, digit);
        }
    } else {
        // Normal clock
        int fontSize = CLOCK_FONT_SIZE;
        char c = digit.charAt(0);
        bool isDigit = c >= '0' && c <= '9' || c == ' ';
        int defaultX = SCREEN_SIZE / 2 + (isDigit ? CLOCK_OFFSET_X_DIGITS : CLOCK_OFFSET_X_COLON);
        int defaultY = SCREEN_SIZE / 2;
        DigitOffset digitOffset = getOffsetForDigit(digit);
        DigitOffset lastDigitOffset = getOffsetForDigit(lastDigit);
        m_manager.selectScreen(displayIndex);
        if (shadowing) {
            m_manager.setFontColor(m_shadow, TFT_BLACK);
            if (CLOCK_FONT == DSEG14) {
                // DSEG14 (from DSEGstended) uses # to fill all segments
                m_manager.drawString("#", defaultX, defaultY, fontSize, Align::MiddleCenter);
            } else if (CLOCK_FONT == DSEG7) {
                // DESG7 uses 8 to fill all segments
                m_manager.drawString("8", defaultX, defaultY, fontSize, Align::MiddleCenter);
            } else {
                // Other fonts can't be shadowed
                m_manager.setFontColor(TFT_BLACK, TFT_BLACK);
                m_manager.drawString(lastDigit, defaultX + lastDigitOffset.x, defaultY + lastDigitOffset.y, fontSize, Align::MiddleCenter);
            }
        } else {
            m_manager.setFontColor(TFT_BLACK, TFT_BLACK);
            m_manager.drawString(lastDigit, defaultX + lastDigitOffset.x, defaultY + lastDigitOffset.y, fontSize, Align::MiddleCenter);
        }
        m_manager.setFontColor(color, TFT_BLACK);
        m_manager.drawString(digit, defaultX + digitOffset.x, defaultY + digitOffset.y, fontSize, Align::MiddleCenter);
    }
    if (displayIndex != 2 || pictureFace()) {
        // An orb was painted over, with no fillScreen to say so. A picture
        // covers the whole orb, and the corners of a 200 px digit reach 2 or
        // 3 px into the ring's band (measured from the font: 124 x 200 px,
        // drawn 10 px left of center). Either way a mark on this orb has to
        // be drawn again. Not the drawn colon: it is painted every second,
        // and it reaches neither the band nor "No WiFi" above it (the colon
        // is 107 px tall, so its top is 18 px below the label).
        m_manager.selectScreen(displayIndex); // a picture face may have drawn nothing
        m_manager.notePainted(TFT_BLACK);
    }
}

void ClockWidget::displayDigit(int displayIndex, const String &lastDigit, const String &digit, uint32_t color) {
    displayDigit(displayIndex, lastDigit, digit, color, CLOCK_SHADOWING);
}

void ClockWidget::displaySeconds(int displayIndex, int seconds, int color) {
    if (m_type == ClockType::NIXIE && color == m_color) {
        // Special color (orange) for nixie
        color = 0xfd40;
    }
    m_manager.selectScreen(displayIndex);
    if (seconds < 30) {
        m_manager.drawSmoothArc(SCREEN_SIZE / 2, SCREEN_SIZE / 2, 120, 110, 6 * seconds + 180, 6 * seconds + 180 + 6, color, TFT_BLACK);
    } else {
        m_manager.drawSmoothArc(SCREEN_SIZE / 2, SCREEN_SIZE / 2, 120, 110, 6 * seconds - 180, 6 * seconds - 180 + 6, color, TFT_BLACK);
    }
}

void ClockWidget::displayImage(int displayIndex, const String &digit) {
    switch (m_type) {
    case ClockType::NIXIE:
        displayNixie(displayIndex, digit);
        break;

    case ClockType::CUSTOM:
        displayCustom(displayIndex, digit);
        break;

    case ClockType::VFD:
        displayVfd(displayIndex, digit);
        break;

    case ClockType::DOTMATRIX:
        displayDotmatrix(displayIndex, digit);
        break;

    case ClockType::STEAMPUNK:
        displaySteampunk(displayIndex, digit);
        break;

    default:
        break;
    }
}

void ClockWidget::displayNixie(int displayIndex, const String &digit) {
#if USE_CLOCK_NIXIE
    if (digit.length() != 1) {
        return;
    }
    m_manager.selectScreen(displayIndex);
    TJpgDec.setJpgScale(1);
    switch (digit.charAt(0)) {
    case '0':
        TJpgDec.drawJpg(0, 0, nixie_0_start, nixie_0_end - nixie_0_start);
        break;
    case '1':
        TJpgDec.drawJpg(0, 0, nixie_1_start, nixie_1_end - nixie_1_start);
        break;
    case '2':
        TJpgDec.drawJpg(0, 0, nixie_2_start, nixie_2_end - nixie_2_start);
        break;
    case '3':
        TJpgDec.drawJpg(0, 0, nixie_3_start, nixie_3_end - nixie_3_start);
        break;
    case '4':
        TJpgDec.drawJpg(0, 0, nixie_4_start, nixie_4_end - nixie_4_start);
        break;
    case '5':
        TJpgDec.drawJpg(0, 0, nixie_5_start, nixie_5_end - nixie_5_start);
        break;
    case '6':
        TJpgDec.drawJpg(0, 0, nixie_6_start, nixie_6_end - nixie_6_start);
        break;
    case '7':
        TJpgDec.drawJpg(0, 0, nixie_7_start, nixie_7_end - nixie_7_start);
        break;
    case '8':
        TJpgDec.drawJpg(0, 0, nixie_8_start, nixie_8_end - nixie_8_start);
        break;
    case '9':
        TJpgDec.drawJpg(0, 0, nixie_9_start, nixie_9_end - nixie_9_start);
        break;
    case ' ':
        TJpgDec.drawJpg(0, 0, nixie_colon_off_start, nixie_colon_off_end - nixie_colon_off_start);
        break;
    case ':':
        TJpgDec.drawJpg(0, 0, nixie_colon_on_start, nixie_colon_on_end - nixie_colon_on_start);
        break;
    }
#endif
}

void ClockWidget::displayCustom(int displayIndex, const String &digit) {
#if USE_CLOCK_CUSTOM
    if (digit.length() != 1) {
        return;
    }
    m_manager.selectScreen(displayIndex);
    TJpgDec.setJpgScale(1);
    switch (digit.charAt(0)) {
    case '0':
        TJpgDec.drawJpg(0, 0, clock_custom_0_start, clock_custom_0_end - clock_custom_0_start);
        break;
    case '1':
        TJpgDec.drawJpg(0, 0, clock_custom_1_start, clock_custom_1_end - clock_custom_1_start);
        break;
    case '2':
        TJpgDec.drawJpg(0, 0, clock_custom_2_start, clock_custom_2_end - clock_custom_2_start);
        break;
    case '3':
        TJpgDec.drawJpg(0, 0, clock_custom_3_start, clock_custom_3_end - clock_custom_3_start);
        break;
    case '4':
        TJpgDec.drawJpg(0, 0, clock_custom_4_start, clock_custom_4_end - clock_custom_4_start);
        break;
    case '5':
        TJpgDec.drawJpg(0, 0, clock_custom_5_start, clock_custom_5_end - clock_custom_5_start);
        break;
    case '6':
        TJpgDec.drawJpg(0, 0, clock_custom_6_start, clock_custom_6_end - clock_custom_6_start);
        break;
    case '7':
        TJpgDec.drawJpg(0, 0, clock_custom_7_start, clock_custom_7_end - clock_custom_7_start);
        break;
    case '8':
        TJpgDec.drawJpg(0, 0, clock_custom_8_start, clock_custom_8_end - clock_custom_8_start);
        break;
    case '9':
        TJpgDec.drawJpg(0, 0, clock_custom_9_start, clock_custom_9_end - clock_custom_9_start);
        break;
    case ' ':
        TJpgDec.drawJpg(0, 0, clock_custom_colon_off_start, clock_custom_colon_off_end - clock_custom_colon_off_start);
        break;
    case ':':
        TJpgDec.drawJpg(0, 0, clock_custom_colon_on_start, clock_custom_colon_on_end - clock_custom_colon_on_start);
        break;
    }
#endif
}

void ClockWidget::displayVfd(int displayIndex, const String &digit) {
#if USE_CLOCK_VFD
    if (digit.length() != 1) {
        return;
    }
    m_manager.selectScreen(displayIndex);
    TJpgDec.setJpgScale(1);
    switch (digit.charAt(0)) {
    case '0':
        TJpgDec.drawJpg(0, 0, vfd_0_start, vfd_0_end - vfd_0_start);
        break;
    case '1':
        TJpgDec.drawJpg(0, 0, vfd_1_start, vfd_1_end - vfd_1_start);
        break;
    case '2':
        TJpgDec.drawJpg(0, 0, vfd_2_start, vfd_2_end - vfd_2_start);
        break;
    case '3':
        TJpgDec.drawJpg(0, 0, vfd_3_start, vfd_3_end - vfd_3_start);
        break;
    case '4':
        TJpgDec.drawJpg(0, 0, vfd_4_start, vfd_4_end - vfd_4_start);
        break;
    case '5':
        TJpgDec.drawJpg(0, 0, vfd_5_start, vfd_5_end - vfd_5_start);
        break;
    case '6':
        TJpgDec.drawJpg(0, 0, vfd_6_start, vfd_6_end - vfd_6_start);
        break;
    case '7':
        TJpgDec.drawJpg(0, 0, vfd_7_start, vfd_7_end - vfd_7_start);
        break;
    case '8':
        TJpgDec.drawJpg(0, 0, vfd_8_start, vfd_8_end - vfd_8_start);
        break;
    case '9':
        TJpgDec.drawJpg(0, 0, vfd_9_start, vfd_9_end - vfd_9_start);
        break;
    case ' ':
        if (displayIndex == 2) {
            // Center orb is the colon - show colon_off
            TJpgDec.drawJpg(0, 0, vfd_colon_off_start, vfd_colon_off_end - vfd_colon_off_start);
        } else {
            // Other orbs - show blank for empty digit positions
#if USE_CLOCK_VFD_BLANK
            TJpgDec.drawJpg(0, 0, vfd_blank_start, vfd_blank_end - vfd_blank_start);
#else
            TJpgDec.drawJpg(0, 0, vfd_colon_off_start, vfd_colon_off_end - vfd_colon_off_start);
#endif
        }
        break;
    case ':':
        TJpgDec.drawJpg(0, 0, vfd_colon_on_start, vfd_colon_on_end - vfd_colon_on_start);
        break;
    }
#endif
}

void ClockWidget::displayDotmatrix(int displayIndex, const String &digit) {
#if USE_CLOCK_DOTMATRIX
    if (digit.length() != 1) {
        return;
    }
    m_manager.selectScreen(displayIndex);
    TJpgDec.setJpgScale(1);
    switch (digit.charAt(0)) {
    case '0':
        TJpgDec.drawJpg(0, 0, dotmatrix_0_start, dotmatrix_0_end - dotmatrix_0_start);
        break;
    case '1':
        TJpgDec.drawJpg(0, 0, dotmatrix_1_start, dotmatrix_1_end - dotmatrix_1_start);
        break;
    case '2':
        TJpgDec.drawJpg(0, 0, dotmatrix_2_start, dotmatrix_2_end - dotmatrix_2_start);
        break;
    case '3':
        TJpgDec.drawJpg(0, 0, dotmatrix_3_start, dotmatrix_3_end - dotmatrix_3_start);
        break;
    case '4':
        TJpgDec.drawJpg(0, 0, dotmatrix_4_start, dotmatrix_4_end - dotmatrix_4_start);
        break;
    case '5':
        TJpgDec.drawJpg(0, 0, dotmatrix_5_start, dotmatrix_5_end - dotmatrix_5_start);
        break;
    case '6':
        TJpgDec.drawJpg(0, 0, dotmatrix_6_start, dotmatrix_6_end - dotmatrix_6_start);
        break;
    case '7':
        TJpgDec.drawJpg(0, 0, dotmatrix_7_start, dotmatrix_7_end - dotmatrix_7_start);
        break;
    case '8':
        TJpgDec.drawJpg(0, 0, dotmatrix_8_start, dotmatrix_8_end - dotmatrix_8_start);
        break;
    case '9':
        TJpgDec.drawJpg(0, 0, dotmatrix_9_start, dotmatrix_9_end - dotmatrix_9_start);
        break;
    case ' ':
        TJpgDec.drawJpg(0, 0, dotmatrix_colon_off_start, dotmatrix_colon_off_end - dotmatrix_colon_off_start);
        break;
    case ':':
        TJpgDec.drawJpg(0, 0, dotmatrix_colon_on_start, dotmatrix_colon_on_end - dotmatrix_colon_on_start);
        break;
    }
#endif
}

void ClockWidget::displaySteampunk(int displayIndex, const String &digit) {
#if USE_CLOCK_STEAMPUNK
    if (digit.length() != 1) {
        return;
    }
    m_manager.selectScreen(displayIndex);
    TJpgDec.setJpgScale(1);
    switch (digit.charAt(0)) {
    case '0':
        TJpgDec.drawJpg(0, 0, steampunk_0_start, steampunk_0_end - steampunk_0_start);
        break;
    case '1':
        TJpgDec.drawJpg(0, 0, steampunk_1_start, steampunk_1_end - steampunk_1_start);
        break;
    case '2':
        TJpgDec.drawJpg(0, 0, steampunk_2_start, steampunk_2_end - steampunk_2_start);
        break;
    case '3':
        TJpgDec.drawJpg(0, 0, steampunk_3_start, steampunk_3_end - steampunk_3_start);
        break;
    case '4':
        TJpgDec.drawJpg(0, 0, steampunk_4_start, steampunk_4_end - steampunk_4_start);
        break;
    case '5':
        TJpgDec.drawJpg(0, 0, steampunk_5_start, steampunk_5_end - steampunk_5_start);
        break;
    case '6':
        TJpgDec.drawJpg(0, 0, steampunk_6_start, steampunk_6_end - steampunk_6_start);
        break;
    case '7':
        TJpgDec.drawJpg(0, 0, steampunk_7_start, steampunk_7_end - steampunk_7_start);
        break;
    case '8':
        TJpgDec.drawJpg(0, 0, steampunk_8_start, steampunk_8_end - steampunk_8_start);
        break;
    case '9':
        TJpgDec.drawJpg(0, 0, steampunk_9_start, steampunk_9_end - steampunk_9_start);
        break;
    case ' ':
        if (displayIndex == 2) {
            // Center orb is the colon - show colon_off
            TJpgDec.drawJpg(0, 0, steampunk_colon_off_start, steampunk_colon_off_end - steampunk_colon_off_start);
        } else {
            // Other orbs - show blank for empty digit positions
#if USE_CLOCK_STEAMPUNK_BLANK
            TJpgDec.drawJpg(0, 0, steampunk_blank_start, steampunk_blank_end - steampunk_blank_start);
#else
            TJpgDec.drawJpg(0, 0, steampunk_colon_off_start, steampunk_colon_off_end - steampunk_colon_off_start);
#endif
        }
        break;
    case ':':
        TJpgDec.drawJpg(0, 0, steampunk_colon_on_start, steampunk_colon_on_end - steampunk_colon_on_start);
        break;
    }
#endif
}

String ClockWidget::getName() {
    return "Clock";
}