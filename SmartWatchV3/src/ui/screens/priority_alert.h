/**
 * Priority Alert Screen
 *
 * Full-screen red overlay that overrides all UI states.
 * Rendered on lv_layer_top(), bypasses screen navigation.
 * Touch anywhere to acknowledge and dismiss.
 */

#ifndef PRIORITY_ALERT_H
#define PRIORITY_ALERT_H

#include <lvgl.h>

namespace PriorityAlert {
    /** Show the priority alert overlay. */
    void show(const char *message);

    /** Dismiss the alert (also called by touch). */
    void dismiss(void);

    /** Is the alert currently showing? */
    bool isActive(void);
}

#endif
