#ifndef STOCK_QUOTE_H
#define STOCK_QUOTE_H

#include <Arduino.h>

namespace stockdata {

static const uint8_t STOCK_SYMBOL_CAPACITY = 12;
static const uint8_t STOCK_INTRADAY_CAPACITY = 27;

struct StockQuote {
  char symbol[STOCK_SYMBOL_CAPACITY];
  float price;
  float previousClose;
  float intradayPrices[STOCK_INTRADAY_CAPACITY];
  uint8_t intradayCount;
};

} // namespace stockdata

#endif