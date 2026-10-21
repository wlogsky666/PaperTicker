# PaperTicker

A configurable stock ticker for e-paper. The project is intended to update selected stock prices every 15 minutes, with a simple setup interface for Wi-Fi, stock selection, and display settings.

## Current example

The [`StockRenderModule`](StockRenderModule/) folder contains a self-contained Arduino example for the Waveshare 1.54-inch B V2 200x200 panel. It demonstrates stock rendering, 15-minute intraday chart slots, partial-day chart rendering, and both streaming and framebuffer output modes.

The example currently uses a fixed historical snapshot; it does not yet connect to Wi-Fi, fetch live market data, or schedule periodic refreshes. See the [module README](StockRenderModule/README.md) for wiring and configuration details.
