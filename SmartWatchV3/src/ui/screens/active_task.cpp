/**
 * Active Task Screen — Implementation
 *
 * Layout (320x240):
 * [Status bar + Focus Shield icon when active]
 * [Task title — 24px, centered]
 * [Arc progress ring with countdown "24:59" — 36px green]
 * [Start/Pause btn (accent)    Stop btn (ghost red)]
 */

#include "active_task.h"
#include "../theme.h"
#include "../gesture.h"
#include "../screen_manager.h"
#include "../widgets/status_bar.h"
#include "../../services/task_service.h"
#include "../../services/focus_service.h"
#include "../../services/notification_service.h"
#include "../../services/power_service.h"
#include "../../hardware_config.h"
#include "../../ble_notifications.h"

static lv_obj_t *_titleLabel = NULL;
static lv_obj_t *_timerLabel = NULL;
static lv_obj_t *_arc = NULL;
static lv_obj_t *_startBtn = NULL;
static lv_obj_t *_stopBtn = NULL;
static lv_obj_t *_completeBtn = NULL;
static lv_timer_t *_updateTimer = NULL;
static bool _celebrationShown = false;

static void _updateUI(void);
static void _showCompletionCelebration(void);

static void _formatTime(uint32_t seconds, char *buf, size_t len) {
    uint32_t m = seconds / 60;
    uint32_t s = seconds % 60;
    snprintf(buf, len, "%02lu:%02lu", (unsigned long)m, (unsigned long)s);
}

static void _onTimerTick(lv_timer_t *t) {
    FocusState prevState = FocusService::getState();
    FocusService::update();
    FocusState newState = FocusService::getState();

    // Detect completion transition
    if (prevState == FOCUS_ACTIVE && newState == FOCUS_COMPLETE && !_celebrationShown) {
        _celebrationShown = true;
        // End focus mode
        PowerService::setFocusMode(false);
        NotificationService::setFocusShieldActive(false);
        _showCompletionCelebration();
    }

    _updateUI();
}

static void _onStartPause(lv_event_t *e) {
    FocusState state = FocusService::getState();
    if (state == FOCUS_IDLE || state == FOCUS_COMPLETE) {
        const Task *task = TaskService::getActiveTask();
        uint16_t dur = task ? task->durationMin : 25;
        FocusService::startTimer(dur);
        _celebrationShown = false;
        // Activate focus mode
        PowerService::setFocusMode(true);
        NotificationService::setFocusShieldActive(true);
    } else if (state == FOCUS_ACTIVE) {
        FocusService::pauseTimer();
    } else if (state == FOCUS_PAUSED) {
        FocusService::resumeTimer();
    }
    _updateUI();
}

static void _onStop(lv_event_t *e) {
    FocusService::stopTimer();
    PowerService::setFocusMode(false);
    NotificationService::setFocusShieldActive(false);
    _updateUI();
}

static void _onComplete(lv_event_t *e) {
    int8_t idx = TaskService::getActiveTaskIndex();
    if (idx >= 0) {
        const Task *task = TaskService::getActiveTask();
        TaskService::completeTask(idx);
        // Sync completion back to phone via BLE (FR3, Story 3.2)
        if (task) {
            sendTaskCompletion(task->id);
        }
    }
    FocusService::stopTimer();
    PowerService::setFocusMode(false);
    NotificationService::setFocusShieldActive(false);
    ScreenManager::goBack();
}

static void _onGesture(GestureDir dir, int16_t startX, int16_t startY) {
    // Only allow back gesture if timer is not active
    if (dir == GESTURE_RIGHT && FocusService::getState() == FOCUS_IDLE) {
        ScreenManager::goBack();
    }
}

