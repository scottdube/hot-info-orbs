#include "StockWidget.h"

#include "Settings.h"
#include "config_helper.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>

#include <iomanip>

StockWidget::StockWidget(ScreenManager &manager) : Widget(manager) {
    // Settings holds a normalized list (1..5, no empties). The bound is checked
    // BEFORE writing: the old loop wrote m_stocks[5] and then broke.
    const std::string &list = Settings::get().tickers;
    m_stockCount = 0;
    size_t start = 0;
    while (start < list.size()) {
        size_t comma = list.find(',', start);
        std::string symbol = list.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
        if (!symbol.empty()) {
            if (m_stockCount >= MAX_STOCKS) {
                Serial.println("MAX STOCKS UNABLE TO ADD MORE");
                break;
            }
            StockDataModel stockModel = StockDataModel();
            stockModel.setSymbol(String(symbol.c_str()));
            m_stocks[m_stockCount] = stockModel;
            m_stockCount++;
        }
        if (comma == std::string::npos) {
            break;
        }
        start = comma + 1;
    }
}

void StockWidget::setup() {
    if (m_stockCount == 0) {
        Serial.println("No stock tickers available");
        return;
    }
}

void StockWidget::draw(bool force) {
    m_manager.setFont(DEFAULT_FONT);
    for (int8_t i = 0; i < m_stockCount; i++) {
        if (m_stocks[i].isChanged() || force) {
            displayStock(i, m_stocks[i], TFT_WHITE, TFT_BLACK);
            m_stocks[i].setChangedStatus(false);
        }
    }
}

void StockWidget::update(bool force) {
    if (force || m_stockDelayPrev == 0 || (millis() - m_stockDelayPrev) >= m_stockDelay) {
        setBusy(true);
        for (int8_t i = 0; i < m_stockCount; i++) {
            getStockData(m_stocks[i]);
        }
        setBusy(false);
        m_stockDelayPrev = millis();
    }
}

void StockWidget::changeMode() {
    update(true);
}

void StockWidget::buttonPressed(uint8_t buttonId, ButtonState state) {
    if (buttonId == BUTTON_OK && state == BTN_SHORT)
        changeMode();
}

void StockWidget::getStockData(StockDataModel &stock) {
    String httpRequestAddress = "https://api.twelvedata.com/quote?apikey=e03fc53524454ab8b65d91b23c669cc5&symbol=" + stock.getSymbol();

    HTTPClient http;
    http.begin(httpRequestAddress);
    int httpCode = http.GET();

    if (httpCode > 0) { // Check for the returning code
        String payload = http.getString();
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
            float currentPrice = doc["close"].as<float>();
            if (currentPrice > 0.0) {
                stock.setCurrentPrice(doc["close"].as<float>());
                stock.setPercentChange(doc["percent_change"].as<float>() / 100);
                stock.setPriceChange(doc["change"].as<float>());
                stock.setHighPrice(doc["fifty_two_week"]["high"].as<float>());
                stock.setLowPrice(doc["fifty_two_week"]["low"].as<float>());
                stock.setCompany(doc["name"].as<String>());
                stock.setTicker(doc["symbol"].as<String>());
                stock.setCurrencySymbol(doc["currency"].as<String>());
            } else {
                Serial.println("skipping invalid data for: " + stock.getSymbol());
            }
        } else {
            // Handle JSON deserialization error
            Serial.println("deserializeJson() failed");
        }
    } else {
        // Handle HTTP request error
        Serial.printf("HTTP request failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
}

void StockWidget::displayStock(int8_t displayIndex, StockDataModel &stock, uint32_t backgroundColor, uint32_t textColor) {
    Serial.println("displayStock - " + stock.getSymbol() + " ~ " + stock.getCurrentPrice());
    if (stock.getCurrentPrice() == 0.0) {
        // There isn't any data to display yet
        return;
    }
    m_manager.selectScreen(displayIndex);

    m_manager.fillScreen(TFT_BLACK);

    // Calculate center positions
    int screenWidth = SCREEN_SIZE;
    int center = 120;
    int arrowOffsetX = 0;
    int arrowOffsetY = -109;

    // Outputs
    m_manager.fillRect(0, 70, screenWidth, 49, TFT_WHITE);
    m_manager.fillRect(0, 111, screenWidth, 20, TFT_LIGHTGREY);
    int smallFontSize = 11;
    int bigFontSize = 29;
    m_manager.setFontColor(TFT_WHITE, TFT_BLACK);
    m_manager.drawCenterString("52 Week:", center, 185, smallFontSize);
    m_manager.drawCenterString("H: " + stock.getCurrencySymbol() + stock.getHighPrice(), center, 200, smallFontSize);
    m_manager.drawCenterString("L: " + stock.getCurrencySymbol() + stock.getLowPrice(), center, 215, smallFontSize);
    m_manager.setFontColor(TFT_BLACK, TFT_LIGHTGREY);
    m_manager.drawString(stock.getCompany(), center, 121, smallFontSize, Align::MiddleCenter);
    if (stock.getPercentChange() < 0.0) {
        m_manager.setFontColor(TFT_RED, TFT_BLACK);
        m_manager.fillTriangle(110 + arrowOffsetX, 120 + arrowOffsetY, 130 + arrowOffsetX, 120 + arrowOffsetY, 120 + arrowOffsetX, 132 + arrowOffsetY, TFT_RED);
        m_manager.drawArc(center, center, 120, 118, 0, 360, TFT_RED, TFT_RED);
    } else {
        m_manager.setFontColor(TFT_GREEN, TFT_BLACK);
        m_manager.fillTriangle(110 + arrowOffsetX, 132 + arrowOffsetY, 130 + arrowOffsetX, 132 + arrowOffsetY, 120 + arrowOffsetX, 120 + arrowOffsetY, TFT_GREEN);
        m_manager.drawArc(center, center, 120, 118, 0, 360, TFT_GREEN, TFT_GREEN);
    }
    m_manager.drawString(stock.getPercentChange(2) + "%", center, 48, bigFontSize, Align::MiddleCenter);
    // Draw stock data
    m_manager.setFontColor(TFT_BLACK, TFT_WHITE);

    m_manager.drawString(stock.getTicker(), center, 92, bigFontSize, Align::MiddleCenter);
    m_manager.setFontColor(TFT_WHITE, TFT_BLACK);

    m_manager.drawString(stock.getCurrencySymbol() + stock.getCurrentPrice(2), center, 155, bigFontSize, Align::MiddleCenter);
}

String StockWidget::getName() {
    return "Stock";
}
