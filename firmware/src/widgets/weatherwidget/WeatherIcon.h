#ifndef WEATHERICON_H
#define WEATHERICON_H

#include "icons.h"
#include <Arduino.h>

// Maps a Visual Crossing (iconSet=icons1) condition name and the screen mode
// to one of the embedded weather JPEGs. Shared by the weather pages and the
// World Clock page so both draw the same art. Returns false for an unknown
// condition, leaving start/size untouched.
namespace wxicon {

// true = Dark mode art (white glyphs), false = Light mode art (black glyphs)
inline bool resolve(const String &condition, bool dark, const byte *&start, int &size) {
    const byte *s = nullptr;
    const byte *e = nullptr;
    if (condition == "partly-cloudy-night") {
        s = dark ? moonCloudB_start : moonCloudW_start;
        e = dark ? moonCloudB_end : moonCloudW_end;
    } else if (condition == "partly-cloudy-day") {
        s = dark ? sunCloudsB_start : sunCloudsW_start;
        e = dark ? sunCloudsB_end : sunCloudsW_end;
    } else if (condition == "clear-day") {
        s = dark ? sunB_start : sunW_start;
        e = dark ? sunB_end : sunW_end;
    } else if (condition == "clear-night") {
        s = dark ? moonB_start : moonW_start;
        e = dark ? moonB_end : moonW_end;
    } else if (condition == "snow") {
        s = dark ? snowB_start : snowW_start;
        e = dark ? snowB_end : snowW_end;
    } else if (condition == "rain") {
        s = dark ? rainB_start : rainW_start;
        e = dark ? rainB_end : rainW_end;
    } else if (condition == "fog" || condition == "wind" || condition == "cloudy") {
        s = dark ? cloudsB_start : cloudsW_start;
        e = dark ? cloudsB_end : cloudsW_end;
    } else {
        return false;
    }
    start = s;
    size = (int)(e - s);
    return size > 0;
}

} // namespace wxicon

#endif // WEATHERICON_H
