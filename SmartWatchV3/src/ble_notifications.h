/**
 * BLE Notifications Handler
 * 
 * Manages Bluetooth Low Energy connections and notification handling
 * for both iOS (ANCS) and Android devices.
 */

#ifndef BLE_NOTIFICATIONS_H
#define BLE_NOTIFICATIONS_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEClient.h>
#include <BLE2902.h>
#include <Arduino.h>
#include <string>
#include "hardware_config.h"

#include <ArduinoJson.h>

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
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// ANCS UUIDs for iOS
#define ANCS_SERVICE_UUID "7905F431-B5CE-4E99-A40F-4B1E122D00D0"
#define NOTIFICATION_SOURCE_UUID "9FBF120D-6301-42D9-8C58-25E699A21DBD"
#define CONTROL_POINT_UUID "69D1D8F3-45E1-49A8-9821-9BBDFDAAD9D9"
#define DATA_SOURCE_UUID "22EAC6E9-24D6-4BB5-BE44-B36ACE7C7BFB"

// Global BLE objects
BLEServer *pServer = NULL;
BLEClient *pClient = NULL;
BLECharacteristic *pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
String connectedDeviceName = "";

// External queue for notifications
extern QueueHandle_t notificationQueue;

// Callback classes
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        Serial.println("BLE Device Connected");
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        Serial.println("BLE Device Disconnected");
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue().c_str();
        
        if (value.length() > 0) {
            Serial.println("Received notification via BLE:");
            Serial.println(value.c_str());
            
            // Parse notification data (expects JSON format from Android app)
            StaticJsonDocument<512> doc;
            DeserializationError error = deserializeJson(doc, value);

            if (error) {
                Serial.print(F("deserializeJson() failed: "));
                Serial.println(error.f_str());
                return;
            }
            
            NotificationData notif;
            
            strlcpy(notif.title, doc["title"] | "No Title", sizeof(notif.title));
            strlcpy(notif.message, doc["message"] | "No Message", sizeof(notif.message));
            strlcpy(notif.app, doc["app"] | "No App", sizeof(notif.app));
            
            notif.timestamp = millis();
            notif.isRead = false;
            notif.category = 3; // Default to social
            
            // Send to notification queue
            if (notificationQueue != NULL) {
                xQueueSend(notificationQueue, &notif, 0);
                
                // Vibrate on notification
                vibrateMotor(200);
            }
        }
    }
};

// Function declarations
void initializeBLE();
void processBLEEvents();
bool isBLEConnected();
void attemptBLEReconnection();
void startBLEAdvertising();
void stopBLEAdvertising();
void sendBatteryLevel(uint8_t level);

// Implementation
void initializeBLE() {
    Serial.println("Initializing BLE...");
    
    // Initialize BLE with device name
    BLEDevice::init("ESP32-Watch");
    
    // Create BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    
    // Create BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);
    
    // Create BLE Characteristic for notifications
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY |
        BLECharacteristic::PROPERTY_INDICATE
    );
    
    // Add descriptor for notifications
    pCharacteristic->addDescriptor(new BLE2902());
    pCharacteristic->setCallbacks(new MyCallbacks());
    
    // Set initial value
    pCharacteristic->setValue("ESP32 Watch Ready");
    
    // Start the service
    pService->start();
    
    // Start advertising
    startBLEAdvertising();
    
    Serial.println("BLE initialization complete");
    Serial.println("Waiting for phone connection...");
}

void startBLEAdvertising() {
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // Functions that help with iPhone connections
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("BLE advertising started");
}

void stopBLEAdvertising() {
    BLEDevice::stopAdvertising();
    Serial.println("BLE advertising stopped");
}

void processBLEEvents() {
    // Handle disconnection
    if (!deviceConnected && oldDeviceConnected) {
        vTaskDelay(pdMS_TO_TICKS(500));  // Non-blocking delay for BT stack
        startBLEAdvertising();  // Restart advertising
        Serial.println("Restarted advertising after disconnect");
        oldDeviceConnected = deviceConnected;
    }
    
    // Handle new connection
    if (deviceConnected && !oldDeviceConnected) {
        // New device connected
        oldDeviceConnected = deviceConnected;
        Serial.println("New device connected, ready for notifications");
        
        // Send current battery level
        sendBatteryLevel(getBatteryPercentage());
    }
}

bool isBLEConnected() {
    return deviceConnected;
}

void attemptBLEReconnection() {
    static unsigned long lastAttempt = 0;
    
    // Try reconnection every 10 seconds
    if (millis() - lastAttempt > 10000) {
        lastAttempt = millis();
        
        if (!deviceConnected) {
            Serial.println("Attempting BLE reconnection...");
            startBLEAdvertising();
        }
    }
}

void sendBatteryLevel(uint8_t level) {
    if (deviceConnected && pCharacteristic) {
        String batteryMsg = "{\"battery\":" + String(level) + "}";
        pCharacteristic->setValue(batteryMsg.c_str());
        pCharacteristic->notify();
    }
}

// iOS ANCS Support (requires additional library)
#ifdef USE_ANCS
#include "esp32notifications.h"

BLENotifications notifications;
    
void initializeANCS() {
    notifications.begin("ESP32 Watch");
    notifications.setConnectionStateChangedCallback(onBLEStateChanged);
    notifications.setNotificationCallback(onNotificationArrived);
}

void onBLEStateChanged(bool state) {
    if (state) {
        Serial.println("ANCS Connected to iPhone");
    } else {
        Serial.println("ANCS Disconnected from iPhone");
    }
    deviceConnected = state;
}

void onNotificationArrived(const ArduinoNotification * notification, const Notification * rawData) {
    NotificationData notif;
    
    strncpy(notif.title, notification->title, 63);
    strncpy(notif.message, notification->message, 255);
    strncpy(notif.app, notification->app, 31);
    notif.category = notification->category;
    notif.timestamp = millis();
    notif.isRead = false;
    
    // Send to notification queue
    if (notificationQueue != NULL) {
        xQueueSend(notificationQueue, &notif, 0);
        vibrateMotor(200);
    }
    
    Serial.print("ANCS Notification: ");
    Serial.println(notification->title);
}
#endif

#endif // BLE_NOTIFICATIONS_H
