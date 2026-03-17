/**
 * Notification Service — Implementation
 */

#include "notification_service.h"
#include "focus_service.h"
#include <Arduino.h>
#include <string.h>

static NotificationData _notifications[MAX_NOTIFICATIONS];
static uint8_t _count = 0;
static bool _focusShieldActive = false;

// Separate queue for notifications received during focus
static NotificationData _focusQueue[MAX_NOTIFICATIONS];
static uint8_t _focusQueueCount = 0;

void NotificationService::init(void) {
    _count = 0;
    _focusQueueCount = 0;
    _focusShieldActive = false;
}

bool NotificationService::processIncoming(const NotificationData &notif) {
    // Priority alerts always bypass
    if (isPriorityAlert(notif)) {
        // Add to main list AND signal priority
        if (_count < MAX_NOTIFICATIONS) {
            _notifications[_count++] = notif;
        }
        return true;  // caller should trigger priority alert UI
    }

    // If Focus Shield is active, queue silently
    if (_focusShieldActive || FocusService::isFocusShieldActive()) {
        if (_focusQueueCount < MAX_NOTIFICATIONS) {
            _focusQueue[_focusQueueCount++] = notif;
        } else {
            // Drop oldest, shift left
            memmove(&_focusQueue[0], &_focusQueue[1],
                    sizeof(NotificationData) * (MAX_NOTIFICATIONS - 1));
            _focusQueue[MAX_NOTIFICATIONS - 1] = notif;
            Serial.println("NotificationService: focus queue overflow, dropped oldest");
        }
        Serial.printf("NotificationService: queued during focus (%d in queue)\n", _focusQueueCount);
        return false;
    }

    // Normal: add to main list
    if (_count < MAX_NOTIFICATIONS) {
        _notifications[_count++] = notif;
    } else {
        // Drop oldest
        memmove(&_notifications[0], &_notifications[1],
                sizeof(NotificationData) * (MAX_NOTIFICATIONS - 1));
        _notifications[MAX_NOTIFICATIONS - 1] = notif;
    }
    return false;
}

const NotificationData *NotificationService::getNotifications(uint8_t &count) {
    count = _count;
    return _notifications;
}

void NotificationService::dismiss(uint8_t index) {
    if (index >= _count) return;
    // Shift remaining
    for (uint8_t i = index; i < _count - 1; i++) {
        _notifications[i] = _notifications[i + 1];
    }
    _count--;
}

void NotificationService::dismissAll(void) {
    _count = 0;
}

uint8_t NotificationService::getUnreadCount(void) {
    uint8_t unread = 0;
    for (uint8_t i = 0; i < _count; i++) {
        if (!_notifications[i].isRead) unread++;
    }
    return unread;
}

void NotificationService::setFocusShieldActive(bool active) {
    _focusShieldActive = active;
    if (!active) {
        flushQueued();
    }
}

void NotificationService::flushQueued(void) {
    for (uint8_t i = 0; i < _focusQueueCount; i++) {
        if (_count < MAX_NOTIFICATIONS) {
            _notifications[_count++] = _focusQueue[i];
        }
    }
    if (_focusQueueCount > 0) {
        Serial.printf("NotificationService: flushed %d queued notifications\n", _focusQueueCount);
    }
    _focusQueueCount = 0;
}

bool NotificationService::isPriorityAlert(const NotificationData &notif) {
    // Category 4 = alarm/priority, or app name contains "priority"
    return notif.category == 4;
}
