/**
 * @file TaskSchedulerDeclarations.h
 * @brief Cooperative multitasking library for Arduino microcontrollers
 * @author Anatoli Arkhipenko
 * @version 4.0.8
 * @date 2015-2026
 * @copyright Copyright (c) 2015-2026 Anatoli Arkhipenko
 *
 * @details A lightweight implementation of cooperative multitasking (task scheduling) supporting:
 * - Periodic task execution, with dynamic execution period in milliseconds (default) or microseconds
 * - Number of iterations (limited or infinite number of iterations)
 * - Execution of tasks in the predefined sequence
 * - Dynamic change of task execution parameters (frequency, number of iterations, callback methods)
 * - Power saving via entering IDLE sleep mode when tasks are not scheduled to run
 * - Support for event-driven task invocation via Status Request object
 * - Support for task IDs and Control Points for error handling and watchdog timer
 * - Support for Local Task Storage pointer (allowing the use of same callback code for multiple tasks)
 * - Support for layered task prioritization
 * - Support for std::functions
 * - Overall task timeout
 * - Static and dynamic callback method binding
 *
 * Scheduling overhead: between 15 and 18 microseconds per scheduling pass (Arduino UNO rev 3 @ 16MHz clock, single scheduler w/o prioritization)
 */

#ifndef _TASKSCHEDULERDECLARATIONS_H_
#define _TASKSCHEDULERDECLARATIONS_H_

// ----------------------------------------
/**
 * @defgroup CompileTimeOptions Compile Time Options
 * @brief Available compile-time configuration options
 *
 * The following defines control library functionality at compile time,
 * and should be used in the main sketch depending on the functionality required.
 * To enable any option, uncomment the corresponding #define in your code.
 *
 * @par _TASK_TIMECRITICAL
 * Enable monitoring scheduling overruns.
 * Enables tracking when the current execution took place relative to when it was scheduled.
 * Provides getStartDelay() and getOverrun() methods for performance monitoring.
 *
 * @par _TASK_SLEEP_ON_IDLE_RUN
 * Enable 1 ms SLEEP_IDLE powerdowns between runs if no callback methods were invoked during the pass.
 * When enabled, the scheduler will place the processor into IDLE sleep mode for approximately 1 ms
 * after what is determined to be an "idle" pass. AVR boards only.
 *
 * @par _TASK_STATUS_REQUEST
 * Compile with support for StatusRequest functionality.
 * Enables triggering tasks on status change events in addition to time-based scheduling.
 * Allows tasks to wait on an event and signal event completion to each other.
 *
 * @par _TASK_WDT_IDS
 * Compile with support for watchdog timer control points and task IDs.
 * Each task can be assigned an ID and Control Points can be defined within tasks
 * for watchdog timer integration and error handling.
 *
 * @par _TASK_LTS_POINTER
 * Compile with support for local task storage pointer.
 * LTS is a generic (void*) pointer that can reference a variable or structure
 * specific to a particular task, allowing the same callback code for multiple tasks.
 *
 * @par _TASK_PRIORITY
 * Support for layered scheduling priority.
 * Enables task prioritization by creating several schedulers and organizing them in priority layers.
 * Higher priority tasks are evaluated more frequently.
 *
 * @par _TASK_MICRO_RES
 * Support for microsecond resolution.
 * Enables microsecond scheduling resolution instead of default millisecond resolution.
 * All time-relevant parameters will be treated as microseconds.
 *
 * @par _TASK_STD_FUNCTION
 * Support for std::function (ESP8266/ESP32 ONLY).
 * Enables support for standard functions instead of function pointers for callbacks.
 *
 * @par _TASK_DEBUG
 * Make all methods and variables public for debug purposes.
 * Should not be used in production. Exposes all private and protected members as public.
 *
 * @par _TASK_INLINE
 * Make all methods "inline".
 * Needed to support some multi-tab, multi-file implementations. Lets compiler optimize.
 *
 * @par _TASK_TIMEOUT
 * Support for overall task timeout.
 * Any task can be set to time out after a certain period, and timeout can be reset.
 * Can be used as an individual Task's watchdog timer.
 *
 * @par _TASK_OO_CALLBACKS
 * Support for callbacks via inheritance.
 * Useful for implementing Tasks as classes derived from the Task class.
 * Enables dynamic binding for object-oriented callback approach.
 *
 * @par _TASK_EXPOSE_CHAIN
 * Methods to access tasks in the task chain.
 * Provides access to scheduling chain methods and tasks on the chain.
 *
 * @par _TASK_SCHEDULING_OPTIONS
 * Support for multiple scheduling options.
 * Enables different task scheduling options like TASK_SCHEDULE, TASK_SCHEDULE_NC, and TASK_INTERVAL.
 *
 * @par _TASK_SELF_DESTRUCT
 * Enable tasks to "self-destruct" after disable.
 * Tasks can be set to automatically delete themselves when disabled.
 *
 * @par _TASK_TICKLESS
 * Enable support for tickless sleep on FreeRTOS.
 * Enables support for tickless sleep mode on FreeRTOS systems.
 *
 * @par _TASK_DO_NOT_YIELD
 * Disable yield() method in execute() for ESP chips.
 * Disables automatic yielding in the execute loop for ESP-based systems.
 *
 * @par _TASK_ISR_SUPPORT
 * For ESP chips - place control methods in IRAM.
 * Places critical control methods in IRAM for ESP8266/ESP32 interrupt support.
 *
 * @par _TASK_NON_ARDUINO
 * For non-Arduino use.
 * Enables compilation for non-Arduino environments.
 *
 * @par _TASK_HEADER_AND_CPP
 * Compile CPP file (non-Arduino IDE platforms).
 * For non-Arduino IDE platforms that require explicit CPP compilation.
 *
 * @par _TASK_THREAD_SAFE
 * Enable additional functionality to request processing in a thread-safe manner.
 * Uses a request queue to safely invoke task control methods from other threads or interrupts.
 * Also enables basic mutex locking to prevent concurrent access issues.
 * Recommended for ESP32 and other MCUs running under preemptive schedulers like FreeRTOS.
 *
 */


#ifdef _TASK_NON_ARDUINO
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#else
#include <Arduino.h>
#endif

/**
 * @brief Forward declaration of Scheduler class
 */
class Scheduler;

/**
 * @defgroup SchedulingOptions Task Scheduling Options
 * @brief Constants defining different task scheduling behaviors
 * @{
 */

/**
 * @def TASK_SCHEDULE
 * @brief Default scheduling option
 * @details Maintains the original schedule. Tasks may need to "catch up" if delayed.
 * If a task is scheduled to run every 10 seconds and starts at 9:00:00,
 * the scheduler will try to invoke tasks as close to 9:00:10, 9:00:20 as possible.
 */
#define TASK_SCHEDULE       0

/**
 * @def TASK_SCHEDULE_NC
 * @brief Schedule with no catch-ups (always in the future)
 * @details Similar to TASK_SCHEDULE but without "catch up". Tasks are invoked at the next
 * scheduled point, but the number of iterations may not be correct if delayed.
 */
#define TASK_SCHEDULE_NC    1

/**
 * @def TASK_INTERVAL
 * @brief Interval-based scheduling (always in the future)
 * @details Schedules the next invocation with priority to a "period". A task scheduled for
 * 9:00:00 with a 10-second interval that was actually invoked at 9:00:06 will be
 * scheduled for the next invocation at 9:00:16.
 */
#define TASK_INTERVAL       2

/** @} */ // End of SchedulingOptions group

#ifdef _TASK_DEBUG
    #define _TASK_SCOPE  public
#else
    #define _TASK_SCOPE  private
#endif

/**
 * @defgroup TaskIterationOptions Task Iteration Options
 * @brief Common options for task scheduling iterations
 * @{
 */

/**
 * @def TASK_IMMEDIATE
 * @brief Task interval for immediate execution
 * @details When used as interval, causes task to execute immediately during next scheduling pass.
 */
#define TASK_IMMEDIATE          0

/**
 * @def TASK_FOREVER
 * @brief Task number of iterations for infinite execution
 * @details When used as iteration count, causes task to run indefinitely until explicitly disabled.
 */
#define TASK_FOREVER         (-1)

/**
 * @def TASK_ONCE
 * @brief Task single iteration
 * @details When used as iteration count, causes task to run only once and then become disabled.
 */
#define TASK_ONCE               1

/** @} */ // End of TaskIterationOptions group

/**
 * @defgroup IntervalNoDelayOptions Options for setIntervalNodelay() method
 * @brief Options controlling how interval changes are handled
 * @{
 */

/**
 * @def TASK_INTERVAL_KEEP
 * @brief Keep current execution timing
 * @details Maintains the current execution schedule when changing interval.
 */
#define TASK_INTERVAL_KEEP      0

/**
 * @def TASK_INTERVAL_RECALC
 * @brief Recalculate execution timing
 * @details Recalculates the next execution time based on the new interval.
 */
#define TASK_INTERVAL_RECALC    1

/**
 * @def TASK_INTERVAL_RESET
 * @brief Reset execution timing
 * @details Resets the execution schedule to start from the current time.
 */
#define TASK_INTERVAL_RESET     2

/** @} */ // End of IntervalNoDelayOptions group

/**
 * @defgroup TimeoutOptions Task Timeout Options
 * @brief Options for task timeout functionality
 * @{
 */
#ifdef _TASK_TIMEOUT
/**
 * @def TASK_NOTIMEOUT
 * @brief No timeout value
 * @details Used to indicate that a task should not have a timeout.
 */
#define TASK_NOTIMEOUT          0
#endif
/** @} */ // End of TimeoutOptions group

#ifdef _TASK_PRIORITY
    extern Scheduler* iCurrentScheduler;
#endif // _TASK_PRIORITY

#ifdef _TASK_INLINE
#define __TASK_INLINE  inline
#else
#define __TASK_INLINE
#endif

#ifdef _TASK_ISR_SUPPORT
#if defined (ARDUINO_ARCH_ESP8266)
#define __TASK_IRAM ICACHE_RAM_ATTR
#endif
#if  defined (ARDUINO_ARCH_ESP32)
#define __TASK_IRAM IRAM_ATTR
#endif
#endif
#ifndef __TASK_IRAM
#define __TASK_IRAM
#endif

/**
 * @defgroup TimeConstants Time Constants
 * @brief Predefined time constants for task intervals
 * @details Values are in milliseconds by default, or microseconds when _TASK_MICRO_RES is enabled
 * @{
 */
#ifndef _TASK_MICRO_RES

/**
 * @def TASK_MILLISECOND
 * @brief One millisecond interval
 */
#define TASK_MILLISECOND       1UL

/**
 * @def TASK_SECOND
 * @brief One second interval (1000 milliseconds)
 */
#define TASK_SECOND         1000UL

/**
 * @def TASK_MINUTE
 * @brief One minute interval (60000 milliseconds)
 */
#define TASK_MINUTE        60000UL

/**
 * @def TASK_HOUR
 * @brief One hour interval (3600000 milliseconds)
 */
#define TASK_HOUR        3600000UL

#else

/**
 * @def TASK_MILLISECOND
 * @brief One millisecond interval (1000 microseconds)
 */
#define TASK_MILLISECOND    1000UL

/**
 * @def TASK_SECOND
 * @brief One second interval (1000000 microseconds)
 */
#define TASK_SECOND      1000000UL

/**
 * @def TASK_MINUTE
 * @brief One minute interval (60000000 microseconds)
 */
#define TASK_MINUTE     60000000UL

/**
 * @def TASK_HOUR
 * @brief One hour interval (3600000000 microseconds)
 */
#define TASK_HOUR     3600000000UL

#endif  // _TASK_MICRO_RES
/** @} */ // End of TimeConstants group

#ifdef _TASK_TICKLESS
/**
 * @defgroup NextRunStates Task Next Run State Definitions
 * @brief State flags for tickless operation next run scheduling
 *
 * These constants define the next execution state of tasks in tickless mode,
 * enabling power-efficient operation by categorizing when tasks need to run.
 * Used internally by the scheduler to optimize sleep/wake cycles and minimize
 * power consumption in battery-powered applications.
 *
 * @note Only available when _TASK_TICKLESS compile-time option is enabled
 * @see _TASK_TICKLESS
 * @{
 */

/**
 * @brief Undefined next run state
 *
 * Indicates that the task's next execution time is not defined or has not
 * been calculated. This is typically an initial state or error condition
 * where the scheduler cannot determine when the task should next execute.
 *
 * Binary value: 0b0 (0)
 *
 * @note Used internally by tickless scheduler for state management
 * @see _TASK_TICKLESS
 * @since Version 3.0.0
 */
#define _TASK_NEXTRUN_UNDEFINED 0b0

/**
 * @brief Immediate next run state
 *
 * Indicates that the task should execute immediately on the next scheduler
 * pass, bypassing any timing delays. Used for tasks that have been triggered
 * by events, forced execution, or have reached their scheduled execution time.
 *
 * Binary value: 0b1 (1)
 *
 * @note In tickless mode, prevents the system from entering deep sleep
 * @see forceNextIteration(), _TASK_TICKLESS
 * @since Version 3.0.0
 */
#define _TASK_NEXTRUN_IMMEDIATE 0b1

/**
 * @brief Timed next run state
 *
 * Indicates that the task has a specific future execution time and should
 * wait until that time arrives. This enables the tickless scheduler to
 * calculate optimal sleep durations and enter power-saving modes until
 * the earliest task execution time.
 *
 * Hexadecimal value: 0x10 (16)
 *
 * @note Enables deep sleep optimization in tickless mode
 * @note Value deliberately different from other states for bit manipulation
 * @see _TASK_TICKLESS, getInterval(), delay()
 * @since Version 3.0.0
 */
#define _TASK_NEXTRUN_TIMED     0x10

/** @} */ // End of NextRunStates group
#endif  //  _TASK_TICKLESS

#ifdef _TASK_THREAD_SAFE

/**
 * @enum _task_request_type_t
 * @brief Task request types for thread-safe operations
 *
 * @details This enum mirrors the Task class API for invocation via requestAction methods
 * when _TASK_THREAD_SAFE option is active. It provides a thread-safe way to execute
 * task control methods by placing requests on a queue that is processed during the
 * scheduler's execute() method.
 *
 * Each enum value corresponds to a specific Task method that can be called safely
 * from other threads or interrupt contexts. The scheduler processes these requests
 * atomically during its execution cycle.
 *
 * @note Only available when compiled with _TASK_THREAD_SAFE support.
 *
 * @see Scheduler::requestAction()
 */
