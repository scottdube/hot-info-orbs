// Host preview of the boot eyes: compiles the real EyeRenderer.cpp and
// NoseRenderer.cpp against stubs and writes orbs 2, 3 and 4 (eye, nose, eye)
// side by side as a PPM, so a look can be judged before flashing an orb.
// Build and run with tools/eye_preview/preview.sh.
//   preview <lidIndex> <irisIndex> <lashes 0|1> <open 0..256> <gazeX> <gazeY> <out.ppm>
#include "EyeRenderer.h"
#include "NoseRenderer.h"
#include <cstdio>
#include <cstdlib>

int main(int argc, char **argv) {
    if (argc < 8) {
        fprintf(stderr, "usage: preview lid iris lashes open gazeX gazeY out.ppm\n");
        return 2;
    }
    TFT_eSPI tft;
    EyeRenderer r(tft);
    r.setStyle(atoi(argv[1]));
    r.setIris(atoi(argv[2]));
    r.setLashes(atoi(argv[3]) != 0);
    EyePose pose{uint16_t(atoi(argv[4])), int16_t(atoi(argv[5])), int16_t(atoi(argv[6])), true};
    static uint16_t orb[3][240 * 240]; // left eye, nose, right eye
    for (int side = 0; side < 2; side++) {
        for (int k = 0; k < 8; k++) { // let the filtered lid tracking settle
            r.draw(side, pose, nullptr);
        }
        for (int i = 0; i < 240 * 240; i++) orb[side * 2][i] = tft.fb[i];
    }
    nose::draw(tft, atoi(argv[1]));
    for (int i = 0; i < 240 * 240; i++) orb[1][i] = tft.fb[i];
    FILE *f = fopen(argv[7], "wb");
    const int gap = 20, pitch = 240 + gap, width = 3 * 240 + 2 * gap;
    fprintf(f, "P6 %d 240 255\n", width);
    for (int y = 0; y < 240; y++) {
        for (int x = 0; x < width; x++) {
            const int n = x / pitch, ox = x % pitch; // which orb, and where on it
            // round panel: outside the circle is dark grey
            const int cx = ox - 120, cy = y - 120;
            unsigned char c[3] = {40, 40, 40};
            if (ox < 240 && cx * cx + cy * cy <= 120 * 120) {
                const uint16_t p = orb[n][y * 240 + ox];
                c[0] = uint8_t((p >> 8) & 0xF8), c[1] = uint8_t((p >> 3) & 0xFC), c[2] = uint8_t((p << 3) & 0xF8);
            }
            fwrite(c, 1, 3, f);
        }
    }
    fclose(f);
    return 0;
}
