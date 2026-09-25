#ifndef WEATHERSOURCE_H
#define WEATHERSOURCE_H

#include "WeatherDataModel.h"
#include <Arduino.h>

// Where a weather page gets its data. WeatherWidget draws; a source fetches
// and fills the model. docs/superpowers/specs/2026-09-24-tempest-weather-design.md
class WeatherSource {
  public:
    virtual ~WeatherSource() = default;
    virtual bool fetch(WeatherDataModel &model) = 0; // true = model filled
    virtual String label() { return ""; } // shown on orb 1 before the first fetch; "" = none
    virtual String name() { return "Weather"; } // WidgetSet's loading screen and log
};

#endif
