#ifndef BLINKER_TEST_TASK_PILOT_POLICY_H
#define BLINKER_TEST_TASK_PILOT_POLICY_H

// Explicit, closed T2 lab workload; NOT a production endurance guarantee.
// Cold bucket: one write credit per 6s, max 10; no free credits on reboot.
// A full 4-task due burst needs 8 writes. Do not flood edits while it is due.
#define BLINKER_TASK_CAPACITY 4U
#define BLINKER_TASK_WINDOW_MS 60000U
#define BLINKER_TASK_WRITES 10U
#define BLINKER_TASK_MIN_START_MS 30000U
#define BLINKER_TASK_MAX_COMMIT_MS 100U

#endif