typedef enum {
#ifdef _TASK_STATUS_REQUEST
    TASK_SR_REQUEST_SETWAITING,        ///< Thread-safe StatusRequest::setWaiting()
    TASK_SR_REQUEST_SIGNAL,            ///< Thread-safe StatusRequest::signal()
    TASK_SR_REQUEST_SIGNALCOMPLETE,    ///< Thread-safe StatusRequest::signalComplete()

#ifdef _TASK_TIMEOUT
    TASK_SR_REQUEST_SETTIMEOUT,        ///< Thread-safe StatusRequest::setTimeout()
    TASK_SR_REQUEST_RESETTIMEOUT,      ///< Thread-safe StatusRequest::resetTimeout()
#endif  // _TASK_TIMEOUT

#endif  // _TASK_STATUS_REQUEST

#ifdef _TASK_LTS_POINTER
    TASK_REQUEST_SETLTSPOINTER,        ///< Thread-safe Task::setLtsPointer()
#endif

#ifdef _TASK_SELF_DESTRUCT
    TASK_REQUEST_SETSELFDESTRUCT,      ///< Thread-safe Task::setSelfDestruct()
#endif  // _TASK_SELF_DESTRUCT

#ifdef _TASK_SCHEDULING_OPTIONS
    TASK_REQUEST_SETSCHEDULINGOPTION,  ///< Thread-safe Task::setSchedulingOption()
#endif  // _TASK_SCHEDULING_OPTIONS

#ifdef _TASK_TIMEOUT
    TASK_REQUEST_SETTIMEOUT,           ///< Thread-safe Task::setTimeout()
    TASK_REQUEST_RESETTIMEOUT,         ///< Thread-safe Task::resetTimeout()
#endif  // _TASK_TIMEOUT

#ifdef _TASK_STATUS_REQUEST
    TASK_REQUEST_WAITFOR,              ///< Thread-safe Task::waitFor()
    TASK_REQUEST_WAITFORDELAYED,       ///< Thread-safe Task::waitForDelayed()
#endif  // _TASK_STATUS_REQUEST

#ifdef _TASK_WDT_IDS
    TASK_REQUEST_SETID,                ///< Thread-safe Task::setId()
    TASK_REQUEST_SETCONTROLPOINT,      ///< Thread-safe Task::setControlPoint()
#endif  // _TASK_WDT_IDS

    TASK_REQUEST_ENABLE,               ///< Thread-safe Task::enable()
    TASK_REQUEST_ENABLEIFNOT,          ///< Thread-safe Task::enableIfNot()
    TASK_REQUEST_ENABLEDELAYED,        ///< Thread-safe Task::enableDelayed()
    TASK_REQUEST_RESTART,              ///< Thread-safe Task::restart()
    TASK_REQUEST_RESTARTDELAYED,       ///< Thread-safe Task::restartDelayed()
    TASK_REQUEST_DELAY,                ///< Thread-safe Task::delay()
    TASK_REQUEST_ADJUST,               ///< Thread-safe Task::adjust()
    TASK_REQUEST_FORCENEXTITERATION,   ///< Thread-safe Task::forceNextIteration()
    TASK_REQUEST_DISABLE,              ///< Thread-safe Task::disable()
    TASK_REQUEST_ABORT,                ///< Thread-safe Task::abort()
    TASK_REQUEST_CANCEL,               ///< Thread-safe Task::cancel()
    TASK_REQUEST_SET,                  ///< Thread-safe Task::set()
    TASK_REQUEST_SETINTERVAL,          ///< Thread-safe Task::setInterval()
    TASK_REQUEST_SETINTERVALNODELAY,   ///< Thread-safe Task::setIntervalNodelay()
    TASK_REQUEST_SETITERATIONS,        ///< Thread-safe Task::setIterations()
    TASK_REQUEST_SETCALLBACK,          ///< Thread-safe Task::setCallback()
    TASK_REQUEST_SETONENABLE,          ///< Thread-safe Task::setOnEnable()
    TASK_REQUEST_SETONDISABLE          ///< Thread-safe Task::setOnDisable()
} _task_request_type_t;

#ifdef _TASK_STATUS_REQUEST
// void setWaiting(unsigned int aCount = 1);
#define TASK_SR_REQUEST_SETWAITING_1        TASK_SR_REQUEST_SETWAITING
#define TASK_SR_REQUEST_SETWAITING_1_DEFAULT   1
// __TASK_IRAM signal(int aStatus = 0);
#define TASK_SR_REQUEST_SIGNAL_1            TASK_SR_REQUEST_SIGNAL
#define TASK_SR_REQUEST_SIGNAL_1_DEFAULT    0
// __TASK_IRAM signalComplete(int aStatus = 0);
#define TASK_SR_REQUEST_SIGNALCOMPLETE_1    TASK_SR_REQUEST_SIGNALCOMPLETE
#define TASK_SR_REQUEST_SIGNALCOMPLETE_1_DEFAULT    0

#ifdef _TASK_TIMEOUT
// void setTimeout(unsigned long aTimeout)
#define TASK_SR_REQUEST_SETTIMEOUT_1        TASK_SR_REQUEST_SETTIMEOUT
// void resetTimeout();
#define TASK_SR_REQUEST_RESETTIMEOUT_0      TASK_SR_REQUEST_RESETTIMEOUT
#endif  // _TASK_TIMEOUT

#endif

#ifdef _TASK_LTS_POINTER
// void  setLtsPointer(void *aPtr);
#define TASK_REQUEST_SETLTSPOINTER_1        TASK_REQUEST_SETLTSPOINTER
#endif

#ifdef _TASK_SELF_DESTRUCT
// void setSelfDestruct(bool aSelfDestruct=true)
#define TASK_REQUEST_SETSELFDESTRUCT_1      TASK_REQUEST_SETSELFDESTRUCT
#define TASK_REQUEST_SETSELFDESTRUCT_1_DEFAULT  true
#endif

#ifdef _TASK_SCHEDULING_OPTIONS
// void setSchedulingOption(unsigned int aOption)
#define TASK_REQUEST_SETSCHEDULINGOPTION_1  TASK_REQUEST_SETSCHEDULINGOPTION
#endif

#ifdef _TASK_TIMEOUT
// void setTimeout(unsigned long aTimeout, bool aReset=false);
#define TASK_REQUEST_SETTIMEOUT_2           TASK_REQUEST_SETTIMEOUT
#define TASK_REQUEST_SETTIMEOUT_2_DEFAULT   false
// void resetTimeout();
#define TASK_REQUEST_RESETTIMEOUT_0         TASK_REQUEST_RESETTIMEOUT
#endif  // _TASK_TIMEOUT

#ifdef _TASK_STATUS_REQUEST
// bool waitFor(StatusRequest* aStatusRequest, unsigned long aInterval = 0, long aIterations = 1)
#define TASK_REQUEST_WAITFOR_3              TASK_REQUEST_WAITFOR
#define TASK_REQUEST_WAITFOR_2_DEFAULT  0
#define TASK_REQUEST_WAITFOR_3_DEFAULT  1
// bool waitForDelayed(StatusRequest* aStatusRequest, unsigned long aInterval = 0, long aIterations = 1)
#define TASK_REQUEST_WAITFORDELAYED_3       TASK_REQUEST_WAITFORDELAYED
#define TASK_REQUEST_WAITFORDELAYED_2_DEFAULT   0
#define TASK_REQUEST_WAITFORDELAYED_3_DEFAULT   1
#endif  // _TASK_STATUS_REQUEST

#ifdef _TASK_WDT_IDS
// __TASK_INLINE void setId(unsigned int aID) ;
#define TASK_REQUEST_SETID_1                TASK_REQUEST_SETID
// __TASK_INLINE void setControlPoint(unsigned int aPoint) ;
#define TASK_REQUEST_SETCONTROLPOINT_1      TASK_REQUEST_SETCONTROLPOINT
#endif  // _TASK_WDT_IDS

// bool __TASK_IRAM enable();
#define TASK_REQUEST_ENABLE_0               TASK_REQUEST_ENABLE
// bool __TASK_IRAM enableIfNot();
#define TASK_REQUEST_ENABLEIFNOT_0          TASK_REQUEST_ENABLEIFNOT
// bool __TASK_IRAM enableDelayed(unsigned long aDelay=0);
#define TASK_REQUEST_ENABLEDELAYED_1        TASK_REQUEST_ENABLEDELAYED
#define TASK_REQUEST_ENABLEDELAYED_1_DEFAULT    0
// bool __TASK_IRAM restart();
#define TASK_REQUEST_RESTART_0              TASK_REQUEST_RESTART
// bool __TASK_IRAM restartDelayed(unsigned long aDelay=0);
#define TASK_REQUEST_RESTARTDELAYED_1       TASK_REQUEST_RESTARTDELAYED
#define TASK_REQUEST_RESTARTDELAYED_1_DEFAULT   0
// void __TASK_IRAM delay(unsigned long aDelay=0);
#define TASK_REQUEST_DELAY_1                TASK_REQUEST_DELAY
#define TASK_REQUEST_DELAY_1_DEFAULT    0
// void adjust(long aInterval);
#define TASK_REQUEST_ADJUST_1               TASK_REQUEST_ADJUST
// void __TASK_IRAM forceNextIteration();
#define TASK_REQUEST_FORCENEXTITERATION_0   TASK_REQUEST_FORCENEXTITERATION
// bool disable();
#define TASK_REQUEST_DISABLE_0              TASK_REQUEST_DISABLE
// void abort();
#define TASK_REQUEST_ABORT_0                TASK_REQUEST_ABORT
// void cancel();
#define TASK_REQUEST_CANCEL_0               TASK_REQUEST_CANCEL
// void set(unsigned long aInterval, long aIterations, TaskCallback aCallback,TaskOnEnable aOnEnable=NULL, TaskOnDisable aOnDisable=NULL);
#define TASK_REQUEST_SET_5                  TASK_REQUEST_SET
#define TASK_REQUEST_SET_4_DEFAULT          NULL
#define TASK_REQUEST_SET_5_DEFAULT          NULL
// void setInterval(unsigned long aInterval);
#define TASK_REQUEST_SETINTERVAL_1          TASK_REQUEST_SETINTERVAL
// void setIntervalNodelay(unsigned long aInterval, unsigned int aOption = TASK_INTERVAL_KEEP);
#define TASK_REQUEST_SETINTERVALNODELAY_2   TASK_REQUEST_SETINTERVALNODELAY
#define TASK_REQUEST_SETINTERVALNODELAY_2_DEFAULT   TASK_INTERVAL_KEEP
// void setIterations(long aIterations);
#define TASK_REQUEST_SETITERATIONS_1        TASK_REQUEST_SETITERATIONS
// void setCallback(TaskCallback aCallback)
#define TASK_REQUEST_SETCALLBACK_1          TASK_REQUEST_SETCALLBACK
// setOnEnable(TaskOnEnable aCallback) ;
#define TASK_REQUEST_SETONENABLE_1          TASK_REQUEST_SETONENABLE
// setOnDisable(TaskOnDisable aCallback) ;
#define TASK_REQUEST_SETONDISABLE_1         TASK_REQUEST_SETONDISABLE


/**
 * @struct _task_request_t
 * @brief Structure for thread-safe task operation requests
 *
 * @details This structure is used to call Scheduler::requestAction() and provide parameters
 * to the respective API method in a thread-safe manner. When _TASK_THREAD_SAFE is enabled,
 * this structure allows tasks to be controlled safely from interrupt contexts or other threads
 * by queuing requests that are processed atomically during the scheduler's execute() cycle.
 *
 * The structure contains the request type and up to 5 parameters that correspond to the
 * parameters of the target Task or StatusRequest method being called. The scheduler will
 * unpack these parameters and call the appropriate method during its execution.
 *
 * Usage example:
 * @code
 * _task_request_t request;
 * request.req_type = TASK_REQUEST_ENABLE;
 * request.object_ptr = &myTask;
 * // param1-param5 not needed for enable()
 * scheduler.requestAction(&request);
 * @endcode
 *
 * @note Only available when compiled with _TASK_THREAD_SAFE support.
 * @note Parameter fields are unsigned long. On platforms where sizeof(void*) >
 *       sizeof(unsigned long), pointer values passed via thread-safe requests
 *       will be truncated. Use _TASK_THREAD_SAFE only on 32-bit targets.
 *
 * @see Scheduler::requestAction(), _task_request_type_t
 */
typedef struct {
    _task_request_type_t req_type;     ///< Type of request (corresponds to Task/StatusRequest method)
    void*           object_ptr;        ///< Pointer to Task or StatusRequest object to operate on
    unsigned long   param1;            ///< First parameter for the target method (if needed)
    unsigned long   param2;            ///< Second parameter for the target method (if needed)
    unsigned long   param3;            ///< Third parameter for the target method (if needed)
    unsigned long   param4;            ///< Fourth parameter for the target method (if needed)
    unsigned long   param5;            ///< Fifth parameter for the target method (if needed)
} _task_request_t;

#endif  //_TASK_THREAD_SAFE

#ifdef _TASK_STATUS_REQUEST

/**
 * @defgroup StatusRequestCodes Status Request Return Codes
 * @brief Status codes returned by StatusRequest operations
 * @{
 */

/**
 * @def TASK_SR_OK
 * @brief Successful completion status
 * @details Default successful completion status for StatusRequest operations.
 */
#define TASK_SR_OK          0

/**
 * @def TASK_SR_ERROR
 * @brief General error status
 * @details Indicates an error condition occurred during StatusRequest operation.
 */
#define TASK_SR_ERROR       (-1)

/**
 * @def TASK_SR_CANCEL
 * @brief Cancelled status
 * @details Indicates the StatusRequest operation was cancelled.
 */
#define TASK_SR_CANCEL      (-32766)

/**
 * @def TASK_SR_ABORT
 * @brief Aborted status
 * @details Indicates the StatusRequest operation was aborted.
 */
#define TASK_SR_ABORT       (-32767)

/**
 * @def TASK_SR_TIMEOUT
 * @brief Timeout status
 * @details Indicates the StatusRequest operation timed out.
 */
#define TASK_SR_TIMEOUT     (-32768)

/** @} */ // End of StatusRequestCodes group

/**
 * @defgroup StatusRequestModes Status Request Internal Modes
 * @brief Internal mode constants for StatusRequest operations
 * @{
 */

/**
 * @def _TASK_SR_NODELAY
 * @brief No delay mode for StatusRequest
 */
#define _TASK_SR_NODELAY    1

/**
 * @def _TASK_SR_DELAY
 * @brief Delay mode for StatusRequest
 */
#define _TASK_SR_DELAY      2

/** @} */ // End of StatusRequestModes group

/**
 * @class StatusRequest
 * @brief Status Request object for event-driven task invocation
 *
 * @details StatusRequest objects allow tasks to wait on an event and signal event completion to each other.
 * This enables event-driven programming where one task can "signal" completion of its function via a
 * StatusRequest object, and other tasks can "wait" on the same StatusRequest object.
 *
 * Key features:
 * - Can wait for multiple events (set via setWaiting())
 * - Supports timeout functionality
 * - Provides completion status codes
 * - Thread-safe signaling mechanisms
 *
 * @note Each task has an internal StatusRequest object that is:
 * - Always waits on 1 event (completion of this task)
 * - Activated (set to "waiting" status) after Task is enabled
 * - Completed after Task is disabled
 *
 * @see Task::waitFor(), Task::waitForDelayed()
 */
