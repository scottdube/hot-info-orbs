#ifndef STOCK_WIDGET_H
#define STOCK_WIDGET_H

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <TFT_eSPI.h>

#include "DeviceLog.h"
#include "StalePolicy.h"
#include "StockDataModel.h"
#include "Widget.h"

#define MAX_STOCKS 5

class StockWidget : public Widget {
public:
    StockWidget(ScreenManager &manager);
    void setup() override;
    void update(bool force = false) override;
    void draw(bool force = false) override;
    void buttonPressed(uint8_t buttonId, ButtonState state) override;
    String getName() override;
    bool dataStale() override { return m_fresh.stale(); }
    const char *dataKind() override { return "stocks"; }
    stale::LastGood lastGood() override { return m_fresh.lastGood(); }
    uint8_t dataOrbs() override { return (uint8_t)((1u << m_stockCount) - 1); } // one orb per ticker
    // Over the whole percent line (y 37..59, up to 100 px wide). The default
    // spot cut that line and the arrow above it in half.
    stale::Plate stalePlate() override { return {34, 112}; }

    void changeMode();

private:
    bool getStockData(StockDataModel &stock); // true when a price was applied
    void displayStock(int8_t displayIndex, StockDataModel &stock, uint32_t backgroundColor, uint32_t textColor);

    unsigned long m_stockDelay = 900000; // default to 15m between updates
    unsigned long m_stockDelayPrev = 0;
    stale::Tracker m_fresh;
    devlog::PageWatch m_logWatch; // the device log gets the changes, not every refresh
    uint8_t m_misses = 0; // failed refreshes in a row

    StockDataModel m_stocks[MAX_STOCKS];
    int8_t m_stockCount;
};
#endif // STOCK_WIDGET_H
