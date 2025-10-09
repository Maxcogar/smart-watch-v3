/**
 * Minimal smartwatch apps built on esp-brookesia.
 */

#ifndef WATCH_APPS_H
#define WATCH_APPS_H

#include <ctime>
#include <lvgl.h>
#include <esp_brookesia.hpp>
#include "hardware_config.h"
#include "notification_app.h"

class WatchFaceApp : public ESP_Brookesia_PhoneApp {
public:
    WatchFaceApp()
    : ESP_Brookesia_PhoneApp("Watch", nullptr, true, true, true) {}

    bool run() override
    {
        screen = lv_scr_act();

        time_label = lv_label_create(screen);
        lv_obj_center(time_label);

        date_label = lv_label_create(screen);
        lv_obj_align(date_label, LV_ALIGN_TOP_MID, 0, 20);

        battery_label = lv_label_create(screen);
        lv_obj_align(battery_label, LV_ALIGN_BOTTOM_MID, 0, -20);

        refresh();
        update_timer = lv_timer_create(timerCallback, 60000, this);
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
        auto *app = static_cast<WatchFaceApp *>(timer->user_data);
        if (app) {
            app->refresh();
        }
    }

    void refresh()
    {
        time_t now = ::time(nullptr);
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);

        char buffer[32];
        strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
        lv_label_set_text(time_label, buffer);

        strftime(buffer, sizeof(buffer), "%a, %b %d", &timeinfo);
        lv_label_set_text(date_label, buffer);

        const uint8_t percent = getBatteryPercentage();
        snprintf(buffer, sizeof(buffer), LV_SYMBOL_BATTERY_FULL " %d%%", percent);
        lv_label_set_text(battery_label, buffer);
    }

    lv_obj_t *screen = nullptr;
    lv_obj_t *time_label = nullptr;
    lv_obj_t *date_label = nullptr;
    lv_obj_t *battery_label = nullptr;
    lv_timer_t *update_timer = nullptr;
};

inline void installWatchApps(ESP_Brookesia_Phone *phone)
{
    if (!phone) {
        return;
    }

    phone->installApp(new WatchFaceApp());
    phone->installApp(new NotificationApp());
}

#endif // WATCH_APPS_H
