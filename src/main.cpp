#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>

#include "config/ConfigManager.h"
#include "display/DisplayManager.h"
#include "ota/OtaManager.h"
#include "project_version.h"
#include "web/Routes.h"
#include "wireless/WiFiManager.h"

namespace {
constexpr uint32_t kSerialBaudRate = 115200;
constexpr uint32_t kBootDelayMs = 200;
constexpr const char* kLegacyOtaPath = "/legacyupdate";

ConfigManager gConfig;
WiFiManager gWifi;
OtaManager gOta;
ESP8266WebServer gWebServer(80);

void showCurrentNetworkOnDisplay() {
  DisplayManager::showNetworkStatus(gWifi.isApMode(), gWifi.activeSsid(), gWifi.ip().toString());
}
}  // namespace

void setup() {
  Serial.begin(kSerialBaudRate);
  delay(kBootDelayMs);
  Serial.println();
  Serial.println(String("GeekMagic Open Firmware ") + PROJECT_VER_STR);

  DisplayManager::begin();
  DisplayManager::showBoot();

  if (!LittleFS.begin()) {
    DisplayManager::showMessage("LittleFS error", "Mount failed");
    return;
  }

  if (!gConfig.load()) {
    DisplayManager::showMessage("Config", "Using defaults");
  }

  gWifi.begin(gConfig.wifiSsid(), gConfig.wifiPassword());

  registerRoutes(gWebServer, gConfig, gWifi, gOta);
  gOta.attachLegacy(gWebServer, kLegacyOtaPath);

  gWebServer.begin();
  showCurrentNetworkOnDisplay();

  EspClass::wdtEnable(WDTO_2S);
}

void loop() {
  gWebServer.handleClient();
  gWifi.processDns();
  gWifi.update();
  gOta.loop();

  static bool wasApMode = false;
  const bool apMode = gWifi.isApMode();
  if (apMode != wasApMode) {
    wasApMode = apMode;
    showCurrentNetworkOnDisplay();
  }

  EspClass::wdtFeed();
  delay(2);
}
