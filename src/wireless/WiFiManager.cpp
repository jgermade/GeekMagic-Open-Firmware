#include "wireless/WiFiManager.h"

namespace {
constexpr uint32_t kInitialConnectTimeoutMs = 15000;
constexpr byte kDnsPort = 53;
constexpr const char* kDnsWildcard = "*";
}  // namespace

WiFiManager::WiFiManager() : apSsid_("GeekMagic-" + String(ESP.getChipId(), HEX)), state_(State::Boot) {}

void WiFiManager::begin(const String& staSsid, const String& staPassword) {
  staSsid_ = staSsid;
  staPassword_ = staPassword;

  if (staSsid_.isEmpty()) {
    startCaptivePortal();
    return;
  }

  if (!connectStation(kInitialConnectTimeoutMs)) {
    startCaptivePortal();
  }
}

void WiFiManager::update() {
  if (state_ == State::StaConnected && WiFi.status() != WL_CONNECTED) {
    startCaptivePortal();
  }
}

void WiFiManager::processDns() {
  if (state_ == State::ApPortal) {
    dnsServer_.processNextRequest();
  }
}

bool WiFiManager::connectToNetwork(const String& ssid, const String& password, uint32_t timeoutMs) {
  staSsid_ = ssid;
  staPassword_ = password;

  stopCaptivePortal();
  if (connectStation(timeoutMs)) {
    return true;
  }

  startCaptivePortal();
  return false;
}

void WiFiManager::scanNetworks(JsonArray& output) {
  const int count = WiFi.scanNetworks();
  for (int i = 0; i < count; ++i) {
    JsonObject network = output.add<JsonObject>();
    network["ssid"] = WiFi.SSID(i);
    network["rssi"] = WiFi.RSSI(i);
    network["enc"] = static_cast<int>(WiFi.encryptionType(i));
  }
  WiFi.scanDelete();
}

bool WiFiManager::isApMode() const { return state_ == State::ApPortal; }

WiFiManager::State WiFiManager::state() const { return state_; }

String WiFiManager::activeSsid() const {
  return isApMode() ? apSsid_ : WiFi.SSID();
}

IPAddress WiFiManager::ip() const {
  return isApMode() ? WiFi.softAPIP() : WiFi.localIP();
}

String WiFiManager::apSsid() const { return apSsid_; }

bool WiFiManager::connectStation(uint32_t timeoutMs) {
  state_ = State::StaConnecting;

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(staSsid_.c_str(), staPassword_.c_str());

  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeoutMs) {
    delay(150);
    yield();
  }

  if (WiFi.status() == WL_CONNECTED) {
    state_ = State::StaConnected;
    return true;
  }

  return false;
}

void WiFiManager::startCaptivePortal() {
  WiFi.disconnect();
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSsid_.c_str());

  dnsServer_.stop();
  dnsServer_.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer_.start(kDnsPort, kDnsWildcard, WiFi.softAPIP());

  state_ = State::ApPortal;
}

void WiFiManager::stopCaptivePortal() {
  dnsServer_.stop();
  WiFi.softAPdisconnect(true);
}
