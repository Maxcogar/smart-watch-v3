/**
 * Task List Screen — Placeholder
 * Full implementation in Phase 2.
 */

#include "task_list.h"
#include "../theme.h"
#include "../gesture.h"
#include "../screen_manager.h"
#include "../widgets/status_bar.h"

static void _onGesture(GestureDir dir, int16_t startX, int16_t startY) {
    switch (dir) {
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
    lv_obj_align(header, LV_ALIGN_TOP_LEFT, SPACE_LG, SPACE_MD);

    // Placeholder message
    lv_obj_t *msg = lv_label_create(content);
    lv_obj_add_style(msg, &style_label_body, 0);
    lv_obj_set_style_text_color(msg, COLOR_TEXT_SECONDARY, 0);
    lv_label_set_text(msg, "No tasks synced yet.\nConnect via BLE to sync.");
    lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(msg, LV_ALIGN_CENTER, 0, 0);

    Gesture::setCallback(_onGesture);
    Gesture::attachToScreen(screen);
}

void TaskList::destroy(void) {}