class StatusRequest {
  friend class Scheduler;
  public:
    /**
     * @brief Default constructor
     * @details Creates Status Request object with "completed" status on creation.
     */
    __TASK_INLINE StatusRequest();

    /**
     * @brief Activates Status Request object
     * @param aCount Number of events to wait for (default = 1)
     * @details By default each object waits on one event only. If aCount is supplied,
     * StatusRequest can wait on multiple events. For example, setWaiting(3) will wait
     * on three signals - useful for waiting for completion of measurements from 3 sensors.
     */
    __TASK_INLINE void setWaiting(unsigned int aCount = 1);

    /**
     * @brief Signals completion of one event
     * @param aStatus Completion code (default = 0)
     * @return true if signal was processed successfully
     * @details Signals completion of the event to the StatusRequest object and passes
     * a completion code. Passing a negative status code is considered reporting an error
     * condition and will complete the status request regardless of how many outstanding
     * signals it is still waiting for. Only the latest status code is kept.
     */
    __TASK_INLINE bool  signal(int aStatus = 0);

    /**
     * @brief Signals completion of ALL events
     * @param aStatus Completion code (default = 0)
     * @details Signals completion of ALL events to the StatusRequest object and passes
     * a completion code. The status request completes regardless of how many events
     * it is still waiting on.
     */
    __TASK_INLINE void  signalComplete(int aStatus = 0);

    /**
     * @brief Check if status request is pending (deprecated)
     * @return true if status request is still waiting for events
     * @deprecated Use isPending() instead
     */
    __TASK_INLINE bool pending() { return isPending(); };

    /**
     * @brief Check if status request is pending
     * @return true if status request is still waiting for events to happen
     */
    __TASK_INLINE bool isPending();

    /**
     * @brief Check if status request is completed (deprecated)
     * @return true if status request event has completed
     * @deprecated Use isCompleted() instead
     */
    __TASK_INLINE bool completed() { return isCompleted(); };

    /**
     * @brief Check if status request is completed
     * @return true if status request event has completed
     */
    __TASK_INLINE bool isCompleted();

    /**
     * @brief Get the status code
     * @return The status code passed to signal() and signalComplete() methods
     * @details Any positive number is considered successful completion status.
     * A 0 (zero) is considered default successful completion status.
     * Any negative number is considered an error code and unsuccessful completion.
     */
    __TASK_INLINE int  getStatus();

    /**
     * @brief Get count of remaining events
     * @return The count of events not yet completed
     * @details Typically by default a StatusRequest object only waits on 1 event.
     * However, when waiting on multiple events, returns number of events not yet completed.
     */
    __TASK_INLINE int  getCount();

#ifdef _TASK_TIMEOUT
    /**
     * @brief Set timeout for this StatusRequest object
     * @param aTimeout Timeout interval in milliseconds (or microseconds)
     * @details When enabled, the activated StatusRequest object will complete with
     * the code TASK_SR_TIMEOUT if no other process calls its signal() or signalComplete() method.
     */
    __TASK_INLINE void setTimeout(unsigned long aTimeout) { iTimeout = aTimeout; };

    /**
     * @brief Get timeout interval
     * @return The timeout interval for the current StatusRequest object
     * @details This is the full original interval, not the remaining time.
     */
    __TASK_INLINE unsigned long getTimeout() { return iTimeout; };

    /**
     * @brief Reset timeout counter
     * @details Resets the current timeout counter to the original value.
     * The timeout countdown starts from the beginning again.
     */
    __TASK_INLINE void resetTimeout();

    /**
     * @brief Get time until timeout
     * @return Number of milliseconds (or microseconds) until timeout
     * @details The value could be negative if the timeout has already occurred.
     */
    __TASK_INLINE long untilTimeout();
#endif

  _TASK_SCOPE:
    unsigned int  iCount;          // number of statuses to wait for. waiting for more that 65000 events seems unreasonable: unsigned int should be sufficient
    int           iStatus;         // status of the last completed request. negative = error;  zero = OK; positive = OK with a specific status (see TASK_SR_ constants)

#ifdef _TASK_TIMEOUT
    unsigned long            iTimeout;               // Task overall timeout
    unsigned long            iStarttime;             // millis at task start time
#endif // _TASK_TIMEOUT
};
#endif  // _TASK_STATUS_REQUEST

/**
 * @defgroup CallbackTypes Task Callback Types
 * @brief Type definitions for task callback functions
 * @{
 */
#ifdef _TASK_STD_FUNCTION
#include <functional>
/**
 * @typedef TaskCallback
 * @brief Main task callback function type (std::function version)
 * @details Function called periodically when task is executed. Should be non-blocking
 * and return quickly to maintain cooperative multitasking behavior.
 */
typedef std::function<void()> TaskCallback;

/**
 * @typedef TaskOnDisable
 * @brief Task disable callback function type (std::function version)
 * @details Function called when task is disabled. Used for cleanup operations.
 */
typedef std::function<void()> TaskOnDisable;

/**
 * @typedef TaskOnEnable
 * @brief Task enable callback function type (std::function version)
 * @details Function called when task is enabled. Should return true to enable task,
 * false to keep task disabled. Used for initialization operations.
 */
typedef std::function<bool()> TaskOnEnable;
#else
/**
 * @typedef TaskCallback
 * @brief Main task callback function pointer type
 * @details Function called periodically when task is executed. Should be non-blocking
 * and return quickly to maintain cooperative multitasking behavior.
 */
typedef void (*TaskCallback)();

/**
 * @typedef TaskOnDisable
 * @brief Task disable callback function pointer type
 * @details Function called when task is disabled. Used for cleanup operations.
 */
typedef void (*TaskOnDisable)();

/**
 * @typedef TaskOnEnable
 * @brief Task enable callback function pointer type
 * @details Function called when task is enabled. Should return true to enable task,
 * false to keep task disabled. Used for initialization operations.
 */
typedef bool (*TaskOnEnable)();
#endif  // _TASK_STD_FUNCTION
/** @} */ // End of CallbackTypes group


#ifdef _TASK_SLEEP_ON_IDLE_RUN
  /**
   * @typedef SleepCallback
   * @brief Sleep function callback type
   * @param aDuration Duration to sleep in milliseconds
   * @details Function pointer type for custom sleep implementations during idle periods.
   */
  typedef void (*SleepCallback)( unsigned long aDuration );

  /**
   * @brief Global pointer to scheduler that controls sleep functionality
   */
  extern Scheduler* iSleepScheduler;

  /**
   * @brief Global pointer to custom sleep method
   */
  extern SleepCallback iSleepMethod;
#endif  // _TASK_SLEEP_ON_IDLE_RUN

/**
 * @struct _task_status
 * @brief Internal task status structure
 * @details Bit-packed structure containing task state flags for efficient memory usage.
 * This structure tracks various task states and conditions.
 */
typedef struct  {
    bool  enabled       : 1;           ///< Indicates that task is enabled or not
    bool  inonenable    : 1;           ///< Indicates that task execution is inside OnEnable method (preventing infinite loops)
    bool  canceled      : 1;           ///< Indication that task has been canceled prior to normal end of all iterations or regular call to disable()
#ifdef _TASK_SELF_DESTRUCT
    bool  selfdestruct  : 1;           ///< Indication that task has been requested to self-destruct on disable
    bool  sd_request    : 1;           ///< Request for scheduler to delete task object and take task out of the queue
#endif  // _TASK_SELF_DESTRUCT
#ifdef _TASK_STATUS_REQUEST
    uint8_t  waiting    : 2;           ///< Indication if task is waiting on the status request
#endif  // _TASK_STATUS_REQUEST

#ifdef _TASK_TIMEOUT
    bool  timeout       : 1;           ///< Indication if task timed out
#endif  //  _TASK_TIMEOUT
} _task_status;


/**
 * @class Task
 * @brief Core task class for cooperative multitasking
 *
 * @details A "Task" is an action, a part of the program logic, which requires scheduled execution.
 * A concept of Task combines the following aspects:
 * - Program code performing specific activities (callback methods)
 * - Execution interval
 * - Number of execution iterations
 * - (Optionally) Execution start event (Status Request)
 * - (Optionally) Pointer to a Local Task Storage area
 *
 * Tasks perform certain functions, which could require periodic or one-time execution,
 * update of specific variables, or waiting for specific events. Tasks also could be
 * controlling specific hardware, or triggered by hardware interrupts.
 *
 * For execution purposes Tasks are linked into execution chains, which are processed
 * by the Scheduler in the order they were added (linked together).
 *
 * Key Features:
 * - Cooperative multitasking (non-preemptive)
 * - Event-driven execution via StatusRequest objects
 * - Dynamic parameter adjustment during runtime
 * - Support for callback chaining and state machines
 * - Built-in timeout and watchdog functionality
 * - Local task storage for task-specific data
 *
 * @note Tasks are responsible for supporting cooperative multitasking by being "good neighbors",
 * i.e., running their callback methods quickly in a non-blocking way, and releasing control
 * back to the scheduler as soon as possible.
 *
 * @see Scheduler, StatusRequest
 */
class Task {
  friend class Scheduler;
  public:

    /**
     * @brief Task constructor with parameters
     * @param aInterval Execution interval in milliseconds (or microseconds if _TASK_MICRO_RES enabled) (default = 0)
     * @param aIterations Number of iterations, -1 for indefinite execution (default = 0)
     * @param aScheduler Optional reference to existing scheduler. If supplied (not NULL) this task will be appended to the task chain (default = NULL)
     * @param aEnable Optional. Value of true will create task enabled (default = false)
     *
     * @details Creates a task that is scheduled to run every aInterval milliseconds, aIterations times.
     * All tasks are created disabled by default (unless aEnable = true). You have to explicitly enable
     * the task for execution.
     *
     * @note Tasks do not remember the number of iterations set initially. After the iterations are done,
     * internal iteration counter is 0. If you need to perform another set of iterations, you need to set
     * the number of iterations again.
     *
     * @note Tasks which performed all their iterations remain active.
     */
#ifdef _TASK_OO_CALLBACKS
    __TASK_INLINE Task(unsigned long aInterval=0, long aIterations=0, Scheduler* aScheduler=NULL, bool aEnable=false
#ifdef _TASK_SELF_DESTRUCT
    , bool aSelfDestruct=false);
#else
    );
#endif  // #ifdef _TASK_SELF_DESTRUCT
#else
    /**
     * @brief Task constructor with callback parameters
     * @param aInterval Execution interval in milliseconds (or microseconds if _TASK_MICRO_RES enabled) (default = 0)
     * @param aIterations Number of iterations, -1 for indefinite execution (default = 0)
     * @param aCallback Pointer to the void callback method without parameters (default = NULL)
     * @param aScheduler Optional reference to existing scheduler. If supplied (not NULL) this task will be appended to the task chain (default = NULL)
     * @param aEnable Optional. Value of true will create task enabled (default = false)
     * @param aOnEnable Pointer to the bool OnEnable callback method without parameters, invoked when task is enabled (default = NULL)
     * @param aOnDisable Pointer to the void OnDisable method without parameters, invoked when task is disabled (default = NULL)
     *
     * @details Creates a task with callback methods. OnEnable method returns true if task should be enabled,
     * false if it should remain disabled. OnDisable method is called when task is disabled.
     *
     * @note OnEnable callback method is called immediately when task is enabled, which could be well ahead
     * of the scheduled execution time of the task. It is advisable to explicitly enable tasks with OnEnable
     * methods after all initialization methods completed (e.g., at the end of setup() method).
     */
    __TASK_INLINE Task(unsigned long aInterval=0, long aIterations=0, TaskCallback aCallback=NULL, Scheduler* aScheduler=NULL, bool aEnable=false, TaskOnEnable aOnEnable=NULL, TaskOnDisable aOnDisable=NULL
#ifdef _TASK_SELF_DESTRUCT
  , bool aSelfDestruct=false);
#else
  );
#endif  // #ifdef _TASK_SELF_DESTRUCT
#endif // _TASK_OO_CALLBACKS


#ifdef _TASK_STATUS_REQUEST
#ifdef _TASK_OO_CALLBACKS
    /**
     * @brief Constructor for StatusRequest-based tasks (Object-Oriented callbacks)
     * @param aScheduler Pointer to scheduler
     * @details Creates a Task for activation on event. Such tasks must run waitFor() method,
     * their interval, iteration and enabled status will be set by that method.
     */
    __TASK_INLINE Task(Scheduler* aScheduler);
#else
    /**
     * @brief Constructor for StatusRequest-based tasks (Function pointer callbacks)
     * @param aCallback Pointer to callback function
     * @param aScheduler Pointer to scheduler
     * @param aOnEnable Pointer to OnEnable callback (default = NULL)
     * @param aOnDisable Pointer to OnDisable callback (default = NULL)
     * @details Creates a Task for activation on event. Such tasks must run waitFor() method,
     * their interval, iteration and enabled status will be set by that method.
     */
    __TASK_INLINE Task(TaskCallback aCallback, Scheduler* aScheduler, TaskOnEnable aOnEnable=NULL, TaskOnDisable aOnDisable=NULL);
#endif // _TASK_OO_CALLBACKS
#endif  // _TASK_STATUS_REQUEST

    /**
     * @brief Virtual destructor
     * @details Properly destroys the task object and cleans up resources.
     */
    virtual __TASK_INLINE ~Task();

#ifdef _TASK_TIMEOUT
    /**
     * @brief Set overall task timeout
     * @param aTimeout Timeout period in milliseconds (or microseconds)
     * @param aReset Whether to reset timeout counter (default = false)
     * @details Any task can be set to time out after a certain period of time.
     * Timeout can be reset, so it can be used as an individual Task's watchdog timer.
     */
    __TASK_INLINE void setTimeout(unsigned long aTimeout, bool aReset=false);

    /**
     * @brief Reset timeout counter
     * @details Resets the timeout counter to start counting from the beginning.
     */
    __TASK_INLINE void resetTimeout();

    /**
     * @brief Get timeout value
     * @return Current timeout value in milliseconds (or microseconds)
     */
    __TASK_INLINE unsigned long getTimeout();

    /**
     * @brief Get time until timeout
     * @return Number of milliseconds (or microseconds) until timeout occurs
     * @details Returns negative value if timeout has already occurred.
     */
    __TASK_INLINE long untilTimeout();

    /**
     * @brief Check if task has timed out (deprecated)
     * @return true if task has timed out
     * @deprecated Use isTimedOut() instead
     */
    __TASK_INLINE bool timedOut() { return isTimedOut(); };

    /**
     * @brief Check if task has timed out
     * @return true if task has timed out
     */
    __TASK_INLINE bool isTimedOut();
