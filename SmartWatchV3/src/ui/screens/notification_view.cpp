/**
 * Notification View — Implementation
 *
 * Displays notifications as cards with swipe-right-to-dismiss.
 * Shows "X of Y" counter and empty state.
 *
 * Layout (320x240):
 * [Status bar]
 * ["Notifications" | "X of Y" counter | Clear All]
 * [Scrollable notification cards]
 * [Empty state: "No notifications"]
 */

#include "notification_view.h"
#include "../theme.h"
#include "../gesture.h"
#include "../screen_manager.h"
#include "../../services/notification_service.h"

static lv_obj_t *_listContainer = NULL;
static lv_obj_t *_counterLabel = NULL;
static lv_obj_t *_emptyLabel = NULL;

static void _buildList(void);

// Swipe-to-dismiss tracking per card
struct SwipeState {
    uint8_t notifIndex;
    int16_t startX;
    bool tracking;
};

static void _onCardSwipe(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *card = (lv_obj_t *)lv_event_get_target(e);
    SwipeState *state = (SwipeState *)lv_event_get_user_data(e);
    if (!state) return;

    lv_indev_t *indev = lv_indev_get_act();
    if (!indev) return;
    lv_point_t point;
    lv_indev_get_point(indev, &point);

    if (code == LV_EVENT_PRESSED) {
        state->startX = point.x;
        state->tracking = true;
    }
    else if (code == LV_EVENT_PRESSING && state->tracking) {
        int16_t dx = point.x - state->startX;
        if (dx > 0) {
            // Move card right
            lv_obj_set_style_translate_x(card, dx, 0);
            // Fade out proportionally
            lv_opa_t opa = (dx > 200) ? LV_OPA_0 : (lv_opa_t)(LV_OPA_COVER - (dx * LV_OPA_COVER / 200));
            lv_obj_set_style_opa(card, opa, 0);
        }
    }
    else if (code == LV_EVENT_RELEASED && state->tracking) {
        state->tracking = false;
        int16_t dx = point.x - state->startX;

        if (dx > 120) {
            // Dismiss — animate off screen then remove
            lv_anim_t a;
            lv_anim_init(&a);
            lv_anim_set_var(&a, card);
            lv_anim_set_values(&a, dx, SCREEN_WIDTH);
            lv_anim_set_time(&a, 150);
            lv_anim_set_exec_cb(&a, [](void *obj, int32_t v) {
                lv_obj_set_style_translate_x((lv_obj_t *)obj, v, 0);
            });
            lv_anim_start(&a);

            // Dismiss after animation
            NotificationService::dismiss(state->notifIndex);

            // Rebuild list after short delay
            lv_timer_t *rebuild = lv_timer_create([](lv_timer_t *t) {
                _buildList();
                lv_timer_del(t);
            }, 200, NULL);
            (void)rebuild;
        } else {
            // Snap back
            lv_anim_t a;
            lv_anim_init(&a);
            lv_anim_set_var(&a, card);
            lv_anim_set_values(&a, dx > 0 ? dx : 0, 0);
            lv_anim_set_time(&a, 150);
            lv_anim_set_exec_cb(&a, [](void *obj, int32_t v) {
                lv_obj_set_style_translate_x((lv_obj_t *)obj, v, 0);
            });
            lv_anim_start(&a);
            lv_obj_set_style_opa(card, LV_OPA_COVER, 0);
        }
    }
}

// Static pool for swipe states (avoids dynamic alloc)
static SwipeState _swipeStates[MAX_NOTIFICATIONS];

static void _onClearAll(lv_event_t *e) {
    NotificationService::dismissAll();
    _buildList();
}

static void _onGesture(GestureDir dir, int16_t startX, int16_t startY) {
    if (dir == GESTURE_DOWN) {
        ScreenManager::goBack();
    }
}

