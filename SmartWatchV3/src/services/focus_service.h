/**
 * Focus Service
 *
 * Manages focus timer, focus shield, and session tracking.
 * Timer uses esp_timer_get_time() for microsecond accuracy (no drift).
 */

#ifndef FOCUS_SERVICE_H
#define FOCUS_SERVICE_H

#include <stdint.h>

enum FocusState {
    FOCUS_IDLE = 0,
    FOCUS_ACTIVE,
    FOCUS_PAUSED,
    FOCUS_COMPLETE
};

namespace FocusService {
    void init(void);

    /** Start a focus timer for the given duration. */
    void startTimer(uint16_t minutes);

    /** Pause the running timer. */
    void pauseTimer(void);

    /** Resume from paused state. */
    void resumeTimer(void);

    /** Stop and reset the timer. */
    void stopTimer(void);

    /** Get current focus state. */
    FocusState getState(void);

    /** Get remaining seconds (0 if idle/complete). */
    uint32_t getRemainingSeconds(void);

    /** Get total duration in seconds. */
    uint32_t getTotalSeconds(void);

    /** Get elapsed seconds. */
    uint32_t getElapsedSeconds(void);

    /** Is the Focus Shield active? (true when FOCUS_ACTIVE) */
    bool isFocusShieldActive(void);

    /** Call periodically to check for timer completion. */
    void update(void);
}

#endif
