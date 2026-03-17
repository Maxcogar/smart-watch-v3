/**
 * Watch Face Screen — Implementation
 *
 * Layout (320x240 landscape):
 * [Status Bar — 28px]
 * [                                    ]
 * [     12:34       — 48px, centered   ]
 * [  Tuesday, Mar 17 — 20px, secondary ]
 * [                                    ]
 * [  [Next Task card]   [Timer card]   ]
 */

#include "watch_face.h"
#include "../theme.h"
#include "../gesture.h"
#include "../screen_manager.h"
#include "../widgets/status_bar.h"
#include "control_panel.h"

static lv_obj_t *_timeLabel = NULL;
static lv_obj_t *_dateLabel = NULL;
static lv_obj_t *_taskCard = NULL;
static lv_obj_t *_timerCard = NULL;
static lv_obj_t *_taskTitle = NULL;
static lv_obj_t *_timerStatus = NULL;
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

    // Time — HH:MM
    lv_label_set_text_fmt(_timeLabel, "%02d:%02d", ti.tm_hour, ti.tm_min);

    // Date — "Wednesday, Mar 17"
    lv_label_set_text_fmt(_dateLabel, "%s, %s %d",
        _dayNames[ti.tm_wday], _monthNames[ti.tm_mon], ti.tm_mday);

    // Also update status bar time
    StatusBar::setTime(ti.tm_hour, ti.tm_min);
}

static void _onTaskCardTap(lv_event_t *e) {
    ScreenManager::navigate(SCREEN_TASK_LIST, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200);
}

static void _onGesture(GestureDir dir, int16_t startX, int16_t startY) {
    switch (dir) {
        case GESTURE_LEFT:
            // Right-edge swipe opens control panel
            if (startX > SCREEN_WIDTH - 40) {
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

void WatchFace::create(lv_obj_t *screen) {
    // Content area below status bar
    lv_obj_t *content = lv_obj_create(screen);
    lv_obj_remove_style_all(content);
    lv_obj_set_size(content, SCREEN_WIDTH, SCREEN_HEIGHT - STATUS_BAR_HEIGHT);
    lv_obj_set_pos(content, 0, STATUS_BAR_HEIGHT);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    // --- Time label (48px, centered) ---
    _timeLabel = lv_label_create(content);
    lv_obj_add_style(_timeLabel, &style_label_time, 0);
    lv_label_set_text(_timeLabel, "--:--");
    lv_obj_align(_timeLabel, LV_ALIGN_TOP_MID, 0, SPACE_XL);

    // --- Date label (20px, below time) ---
    _dateLabel = lv_label_create(content);
    lv_obj_add_style(_dateLabel, &style_label_body, 0);
    lv_obj_set_style_text_color(_dateLabel, COLOR_TEXT_SECONDARY, 0);
    lv_label_set_text(_dateLabel, "");
    lv_obj_align_to(_dateLabel, _timeLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, SPACE_XS);

    // --- Bottom card row ---
    lv_obj_t *cardRow = lv_obj_create(content);
    lv_obj_remove_style_all(cardRow);
    lv_obj_set_size(cardRow, SCREEN_WIDTH - SPACE_LG * 2, 72);
    lv_obj_align(cardRow, LV_ALIGN_BOTTOM_MID, 0, -SPACE_SM);
    lv_obj_set_flex_flow(cardRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(cardRow, SPACE_MD, 0);
    lv_obj_clear_flag(cardRow, LV_OBJ_FLAG_SCROLLABLE);

    // --- Task preview card ---
    _taskCard = theme_create_card(cardRow);
    lv_obj_set_flex_grow(_taskCard, 1);
    lv_obj_set_height(_taskCard, 68);

    lv_obj_t *taskIcon = lv_label_create(_taskCard);
    lv_obj_add_style(taskIcon, &style_label_caption, 0);
    lv_obj_set_style_text_color(taskIcon, COLOR_ACCENT, 0);
    lv_label_set_text(taskIcon, LV_SYMBOL_LIST "  Tasks");
    lv_obj_align(taskIcon, LV_ALIGN_TOP_LEFT, 0, 0);

    _taskTitle = lv_label_create(_taskCard);
    lv_obj_add_style(_taskTitle, &style_label_body, 0);
    lv_label_set_text(_taskTitle, "No active task");
    lv_obj_set_style_text_color(_taskTitle, COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_width(_taskTitle, lv_pct(90));
    lv_label_set_long_mode(_taskTitle, LV_LABEL_LONG_DOT);
    lv_obj_align(_taskTitle, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    lv_obj_add_event_cb(_taskCard, _onTaskCardTap, LV_EVENT_CLICKED, NULL);

    // --- Timer preview card ---
    _timerCard = theme_create_card(cardRow);
    lv_obj_set_flex_grow(_timerCard, 1);
    lv_obj_set_height(_timerCard, 68);

    lv_obj_t *timerIcon = lv_label_create(_timerCard);
    lv_obj_add_style(timerIcon, &style_label_caption, 0);
    lv_obj_set_style_text_color(timerIcon, COLOR_ACCENT_GREEN, 0);
    lv_label_set_text(timerIcon, LV_SYMBOL_PLAY "  Timer");
    lv_obj_align(timerIcon, LV_ALIGN_TOP_LEFT, 0, 0);

    _timerStatus = lv_label_create(_timerCard);
    lv_obj_add_style(_timerStatus, &style_label_body, 0);
    lv_label_set_text(_timerStatus, "Idle");
    lv_obj_set_style_text_color(_timerStatus, COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(_timerStatus, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    // --- Clock update timer (every second) ---
    _clockTimer = lv_timer_create(_updateClock, 1000, NULL);
    _updateClock(NULL);  // immediate first update

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
    _taskCard = NULL;
    _timerCard = NULL;
    _taskTitle = NULL;
    _timerStatus = NULL;
}
