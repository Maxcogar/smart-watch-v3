/**
 * Gesture Detection — Implementation
 *
 * Tracks touch press/release coordinates and timing to detect swipes.
 * Swipe: >40px movement, <400ms, dominant axis >2x minor axis.
 * Right-edge: startX > 280 for control panel trigger.
 */

#include "gesture.h"
#include "theme.h"
#include "../services/power_service.h"

static GestureCallback _callback = NULL;
static bool _tracking = false;
static int16_t _startX = 0;
static int16_t _startY = 0;
static uint32_t _startTime = 0;

// Swipe thresholds
#define SWIPE_MIN_DISTANCE   40
#define SWIPE_MAX_TIME_MS    400
#define SWIPE_AXIS_RATIO     2

static void _gestureEvent(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev = lv_indev_get_act();
    if (!indev) return;

    lv_point_t point;
    lv_indev_get_point(indev, &point);

    if (code == LV_EVENT_PRESSED) {
        _tracking = true;
        _startX = point.x;
        _startY = point.y;
        _startTime = lv_tick_get();
        // Reset power idle timer on any touch
        PowerService::resetIdleTimer();
    }
    else if (code == LV_EVENT_RELEASED && _tracking) {
        _tracking = false;

        uint32_t elapsed = lv_tick_elaps(_startTime);
        if (elapsed > SWIPE_MAX_TIME_MS) return;

        int16_t dx = point.x - _startX;
        int16_t dy = point.y - _startY;
        int16_t absDx = dx < 0 ? -dx : dx;
        int16_t absDy = dy < 0 ? -dy : dy;

        if (absDx < SWIPE_MIN_DISTANCE && absDy < SWIPE_MIN_DISTANCE) return;

        GestureDir dir = GESTURE_NONE;

        if (absDx > absDy * SWIPE_AXIS_RATIO) {
            // Horizontal swipe
            dir = (dx < 0) ? GESTURE_LEFT : GESTURE_RIGHT;
        } else if (absDy > absDx * SWIPE_AXIS_RATIO) {
            // Vertical swipe
            dir = (dy < 0) ? GESTURE_UP : GESTURE_DOWN;
        }

        if (dir != GESTURE_NONE && _callback) {
            _callback(dir, _startX, _startY);
        }
    }
}

void Gesture::attachToScreen(lv_obj_t *screen) {
    lv_obj_add_event_cb(screen, _gestureEvent, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(screen, _gestureEvent, LV_EVENT_RELEASED, NULL);
}

void Gesture::setCallback(GestureCallback cb) {
    _callback = cb;
}
