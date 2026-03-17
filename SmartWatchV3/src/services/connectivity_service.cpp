/**
 * Connectivity Service — Implementation
 */

#include "connectivity_service.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <Arduino.h>

static WebServer *_alertServer = NULL;
static bool _serverRunning = false;

// Callback for priority alert (set from main code)
static void (*_alertCallback)(const char *message) = NULL;

void ConnectivityService::init(void) {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
}

bool ConnectivityService::connectWiFi(const char *ssid, const char *password) {
    Serial.printf("WiFi: connecting to %s...\n", ssid);
    WiFi.begin(ssid, password);

    // Wait up to 10 seconds
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
        delay(100);
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("WiFi: connected, IP: %s\n", WiFi.localIP().toString().c_str());
        return true;
    }

    Serial.println("WiFi: connection failed");
    return false;
}

void ConnectivityService::disconnectWiFi(void) {
    WiFi.disconnect();
    Serial.println("WiFi: disconnected");
}

bool ConnectivityService::isWiFiConnected(void) {
    return WiFi.status() == WL_CONNECTED;
}

bool ConnectivityService::sendCompressorCommand(const char *ip, bool state) {
    if (!isWiFiConnected()) {
        Serial.println("HTTP: WiFi not connected");
        return false;
    }

    HTTPClient http;
    char url[64];
    snprintf(url, sizeof(url), "http://%s/control", ip);

    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(5000);

    char payload[64];
    snprintf(payload, sizeof(payload), "{\"compressor\":%s}", state ? "true" : "false");

    int code = http.POST(payload);
    http.end();

    Serial.printf("HTTP POST %s -> %d\n", url, code);
    return code >= 200 && code < 300;
}

void ConnectivityService::startAlertServer(uint16_t port) {
    if (_serverRunning) return;

    _alertServer = new WebServer(port);
    _alertServer->on("/alert", HTTP_POST, []() {
        String body = _alertServer->arg("plain");
        Serial.printf("Alert received: %s\n", body.c_str());
        _alertServer->send(200, "text/plain", "OK");

        // Trigger priority alert via callback
        if (_alertCallback) {
            _alertCallback(body.c_str());
        }
    });

    _alertServer->begin();
    _serverRunning = true;
    Serial.printf("Alert server started on port %d\n", port);
}

void ConnectivityService::stopAlertServer(void) {
    if (!_serverRunning) return;
    _alertServer->stop();
    delete _alertServer;
    _alertServer = NULL;
    _serverRunning = false;
}

void ConnectivityService::handleAlertServer(void) {
    if (_serverRunning && _alertServer) {
        _alertServer->handleClient();
    }
}
