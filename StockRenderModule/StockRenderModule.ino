#include "EpdDisplay.h"
#include "StockRenderConfig.h"
#include "StockRenderer.h"

using namespace stockrender;

static EpdDisplay display;
static const uint8_t TRADING_DAY_POINTS = 27;
static const uint8_t TSLA_NOON_POINTS = 10;

// Oct 19, 2026 15-minute closes, normalized with the Oct 16 close to 0-100.
static const uint8_t AAPL_SPARK[] = {
    70, 84, 100, 99, 80, 67, 43, 43, 31, 12, 0, 19, 16, 8,
    12, 20, 31, 37, 57, 56, 45, 47, 43, 41, 31, 39, 39};
static const uint8_t MSFT_SPARK[] = {
    45, 42, 86, 83, 82, 98, 93, 98, 100, 92, 86, 72, 79, 73,
    75, 75, 83, 83, 84, 82, 88, 88, 91, 87, 90, 77, 76};
static const uint8_t GOOG_SPARK[] = {
    55, 60, 96, 91, 82, 95, 100, 94, 93, 96, 89, 90, 87, 74,
    66, 68, 72, 64, 66, 66, 68, 76, 88, 94, 92, 66, 66};
// Simulated noon update: first 10 completed bars; remaining slots are unused.
static const uint8_t TSLA_SPARK[TRADING_DAY_POINTS] = {
    41, 53, 69, 81, 83, 87, 91, 98, 98, 100};

// Oct 19, 2026 close; daily change is versus the Oct 16 close.
static const Stock STOCKS[] = {
    {"AAPL", "$342.94", "-0.15%", true, 53, AAPL_SPARK, TRADING_DAY_POINTS,
     TRADING_DAY_POINTS},
    {"MSFT", "$533.52", "+0.84%", false, 0, MSFT_SPARK, TRADING_DAY_POINTS,
     TRADING_DAY_POINTS},
    {"GOOG", "$340.16", "+0.96%", false, 0, GOOG_SPARK, TRADING_DAY_POINTS,
     TRADING_DAY_POINTS},
    {"TSLA", "$368.34", "+3.84%", false, 0, TSLA_SPARK, TSLA_NOON_POINTS,
     TRADING_DAY_POINTS}};

static const Layout DASHBOARD_LAYOUT = {2, 2, 2, 62};
static const PixelSource RENDERER(STOCKS, sizeof(STOCKS) / sizeof(STOCKS[0]),
                                  DASHBOARD_LAYOUT);

#if STOCK_RENDER_USE_FRAMEBUFFER
static uint8_t blackPlane[PLANE_BYTES];
static uint8_t redPlane[PLANE_BYTES];
#endif

static void writeStreamByte(Plane plane, uint8_t value) {
  display.writePlaneByte(plane == BlackPlane ? EpdPlane::Black : EpdPlane::Red,
                         value);
}

static void refreshDisplay() { display.refresh(); }

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
  Serial.println(F("Oct 19, 2026 15-minute chart; TSLA simulates noon"));

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
