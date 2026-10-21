#include "EpdDisplay.h"

#include "StockRenderConfig.h"
#include <SPI.h>
#include <limits.h>

int EpdDisplay::begin() {
  pinMode(STOCK_RENDER_PIN_CS, OUTPUT);
  pinMode(STOCK_RENDER_PIN_RST, OUTPUT);
  pinMode(STOCK_RENDER_PIN_DC, OUTPUT);
  pinMode(STOCK_RENDER_PIN_BUSY, INPUT);
  pinMode(STOCK_RENDER_PIN_POWER, OUTPUT);
  digitalWrite(STOCK_RENDER_PIN_POWER, HIGH);

  SPI.begin();
  SPI.beginTransaction(
      SPISettings(STOCK_RENDER_SPI_CLOCK_HZ, MSBFIRST, SPI_MODE0));
  reset();
  waitUntilIdle();

  sendCommand(0x12);
  waitUntilIdle();
  sendCommand(0x01);
  sendData(0xC7);
  sendData(0x00);
  sendData(0x01);
  sendCommand(0x11);
  sendData(0x01);
  sendCommand(0x44);
  sendData(0x00);
  sendData(0x18);
  sendCommand(0x45);
  sendData(0xC7);
  sendData(0x00);
  sendData(0x00);
  sendData(0x00);
  sendCommand(0x3C);
  sendData(0x05);
  sendCommand(0x18);
  sendData(0x80);
  sendCommand(0x4E);
  sendData(0x00);
  sendCommand(0x4F);
  sendData(0xC7);
  sendData(0x00);
  waitUntilIdle();
  activePlane_ = -1;
  return 0;
}

void EpdDisplay::writePlaneByte(EpdPlane plane, uint8_t value) {
  int8_t selectedPlane = static_cast<int8_t>(plane);
  if (activePlane_ != selectedPlane) {
    sendCommand(plane == EpdPlane::Black ? 0x24 : 0x26);
    activePlane_ = selectedPlane;
  }
  sendData(value);
}

void EpdDisplay::refresh() {
  sendCommand(0x22);
  sendData(0xF7);
  sendCommand(0x20);
  waitUntilIdle();
  activePlane_ = -1;
}

void EpdDisplay::clear() {
  const uint32_t planeBytes = (uint32_t)STOCK_RENDER_DISPLAY_WIDTH *
                              STOCK_RENDER_DISPLAY_HEIGHT / CHAR_BIT;

  sendCommand(0x24);
  for (uint32_t i = 0; i < planeBytes; ++i)
    sendData(0xFF);
  sendCommand(0x26);
  for (uint32_t i = 0; i < planeBytes; ++i)
    sendData(0x00);
  refresh();
}

void EpdDisplay::sleep() {
  sendCommand(0x10);
  sendData(0x01);
  delay(100);
}

void EpdDisplay::sendCommand(uint8_t command) {
  digitalWrite(STOCK_RENDER_PIN_DC, LOW);
  transfer(command);
}

void EpdDisplay::sendData(uint8_t data) {
  digitalWrite(STOCK_RENDER_PIN_DC, HIGH);
  transfer(data);
}

void EpdDisplay::transfer(uint8_t data) {
  digitalWrite(STOCK_RENDER_PIN_CS, LOW);
  SPI.transfer(data);
  digitalWrite(STOCK_RENDER_PIN_CS, HIGH);
}

void EpdDisplay::reset() {
  digitalWrite(STOCK_RENDER_PIN_RST, HIGH);
  delay(200);
  digitalWrite(STOCK_RENDER_PIN_RST, LOW);
  delay(10);
  digitalWrite(STOCK_RENDER_PIN_RST, HIGH);
  delay(200);
}

void EpdDisplay::waitUntilIdle() {
  while (digitalRead(STOCK_RENDER_PIN_BUSY) != LOW)
    delay(100);
}
