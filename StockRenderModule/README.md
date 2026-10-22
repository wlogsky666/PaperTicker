# StockRenderModule

A rendering and display-driver example for the Waveshare 1.54-inch B V2 200x200 e-paper panel. It draws four stock cells with prices, daily changes, sparklines, and yesterday-close reference lines.

## Arduino example

Open `StockRenderModule.ino` from this folder. Edit `StockRenderConfig.h` to change display pins, SPI clock, serial baud rate, rotation, or rendering mode. The default pin map is DIN D11, CLK D13, CS D10, DC D9, RST D8, BUSY D7, and power D6.

`STOCK_RENDER_USE_FRAMEBUFFER` selects the output path:

- `0`: stream bytes directly to the panel; default and suitable for Uno/Nano SRAM limits.
- `1`: render to two static 5 KB planes before transfer; requires a board with enough RAM, such as an ESP32.

The sketch requests AAPL, MSFT, GOOG, and TSLA from `FixedStockPriceClient` and renders only returned quote/history data. The sketch contains the requested symbols, but no embedded quote values or sparkline data. The fixed provider stores chart-level samples in program memory with `PROGMEM` and reads them using `pgm_read_byte`, supported by the Arduino AVR and ESP32 cores. It supplies 27 samples for AAPL, MSFT, and GOOG and a simulated 10-sample noon update for TSLA. If a symbol fetch fails, its symbol remains visible without a price or chart. This demo does not contact Yahoo or schedule periodic refreshes.

`src/render/StockRenderer.h` provides the renderer API. `Stock` carries quote values and a normalized sparkline. `sparklineCount` is the valid prefix of the data and `sparklineCapacity` is the fixed timeline length, allowing partial-day data to leave future chart space blank. `PixelSource` maps coordinates to pixel colors. `StreamSink` emits packed bytes immediately, while `FramebufferSink` writes them into caller-owned buffers.

`src/display/EpdDisplay.h/.cpp` encapsulates the panel protocol and Arduino SPI/GPIO operations. `StockRenderConfig.h` remains at the sketch root for panel, pin, refresh mode, rotation, and SPI settings.

## Live-data module draft

The `src/network/` folder contains the HTTP transport boundary in `NetworkClient.h`. A Wi-Fi, Ethernet, or cellular implementation can provide that interface without making stock-provider parsing depend on the connection type. The `src/marketdata/` folder contains `StockQuote.h`, the provider-facing contract in `StockPriceClient.h`, and `YahooFinanceStockPriceClient.h`, which builds the Yahoo chart URL and parses quote/history data. Quote values are numeric and separate from the display formatting in `src/render/StockRenderer.h`.

`src/marketdata/YahooFinanceStockPriceClient.h` is a separate provider/parser component and is not used by the main display sketch.

Yahoo's chart endpoint is unofficial and may change; this implementation is a prototype, not a guaranteed data service. The target board and an HTTPS-capable network adapter still need to be selected.