static void _buildList(void) {
    if (!_listContainer) return;
    lv_obj_clean(_listContainer);

    uint8_t count = 0;
    const NotificationData *notifs = NotificationService::getNotifications(count);

    // Update counter
    if (_counterLabel) {
        if (count > 0) {
            lv_label_set_text_fmt(_counterLabel, "%d", count);
            lv_obj_clear_flag(_counterLabel, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(_counterLabel, LV_OBJ_FLAG_HIDDEN);
        }
    }

    // Empty state
    if (_emptyLabel) {
        if (count == 0) {
            lv_obj_clear_flag(_emptyLabel, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(_emptyLabel, LV_OBJ_FLAG_HIDDEN);
        }
    }

    for (uint8_t i = 0; i < count; i++) {
        const NotificationData &n = notifs[i];

        lv_obj_t *card = theme_create_card(_listContainer);
        lv_obj_set_width(card, lv_pct(100));
        lv_obj_set_height(card, LV_SIZE_CONTENT);
        lv_obj_set_style_min_height(card, 52, 0);

        // App name / sender
        lv_obj_t *sender = lv_label_create(card);
        lv_obj_add_style(sender, &style_label_caption, 0);
        lv_obj_set_style_text_color(sender, COLOR_ACCENT, 0);
        lv_label_set_text(sender, n.app);
        lv_obj_align(sender, LV_ALIGN_TOP_LEFT, 0, 0);

        // Title
        lv_obj_t *title = lv_label_create(card);
        lv_obj_add_style(title, &style_label_body, 0);
        lv_label_set_text(title, n.title);
        lv_label_set_long_mode(title, LV_LABEL_LONG_DOT);
        lv_obj_set_width(title, lv_pct(90));
        lv_obj_align_to(title, sender, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 2);

        // Message preview (50 chars)
        if (strlen(n.message) > 0) {
            lv_obj_t *msg = lv_label_create(card);
            lv_obj_add_style(msg, &style_label_caption, 0);
            lv_label_set_text(msg, n.message);
            lv_label_set_long_mode(msg, LV_LABEL_LONG_DOT);
            lv_obj_set_width(msg, lv_pct(90));
            lv_obj_align_to(msg, title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 2);
        }

        // Swipe-to-dismiss events
        _swipeStates[i].notifIndex = i;
        _swipeStates[i].tracking = false;
        lv_obj_add_event_cb(card, _onCardSwipe, LV_EVENT_PRESSED, &_swipeStates[i]);
        lv_obj_add_event_cb(card, _onCardSwipe, LV_EVENT_PRESSING, &_swipeStates[i]);
        lv_obj_add_event_cb(card, _onCardSwipe, LV_EVENT_RELEASED, &_swipeStates[i]);
    }
}

void NotificationView::create(lv_obj_t *screen) {
    lv_obj_t *content = lv_obj_create(screen);
    lv_obj_remove_style_all(content);
    lv_obj_set_size(content, SCREEN_WIDTH, SCREEN_HEIGHT - STATUS_BAR_HEIGHT);
    lv_obj_set_pos(content, 0, STATUS_BAR_HEIGHT);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    // Header row
    lv_obj_t *headerRow = lv_obj_create(content);
    lv_obj_remove_style_all(headerRow);
    lv_obj_set_size(headerRow, SCREEN_WIDTH - SPACE_LG * 2, 32);
    lv_obj_align(headerRow, LV_ALIGN_TOP_MID, 0, SPACE_XS);
    lv_obj_clear_flag(headerRow, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *header = lv_label_create(headerRow);
    lv_obj_add_style(header, &style_label_heading, 0);
    lv_label_set_text(header, "Notifications");
    lv_obj_align(header, LV_ALIGN_LEFT_MID, 0, 0);

    _counterLabel = lv_label_create(headerRow);
    lv_obj_add_style(_counterLabel, &style_label_caption, 0);
    lv_obj_set_style_text_color(_counterLabel, COLOR_ACCENT, 0);
    lv_obj_align(_counterLabel, LV_ALIGN_CENTER, 0, 0);

    // Clear all button
    lv_obj_t *clearBtn = lv_label_create(headerRow);
    lv_obj_add_style(clearBtn, &style_label_caption, 0);
    lv_obj_set_style_text_color(clearBtn, COLOR_ACCENT_RED, 0);
    lv_label_set_text(clearBtn, "Clear All");
    lv_obj_align(clearBtn, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_flag(clearBtn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(clearBtn, _onClearAll, LV_EVENT_CLICKED, NULL);

    // List container
    _listContainer = lv_obj_create(content);
    lv_obj_remove_style_all(_listContainer);
    lv_obj_set_size(_listContainer, SCREEN_WIDTH - SPACE_LG * 2, SCREEN_HEIGHT - STATUS_BAR_HEIGHT - 48);
    lv_obj_align(_listContainer, LV_ALIGN_BOTTOM_MID, 0, -SPACE_XS);
    lv_obj_set_flex_flow(_listContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(_listContainer, SPACE_SM, 0);
    lv_obj_add_flag(_listContainer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(_listContainer, LV_DIR_VER);

    // Empty state (shown when no notifications)
    _emptyLabel = lv_label_create(content);
    lv_obj_add_style(_emptyLabel, &style_label_body, 0);
    lv_obj_set_style_text_color(_emptyLabel, COLOR_TEXT_SECONDARY, 0);
    lv_label_set_text(_emptyLabel, "No notifications");
    lv_obj_set_style_text_align(_emptyLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(_emptyLabel, LV_ALIGN_CENTER, 0, 0);

    _buildList();

    Gesture::setCallback(_onGesture);
    Gesture::attachToScreen(screen);
}

void NotificationView::destroy(void) {
    _listContainer = NULL;
    _counterLabel = NULL;
    _emptyLabel = NULL;
}
