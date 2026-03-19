/**
 * BLE Notifications Handler — Implementation
 */

#include "ble_notifications.h"
#include "hardware_config.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEClient.h>
#include <BLE2902.h>
#include <ArduinoJson.h>
#include <string>
#include <time.h>

// Global BLE objects
static BLEServer *pServer = NULL;
static BLEClient *pClient = NULL;
static BLECharacteristic *pCharacteristic = NULL;
static BLECharacteristic *pTaskCharacteristic = NULL;
static BLECharacteristic *pTimeCharacteristic = NULL;
static bool deviceConnected = false;
static bool oldDeviceConnected = false;

// Callback classes
class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *pServer) {
        deviceConnected = true;
        Serial.println("BLE Device Connected");
    }

    void onDisconnect(BLEServer *pServer) {
        deviceConnected = false;
        Serial.println("BLE Device Disconnected");
    }
};

class MyCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();

        if (value.length() > 0) {
            Serial.println("Received notification via BLE:");
            Serial.println(value.c_str());

            StaticJsonDocument<512> doc;
            DeserializationError error = deserializeJson(doc, value.c_str(), value.length());

            if (error) {
                Serial.print(F("deserializeJson() failed: "));
                Serial.println(error.f_str());
                return;
            }

            NotificationData notif;

            strlcpy(notif.title, doc["title"] | "No Title", sizeof(notif.title));
            strlcpy(notif.message, doc["message"] | "No Message", sizeof(notif.message));
            strlcpy(notif.app, doc["app"] | "No App", sizeof(notif.app));

            notif.timestamp = (uint32_t)time(NULL);
            notif.isRead = false;
            notif.category = doc["category"] | 0;

            if (notificationQueue != NULL) {
                xQueueSend(notificationQueue, &notif, 0);
                vibrateMotor(200);
            }
        }
    }
};

class TaskSyncCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
            Serial.println("Received task sync via BLE");
            onTaskSyncReceived(value.c_str(), value.length());
        }
    }
};

class TimeSyncCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
            uint32_t epoch = strtoul(value.c_str(), NULL, 10);
            if (epoch > 1000000000UL) {
                Serial.printf("Received time sync: %lu\n", (unsigned long)epoch);
                onTimeSyncReceived(epoch);
            }
        }
    }
};

void initializeBLE() {
    Serial.println("Initializing BLE...");

    BLEDevice::init("ESP32-Watch");

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);

    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY |
        BLECharacteristic::PROPERTY_INDICATE);

    pCharacteristic->addDescriptor(new BLE2902());
    pCharacteristic->setCallbacks(new MyCallbacks());
    pCharacteristic->setValue("ESP32 Watch Ready");

    pTaskCharacteristic = pService->createCharacteristic(
        TASK_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE);
    pTaskCharacteristic->setCallbacks(new TaskSyncCallbacks());

    pTimeCharacteristic = pService->createCharacteristic(
        TIME_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE);
    pTimeCharacteristic->setCallbacks(new TimeSyncCallbacks());

    pService->start();
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
    if (!deviceConnected && oldDeviceConnected) {
        vTaskDelay(pdMS_TO_TICKS(500));
        startBLEAdvertising();
        Serial.println("Restarted advertising after disconnect");
        oldDeviceConnected = deviceConnected;
    }

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
        char msg[32];
        snprintf(msg, sizeof(msg), "{\"battery\":%d}", level);
        pCharacteristic->setValue(msg);
        pCharacteristic->notify();
    }
}

void sendTaskCompletion(const char *taskId) {
    if (deviceConnected && pCharacteristic) {
        char msg[96];
        snprintf(msg, sizeof(msg), "{\"action\":\"complete\",\"id\":\"%s\"}", taskId ? taskId : "");
        pCharacteristic->setValue(msg);
        pCharacteristic->notify();
        Serial.printf("BLE: sent task completion for id=%s\n", taskId ? taskId : "(null)");
    }
}

// iOS ANCS Support (requires additional library)
#ifdef USE_ANCS
#include "esp32notifications.h"

static BLENotifications notifications;

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

void onNotificationArrived(const ArduinoNotification *notification, const Notification *rawData) {
    NotificationData notif;

    strlcpy(notif.title, notification->title, sizeof(notif.title));
    strlcpy(notif.message, notification->message, sizeof(notif.message));
    strlcpy(notif.app, notification->app, sizeof(notif.app));
    notif.category = notification->category;
    notif.timestamp = (uint32_t)time(NULL);
    notif.isRead = false;

    if (notificationQueue != NULL) {
        xQueueSend(notificationQueue, &notif, 0);
        vibrateMotor(200);
    }

    Serial.print("ANCS Notification: ");
    Serial.println(notification->title);
}
#endif
