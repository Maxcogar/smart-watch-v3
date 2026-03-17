/**
 * Power Service
 *
 * Manages screen timeout, brightness dimming, light sleep,
 * and wake-on-touch. Respects focus mode (no timeout during focus).
 */

#ifndef POWER_SERVICE_H
#define POWER_SERVICE_H

#include <stdint.h>

namespace PowerService {
    void init(void);

    /** Call every loop iteration to track idle time. */
    void update(void);

    /** Reset the idle timer (called on touch events). */
    void resetIdleTimer(void);

    /** Set screen timeout in seconds. 0 = never timeout. */
    void setScreenTimeout(uint16_t seconds);

    /** Set whether focus mode is active (disables timeout). */
    void setFocusMode(bool active);

    /** Get current brightness level (0-255). */
    uint8_t getCurrentBrightness(void);

    /** Set the active brightness (used when awake). */
    void setActiveBrightness(uint8_t brightness);

    /** Is the display currently dimmed? */
    bool isDimmed(void);
}

#endif
