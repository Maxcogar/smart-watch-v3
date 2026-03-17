/**
 * Task List Screen
 *
 * Scrollable list of tasks with priority indicators.
 * Tap a task to set it active and navigate to Active Task Screen.
 *
 * Layout (320x240):
 * [Status bar — 28px]
 * [Header: "Tasks" — 24px]
 * [Scrollable card list]
 *   [Priority dot | Title | Duration badge]
 *   [56px min height per card]
 */

#include "task_list.h"
#include "../theme.h"
#include "../gesture.h"
#include "../screen_manager.h"
#include "../../services/task_service.h"
#include "control_panel.h"

static lv_obj_t *_listContainer = NULL;

static lv_color_t _priorityColor(uint8_t priority) {
    switch (priority) {
        case 1: return COLOR_ACCENT_RED;
        case 2: return COLOR_ACCENT_AMBER;
        default: return COLOR_ACCENT_GREEN;
    }
}

static void _onTaskTap(lv_event_t *e) {
    int8_t index = (int8_t)(intptr_t)lv_event_get_user_data(e);
    TaskService::setActiveTask(index);
    ScreenManager::navigate(SCREEN_ACTIVE_TASK, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200);
}

static void _onGesture(GestureDir dir, int16_t startX, int16_t startY) {
    switch (dir) {
        case GESTURE_LEFT:
            if (startX > SCREEN_WIDTH - 40) {
                ControlPanel::show();
            }
            break;
        case GESTURE_RIGHT:
            ScreenManager::goBack();
            break;
        case GESTURE_UP:
            ScreenManager::navigate(SCREEN_NOTIFICATIONS, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 200);
            break;
        default:
            break;
    }
}

static void _buildList(void) {
    if (!_listContainer) return;

    // Clear existing children
    lv_obj_clean(_listContainer);

    uint8_t count = 0;
    const Task *tasks = TaskService::getTasks(count);

    if (count == 0) {
        lv_obj_t *msg = lv_label_create(_listContainer);
        lv_obj_add_style(msg, &style_label_body, 0);
        lv_obj_set_style_text_color(msg, COLOR_TEXT_SECONDARY, 0);
        lv_label_set_text(msg, "No tasks synced yet.\nConnect via BLE to sync.");
        lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_width(msg, lv_pct(100));
        lv_obj_set_style_pad_top(msg, SPACE_XL * 2, 0);
        return;
    }

    for (uint8_t i = 0; i < count; i++) {
        const Task &t = tasks[i];

        // Card for each task
        lv_obj_t *card = theme_create_card(_listContainer);
        lv_obj_set_width(card, lv_pct(100));
        lv_obj_set_height(card, 56);
        lv_obj_set_flex_flow(card, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_gap(card, SPACE_MD, 0);

        // Priority dot
        lv_obj_t *dot = lv_obj_create(card);
        lv_obj_remove_style_all(dot);
        lv_obj_set_size(dot, 10, 10);
        lv_obj_set_style_radius(dot, 5, 0);
        lv_obj_set_style_bg_color(dot, _priorityColor(t.priority), 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);

        // Title
        lv_obj_t *title = lv_label_create(card);
        lv_obj_add_style(title, &style_label_body, 0);
        lv_label_set_text(title, t.title);
        lv_label_set_long_mode(title, LV_LABEL_LONG_DOT);
        lv_obj_set_flex_grow(title, 1);
        if (t.completed) {
            lv_obj_set_style_text_decor(title, LV_TEXT_DECOR_STRIKETHROUGH, 0);
            lv_obj_set_style_text_color(title, COLOR_TEXT_DISABLED, 0);
        }

        // Duration badge
        lv_obj_t *badge = lv_label_create(card);
        lv_obj_add_style(badge, &style_label_caption, 0);
        lv_label_set_text_fmt(badge, "%dm", t.durationMin);

        // Tap handler
        lv_obj_add_event_cb(card, _onTaskTap, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    }
}

void TaskList::create(lv_obj_t *screen) {
    lv_obj_t *content = lv_obj_create(screen);
    lv_obj_remove_style_all(content);
    lv_obj_set_size(content, SCREEN_WIDTH, SCREEN_HEIGHT - STATUS_BAR_HEIGHT);
    lv_obj_set_pos(content, 0, STATUS_BAR_HEIGHT);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    // Header
    lv_obj_t *header = lv_label_create(content);
    lv_obj_add_style(header, &style_label_heading, 0);
    lv_label_set_text(header, "Tasks");
    lv_obj_align(header, LV_ALIGN_TOP_LEFT, SPACE_LG, SPACE_SM);

    // Scrollable list area
    _listContainer = lv_obj_create(content);
    lv_obj_remove_style_all(_listContainer);
    lv_obj_set_size(_listContainer, SCREEN_WIDTH - SPACE_LG * 2, SCREEN_HEIGHT - STATUS_BAR_HEIGHT - 44);
    lv_obj_align(_listContainer, LV_ALIGN_BOTTOM_MID, 0, -SPACE_SM);
    lv_obj_set_flex_flow(_listContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(_listContainer, SPACE_SM, 0);
    lv_obj_add_flag(_listContainer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(_listContainer, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(_listContainer, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_style_scrollbar_mode(_listContainer, LV_SCROLLBAR_MODE_AUTO, 0);

    _buildList();

    Gesture::setCallback(_onGesture);
    Gesture::attachToScreen(screen);
}

void TaskList::destroy(void) {
    _listContainer = NULL;
}
