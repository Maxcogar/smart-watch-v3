/**
 * Notification View — Placeholder
 * Full implementation in Phase 3.
 */

#include "notification_view.h"
#include "../theme.h"
#include "../gesture.h"
#include "../screen_manager.h"
#include "../widgets/status_bar.h"

static void _onGesture(GestureDir dir, int16_t startX, int16_t startY) {
    switch (dir) {
        case GESTURE_DOWN:
            ScreenManager::goBack();
            break;
        default:
            break;
    }
}

void NotificationView::create(lv_obj_t *screen) {
    lv_obj_t *content = lv_obj_create(screen);
    lv_obj_remove_style_all(content);
    lv_obj_set_size(content, SCREEN_WIDTH, SCREEN_HEIGHT - STATUS_BAR_HEIGHT);
    lv_obj_set_pos(content, 0, STATUS_BAR_HEIGHT);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    // Header
    lv_obj_t *header = lv_label_create(content);
    lv_obj_add_style(header, &style_label_heading, 0);
    lv_label_set_text(header, "Notifications");
    lv_obj_align(header, LV_ALIGN_TOP_LEFT, SPACE_LG, SPACE_MD);

    // Placeholder
    lv_obj_t *msg = lv_label_create(content);
    lv_obj_add_style(msg, &style_label_body, 0);
    lv_obj_set_style_text_color(msg, COLOR_TEXT_SECONDARY, 0);
    lv_label_set_text(msg, "No notifications");
    lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(msg, LV_ALIGN_CENTER, 0, 0);

    // Hint
    lv_obj_t *hint = lv_label_create(content);
    lv_obj_add_style(hint, &style_label_caption, 0);
    lv_label_set_text(hint, LV_SYMBOL_DOWN " Swipe down to go back");
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -SPACE_LG);

    Gesture::setCallback(_onGesture);
    Gesture::attachToScreen(screen);
}

void NotificationView::destroy(void) {}
