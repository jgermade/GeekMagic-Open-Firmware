#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESP8266WiFi.h>

class WiFiManager {
 public:
  enum class State {
    Boot,
    StaConnecting,
    StaConnected,
    ApPortal,
  };

  WiFiManager();

  void begin(const String& staSsid, const String& staPassword);
  void update();
  void processDns();

  bool connectToNetwork(const String& ssid, const String& password, uint32_t timeoutMs);
  void scanNetworks(JsonArray& output);

  bool isApMode() const;
  State state() const;
  String activeSsid() const;
  IPAddress ip() const;
  String apSsid() const;

 private:
  bool connectStation(uint32_t timeoutMs);
  void startCaptivePortal();
  void stopCaptivePortal();

  String staSsid_;
  String staPassword_;
  String apSsid_;

  DNSServer dnsServer_;
  State state_;
};
