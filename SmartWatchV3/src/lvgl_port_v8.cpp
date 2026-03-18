/**
 * LVGL Porting Layer Implementation — LVGL 9.x API
 *
 * This follows the EXACT rendering pattern from the manufacturer's working
 * examples (05_lvgl_qmi8658, 09_lvgl_camera), updated for LVGL 9 API:
 *
 *   - Full-screen framebuffer (320 * 240 pixels in landscape)
 *   - LV_DISPLAY_RENDER_MODE_DIRECT
 *   - Single draw buffer (no double-buffering)
 *   - Buffer allocation: try INTERNAL|8BIT first, fall back to 8BIT
 *   - Flush callback does the full-screen draw
 *   - Touch read via bsp_cst816, identical to examples
 *
 * Additions beyond the simple examples:
 *   - Recursive mutex (from example 09_lvgl_camera pattern)
 *   - Dedicated FreeRTOS task on Core 1
 *   - esp_timer for lv_tick_inc
 */

#include "lvgl_port_v8.h"
#include <Arduino.h>
#include "bsp_cst816.h"
#include <Wire.h>
#include <Arduino_GFX_Library.h>

// Static variables
static uint8_t           *disp_draw_buf = NULL;
static lv_display_t      *disp         = NULL;
static lv_indev_t        *indev        = NULL;

static SemaphoreHandle_t  lvgl_mutex      = NULL;
static TaskHandle_t       lvgl_task_handle = NULL;
static Arduino_GFX       *gfx             = NULL;

static uint32_t screenWidth;
static uint32_t screenHeight;
static uint32_t bufSize;

// Forward declarations
static void lvgl_flush_cb(lv_display_t *display, const lv_area_t *area, uint8_t *px_map);
static void lvgl_touch_cb(lv_indev_t *indev_drv, lv_indev_data_t *data);
static void lvgl_tick_task(void *arg);
static void lvgl_task(void *arg);

/**
 * Initialize LVGL with display and touch.
 *
 * The buffer allocation and display-driver setup mirror examples
 * 05_lvgl_qmi8658 and 09_lvgl_camera, updated for LVGL 9 API.
 */
void lvgl_port_init(void *lcd, void *touch) {
    gfx = (Arduino_GFX *)lcd;

    // ---- LVGL core init ----
    lv_init();

    screenWidth  = gfx->width();
    screenHeight = gfx->height();
    bufSize      = screenWidth * screenHeight;   // full-screen buffer (in pixels)

    // ---- Buffer allocation — identical to examples ----
    // RGB565 = 2 bytes per pixel
    uint32_t bufBytes = bufSize * sizeof(uint16_t);

    disp_draw_buf = (uint8_t *)heap_caps_malloc(
        bufBytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!disp_draw_buf) {
        Serial.println("Internal RAM insufficient, falling back...");
        disp_draw_buf = (uint8_t *)heap_caps_malloc(bufBytes, MALLOC_CAP_8BIT);
    }

    if (!disp_draw_buf) {
        Serial.println("ERROR: LVGL disp_draw_buf allocate failed!");
        return;
    }

    Serial.printf("LVGL buffer allocated: %u pixels (%u bytes)\n",
                  bufSize, bufBytes);

    // ---- Display driver (LVGL 9 API) ----
    disp = lv_display_create(screenWidth, screenHeight);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(disp, lvgl_flush_cb);
    lv_display_set_buffers(disp, disp_draw_buf, NULL, bufBytes,
                           LV_DISPLAY_RENDER_MODE_DIRECT);

    // ---- Touch input driver (LVGL 9 API) ----
    indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, lvgl_touch_cb);

    // ---- Recursive mutex (from example 09_lvgl_camera) ----
    lvgl_mutex = xSemaphoreCreateRecursiveMutex();
    if (!lvgl_mutex) {
        Serial.println("ERROR: Failed to create LVGL mutex!");
        return;
    }

    // ---- Tick timer via esp_timer ----
    const esp_timer_create_args_t tick_args = {
        .callback = &lvgl_tick_task,
        .name     = "lvgl_tick"
    };
    esp_timer_handle_t tick_timer = NULL;
    esp_timer_create(&tick_args, &tick_timer);
    esp_timer_start_periodic(tick_timer, LVGL_PORT_TICK_PERIOD_MS * 1000);

    // ---- LVGL task on Core 1 ----
    xTaskCreatePinnedToCore(
        lvgl_task,
        "LVGL",
        LVGL_PORT_TASK_STACK_SIZE,
        NULL,
        LVGL_PORT_TASK_PRIORITY,
        &lvgl_task_handle,
        LVGL_PORT_TASK_CORE);

    Serial.println("LVGL port initialization complete");
}

/**
 * Lock LVGL mutex (recursive, matching example 09).
 */
void lvgl_port_lock(int timeout_ms) {
    if (!lvgl_mutex) return;
    const TickType_t ticks = (timeout_ms < 0)
        ? portMAX_DELAY
        : pdMS_TO_TICKS(timeout_ms);
    xSemaphoreTakeRecursive(lvgl_mutex, ticks);
}

/**
 * Unlock LVGL mutex.
 */
void lvgl_port_unlock(void) {
    if (lvgl_mutex) {
        xSemaphoreGiveRecursive(lvgl_mutex);
    }
}

/**
 * Check if LVGL is ready.
 */
bool lvgl_port_lock_ready(void) {
    return (lvgl_mutex != NULL);
}

/**
 * Display flush callback (LVGL 9 API).
 *
 * Matches example 09_lvgl_camera — full-screen draw in flush.
 * In LVGL 9, px_map is uint8_t* and we cast to uint16_t* for RGB565.
 */
static void lvgl_flush_cb(lv_display_t *display, const lv_area_t *area, uint8_t *px_map) {
    if (!gfx) {
        lv_display_flush_ready(display);
        return;
    }

    // Full-screen blit — matches examples' direct_mode pattern.
    gfx->draw16bitRGBBitmap(0, 0, (uint16_t *)disp_draw_buf, screenWidth, screenHeight);

    lv_display_flush_ready(display);
}

/**
 * Touch read callback (LVGL 9 API).
 */
static void lvgl_touch_cb(lv_indev_t *indev_drv, lv_indev_data_t *data) {
    uint16_t touchpad_x;
    uint16_t touchpad_y;

    bsp_touch_read();
    if (bsp_touch_get_coordinates(&touchpad_x, &touchpad_y)) {
        data->point.x = touchpad_x;
        data->point.y = touchpad_y;
        data->state   = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

/**
 * LVGL tick timer callback.
 */
static void lvgl_tick_task(void *arg) {
    lv_tick_inc(LVGL_PORT_TICK_PERIOD_MS);
}

/**
 * LVGL task — handles display updates with mutex protection.
 * Mirrors example 09_lvgl_camera's loop pattern.
 */
static void lvgl_task(void *arg) {
    Serial.println("LVGL task started");

    while (1) {
        if (xSemaphoreTakeRecursive(lvgl_mutex, portMAX_DELAY) == pdTRUE) {
            lv_timer_handler();
            xSemaphoreGiveRecursive(lvgl_mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(5));   // ~200 Hz, matches example 09
    }
}
