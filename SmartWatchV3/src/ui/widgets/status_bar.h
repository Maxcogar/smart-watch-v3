/**
 * Status Bar Widget
 *
 * Persistent bar rendered on lv_layer_top() so it survives screen transitions.
 * Shows time, battery %, BLE status, and Focus Shield indicator.
 */

#ifndef STATUS_BAR_H
#define STATUS_BAR_H

#include <lvgl.h>

namespace StatusBar {
    /** Create the status bar on lv_layer_top(). Call once after theme_init(). */
    void init(void);

    /** Update the displayed time. */
    void setTime(uint8_t hour, uint8_t minute);

    /** Update battery percentage. */
    void setBattery(uint8_t percent);

    /** Update BLE connection status. */
    void setBLEConnected(bool connected);

    /** Show/hide the Focus Shield icon. */
    void setFocusShield(bool active);

    /** Show/hide the entire status bar. */
    void setVisible(bool visible);
}

#endif // STATUS_BAR_H