#endif

    /**
     * @brief Enable the task
     * @return true if task was successfully enabled
     * @details Enables the task and schedules it for immediate execution (without delay) at this or next
     * scheduling pass. Scheduler will execute the next pass without any delay because there is a task
     * which was enabled and requires execution.
     *
     * @note If task being enabled is not assigned to a scheduler and is not part of execution chain,
     * then task will not be enabled.
     *
     * @note enable() invokes task's OnEnable method (if not NULL) immediately, which can prepare task
     * for execution. OnEnable must return true for task to be enabled. If OnEnable returns false,
     * task remains disabled.
     *
     * @warning OnEnable is invoked every time enable() is called, regardless if task is already enabled.
     * Alignment to current millis() is performed after OnEnable exits.
     */
    __TASK_INLINE bool  enable();

    /**
     * @brief Enable the task only if it was previously disabled
     * @return Previous enable state: true if task was already enabled, false if task was disabled
     * @details Since enable() schedules Task for execution immediately, this method provides a way
     * to activate tasks and schedule them for immediate execution only if they are not active already.
     * All NOTES from the enable() method apply.
     */
    __TASK_INLINE bool  enableIfNot();

    /**
     * @brief Enable the task with delay
     * @param aDelay Delay before first execution in milliseconds (default = 0)
     * @return true if task was successfully enabled
     * @details Enables the task and schedules it for execution after a specific delay.
     */
    __TASK_INLINE bool  enableDelayed(unsigned long aDelay=0);

    /**
     * @brief Restart the task
     * @return true if task was successfully restarted
     * @details For tasks with limited number of iterations only, restart method will re-enable the task,
     * set the number of iterations back to last set value, and schedule task for execution as soon as possible.
     */
    __TASK_INLINE bool  restart();

    /**
     * @brief Restart the task with delay
     * @param aDelay Delay before first execution in milliseconds (default = 0)
     * @return true if task was successfully restarted
     * @details Same as restart() method, with the only difference being that Task is scheduled to run
     * first iteration after a delay.
     */
    __TASK_INLINE bool  restartDelayed(unsigned long aDelay=0);

    /**
     * @brief Delay the task execution
     * @param aDelay Delay in milliseconds (default = 0)
     * @details Schedules the task for execution after a delay, but does not change the enabled/disabled
     * status of the task. A delay of 0 (zero) will delay task for current execution interval.
     * Use forceNextIteration() method to force execution during immediate next scheduling pass.
     */
    __TASK_INLINE void  delay(unsigned long aDelay=0);

    /**
     * @brief Adjust task interval
     * @param aInterval New interval value (can be negative for adjustment)
     * @details Adjusts the task execution interval by the specified amount.
     */
    __TASK_INLINE void  adjust(long aInterval);

    /**
     * @brief Force next iteration to execute immediately
     * @details Schedules the task for execution during immediate next scheduling pass.
     * The Task must be already enabled prior to this method. Task's schedule is adjusted
     * to run from this moment in time.
     *
     * @note If a task was running every 10 seconds: 10, 20, 30, calling forceNextIteration
     * at 44th second will make subsequent schedule look like: 44, 54, 64, 74, ..
     */
    __TASK_INLINE void  forceNextIteration();

    /**
     * @brief Disable the task
     * @return Previous enabled state: true if task was enabled, false otherwise
     * @details Scheduler will not execute this task any longer, even if it remains in the chain.
     * Task can be later re-enabled for execution. If not NULL, task's OnDisable method is invoked
     * almost immediately (actually during the next scheduling pass).
     * OnDisable is invoked only if task was in enabled state.
     */
    __TASK_INLINE bool  disable();

    /**
     * @brief Abort the task
     * @details Immediately stops task execution and marks it as aborted.
     * Different from disable() in that it indicates abnormal termination.
     * OnDisable method is NOT invoked for Aborted tasks.
     */
    __TASK_INLINE void  abort();

    /**
     * @brief Cancel the task
     * @details Marks the task as cancelled, indicating it was stopped before normal completion.
     * OnDisable method is invoked if task was in enabled state.
     */
    __TASK_INLINE void  cancel();

    /**
     * @brief Check if task is enabled
     * @return true if task is enabled
     */
    __TASK_INLINE bool  isEnabled();

    /**
     * @brief Check if task is cancelled
     * @return true if task has been cancelled
     */
    __TASK_INLINE bool  isCanceled();

    /**
     * @brief Check if task is cancelled (deprecated)
     * @return true if task has been cancelled
     * @deprecated Use isCanceled() instead
     */
    __TASK_INLINE bool  canceled() { return isCanceled(); };

#ifdef _TASK_SCHEDULING_OPTIONS
    /**
     * @brief Get the current scheduling option for this task
     *
     * Returns the current scheduling behavior option that determines how the task's
     * interval and iterations are managed during execution.
     *
     * Available scheduling options:
     * - TASK_SCHEDULE: Default behavior - interval countdown starts after OnEnable
     * - TASK_SCHEDULE_NC: No countdown - interval countdown starts immediately upon enable
     * - TASK_INTERVAL: Interval mode - maintains exact intervals regardless of execution time
     *
     * @return unsigned int Current scheduling option value (TASK_SCHEDULE, TASK_SCHEDULE_NC, or TASK_INTERVAL)
     *
     * @note This method is only available when _TASK_SCHEDULING_OPTIONS compile-time option is enabled
     * @see setSchedulingOption(), TASK_SCHEDULE, TASK_SCHEDULE_NC, TASK_INTERVAL
     * @since Version 3.0.0
     */
    __TASK_INLINE unsigned int getSchedulingOption() { return iOption; }

    /**
     * @brief Set the scheduling option for this task
     *
     * Sets the scheduling behavior option that determines how the task's interval
     * and iterations are managed during execution. This affects when the interval
     * countdown begins and how timing is calculated.
     *
     * Scheduling options:
     * - TASK_SCHEDULE: (Default) Interval countdown starts after OnEnable callback completes
     * - TASK_SCHEDULE_NC: No countdown - interval countdown starts immediately when task is enabled
     * - TASK_INTERVAL: Interval mode - maintains exact intervals by accounting for execution time
     *
     * @param aOption The scheduling option to set (TASK_SCHEDULE, TASK_SCHEDULE_NC, or TASK_INTERVAL)
     *
     * @note This method is only available when _TASK_SCHEDULING_OPTIONS compile-time option is enabled
     * @note The scheduling option can be changed at any time, even when the task is running
     * @warning Using TASK_INTERVAL may cause timing drift if task execution time exceeds the interval
     * @see getSchedulingOption(), TASK_SCHEDULE, TASK_SCHEDULE_NC, TASK_INTERVAL
     * @since Version 3.0.0
     */
    __TASK_INLINE void setSchedulingOption(unsigned int aOption) {  iOption = aOption; }
#endif  //_TASK_SCHEDULING_OPTIONS

#ifdef _TASK_OO_CALLBACKS
    /**
     * @brief Set task parameters (Object-Oriented callbacks)
     * @param aInterval Execution interval in milliseconds (or microseconds)
     * @param aIterations Number of iterations (-1 for infinite)
     * @details Allows dynamic control of task execution parameters in one method call.
     */
    __TASK_INLINE void set(unsigned long aInterval, long aIterations);
#else
    /**
     * @brief Set task parameters (Function pointer callbacks)
     * @param aInterval Execution interval in milliseconds (or microseconds)
     * @param aIterations Number of iterations (-1 for infinite)
     * @param aCallback Pointer to callback function
     * @param aOnEnable Pointer to OnEnable callback (default = NULL)
     * @param aOnDisable Pointer to OnDisable callback (default = NULL)
     * @details Allows dynamic control of task execution parameters in one method call.
     * OnEnable and OnDisable parameters can be omitted (assigned to NULL).
     */
    __TASK_INLINE void set(unsigned long aInterval, long aIterations, TaskCallback aCallback,TaskOnEnable aOnEnable=NULL, TaskOnDisable aOnDisable=NULL);
#endif // _TASK_OO_CALLBACKS

    /**
     * @brief Set task execution interval
     * @param aInterval New interval in milliseconds (or microseconds)
     * @details Next execution time calculation takes place after the callback method is called,
     * so new interval will be used immediately by the scheduler. Execution is delayed by the
     * provided interval. If immediate invocation is required, call forceNextIteration() after
     * setting new interval or use setIntervalNoDelay() method.
     */
    __TASK_INLINE void setInterval(unsigned long aInterval);

    /**
     * @brief Set task interval without delay
     * @param aInterval New interval in milliseconds (or microseconds)
     * @param aOption How to handle the interval change (default = TASK_INTERVAL_KEEP)
     * TASK_INTERVAL_KEEP - keep the current delay, new interval will be used after current delay expires
     * TASK_INTERVAL_RECALC - recalculate next execution time based on new interval
     * TASK_INTERVAL_RESET - reset schedule, next execution time and new interval are updated
     * @details Sets new interval and provides options for how to handle the timing change.
     */
    __TASK_INLINE void setIntervalNodelay(unsigned long aInterval, unsigned int aOption = TASK_INTERVAL_KEEP);

    /**
     * @brief Get current task interval
     * @return Current execution interval in milliseconds (or microseconds)
     */
    __TASK_INLINE unsigned long getInterval();

    /**
     * @brief Set number of iterations
     * @param aIterations Number of iterations (-1 for infinite)
     * @details Tasks that ran through all their allocated iterations are disabled.
     * SetIterations() method DOES NOT enable the task. Either enable explicitly,
     * or use restart methods.
     */
    __TASK_INLINE void setIterations(long aIterations);

    /**
     * @brief Get remaining iterations
     * @return Number of remaining iterations
     */
    __TASK_INLINE long getIterations();

    /**
     * @brief Get run counter
     * @return Number of times callback method has been invoked since last enable
     * @details The runCounter value is incremented before callback method is invoked.
     * If a task is checking the runCounter value within its callback method, then
     * the first run value is 1.
     */
    __TASK_INLINE unsigned long getRunCounter();

#ifdef _TASK_SELF_DESTRUCT
    /**
     * @brief Enable or disable self-destruction for this task
     *
     * When self-destruction is enabled, the task will automatically delete itself
     * from the scheduler after it completes its final iteration. This is useful
     * for one-time or temporary tasks that should clean themselves up automatically.
     *
     * Self-destruction behavior:
     * - Task removes itself from the scheduler when it finishes its last iteration
     * - Applies to tasks with finite iterations (setIterations() > 0)
     * - Does not apply to infinite tasks (TASK_FOREVER iterations)
     * - Task object memory is not freed - only removed from scheduler
     *
     * @param aSelfDestruct True to enable self-destruction, false to disable (default: true)
     *
     * @note This method is only available when _TASK_SELF_DESTRUCT compile-time option is enabled
     * @note Self-destruction only occurs after the task naturally completes its iterations
     * @note Manually disabled tasks will not self-destruct
     * @warning Ensure task object remains valid until self-destruction occurs
     * @see getSelfDestruct(), setIterations(), disable()
     * @since Version 3.0.0
     */
    __TASK_INLINE void setSelfDestruct(bool aSelfDestruct=true) { iStatus.selfdestruct = aSelfDestruct; }

    /**
     * @brief Check if self-destruction is enabled for this task
     *
     * Returns whether the task is configured to automatically remove itself from
     * the scheduler after completing its final iteration.
     *
     * @return bool True if self-destruction is enabled, false otherwise
     *
     * @note This method is only available when _TASK_SELF_DESTRUCT compile-time option is enabled
     * @see setSelfDestruct(), setIterations()
     * @since Version 3.0.0
     */
    __TASK_INLINE bool getSelfDestruct() { return iStatus.selfdestruct; }
#endif  //  #ifdef _TASK_SELF_DESTRUCT

#ifdef _TASK_OO_CALLBACKS
    /**
     * @brief Pure virtual main task execution callback (Object-Oriented mode)
     *
     * This pure virtual method must be implemented by derived classes to define
     * the task's main execution logic. Called by the scheduler when the task
     * is ready to execute based on its timing and iteration settings.
     *
     * @return bool True if the execution was "productive" (performed meaningful work),
     *              false if the task was idle. A productive return value prevents
     *              the scheduler from entering sleep mode on the next idle cycle,
     *              optimizing power management for active tasks.
     *
     * @note Only available when _TASK_OO_CALLBACKS compile-time option is enabled
     * @note This is a pure virtual function - derived classes MUST implement it
     * @note Return value affects power management in _TASK_SLEEP_ON_IDLE_RUN mode
     * @see _TASK_OO_CALLBACKS, _TASK_SLEEP_ON_IDLE_RUN
     * @since Version 3.0.0
     *
     * @par Example:
     * @code
     * class MyTask : public Task {
     * public:
     *     bool Callback() override {
     *         // Perform task work
     *         Serial.println("Task executing");
     *         return true; // Productive execution
     *     }
     * };
     * @endcode
     */
    virtual __TASK_INLINE bool Callback() =0;

    /**
     * @brief Virtual task enable lifecycle callback (Object-Oriented mode)
     *
     * Called when the task transitions from disabled to enabled state. Allows
     * derived classes to implement custom initialization logic, resource allocation,
     * or conditional enabling based on system state.
     *
     * @return bool True to allow the task to be enabled, false to prevent enabling.
     *              Returning false keeps the task disabled despite the enable request.
     *
     * @note Only available when _TASK_OO_CALLBACKS compile-time option is enabled
     * @note Default implementation returns true (allow enabling)
     * @note Called synchronously during enable() operation
     * @note Conditional enabling useful for resource-dependent tasks
     * @see _TASK_OO_CALLBACKS, enable(), OnDisable()
     * @since Version 3.0.0
     *
     * @par Example:
     * @code
     * class ConditionalTask : public Task {
     * public:
     *     bool OnEnable() override {
     *         if (systemReady()) {
     *             initializeResources();
     *             return true; // Allow enabling
     *         }
     *         return false; // Prevent enabling
     *     }
     * };
     * @endcode
     */
    virtual __TASK_INLINE bool OnEnable();

    /**
     * @brief Virtual task disable lifecycle callback (Object-Oriented mode)
     *
     * Called when the task transitions from enabled to disabled state. Allows
     * derived classes to implement custom cleanup logic, resource deallocation,
     * or state preservation when the task is deactivated.
     *
     * @note Only available when _TASK_OO_CALLBACKS compile-time option is enabled
     * @note Called synchronously during disable() operation
     * @note Called during automatic disable (iteration completion, errors)
     * @note Default implementation performs no action
     * @see _TASK_OO_CALLBACKS, disable(), OnEnable()
     * @since Version 3.0.0
     *
     * @par Example:
     * @code
     * class ResourceTask : public Task {
     * public:
     *     void OnDisable() override {
     *         cleanupResources();
     *         saveState();
     *         Serial.println("Task disabled");
     *     }
     * };
     * @endcode
     */
    virtual __TASK_INLINE void OnDisable();
