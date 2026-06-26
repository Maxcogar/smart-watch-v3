/**
 * LVGL Porting Layer Implementation
 *
 * Framebuffer strategy matches the factory BSP (examples/01_factory/
 * bsp_lv_port.cpp): two FULL-SCREEN draw buffers allocated in PSRAM via
 * MALLOC_CAP_SPIRAM. The board has 8MB PSRAM precisely so the framebuffers
 * do not compete with internal RAM. If PSRAM is unavailable at runtime we
 * fall back to a single 1/10-screen partial buffer in internal RAM so the
 * display still comes up.
 *
 *   - Two full-screen buffers in PSRAM (factory pattern), or partial fallback
 *   - Flush callback draws the dirty area with the correct
 *     draw16bitRGBBitmap / draw16bitBeRGBBitmap depending on LV_COLOR_16_SWAP
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
static lv_disp_draw_buf_t draw_buf;
static lv_disp_drv_t      disp_drv;
static lv_indev_drv_t     indev_drv;
static lv_color_t        *disp_draw_buf  = NULL;
static lv_color_t        *disp_draw_buf2 = NULL;

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

    // Factory pattern: two full-screen buffers in PSRAM (8MB available).
    // See examples/01_factory/bsp_lv_port.cpp.
    bufSize = screenWidth * screenHeight;
    disp_draw_buf = (lv_color_t *)heap_caps_malloc(
        bufSize * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    disp_draw_buf2 = (lv_color_t *)heap_caps_malloc(
        bufSize * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);

    if (disp_draw_buf && disp_draw_buf2) {
        Serial.printf("LVGL: 2x full-screen buffers in PSRAM (%u px, %u bytes total)\n",
                      bufSize, (unsigned)(bufSize * sizeof(lv_color_t) * 2));
        lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, disp_draw_buf2, bufSize);
    } else {
        // PSRAM unavailable — fall back to a single 1/10-screen partial
        // buffer in internal RAM so the display still initializes.
        if (disp_draw_buf)  { heap_caps_free(disp_draw_buf);  disp_draw_buf  = NULL; }
        if (disp_draw_buf2) { heap_caps_free(disp_draw_buf2); disp_draw_buf2 = NULL; }
        bufSize = screenWidth * (screenHeight / 10);
        disp_draw_buf = (lv_color_t *)heap_caps_malloc(
            bufSize * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!disp_draw_buf) {
            Serial.println("ERROR: LVGL disp_draw_buf allocate failed!");
            return;
        }
        Serial.printf("LVGL: PSRAM unavailable, partial internal buffer (%u px)\n", bufSize);
        lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, bufSize);
    }

    // ---- Display driver ----
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res     = screenWidth;
    disp_drv.ver_res     = screenHeight;
    disp_drv.flush_cb    = lvgl_flush_cb;
    disp_drv.draw_buf    = &draw_buf;
    disp_drv.direct_mode = false;          // partial buffer — cannot use direct_mode
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

    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    // Draw only the dirty area — works with partial buffer mode
#if (LV_COLOR_16_SWAP != 0)
    gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)color_p, w, h);
#else
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)color_p, w, h);
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
