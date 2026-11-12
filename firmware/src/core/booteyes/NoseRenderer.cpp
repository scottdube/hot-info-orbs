#ifdef BOOT_EYES
    #include "NoseRenderer.h"
    #include "NoseShape.h"
    #include <Arduino.h>

// The whole panel in one window, a line at a time like EyeRenderer: no frame
// buffer (115 KB), and the black around the nose clears whatever was there.
void nose::draw(TFT_eSPI &tft, int lidStyle) {
    const lids::Style &s = style(lidStyle);
    uint16_t line[240];
    const bool swap = tft.getSwapBytes();
    tft.setSwapBytes(false); // bytes are swapped by hand below, as in EyeRenderer
    tft.startWrite();
    tft.setAddrWindow(0, 0, 240, 240);
    for (int y = 0; y < 240; y++) {
        for (int x = 0; x < 240; x++) {
            const uint16_t p = lids::to565(pixel(s, x, y));
            line[x] = uint16_t(p >> 8 | p << 8);
        }
        tft.pushPixels(line, 240);
    }
    tft.endWrite();
    tft.setSwapBytes(swap);
}
#endif // BOOT_EYES
