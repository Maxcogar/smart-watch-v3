/**
 * Control Panel — Implementation
 *
 * Rendered as overlay on lv_layer_top() (below status bar).
 * Panel is 200px wide, slides in from right edge.
 * Left area is a dimmed overlay that dismisses on tap.
 */

#include "control_panel.h"
#include "../theme.h"
#include "../../services/connectivity_service.h"
#include "../../services/storage_service.h"
#include "../../hardware_config.h"

static lv_obj_t *_overlay = NULL;
static lv_obj_t *_panel = NULL;
static bool _visible = false;

static lv_obj_t *_brightnessSlider = NULL;
static lv_obj_t *_wifiSwitch = NULL;
static lv_obj_t *_compressorSwitch = NULL;
static lv_obj_t *_batteryDetail = NULL;

#define PANEL_WIDTH 200

static void _onOverlayTap(lv_event_t *e) {
    lv_obj_t *target = (lv_obj_t *)lv_event_get_target(e);
    // Only dismiss if tapping the overlay background, not the panel
    if (target == _overlay) {
        ControlPanel::hide();
    }
}

static void _onBrightnessChange(lv_event_t *e) {
    lv_obj_t *slider = (lv_obj_t *)lv_event_get_target(e);
    int32_t val = lv_slider_get_value(slider);
    // Map 0-100 to 0-255
    uint8_t brightness = (uint8_t)(val * 255 / 100);
    setDisplayBrightness(brightness);
}

static void _onWifiToggle(lv_event_t *e) {
    lv_obj_t *sw = (lv_obj_t *)lv_event_get_target(e);
    bool on = lv_obj_has_state(sw, LV_STATE_CHECKED);
    if (on) {
        // Load WiFi creds from storage
        WatchSettings settings;
        if (StorageService::loadSettings(settings) && strlen(settings.wifiSSID) > 0) {
            ConnectivityService::connectWiFi(settings.wifiSSID, settings.wifiPass);
        } else {
            Serial.println("No WiFi credentials configured");
            lv_obj_clear_state(sw, LV_STATE_CHECKED);
        }
    } else {
        ConnectivityService::disconnectWiFi();
    }
}

static void _onCompressorToggle(lv_event_t *e) {
    lv_obj_t *sw = (lv_obj_t *)lv_event_get_target(e);
    bool on = lv_obj_has_state(sw, LV_STATE_CHECKED);

    WatchSettings settings;
    if (!StorageService::loadSettings(settings)) {
        Serial.println("No compressor IPs configured");
        return;
    }

    // Send to both IPs
    bool ok1 = ConnectivityService::sendCompressorCommand(settings.compressorIP1, on);
    bool ok2 = ConnectivityService::sendCompressorCommand(settings.compressorIP2, on);

    if (!ok1 && !ok2) {
        // Revert switch on failure
        if (on) lv_obj_clear_state(sw, LV_STATE_CHECKED);
        else lv_obj_add_state(sw, LV_STATE_CHECKED);
    }
}

static lv_obj_t *_createRow(lv_obj_t *parent, const char *label) {
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, lv_pct(100), TOUCH_MIN_SIZE);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(row);
    lv_obj_add_style(lbl, &style_label_body, 0);
    lv_label_set_text(lbl, label);

    return row;
}

