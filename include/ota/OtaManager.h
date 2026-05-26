#pragma once

#include <Arduino.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>

#include "config/ConfigManager.h"

class OtaManager {
 public:
  OtaManager();

  void attachLegacy(ESP8266WebServer& server, const String& path = "/legacyupdate");
  void registerApi(ESP8266WebServer& server, const ConfigManager& config);
  void loop();

  bool inProgress() const;
  bool hasError() const;
  const String& status() const;

 private:
  void handleUploadStart(HTTPUpload& upload);
  void handleUploadWrite(HTTPUpload& upload);
  void handleUploadEnd();
  void handleUploadAbort();

  bool isAuthorized(ESP8266WebServer& server, const ConfigManager& config) const;

  ESP8266HTTPUpdateServer legacyUpdater_;

  bool inProgress_;
  bool error_;
  bool cancelRequested_;
  size_t written_;
  size_t total_;
  String status_;

  bool rebootScheduled_;
  uint32_t rebootAtMs_;
};
