#ifndef STOCK_RENDER_CONFIG_H
#define STOCK_RENDER_CONFIG_H

// Panel geometry and packed pixel format.
#define STOCK_RENDER_DISPLAY_WIDTH 200
#define STOCK_RENDER_DISPLAY_HEIGHT 200

// Counter-clockwise display rotation in degrees: 0, 90, 180, or 270.
#define STOCK_RENDER_ROTATION 90

// Rendering backend: 0 streams bytes; 1 allocates two planes in SRAM
// (10 KB at 200x200). Keep 0 on Uno/Nano; 1 needs a board with enough RAM.
#define STOCK_RENDER_USE_FRAMEBUFFER 0

// E-paper module GPIO connections (SPI data/clock use the board's SPI pins).
#define STOCK_RENDER_PIN_RST 8
#define STOCK_RENDER_PIN_DC 9
#define STOCK_RENDER_PIN_CS 10
#define STOCK_RENDER_PIN_BUSY 7
#define STOCK_RENDER_PIN_POWER 6

// SPI clock frequency in hertz.
#define STOCK_RENDER_SPI_CLOCK_HZ 2000000

#define STOCK_RENDER_SERIAL_BAUD 115200

#endif
