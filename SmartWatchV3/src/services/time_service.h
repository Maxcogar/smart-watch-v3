/**
 * Time Service
 *
 * Manages time synchronization via NTP (when WiFi available)
 * or BLE (phone sends epoch). Provides formatted time/date strings.
 */

#ifndef TIME_SERVICE_H
#define TIME_SERVICE_H

#include <stdint.h>
#include <time.h>

namespace TimeService {
    void init(void);

    /** Sync time via NTP. Requires WiFi connection. */
    bool syncNTP(void);

    /** Set time from BLE companion (epoch seconds). */
    void setTimeFromEpoch(uint32_t epoch);

    /** Get current time. */
    time_t getTime(void);

    /** Format time as "HH:MM". */
    void getFormattedTime(char *buf, size_t len);

    /** Format date as "Wednesday, Mar 17". */
    void getFormattedDate(char *buf, size_t len);

    /** Get hour (0-23). */
    uint8_t getHour(void);

    /** Get minute (0-59). */
    uint8_t getMinute(void);
}

#endif
