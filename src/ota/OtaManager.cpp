#include "ota/OtaManager.h"

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Updater.h>

#include "display/DisplayManager.h"

namespace {
constexpr uint32_t kRebootDelayMs = 1500;
constexpr uint32_t kSecuritySpace = 0x1000;
constexpr uint32_t kBinMask = 0xFFFFF000;

String jsonString(const JsonDocument& doc) {
    String payload;
    serializeJson(doc, payload);
    return payload;
}
}  // namespace

OtaManager::OtaManager()
    : inProgress_(false),
      error_(false),
      cancelRequested_(false),
      written_(0),
      total_(0),
      rebootScheduled_(false),
      rebootAtMs_(0) {}

void OtaManager::attachLegacy(ESP8266WebServer& server, const String& path) { legacyUpdater_.setup(&server, path); }

void OtaManager::registerApi(ESP8266WebServer& server, const ConfigManager& config) {
    server.on("/api/v1/ota/status", HTTP_GET, [&server, this]() {
        JsonDocument doc;
        doc["inProgress"] = inProgress_;
        doc["error"] = error_;
        doc["bytesWritten"] = static_cast<uint32_t>(written_);
        doc["totalBytes"] = static_cast<uint32_t>(total_);
        doc["message"] = status_;
        doc["rebootScheduled"] = rebootScheduled_;

        server.send(200, "application/json", jsonString(doc));
    });

    server.on("/api/v1/ota/cancel", HTTP_POST, [&server, this, &config]() {
        if (!isAuthorized(server, config)) {
            server.send(401, "application/json", "{\"error\":\"unauthorized\"}");
            return;
        }

        cancelRequested_ = true;
        server.send(200, "application/json", "{\"ok\":true}");
    });

    server.on(
        "/api/v1/ota/fw", HTTP_POST,
        [&server, this, &config]() {
            if (!isAuthorized(server, config)) {
                server.send(401, "application/json", "{\"error\":\"unauthorized\"}");
                return;
            }

            JsonDocument doc;
            doc["ok"] = !error_;
            doc["inProgress"] = inProgress_;
            doc["message"] = status_;
            server.send(error_ ? 500 : 200, "application/json", jsonString(doc));
        },
        [&server, this, &config]() {
            if (!isAuthorized(server, config)) {
                return;
            }

            HTTPUpload& upload = server.upload();

            switch (upload.status) {
                case UPLOAD_FILE_START:
                    handleUploadStart(upload);
                    break;
                case UPLOAD_FILE_WRITE:
                    handleUploadWrite(upload);
                    break;
                case UPLOAD_FILE_END:
                    handleUploadEnd();
                    break;
                case UPLOAD_FILE_ABORTED:
                    handleUploadAbort();
                    break;
                default:
                    break;
            }
        });
}

void OtaManager::loop() {
    if (rebootScheduled_ && millis() >= rebootAtMs_) {
        ESP.restart();
    }
}

bool OtaManager::inProgress() const { return inProgress_; }

bool OtaManager::hasError() const { return error_; }

const String& OtaManager::status() const { return status_; }

void OtaManager::handleUploadStart(HTTPUpload& upload) {
    inProgress_ = true;
    error_ = false;
    cancelRequested_ = false;
    written_ = 0;
    total_ = static_cast<size_t>(upload.contentLength);
    status_ = "Uploading firmware";

    const size_t maxSketchSpace =
        (ESP.getFreeSketchSpace() - kSecuritySpace) & kBinMask;  // NOLINT(readability-static-accessed-through-instance)

    if (!Update.begin(maxSketchSpace, U_FLASH)) {
        error_ = true;
        inProgress_ = false;
        status_ = Update.getErrorString();
        return;
    }

    DisplayManager::showMessage("OTA", "Uploading...");
    DisplayManager::showOtaProgress(0, total_);
}

void OtaManager::handleUploadWrite(HTTPUpload& upload) {
    if (!inProgress_ || error_) {
        return;
    }

    if (cancelRequested_) {
        Update.end();
        error_ = true;
        inProgress_ = false;
        status_ = "Canceled";
        DisplayManager::showMessage("OTA canceled");
        return;
    }

    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        error_ = true;
        inProgress_ = false;
        status_ = Update.getErrorString();
        DisplayManager::showMessage("OTA error", status_);
        return;
    }

    written_ += upload.currentSize;
    DisplayManager::showOtaProgress(written_, total_);
}

void OtaManager::handleUploadEnd() {
    if (error_) {
        return;
    }

    if (!Update.end(true)) {
        error_ = true;
        inProgress_ = false;
        status_ = Update.getErrorString();
        DisplayManager::showMessage("OTA error", status_);
        return;
    }

    inProgress_ = false;
    status_ = "Update complete";
    rebootScheduled_ = true;
    rebootAtMs_ = millis() + kRebootDelayMs;
    DisplayManager::showMessage("OTA success", "Rebooting...");
}

void OtaManager::handleUploadAbort() {
    Update.end();
    inProgress_ = false;
    error_ = true;
    status_ = "Upload aborted";
    DisplayManager::showMessage("OTA aborted");
}

bool OtaManager::isAuthorized(ESP8266WebServer& server, const ConfigManager& config) const {
    const String token = config.apiToken();
    if (token.isEmpty()) {
        return true;
    }

    const String auth = server.header("Authorization");
    if (!auth.startsWith("Bearer ")) {
        return false;
    }

    return auth.substring(7) == token;
}
