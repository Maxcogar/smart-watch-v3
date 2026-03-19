/**
 * ESP32-S3 ADHD-Friendly SmartWatch
 *
 * Hardware: Waveshare ESP32-S3-Touch-LCD-2
 * Display:  320x240 landscape (ST7789 via SPI)
 * Touch:    CST816S (I2C)
 * UI:       LVGL 8.3.11 (direct, no framework)
 *
 * Required Libraries (install via Arduino Library Manager):
 * - lvgl (v8.3.11)
 * - ESP32 BLE Arduino
 * - ArduinoJson (v7.x)
 *
 * Board Settings:
 * - Board: ESP32S3 Dev Module
 * - USB CDC On Boot: Enabled
 * - PSRAM: OPI PSRAM
 * - Partition Scheme: Huge APP (3MB No OTA/1MB SPIFFS)
 * - CPU Frequency: 240MHz (WiFi)
 * - Upload Speed: 921600
 */

// Include C++ STL headers before any C headers to prevent linkage errors
#include <type_traits>
#include <chrono>
#include <string>
#include "bsp_cst816.h"
#include <Wire.h>
#include <Arduino_GFX_Library.h>

// --- Pin Definitions (match manufacturer examples) ---
#define PIN_LCD_SCLK  39
#define PIN_LCD_MOSI  38
#define PIN_LCD_MISO  40
#define PIN_LCD_DC    42
#define PIN_LCD_RST   -1
#define PIN_LCD_CS    45
#define PIN_LCD_BL    1
#define PIN_TP_SDA    48
#define PIN_TP_SCL    47

// --- Display Configuration: Landscape 320x240 ---
#define LCD_ROTATION  1
#define LCD_H_RES     320
#define LCD_V_RES     240

// --- Backlight PWM ---
#define BL_FREQ       5000
#define BL_RESOLUTION 10

#define LV_CONF_INCLUDE_SIMPLE

#include <Arduino.h>
#include <lvgl.h>
#include "src/lvgl_port_v8.h"
#include "src/ble_notifications.h"
#include "src/hardware_config.h"

// Services
#include "src/services/storage_service.h"
#include "src/services/task_service.h"
#include "src/services/focus_service.h"
#include "src/services/notification_service.h"
#include "src/services/connectivity_service.h"
#include "src/services/time_service.h"
#include "src/services/power_service.h"

// UI system
#include "src/ui/theme.h"
#include "src/ui/screen_manager.h"
#include "src/ui/gesture.h"
#include "src/ui/widgets/status_bar.h"
#include "src/ui/screens/watch_face.h"
#include "src/ui/screens/task_list.h"
#include "src/ui/screens/active_task.h"
#include "src/ui/screens/notification_view.h"
#include "src/ui/screens/priority_alert.h"
#include "src/ui/screens/control_panel.h"

// --- Display Objects ---
Arduino_DataBus *bus = new Arduino_ESP32SPI(
    PIN_LCD_DC, PIN_LCD_CS, PIN_LCD_SCLK, PIN_LCD_MOSI, PIN_LCD_MISO);

Arduino_GFX *gfx = new Arduino_ST7789(
    bus, PIN_LCD_RST, LCD_ROTATION, true /* IPS */);

// --- FreeRTOS Handles ---
TaskHandle_t bleTaskHandle = NULL;
QueueHandle_t notificationQueue = NULL;

// Forward declarations
void bleTask(void *parameter);
void checkMemory();

// BLE sync callbacks (called from BLE characteristic writes)
void onTaskSyncReceived(const char *json, size_t len) {
    TaskService::syncFromJSON(json, len);
}

void onTimeSyncReceived(uint32_t epoch) {
    TimeService::setTimeFromEpoch(epoch);
}

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32-S3 SmartWatch Starting...");

    // --- Init Display ---
    if (!gfx->begin()) {
        Serial.println("ERROR: gfx->begin() failed!");
        while (1) delay(100);
    }
    gfx->fillScreen(BLACK);

    // Backlight at 80%
    ledcAttach(PIN_LCD_BL, BL_FREQ, BL_RESOLUTION);
    ledcWrite(PIN_LCD_BL, (1 << BL_RESOLUTION) / 100 * 80);

    // --- Init Touch ---
    Wire.begin(PIN_TP_SDA, PIN_TP_SCL);
    bsp_touch_init(&Wire, gfx->getRotation(), gfx->width(), gfx->height());

    // --- Init LVGL ---
    lvgl_port_init(gfx, nullptr);

    // --- Init Services ---
    StorageService::init();
    TaskService::init();
    FocusService::init();
    NotificationService::init();
    ConnectivityService::init();
    TimeService::init();
    PowerService::init();

    // --- Init UI system ---
    lvgl_port_lock(-1);

    // Theme (colors, styles, fonts)
    theme_init();

    // Status bar (persistent on lv_layer_top)
    StatusBar::init();

    // Register screens
    ScreenManager::registerScreen(SCREEN_WATCH_FACE, WatchFace::create, WatchFace::destroy, "Watch Face");
    ScreenManager::registerScreen(SCREEN_TASK_LIST, TaskList::create, TaskList::destroy, "Tasks");
    ScreenManager::registerScreen(SCREEN_ACTIVE_TASK, ActiveTask::create, ActiveTask::destroy, "Active Task");
    ScreenManager::registerScreen(SCREEN_NOTIFICATIONS, NotificationView::create, NotificationView::destroy, "Notifications");

    // Start on watch face
    ScreenManager::init();

    lvgl_port_unlock();

    // --- Notification queue for BLE -> UI ---
    notificationQueue = xQueueCreate(20, sizeof(NotificationData));

    // --- Init BLE ---
    Serial.println("Initializing BLE...");
    initializeBLE();

    // --- BLE Task on Core 0 ---
    xTaskCreatePinnedToCore(
        bleTask, "BLE_Task", 4096, NULL, 2, &bleTaskHandle, 0);

    Serial.println("SmartWatch init complete!");
    checkMemory();
}

void loop() {
    // Power management (runs every iteration)
    PowerService::update();

    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 5000) {
        lastUpdate = millis();

        // Update status bar with live data
        lvgl_port_lock(-1);
        StatusBar::setBattery(getBatteryPercentage());
        StatusBar::setBLEConnected(isBLEConnected());
        lvgl_port_unlock();

        // Memory monitoring
        Serial.printf("Memory - SRAM: %dKB/%dKB | PSRAM: %dKB/%dKB\n",
            heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024,
            heap_caps_get_total_size(MALLOC_CAP_INTERNAL) / 1024,
            heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024,
            heap_caps_get_total_size(MALLOC_CAP_SPIRAM) / 1024);
    }
    vTaskDelay(pdMS_TO_TICKS(100));
}

void bleTask(void *parameter) {
    Serial.println("BLE task started on core 0");
    NotificationData notif;

    while (1) {
        processBLEEvents();

        // Process incoming notifications from BLE queue
        while (xQueueReceive(notificationQueue, &notif, 0) == pdTRUE) {
            lvgl_port_lock(-1);
            bool isPriority = NotificationService::processIncoming(notif);
            if (isPriority) {
                PriorityAlert::show(notif.message);
            }
            lvgl_port_unlock();
        }

        if (!isBLEConnected()) {
            attemptBLEReconnection();
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void checkMemory() {
    Serial.println("=== Memory Status ===");
    Serial.printf("Total heap: %d bytes\n", ESP.getHeapSize());
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Total PSRAM: %d bytes\n", ESP.getPsramSize());
    Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());
    Serial.printf("Largest free block: %d bytes\n", ESP.getMaxAllocHeap());
    Serial.println("===================");
}
