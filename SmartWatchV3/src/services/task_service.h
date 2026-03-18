/**
 * Task Service
 *
 * Manages the task list, active task selection, and sync with companion app.
 */

#ifndef TASK_SERVICE_H
#define TASK_SERVICE_H

#include <stddef.h>
#include "storage_service.h"

namespace TaskService {
    void init(void);

    /** Get the task array and count. */
    const Task *getTasks(uint8_t &count);

    /** Add a task. Returns index or -1 on failure. */
    int8_t addTask(const Task &task);

    /** Remove a task by index. */
    bool removeTask(uint8_t index);

    /** Mark a task as complete. */
    bool completeTask(uint8_t index);

    /** Set the active task for focus mode. */
    void setActiveTask(int8_t index);

    /** Get the active task, or NULL if none. */
    const Task *getActiveTask(void);

    /** Get the active task index, or -1 if none. */
    int8_t getActiveTaskIndex(void);

    /** Sync tasks from JSON payload (received via BLE from companion app). */
    void syncFromJSON(const char *jsonPayload, size_t len);

    /** Persist current state to NVS. */
    void save(void);
}

#endif
