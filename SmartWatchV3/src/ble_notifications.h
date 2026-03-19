/**
 * BLE Notifications Handler
 *
 * Manages Bluetooth Low Energy connections and notification handling
 * for both iOS (ANCS) and Android devices.
 */

#ifndef BLE_NOTIFICATIONS_H
#define BLE_NOTIFICATIONS_H

#include <Arduino.h>
#include <stdint.h>

// Notification data structure
struct NotificationData {
    char title[64];
    char message[256];
    char app[32];
    uint8_t category;  // 0=other, 1=call, 2=email, 3=social, 4=alarm
    uint32_t timestamp;
    bool isRead;
};

// Function declarations
void initializeBLE();
void processBLEEvents();
bool isBLEConnected();
void attemptBLEReconnection();
void startBLEAdvertising();
void stopBLEAdvertising();
void sendBatteryLevel(uint8_t level);

#endif // BLE_NOTIFICATIONS_H
