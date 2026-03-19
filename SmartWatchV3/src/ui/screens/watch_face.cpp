/**
 * Watch Face Screen — Implementation
 *
 * Layout (320x240 landscape, PRD AC 1.2.1–1.2.6):
 * [Status Bar — 28px]
 * [     12:34       — 48px, centered              ]
 * [  Tuesday, Mar 17 — 20px, secondary            ]
 * [  [Tasks] [Timer]   — 2x2 icon grid, 44px min  ]
 * [  [Notif] [Settings]                            ]
 */

#include "watch_face.h"
#include <ctime>
#include "../theme.h"
#include "../gesture.h"
#include "../screen_manager.h"
#include "../widgets/status_bar.h"
#include "control_panel.h"

static lv_obj_t *_timeLabel = NULL;
static lv_obj_t *_dateLabel = NULL;
static lv_timer_t *_clockTimer = NULL;

static const char *_dayNames[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday"
};
static const char *_monthNames[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static void _updateClock(lv_timer_t *t) {
    if (!_timeLabel || !_dateLabel) return;

    time_t now;
    struct tm ti;
    time(&now);
    localtime_r(&now, &ti);

    lv_label_set_text_fmt(_timeLabel, "%02d:%02d", ti.tm_hour, ti.tm_min);
    lv_label_set_text_fmt(_dateLabel, "%s, %s %d",
        _dayNames[ti.tm_wday], _monthNames[ti.tm_mon], ti.tm_mday);

    StatusBar::setTime(ti.tm_hour, ti.tm_min);
}

static void _onNavTasks(lv_event_t *e) {
    ScreenManager::navigate(SCREEN_TASK_LIST, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200);
}

static void _onNavTimer(lv_event_t *e) {
    ScreenManager::navigate(SCREEN_ACTIVE_TASK, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200);
}

static void _onNavNotif(lv_event_t *e) {
    ScreenManager::navigate(SCREEN_NOTIFICATIONS, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 200);
}

static void _onNavSettings(lv_event_t *e) {
    ControlPanel::show();
}

static void _onGesture(GestureDir dir, int16_t startX, int16_t startY) {
    switch (dir) {
        case GESTURE_LEFT:
            if (startX > SCREEN_WIDTH - 20) {  // PRD AC 3.3.1: within 20px of right edge
                ControlPanel::show();
            } else {
                ScreenManager::navigate(SCREEN_TASK_LIST, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200);
            }
            break;
        case GESTURE_UP:
            ScreenManager::navigate(SCREEN_NOTIFICATIONS, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 200);
            break;
        default:
            break;
    }
}

/**
 * Create a navigation icon button for the 2x2 grid.
 * 44px minimum touch target per PRD AC 1.2.2.
 */
static lv_obj_t *_createNavBtn(lv_obj_t *parent, const char *icon, const char *label,
                                lv_color_t iconColor, lv_event_cb_t cb) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_remove_style_all(btn);
    lv_obj_add_style(btn, &style_card, 0);
    lv_obj_add_style(btn, &style_card_pressed, LV_STATE_PRESSED);
    lv_obj_set_size(btn, (SCREEN_WIDTH - SPACE_LG * 2 - SPACE_MD) / 2, TOUCH_MIN_SIZE + SPACE_SM);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_hor(btn, SPACE_MD, 0);
    lv_obj_set_style_pad_gap(btn, SPACE_SM, 0);

    lv_obj_t *ico = lv_label_create(btn);
    lv_obj_add_style(ico, &style_label_body, 0);
    lv_obj_set_style_text_color(ico, iconColor, 0);
    lv_label_set_text(ico, icon);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_obj_add_style(lbl, &style_label_body, 0);
    lv_label_set_text(lbl, label);

    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);

    return btn;
}

void WatchFace::create(lv_obj_t *screen) {
    lv_obj_t *content = lv_obj_create(screen);
    lv_obj_remove_style_all(content);
    lv_obj_set_size(content, SCREEN_WIDTH, SCREEN_HEIGHT - STATUS_BAR_HEIGHT);
    lv_obj_set_pos(content, 0, STATUS_BAR_HEIGHT);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    // --- Time label (48px, centered — PRD AC 1.2.1) ---
    _timeLabel = lv_label_create(content);
    lv_obj_add_style(_timeLabel, &style_label_time, 0);
    lv_label_set_text(_timeLabel, "--:--");
    lv_obj_align(_timeLabel, LV_ALIGN_TOP_MID, 0, SPACE_SM);

    // --- Date label (20px, below time) ---
    _dateLabel = lv_label_create(content);
    lv_obj_add_style(_dateLabel, &style_label_body, 0);
    lv_obj_set_style_text_color(_dateLabel, COLOR_TEXT_SECONDARY, 0);
    lv_label_set_text(_dateLabel, "");
    lv_obj_align_to(_dateLabel, _timeLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, SPACE_XS);

    // --- 2x2 Navigation grid (PRD AC 1.2.2) ---
    lv_obj_t *grid = lv_obj_create(content);
    lv_obj_remove_style_all(grid);
    lv_obj_set_size(grid, SCREEN_WIDTH - SPACE_LG * 2,
                    (TOUCH_MIN_SIZE + SPACE_SM) * 2 + SPACE_SM);
    lv_obj_align(grid, LV_ALIGN_BOTTOM_MID, 0, -SPACE_XS);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_gap(grid, SPACE_SM, 0);
    lv_obj_set_style_pad_row(grid, SPACE_SM, 0);
    lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);

    _createNavBtn(grid, LV_SYMBOL_LIST,     "Tasks",   COLOR_ACCENT,       _onNavTasks);
    _createNavBtn(grid, LV_SYMBOL_PLAY,     "Timer",   COLOR_ACCENT_GREEN, _onNavTimer);
    _createNavBtn(grid, LV_SYMBOL_BELL,     "Notif",   COLOR_ACCENT_AMBER, _onNavNotif);
    _createNavBtn(grid, LV_SYMBOL_SETTINGS, "Settings",COLOR_TEXT_SECONDARY,_onNavSettings);

    // --- Clock update timer (every second) ---
    _clockTimer = lv_timer_create(_updateClock, 1000, NULL);
    _updateClock(NULL);

    // --- Gesture detection ---
    Gesture::setCallback(_onGesture);
    Gesture::attachToScreen(screen);
}

void WatchFace::destroy(void) {
    if (_clockTimer) {
        lv_timer_del(_clockTimer);
        _clockTimer = NULL;
    }
    _timeLabel = NULL;
    _dateLabel = NULL;
}
