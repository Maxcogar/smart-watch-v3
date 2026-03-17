/**
 * Active Task Screen
 *
 * Shows the selected task with a focus timer featuring an arc progress ring.
 * Activates the Focus Shield when the timer is running.
 */

#ifndef ACTIVE_TASK_H
#define ACTIVE_TASK_H

#include <lvgl.h>

namespace ActiveTask {
    void create(lv_obj_t *screen);
    void destroy(void);
}

#endif
