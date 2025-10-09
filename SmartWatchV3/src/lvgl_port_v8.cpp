/**
 * LVGL Porting Layer Implementation
 */

#include "lvgl_port_v8.h"
#include <Arduino.h>
#include "bsp_cst816.h"
#include <Wire.h>
#include <Arduino_GFX_Library.h>

// Static variables
static lv_disp_draw_buf_t disp_buf;
static lv_disp_drv_t disp_drv;
static lv_indev_drv_t indev_drv;
static SemaphoreHandle_t lvgl_mutex = NULL;
static TaskHandle_t lvgl_task_handle = NULL;
static Arduino_GFX *gfx = NULL;

// Display buffers (using PSRAM if available)
static lv_color_t *buf1 = NULL;
static lv_color_t *buf2 = NULL;

// Forward declarations
static void lvgl_flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
static void lvgl_touch_cb(lv_indev_drv_t *indev, lv_indev_data_t *data);
static void lvgl_tick_task(void *arg);
static void lvgl_task(void *arg);

/**
 * Initialize LVGL with display and touch
 */
void lvgl_port_init(void *lcd, void *touch) {
    gfx = (Arduino_GFX *)lcd;
    
    // Initialize LVGL
    lv_init();
    
    // Allocate display buffers in PSRAM if available
    size_t buffer_size = LVGL_PORT_DISP_BUFFER_SIZE * sizeof(lv_color_t);
    
    if (psramFound()) {
        Serial.println("Allocating LVGL buffers in PSRAM");
        buf1 = (lv_color_t *)ps_malloc(buffer_size);
        buf2 = (lv_color_t *)ps_malloc(buffer_size);
    } else {
        Serial.println("Allocating LVGL buffers in regular RAM");
        buf1 = (lv_color_t *)malloc(buffer_size);
        buf2 = (lv_color_t *)malloc(buffer_size);
    }
    
    if (!buf1 || !buf2) {
        Serial.println("ERROR: Failed to allocate LVGL buffers!");
        return;
    }
    
    // Initialize display buffer
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, LVGL_PORT_DISP_BUFFER_SIZE);
    
    // Initialize display driver
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 240;  // Display width
    disp_drv.ver_res = 320;  // Display height
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &disp_buf;
    lv_disp_drv_register(&disp_drv);
    
    // Initialize touch driver
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = lvgl_touch_cb;
    lv_indev_drv_register(&indev_drv);
    
    // Create mutex for thread safety
    lvgl_mutex = xSemaphoreCreateMutex();
    if (!lvgl_mutex) {
        Serial.println("ERROR: Failed to create LVGL mutex!");
        return;
    }
    
    // Create tick timer (using ESP timer for accuracy)
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &lvgl_tick_task,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer);
    esp_timer_start_periodic(lvgl_tick_timer, LVGL_PORT_TICK_PERIOD_MS * 1000);
    
    // Create LVGL task
    xTaskCreatePinnedToCore(
        lvgl_task,
        "LVGL",
        LVGL_PORT_TASK_STACK_SIZE,
        NULL,
        LVGL_PORT_TASK_PRIORITY,
        &lvgl_task_handle,
        LVGL_PORT_TASK_CORE
    );
    
    Serial.println("LVGL port initialization complete");
}

/**
 * Lock LVGL mutex
 */
void lvgl_port_lock(int timeout_ms) {
    if (lvgl_mutex) {
        if (timeout_ms < 0) {
            xSemaphoreTake(lvgl_mutex, portMAX_DELAY);
        } else {
            xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(timeout_ms));
        }
    }
}

/**
 * Unlock LVGL mutex
 */
void lvgl_port_unlock(void) {
    if (lvgl_mutex) {
        xSemaphoreGive(lvgl_mutex);
    }
}

/**
 * Check if LVGL is ready
 */
bool lvgl_port_lock_ready(void) {
    return (lvgl_mutex != NULL);
}

/**
 * Display flush callback
 */
static void lvgl_flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    if (!gfx) {
        lv_disp_flush_ready(disp);
        return;
    }
    
    int32_t w = area->x2 - area->x1 + 1;
    int32_t h = area->y2 - area->y1 + 1;
    
    // Draw to display
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)color_p, w, h);
    
    // Notify LVGL that flushing is done
    lv_disp_flush_ready(disp);
}

/**
 * Touch read callback
 */
static void lvgl_touch_cb(lv_indev_drv_t *indev, lv_indev_data_t *data) {
    uint16_t touchpad_x;
    uint16_t touchpad_y;

    bsp_touch_read();
    if (bsp_touch_get_coordinates(&touchpad_x, &touchpad_y)) {
        data->point.x = touchpad_x;
        data->point.y = touchpad_y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

/**
 * LVGL tick timer callback
 */
static void lvgl_tick_task(void *arg) {
    lv_tick_inc(LVGL_PORT_TICK_PERIOD_MS);
}

/**
 * LVGL task - handles display updates
 */
static void lvgl_task(void *arg) {
    Serial.println("LVGL task started");
    
    while (1) {
        lvgl_port_lock(-1);
        lv_task_handler();
        lvgl_port_unlock();
        
        // Run at ~30 FPS
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}
