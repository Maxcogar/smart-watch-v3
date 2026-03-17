/**
 * ESP32-S3 SmartWatch with esp-brookesia UI
 * 
 * Hardware: ESP32-S3-Touch-LCD-2 Development Board
 * Features: 
 * - Modern smartphone-style UI using esp-brookesia
 * - BLE notifications from phone
 * - Touch gesture support
 * - Battery monitoring
 * - IMU sensor integration
 * 
 * Required Libraries (install via Arduino Library Manager):
 * - esp-brookesia (v0.4.2+)
 * - ESP32_Display_Panel (v0.2.0+)
 * - ESP32_IO_Expander (v0.1.0+)
 * - lvgl (v8.3.11)
 * - ESP32 BLE Arduino
 * 
 * Board Settings:
 * - Board: ESP32S3 Dev Module
 * - USB CDC On Boot: Enabled
 * - PSRAM: OPI PSRAM
 * - Partition Scheme: Huge APP (3MB No OTA/1MB SPIFFS)
 * - Upload Speed: 921600
 */

// Include C++ STL headers before any C headers to prevent linkage errors with FreeRTOS
#include <type_traits>
#include <chrono>
#include <string>
#include "bsp_cst816.h"
#include <Wire.h>
#include <Arduino_GFX_Library.h>

#define EXAMPLE_PIN_NUM_LCD_SCLK 39
#define EXAMPLE_PIN_NUM_LCD_MOSI 38
#define EXAMPLE_PIN_NUM_LCD_MISO 40
#define EXAMPLE_PIN_NUM_LCD_DC 42
#define EXAMPLE_PIN_NUM_LCD_RST -1
#define EXAMPLE_PIN_NUM_LCD_CS 45
#define EXAMPLE_PIN_NUM_LCD_BL 1
#define EXAMPLE_PIN_NUM_TP_SDA 48
#define EXAMPLE_PIN_NUM_TP_SCL 47

// Backlight PWM — matches manufacturer examples exactly
#define LEDC_FREQ             5000
#define LEDC_TIMER_10_BIT     10

#define EXAMPLE_LCD_ROTATION 0
#define EXAMPLE_LCD_H_RES 240
#define EXAMPLE_LCD_V_RES 320

#define LV_CONF_INCLUDE_SIMPLE

#include <Arduino.h>
#include <esp_brookesia.hpp>
#include <lvgl.h>
#include "src/lvgl_port_v8.h"
#include "src/ble_notifications.h"
#include "src/watch_apps.h"
#include "src/hardware_config.h"

// Global GFX objects
Arduino_DataBus *bus = new Arduino_ESP32SPI(
  EXAMPLE_PIN_NUM_LCD_DC /* DC */, EXAMPLE_PIN_NUM_LCD_CS /* CS */,
  EXAMPLE_PIN_NUM_LCD_SCLK /* SCK */, EXAMPLE_PIN_NUM_LCD_MOSI /* MOSI */, EXAMPLE_PIN_NUM_LCD_MISO /* MISO */);

Arduino_GFX *gfx = new Arduino_ST7789(
  bus, EXAMPLE_PIN_NUM_LCD_RST /* RST */, EXAMPLE_LCD_ROTATION /* rotation */, true /* IPS */,
  EXAMPLE_LCD_H_RES /* width */, EXAMPLE_LCD_V_RES /* height */);

// Enable debug output
#define DEBUG_OUTPUT 1

// Memory monitoring
#define SHOW_MEM_INFO 1

// Global objects
ESP_Brookesia_Phone *phone = nullptr;
static char buffer[128];
static size_t internal_free = 0;
static size_t internal_total = 0;
static size_t external_free = 0;
static size_t external_total = 0;

// FreeRTOS handles
TaskHandle_t bleTaskHandle = NULL;
TaskHandle_t sensorTaskHandle = NULL;
QueueHandle_t notificationQueue = NULL;

