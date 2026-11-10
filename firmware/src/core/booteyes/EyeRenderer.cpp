#ifdef BOOT_EYES
    #include "EyeRenderer.h"
    #include "IrisPalette.h"
    #include "LidPalette.h"
    #include <Arduino.h>

namespace eyedata {
    #include "eyeData.h"
}
using namespace eyedata;

namespace {
constexpr int kCrop = 4;             // 128 - 2*4 = 120 source px -> 240 panel px
constexpr uint32_t kIrisScale = 110; // fixed pupil: midpoint of the example's IRIS_MIN..IRIS_MAX (90..130)
// Lids, in lid-map units: margin past the closing threshold, small = lid edge.
// Line width and colors are in LidPalette.h.
using lids::kLashW;
constexpr int32_t kLashLen = 45;  // lash strands, upper lid only
constexpr int32_t kLashSp = 6;    // strand spacing, source px
constexpr int32_t kLashFan16 = 5; // outward lean, x16
} // namespace

EyeRenderer::EyeRenderer(TFT_eSPI &tft) : m_tft(tft) {
    setStyle(lids::kDefault);
    uint32_t hist[256] = {};
    for (uint32_t i = 0; i < IRIS_MAP_WIDTH * IRIS_MAP_HEIGHT; i++) {
        hist[iriscolor::luma565(pgm_read_word(eyedata::iris + i))]++;
    }
    iriscolor::range(hist, m_irisLo, m_irisHi);
}

void EyeRenderer::setIris(int index) {
    m_irisNatural = !iriscolor::valid(index) || index == iriscolor::kNatural;
    if (!m_irisNatural) {
        iriscolor::buildRamp(iriscolor::kIrises[index], m_irisLo, m_irisHi, m_irisRamp);
    }
}

void EyeRenderer::setStyle(int index) {
    if (index < 0 || index >= lids::kCount) {
        index = lids::kDefault;
    }
    if (index == m_style) {
        return;
    }
    const lids::Style &s = lids::kStyles[index];
    for (int level = 0; level < lids::kBrushLevels; level++) {
        for (int m = 0; m < 256; m++) {
            m_body[level][m] = lids::to565(lids::body(s, m, level));
        }
    }
    m_lash = lids::to565(s.lash);
    m_style = index;
}

