#include "display/DisplayManager.h"

#include <Arduino_GFX_Library.h>
#include <SPI.h>

namespace {
constexpr int16_t kLcdWidth = 240;
constexpr int16_t kLcdHeight = 240;
constexpr int8_t kLcdMosiPin = 13;
constexpr int8_t kLcdSckPin = 14;
constexpr int8_t kLcdDcPin = 0;
constexpr int8_t kLcdRstPin = 2;
constexpr int8_t kLcdBacklightPin = 5;
constexpr bool kLcdBacklightActiveLow = true;

constexpr uint16_t kColorBlack = 0x0000;
constexpr uint16_t kColorWhite = 0xFFFF;
constexpr uint16_t kColorGreen = 0x07E0;
constexpr uint16_t kColorOrange = 0xFD20;
constexpr uint16_t kColorBlue = 0x001F;
constexpr uint16_t kColorDarkGrey = 0x39E7;

Arduino_HWSPI gLcdBus(kLcdDcPin, -1, &SPI, true);
Arduino_ST7789 gLcd(&gLcdBus, kLcdRstPin, 0, true, kLcdWidth, kLcdHeight);

bool gDisplayReady = false;

void printLine(const String& text, int16_t y, uint8_t textSize = 2) {
  gLcd.setCursor(10, y);
  gLcd.setTextSize(textSize);
  gLcd.println(text);
}
}  // namespace

void DisplayManager::begin() {
  pinMode(kLcdBacklightPin, OUTPUT);
  digitalWrite(kLcdBacklightPin, kLcdBacklightActiveLow ? LOW : HIGH);

  SPI.begin();
  gLcd.begin();
  gLcd.setRotation(0);
  gLcd.fillScreen(kColorBlack);
  gLcd.setTextColor(kColorWhite, kColorBlack);

  gDisplayReady = true;
}

void DisplayManager::showBoot() {
  if (!gDisplayReady) {
    return;
  }

  gLcd.fillScreen(kColorBlack);
  printLine("GeekMagic", 30, 3);
  printLine("Booting...", 80, 2);
  drawStatusBar(0.15F, kColorBlue);
}

void DisplayManager::showNetworkStatus(bool apMode, const String& ssid, const String& ip) {
  if (!gDisplayReady) {
    return;
  }

  gLcd.fillScreen(kColorBlack);
  printLine(apMode ? "Mode: AP" : "Mode: STA", 20);
  printLine(String("SSID: ") + ssid, 55);
  printLine(String("IP: ") + ip, 90);
  printLine(apMode ? "Portal captive ON" : "Connected", 125);
  drawStatusBar(1.0F, apMode ? kColorOrange : kColorGreen);
}

void DisplayManager::showOtaProgress(size_t current, size_t total) {
  if (!gDisplayReady) {
    return;
  }

  gLcd.fillRect(0, 145, kLcdWidth, 75, kColorBlack);
  printLine("OTA update", 150, 2);

  const float progress = (total == 0) ? 0.0F : static_cast<float>(current) / static_cast<float>(total);
  int percent = static_cast<int>(progress * 100.0F);
  if (percent < 0) {
    percent = 0;
  }
  if (percent > 100) {
    percent = 100;
  }

  printLine(String(percent) + "%", 180, 2);
  drawStatusBar(progress, kColorGreen);
}

void DisplayManager::showMessage(const String& title, const String& body) {
  if (!gDisplayReady) {
    return;
  }

  gLcd.fillScreen(kColorBlack);
  printLine(title, 35, 2);
  if (!body.isEmpty()) {
    printLine(body, 70, 2);
  }
  drawStatusBar(1.0F, kColorDarkGrey);
}

void DisplayManager::drawStatusBar(float progress, uint16_t color) {
  if (!gDisplayReady) {
    return;
  }

  if (progress < 0.0F) {
    progress = 0.0F;
  }
  if (progress > 1.0F) {
    progress = 1.0F;
  }

  constexpr int barY = 220;
  constexpr int barH = 12;
  constexpr int barPadding = 10;
  const int barW = kLcdWidth - (barPadding * 2);
  const int fillW = static_cast<int>(barW * progress);

  gLcd.drawRect(barPadding, barY, barW, barH, kColorWhite);
  gLcd.fillRect(barPadding + 1, barY + 1, barW - 2, barH - 2, kColorBlack);
  if (fillW > 2) {
    gLcd.fillRect(barPadding + 1, barY + 1, fillW - 2, barH - 2, color);
  }
}
