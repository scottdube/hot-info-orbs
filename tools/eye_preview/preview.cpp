// Host preview of the boot eyes: compiles the real EyeRenderer.cpp against
// stubs and writes both eyes side by side as a PPM, so a look can be judged
// before flashing an orb. Build and run with tools/eye_preview/preview.sh.
//   preview <lidIndex> <irisIndex> <lashes 0|1> <open 0..256> <gazeX> <gazeY> <out.ppm>
#include "EyeRenderer.h"
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
    static uint16_t both[2][240 * 240];
    for (int side = 0; side < 2; side++) {
        for (int k = 0; k < 8; k++) { // let the filtered lid tracking settle
            r.draw(side, pose, nullptr);
        }
        for (int i = 0; i < 240 * 240; i++) both[side][i] = tft.fb[i];
    }
    FILE *f = fopen(argv[7], "wb");
    const int gap = 20;
    fprintf(f, "P6 %d 240 255\n", 480 + gap);
    for (int y = 0; y < 240; y++) {
        for (int x = 0; x < 480 + gap; x++) {
            uint16_t p = 0;
            if (x < 240) p = both[0][y * 240 + x];
            else if (x >= 240 + gap) p = both[1][y * 240 + x - 240 - gap];
            // round panel: outside the circle is dark grey
            const int cx = (x < 240 ? x : x - 240 - gap) - 120, cy = y - 120;
            unsigned char c[3] = {uint8_t((p >> 8) & 0xF8), uint8_t((p >> 3) & 0xFC), uint8_t((p << 3) & 0xF8)};
            if ((x >= 240 && x < 240 + gap) || cx * cx + cy * cy > 120 * 120) c[0] = c[1] = c[2] = 40;
            fwrite(c, 1, 3, f);
        }
    }
    fclose(f);
    return 0;
}
