#ifndef TTF_FONTS_H
#define TTF_FONTS_H

#include <Arduino.h>

// All available TTF fonts
enum TTF_Font {
    NONE,
    ROBOTO_REGULAR,
    FINAL_FRONTIER,
    DSEG7,
    DSEG14
};

struct TTF_FontMetric {
    TTF_Font font;
    float scale;
};

// Set scaling factor for each font if necessary
const TTF_FontMetric ttfFontMetrics[] = {{ROBOTO_REGULAR, 1.37}, {FINAL_FRONTIER, 1.5}};

// These symbols are generated from the files specified in platformio.ini under 'board_build.embed_files'
// See https://docs.platformio.org/en/latest/platforms/espressif32.html#embedding-binary-data for more info

extern const byte robotoRegular_start[] asm("_binary_fonts_RobotoRegular_ttf_start");
extern const byte robotoRegular_end[] asm("_binary_fonts_RobotoRegular_ttf_end");

// FinalFrontier (19.8 KB) is left out of the image because no widget uses it.
// To use it: uncomment it in platformio.ini and add -D USE_FINAL_FRONTIER.
#ifdef USE_FINAL_FRONTIER
extern const byte finalFrontier_start[] asm("_binary_fonts_FinalFrontier_ttf_start");
extern const byte finalFrontier_end[] asm("_binary_fonts_FinalFrontier_ttf_end");
#endif

// Choose either Classic or Modern here and remember to also adjust platformio.ini
// *******************************************************************************
// extern const byte dseg7_start[] asm("_binary_fonts_DSEG7ClassicBold_ttf_start");
// extern const byte dseg7_end[] asm("_binary_fonts_DSEG7ClassicBold_ttf_end");
// extern const byte dseg14_start[] asm("_binary_fonts_DSEG14ClassicBold_ttf_start");
// extern const byte dseg14_end[] asm("_binary_fonts_DSEG14ClassicBold_ttf_end");
extern const byte dseg7_start[] asm("_binary_fonts_DSEG7ModernBold_ttf_start");
extern const byte dseg7_end[] asm("_binary_fonts_DSEG7ModernBold_ttf_end");
extern const byte dseg14_start[] asm("_binary_fonts_DSEG14ModernBold_ttf_start");
extern const byte dseg14_end[] asm("_binary_fonts_DSEG14ModernBold_ttf_end");

#endif
