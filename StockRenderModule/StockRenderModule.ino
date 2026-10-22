#include "StockRenderConfig.h"
#include "src/display/EpdDisplay.h"
#include "src/marketdata/FixedStockPriceClient.h"
#include "src/render/StockRenderer.h"

using namespace stockrender;
using namespace stockdata;

static EpdDisplay display;
static FixedStockPriceClient stockPriceClient;
static const uint8_t TRADING_DAY_POINTS = 27;
static const char *const STOCK_SYMBOLS[] = {"AAPL", "MSFT", "GOOG", "TSLA"};
static const uint8_t STOCK_COUNT =
    sizeof(STOCK_SYMBOLS) / sizeof(STOCK_SYMBOLS[0]);

static Stock ACTIVE_STOCKS[STOCK_COUNT];
static uint8_t ACTIVE_SPARKLINES[STOCK_COUNT][TRADING_DAY_POINTS];
static char ACTIVE_PRICES[STOCK_COUNT][16];
static char ACTIVE_CHANGES[STOCK_COUNT][16];
static const Layout DASHBOARD_LAYOUT = {2, 2, 2, 62};
static const PixelSource RENDERER(ACTIVE_STOCKS, STOCK_COUNT, DASHBOARD_LAYOUT);

#if STOCK_RENDER_USE_FRAMEBUFFER
static uint8_t blackPlane[PLANE_BYTES];
static uint8_t redPlane[PLANE_BYTES];
#endif

static void writeStreamByte(Plane plane, uint8_t value) {
  display.writePlaneByte(plane == BlackPlane ? EpdPlane::Black : EpdPlane::Red,
                         value);
}

static void refreshDisplay() { display.refresh(); }

static bool applyQuote(uint8_t index, const StockQuote &quote) {
  if (quote.intradayCount < 2 || quote.intradayCount > TRADING_DAY_POINTS ||
      quote.previousClose <= 0.0f)
    return false;

  float minimum = quote.previousClose;
  float maximum = quote.previousClose;
  for (uint8_t i = 0; i < quote.intradayCount; ++i) {
    if (quote.intradayPrices[i] < minimum)
      minimum = quote.intradayPrices[i];
    if (quote.intradayPrices[i] > maximum)
      maximum = quote.intradayPrices[i];
  }

  float span = maximum - minimum;
  for (uint8_t i = 0; i < quote.intradayCount; ++i) {
    float normalized =
        span == 0.0f ? 50.0f
                     : (quote.intradayPrices[i] - minimum) * 100.0f / span;
    ACTIVE_SPARKLINES[index][i] = (uint8_t)(normalized + 0.5f);
  }

  float previousCloseLevel =
      span == 0.0f ? 50.0f : (quote.previousClose - minimum) * 100.0f / span;
  float changePercent =
      (quote.price - quote.previousClose) * 100.0f / quote.previousClose;

  String priceText = "$";
  priceText += String(quote.price, 2);
  priceText.toCharArray(ACTIVE_PRICES[index], sizeof(ACTIVE_PRICES[index]));

  String changeText = changePercent < 0.0f ? "-" : "+";
  changeText +=
      String(changePercent < 0.0f ? -changePercent : changePercent, 2);
  changeText += '%';
  changeText.toCharArray(ACTIVE_CHANGES[index], sizeof(ACTIVE_CHANGES[index]));

  ACTIVE_STOCKS[index].price = ACTIVE_PRICES[index];
  ACTIVE_STOCKS[index].change = ACTIVE_CHANGES[index];
  ACTIVE_STOCKS[index].changeIsRed = changePercent < 0.0f;
  ACTIVE_STOCKS[index].previousCloseLevel =
      (uint8_t)(previousCloseLevel + 0.5f);
  ACTIVE_STOCKS[index].sparkline = ACTIVE_SPARKLINES[index];
  ACTIVE_STOCKS[index].sparklineCount = quote.intradayCount;
  ACTIVE_STOCKS[index].sparklineCapacity = TRADING_DAY_POINTS;
  return true;
}

static void fetchStockQuotes() {
  for (uint8_t i = 0; i < STOCK_COUNT; ++i)
    ACTIVE_STOCKS[i] = {STOCK_SYMBOLS[i],  NULL, NULL, false, 50, NULL, 0,
                        TRADING_DAY_POINTS};

  for (uint8_t i = 0; i < STOCK_COUNT; ++i) {
    StockQuote quote = {};
    bool fetched = stockPriceClient.fetch(STOCK_SYMBOLS[i], quote);
    if (!fetched) {
      Serial.print(F("Fetch/parse failed for "));
      Serial.print(STOCK_SYMBOLS[i]);
      Serial.print(F(": "));
      Serial.println(F("no demo fixture available"));
      continue;
    }

    if (applyQuote(i, quote)) {
      Serial.print(F("Loaded quote for "));
      Serial.println(STOCK_SYMBOLS[i]);
      Serial.print(F(" price="));
      Serial.print(quote.price, 2);
      Serial.print(F(" samples="));
      Serial.println(quote.intradayCount);
    } else {
      Serial.print(F("Quote rejected for "));
      Serial.print(STOCK_SYMBOLS[i]);
      Serial.print(F("; price="));
      Serial.print(quote.price, 2);
      Serial.print(F(" previousClose="));
      Serial.print(quote.previousClose, 2);
      Serial.print(F(" samples="));
      Serial.println(quote.intradayCount);
    }
  }
}

static void drawDashboard() {
#if STOCK_RENDER_USE_FRAMEBUFFER
  FramebufferSink sink(blackPlane, redPlane);
  sink.render(RENDERER);

  for (size_t i = 0; i < PLANE_BYTES; ++i)
    display.writePlaneByte(EpdPlane::Black, blackPlane[i]);
  for (size_t i = 0; i < PLANE_BYTES; ++i)
    display.writePlaneByte(EpdPlane::Red, redPlane[i]);
#else
  StreamSink sink(writeStreamByte);
  sink.render(RENDERER);
#endif
  refreshDisplay();
}

void setup() {
  Serial.begin(STOCK_RENDER_SERIAL_BAUD);
  Serial.println(F("Fetching stock data"));
  fetchStockQuotes();

  if (display.begin() != 0) {
    Serial.println(F("Display initialization failed"));
    while (true)
      delay(1000);
  }

  unsigned long started = millis();
  drawDashboard();
  Serial.print(F("Refresh finished in "));
  Serial.print(millis() - started);
  Serial.println(F(" ms"));
  display.sleep();
}

void loop() {}
