/**
 * Watch Face Screen
 *
 * Home screen showing large time, date, and preview cards for
 * next task and timer status.
 */

#ifndef WATCH_FACE_H
#define WATCH_FACE_H

#include <lvgl.h>

namespace WatchFace {
    /** Create the watch face content on the given screen. */
    void create(lv_obj_t *screen);

    /** Cleanup when navigating away. */
    void destroy(void);
}

#endif // WATCH_FACE_H