#else
    /**
     * @brief Set the main task execution callback function
     *
     * Assigns a new callback function that will be executed when the task runs.
     * The callback function defines the task's main execution logic and is called
     * by the scheduler when the task is ready to execute.
     *
     * @param aCallback Function pointer to the new callback function.
     *                  Must have signature: void callbackFunction()
     *
     * @note Only available when _TASK_OO_CALLBACKS is NOT enabled (function pointer mode)
     * @note Callback can be changed dynamically during task execution
     * @note Setting nullptr callback is allowed but will result in no-op execution
     * @see TaskCallback, yield(), yieldOnce()
     * @since Version 1.0.0
     *
     * @par Example:
     * @code
     * void task1Function() {
     *     Serial.println("Task 1 executing");
     * }
     *
     * void task2Function() {
     *     Serial.println("Task 2 executing");
     * }
     *
     * Task myTask(1000, TASK_FOREVER, task1Function, &scheduler);
     * myTask.setCallback(task2Function); // Switch to different function
     * @endcode
     */
    __TASK_INLINE void setCallback(TaskCallback aCallback) ;

    /**
     * @brief Set the task enable lifecycle callback function
     *
     * Assigns a callback function that will be called when the task transitions
     * from disabled to enabled state. Enables custom initialization logic and
     * conditional enabling based on system conditions.
     *
     * @param aCallback Function pointer to the enable callback function.
     *                  Must have signature: bool onEnableFunction()
     *                  Return true to allow enabling, false to prevent it.
     *
     * @note Only available when _TASK_OO_CALLBACKS is NOT enabled (function pointer mode)
     * @note Called synchronously during enable() operation
     * @note Returning false from callback prevents task enabling
     * @note Setting nullptr removes the enable callback
     * @see TaskOnEnable, setOnDisable(), enable()
     * @since Version 2.0.0
     *
     * @par Example:
     * @code
     * bool checkSystemReady() {
     *     return digitalRead(READY_PIN) == HIGH;
     * }
     *
     * Task myTask(1000, 10, mainCallback, &scheduler);
     * myTask.setOnEnable(checkSystemReady); // Conditional enabling
     * @endcode
     */
    __TASK_INLINE void setOnEnable(TaskOnEnable aCallback) ;

    /**
     * @brief Set the task disable lifecycle callback function
     *
     * Assigns a callback function that will be called when the task transitions
     * from enabled to disabled state. Enables custom cleanup logic and resource
     * management when the task is deactivated.
     *
     * @param aCallback Function pointer to the disable callback function.
     *                  Must have signature: void onDisableFunction()
     *
     * @note Only available when _TASK_OO_CALLBACKS is NOT enabled (function pointer mode)
     * @note Called synchronously during disable() operation
     * @note Called during automatic disable (iteration completion, errors)
     * @note Setting nullptr removes the disable callback
     * @see TaskOnDisable, setOnEnable(), disable()
     * @since Version 2.0.0
     *
     * @par Example:
     * @code
     * void cleanupTask() {
     *     Serial.println("Task cleaning up");
     *     // Release resources, save state, etc.
     * }
     *
     * Task myTask(1000, 5, mainCallback, &scheduler);
     * myTask.setOnDisable(cleanupTask); // Cleanup on disable
     * @endcode
     */
    __TASK_INLINE void setOnDisable(TaskOnDisable aCallback) ;

    /**
     * @brief Permanently switch to a new callback function
     *
     * Changes the task's callback function to the specified new function for all
     * subsequent executions. This enables state machine behavior and multi-phase
     * task processing within a single task object.
     *
     * @param aCallback Function pointer to the new callback function.
     *                  Must have signature: void newCallbackFunction()
     *
     * @note Only available when _TASK_OO_CALLBACKS is NOT enabled (function pointer mode)
     * @note Change takes effect immediately and is permanent
     * @note All future executions will use the new callback
     * @note Different from yieldOnce() which executes new callback only once
     * @see yieldOnce(), setCallback(), TaskCallback
     * @since Version 2.0.0
     *
     * @par Example:
     * @code
     * void phase1Callback() {
     *     Serial.println("Phase 1");
     *     // Switch to phase 2 after some condition
     *     if (conditionMet()) {
     *         myTask.yield(phase2Callback);
     *     }
     * }
     *
     * void phase2Callback() {
     *     Serial.println("Phase 2");
     * }
     * @endcode
     */
    __TASK_INLINE void yield(TaskCallback aCallback);

    /**
     * @brief Execute a callback function once, then disable the task
     *
     * Switches to the specified callback function for exactly one execution,
     * then automatically disables the task. Useful for one-time completion
     * logic, cleanup operations, or finalization steps.
     *
     * @param aCallback Function pointer to the callback function to execute once.
     *                  Must have signature: void onceCallbackFunction()
     *
     * @note Only available when _TASK_OO_CALLBACKS is NOT enabled (function pointer mode)
     * @note Task executes the new callback exactly once on next scheduler pass
     * @note Task automatically disables after the single execution
     * @note Original callback is not restored (task becomes inactive)
     * @note Useful for completion handlers and one-shot operations
     * @see yield(), setCallback(), TaskCallback
     * @since Version 2.0.0
     *
     * @par Example:
     * @code
     * void mainCallback() {
     *     static int count = 0;
     *     Serial.print("Main execution: ");
     *     Serial.println(++count);
     *
     *     if (count >= 5) {
     *         myTask.yieldOnce(finalizationCallback);
     *     }
     * }
     *
     * void finalizationCallback() {
     *     Serial.println("Task completed - final cleanup");
     *     // Task will automatically disable after this
     * }
     * @endcode
     */
    __TASK_INLINE void yieldOnce(TaskCallback aCallback);
#endif // _TASK_OO_CALLBACKS

    /**
     * @brief Check if this is the first iteration
     * @return true if current pass is (or will be) a first iteration of the task
     */
    __TASK_INLINE bool isFirstIteration() ;

    /**
     * @brief Check if this is the last iteration
     * @return true if current pass is the last iteration (for tasks with limited iterations only)
     */
    __TASK_INLINE bool isLastIteration() ;

#ifdef _TASK_TIMECRITICAL
    /**
     * @brief Get the execution overrun time in microseconds
     *
     * Returns the amount of time (in microseconds) that the task's execution
     * exceeded its scheduled interval. This measures how much longer the task
     * took to complete compared to its allocated time budget, helping identify
     * timing violations and performance bottlenecks.
     *
     * @return long Overrun time in microseconds. Positive values indicate the task
     *              ran longer than its interval, negative values indicate it finished
     *              early. Zero indicates the task completed exactly on schedule.
     *
     * @note Only available when _TASK_TIMECRITICAL compile-time option is enabled
     * @note Measured from scheduled execution time to actual completion time
     * @note Useful for real-time system analysis and performance optimization
     * @see getStartDelay(), _TASK_TIMECRITICAL
     * @since Version 3.0.0
     *
     * @par Example:
     * @code
     * void criticalTaskCallback() {
     *     // Perform time-critical work
     *     performCriticalOperation();
     *
     *     // Check if we exceeded our time budget
     *     long overrun = myTask.getOverrun();
     *     if (overrun > 1000) { // More than 1ms overrun
     *         Serial.print("Warning: Task overrun by ");
     *         Serial.print(overrun);
     *         Serial.println(" microseconds");
     *     }
     * }
     * @endcode
     */
    __TASK_INLINE long getOverrun() ;

    /**
     * @brief Get the task start delay in microseconds
     *
     * Returns the amount of time (in microseconds) between when the task was
     * scheduled to start and when it actually began execution. This measures
     * scheduler latency and helps identify system timing accuracy and load issues.
     *
     * @return long Start delay in microseconds. Positive values indicate the task
     *              started later than scheduled, negative values indicate it started
     *              early. Zero indicates perfect timing accuracy.
     *
     * @note Only available when _TASK_TIMECRITICAL compile-time option is enabled
     * @note Measured from scheduled start time to actual execution start time
     * @note High values may indicate system overload or scheduler inefficiency
     * @see getOverrun(), _TASK_TIMECRITICAL
     * @since Version 3.0.0
     *
     * @par Example:
     * @code
     * void timingAnalysisCallback() {
     *     long startDelay = myTask.getStartDelay();
     *     long overrun = myTask.getOverrun();
     *
     *     Serial.print("Start delay: ");
     *     Serial.print(startDelay);
     *     Serial.print("us, Overrun: ");
     *     Serial.print(overrun);
     *     Serial.println("us");
     *
     *     if (startDelay > 500) {
     *         Serial.println("Warning: High scheduler latency detected");
     *     }
     * }
     * @endcode
     */
    __TASK_INLINE long getStartDelay() ;
#endif  // _TASK_TIMECRITICAL

#ifdef _TASK_STATUS_REQUEST
    /**
     * @brief Wait for a StatusRequest to be signaled, then execute with new parameters
     *
     * Suspends this task until the specified StatusRequest is signaled by another task,
     * then reconfigures this task with new interval and iterations before resuming.
     * Enables sophisticated inter-task communication and coordination patterns.
     *
     * @param aStatusRequest Pointer to StatusRequest object to wait for
     * @param aInterval New execution interval in milliseconds (default: 0 = keep current)
     * @param aIterations New iteration count (default: 1 = single execution)
     *
     * @return bool True if wait was successful and task was reconfigured,
     *              false if StatusRequest is invalid or operation failed
     *
     * @note Only available when _TASK_STATUS_REQUEST compile-time option is enabled
     * @note Task becomes inactive until StatusRequest is signaled
     * @note StatusRequest must be signaled by another task's completion
     * @note Interval of 0 keeps current task interval unchanged
     * @see waitForDelayed(), getStatusRequest(), StatusRequest
     * @since Version 3.0.0
     *
     * @par Example:
     * @code
     * Task task1(1000, TASK_FOREVER, task1Callback, &scheduler);
     * Task task2(500, 1, task2Callback, &scheduler);
     *
     * void task1Callback() {
     *     // Wait for task2 to complete, then run every 200ms for 5 iterations
     *     if (task1.waitFor(task2.getStatusRequest(), 200, 5)) {
     *         Serial.println("Task1 resumed after task2 completion");
     *     }
     * }
     * @endcode
     */
    __TASK_INLINE bool waitFor(StatusRequest* aStatusRequest, unsigned long aInterval = 0, long aIterations = 1);

    /**
     * @brief Wait for a StatusRequest with delay, then execute with new parameters
     *
     * Similar to waitFor() but introduces an additional delay before the task
     * becomes eligible for execution after the StatusRequest is signaled.
     * Useful for staggered execution and preventing simultaneous task activation.
     *
     * @param aStatusRequest Pointer to StatusRequest object to wait for
     * @param aInterval New execution interval in milliseconds (default: 0 = keep current)
     * @param aIterations New iteration count (default: 1 = single execution)
     *
     * @return bool True if wait was successful and task was reconfigured,
     *              false if StatusRequest is invalid or operation failed
     *
     * @note Only available when _TASK_STATUS_REQUEST compile-time option is enabled
     * @note Adds scheduling delay after StatusRequest is signaled
     * @note Prevents immediate execution, allowing for staggered task activation
     * @note Interval of 0 keeps current task interval unchanged
     * @see waitFor(), getStatusRequest(), StatusRequest
     * @since Version 3.0.0
     *
     * @par Example:
     * @code
     * void coordinatorCallback() {
     *     // Wait for sensor task, then start processing with 1 second delay
     *     if (processingTask.waitForDelayed(sensorTask.getStatusRequest(), 1000, 10)) {
     *         Serial.println("Processing will start 1 second after sensor completion");
     *     }
     * }
     * @endcode
     */
    __TASK_INLINE bool waitForDelayed(StatusRequest* aStatusRequest, unsigned long aInterval = 0, long aIterations = 1);

    /**
     * @brief Get the external StatusRequest object for this task
     *
     * Returns a pointer to the StatusRequest object that other tasks can use
     * to wait for this task's completion. This enables the creation of task
     * dependency chains and sophisticated coordination patterns.
     *
     * @return StatusRequest* Pointer to the task's external StatusRequest object,
     *                        or nullptr if StatusRequest functionality is not available
     *
     * @note Only available when _TASK_STATUS_REQUEST compile-time option is enabled
     * @note External StatusRequest is signaled when task completes its iterations
     * @note Used by other tasks in waitFor() and waitForDelayed() calls
     * @note Different from getInternalStatusRequest() which is used for internal coordination
     * @see getInternalStatusRequest(), waitFor(), StatusRequest
     * @since Version 3.0.0
     *
     * @par Example:
     * @code
     * Task producerTask(1000, 5, producerCallback, &scheduler);
     * Task consumerTask(100, 1, consumerCallback, &scheduler);
     *
     * void setupTasks() {
     *     // Consumer waits for producer to complete
     *     StatusRequest* producerStatus = producerTask.getStatusRequest();
     *     consumerTask.waitFor(producerStatus, 500, TASK_FOREVER);
     * }
     * @endcode
     */
    __TASK_INLINE StatusRequest* getStatusRequest() ;

    /**
     * @brief Get the internal StatusRequest object for this task
     *
     * Returns a pointer to the internal StatusRequest object used for advanced
     * task coordination patterns within the same task or tightly coupled task groups.
     * Provides finer control over task signaling and coordination mechanisms.
     *
     * @return StatusRequest* Pointer to the task's internal StatusRequest object,
     *                        or nullptr if StatusRequest functionality is not available
     *
     * @note Only available when _TASK_STATUS_REQUEST compile-time option is enabled
     * @note Internal StatusRequest provides advanced coordination capabilities
     * @note Used for complex task orchestration and state machine patterns
     * @note Separate from external StatusRequest for different coordination levels
     * @see getStatusRequest(), waitFor(), StatusRequest
     * @since Version 3.0.0
     *
     * @par Example:
     * @code
     * void complexCoordinationCallback() {
     *     StatusRequest* internalStatus = myTask.getInternalStatusRequest();
     *     // Use internal status for advanced coordination
     *     otherTask.waitForDelayed(internalStatus, 100, 1);
     * }
     * @endcode
     */
    __TASK_INLINE StatusRequest* getInternalStatusRequest() ;
#endif  // _TASK_STATUS_REQUEST

