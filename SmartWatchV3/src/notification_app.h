/**
 * Notification App
 * 
 * Displays notifications received from the phone.
 */

#ifndef NOTIFICATION_APP_H
#define NOTIFICATION_APP_H

#include <lvgl.h>
#include <esp_brookesia.hpp>
#include "ble_notifications.h"

class NotificationApp : public ESP_Brookesia_PhoneApp {
public:
    NotificationApp()
    : ESP_Brookesia_PhoneApp("Notifications", nullptr, true, true, true) {}

    bool run() override
    {
        screen = lv_obj_create(nullptr);
        lv_obj_set_style_bg_color(screen, lv_color_black(), 0);

        notification_list = lv_list_create(screen);
        lv_obj_set_size(notification_list, TFT_WIDTH, TFT_HEIGHT);
        lv_obj_center(notification_list);

        lv_obj_set_style_bg_color(notification_list, lv_color_black(), 0);
        lv_obj_set_style_border_width(notification_list, 0, 0);

        update_timer = lv_timer_create(timerCallback, 500, this);

        lv_scr_load(screen);

        return true;
    }

    bool back() override
    {
        notifyCoreClosed();
        return true;
    }

    bool close() override
    {
        if (update_timer) {
            lv_timer_del(update_timer);
            update_timer = nullptr;
        }
        return true;
    }

private:
    static void timerCallback(lv_timer_t *timer)
    {
        auto *app = static_cast<NotificationApp *>(timer->user_data);
        if (app) {
            app->refresh();
        }
    }

    void refresh()
    {
        NotificationData notif;
        if (xQueueReceive(notificationQueue, &notif, 0) == pdPASS) {
            char title[128];
            snprintf(title, sizeof(title), "%s - %s", notif.app, notif.title);

            lv_obj_t *btn = lv_list_add_btn(notification_list, NULL, title);
            lv_obj_add_event_cb(btn, event_handler, LV_EVENT_CLICKED, this);

            lv_obj_t *label = lv_obj_get_child(btn, 0); // Get the label child of the button
            lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
            lv_obj_set_width(label, TFT_WIDTH - 40);

            lv_list_add_text(notification_list, notif.message);
        }
    }

    static void event_handler(lv_event_t * e)
    {
        lv_event_code_t code = lv_event_get_code(e);
        NotificationApp* app = (NotificationApp*)lv_event_get_user_data(e);

        if(code == LV_EVENT_CLICKED) {
            // Handle click event
        }
    }

    lv_obj_t *screen = nullptr;
    lv_obj_t *notification_list = nullptr;
    lv_timer_t *update_timer = nullptr;
};

#endif // NOTIFICATION_APP_H