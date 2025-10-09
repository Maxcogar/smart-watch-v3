/**
 * LVGL Porting Layer for ESP32-S3
 * 
 * This file configures LVGL to work with the display and touch hardware
 */

#ifndef LVGL_PORT_V8_H
#define LVGL_PORT_V8_H

#ifdef __cplusplus
#include <chrono>
#include <ctime>
#include <string>
#include <type_traits>
#endif

#include <lvgl.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#ifdef __cplusplus
extern "C" {
#endif

// Display buffer configuration
#define LVGL_PORT_DISP_BUFFER_NUM       2
#define LVGL_PORT_DISP_BUFFER_SIZE      (240 * 40)  // Partial buffer for 240x320 display
#define LVGL_PORT_RGB_BOUNCE_BUFFER_SIZE (240 * 10)
#define LVGL_PORT_AVOID_TEAR             0  // Set to 1 if tearing occurs

// Touch configuration
#define LVGL_PORT_TOUCH_POINTS_NUM      1  // Single touch support

// Task and timer configuration
#define LVGL_PORT_TICK_PERIOD_MS        2
#define LVGL_PORT_TASK_PRIORITY         4
#define LVGL_PORT_TASK_STACK_SIZE       4096
#define LVGL_PORT_TASK_CORE             1  // Run on Core 1

// Function declarations
void lvgl_port_init(void *lcd, void *touch);
void lvgl_port_lock(int timeout_ms);
void lvgl_port_unlock(void);
bool lvgl_port_lock_ready(void);

#ifdef __cplusplus
}
#endif

#endif // LVGL_PORT_V8_H
