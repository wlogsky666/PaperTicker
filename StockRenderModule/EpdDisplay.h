#ifndef EPD_DISPLAY_H
#define EPD_DISPLAY_H

#include <Arduino.h>

enum class EpdPlane : uint8_t { Black, Red };

// Arduino transport and Waveshare 1.54-inch B V2 panel operations.
class EpdDisplay {
public:
  // Configure pins/SPI and initialize the panel. Returns 0 on success.
  int begin();

  // Append one panel-format byte to the selected plane. Call for each plane in
  // display order, then call refresh() after the complete frame is transferred.
  void writePlaneByte(EpdPlane plane, uint8_t value);

  // Start the panel refresh and wait until the panel is idle.
  void refresh();

  // Write a white frame to both planes and refresh the panel.
  void clear();

  // Put the panel controller into sleep mode.
  void sleep();

private:
  int8_t activePlane_ = -1;

  void sendCommand(uint8_t command);
  void sendData(uint8_t data);
  void transfer(uint8_t data);
  void reset();
  void waitUntilIdle();
};

#endif
