#pragma once
// Host stub: a 240x240 frame buffer that records what EyeRenderer pushes
#include <cstdint>
class TFT_eSPI {
  public:
    uint16_t fb[240 * 240] = {};
    int n = 0;
    bool swap = true;
    bool getSwapBytes() { return swap; }
    void setSwapBytes(bool s) { swap = s; }
    void startWrite() {}
    void endWrite() {}
    void setAddrWindow(int, int, int, int) { n = 0; }
    void pushPixels(const uint16_t *p, uint32_t len) {
        for (uint32_t i = 0; i < len && n < 240 * 240; i++) {
            fb[n++] = uint16_t(p[i] >> 8 | p[i] << 8); // undo the renderer's byte swap
        }
    }
};
