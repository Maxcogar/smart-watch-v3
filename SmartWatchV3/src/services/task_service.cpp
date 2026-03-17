/**
 * Task Service — Implementation
 */

#include "task_service.h"
#include "storage_service.h"
#include <Arduino.h>
#include <ArduinoJson.h>

static Task _tasks[MAX_TASKS];
static uint8_t _taskCount = 0;
static int8_t _activeIndex = -1;

void TaskService::init(void) {
    _taskCount = StorageService::getTaskCount();
    if (_taskCount > MAX_TASKS) _taskCount = MAX_TASKS;

    for (uint8_t i = 0; i < _taskCount; i++) {
        StorageService::loadTask(i, _tasks[i]);
    }
    _activeIndex = -1;
    Serial.printf("TaskService: loaded %d tasks\n", _taskCount);
}

const Task *TaskService::getTasks(uint8_t &count) {
    count = _taskCount;
    return _tasks;
}

int8_t TaskService::addTask(const Task &task) {
    if (_taskCount >= MAX_TASKS) return -1;
    _tasks[_taskCount] = task;
    StorageService::saveTask(_taskCount, task);
    _taskCount++;
    StorageService::setTaskCount(_taskCount);
    return _taskCount - 1;
}

bool TaskService::removeTask(uint8_t index) {
    if (index >= _taskCount) return false;

    // Shift remaining tasks
    for (uint8_t i = index; i < _taskCount - 1; i++) {
        _tasks[i] = _tasks[i + 1];
        StorageService::saveTask(i, _tasks[i]);
    }
    _taskCount--;
    StorageService::setTaskCount(_taskCount);

    // Adjust active index
    if (_activeIndex == index) _activeIndex = -1;
    else if (_activeIndex > index) _activeIndex--;

    return true;
}

bool TaskService::completeTask(uint8_t index) {
    if (index >= _taskCount) return false;
    _tasks[index].completed = true;
    StorageService::saveTask(index, _tasks[index]);
    return true;
}

void TaskService::setActiveTask(int8_t index) {
    if (index >= 0 && index < _taskCount) {
        _activeIndex = index;
    } else {
        _activeIndex = -1;
    }
}

const Task *TaskService::getActiveTask(void) {
    if (_activeIndex < 0 || _activeIndex >= _taskCount) return nullptr;
    return &_tasks[_activeIndex];
}

int8_t TaskService::getActiveTaskIndex(void) {
    return _activeIndex;
}

void TaskService::syncFromJSON(const char *jsonPayload, size_t len) {
    StaticJsonDocument<2048> doc;
    DeserializationError err = deserializeJson(doc, jsonPayload, len);
    if (err) {
        Serial.printf("Task sync JSON error: %s\n", err.c_str());
        return;
    }

    JsonArray tasks = doc["tasks"].as<JsonArray>();
    _taskCount = 0;

    for (JsonObject t : tasks) {
        if (_taskCount >= MAX_TASKS) break;

        Task &task = _tasks[_taskCount];
        strlcpy(task.title, t["title"] | "Untitled", sizeof(task.title));
        task.priority = t["priority"] | 3;
        task.durationMin = t["duration"] | 25;
        task.completed = t["completed"] | false;
        task.createdAt = t["created"] | 0;

        StorageService::saveTask(_taskCount, task);
        _taskCount++;
    }

    StorageService::setTaskCount(_taskCount);
    _activeIndex = -1;
    Serial.printf("TaskService: synced %d tasks\n", _taskCount);
}

void TaskService::save(void) {
    for (uint8_t i = 0; i < _taskCount; i++) {
        StorageService::saveTask(i, _tasks[i]);
    }
    StorageService::setTaskCount(_taskCount);
}
