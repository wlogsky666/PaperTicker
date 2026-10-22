#ifndef STOCK_PRICE_CLIENT_H
#define STOCK_PRICE_CLIENT_H

#include "StockQuote.h"

namespace stockdata {

class StockPriceClient {
public:
  virtual ~StockPriceClient() {}

  virtual bool fetch(const char *symbol, StockQuote &quote) = 0;
};

} // namespace stockdata

#endif