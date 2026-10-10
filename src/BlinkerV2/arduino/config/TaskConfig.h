#ifndef BLINKER_ARDUINO_TASK_CONFIG_H
#define BLINKER_ARDUINO_TASK_CONFIG_H

// Maintainer admission, NOT per-user task definitions or credentials. A tested
// candidate supplies capacity and all four storage policy values library-wide.
// Stock source is closed; enabling a Sketch cannot invent production defaults.
#ifndef BLINKER_TASK_CAPACITY
#define BLINKER_TASK_CAPACITY 0
#endif

#if defined(BLINKER_ENABLE_TASKS) && BLINKER_ENABLE_TASKS
#if BLINKER_TASK_CAPACITY < 1 || BLINKER_TASK_CAPACITY > 10
#error "Tasks require a maintainer-admitted BLINKER_TASK_CAPACITY (1..10)"
#endif
#if !defined(BLINKER_TASK_WINDOW_MS) || !defined(BLINKER_TASK_WRITES) || \
    !defined(BLINKER_TASK_MIN_START_MS) || !defined(BLINKER_TASK_MAX_COMMIT_MS)
#error "Tasks require an explicit maintainer storage policy; no production defaults"
#endif
#if BLINKER_TASK_WRITES < 2 || BLINKER_TASK_WRITES > 65535 || \
    BLINKER_TASK_WINDOW_MS < 1 || BLINKER_TASK_WINDOW_MS > 0x7FFFFFFF || \
    BLINKER_TASK_MIN_START_MS < 1 || BLINKER_TASK_MIN_START_MS > 0x7FFFFFFF || \
    BLINKER_TASK_MAX_COMMIT_MS < 1 || BLINKER_TASK_MAX_COMMIT_MS > 0x7FFFFFFF
#error "Task storage policy values are out of range"
#endif
#endif

#endif
