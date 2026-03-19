/**
 * BLE Notifications — Implementation
 *
 * All BLE globals, callback classes, and function bodies live here
 * (not in the header) to avoid multiple-definition linker errors
 * when ble_notifications.h is included from multiple .cpp files.
 */

#include "ble_notifications.h"
#include "hardware_config.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEClient.h>
#include <BLE2902.h>
#include <string>
#include <ArduinoJson.h>

// BLE UUIDs for custom notification service (Android)
#define SERVICE_UUID            "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID     "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define TASK_CHAR_UUID          "beb5483e-36e1-4688-b7f5-ea07361b26a9"
#define TIME_CHAR_UUID          "beb5483e-36e1-4688-b7f5-ea07361b26aa"

// Global BLE objects
static BLEServer *pServer = NULL;
static BLEClient *pClient = NULL;
static BLECharacteristic *pCharacteristic = NULL;
static BLECharacteristic *pTaskCharacteristic = NULL;
static BLECharacteristic *pTimeCharacteristic = NULL;
static bool deviceConnected = false;
static bool oldDeviceConnected = false;

// External task sync callback (set from main code)
extern void onTaskSyncReceived(const char *json, size_t len);
extern void onTimeSyncReceived(uint32_t epoch);

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
            notif.category = doc["category"] | 0;

            // Send to notification queue
            if (notificationQueue != NULL) {
                xQueueSend(notificationQueue, &notif, 0);

                // Vibrate on notification
                vibrateMotor(200);
            }
        }
    }
};

// Task sync callback
class TaskSyncCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue().c_str();
        if (value.length() > 0) {
            Serial.println("Received task sync via BLE");
            onTaskSyncReceived(value.c_str(), value.length());
        }
    }
};

// Time sync callback
class TimeSyncCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue().c_str();
        if (value.length() > 0) {
            uint32_t epoch = strtoul(value.c_str(), NULL, 10);
            if (epoch > 1000000000UL) {  // sanity check
                Serial.printf("Received time sync: %lu\n", (unsigned long)epoch);
                onTimeSyncReceived(epoch);
            }
        }
    }
};

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

    // Create task sync characteristic
    pTaskCharacteristic = pService->createCharacteristic(
        TASK_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    pTaskCharacteristic->setCallbacks(new TaskSyncCallbacks());

    // Create time sync characteristic
    pTimeCharacteristic = pService->createCharacteristic(
        TIME_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    pTimeCharacteristic->setCallbacks(new TimeSyncCallbacks());

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
    pAdvertising->setMinPreferred(0x06);
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
        vTaskDelay(pdMS_TO_TICKS(500));
        startBLEAdvertising();
        Serial.println("Restarted advertising after disconnect");
        oldDeviceConnected = deviceConnected;
    }

    // Handle new connection
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
        Serial.println("New device connected, ready for notifications");
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
