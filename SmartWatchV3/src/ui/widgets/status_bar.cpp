/**
 * Status Bar Widget — Implementation
 */

#include "status_bar.h"
#include "../theme.h"

static lv_obj_t *_bar = NULL;
static lv_obj_t *_timeLabel = NULL;
static lv_obj_t *_batteryLabel = NULL;
static lv_obj_t *_bleIcon = NULL;
static lv_obj_t *_shieldIcon = NULL;

void StatusBar::init(void) {
    // Create on top layer so it persists across screen transitions
    _bar = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(_bar);
    lv_obj_add_style(_bar, &style_status_bar, 0);
    lv_obj_set_size(_bar, SCREEN_WIDTH, STATUS_BAR_HEIGHT);
    lv_obj_set_pos(_bar, 0, 0);
    lv_obj_clear_flag(_bar, LV_OBJ_FLAG_SCROLLABLE);

    // Use flex layout: left-to-right, space between
    lv_obj_set_flex_flow(_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(_bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Left group: shield icon + time
    lv_obj_t *left = lv_obj_create(_bar);
    lv_obj_remove_style_all(left);
    lv_obj_set_size(left, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(left, SPACE_SM, 0);
    lv_obj_clear_flag(left, LV_OBJ_FLAG_SCROLLABLE);

    _shieldIcon = lv_label_create(left);
    lv_obj_add_style(_shieldIcon, &style_label_caption, 0);
    lv_label_set_text(_shieldIcon, LV_SYMBOL_EYE_CLOSE);
    lv_obj_set_style_text_color(_shieldIcon, COLOR_ACCENT_GREEN, 0);
    lv_obj_add_flag(_shieldIcon, LV_OBJ_FLAG_HIDDEN);  // hidden by default

    _timeLabel = lv_label_create(left);
    lv_obj_add_style(_timeLabel, &style_label_caption, 0);
    lv_label_set_text(_timeLabel, "--:--");

    // Right group: BLE + battery
    lv_obj_t *right = lv_obj_create(_bar);
    lv_obj_remove_style_all(right);
    lv_obj_set_size(right, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(right, SPACE_SM, 0);
    lv_obj_clear_flag(right, LV_OBJ_FLAG_SCROLLABLE);

    _bleIcon = lv_label_create(right);
    lv_obj_add_style(_bleIcon, &style_label_caption, 0);
    lv_label_set_text(_bleIcon, LV_SYMBOL_BLUETOOTH);
    lv_obj_set_style_text_color(_bleIcon, COLOR_TEXT_DISABLED, 0);

    _batteryLabel = lv_label_create(right);
    lv_obj_add_style(_batteryLabel, &style_label_caption, 0);
    lv_label_set_text(_batteryLabel, "-- %");
}

void StatusBar::setTime(uint8_t hour, uint8_t minute) {
    if (!_timeLabel) return;
    lv_label_set_text_fmt(_timeLabel, "%02d:%02d", hour, minute);
}

void StatusBar::setBattery(uint8_t percent) {
    if (!_batteryLabel) return;
    lv_label_set_text_fmt(_batteryLabel, "%d%%", percent);

    // Amber warning when low
    if (percent <= 20) {
        lv_obj_set_style_text_color(_batteryLabel, COLOR_ACCENT_AMBER, 0);
    } else {
        lv_obj_set_style_text_color(_batteryLabel, COLOR_TEXT_SECONDARY, 0);
    }
}

void StatusBar::setBLEConnected(bool connected) {
    if (!_bleIcon) return;
    lv_obj_set_style_text_color(_bleIcon,
        connected ? COLOR_ACCENT : COLOR_TEXT_DISABLED, 0);
}

void StatusBar::setFocusShield(bool active) {
    if (!_shieldIcon) return;
    if (active) {
        lv_obj_clear_flag(_shieldIcon, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(_shieldIcon, LV_OBJ_FLAG_HIDDEN);
    }
}

void StatusBar::setVisible(bool visible) {
    if (!_bar) return;
    if (visible) {
        lv_obj_clear_flag(_bar, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(_bar, LV_OBJ_FLAG_HIDDEN);
    }
}
