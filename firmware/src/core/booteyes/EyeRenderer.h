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

  private:
    uint16_t nightKingIris(int32_t irisX, int32_t irisY, uint32_t scleraX, uint32_t scleraY) const;
    TFT_eSPI &m_tft;
    uint8_t m_uThreshold[2] = {128, 128}; // upper-lid tracking, filtered per eye
    uint16_t m_line[240];
    // Lid body by [brushing step][margin], RGB565; flat styles fill every step alike
    uint16_t m_body[4][256];
    uint16_t m_lash;
    bool m_strands = true;
    int m_style = -1;
    uint16_t m_irisRamp[256]; // by luma; unused while m_irisNatural
    bool m_irisNatural = true;
    // Night King iris (IrisPalette.h nightking::): sclera ramp and fixed colors
    bool m_nightKing = false;
    uint16_t m_scleraRamp[256]; // by luma; only while m_nightKing
    uint16_t m_nkPupil = 0, m_nkSpeck = 0, m_nkRing = 0;
    int m_irisLo = 0, m_irisHi = 0; // the iris texture's luma range, measured once
    int m_scleraLo = 0, m_scleraHi = 0; // the sclera texture's, likewise
};
