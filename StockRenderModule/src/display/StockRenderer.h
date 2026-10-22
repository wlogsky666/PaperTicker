#ifndef STOCK_RENDERER_H
#define STOCK_RENDERER_H

#include "../../StockRenderConfig.h"
#include <Arduino.h>
#include <limits.h>
#include <stdlib.h>

namespace stockrender {

// Geometry and packed plane size for the configured panel.
static const uint16_t DISPLAY_WIDTH = STOCK_RENDER_DISPLAY_WIDTH;
static const uint16_t DISPLAY_HEIGHT = STOCK_RENDER_DISPLAY_HEIGHT;
static const uint8_t PLANE_COUNT = 2;
static const size_t PLANE_BYTES =
    (size_t)DISPLAY_WIDTH * DISPLAY_HEIGHT / CHAR_BIT;
static const int16_t CHART_PLOT_PADDING = 3;

// Physical display plane that receives a rendered foreground pixel.
enum Plane : uint8_t { BlackPlane = 0, RedPlane = 1 };

// Quote text and normalized intraday data used to render one dashboard cell.
// Keep the pointed-to strings and sparkline alive while rendering; values in
// sparkline and previousCloseLevel are normalized from 0 through 100.
struct Stock {
  const char *symbol;
  const char *price;
  const char *change;
  bool changeIsRed;
  // Yesterday's close position on the same normalized vertical chart scale.
  uint8_t previousCloseLevel;
  const uint8_t *sparkline;
  // Valid prefix length and fixed full-timeline storage length, respectively.
  uint8_t sparklineCount;
  uint8_t sparklineCapacity;
};

// Dashboard grid and drawing dimensions. columns and rows must be nonzero.
struct Layout {
  uint8_t columns;
  uint8_t rows;
  uint8_t symbolScale;
  uint8_t chartHeight;
};

// Result for one coordinate. black=false means white; otherwise plane selects
// which of the two color planes receives the pixel.
struct Pixel {
  int16_t x;
  int16_t y;
  Plane plane;
  bool black;
};

class PixelSource {
public:
  // Build a pixel source over stockCount entries arranged using layout.
  // The stocks and their referenced data must remain valid during rendering.
  PixelSource(const Stock *stocks, uint8_t stockCount, const Layout &layout)
      : stocks_(stocks), stockCount_(stockCount), layout_(layout) {}

  // Compute the color assignment at a display coordinate; out-of-range
  // coordinates and unused cells are white.
  Pixel pixelAt(int16_t x, int16_t y) const {
    Pixel result = {x, y, BlackPlane, false};
    if (x < 0 || y < 0 || x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT ||
        layout_.columns == 0 || layout_.rows == 0)
      return result;

#if STOCK_RENDER_ROTATION == 90
    int16_t unrotatedX = DISPLAY_WIDTH - 1 - y;
    y = x;
    x = unrotatedX;
#elif STOCK_RENDER_ROTATION == 180
    x = DISPLAY_WIDTH - 1 - x;
    y = DISPLAY_HEIGHT - 1 - y;
#elif STOCK_RENDER_ROTATION == 270
    int16_t unrotatedX = y;
    y = DISPLAY_HEIGHT - 1 - x;
    x = unrotatedX;
#endif

    if (x == 0 || y == 0 || x == DISPLAY_WIDTH - 1 || y == DISPLAY_HEIGHT - 1) {
      result.black = true;
      return result;
    }

    int16_t cellWidth = DISPLAY_WIDTH / layout_.columns;
    int16_t cellHeight = DISPLAY_HEIGHT / layout_.rows;
    uint8_t col = x / cellWidth;
    uint8_t row = y / cellHeight;
    uint16_t index = (uint16_t)row * layout_.columns + col;

    if ((x % cellWidth) == cellWidth - 1 ||
        (y % cellHeight) == cellHeight - 1) {
      result.black = true;
      return result;
    }
    if (index >= stockCount_)
      return result;

    const Stock &stock = stocks_[index];
    int16_t localX = x % cellWidth;
    int16_t localY = y % cellHeight;
    int16_t chartY = cellHeight - layout_.chartHeight - 4;

    if (drawText(stock.symbol, 4, 10, layout_.symbolScale, localX, localY)) {
      result.black = true;
      return result;
    }
    if (drawText(stock.price, 54, 8, 1, localX, localY)) {
      result.black = true;
      return result;
    }
    if (drawText(stock.change, 54, 20, 1, localX, localY)) {
      result.plane = stock.changeIsRed ? RedPlane : BlackPlane;
      result.black = true;
      return result;
    }

    int16_t chartX = 4;
    int16_t chartWidth = cellWidth - 10;
    if (closeReferencePixel(stock, chartX, chartY, chartWidth,
                            layout_.chartHeight, localX, localY)) {
      result.plane = RedPlane;
      result.black = true;
      return result;
    }

    result.black = chartPixel(stock, chartX, chartY, chartWidth,
                              layout_.chartHeight, localX, localY);
    return result;
  }

private:
  const Stock *stocks_;
  uint8_t stockCount_;
  Layout layout_;

