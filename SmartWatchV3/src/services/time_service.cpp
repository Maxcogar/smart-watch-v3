/**
 * Time Service — Implementation
 */

#include "time_service.h"
#include <Arduino.h>

static const char *_dayNames[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday"
};
static const char *_monthNames[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

void TimeService::init(void) {
    // Set timezone (default UTC, user can configure later)
    setenv("TZ", "UTC0", 1);
    tzset();
}

bool TimeService::syncNTP(void) {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    // Wait up to 5 seconds for sync
    time_t now = 0;
    struct tm ti = {};
    unsigned long start = millis();
    while (ti.tm_year < (2025 - 1900) && millis() - start < 5000) {
        time(&now);
        localtime_r(&now, &ti);
        delay(100);
    }

    if (ti.tm_year >= (2025 - 1900)) {
        Serial.printf("NTP: synced to %04d-%02d-%02d %02d:%02d:%02d\n",
            ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday,
            ti.tm_hour, ti.tm_min, ti.tm_sec);
        return true;
    }

    Serial.println("NTP: sync failed");
    return false;
}

void TimeService::setTimeFromEpoch(uint32_t epoch) {
    struct timeval tv = { .tv_sec = (time_t)epoch, .tv_usec = 0 };
    settimeofday(&tv, NULL);
    Serial.printf("Time set from BLE: epoch %lu\n", (unsigned long)epoch);
}

time_t TimeService::getTime(void) {
    time_t now;
    time(&now);
    return now;
}

void TimeService::getFormattedTime(char *buf, size_t len) {
    time_t now;
    struct tm ti;
    time(&now);
    localtime_r(&now, &ti);
    snprintf(buf, len, "%02d:%02d", ti.tm_hour, ti.tm_min);
}

void TimeService::getFormattedDate(char *buf, size_t len) {
    time_t now;
    struct tm ti;
    time(&now);
    localtime_r(&now, &ti);
    snprintf(buf, len, "%s, %s %d",
        _dayNames[ti.tm_wday], _monthNames[ti.tm_mon], ti.tm_mday);
}

uint8_t TimeService::getHour(void) {
    time_t now;
    struct tm ti;
    time(&now);
    localtime_r(&now, &ti);
    return ti.tm_hour;
}

uint8_t TimeService::getMinute(void) {
    time_t now;
    struct tm ti;
    time(&now);
    localtime_r(&now, &ti);
    return ti.tm_min;
}
