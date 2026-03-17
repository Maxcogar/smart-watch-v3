/**
 * Storage Service — Implementation
 */

#include "storage_service.h"
#include <Preferences.h>
#include <Arduino.h>

static Preferences _prefs;

void StorageService::init(void) {
    _prefs.begin("smartwatch", false);
    Serial.printf("StorageService: %d tasks stored\n", getTaskCount());
}

bool StorageService::saveTask(uint8_t index, const Task &task) {
    if (index >= MAX_TASKS) return false;
    char key[12];
    snprintf(key, sizeof(key), "task_%02d", index);
    return _prefs.putBytes(key, &task, sizeof(Task)) == sizeof(Task);
}

bool StorageService::loadTask(uint8_t index, Task &task) {
    if (index >= MAX_TASKS) return false;
    char key[12];
    snprintf(key, sizeof(key), "task_%02d", index);
    return _prefs.getBytes(key, &task, sizeof(Task)) == sizeof(Task);
}

uint8_t StorageService::getTaskCount(void) {
    return _prefs.getUChar("task_count", 0);
}

void StorageService::setTaskCount(uint8_t count) {
    _prefs.putUChar("task_count", count);
}

void StorageService::saveSettings(const WatchSettings &settings) {
    _prefs.putBytes("settings", &settings, sizeof(WatchSettings));
}

bool StorageService::loadSettings(WatchSettings &settings) {
    return _prefs.getBytes("settings", &settings, sizeof(WatchSettings)) == sizeof(WatchSettings);
}