  static bool drawText(const char *text, int16_t originX, int16_t originY,
                       uint8_t scale, int16_t x, int16_t y) {
    if (text == NULL || scale == 0)
      return false;
    int16_t penX = originX;
    for (const char *p = text; *p; ++p, penX += 6 * scale) {
      if (x < penX || x >= penX + 5 * scale || y < originY ||
          y >= originY + 7 * scale)
        continue;
      return glyphPixel(*p, (x - penX) / scale, (y - originY) / scale);
    }
    return false;
  }

  static bool glyphPixel(char character, int column, int row) {
    static const uint8_t digits[10][5] = {
        {0x3E, 0x51, 0x49, 0x45, 0x3E}, {0x00, 0x42, 0x7F, 0x40, 0x00},
        {0x42, 0x61, 0x51, 0x49, 0x46}, {0x21, 0x41, 0x45, 0x4B, 0x31},
        {0x18, 0x14, 0x12, 0x7F, 0x10}, {0x27, 0x45, 0x45, 0x45, 0x39},
        {0x3C, 0x4A, 0x49, 0x49, 0x30}, {0x01, 0x71, 0x09, 0x05, 0x03},
        {0x36, 0x49, 0x49, 0x49, 0x36}, {0x06, 0x49, 0x49, 0x29, 0x1E}};
    static const uint8_t letters[26][5] = {
        {0x7E, 0x11, 0x11, 0x11, 0x7E}, {0x7F, 0x49, 0x49, 0x49, 0x36},
        {0x3E, 0x41, 0x41, 0x41, 0x22}, {0x7F, 0x41, 0x41, 0x22, 0x1C},
        {0x7F, 0x49, 0x49, 0x49, 0x41}, {0x7F, 0x09, 0x09, 0x09, 0x01},
        {0x3E, 0x41, 0x49, 0x49, 0x7A}, {0x7F, 0x08, 0x08, 0x08, 0x7F},
        {0x00, 0x41, 0x7F, 0x41, 0x00}, {0x20, 0x40, 0x41, 0x3F, 0x01},
        {0x7F, 0x08, 0x14, 0x22, 0x41}, {0x7F, 0x40, 0x40, 0x40, 0x40},
        {0x7F, 0x02, 0x0C, 0x02, 0x7F}, {0x7F, 0x04, 0x08, 0x10, 0x7F},
        {0x3E, 0x41, 0x41, 0x41, 0x3E}, {0x7F, 0x09, 0x09, 0x09, 0x06},
        {0x3E, 0x41, 0x51, 0x21, 0x5E}, {0x7F, 0x09, 0x19, 0x29, 0x46},
        {0x46, 0x49, 0x49, 0x49, 0x31}, {0x01, 0x01, 0x7F, 0x01, 0x01},
        {0x3F, 0x40, 0x40, 0x40, 0x3F}, {0x1F, 0x20, 0x40, 0x20, 0x1F},
        {0x3F, 0x40, 0x38, 0x40, 0x3F}, {0x63, 0x14, 0x08, 0x14, 0x63},
        {0x07, 0x08, 0x70, 0x08, 0x07}, {0x61, 0x51, 0x49, 0x45, 0x43}};
    static const uint8_t dollar[5] = {0x24, 0x2A, 0x7F, 0x2A, 0x12};
    static const uint8_t plus[5] = {0x08, 0x08, 0x3E, 0x08, 0x08};
    static const uint8_t minus[5] = {0x08, 0x08, 0x08, 0x08, 0x08};
    static const uint8_t dot[5] = {0x00, 0x60, 0x60, 0x00, 0x00};
    static const uint8_t percent[5] = {0x23, 0x13, 0x08, 0x64, 0x62};
    const uint8_t *glyph = NULL;

    if (column < 0 || column >= 5 || row < 0 || row >= 7)
      return false;
    if (character >= '0' && character <= '9')
      glyph = digits[character - '0'];
    else if (character >= 'A' && character <= 'Z')
      glyph = letters[character - 'A'];
    else if (character >= 'a' && character <= 'z')
      glyph = letters[character - 'a'];
    else if (character == '$')
      glyph = dollar;
    else if (character == '+')
      glyph = plus;
    else if (character == '-')
      glyph = minus;
    else if (character == '.')
      glyph = dot;
    else if (character == '%')
      glyph = percent;
    return glyph != NULL && (glyph[column] & (1 << row)) != 0;
  }

  static int16_t sparkY(const Stock &stock, int16_t x, int16_t width,
                        int16_t height) {
    if (stock.sparkline == NULL || stock.sparklineCount < 2 ||
        stock.sparklineCapacity < stock.sparklineCount || width <= 3 ||
        height <= 2 * CHART_PLOT_PADDING + 1)
      return -1;
    int16_t innerWidth = width - 2;
    int16_t boundedX = x < 0 ? 0 : (x >= innerWidth ? innerWidth - 1 : x);
    uint16_t timelineWidth = innerWidth - 1;
    uint32_t scaled = (uint32_t)boundedX * (stock.sparklineCapacity - 1);
    uint8_t index = scaled / timelineWidth;
    uint32_t remainder = scaled % timelineWidth;
    if (index >= stock.sparklineCount - 1) {
      if (index != stock.sparklineCount - 1 || remainder != 0)
        return -1;
      index = stock.sparklineCount - 2;
      remainder = timelineWidth;
    }
    int32_t first = stock.sparkline[index];
    int32_t second = stock.sparkline[index + 1];
    int32_t value =
        first + (second - first) * (int32_t)remainder / timelineWidth;
    if (value > 100)
      value = 100;
    int16_t plotHeight = height - 2 * CHART_PLOT_PADDING;
    return CHART_PLOT_PADDING + plotHeight - 1 - value * (plotHeight - 1) / 100;
  }

