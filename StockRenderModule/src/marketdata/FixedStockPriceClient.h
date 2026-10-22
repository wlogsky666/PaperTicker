#ifndef FIXED_STOCK_PRICE_CLIENT_H
#define FIXED_STOCK_PRICE_CLIENT_H

#include <string.h>
#if defined(ARDUINO_ARCH_AVR)
#include <avr/pgmspace.h>
#else
#include <pgmspace.h>
#endif

#include "StockPriceClient.h"

namespace stockdata {

namespace fixedstockdata {

static const uint8_t AAPL_LEVELS[] PROGMEM = {
    70, 84, 100, 99, 80, 67, 43, 43, 31, 12, 0,  19, 16, 8,
    12, 20, 31,  37, 57, 56, 45, 47, 43, 41, 31, 39, 39};
static const uint8_t MSFT_LEVELS[] PROGMEM = {
    45, 42, 86, 83, 82, 98, 93, 98, 100, 92, 86, 72, 79, 73,
    75, 75, 83, 83, 84, 82, 88, 88, 91,  87, 90, 77, 76};
static const uint8_t GOOG_LEVELS[] PROGMEM = {
    55, 60, 96, 91, 82, 95, 100, 94, 93, 96, 89, 90, 87, 74,
    66, 68, 72, 64, 66, 66, 68,  76, 88, 94, 92, 66, 66};
static const uint8_t TSLA_LEVELS[] PROGMEM = {41, 53, 69, 81, 83,
                                              87, 91, 98, 98, 100};

} // namespace fixedstockdata

class FixedStockPriceClient : public StockPriceClient {
public:
  FixedStockPriceClient() {}

  bool fetch(const char *symbol, StockQuote &quote) override {
    if (symbol == 0)
      return false;

    const Fixture *fixture = findFixture(symbol);
    if (fixture == 0)
      return false;

    StockQuote parsed = {};
    strncpy(parsed.symbol, fixture->symbol, STOCK_SYMBOL_CAPACITY - 1);
    parsed.symbol[STOCK_SYMBOL_CAPACITY - 1] = '\0';
    parsed.price = fixture->price;
    parsed.previousClose =
        fixture->price / (1.0f + fixture->dailyChangePercent / 100.0f);
    parsed.intradayCount = fixture->sampleCount;

    uint8_t finalLevel =
        pgm_read_byte(fixture->levels + fixture->sampleCount - 1);
    float priceRange = parsed.price - parsed.previousClose;
    for (uint8_t i = 0; i < fixture->sampleCount; ++i) {
      uint8_t level = pgm_read_byte(fixture->levels + i);
      parsed.intradayPrices[i] =
          parsed.previousClose + priceRange * level / finalLevel;
    }

    quote = parsed;
    return true;
  }

private:
  struct Fixture {
    const char *symbol;
    float price;
    float dailyChangePercent;
    const uint8_t *levels;
    uint8_t sampleCount;
  };

  static const Fixture *findFixture(const char *symbol) {
    static const Fixture FIXTURES[] = {
        {"AAPL", 342.94f, -0.15f, fixedstockdata::AAPL_LEVELS,
         sizeof(fixedstockdata::AAPL_LEVELS)},
        {"MSFT", 533.52f, 0.84f, fixedstockdata::MSFT_LEVELS,
         sizeof(fixedstockdata::MSFT_LEVELS)},
        {"GOOG", 340.16f, 0.96f, fixedstockdata::GOOG_LEVELS,
         sizeof(fixedstockdata::GOOG_LEVELS)},
        {"TSLA", 368.34f, 3.84f, fixedstockdata::TSLA_LEVELS,
         sizeof(fixedstockdata::TSLA_LEVELS)}};

    for (uint8_t i = 0; i < sizeof(FIXTURES) / sizeof(FIXTURES[0]); ++i) {
      if (strcmp(symbol, FIXTURES[i].symbol) == 0)
        return &FIXTURES[i];
    }
    return 0;
  }
};

} // namespace stockdata

#endif