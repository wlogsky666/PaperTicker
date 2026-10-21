# StockRenderModule

A rendering and display-driver example for the Waveshare 1.54-inch B V2 200x200 e-paper panel. It draws four stock cells with prices, daily changes, sparklines, and yesterday-close reference lines.

## Arduino example

Open `StockRenderModule.ino` from this folder. Edit `StockRenderConfig.h` to change display pins, SPI clock, serial baud rate, rotation, or rendering mode. The default pin map is DIN D11, CLK D13, CS D10, DC D9, RST D8, BUSY D7, and power D6.

`STOCK_RENDER_USE_FRAMEBUFFER` selects the output path:

- `0`: stream bytes directly to the panel; default and suitable for Uno/Nano SRAM limits.
- `1`: render to two static 5 KB planes before transfer; requires a board with enough RAM, such as an ESP32.

The static quote snapshot is for October 19, 2026. The regular U.S. session is 9:30 a.m.-4:00 p.m. Eastern, represented by 27 slots at 15-minute intervals including the open. AAPL, MSFT, and GOOG show the full session. TSLA intentionally simulates a noon update with 10 samples; its remaining timeline slots are blank. The sketch does not fetch live quotes or schedule periodic refreshes.

`StockRenderer.h` provides the renderer API. `Stock` carries quote values and a normalized sparkline. `sparklineCount` is the valid prefix of the data and `sparklineCapacity` is the fixed timeline length, allowing partial-day data to leave future chart space blank. `PixelSource` maps coordinates to pixel colors. `StreamSink` emits packed bytes immediately, while `FramebufferSink` writes them into caller-owned buffers.

`EpdDisplay.h/.cpp` encapsulates the panel protocol and Arduino SPI/GPIO operations. `StockRenderConfig.h` holds panel, pin, refresh mode, rotation, and SPI settings.
