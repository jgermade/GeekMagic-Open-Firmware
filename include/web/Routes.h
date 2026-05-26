#pragma once

#include <ESP8266WebServer.h>

#include "config/ConfigManager.h"
#include "ota/OtaManager.h"
#include "wireless/WiFiManager.h"

void registerRoutes(ESP8266WebServer& server, ConfigManager& config, WiFiManager& wifiManager, OtaManager& otaManager);
