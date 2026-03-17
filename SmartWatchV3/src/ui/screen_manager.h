/**
 * Screen Manager
 *
 * Replaces esp-brookesia's app/screen management with a simple state machine.
 * Handles navigation, transitions, back-stack, and overlays.
 */

#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

#include <lvgl.h>

enum ScreenID {
    SCREEN_WATCH_FACE = 0,
    SCREEN_TASK_LIST,
    SCREEN_ACTIVE_TASK,
    SCREEN_NOTIFICATIONS,
    SCREEN_COUNT,
    // Overlays (not in main navigation)
    SCREEN_CONTROL_PANEL,
    SCREEN_PRIORITY_ALERT
};

// Screen lifecycle callbacks
typedef void (*ScreenCreateFn)(lv_obj_t *screen);
typedef void (*ScreenDestroyFn)(void);

struct ScreenDef {
    ScreenCreateFn create;
    ScreenDestroyFn destroy;
    const char *name;
};

namespace ScreenManager {
    /** Initialize the screen manager. Call after theme_init(). */
    void init(void);

    /** Register a screen's lifecycle callbacks. */
    void registerScreen(ScreenID id, ScreenCreateFn create, ScreenDestroyFn destroy, const char *name);

    /** Navigate to a screen with animation. */
    void navigate(ScreenID target,
                  lv_scr_load_anim_t anim = LV_SCR_LOAD_ANIM_MOVE_LEFT,
                  uint32_t duration = 200);

    /** Go back to the previous screen. */
    void goBack(void);

    /** Get the currently active screen ID. */
    ScreenID currentScreen(void);

    /** Get the LVGL screen object for the current screen. */
    lv_obj_t *currentScreenObj(void);

    /** Check if an overlay is currently active. */
    bool isOverlayActive(void);
}

#endif // SCREEN_MANAGER_H
