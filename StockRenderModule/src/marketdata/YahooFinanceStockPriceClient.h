#ifndef YAHOO_FINANCE_STOCK_PRICE_CLIENT_H
#define YAHOO_FINANCE_STOCK_PRICE_CLIENT_H

#include <ArduinoJson.h>
#include <string.h>

#include "../network/NetworkClient.h"
#include "StockPriceClient.h"

namespace stockdata {

class YahooFinanceStockPriceClient : public StockPriceClient {
public:
  explicit YahooFinanceStockPriceClient(NetworkClient &network)
      : network_(network), lastError_("") {}

  const char *lastError() const { return lastError_; }

  bool fetch(const char *symbol, StockQuote &quote) override {
    if (symbol == 0 || symbol[0] == '\0' ||
        strlen(symbol) >= STOCK_SYMBOL_CAPACITY) {
      lastError_ = "invalid symbol";
      return false;
    }

    char url[128];
    if (!buildUrl(url, sizeof(url), symbol)) {
      lastError_ = "URL buffer too small";
      return false;
    }

    const HttpHeader headers[] = {{"Accept", "application/json"}};
    HttpResponse response;
    if (!network_.get(url, headers, 1, response)) {
      lastError_ = "transport failed";
      return false;
    }
    if (response.statusCode != 200) {
      lastError_ = "HTTP status not 200";
      return false;
    }
    if (response.body == 0 || response.bodyLength == 0) {
      lastError_ = "empty response body";
      return false;
    }

    JsonDocument document;
    if (deserializeJson(document, response.body, response.bodyLength)) {
      lastError_ = "invalid JSON";
      return false;
    }

    JsonObjectConst chart = document["chart"].as<JsonObjectConst>();
    JsonArrayConst results = chart["result"].as<JsonArrayConst>();
    if (chart["error"].is<JsonObjectConst>()) {
      lastError_ = "provider returned chart error";
      return false;
    }
    if (results.isNull() || results.size() == 0) {
      lastError_ = "chart result missing";
      return false;
    }

    JsonObjectConst result = results[0].as<JsonObjectConst>();
    JsonObjectConst meta = result["meta"].as<JsonObjectConst>();
    JsonObjectConst indicators = result["indicators"].as<JsonObjectConst>();
    JsonArrayConst quoteIndicators = indicators["quote"].as<JsonArrayConst>();
    if (meta.isNull() || quoteIndicators.isNull() ||
        quoteIndicators.size() == 0) {
      lastError_ = "quote metadata missing";
      return false;
    }
    if (!meta["regularMarketPrice"].is<float>()) {
      lastError_ = "regular market price missing";
      return false;
    }

    JsonVariantConst previousClose = meta["chartPreviousClose"];
    if (previousClose.isNull())
      previousClose = meta["previousClose"];
    if (!previousClose.is<float>()) {
      lastError_ = "previous close missing";
      return false;
    }

    JsonArrayConst closes = quoteIndicators[0]["close"].as<JsonArrayConst>();
    if (closes.isNull()) {
      lastError_ = "intraday closes missing";
      return false;
    }

    StockQuote parsed = {};
    strncpy(parsed.symbol, symbol, STOCK_SYMBOL_CAPACITY - 1);
    parsed.symbol[STOCK_SYMBOL_CAPACITY - 1] = '\0';
    parsed.price = meta["regularMarketPrice"].as<float>();
    parsed.previousClose = previousClose.as<float>();

    for (JsonVariantConst close : closes) {
      if (close.isNull())
        break;
      if (!close.is<float>()) {
        lastError_ = "invalid intraday close";
        return false;
      }
      if (parsed.intradayCount == STOCK_INTRADAY_CAPACITY)
        break;
      parsed.intradayPrices[parsed.intradayCount++] = close.as<float>();
    }

    if (parsed.intradayCount == 0) {
      lastError_ = "no intraday samples";
      return false;
    }

    quote = parsed;
    lastError_ = "";
    return true;
  }

private:
  static bool buildUrl(char *url, size_t capacity, const char *symbol) {
    static const char PREFIX[] =
        "https://query1.finance.yahoo.com/v8/finance/chart/";
    static const char SUFFIX[] = "?range=1d&interval=15m&includePrePost=false";
    static const char HEX_DIGITS[] = "0123456789ABCDEF";
    size_t length = sizeof(PREFIX) - 1;
    if (length >= capacity)
      return false;
    memcpy(url, PREFIX, length);

    for (const unsigned char *cursor =
             reinterpret_cast<const unsigned char *>(symbol);
         *cursor != '\0'; ++cursor) {
      unsigned char value = *cursor;
      if ((value >= 'A' && value <= 'Z') || (value >= 'a' && value <= 'z') ||
          (value >= '0' && value <= '9') || value == '-' || value == '_' ||
          value == '.' || value == '~') {
        if (length + 1 >= capacity)
          return false;
        url[length++] = static_cast<char>(value);
      } else {
        if (length + 3 >= capacity)
          return false;
        url[length++] = '%';
        url[length++] = HEX_DIGITS[value >> 4];
        url[length++] = HEX_DIGITS[value & 0x0F];
      }
    }

    size_t suffixLength = sizeof(SUFFIX) - 1;
    if (length + suffixLength >= capacity)
      return false;
    memcpy(url + length, SUFFIX, suffixLength + 1);
    return true;
  }

  NetworkClient &network_;
  const char *lastError_;
};

} // namespace stockdata

#endif