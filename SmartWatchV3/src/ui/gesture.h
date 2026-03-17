/**
 * Gesture Detection
 *
 * Software gesture recognition from CST816S single-point touch data.
 * Detects swipe direction, edge swipes, and taps.
 */

#ifndef GESTURE_H
#define GESTURE_H

#include <lvgl.h>

enum GestureDir {
    GESTURE_NONE = 0,
    GESTURE_LEFT,
    GESTURE_RIGHT,
    GESTURE_UP,
    GESTURE_DOWN
};

typedef void (*GestureCallback)(GestureDir dir, int16_t startX, int16_t startY);

namespace Gesture {
    /**
     * Initialize gesture detection.
     * Installs an event handler on the given screen object.
     * Call this each time a new screen is created.
     */
    void attachToScreen(lv_obj_t *screen);

    /** Set the callback for detected gestures. */
    void setCallback(GestureCallback cb);
}

#endif // GESTURE_H
