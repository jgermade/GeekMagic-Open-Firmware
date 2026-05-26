#include "web/Routes.h"

#include <ArduinoJson.h>

#include "display/DisplayManager.h"

namespace {
String toJson(const JsonDocument& doc) {
  String out;
  serializeJson(doc, out);
  return out;
}

const char kPortalHtml[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8" />
<meta name="viewport" content="width=device-width, initial-scale=1" />
<title>GeekMagic Setup</title>
<style>
:root { --bg:#0b1320; --card:#132238; --fg:#f4f8ff; --accent:#3ddc97; --warn:#ffb454; }
body { margin:0; font-family:ui-rounded, system-ui, sans-serif; background:radial-gradient(circle at 20% 0%, #20385f, var(--bg)); color:var(--fg); }
main { max-width:460px; margin:8vh auto; padding:1.2rem; }
.card { background:var(--card); border-radius:16px; padding:1rem; box-shadow:0 12px 30px rgba(0,0,0,0.35); }
h1 { margin-top:0; font-size:1.4rem; }
label { display:block; margin:.8rem 0 .3rem; font-size:.9rem; opacity:.9; }
input { width:100%; padding:.75rem; border-radius:10px; border:1px solid #2b4467; background:#0d1b2e; color:var(--fg); }
button { margin-top:1rem; width:100%; padding:.8rem; border:none; border-radius:10px; background:var(--accent); color:#042614; font-weight:700; }
#status { min-height:1.3rem; margin-top:.8rem; color:var(--warn); }
.small { opacity:.8; font-size:.85rem; margin-top:1rem; }
</style>
</head>
<body>
<main>
  <section class="card">
    <h1>GeekMagic WiFi Setup</h1>
    <p>Connect device to your WiFi, then it will leave portal mode automatically.</p>
    <label for="ssid">WiFi SSID</label>
    <input id="ssid" autocomplete="off" />
    <label for="pass">WiFi Password</label>
    <input id="pass" type="password" autocomplete="off" />
    <button id="save">Connect</button>
    <div id="status"></div>
    <div class="small">AP is temporary and open only during setup.</div>
  </section>
</main>
<script>
const statusEl = document.getElementById('status');
document.getElementById('save').addEventListener('click', async () => {
  statusEl.textContent = 'Connecting...';
  const body = {
    ssid: document.getElementById('ssid').value,
    password: document.getElementById('pass').value,
  };
  try {
    const res = await fetch('/api/v1/wifi/connect', {
      method: 'POST',
      headers: { 'content-type': 'application/json' },
      body: JSON.stringify(body),
    });
    const data = await res.json();
    if (data.status === 'connected') {
      statusEl.style.color = '#3ddc97';
      statusEl.textContent = 'Connected. IP: ' + data.ip;
      return;
    }
    statusEl.textContent = data.message || 'Failed to connect';
  } catch (e) {
    statusEl.textContent = 'Request failed';
  }
});
</script>
</body>
</html>
)HTML";

void sendPortal(ESP8266WebServer& server) { server.send_P(200, "text/html", kPortalHtml); }

bool isCaptivePath(const String& path) {
  return path == "/" || path == "/generate_204" || path == "/hotspot-detect.html" || path == "/fwlink";
}
}  // namespace

void registerRoutes(ESP8266WebServer& server, ConfigManager& config, WiFiManager& wifiManager, OtaManager& otaManager) {
  server.collectHeaders("Authorization");

  otaManager.registerApi(server, config);

  server.on("/api/v1/wifi/status", HTTP_GET, [&server, &wifiManager]() {
    JsonDocument doc;
    const bool connected = !wifiManager.isApMode() && WiFi.status() == WL_CONNECTED;

    doc["connected"] = connected;
    doc["apMode"] = wifiManager.isApMode();
    doc["ssid"] = wifiManager.activeSsid();
    doc["ip"] = wifiManager.ip().toString();

    server.send(200, "application/json", toJson(doc));
  });

  server.on("/api/v1/wifi/scan", HTTP_GET, [&server, &wifiManager]() {
    JsonDocument doc;
    JsonArray networks = doc["networks"].to<JsonArray>();
    wifiManager.scanNetworks(networks);
    server.send(200, "application/json", toJson(doc));
  });

  server.on("/api/v1/wifi/connect", HTTP_POST, [&server, &wifiManager, &config]() {
    JsonDocument input;
    const DeserializationError err = deserializeJson(input, server.arg("plain"));
    if (err) {
      server.send(400, "application/json", "{\"error\":\"invalid-json\"}");
      return;
    }

    const String ssid = input["ssid"] | "";
    const String password = input["password"] | "";

    if (ssid.isEmpty()) {
      server.send(400, "application/json", "{\"error\":\"ssid-required\"}");
      return;
    }

    DisplayManager::showMessage("WiFi", "Connecting...");
    const bool ok = wifiManager.connectToNetwork(ssid, password, 20000);

    JsonDocument doc;
    doc["status"] = ok ? "connected" : "error";
    doc["ssid"] = ssid;
    if (ok) {
      config.setWiFi(ssid, password);
      config.save();
      doc["ip"] = wifiManager.ip().toString();
      DisplayManager::showNetworkStatus(false, wifiManager.activeSsid(), wifiManager.ip().toString());
    } else {
      doc["message"] = "failed-to-connect";
      DisplayManager::showNetworkStatus(true, wifiManager.activeSsid(), wifiManager.ip().toString());
    }

    server.send(ok ? 200 : 500, "application/json", toJson(doc));
  });

  server.on("/", HTTP_GET, [&server]() { sendPortal(server); });
  server.on("/generate_204", HTTP_GET, [&server]() { sendPortal(server); });
  server.on("/hotspot-detect.html", HTTP_GET, [&server]() { sendPortal(server); });
  server.on("/fwlink", HTTP_GET, [&server]() { sendPortal(server); });

  server.onNotFound([&server, &wifiManager]() {
    if (wifiManager.isApMode() && !isCaptivePath(server.uri())) {
      server.sendHeader("Location", String("http://") + wifiManager.ip().toString() + "/", true);
      server.send(302, "text/plain", "");
      return;
    }

    server.send(404, "application/json", "{\"error\":\"not-found\"}");
  });
}
