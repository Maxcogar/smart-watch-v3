/**
 * LVGL Porting Layer for ESP32-S3
 *
 * Matches the proven LVGL init pattern from the manufacturer's working
 * examples (05_lvgl_qmi8658, 09_lvgl_camera) with the addition of
 * FreeRTOS task + recursive mutex for esp-brookesia thread-safety.
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

// Task and timer configuration
#define LVGL_PORT_TICK_PERIOD_MS        2
#define LVGL_PORT_TASK_PRIORITY         4
#define LVGL_PORT_TASK_STACK_SIZE       (8 * 1024)   // 8 KB — esp-brookesia needs headroom
#define LVGL_PORT_TASK_CORE             1            // Run on Core 1

// Function declarations
void lvgl_port_init(void *lcd, void *touch);
void lvgl_port_lock(int timeout_ms);
void lvgl_port_unlock(void);
bool lvgl_port_lock_ready(void);

#ifdef __cplusplus
}
#endif

#endif // LVGL_PORT_V8_H