#ifdef _TASK_WDT_IDS
    /**
     * @brief Set the watchdog timer ID for this task
     *
     * Assigns a unique identifier to the task for watchdog timer monitoring.
     * The ID is used by external watchdog systems to track task execution
     * and detect hung or malfunctioning tasks in mission-critical applications.
     *
     * @param aID Unique identifier for watchdog monitoring (typically 0-255 range)
     *
     * @note Only available when _TASK_WDT_IDS compile-time option is enabled
     * @note ID should be unique across all tasks in the system
     * @note Used in conjunction with external watchdog hardware/software
     * @note Essential for safety-critical and mission-critical applications
     * @see getId(), setControlPoint(), _TASK_WDT_IDS
     * @since Version 3.0.0
     *
     * @par Example:
     * @code
     * Task criticalTask(1000, TASK_FOREVER, criticalCallback, &scheduler);
     * Task monitorTask(5000, TASK_FOREVER, monitorCallback, &scheduler);
     *
     * void setup() {
     *     criticalTask.setId(1);  // Critical system task
     *     monitorTask.setId(2);   // System monitor task
     * }
     *
     * void criticalCallback() {
     *     // Reset watchdog for this task ID
     *     resetWatchdog(criticalTask.getId());
     * }
     * @endcode
     */
    __TASK_INLINE void setId(unsigned int aID) ;

    /**
     * @brief Get the watchdog timer ID for this task
     *
     * Returns the unique identifier assigned to this task for watchdog monitoring.
     * Used by watchdog systems and monitoring code to identify which task is
     * currently executing or has failed to respond.
     *
     * @return unsigned int The task's watchdog ID, or 0 if no ID has been set
     *
     * @note Only available when _TASK_WDT_IDS compile-time option is enabled
     * @note Returns the ID set by setId(), or 0 for uninitialized tasks
     * @note Used for watchdog reset operations and task identification
     * @see setId(), getControlPoint(), _TASK_WDT_IDS
     * @since Version 3.0.0
     *
     * @par Example:
     * @code
     * void watchdogResetCallback() {
     *     unsigned int taskId = myTask.getId();
     *     if (taskId > 0) {
     *         // Reset watchdog for this specific task
     *         resetWatchdogTimer(taskId);
     *         Serial.print("Watchdog reset for task ID: ");
     *         Serial.println(taskId);
     *     }
     * }
     * @endcode
     */
    __TASK_INLINE unsigned int getId() ;

    /**
     * @brief Set the control point identifier for this task
     *
     * Assigns a control point identifier used for fine-grained execution monitoring
     * and debugging. Control points can mark specific execution phases or critical
     * sections within task callbacks for detailed system analysis.
     *
     * @param aPoint Control point identifier (application-defined meaning)
     *
     * @note Only available when _TASK_WDT_IDS compile-time option is enabled
     * @note Control points provide execution phase tracking within tasks
     * @note Used for debugging, profiling, and fine-grained monitoring
     * @note Meaning and usage are application-specific
     * @see getControlPoint(), setId(), _TASK_WDT_IDS
     * @since Version 3.0.0
     *
     * @par Example:
     * @code
     * void complexTaskCallback() {
     *     myTask.setControlPoint(1);  // Phase 1: Sensor reading
     *     readSensors();
     *
     *     myTask.setControlPoint(2);  // Phase 2: Data processing
     *     processData();
     *
     *     myTask.setControlPoint(3);  // Phase 3: Output generation
     *     generateOutput();
     *
     *     myTask.setControlPoint(0);  // Phase complete
     * }
     * @endcode
     */
    __TASK_INLINE void setControlPoint(unsigned int aPoint) ;

    /**
     * @brief Get the current control point identifier for this task
     *
     * Returns the current control point identifier, indicating which execution
     * phase or critical section the task is currently in or last executed.
     * Useful for debugging hung tasks and execution flow analysis.
     *
     * @return unsigned int Current control point identifier, or 0 if none set
     *
     * @note Only available when _TASK_WDT_IDS compile-time option is enabled
     * @note Returns the most recent control point set by setControlPoint()
     * @note Used for debugging and execution phase identification
     * @note Can help identify where tasks hang or fail
     * @see setControlPoint(), getId(), _TASK_WDT_IDS
     * @since Version 3.0.0
     *
     * @par Example:
     * @code
     * void monitorTaskCallback() {
     *     unsigned int controlPoint = criticalTask.getControlPoint();
     *     unsigned long lastRun = criticalTask.getRunCounter();
     *
     *     if (lastRun == previousRunCount && controlPoint != 0) {
     *         Serial.print("Critical task hung at control point: ");
     *         Serial.println(controlPoint);
     *         // Take corrective action
     *     }
     *     previousRunCount = lastRun;
     * }
     * @endcode
     */
    __TASK_INLINE unsigned int getControlPoint() ;
#endif  // _TASK_WDT_IDS

#ifdef _TASK_LTS_POINTER
    /**
     * @brief Set the Local Task Storage (LTS) pointer for this task
     *
     * Assigns a pointer to task-specific data storage that persists across
     * task executions. This enables tasks to maintain state, store working
     * data, or reference external resources without using global variables.
     *
     * @param aPtr Pointer to task-local storage area (any data type)
     *
     * @note Only available when _TASK_LTS_POINTER compile-time option is enabled
     * @note Pointer can reference any data structure or object
     * @note Memory management is application responsibility
     * @note Enables object-oriented patterns and data encapsulation
     * @see getLtsPointer(), _TASK_LTS_POINTER
     * @since Version 3.0.0
     *
     * @par Example:
     * @code
     * struct TaskData {
     *     int counter;
     *     float average;
     *     bool initialized;
     * };
     *
     * TaskData myTaskData = {0, 0.0, false};
     * Task dataTask(1000, TASK_FOREVER, dataCallback, &scheduler);
     *
     * void setup() {
     *     dataTask.setLtsPointer(&myTaskData);
     * }
     *
     * void dataCallback() {
     *     TaskData* data = (TaskData*)dataTask.getLtsPointer();
     *     if (data) {
     *         data->counter++;
     *         // Use task-specific data
     *     }
     * }
     * @endcode
     */
    __TASK_INLINE void  setLtsPointer(void *aPtr) ;

    /**
     * @brief Get the Local Task Storage (LTS) pointer for this task
     *
     * Returns the pointer to task-specific data storage previously set with
     * setLtsPointer(). Enables tasks to access their persistent data without
     * relying on global variables or external storage mechanisms.
     *
     * @return void* Pointer to task-local storage, or nullptr if none set
     *
     * @note Only available when _TASK_LTS_POINTER compile-time option is enabled
     * @note Returned pointer must be cast to appropriate data type
     * @note Returns nullptr if no LTS pointer has been set
     * @note Enables clean data encapsulation and object-oriented patterns
     * @see setLtsPointer(), _TASK_LTS_POINTER
     * @since Version 3.0.0
     *
     * @par Example:
     * @code
     * struct SensorData {
     *     float temperature;
     *     float humidity;
     *     unsigned long lastReading;
     * };
     *
     * void sensorCallback() {
     *     SensorData* data = (SensorData*)sensorTask.getLtsPointer();
     *     if (data) {
     *         data->temperature = readTemperature();
     *         data->humidity = readHumidity();
     *         data->lastReading = millis();
     *
     *         Serial.print("Temp: ");
     *         Serial.print(data->temperature);
     *         Serial.print("°C, Humidity: ");
     *         Serial.print(data->humidity);
     *         Serial.println("%");
     *     }
     * }
     * @endcode
     */
    __TASK_INLINE void* getLtsPointer() ;
#endif  // _TASK_LTS_POINTER

#ifdef _TASK_EXPOSE_CHAIN
    /**
     * @brief Get pointer to the previous task in the scheduler's task chain
     *
     * Returns a pointer to the task that precedes this task in the scheduler's
     * internal linked list. This enables task chain traversal, debugging, and
     * advanced task management operations for custom scheduler behaviors.
     *
     * @return Task* Pointer to the previous task in the chain, or nullptr if:
     *               - This is the first task in the chain
     *               - Task is not currently registered with a scheduler
     *               - Task chain is empty
     *
     * @note Only available when _TASK_EXPOSE_CHAIN compile-time option is enabled
     * @note Exposes internal scheduler data structure for advanced operations
     * @note Chain order may not correlate with execution order (depends on timing)
     * @note Useful for debugging, custom algorithms, and chain analysis
     * @warning Direct chain manipulation can corrupt scheduler state
     * @see getNextTask(), _TASK_EXPOSE_CHAIN
     * @since Version 3.0.0
     *
     * @par Example:
     * @code
     * void analyzeTaskChain(Task& startTask) {
     *     Task* current = &startTask;
     *     int position = 0;
     *
     *     // Walk backwards through the chain
     *     while (current->getPreviousTask() != nullptr) {
     *         current = current->getPreviousTask();
     *         position++;
     *     }
     *
     *     Serial.print("Task is at position ");
     *     Serial.print(position);
     *     Serial.println(" from chain start");
     * }
     *
     * void debugTaskChain(Scheduler& scheduler) {
     *     // Find chain relationships for debugging
     *     Serial.println("Task Chain Analysis:");
     *     // Implementation would traverse and analyze the chain
     * }
     * @endcode
     */
    __TASK_INLINE Task*  getPreviousTask() { return iPrev; };

    /**
     * @brief Get pointer to the next task in the scheduler's task chain
     *
     * Returns a pointer to the task that follows this task in the scheduler's
     * internal linked list. This enables forward chain traversal, task enumeration,
     * and advanced scheduler analysis for custom task management operations.
     *
     * @return Task* Pointer to the next task in the chain, or nullptr if:
     *               - This is the last task in the chain
     *               - Task is not currently registered with a scheduler
     *               - Task chain is empty
     *
     * @note Only available when _TASK_EXPOSE_CHAIN compile-time option is enabled
     * @note Exposes internal scheduler data structure for advanced operations
     * @note Chain order is registration order, not execution priority
     * @note Useful for iteration, monitoring, and custom scheduler algorithms
     * @warning Direct chain manipulation can corrupt scheduler state
     * @see getPreviousTask(), _TASK_EXPOSE_CHAIN
     * @since Version 3.0.0
     *
     * @par Example:
     * @code
     * void printAllTasks(Task& firstTask) {
     *     Task* current = &firstTask;
     *     int taskCount = 0;
     *
     *     Serial.println("=== Task Chain Report ===");
     *     while (current != nullptr) {
     *         taskCount++;
     *         Serial.print("Task ");
     *         Serial.print(taskCount);
     *         Serial.print(": Enabled=");
     *         Serial.print(current->isEnabled() ? "YES" : "NO");
     *         Serial.print(", Interval=");
     *         Serial.print(current->getInterval());
     *         Serial.print("ms, Iterations=");
     *         Serial.println(current->getIterations());
     *
     *         current = current->getNextTask();
     *     }
     *     Serial.print("Total tasks: ");
     *     Serial.println(taskCount);
     * }
     *
     * int countEnabledTasks(Task& anyTask) {
     *     // Find start of chain
     *     Task* start = &anyTask;
     *     while (start->getPreviousTask() != nullptr) {
     *         start = start->getPreviousTask();
     *     }
     *
     *     // Count enabled tasks
     *     int enabledCount = 0;
     *     Task* current = start;
     *     while (current != nullptr) {
     *         if (current->isEnabled()) {
     *             enabledCount++;
     *         }
     *         current = current->getNextTask();
     *     }
     *     return enabledCount;
     * }
     * @endcode
     */
    __TASK_INLINE Task*  getNextTask()     { return iNext; };
#endif // _TASK_EXPOSE_CHAIN

  _TASK_SCOPE:
    __TASK_INLINE void reset();

    volatile _task_status     iStatus;
    volatile unsigned long    iInterval;             // execution interval in milliseconds (or microseconds). 0 - immediate
    volatile unsigned long    iDelay;                // actual delay until next execution (usually equal iInterval)
    volatile unsigned long    iPreviousMillis;       // previous invocation time (millis).  Next invocation = iPreviousMillis + iInterval.  Delayed tasks will "catch up"

#ifdef _TASK_SCHEDULING_OPTIONS
    unsigned int              iOption;               // scheduling option
#endif  // _TASK_SCHEDULING_OPTIONS

#ifdef _TASK_TIMECRITICAL
    volatile long             iOverrun;              // negative if task is "catching up" to it's schedule (next invocation time is already in the past)
    volatile long             iStartDelay;           // actual execution of the task's callback method was delayed by this number of millis
#endif  // _TASK_TIMECRITICAL

    volatile long             iIterations;           // number of iterations left. 0 - last iteration. -1 - infinite iterations
    long                      iSetIterations;        // number of iterations originally requested (for restarts)
    unsigned long             iRunCounter;           // current number of iteration (starting with 1). Resets on enable.

#ifndef _TASK_OO_CALLBACKS
    TaskCallback              iCallback;             // pointer to the void callback method
    TaskOnEnable              iOnEnable;             // pointer to the bool OnEnable callback method
    TaskOnDisable             iOnDisable;            // pointer to the void OnDisable method
#endif // _TASK_OO_CALLBACKS

    Task                     *iPrev, *iNext;         // pointers to the previous and next tasks in the chain
    Scheduler                *iScheduler;            // pointer to the current scheduler

#ifdef _TASK_STATUS_REQUEST
    StatusRequest            *iStatusRequest;        // pointer to the status request task is or was waiting on
    StatusRequest             iMyStatusRequest;      // internal Status request to let other tasks know of completion
#endif  // _TASK_STATUS_REQUEST

#ifdef _TASK_WDT_IDS
    unsigned int              iTaskID;               // task ID (for debugging and watchdog identification)
    unsigned int              iControlPoint;         // current control point within the callback method. Reset to 0 by scheduler at the beginning of each pass
#endif  // _TASK_WDT_IDS

#ifdef _TASK_LTS_POINTER
    void                     *iLTS;                  // pointer to task's local storage. Needs to be recast to appropriate type (usually a struct).
#endif  // _TASK_LTS_POINTER

#ifdef _TASK_TIMEOUT
    unsigned long            iTimeout;               // Task overall timeout
    unsigned long            iStarttime;             // millis at task start time
#endif // _TASK_TIMEOUT
};

/**
 * @class Scheduler
 * @brief Task scheduler for cooperative multitasking
 *
 * @details The Scheduler class manages the execution of tasks in a cooperative multitasking environment.
 * It maintains a chain of tasks and executes them according to their schedules and priorities.
 *
 * Key features:
 * - Cooperative multitasking (non-preemptive)
 * - Task chain management
 * - Priority layer support
 * - Sleep mode support for power saving
 * - Thread safety options
 * - Performance monitoring
 *
 * The scheduler executes Tasks' callback methods in the order the tasks were added to the chain,
 * from first to last. Scheduler stops and exits after processing the chain once in order to allow
 * other statements in the main code of loop() method to run. This is referred to as a "scheduling pass".
 *
 * @note Normally, there is no need to have any other statements in the loop() method other than
 * the Scheduler's execute() method.
 *
 * @see Task, StatusRequest
 */
class Scheduler {
  friend class Task;
  public:
    /**
     * @brief Default constructor
     * @details Creates task scheduler with default parameters and an empty task queue.
     */
    __TASK_INLINE Scheduler();

    /**
     * @brief Initialize the scheduler
     * @details Initializes the task queue and scheduler parameters. Executed as part of constructor,
     * so doesn't need to be explicitly called after creation.
     *
     * @note By default scheduler is allowed to put processor to IDLE sleep mode. If this behavior
     * was changed via allowSleep() method, init() will NOT reset the sleep parameter.
     */
    __TASK_INLINE void init();

    /**
     * @brief Add task to execution chain
     * @param aTask Reference to task to be added
     * @details Adds task to the execution queue (or chain) of tasks by appending it to the end
     * of the chain. If two tasks are scheduled for execution, the sequence will match the order
     * in which tasks were appended to the chain.
     *
     * @note Currently, changing the execution sequence in a chain dynamically is not supported.
     * If you need to reorder the chain sequence – initialize the scheduler and re-add the tasks
     * in a different order.
     */
    __TASK_INLINE void addTask(Task& aTask);