static void _updateUI(void) {
    FocusState state = FocusService::getState();

    // Timer text
    if (_timerLabel) {
        char buf[8];
        uint32_t remaining = FocusService::getRemainingSeconds();
        _formatTime(remaining, buf, sizeof(buf));
        lv_label_set_text(_timerLabel, buf);

        // Color based on state
        lv_color_t color;
        switch (state) {
            case FOCUS_ACTIVE:  color = COLOR_ACCENT_GREEN; break;
            case FOCUS_PAUSED:  color = COLOR_ACCENT_AMBER; break;
            case FOCUS_COMPLETE: color = COLOR_ACCENT; break;
            default:            color = COLOR_TEXT_SECONDARY; break;
        }
        lv_obj_set_style_text_color(_timerLabel, color, 0);
    }

    // Arc progress
    if (_arc) {
        uint32_t total = FocusService::getTotalSeconds();
        uint32_t elapsed = FocusService::getElapsedSeconds();
        if (total > 0) {
            uint16_t angle = (uint16_t)((elapsed * 360UL) / total);
            lv_arc_set_value(_arc, angle);
        } else {
            lv_arc_set_value(_arc, 0);
        }

        // Arc color
        if (state == FOCUS_ACTIVE) {
            lv_obj_set_style_arc_color(_arc, COLOR_ACCENT_GREEN, LV_PART_INDICATOR);
        } else if (state == FOCUS_PAUSED) {
            lv_obj_set_style_arc_color(_arc, COLOR_ACCENT_AMBER, LV_PART_INDICATOR);
        } else if (state == FOCUS_COMPLETE) {
            lv_obj_set_style_arc_color(_arc, COLOR_ACCENT, LV_PART_INDICATOR);
        }
    }

    // Button labels
    if (_startBtn) {
        lv_obj_t *label = lv_obj_get_child(_startBtn, 0);
        if (label) {
            switch (state) {
                case FOCUS_ACTIVE:  lv_label_set_text(label, LV_SYMBOL_PAUSE " Pause"); break;
                case FOCUS_PAUSED:  lv_label_set_text(label, LV_SYMBOL_PLAY " Resume"); break;
                case FOCUS_COMPLETE: lv_label_set_text(label, LV_SYMBOL_REFRESH " Restart"); break;
                default:            lv_label_set_text(label, LV_SYMBOL_PLAY " Start"); break;
            }
        }
    }

    // Stop button visibility
    if (_stopBtn) {
        if (state == FOCUS_ACTIVE || state == FOCUS_PAUSED) {
            lv_obj_clear_flag(_stopBtn, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(_stopBtn, LV_OBJ_FLAG_HIDDEN);
        }
    }

    // Focus Shield indicator
    StatusBar::setFocusShield(state == FOCUS_ACTIVE);
}

void ActiveTask::create(lv_obj_t *screen) {
    const Task *task = TaskService::getActiveTask();

    lv_obj_t *content = lv_obj_create(screen);
    lv_obj_remove_style_all(content);
    lv_obj_set_size(content, SCREEN_WIDTH, SCREEN_HEIGHT - STATUS_BAR_HEIGHT);
    lv_obj_set_pos(content, 0, STATUS_BAR_HEIGHT);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    // Task title
    _titleLabel = lv_label_create(content);
    lv_obj_add_style(_titleLabel, &style_label_heading, 0);
    lv_label_set_text(_titleLabel, task ? task->title : "No Task Selected");
    lv_label_set_long_mode(_titleLabel, LV_LABEL_LONG_DOT);
    lv_obj_set_width(_titleLabel, SCREEN_WIDTH - SPACE_LG * 2);
    lv_obj_set_style_text_align(_titleLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(_titleLabel, LV_ALIGN_TOP_MID, 0, SPACE_SM);

    // Arc progress ring
    _arc = lv_arc_create(content);
    lv_obj_set_size(_arc, 120, 120);
    lv_arc_set_rotation(_arc, 270);
    lv_arc_set_range(_arc, 0, 360);
    lv_arc_set_value(_arc, 0);
    lv_arc_set_bg_angles(_arc, 0, 360);
    lv_obj_remove_style(_arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(_arc, 6, LV_PART_MAIN);
    lv_obj_set_style_arc_color(_arc, COLOR_DIVIDER, LV_PART_MAIN);
    lv_obj_set_style_arc_width(_arc, 6, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(_arc, COLOR_ACCENT_GREEN, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(_arc, true, LV_PART_INDICATOR);
    lv_obj_align(_arc, LV_ALIGN_CENTER, 0, -8);

    // Timer text inside arc
    _timerLabel = lv_label_create(content);
    lv_obj_add_style(_timerLabel, &style_label_heading, 0);
    lv_obj_set_style_text_font(_timerLabel, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_color(_timerLabel, COLOR_TEXT_SECONDARY, 0);
    char buf[8];
    uint16_t dur = task ? task->durationMin : 25;
    _formatTime(dur * 60, buf, sizeof(buf));
    lv_label_set_text(_timerLabel, buf);
    lv_obj_align(_timerLabel, LV_ALIGN_CENTER, 0, -8);

    // Button row at bottom
    lv_obj_t *btnRow = lv_obj_create(content);
    lv_obj_remove_style_all(btnRow);
    lv_obj_set_size(btnRow, SCREEN_WIDTH - SPACE_LG * 2, TOUCH_MIN_SIZE + SPACE_SM);
    lv_obj_align(btnRow, LV_ALIGN_BOTTOM_MID, 0, -SPACE_SM);
    lv_obj_set_flex_flow(btnRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btnRow, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(btnRow, SPACE_MD, 0);
    lv_obj_clear_flag(btnRow, LV_OBJ_FLAG_SCROLLABLE);

    // Start/Pause button
    _startBtn = theme_create_btn_primary(btnRow, LV_SYMBOL_PLAY " Start");
    lv_obj_set_flex_grow(_startBtn, 1);
    lv_obj_add_event_cb(_startBtn, _onStartPause, LV_EVENT_CLICKED, NULL);

    // Stop button (hidden initially)
    _stopBtn = theme_create_btn_ghost(btnRow, LV_SYMBOL_STOP " Stop");
    lv_obj_set_flex_grow(_stopBtn, 1);
    lv_obj_set_style_border_color(_stopBtn, COLOR_ACCENT_RED, 0);
    lv_obj_set_style_text_color(_stopBtn, COLOR_ACCENT_RED, 0);
    lv_obj_add_event_cb(_stopBtn, _onStop, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(_stopBtn, LV_OBJ_FLAG_HIDDEN);

    // Complete button (small, top right)
    _completeBtn = lv_btn_create(content);
    lv_obj_remove_style_all(_completeBtn);
    lv_obj_add_style(_completeBtn, &style_btn_ghost, 0);
    lv_obj_add_style(_completeBtn, &style_btn_ghost_pressed, LV_STATE_PRESSED);
    lv_obj_set_size(_completeBtn, LV_SIZE_CONTENT, 32);
    lv_obj_set_style_pad_hor(_completeBtn, SPACE_MD, 0);
    lv_obj_set_style_min_height(_completeBtn, 32, 0);
    lv_obj_set_style_text_font(_completeBtn, &lv_font_montserrat_14, 0);
    lv_obj_align(_completeBtn, LV_ALIGN_TOP_RIGHT, -SPACE_SM, SPACE_SM);
    lv_obj_t *compLabel = lv_label_create(_completeBtn);
    lv_label_set_text(compLabel, LV_SYMBOL_OK " Done");
    lv_obj_center(compLabel);
    lv_obj_add_event_cb(_completeBtn, _onComplete, LV_EVENT_CLICKED, NULL);

    // Focus Shield activates when Active Task Screen is displayed (PRD AC 2.5.1)
    NotificationService::setFocusShieldActive(true);
    StatusBar::setFocusShield(true);

    // Timer update (every second)
    _updateTimer = lv_timer_create(_onTimerTick, 1000, NULL);

    // Initial UI state
    _updateUI();

    // Gesture
    Gesture::setCallback(_onGesture);
    Gesture::attachToScreen(screen);
}

/**
 * Timer completion celebration.
 * Full-screen green flash for 3 seconds with "Session Complete!" text.
 * Vibrates if motor available.
 */
static void _showCompletionCelebration(void) {
    vibrateMotor(200);

    // Green flash overlay on lv_layer_top
    lv_obj_t *flash = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(flash);
    lv_obj_set_size(flash, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(flash, 0, 0);
    lv_obj_set_style_bg_color(flash, COLOR_ACCENT_GREEN, 0);
    lv_obj_set_style_bg_opa(flash, LV_OPA_COVER, 0);
    lv_obj_clear_flag(flash, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *icon = lv_label_create(flash);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(icon, lv_color_white(), 0);
    lv_label_set_text(icon, LV_SYMBOL_OK);
    lv_obj_align(icon, LV_ALIGN_CENTER, 0, -30);

    lv_obj_t *msg = lv_label_create(flash);
    lv_obj_set_style_text_font(msg, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(msg, lv_color_white(), 0);
    lv_label_set_text(msg, "Session Complete!");
    lv_obj_align(msg, LV_ALIGN_CENTER, 0, 20);

    // Auto-remove after 3 seconds
    lv_timer_t *removeTimer = lv_timer_create([](lv_timer_t *t) {
        lv_obj_t *obj = (lv_obj_t *)t->user_data;
        if (obj) lv_obj_del(obj);
        lv_timer_del(t);
    }, 3000, flash);
    lv_timer_set_repeat_count(removeTimer, 1);
}

void ActiveTask::destroy(void) {
    if (_updateTimer) {
        lv_timer_del(_updateTimer);
        _updateTimer = NULL;
    }
    // Deactivate Focus Shield on screen exit (PRD AC 2.5.6)
    NotificationService::setFocusShieldActive(false);
    PowerService::setFocusMode(false);
    StatusBar::setFocusShield(false);
    _titleLabel = NULL;
    _timerLabel = NULL;
    _arc = NULL;
    _startBtn = NULL;
    _stopBtn = NULL;
    _completeBtn = NULL;
    _celebrationShown = false;
}
