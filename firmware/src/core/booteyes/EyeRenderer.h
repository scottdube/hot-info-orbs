#pragma once
// Draws one "Uncanny Eyes" eye (Phil Burgess / Adafruit, MIT; via TFT_eSPI's
// Animated_Eyes_2) pixel-doubled to fill a 240x240 orb, with skin-colored lids
// and lashes. DMA stays off: on this board it gave all-black screens.
#include "WakeScript.h"
#include <TFT_eSPI.h>

class EyeRenderer {
  public:
    explicit EyeRenderer(TFT_eSPI &tft);
    // side 0 = left eye (orb 2), 1 = right eye (orb 4). The caller has
    // selected the panel. Returns false if *abort became true mid-frame (the
    // frame is then left part-drawn; the caller blanks the orb).
    bool draw(uint8_t side, const EyePose &pose, const volatile bool *abort);

  private:
    TFT_eSPI &m_tft;
    uint8_t m_uThreshold[2] = {128, 128}; // upper-lid tracking, filtered per eye
    uint16_t m_line[240];
    uint16_t m_skin, m_shadow, m_lash;
};