    /**
     * @brief Delete task from execution chain
     * @param aTask Reference to task to be deleted
     * @details Deletes task from the execution chain. The chain of remaining tasks is linked
     * together. It is not required to delete a task from the chain. A disabled task will not
     * be executed anyway, but you save a few microseconds per scheduling pass by deleting it.
     */
    __TASK_INLINE void deleteTask(Task& aTask);

    /**
     * @brief Pause the scheduler
     * @details Temporarily suspends all task execution. Tasks remain in their current state
     * but will not be executed until resume() is called.
     */
    __TASK_INLINE void pause() { iPaused = true; };

    /**
     * @brief Resume the scheduler
     * @details Resumes task execution after pause().
     */
    __TASK_INLINE void resume() { iPaused = false; };

    /**
     * @brief Enable the scheduler
     * @details Enables the scheduler for task execution.
     */
    __TASK_INLINE void enable() { iEnabled = true; };

    /**
     * @brief Disable the scheduler
     * @details Disables the scheduler, preventing any task execution.
     */
    __TASK_INLINE void disable() { iEnabled = false; };
#ifdef _TASK_PRIORITY
    /**
     * @brief Disable all tasks (with priority support)
     * @param aRecursive If true (default), disable higher priority tasks as well
     * @details Convenient method to disable majority of tasks.
     */
    __TASK_INLINE void disableAll(bool aRecursive = true);

    /**
     * @brief Enable all tasks (with priority support)
     * @param aRecursive If true (default), enable higher priority tasks as well
     * @details Convenient method to enable majority of tasks.
     */
    __TASK_INLINE void enableAll(bool aRecursive = true);

    /**
     * @brief Set all tasks to start immediately (with priority support)
     * @param aRecursive If true (default), affect higher priority tasks as well
     * @details Sets ALL active tasks to start execution immediately. Should be placed
     * at the end of setup() method to prevent task execution race due to long running
     * setup tasks. Any tasks which should execute after a delay should be explicitly
     * delayed after call to startNow() method.
     */
    __TASK_INLINE void startNow(bool aRecursive = true);
#else
    /**
     * @brief Disable all tasks
     * @details Convenient method to disable majority of tasks.
     */
    __TASK_INLINE void disableAll();

    /**
     * @brief Enable all tasks
     * @details Convenient method to enable majority of tasks.
     */
    __TASK_INLINE void enableAll();

    /**
     * @brief Set all tasks to start immediately
     * @details Sets ALL active tasks to start execution immediately. Should be placed
     * at the end of setup() method to prevent task execution race due to long running
     * setup tasks.
     */
    __TASK_INLINE void startNow();
#endif

    /**
     * @brief Execute one scheduling pass
     * @return true if none of the tasks' callback methods was invoked (true = idle run)
     * @details Executes one scheduling pass, including (in case of the base priority scheduler)
     * end-of-pass sleep. This method should be placed inside the loop() method of the sketch.
     * Since execute exits after every pass, you can put additional statements after execute
     * inside the loop().
     *
     * If layered task prioritization is enabled, all higher priority tasks will be evaluated
     * and invoked by the base execute() method. There is no need to call execute() of the
     * higher priority schedulers explicitly.
     *
     * The execute method performs the following steps:
     * 1. Call higher priority scheduler's execute method, if provided
     * 2. Ignore task completely if it is disabled
     * 3. Disable task if it ran out of iterations (calling OnDisable, if necessary)
     * 4. Check if task is waiting on a StatusRequest object, and make appropriate scheduling arrangements
     * 5. Perform necessary timing calculations
     * 6. Invoke task's callback method, if it is time to do so, and one is provided
     * 7. Put microcontroller to sleep (if requested and supported) if none of the tasks were invoked
     *
     * @note Schedule-related calculations are performed prior to task's callback method invocation.
     * This allows tasks to manipulate their runtime parameters (like execution interval) directly.
     */
    __TASK_INLINE bool execute();

    /**
     * @brief Get reference to currently executing task (deprecated)
     * @return Reference to the currently active task
     * @deprecated Use getCurrentTask() instead
     */
    __TASK_INLINE Task& currentTask() ;

    /**
     * @brief Get pointer to currently executing task
     * @return Pointer to the currently active task
     * @details Returns pointer to the task currently executing via execute() loop OR for
     * OnEnable and OnDisable methods, pointer to the task being enabled or disabled.
     * This distinction is important because one task can activate another.
     * Could be used by callback methods to identify which Task invoked this callback method.
     */
    __TASK_INLINE Task* getCurrentTask() ;

    /**
     * @brief Get time until next iteration of a task
     * @param aTask Reference to the task to query
     * @return Number of milliseconds (or microseconds) until next scheduled iteration
     * @details Returns 0 if next iteration is already due (or overdue).
     * Returns -1 if a Task is not active or waiting on an event, and next iteration
     * runtime cannot be determined.
     */
    __TASK_INLINE long timeUntilNextIteration(Task& aTask);

    /**
     * @brief Get number of active tasks
     * @return Number of currently active (enabled) tasks
     */
    __TASK_INLINE unsigned long getActiveTasks() { return iActiveTasks; }

    /**
     * @brief Get total number of tasks
     * @return Total number of tasks in the scheduler
     */
    __TASK_INLINE unsigned long getTotalTasks() { return iTotalTasks; }

    /**
     * @brief Get number of invoked tasks
     * @return Number of tasks that were invoked during the last execute() pass
     */
    __TASK_INLINE unsigned long getInvokedTasks() { return iInvokedTasks; }

    /**
     * @brief Get the number of tasks currently in the scheduler chain
     * @return Number of tasks added to this scheduler (enabled and disabled)
     * @details Unlike getTotalTasks() which is only updated during execute(),
     *          this method returns an accurate count at any time, including
     *          before the first execute() call.
     */
    __TASK_INLINE unsigned long getChainLength() { return iChainLength; }

#ifdef _TASK_TICKLESS
    /**
     * @brief Get next run time (tickless mode)
     * @return Next scheduled run time
     * @details Available only when compiled with _TASK_TICKLESS support.
     */
    __TASK_INLINE unsigned long getNextRun() { return iNextRun; }
#endif

#ifdef _TASK_SLEEP_ON_IDLE_RUN
    /**
     * @brief Enable or disable sleep mode during idle scheduler cycles
     *
     * Controls whether the scheduler should enter sleep mode when no tasks
     * are ready for execution. Sleep mode reduces power consumption by
     * halting CPU execution until the next task is scheduled to run.
     *
     * @param aState True to enable sleep mode (default), false to disable
     *
     * @note Only available when _TASK_SLEEP_ON_IDLE_RUN compile-time option is enabled
     * @note Sleep is only activated when all tasks return false from their callbacks
     * @note Tasks returning true (productive execution) prevent sleep on next cycle
     * @note Custom sleep method can be set with setSleepMethod()
     * @see setSleepMethod(), _TASK_SLEEP_ON_IDLE_RUN
     * @since Version 3.0.0
     *
     * @par Example:
     * @code
     * Scheduler powerEfficientScheduler;
     *
     * void setup() {
     *     // Enable power-saving sleep mode
     *     powerEfficientScheduler.allowSleep(true);
     *
     *     // Custom sleep implementation
     *     powerEfficientScheduler.setSleepMethod(customSleepCallback);
     * }
     *
     * void customSleepCallback(unsigned long aTimeToSleep) {
     *     if (aTimeToSleep > 1000) {
     *         // Deep sleep for longer periods
     *         ESP.deepSleep(aTimeToSleep * 1000); // Convert to microseconds
     *     } else {
     *         // Light sleep for short periods
     *         delay(aTimeToSleep);
     *     }
     * }
     * @endcode
     */
    __TASK_INLINE void allowSleep(bool aState = true);

    /**
     * @brief Set custom sleep implementation callback
     *
     * Assigns a custom callback function that will be called when the scheduler
     * enters sleep mode. This allows for platform-specific power management
     * implementations, such as deep sleep modes on ESP32 or low-power modes
     * on Arduino boards.
     *
     * @param aCallback Function pointer to sleep callback with signature:
     *                  void sleepCallback(unsigned long sleepTime)
     *                  where sleepTime is in milliseconds
     *
     * @note Only available when _TASK_SLEEP_ON_IDLE_RUN compile-time option is enabled
     * @note Sleep callback receives the calculated sleep duration in milliseconds
     * @note Default implementation uses delay() if no custom callback is set
     * @note Callback should implement appropriate power management for the platform
     * @see allowSleep(), SleepCallback, _TASK_SLEEP_ON_IDLE_RUN
     * @since Version 3.0.0
     *
     * @par Example:
     * @code
     * void arduinoSleep(unsigned long sleepTime) {
     *     // Arduino-specific power management
     *     if (sleepTime > 100) {
     *         // Use watchdog timer for longer sleeps
     *         set_sleep_mode(SLEEP_MODE_PWR_DOWN);
     *         sleep_enable();
     *         sleep_mode();
     *         sleep_disable();
     *     } else {
     *         delay(sleepTime);
     *     }
     * }
     *
     * void esp32Sleep(unsigned long sleepTime) {
     *     // ESP32-specific power management
     *     if (sleepTime > 1000) {
     *         esp_sleep_enable_timer_wakeup(sleepTime * 1000ULL);
     *         esp_light_sleep_start();
     *     } else {
     *         delayMicroseconds(sleepTime * 1000);
     *     }
     * }
     *
     * scheduler.setSleepMethod(esp32Sleep);
     * @endcode
     */
    __TASK_INLINE void setSleepMethod( SleepCallback aCallback );
#endif  // _TASK_SLEEP_ON_IDLE_RUN

#ifdef _TASK_LTS_POINTER
    /**
     * @brief Get Local Task Storage pointer of currently executing task
     *
     * Returns the LTS (Local Task Storage) pointer of the task that is
     * currently being executed by the scheduler. This provides access to
     * task-specific data from within the scheduler context or shared utilities.
     *
     * @return void* Pointer to the current task's local storage, or nullptr if:
     *               - No task is currently executing
     *               - Current task has no LTS pointer set
     *               - Called outside of task execution context
     *
     * @note Only available when _TASK_LTS_POINTER compile-time option is enabled
     * @note Must be called from within task execution context for valid results
     * @note Returned pointer must be cast to appropriate data type
     * @note Useful for shared utilities that need access to current task data
     * @see Task::setLtsPointer(), Task::getLtsPointer(), _TASK_LTS_POINTER
     * @since Version 3.0.0
     *
     * @par Example:
     * @code
     * struct TaskData {
     *     int sensorValue;
     *     float average;
     *     bool alertFlag;
     * };
     *
     * void sharedUtilityFunction() {
     *     TaskData* currentData = (TaskData*)scheduler.currentLts();
     *     if (currentData) {
     *         // Access current task's data
     *         Serial.print("Current task sensor value: ");
     *         Serial.println(currentData->sensorValue);
     *
     *         if (currentData->sensorValue > THRESHOLD) {
     *             currentData->alertFlag = true;
     *         }
     *     }
     * }
     *
     * void taskCallback() {
     *     // Task-specific work
     *     readSensors();
     *
     *     // Call shared utility that accesses current task's LTS
     *     sharedUtilityFunction();
     * }
     * @endcode
     */
    __TASK_INLINE void* currentLts();
#endif  // _TASK_LTS_POINTER

#ifdef _TASK_TIMECRITICAL
    /**
     * @brief Check if any task execution overran its allocated time
     *
     * Returns whether any task in the current execution cycle exceeded its
     * scheduled interval time. This provides system-wide timing violation
     * detection for real-time system monitoring and performance analysis.
     *
     * @return bool True if any task overran its allocated time, false otherwise
     *
     * @note Only available when _TASK_TIMECRITICAL compile-time option is enabled
     * @note Checks for timing violations across all tasks in the scheduler
     * @note Useful for system-wide real-time performance monitoring
     * @note Should be checked after each scheduler execution cycle
     * @see Task::getOverrun(), getCpuLoadTotal(), _TASK_TIMECRITICAL
     * @since Version 3.0.0
     *
     * @par Example:
     * @code
     * void loop() {
     *     scheduler.execute();
     *
     *     // Check for system-wide timing violations
     *     if (scheduler.isOverrun()) {
     *         Serial.println("WARNING: System overrun detected!");
     *
     *         // Log CPU load statistics
     *         unsigned long cpuLoad = scheduler.getCpuLoadTotal();
     *         Serial.print("CPU Load: ");
     *         Serial.print(cpuLoad);
     *         Serial.println("%");
     *
     *         // Take corrective action
     *         if (cpuLoad > 90) {
     *             // Reduce system load
     *             disableNonCriticalTasks();
     *         }
     *     }
     * }
     * @endcode
     */
    __TASK_INLINE bool isOverrun();

    /**
     * @brief Reset CPU load measurement counters
     *
     * Resets the internal counters used for CPU load calculation, providing
     * a fresh measurement baseline. This allows for periodic CPU load monitoring
     * and performance analysis over specific time intervals.
     *
     * @note Only available when _TASK_TIMECRITICAL compile-time option is enabled
     * @note Resets both cycle time and idle time counters
     * @note Should be called before starting a new measurement period
     * @note Useful for periodic performance monitoring and load analysis
     * @see getCpuLoadTotal(), getCpuLoadCycle(), getCpuLoadIdle()
     * @since Version 3.0.0
     *
     * @par Example:
     * @code
     * void performanceMonitor() {
     *     static unsigned long lastReset = 0;
     *     unsigned long now = millis();
     *
     *     // Measure CPU load every 10 seconds
     *     if (now - lastReset > 10000) {
     *         unsigned long cpuLoad = scheduler.getCpuLoadTotal();
     *         unsigned long cycleTime = scheduler.getCpuLoadCycle();
     *         unsigned long idleTime = scheduler.getCpuLoadIdle();
     *
     *         Serial.print("10s Performance Report - CPU Load: ");
     *         Serial.print(cpuLoad);
     *         Serial.print("%, Cycle: ");
     *         Serial.print(cycleTime);
     *         Serial.print("us, Idle: ");
     *         Serial.print(idleTime);
     *         Serial.println("us");
     *
     *         // Reset for next measurement period
     *         scheduler.cpuLoadReset();
     *         lastReset = now;
     *     }
     * }
     * @endcode
     */
    __TASK_INLINE void cpuLoadReset();

