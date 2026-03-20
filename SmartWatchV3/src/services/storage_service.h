/**
 * Storage Service
 *
 * Persistent storage via ESP32 Preferences (NVS).
 * Stores tasks, settings, and cached state.
 */

#ifndef STORAGE_SERVICE_H
#define STORAGE_SERVICE_H

#include <stdint.h>

// Task data structure
struct Task {
    char id[32];            // unique id for sync with Microsoft ToDo
    char title[48];
    uint8_t priority;       // 1=high, 2=medium, 3=low
    uint16_t durationMin;   // estimated focus time in minutes
    bool completed;
    uint32_t createdAt;     // epoch timestamp
};

// Settings
struct WatchSettings {
    uint8_t brightness;     // 0-255
    uint16_t screenTimeout; // seconds
    char wifiSSID[33];
    char wifiPass[65];
    char compressorIP1[16];
    char compressorIP2[16];
};

#define MAX_TASKS 20

namespace StorageService {
    void init(void);

    // Tasks
    bool saveTask(uint8_t index, const Task &task);
    bool loadTask(uint8_t index, Task &task);
    uint8_t getTaskCount(void);
    void setTaskCount(uint8_t count);

    // Settings
    void saveSettings(const WatchSettings &settings);
    bool loadSettings(WatchSettings &settings);
}

#endif