bool EyeRenderer::draw(uint8_t side, const EyePose &pose, const volatile bool *abort) {
    // Gaze 0..1023 -> window offset into the 200x200 sclera, slightly converged
    int32_t eyeX = map(pose.gazeX, 0, 1023, 0, SCLERA_WIDTH - SCREEN_WIDTH) + (side ? 4 : -4);
    int32_t eyeY = map(pose.gazeY, 0, 1023, 0, SCLERA_HEIGHT - SCREEN_HEIGHT);
    eyeX = constrain(eyeX, 0, SCLERA_WIDTH - SCREEN_WIDTH);
    eyeY = constrain(eyeY, 0, SCLERA_HEIGHT - SCREEN_HEIGHT);

    // Upper lid tracks the pupil (sampled just above it); the lower lid follows
    const int32_t sampleX = SCLERA_WIDTH / 2 - eyeX / 2;
    const int32_t sampleY = SCLERA_HEIGHT / 2 - (eyeY + IRIS_HEIGHT / 4);
    uint32_t n = 0;
    if (sampleY >= 0) {
        n = (pgm_read_byte(upper + sampleY * SCREEN_WIDTH + sampleX) +
             pgm_read_byte(upper + sampleY * SCREEN_WIDTH + (SCREEN_WIDTH - 1 - sampleX))) /
            2;
    }
    m_uThreshold[side] = (m_uThreshold[side] * 3 + n) / 4;
    uint32_t uT = m_uThreshold[side];
    uint32_t lT = 254 - uT;
    // The script closes the lids over the tracking: 254 = shut
    const uint32_t open = pose.open > 256 ? 256 : pose.open;
    uT = (uT * open + 254 * (256 - open)) / 256;
    lT = (lT * open + 254 * (256 - open)) / 256;

    const bool swap = m_tft.getSwapBytes();
    m_tft.setSwapBytes(false); // bytes are swapped by hand below, as in the example
    m_tft.startWrite();
    m_tft.setAddrWindow(0, 0, 240, 240);

    uint32_t scleraY = eyeY;
    int32_t irisY = eyeY - (SCLERA_HEIGHT - IRIS_HEIGHT) / 2;
    const int16_t dLidX = side ? 1 : -1; // lid maps are mirrored for the left eye
    bool finished = true;
    for (int screenY = 0; screenY < SCREEN_HEIGHT; screenY++, scleraY++, irisY++) {
        if (screenY < kCrop || screenY >= SCREEN_HEIGHT - kCrop) {
            continue;
        }
        if (abort && *abort) {
            finished = false;
            break;
        }
        uint32_t scleraX = eyeX;
        int32_t irisX = eyeX - (SCLERA_WIDTH - IRIS_WIDTH) / 2;
        int16_t lidX = side ? 0 : SCREEN_WIDTH - 1;
        uint16_t o = 0;
        const uint16_t *body = m_body[lids::brushLevel(screenY)]; // one brushing step per row
        for (int screenX = 0; screenX < SCREEN_WIDTH; screenX++, scleraX++, irisX++, lidX += dLidX) {
            if (screenX < kCrop || screenX >= SCREEN_WIDTH - kCrop) {
                continue;
            }
            const uint32_t vLo = pgm_read_byte(lower + screenY * SCREEN_WIDTH + lidX);
            const uint32_t vUp = pgm_read_byte(upper + screenY * SCREEN_WIDTH + lidX);
            uint32_t p;
            if (vLo <= lT || vUp <= uT) {
                const int32_t mU = (vUp <= uT) ? int32_t(uT) - int32_t(vUp) : 9999;
                const int32_t mL = (vLo <= lT) ? int32_t(lT) - int32_t(vLo) : 9999;
                const int32_t m = mU < mL ? mU : mL;
                const bool upperHit = (vUp <= uT) && (mU <= mL);
                p = body[m > 255 ? 255 : m];
                if (m < kLashW) {
                    p = m_lash; // lash line
                } else if (m_strands && upperHit && m < kLashW + kLashLen) { // strands, fanning out
                    const int32_t k = m - kLashW;
                    const int32_t ox = int32_t(screenX) - 64;
                    const int32_t sx = int32_t(screenX) * 16 - (ox * k * kLashFan16) / 64;
                    const int32_t per = kLashSp * 16;
                    const int32_t ph = ((sx % per) + per) % per;
                    const int32_t th = (k < kLashLen / 2) ? 24 : 14; // taper, x16 px
                    if (ph < th) {
                        p = m_lash;
                    }
                }
            } else if (irisY < 0 || irisY >= IRIS_HEIGHT || irisX < 0 || irisX >= IRIS_WIDTH) {
                p = pgm_read_word(sclera + scleraY * SCLERA_WIDTH + scleraX);
            } else {
                p = pgm_read_word(polar + irisY * IRIS_WIDTH + irisX);
                const uint32_t d = (kIrisScale * (p & 0x7F)) / 128;
                if (d < IRIS_MAP_HEIGHT) {
                    const uint32_t a = (IRIS_MAP_WIDTH * (p >> 7)) / 512;
                    p = pgm_read_word(iris + d * IRIS_MAP_WIDTH + a);
                    if (!m_irisNatural) {
                        p = m_irisRamp[iriscolor::luma565(p)];
                    }
                } else {
                    p = pgm_read_word(sclera + scleraY * SCLERA_WIDTH + scleraX);
                }
            }
            const uint16_t q = uint16_t(p >> 8 | p << 8);
            m_line[o++] = q;
            m_line[o++] = q;
        }
        m_tft.pushPixels(m_line, 240);
        m_tft.pushPixels(m_line, 240);
    }
    m_tft.endWrite();
    m_tft.setSwapBytes(swap);
    return finished;
}
#endif // BOOT_EYES