// Forward declarations
static void onClockUpdateTimerCallback(struct _lv_timer_t *t);
void bleTask(void *parameter);
void sensorTask(void *parameter);
void checkMemory();

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32-S3 SmartWatch v3.0 Starting...");
    
    // Init Display
    Serial.println("Initialize GFX display");
    if (!gfx->begin()) {
        Serial.println("gfx->begin() failed!");
        while(1) delay(100);
    }
    gfx->fillScreen(BLACK);

    // Init Backlight — identical to manufacturer examples
    ledcAttach(EXAMPLE_PIN_NUM_LCD_BL, LEDC_FREQ, LEDC_TIMER_10_BIT);
    ledcWrite(EXAMPLE_PIN_NUM_LCD_BL, (1 << LEDC_TIMER_10_BIT) / 100 * 80);
    
    // Init Touch
    Serial.println("Initialize touch device");
    Wire.begin(EXAMPLE_PIN_NUM_TP_SDA, EXAMPLE_PIN_NUM_TP_SCL);
    bsp_touch_init(&Wire, gfx->getRotation(), gfx->width(), gfx->height());
    
    // Initialize LVGL
    Serial.println("Initialize LVGL");
    lvgl_port_init(gfx, nullptr);
    
    // Create esp-brookesia phone UI
    Serial.println("Create and configure phone UI");
    lvgl_port_lock(-1);
    
    phone = new ESP_Brookesia_Phone();
    if (!phone) {
        Serial.println("ERROR: Failed to create phone object");
        while(1) delay(100);
    }

    // Configure phone UI callbacks (must be set before begin)
    phone->registerLvLockCallback((ESP_Brookesia_LvLockCallback_t)(lvgl_port_lock), -1);
    phone->registerLvUnlockCallback((ESP_Brookesia_LvUnlockCallback_t)(lvgl_port_unlock));

    // Use the small (240x240) stylesheet and adapt for our 240x320 display.
    // esp-brookesia requires an explicit stylesheet; relying on auto-detect often
    // fails on non-standard resolutions.
    ESP_Brookesia_PhoneStylesheet_t *stylesheet = new ESP_Brookesia_PhoneStylesheet_t;
    *stylesheet = ESP_BROOKESIA_PHONE_DEFAULT_DARK_STYLESHEET();
    // Patch resolution to match our display
    stylesheet->core.screen_size = {EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES};
    if (!phone->addStylesheet(stylesheet)) {
        Serial.println("WARNING: Failed to add stylesheet, trying begin without it");
    }

    // Start the phone UI
    if (!phone->begin()) {
        Serial.println("ERROR: Failed to begin phone");
        // Try once more with a clean default (no custom stylesheet)
        Serial.println("Retrying phone->begin()...");
        if (!phone->begin()) {
            Serial.println("ERROR: phone->begin() failed on retry");
            while(1) delay(100);
        }
    }
    
    // Create notification queue BEFORE installing apps (apps reference it)
    notificationQueue = xQueueCreate(10, sizeof(NotificationData));

    // Install smartwatch apps
    Serial.println("Installing smartwatch apps...");
    installWatchApps(phone);

    // Create clock update timer (60 second interval)
    lv_timer_create(onClockUpdateTimerCallback, 60000, phone);

    lvgl_port_unlock();
    
    // Initialize BLE for notifications
    Serial.println("Initializing BLE notifications...");
    initializeBLE();
    
    // Create FreeRTOS tasks
    Serial.println("Creating FreeRTOS tasks...");
    xTaskCreatePinnedToCore(
        bleTask,        // Task function
        "BLE_Task",     // Task name
        4096,           // Stack size
        NULL,           // Parameters
        2,              // Priority
        &bleTaskHandle, // Task handle
        0               // Core 0
    );
    
    xTaskCreatePinnedToCore(
        sensorTask,      // Task function
        "Sensor_Task",   // Task name
        2048,            // Stack size
        NULL,            // Parameters
        1,               // Priority
        &sensorTaskHandle, // Task handle
        0                 // Core 0
    );
    
    Serial.println("SmartWatch initialization complete!");
    checkMemory();
}

void loop() {
    #if SHOW_MEM_INFO
    // Monitor memory usage every 5 seconds
    static unsigned long lastMemCheck = 0;
    if (millis() - lastMemCheck > 5000) {
        lastMemCheck = millis();
        
        internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        internal_total = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
        external_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        external_total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
        
        sprintf(buffer, "Memory - SRAM: %dKB/%dKB | PSRAM: %dKB/%dKB",
                internal_free/1024, internal_total/1024,
                external_free/1024, external_total/1024);
        Serial.println(buffer);
        
        // Update memory display in UI if recents screen is active
        // (guard against null — recents screen may not exist yet)
        lvgl_port_lock(-1);
        auto *recents = phone->getHome().getRecentsScreen();
        if (recents) {
            recents->setMemoryLabel(
                internal_free / 1024, internal_total / 1024,
                external_free / 1024, external_total / 1024);
        }
        lvgl_port_unlock();
    }
    #endif
    
    // Main loop runs at lower frequency to save power
    vTaskDelay(pdMS_TO_TICKS(100));
}

// Clock update callback
static void onClockUpdateTimerCallback(struct _lv_timer_t *t) {
    time_t now;
    struct tm timeinfo;
    bool is_time_pm = false;
    ESP_Brookesia_Phone *phone = (ESP_Brookesia_Phone *)t->user_data;
    
    time(&now);
    localtime_r(&now, &timeinfo);
    is_time_pm = (timeinfo.tm_hour >= 12);
    
    // Update clock on status bar
    phone->getHome().getStatusBar()->setClock(
        timeinfo.tm_hour, timeinfo.tm_min, is_time_pm
    );
}

// BLE task - handles Bluetooth communication
void bleTask(void *parameter) {
    Serial.println("BLE task started on core 0");
    
    while(1) {
        // Process BLE events
        processBLEEvents();
        
        // Check for disconnection/reconnection
        if (!isBLEConnected()) {
            attemptBLEReconnection();
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Sensor task - reads IMU and battery
void sensorTask(void *parameter) {
    Serial.println("Sensor task started on core 0");
    
    while(1) {
        // Read battery voltage
        updateBatteryStatus();
        
        // Read IMU for step counting
        updateStepCount();
        
        // Check for wrist raise gesture
        if (checkWristRaise()) {
            wakeDisplay();
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// Memory monitoring helper
void checkMemory() {
    Serial.println("=== Memory Status ===");
    Serial.printf("Total heap: %d bytes\n", ESP.getHeapSize());
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Total PSRAM: %d bytes\n", ESP.getPsramSize());
    Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());
    Serial.printf("Largest free block: %d bytes\n", ESP.getMaxAllocHeap());
    Serial.println("===================");
}
