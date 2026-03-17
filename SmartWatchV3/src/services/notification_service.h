/**
 * Notification Service
 *
 * Manages notification queue, Focus Shield integration, and priority alerts.
 * Circular buffer of 20 items, FIFO display, oldest dropped at capacity.
 */

#ifndef NOTIFICATION_SERVICE_H
#define NOTIFICATION_SERVICE_H

#include "../ble_notifications.h"
#include <stdint.h>

#define MAX_NOTIFICATIONS 20

namespace NotificationService {
    void init(void);

    /**
     * Process an incoming notification.
     * If Focus Shield is active, silently queues it.
     * If it's a priority alert, returns true to signal override.
     */
    bool processIncoming(const NotificationData &notif);

    /** Get all notifications. */
    const NotificationData *getNotifications(uint8_t &count);

    /** Dismiss a notification by index. */
    void dismiss(uint8_t index);

    /** Dismiss all notifications. */
    void dismissAll(void);

    /** Get count of unread notifications. */
    uint8_t getUnreadCount(void);

    /** Set Focus Shield active state. */
    void setFocusShieldActive(bool active);

    /** Flush queued notifications (called when focus ends). */
    void flushQueued(void);

    /** Check if a notification is a priority alert. */
    bool isPriorityAlert(const NotificationData &notif);
}

#endif
