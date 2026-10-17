// Display edge test: how much of each GC9A01 is usable out to the rim.
// Built only by `pio run -e displaytest`; the normal firmware compiles this
// file to nothing. Written for enclosure design (2026-10-17): it answers
// whether there is dead black space between the lit area and the glass edge.
//
// The four patterns run side by side: DS1 FULL, DS2 RINGS, DS3 GRID, DS4 BEYOND,
// DS5 FULL. Any button moves every display on to its next pattern, so four
// presses put each pattern on each display.
//   1 FULL   - every pixel white, corners included. Caliper the lit diameter
//              against the glass and the module outline.
//   2 RINGS  - 2 px white/black bands from r=120 inwards; the outermost white
//              band is r 118-120. Count white bands visible on each side.
//   3 GRID   - 10 px grid about the true center (119.5, 119.5) plus the r=120
//              circle, to see whether the active area is centered in the glass.
//   4 BEYOND - five 2 px white rings, three inside the assumed r=120 edge
//              (r 110-112, 114-116, 118-120) and two outside it (122-124,
//              126-128). Outside r=120 the 240x240 frame only has pixels
//              toward the corners, so the outer two can only appear as arcs
//              on the diagonals. Seeing them means pixels exist under the
//              dark band.
//
// Pin map is the v1.1 SuperMini board, set in platformio.ini [env:displaytest].
#ifdef DISPLAY_TEST

#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <math.h>

static const uint8_t SCREEN_CS[5] = {4, 5, 6, 7, 8};
static const uint8_t BUTTONS[3] = {1, 2, 10}; // active high, INPUT_PULLDOWN
static const int N = 240;
static const float C = 119.5f; // pixel centers run 0..239, so the center falls between pixels

static TFT_eSPI tft;
static uint16_t row[N];
static const int NPAT = 4;
static int offset = 0; // display s shows pattern (s + offset) % NPAT

static void selectScreen(int s) {
    for (int i = 0; i < 5; i++) {
        digitalWrite(SCREEN_CS[i], (s < 0 || i == s) ? LOW : HIGH); // s < 0 selects all
    }
}

static float dist(int x, int y) {
    return hypotf(x - C, y - C);
}

static void drawFull() {
    tft.fillScreen(TFT_WHITE);
}

// Band k counts 2 px steps in from r=120: k=0 (r 118-120) white, k=1 black, ...
static void drawRings() {
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            float d = dist(x, y);
            uint16_t c;
            if (d >= 120.0f) {
                c = TFT_DARKGREY; // corners: off the round glass
            } else if (d >= 100.0f) {
                int k = (int)((120.0f - d) / 2.0f);
                c = (k % 2 == 0) ? TFT_WHITE : TFT_BLACK;
            } else {
                c = TFT_NAVY;
            }
            row[x] = c;
        }
        tft.pushImage(0, y, N, 1, row);
    }
}

static void drawGrid() {
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            // Lines sit on the pixel pairs either side of the center (119|120)
            // and every 10 px out from them, so the grid is symmetric.
            int gx = (x < 120) ? (119 - x) : (x - 120);
            int gy = (y < 120) ? (119 - y) : (y - 120);
            bool center = (gx == 0 || gy == 0);
            bool line = (gx % 10 == 0 || gy % 10 == 0);
            float d = dist(x, y);
            uint16_t c = TFT_BLACK;
            if (line) c = TFT_DARKGREY;
            if (center) c = TFT_WHITE;
            if (d >= 119.0f && d < 120.0f) c = TFT_YELLOW; // the r=120 rim
            row[x] = c;
        }
        tft.pushImage(0, y, N, 1, row);
    }
}

static void drawBeyond() {
    static const float RINGS[5] = {110, 114, 118, 122, 126}; // inner radius of each 2 px white ring
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            float d = dist(x, y);
            uint16_t c = (d < 108.0f) ? TFT_NAVY : TFT_BLACK;
            for (int i = 0; i < 5; i++) {
                if (d >= RINGS[i] && d < RINGS[i] + 2.0f) c = TFT_WHITE;
            }
            row[x] = c;
        }
        tft.pushImage(0, y, N, 1, row);
    }
}

static const char *NAMES[NPAT] = {"FULL", "RINGS", "GRID", "BEYOND"};

static void label(int screen, int pattern) {
    // Dark text on the white pattern, white elsewhere.
    uint16_t fg = (pattern == 0) ? TFT_BLACK : TFT_WHITE;
    uint16_t bg = (pattern == 0) ? TFT_WHITE : (pattern == 2 ? TFT_BLACK : TFT_NAVY);
    tft.setTextColor(fg, bg);
    tft.setTextDatum(MC_DATUM);
    char buf[24];
    snprintf(buf, sizeof buf, "DS%d", screen + 1);
    tft.drawString(buf, 120, 96, 4);
    snprintf(buf, sizeof buf, "%d %s", pattern + 1, NAMES[pattern]);
    tft.drawString(buf, 120, 124, 2);
    if (pattern == 1) tft.drawString("outer white r118-120", 120, 144, 2);
    if (pattern == 1) tft.drawString("2 px = 0.27 mm nom.", 120, 162, 2);
    if (pattern == 2) tft.drawString("grid 10 px", 120, 144, 2);
    if (pattern == 3) tft.drawString("3 in, 2 past r120", 120, 144, 2);
    if (pattern == 3) tft.drawString("outer 2: diagonals only", 120, 162, 2);
    if (pattern == 0) tft.drawString("any button: next", 120, 144, 2);
}

static void show() {
    for (int scr = 0; scr < 5; scr++) {
        int pattern = (scr + offset) % NPAT;
        selectScreen(scr);
        if (pattern == 0) drawFull();
        if (pattern == 1) drawRings();
        if (pattern == 2) drawGrid();
        if (pattern == 3) drawBeyond();
        label(scr, pattern);
    }
    selectScreen(-1);
}

void setup() {
    Serial.begin(115200);
    for (int i = 0; i < 5; i++) {
        pinMode(SCREEN_CS[i], OUTPUT);
    }
    for (int i = 0; i < 3; i++) {
        pinMode(BUTTONS[i], INPUT_PULLDOWN);
    }
    selectScreen(-1); // init all five together, as ScreenManager does
    tft.init();
    tft.setRotation(0);
    tft.setSwapBytes(true);
    show();
}

void loop() {
    static bool wasDown = false;
    bool down = false;
    for (int i = 0; i < 3; i++) {
        down |= digitalRead(BUTTONS[i]) == HIGH;
    }
    if (down && !wasDown) {
        offset = (offset + 1) % NPAT;
        show();
        delay(250); // crude debounce; this is a test tool
    }
    wasDown = down;
    delay(10);
}

#endif // DISPLAY_TEST
