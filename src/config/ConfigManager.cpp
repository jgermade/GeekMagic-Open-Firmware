#include "config/ConfigManager.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

namespace {
constexpr const char* kDefaultApiToken = "changeme";
}  // namespace

ConfigManager::ConfigManager(const char* filePath) : filePath_(filePath) { applyDefaults(); }

bool ConfigManager::load() {
  applyDefaults();

  if (!LittleFS.exists(filePath_)) {
    return save();
  }

  File file = LittleFS.open(filePath_, "r");
  if (!file) {
    return false;
  }

  JsonDocument doc;
  const DeserializationError err = deserializeJson(doc, file);
  file.close();
  if (err) {
    return false;
  }

  data_.wifiSsid = doc["wifi_ssid"] | "";
  data_.wifiPassword = doc["wifi_password"] | "";
  data_.apiToken = doc["api_token"] | kDefaultApiToken;
  if (data_.apiToken.isEmpty()) {
    data_.apiToken = kDefaultApiToken;
  }

  return true;
}

bool ConfigManager::save() {
  JsonDocument doc;
  doc["wifi_ssid"] = data_.wifiSsid;
  doc["wifi_password"] = data_.wifiPassword;
  doc["api_token"] = data_.apiToken;

  File file = LittleFS.open(filePath_, "w");
  if (!file) {
    return false;
  }

  const size_t written = serializeJsonPretty(doc, file);
  file.close();

  return written > 0;
}

void ConfigManager::setWiFi(const String& ssid, const String& password) {
  data_.wifiSsid = ssid;
  data_.wifiPassword = password;
}

void ConfigManager::setApiToken(const String& token) {
  if (token.isEmpty()) {
    data_.apiToken = kDefaultApiToken;
    return;
  }

  data_.apiToken = token;
}

const String& ConfigManager::wifiSsid() const { return data_.wifiSsid; }

const String& ConfigManager::wifiPassword() const { return data_.wifiPassword; }

const String& ConfigManager::apiToken() const { return data_.apiToken; }

void ConfigManager::applyDefaults() {
  data_.wifiSsid = "";
  data_.wifiPassword = "";
  data_.apiToken = kDefaultApiToken;
}
