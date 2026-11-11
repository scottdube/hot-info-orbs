#pragma once
// Draws one "Uncanny Eyes" eye (Phil Burgess / Adafruit, MIT; via TFT_eSPI's
// Animated_Eyes_2) pixel-doubled to fill a 240x240 orb, with lids in one of
// the LidPalette.h styles. DMA stays off: on this board it gave all-black screens.
#include "WakeScript.h"
#include <TFT_eSPI.h>

class EyeRenderer {
  public:
    explicit EyeRenderer(TFT_eSPI &tft);
    // side 0 = left eye (orb 2), 1 = right eye (orb 4). The caller has
    // selected the panel. Returns false if *abort became true mid-frame (the
    // frame is then left part-drawn; the caller blanks the orb).
    bool draw(uint8_t side, const EyePose &pose, const volatile bool *abort);
    // index into lids::kStyles; out of range = lids::kDefault
    void setStyle(int index);
    // index into iriscolor::kIrises; out of range = natural
    void setIris(int index);
    void setLashes(bool on) { m_strands = on; } // strands on the upper lid
    // Turn the next draw's eye inward (toward orb 3) by this many degrees,
    // negative = outward. The eye is drawn as a ball rotated about its
    // vertical axis: every pixel samples the straight-on art where the turned
    // ball puts it, so iris, pupil and veins turn together and the iris
    // squashes toward the far side. The lids stay put. 0 = the waking eyes
    // (gazeX drives them). The crossed eyes (issue #20) set it.
    // down = then rolled down about the horizontal axis, degrees; negative = up.
    // nudge = the whole ball moved toward the nose, source px (panel px / 2).
    void setTurn(float inward, float down, float nudge = 0);

  private:
    TFT_eSPI &m_tft;
    bool m_turned = false;
    float m_turnSin = 0, m_turnCos = 1; // of the inward turn
    float m_downSin = 0, m_downCos = 1; // of the downward roll
    float m_nudge = 0;                  // ball center toward the nose, source px
    uint8_t m_uThreshold[2] = {128, 128}; // upper-lid tracking, filtered per eye
    uint16_t m_line[240];
    // Lid body by [brushing step][margin], RGB565; flat styles fill every step alike
    uint16_t m_body[4][256];
    uint16_t m_lash;
    bool m_strands = true;
    int m_style = -1;
    uint16_t m_irisRamp[256]; // by luma; unused while m_irisNatural
    bool m_irisNatural = true;
    int m_irisLo = 0, m_irisHi = 0; // the iris texture's luma range, measured once
};
