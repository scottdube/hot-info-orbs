#ifndef VISUALCROSSINGSOURCE_H
#define VISUALCROSSINGSOURCE_H

#include "WeatherSource.h"

// Visual Crossing timeline API, location and units from Settings. The body
// moved from WeatherWidget::getWeatherData() unchanged.
class VisualCrossingSource : public WeatherSource {
  public:
    VisualCrossingSource();
    bool fetch(WeatherDataModel &model) override;

  private:
    String m_url;
};

#endif