  static bool chartPixel(const Stock &stock, int16_t chartX, int16_t chartY,
                         int16_t width, int16_t height, int16_t x, int16_t y) {
    if (x < chartX || x >= chartX + width || y < chartY ||
        y >= chartY + height || width < 3 || height < 3)
      return false;
    int16_t localX = x - chartX;
    int16_t localY = y - chartY;
    if (localX == 0 || localX == width - 1 || localY == 0 ||
        localY == height - 1)
      return true;
    if (stock.sparkline == NULL || stock.sparklineCount < 2 ||
        stock.sparklineCapacity < stock.sparklineCount)
      return false;
    int16_t lineY = sparkY(stock, localX - 1, width, height);
    if (lineY < 0)
      return false;
    return abs(localY - lineY) <= 1;
  }

  static bool closeReferencePixel(const Stock &stock, int16_t chartX,
                                  int16_t chartY, int16_t width, int16_t height,
                                  int16_t x, int16_t y) {
    if (x <= chartX || x >= chartX + width - 1 || y <= chartY ||
        y >= chartY + height - 1 || height <= 2 * CHART_PLOT_PADDING + 1)
      return false;
    int16_t level =
        stock.previousCloseLevel > 100 ? 100 : stock.previousCloseLevel;
    int16_t plotHeight = height - 2 * CHART_PLOT_PADDING;
    int16_t referenceY = chartY + CHART_PLOT_PADDING + plotHeight - 1 -
                         level * (plotHeight - 1) / 100;
    return y == referenceY && ((x - chartX - 1) % 6) < 3;
  }
};

class StreamSink {
public:
  // Create a byte-at-a-time output sink. The callback receives each packed
  // byte and plane, with red-plane polarity already converted for this panel.
  explicit StreamSink(void (*writeByte)(Plane, uint8_t))
      : writeByte_(writeByte) {}

  // Render black then red, forwarding each byte immediately to the callback.
  template <typename Renderer> void render(const Renderer &renderer) {
    for (uint8_t plane = 0; plane < PLANE_COUNT; ++plane) {
      Plane selected = (Plane)plane;
      for (uint16_t y = 0; y < DISPLAY_HEIGHT; ++y) {
        for (uint16_t byteX = 0; byteX < DISPLAY_WIDTH / CHAR_BIT; ++byteX) {
          uint8_t output = 0xFF;
          for (uint8_t bit = 0; bit < CHAR_BIT; ++bit) {
            Pixel pixel = renderer.pixelAt(byteX * CHAR_BIT + bit, y);
            if (pixel.black && pixel.plane == selected)
              output &= ~(0x80 >> bit);
          }
          if (writeByte_ != NULL)
            writeByte_(selected,
                       selected == RedPlane ? (uint8_t)~output : output);
        }
      }
    }
  }

private:
  void (*writeByte_)(Plane, uint8_t);
};

class FramebufferSink {
public:
  // Use caller-owned plane buffers, each at least PLANE_BYTES in size. The
  // buffers are not allocated or freed by the sink.
  FramebufferSink(uint8_t *blackPlane, uint8_t *redPlane)
      : blackPlane_(blackPlane), redPlane_(redPlane) {}

  // Render both planes into the supplied buffers. Red-plane polarity is
  // converted to the panel's transfer format, matching StreamSink output.
  template <typename Renderer> void render(const Renderer &renderer) {
    for (uint8_t plane = 0; plane < PLANE_COUNT; ++plane) {
      Plane selected = (Plane)plane;
      uint8_t *destination = selected == BlackPlane ? blackPlane_ : redPlane_;
      for (uint16_t y = 0; y < DISPLAY_HEIGHT; ++y) {
        for (uint16_t byteX = 0; byteX < DISPLAY_WIDTH / CHAR_BIT; ++byteX) {
          uint8_t output = 0xFF;
          for (uint8_t bit = 0; bit < CHAR_BIT; ++bit) {
            Pixel pixel = renderer.pixelAt(byteX * CHAR_BIT + bit, y);
            if (pixel.black && pixel.plane == selected)
              output &= ~(0x80 >> bit);
          }
          size_t offset = (size_t)y * (DISPLAY_WIDTH / CHAR_BIT) + byteX;
          destination[offset] =
              selected == RedPlane ? (uint8_t)~output : output;
        }
      }
    }
  }

private:
  uint8_t *blackPlane_;
  uint8_t *redPlane_;
};

} // namespace stockrender

#endif
