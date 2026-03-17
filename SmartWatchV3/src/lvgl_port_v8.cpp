/**
 * LVGL Porting Layer Implementation
 *
 * This follows the EXACT rendering pattern from the manufacturer's working
 * examples (05_lvgl_qmi8658, 09_lvgl_camera):
 *
 *   - Full-screen framebuffer (240 * 320 pixels)
 *   - direct_mode = true
 *   - Single draw buffer (no double-buffering)
 *   - Buffer allocation: try INTERNAL|8BIT first, fall back to 8BIT
 *   - Flush callback does the full-screen draw with the correct
 *     draw16bitRGBBitmap / draw16bitBeRGBBitmap depending on LV_COLOR_16_SWAP
 *   - Touch read via bsp_cst816, identical to examples
 *
 * Added for esp-brookesia (not in the simple examples):
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
static lv_disp_draw_buf_t draw_buf;
static lv_disp_drv_t      disp_drv;
static lv_indev_drv_t     indev_drv;
static lv_color_t        *disp_draw_buf = NULL;

static SemaphoreHandle_t  lvgl_mutex      = NULL;
static TaskHandle_t       lvgl_task_handle = NULL;
static Arduino_GFX       *gfx             = NULL;

static uint32_t screenWidth;
static uint32_t screenHeight;
static uint32_t bufSize;

// Forward declarations
static void lvgl_flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
static void lvgl_touch_cb(lv_indev_drv_t *indev, lv_indev_data_t *data);
static void lvgl_tick_task(void *arg);
static void lvgl_task(void *arg);

/**
 * Initialize LVGL with display and touch.
 *
 * The buffer allocation and display-driver setup mirror examples
 * 05_lvgl_qmi8658 and 09_lvgl_camera line-for-line.
 */
void lvgl_port_init(void *lcd, void *touch) {
    gfx = (Arduino_GFX *)lcd;

    // ---- LVGL core init (same as examples) ----
    lv_init();

    screenWidth  = gfx->width();
    screenHeight = gfx->height();
    bufSize      = screenWidth * screenHeight;   // full-screen buffer

    // ---- Buffer allocation — identical to examples ----
    // Try internal RAM first, fall back to any available (may hit PSRAM)
    disp_draw_buf = (lv_color_t *)heap_caps_malloc(
        bufSize * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!disp_draw_buf) {
        Serial.println("Internal RAM insufficient, falling back...");
        disp_draw_buf = (lv_color_t *)heap_caps_malloc(
            bufSize * sizeof(lv_color_t), MALLOC_CAP_8BIT);
    }

    if (!disp_draw_buf) {
        Serial.println("ERROR: LVGL disp_draw_buf allocate failed!");
        return;
    }

    Serial.printf("LVGL buffer allocated: %u pixels (%u bytes)\n",
                  bufSize, (unsigned)(bufSize * sizeof(lv_color_t)));

    // Single buffer, NULL for second — matches examples exactly
    lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, bufSize);

    // ---- Display driver — matches examples ----
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res     = screenWidth;
    disp_drv.ver_res     = screenHeight;
    disp_drv.flush_cb    = lvgl_flush_cb;
    disp_drv.draw_buf    = &draw_buf;
    disp_drv.direct_mode = true;           // <-- critical, matches examples
    lv_disp_drv_register(&disp_drv);

    // ---- Touch input driver — identical to examples ----
    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = lvgl_touch_cb;
    lv_indev_drv_register(&indev_drv);

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
 * Display flush callback.
 *
 * Matches example 09_lvgl_camera — full-screen draw in flush, with the
 * LV_COLOR_16_SWAP check that ALL manufacturer examples use.
 */
static void lvgl_flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    if (!gfx) {
        lv_disp_flush_ready(disp);
        return;
    }

    // Full-screen blit — matches examples' direct_mode pattern.
    // The LV_COLOR_16_SWAP check selects the correct byte-order draw function,
    // exactly as every working example does.
#if (LV_COLOR_16_SWAP != 0)
    gfx->draw16bitBeRGBBitmap(0, 0, (uint16_t *)disp_draw_buf, screenWidth, screenHeight);
#else
    gfx->draw16bitRGBBitmap(0, 0, (uint16_t *)disp_draw_buf, screenWidth, screenHeight);
#endif

    lv_disp_flush_ready(disp);
}

/**
 * Touch read callback — identical to examples.
 */
static void lvgl_touch_cb(lv_indev_drv_t *indev, lv_indev_data_t *data) {
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
            lv_task_handler();
            xSemaphoreGiveRecursive(lvgl_mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(5));   // ~200 Hz, matches example 09
    }
}
