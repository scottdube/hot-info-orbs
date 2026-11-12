#pragma once
// Puts the NoseShape.h nose on a 240x240 orb. The caller has selected the
// panel and, while the eye task runs, holds the bus.
#include <TFT_eSPI.h>

namespace nose {
// lidStyle = the "lids" setting, as nose::style() reads it
void draw(TFT_eSPI &tft, int lidStyle);
} // namespace nose
