/**
 * Connectivity Service
 *
 * Manages WiFi connection, HTTP client for compressor control,
 * and HTTP server for receiving priority alerts.
 * ESP32-S3 supports BLE + WiFi simultaneously.
 */

#ifndef CONNECTIVITY_SERVICE_H
#define CONNECTIVITY_SERVICE_H

#include <stdint.h>

namespace ConnectivityService {
    void init(void);

    // WiFi
    bool connectWiFi(const char *ssid, const char *password);
    void disconnectWiFi(void);
    bool isWiFiConnected(void);

    // Compressor control (HTTP POST to local IPs)
    bool sendCompressorCommand(const char *ip, bool state);

    // Priority Alert HTTP server
    void startAlertServer(uint16_t port = 8080);
    void stopAlertServer(void);
    void handleAlertServer(void);  // call in loop
}

#endif