    /**
     * @brief Get total CPU cycle time in microseconds
     *
     * Returns the cumulative time spent in CPU cycles (active execution)
     * since the last reset. This measures the total processing time used
     * by all tasks and scheduler overhead.
     *
     * @return unsigned long Total CPU cycle time in microseconds
     *
     * @note Only available when _TASK_TIMECRITICAL compile-time option is enabled
     * @note Includes task execution time and scheduler overhead
     * @note Used in conjunction with getCpuLoadIdle() for load calculation
     * @note Accumulates until reset with cpuLoadReset()
     * @see getCpuLoadIdle(), getCpuLoadTotal(), cpuLoadReset()
     * @since Version 3.0.0
     *
     * @par Example:
     * @code
     * void detailedPerformanceAnalysis() {
     *     unsigned long cycleTime = scheduler.getCpuLoadCycle();
     *     unsigned long idleTime = scheduler.getCpuLoadIdle();
     *     unsigned long totalTime = cycleTime + idleTime;
     *
     *     if (totalTime > 0) {
     *         float cpuUtilization = (float)cycleTime / totalTime * 100.0;
     *         float idlePercentage = (float)idleTime / totalTime * 100.0;
     *
     *         Serial.print("Detailed Analysis - Active: ");
     *         Serial.print(cpuUtilization, 2);
     *         Serial.print("%, Idle: ");
     *         Serial.print(idlePercentage, 2);
     *         Serial.print("%, Total time: ");
     *         Serial.print(totalTime);
     *         Serial.println("us");
     *     }
     * }
     * @endcode
     */
    __TASK_INLINE unsigned long getCpuLoadCycle(){ return iCPUCycle; };

    /**
     * @brief Get total CPU idle time in microseconds
     *
     * Returns the cumulative time spent in idle state (no tasks executing)
     * since the last reset. This measures the total time the CPU was
     * available but not processing tasks.
     *
     * @return unsigned long Total CPU idle time in microseconds
     *
     * @note Only available when _TASK_TIMECRITICAL compile-time option is enabled
     * @note Includes time spent waiting for next task execution
     * @note Used in conjunction with getCpuLoadCycle() for load calculation
     * @note Accumulates until reset with cpuLoadReset()
     * @see getCpuLoadCycle(), getCpuLoadTotal(), cpuLoadReset()
     * @since Version 3.0.0
     *
     * @par Example:
     * @code
     * void powerManagementAnalysis() {
     *     unsigned long idleTime = scheduler.getCpuLoadIdle();
     *     unsigned long cycleTime = scheduler.getCpuLoadCycle();
     *
     *     // Calculate power saving potential
     *     if (idleTime > cycleTime) {
     *         float powerSavingPotential = (float)idleTime / (idleTime + cycleTime) * 100.0;
     *
     *         Serial.print("Power saving potential: ");
     *         Serial.print(powerSavingPotential, 1);
     *         Serial.println("%");
     *
     *         if (powerSavingPotential > 50.0) {
     *             Serial.println("Consider enabling sleep mode for better power efficiency");
     *         }
     *     }
     * }
     * @endcode
     */
    __TASK_INLINE unsigned long getCpuLoadIdle() { return iCPUIdle; };

    /**
     * @brief Get total CPU load percentage
     *
     * Calculates and returns the CPU load as a percentage based on the ratio
     * of active cycle time to total time (cycle + idle). This provides a
     * normalized measure of system utilization for performance monitoring.
     *
     * @return unsigned long CPU load percentage (0-100)
     *
     * @note Only available when _TASK_TIMECRITICAL compile-time option is enabled
     * @note Returns percentage of time spent in active execution vs idle
     * @note Calculation: (cycleTime / (cycleTime + idleTime)) * 100
     * @note Returns 0 if no measurements have been taken
     * @see getCpuLoadCycle(), getCpuLoadIdle(), cpuLoadReset()
     * @since Version 3.0.0
     *
     * @par Example:
     * @code
     * void systemHealthMonitor() {
     *     unsigned long cpuLoad = scheduler.getCpuLoadTotal();
     *
     *     // System health thresholds
     *     if (cpuLoad > 95) {
     *         Serial.println("CRITICAL: CPU load extremely high!");
     *         // Emergency measures
     *         disableNonEssentialTasks();
     *     } else if (cpuLoad > 80) {
     *         Serial.println("WARNING: High CPU load detected");
     *         // Performance optimization
     *         optimizeTaskIntervals();
     *     } else if (cpuLoad < 10) {
     *         Serial.println("INFO: Low CPU load - power optimization opportunity");
     *         // Enable power saving features
     *         scheduler.allowSleep(true);
     *     }
     *
     *     // Log current status
     *     Serial.print("Current CPU Load: ");
     *     Serial.print(cpuLoad);
     *     Serial.println("%");
     * }
     * @endcode
     */
    __TASK_INLINE unsigned long getCpuLoadTotal();
#endif  // _TASK_TIMECRITICAL

#ifdef _TASK_PRIORITY
    /**
     * @brief Sets a higher priority scheduler for priority-based task scheduling
     *
     * This method establishes a hierarchical relationship between schedulers, allowing
     * the current scheduler to defer execution to a higher priority scheduler when needed.
     * This enables multi-level priority scheduling where critical tasks can preempt
     * normal task execution.
     *
     * @param aScheduler Pointer to the higher priority scheduler to associate with this scheduler.
     *                   Pass nullptr to remove the high priority scheduler association.
     *
     * @note This method is only available when _TASK_PRIORITY compilation option is enabled.
     * @note The high priority scheduler should be executed more frequently than this scheduler
     *       to ensure priority-based preemption works correctly.
     * @note Circular priority relationships should be avoided to prevent infinite loops.
     *
     * @see currentScheduler()
     * @see _TASK_PRIORITY
     *
     * Example usage:
     * @code
     * Scheduler highPriorityScheduler;
     * Scheduler normalScheduler;
     *
     * // Set up priority relationship
     * normalScheduler.setHighPriorityScheduler(&highPriorityScheduler);
     *
     * // In main loop, execute high priority first
     * highPriorityScheduler.execute();
     * normalScheduler.execute();
     * @endcode
     */
    __TASK_INLINE void setHighPriorityScheduler(Scheduler* aScheduler);

    /**
     * @brief Returns a reference to the currently executing scheduler
     *
     * This static method provides access to the scheduler that is currently executing tasks.
     * It's primarily used internally by the library to maintain context during task execution
     * and to support priority-based scheduling operations.
     *
     * @return Reference to the currently active scheduler instance
     *
     * @note This method is only available when _TASK_PRIORITY compilation option is enabled.
     * @note This is a static method that can be called without a scheduler instance.
     * @note The returned reference is valid only during task execution context.
     *
     * @warning Do not store the returned reference for long-term use as the current
     *          scheduler context changes during execution.
     *
     * @see setHighPriorityScheduler()
     * @see _TASK_PRIORITY
     *
     * Example usage:
     * @code
     * void taskCallback() {
     *     // Get reference to the scheduler executing this task
     *     Scheduler& currentSched = Scheduler::currentScheduler();
     *
     *     // Access scheduler information
     *     Serial.print("Active tasks: ");
     *     Serial.println(currentSched.size());
     * }
     * @endcode
     */
    __TASK_INLINE static Scheduler& currentScheduler() { return *(iCurrentScheduler); };
#endif  // _TASK_PRIORITY

#ifdef _TASK_EXPOSE_CHAIN
    /**
     * @brief Returns a pointer to the first task in the scheduler's task chain
     *
     * This method provides direct access to the first task in the internal linked list
     * of tasks managed by this scheduler. It's primarily used for advanced task chain
     * manipulation, debugging, and custom iteration over all tasks.
     *
     * @return Pointer to the first task in the chain, or nullptr if no tasks are registered
     *
     * @note This method is only available when _TASK_EXPOSE_CHAIN compilation option is enabled.
     * @note The returned pointer should be treated as read-only to avoid corrupting the task chain.
     * @note Tasks are stored in a doubly-linked list internally.
     *
     * @warning Direct manipulation of the task chain can lead to undefined behavior.
     *          Use the standard addTask() and deleteTask() methods instead.
     *
     * @see getLastTask()
     * @see addTask()
     * @see deleteTask()
     * @see _TASK_EXPOSE_CHAIN
     *
     * Example usage:
     * @code
     * Scheduler ts;
     * Task t1, t2, t3;
     *
     * ts.addTask(t1);
     * ts.addTask(t2);
     * ts.addTask(t3);
     *
     * // Iterate through all tasks
     * Task* current = ts.getFirstTask();
     * while (current != nullptr) {
     *     Serial.print("Task ID: ");
     *     Serial.println(current->getId());
     *     current = current->getNext();
     * }
     * @endcode
     */
    __TASK_INLINE Task*  getFirstTask() { return iFirst; };

    /**
     * @brief Returns a pointer to the last task in the scheduler's task chain
     *
     * This method provides direct access to the last task in the internal linked list
     * of tasks managed by this scheduler. It's useful for reverse iteration through
     * the task chain and for advanced task management operations.
     *
     * @return Pointer to the last task in the chain, or nullptr if no tasks are registered
     *
     * @note This method is only available when _TASK_EXPOSE_CHAIN compilation option is enabled.
     * @note The returned pointer should be treated as read-only to avoid corrupting the task chain.
     * @note Tasks are stored in a doubly-linked list internally.
     *
     * @warning Direct manipulation of the task chain can lead to undefined behavior.
     *          Use the standard addTask() and deleteTask() methods instead.
     *
     * @see getFirstTask()
     * @see addTask()
     * @see deleteTask()
     * @see _TASK_EXPOSE_CHAIN
     *
     * Example usage:
     * @code
     * Scheduler ts;
     * Task t1, t2, t3;
     *
     * ts.addTask(t1);
     * ts.addTask(t2);
     * ts.addTask(t3);
     *
     * // Iterate through tasks in reverse order
     * Task* current = ts.getLastTask();
     * while (current != nullptr) {
     *     Serial.print("Task ID (reverse): ");
     *     Serial.println(current->getId());
     *     current = current->getPrev();
     * }
     * @endcode
     */
    __TASK_INLINE Task*  getLastTask()  { return iLast;  };
#endif // _TASK_EXPOSE_CHAIN

#ifdef _TASK_THREAD_SAFE
    /**
     * @brief Queues a task operation request for thread-safe execution
     *
     * This method places a pre-constructed request structure onto the task request queue
     * for later processing during the next scheduler execution cycle. This provides
     * thread-safe access to task operations from interrupt service routines or other
     * execution contexts.
     *
     * @param aRequest Pointer to a _task_request_t structure containing the operation details.
     *                 The structure should be properly initialized with the target object,
     *                 operation type, and parameters.
     *
     * @return true if the request was successfully queued, false if the queue is full
     *
     * @note This method is only available when _TASK_THREAD_SAFE compilation option is enabled.
     * @note The request structure must remain valid until the request is processed.
     * @note Requests are processed in FIFO order during scheduler execution.
     * @note The maximum number of queued requests is limited by the request queue size.
     *
     * @warning The aRequest pointer must point to a valid _task_request_t structure.
     *          Invalid pointers will cause undefined behavior.
     *
     * @see requestAction(void*, _task_request_type_t, unsigned long, unsigned long, unsigned long, unsigned long, unsigned long)
     * @see processRequests()
     * @see _task_request_t
     * @see _TASK_THREAD_SAFE
     *
     * Example usage:
     * @code
     * _task_request_t request;
     * request.object = &myTask;
     * request.type = TASK_REQUEST_ENABLE;
     * request.param1 = 0;
     * request.param2 = 0;
     * request.param3 = 0;
     * request.param4 = 0;
     * request.param5 = 0;
     *
     * if (ts.requestAction(&request)) {
     *     Serial.println("Request queued successfully");
     * } else {
     *     Serial.println("Request queue full");
     * }
     * @endcode
     */
    __TASK_INLINE bool   requestAction(_task_request_t* aRequest);

    /**
     * @brief Queues a task operation request with individual parameters for thread-safe execution
     *
     * This method creates and queues a task operation request using individual parameters.
     * It provides a convenient interface for thread-safe task operations without requiring
     * manual construction of the request structure.
     *
     * @param aObject Pointer to the target object (Task or Scheduler) for the operation
     * @param aType Type of operation to perform (from _task_request_type_t enum)
     * @param aParam1 First parameter for the operation (interpretation depends on aType)
     * @param aParam2 Second parameter for the operation (interpretation depends on aType)
     * @param aParam3 Third parameter for the operation (interpretation depends on aType)
     * @param aParam4 Fourth parameter for the operation (interpretation depends on aType)
     * @param aParam5 Fifth parameter for the operation (interpretation depends on aType)
     *
     * @return true if the request was successfully queued, false if the queue is full
     *
     * @note This method is only available when _TASK_THREAD_SAFE compilation option is enabled.
     * @note Parameter interpretation varies by operation type - see _task_request_type_t documentation.
     * @note Unused parameters should be set to 0.
     * @note Requests are processed in FIFO order during scheduler execution.
     *
     * @warning The aObject pointer must point to a valid Task or Scheduler instance.
     *          Invalid pointers will cause undefined behavior when the request is processed.
     *
     * @see requestAction(_task_request_t*)
     * @see processRequests()
     * @see _task_request_type_t
     * @see _TASK_THREAD_SAFE
     *
     * Example usage:
     * @code
     * // Request to enable a task from an ISR
     * void IRAM_ATTR sensorISR() {
     *     ts.requestAction(&sensorTask, TASK_REQUEST_ENABLE, 0, 0, 0, 0, 0);
     * }
     *
     * // Request to restart a task with new interval
     * ts.requestAction(&myTask, TASK_REQUEST_RESTART, 5000, 0, 0, 0, 0); // 5 second interval
     *
     * // Request to set task iterations
     * ts.requestAction(&myTask, TASK_REQUEST_SET_ITERATIONS, 10, 0, 0, 0, 0); // 10 iterations
     * @endcode
     */
    __TASK_INLINE bool   requestAction(void* aObject, _task_request_type_t aType, unsigned long aParam1, unsigned long aParam2, unsigned long aParam3, unsigned long aParam4, unsigned long aParam5);
#endif

  _TASK_SCOPE:
#ifdef _TASK_THREAD_SAFE
    __TASK_INLINE void   processRequests();
#endif
    Task          *iFirst, *iLast, *iCurrent, *iNextExecute;  // pointers to first, last, current and next-to-execute tasks in the chain

    volatile bool iPaused, iEnabled;
    unsigned long iActiveTasks;
    unsigned long iTotalTasks;
    unsigned long iInvokedTasks;
    unsigned long iChainLength;

#ifdef _TASK_SLEEP_ON_IDLE_RUN
    bool          iAllowSleep;                      // indication if putting MC to IDLE_SLEEP mode is allowed by the program at this time.
#endif  // _TASK_SLEEP_ON_IDLE_RUN

#ifdef _TASK_PRIORITY
    Scheduler*    iHighPriority;                    // Pointer to a higher priority scheduler
#endif  // _TASK_PRIORITY

#ifdef _TASK_TIMECRITICAL
    unsigned long iCPUStart;
    unsigned long iCPUCycle;
    unsigned long iCPUIdle;
#endif  // _TASK_TIMECRITICAL

#ifdef _TASK_TICKLESS
    unsigned long iNextRun;
#endif
};


#endif /* _TASKSCHEDULERDECLARATIONS_H_ */
