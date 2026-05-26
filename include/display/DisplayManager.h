#pragma once

#include <Arduino.h>

class DisplayManager {
 public:
  static void begin();
  static void showBoot();
  static void showNetworkStatus(bool apMode, const String& ssid, const String& ip);
  static void showOtaProgress(size_t current, size_t total);
  static void showMessage(const String& title, const String& body = "");

 private:
  static void drawStatusBar(float progress, uint16_t color);
};
