/**
 * Control Panel
 *
 * Slide-in overlay from right edge. Contains brightness slider,
 * WiFi/BLE toggles, compressor control, and battery detail.
 * Triggered by right-edge swipe gesture.
 */

#ifndef CONTROL_PANEL_H
#define CONTROL_PANEL_H

#include <lvgl.h>

namespace ControlPanel {
    /** Show the control panel (slides in from right). */
    void show(void);

    /** Hide the control panel. */
    void hide(void);

    /** Is the panel currently visible? */
    bool isVisible(void);
}

#endif