void ControlPanel::show(void) {
    if (_visible) return;
    _visible = true;

    // Full-screen overlay for tap-to-dismiss
    _overlay = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(_overlay);
    lv_obj_set_size(_overlay, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(_overlay, 0, 0);
    lv_obj_set_style_bg_color(_overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(_overlay, LV_OPA_40, 0);
    lv_obj_add_flag(_overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(_overlay, _onOverlayTap, LV_EVENT_CLICKED, NULL);

    // Panel (right side)
    _panel = lv_obj_create(_overlay);
    lv_obj_remove_style_all(_panel);
    lv_obj_set_size(_panel, PANEL_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_style_bg_color(_panel, COLOR_BG_CARD, 0);
    lv_obj_set_style_bg_opa(_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(_panel, RADIUS_CARD, 0);
    lv_obj_set_style_border_width(_panel, 1, 0);
    lv_obj_set_style_border_color(_panel, COLOR_DIVIDER, 0);
    lv_obj_set_style_border_side(_panel, LV_BORDER_SIDE_LEFT, 0);
    lv_obj_set_style_pad_all(_panel, SPACE_LG, 0);
    lv_obj_set_style_pad_gap(_panel, SPACE_SM, 0);
    lv_obj_set_flex_flow(_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(_panel, LV_OBJ_FLAG_SCROLLABLE);

    // Start off-screen right, animate in
    lv_obj_set_pos(_panel, SCREEN_WIDTH, 0);

    // Header
    lv_obj_t *header = lv_label_create(_panel);
    lv_obj_add_style(header, &style_label_heading, 0);
    lv_label_set_text(header, "Controls");

    // Brightness
    lv_obj_t *brightRow = _createRow(_panel, LV_SYMBOL_SETTINGS " Brightness");
    _brightnessSlider = lv_slider_create(brightRow);
    lv_obj_set_width(_brightnessSlider, 80);
    lv_slider_set_range(_brightnessSlider, 10, 100);
    lv_slider_set_value(_brightnessSlider, 80, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(_brightnessSlider, COLOR_DIVIDER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(_brightnessSlider, COLOR_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(_brightnessSlider, COLOR_ACCENT, LV_PART_KNOB);
    lv_obj_add_event_cb(_brightnessSlider, _onBrightnessChange, LV_EVENT_VALUE_CHANGED, NULL);

    // WiFi toggle
    lv_obj_t *wifiRow = _createRow(_panel, LV_SYMBOL_WIFI " WiFi");
    _wifiSwitch = lv_switch_create(wifiRow);
    lv_obj_set_style_bg_color(_wifiSwitch, COLOR_DIVIDER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(_wifiSwitch, COLOR_ACCENT, LV_PART_INDICATOR | LV_STATE_CHECKED);
    if (ConnectivityService::isWiFiConnected()) {
        lv_obj_add_state(_wifiSwitch, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(_wifiSwitch, _onWifiToggle, LV_EVENT_VALUE_CHANGED, NULL);

    // Compressor toggle
    lv_obj_t *compRow = _createRow(_panel, "Compressor");
    _compressorSwitch = lv_switch_create(compRow);
    lv_obj_set_style_bg_color(_compressorSwitch, COLOR_DIVIDER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(_compressorSwitch, COLOR_ACCENT_GREEN, LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_add_event_cb(_compressorSwitch, _onCompressorToggle, LV_EVENT_VALUE_CHANGED, NULL);

    // Battery detail
    _batteryDetail = lv_label_create(_panel);
    lv_obj_add_style(_batteryDetail, &style_label_caption, 0);
    lv_label_set_text_fmt(_batteryDetail, LV_SYMBOL_CHARGE " Battery: %d%%", getBatteryPercentage());

    // BLE status
    lv_obj_t *bleLabel = lv_label_create(_panel);
    lv_obj_add_style(bleLabel, &style_label_caption, 0);
    lv_label_set_text(bleLabel, LV_SYMBOL_BLUETOOTH " BLE: Connected");

    // Slide-in animation
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, _panel);
    lv_anim_set_values(&a, SCREEN_WIDTH, SCREEN_WIDTH - PANEL_WIDTH);
    lv_anim_set_time(&a, 200);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_exec_cb(&a, [](void *obj, int32_t v) {
        lv_obj_set_x((lv_obj_t *)obj, v);
    });
    lv_anim_start(&a);
}

void ControlPanel::hide(void) {
    if (!_visible) return;
    _visible = false;

    if (_panel) {
        // Slide-out animation
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, _panel);
        lv_anim_set_values(&a, SCREEN_WIDTH - PANEL_WIDTH, SCREEN_WIDTH);
        lv_anim_set_time(&a, 200);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
        lv_anim_set_exec_cb(&a, [](void *obj, int32_t v) {
            lv_obj_set_x((lv_obj_t *)obj, v);
        });
        lv_anim_set_ready_cb(&a, [](lv_anim_t *a) {
            if (_overlay) {
                lv_obj_del(_overlay);
                _overlay = NULL;
                _panel = NULL;
            }
        });
        lv_anim_start(&a);
    }
}

bool ControlPanel::isVisible(void) {
    return _visible;
}
