#pragma once

#include <Arduino.h>

class ConfigManager {
 public:
  struct Data {
    String wifiSsid;
    String wifiPassword;
    String apiToken;
  };

  explicit ConfigManager(const char* filePath = "/config.json");

  bool load();
  bool save();

  void setWiFi(const String& ssid, const String& password);
  void setApiToken(const String& token);

  const String& wifiSsid() const;
  const String& wifiPassword() const;
  const String& apiToken() const;

 private:
  const char* filePath_;
  Data data_;

  void applyDefaults();
};
