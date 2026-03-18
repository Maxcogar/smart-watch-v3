/**
 * Priority Alert — Implementation
 *
 * Full-screen red overlay on lv_layer_top().
 * Shows for 3 seconds minimum, then waits for touch to dismiss.
 * Bypasses Focus Shield.
 */

#include "priority_alert.h"
#include <cstring>
#include "../theme.h"

static lv_obj_t *_overlay = NULL;
static lv_timer_t *_autoTimer = NULL;
static bool _active = false;
static bool _dismissable = false;

static void _onTouch(lv_event_t *e) {
    if (_dismissable) {
        PriorityAlert::dismiss();
    }
}

static void _onAutoTimeout(lv_timer_t *t) {
    _dismissable = true;
    lv_timer_del(t);
    _autoTimer = NULL;
}

void PriorityAlert::show(const char *message) {
    if (_active) return;
    _active = true;
    _dismissable = false;

    // Create full-screen overlay on top layer
    _overlay = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(_overlay);
    lv_obj_set_size(_overlay, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(_overlay, 0, 0);
    lv_obj_set_style_bg_color(_overlay, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_bg_opa(_overlay, LV_OPA_COVER, 0);
    lv_obj_clear_flag(_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(_overlay, LV_OBJ_FLAG_CLICKABLE);

    // Warning icon
    lv_obj_t *icon = lv_label_create(_overlay);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(icon, lv_color_white(), 0);
    lv_label_set_text(icon, LV_SYMBOL_WARNING);
    lv_obj_align(icon, LV_ALIGN_CENTER, 0, -50);

    // "PRIORITY ALERT" text
    lv_obj_t *title = lv_label_create(_overlay);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_label_set_text(title, "PRIORITY ALERT");
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 10);

    // Message
    if (message && strlen(message) > 0) {
        lv_obj_t *msg = lv_label_create(_overlay);
        lv_obj_set_style_text_font(msg, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(msg, lv_color_white(), 0);
        lv_label_set_text(msg, message);
        lv_obj_set_width(msg, SCREEN_WIDTH - SPACE_XL * 2);
        lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);
        lv_label_set_long_mode(msg, LV_LABEL_LONG_WRAP);
        lv_obj_align(msg, LV_ALIGN_CENTER, 0, 55);
    }

    // Hint (appears after 3 seconds)
    lv_obj_t *hint = lv_label_create(_overlay);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(hint, lv_color_white(), 0);
    lv_obj_set_style_opa(hint, LV_OPA_60, 0);
    lv_label_set_text(hint, "Tap anywhere to dismiss");
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -SPACE_LG);

    // Touch to dismiss
    lv_obj_add_event_cb(_overlay, _onTouch, LV_EVENT_CLICKED, NULL);

    // 3-second minimum display
    _autoTimer = lv_timer_create(_onAutoTimeout, 3000, NULL);
    lv_timer_set_repeat_count(_autoTimer, 1);

    // Fade in
    lv_obj_set_style_opa(_overlay, LV_OPA_0, 0);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, _overlay);
    lv_anim_set_values(&a, 0, 255);
    lv_anim_set_time(&a, 100);
    lv_anim_set_exec_cb(&a, [](void *obj, int32_t v) {
        lv_obj_set_style_opa((lv_obj_t *)obj, (lv_opa_t)v, 0);
    });
    lv_anim_start(&a);
}

void PriorityAlert::dismiss(void) {
    if (!_active) return;
    _active = false;

    if (_autoTimer) {
        lv_timer_del(_autoTimer);
        _autoTimer = NULL;
    }

    if (_overlay) {
        lv_obj_del(_overlay);
        _overlay = NULL;
    }
}

bool PriorityAlert::isActive(void) {
    return _active;
}
