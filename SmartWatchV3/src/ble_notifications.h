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

// BLE UUIDs for custom notification service (Android)
#define SERVICE_UUID            "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID     "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define TASK_CHAR_UUID          "beb5483e-36e1-4688-b7f5-ea07361b26a9"
#define TIME_CHAR_UUID          "beb5483e-36e1-4688-b7f5-ea07361b26aa"

// ANCS UUIDs for iOS
#define ANCS_SERVICE_UUID "7905F431-B5CE-4E99-A40F-4B1E122D00D0"
#define NOTIFICATION_SOURCE_UUID "9FBF120D-6301-42D9-8C58-25E699A21DBD"
#define CONTROL_POINT_UUID "69D1D8F3-45E1-49A8-9821-9BBDFDAAD9D9"
#define DATA_SOURCE_UUID "22EAC6E9-24D6-4BB5-BE44-B36ACE7C7BFB"

// External task sync callback (set from main code)
extern void onTaskSyncReceived(const char *json, size_t len);
extern void onTimeSyncReceived(uint32_t epoch);

// External queue for notifications
extern QueueHandle_t notificationQueue;

// Function declarations
void initializeBLE();
void processBLEEvents();
bool isBLEConnected();
void attemptBLEReconnection();
void startBLEAdvertising();
void stopBLEAdvertising();
void sendBatteryLevel(uint8_t level);
void sendTaskCompletion(const char *taskId);

#endif // BLE_NOTIFICATIONS_H
