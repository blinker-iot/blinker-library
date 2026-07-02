/*
Cooperative multitasking library for Arduino
Copyright (c) 2015-2026 Anatoli Arkhipenko

Changelog:
v1.0.0:
    2015-02-24 - Initial release
    2015-02-28 - added delay() and disableOnLastIteration() methods
    2015-03-25 - changed scheduler execute() method for a more precise delay calculation:
                 1. Do not delay if any of the tasks ran (making request for immediate execution redundant)
                 2. Delay is invoked only if none of the tasks ran
                 3. Delay is based on the min anticipated wait until next task _AND_ the runtime of execute method itself.
    2015-05-11 - added  restart() and restartDelayed() methods to restart tasks which are on hold after running all iterations
    2015-05-19 - completely removed  delay from the scheduler since there are no power saving there. using 1 ms sleep instead

v1.4.1:
    2015-09-15 - more careful placement of AVR-specific includes for sleep method (compatibility with DUE)
                 sleep on idle run is no longer a default and should be explicitly compiled with
                _TASK_SLEEP_ON_IDLE_RUN defined

v1.5.0:
    2015-09-20 - access to currently executing task (for callback methods)
    2015-09-20 - pass scheduler as a parameter to the task constructor to append the task to the end of the chain
    2015-09-20 - option to create a task already enabled

v1.5.1:
    2015-09-21 - bug fix: incorrect handling of active tasks via set() and setIterations().
                 Thanks to Hannes Morgenstern for catching this one

v1.6.0:
    2015-09-22 - revert back to having all tasks disable on last iteration.
    2015-09-22 - deprecated disableOnLastIteration method as a result
    2015-09-22 - created a separate branch 'disable-on-last-iteration' for this
    2015-10-01 - made version numbers semver compliant (documentation only)

v1.7.0:
   2015-10-08 - introduced callback run counter - callback methods can branch on the iteration number.
   2015-10-11 - enableIfNot() - enable a task only if it is not already enabled. Returns true if was already enabled,
                false if was disabled.
   2015-10-11 - disable() returns previous enable state (true if was enabled, false if was already disabled)
   2015-10-11 - introduced callback methods "on enable" and "on disable". On enable runs every time enable is called,
                on disable runs only if task was enabled
   2015-10-12 - new Task method: forceNextIteration() - makes next iteration happen immediately during the next pass
                regardless how much time is left

v1.8.0:
   2015-10-13 - support for status request objects allowing tasks waiting on requests
   2015-10-13 - moved to a single header file to allow compilation control via #defines from the main sketch

v1.8.1:
   2015-10-22 - implement Task id and control points to support identification of failure points for watchdog timer logging

v1.8.2:
   2015-10-27 - implement Local Task Storage Pointer (allow use of same callback code for different tasks)
   2015-10-27 - bug: currentTask() method returns incorrect Task reference if called within OnEnable and OnDisable methods
   2015-10-27 - protection against infinite loop in OnEnable (if enable() methods are called within OnEnable)
   2015-10-29 - new currentLts() method in the scheduler class returns current task's LTS pointer in one call

v1.8.3:
   2015-11-05 - support for task activation on a status request with arbitrary interval and number of iterations
               (0 and 1 are still default values)
   2015-11-05 - implement waitForDelayed() method to allow task activation on the status request completion
                delayed for one current interval
   2015-11-09 - added callback methods prototypes to all examples for Arduino IDE 1.6.6 compatibility
   2015-11-14 - added several constants to be used as task parameters for readability (e.g, TASK_FOREVER, TASK_SECOND, etc.)
   2015-11-14 - significant optimization of the scheduler's execute loop, including millis() rollover fix option

v1.8.4:
   2015-11-15 - bug fix: Task alignment with millis() for scheduling purposes should be done after OnEnable, not before.
                Especially since OnEnable method can change the interval
   2015-11-16 - further optimizations of the task scheduler execute loop

v1.8.5:
   2015-11-23 - bug fix: incorrect calculation of next task invocation in case callback changed the interval
   2015-11-23 - bug fix: Task::set() method calls setInterval() explicitly, therefore delaying the task in the same manner

v1.9.0:
   2015-11-24 - packed three byte-long status variables into bit array structure data type - saving 2 bytes per each task instance

v1.9.2:
   2015-11-28 - _TASK_ROLLOVER_FIX is deprecated (not necessary)
   2015-12-16 - bug fixes: automatic millis rollover support for delay methods
   2015-12-17 - new method for _TASK_TIMECRITICAL option: getStartDelay()

v2.0.0:
   2015-12-22 - _TASK_PRIORITY - support for layered task prioritization

v2.0.1:
   2016-01-02 - bug fix: issue#11 Xtensa compiler (esp8266): Declaration of constructor does not match implementation

v2.0.2:
   2016-01-05 - bug fix: time constants wrapped inside compile option
   2016-01-05 - support for ESP8266 wifi power saving mode for _TASK_SLEEP_ON_IDLE_RUN compile option

v2.1.0:
   2016-02-01 - support for microsecond resolution
   2016-02-02 - added Scheduler baseline start time reset method: startNow()

v2.2.0:
   2016-11-17 - all methods made 'inline' to support inclusion of TaskSchedule.h file into other header files

v2.2.1:
   2016-11-30 - inlined constructors. Added "yield()" and "yieldOnce()" functions to easily break down and chain
                back together long running callback methods
   2016-12-16 - added "getCount()" to StatusRequest objects, made every task StatusRequest enabled.
                Internal StatusRequest objects are accessible via "getInternalStatusRequest()" method.

v2.3.0:
   2017-02-24 - new timeUntilNextIteration() method within Scheduler class - inquire when a particlar task is
                scheduled to run next time

v2.4.0:
   2017-04-27 - added destructor to the Task class to ensure tasks are disables and taken off the execution chain
                upon destruction. (Contributed by Edwin van Leeuwen [BlackEdder - https://github.com/BlackEdder)

v2.5.0:
   2017-04-27 - ESP8266 ONLY: added optional support for std::functions via _TASK_STD_FUNCTION compilation option
                (Contributed by Edwin van Leeuwen [BlackEdder - https://github.com/BlackEdder)
   2017-08-30 - add _TASK_DEBUG making all methods and variables public FOR DEBUGGING PURPOSES ONLY!
                Use at your own risk!
   2017-08-30 - bug fix: Scheduler::addTask() checks if task is already part of an execution chain (github issue #37)
   2017-08-30 - support for multi-tab sketches (Contributed by Adam Ryczkowski - https://github.com/adamryczkowski)

v2.5.1:
   2018-01-06 - support for IDLE sleep on Teensy boards (tested on Teensy 3.5)

v2.5.2:
   2018-01-09 - _TASK_INLINE compilation directive making all methods declared "inline" (issue #42)

v2.6.0:
   2018-01-30 - _TASK_TIMEOUT compilation directive: Task overall timeout functionality
   2018-01-30 - ESP32 support (experimental)
                (Contributed by Marco Tombesi: https://github.com/baggior)

v2.6.1:
   2018-02-13 - Bug: support for task self-destruction in the OnDisable method
                Example 19: dynamic tasks creation and destruction
   2018-03-14 - Bug: high level scheduler ignored if lower level chain is empty
                Example 20: use of local task storage to work with task-specific class objects

v3.0.0:
   2018-03-15 - Major Release: Support for dynamic callback methods binding via compilation parameter _TASK_OO_CALLBACKS

v3.0.1:
   2018-11-09 - bug: task deleted from the execution chain cannot be added back (github issue #67)

v3.0.2:
   2018-11-11 - bug: default constructor is ambiguous when Status Request objects are enabled (github issue #65 & #68)

v3.0.3:
   2019-06-13 - feature: custom sleep callback method: setSleepMethod() - ability to dynamically control idle sleep for various microcontrollers
              - feature: support for MSP430 and MSP432 boards (pull request #75: big thanks to Guillaume Pirou, https://github.com/elominp)
              - officially discontinued support for offile documentation in favor of updating the Wiki pages

v3.1.0:
   2020-01-07 - feature: added 4 cpu load monitoring methods for _TASK_TIMECRITICAL compilation option

v3.1.1:
   2020-01-09 - update: more precise CPU load measuring. Ability to define idle sleep threshold for ESP chips

v3.1.2:
   2020-01-17 - bug fix: corrected external forward definitions of millis() and micros

v3.1.3:
   2020-01-30 - bug fix: _TASK_DEFINE_MILLIS to force forward definition of millis and micros. Not defined by default.
   2020-02-16 - bug fix: add 'virtual' to the Task destructor definition (issue #86)

v3.1.4:
   2020-02-22 - bug: get rid of unnecessary compiler warnings
   2020-02-22 - feature: access to the task chain with _TASK_EXPOSE_CHAIN compile option

v3.1.5:
   2020-05-08 - feature: implemented light sleep for esp32

v3.1.6:
   2020-05-12 - bug fix: deleteTask and addTask should check task ownership first (Issue #97)

v3.1.7:
   2020-07-07 - warning fix: unused parameter 'aRecursive' (Issue #99)

v3.2.0:
   2020-08-16 - feature: scheduling options

v3.2.1:
   2020-10-04 - feature: Task.abort method. Stop task execution without calling OnDisable().

v3.2.2:
   2020-12-14 - feature: enable and restart methods return true if task enabled
                feature: Task.cancel() method - disable task with a cancel flag (could be used for alt. path
                         processing in the onDisable method.
                feature: Task.cancelled() method - indicates that task was disabled with a cancel() method.

v3.2.3:
   2021-01-01 - feature: discontinued use of 'register' keyword. Depricated in C++ 11
                feature: add STM32 as a platform supporting _TASK_STD_FUNCTION. (PR #105)

v3.3.0:
   2021-05-11 - feature: Timeout() methods for StatusRequest objects

v3.4.0:
   2021-07-14 - feature: ability to Enable/Disable and Pause/Resume scheduling
              - feature: optional use of external millis/micros methods

v3.5.0:
   2021-11-01 - feature: adjust(long aInterval) method - adjust execution schedule:
                + aInterval - shift schedule forward (later)
                - aInterval - shift schedule backwards (earlier)

v3.6.0:
   2021-11-01 - feature: _TASK_THREAD_SAFE compile option for multi-core systems or running under RTOS

v3.6.1:
   2022-06-28 - bug: Internal Status Request of the canceled and aborted tasks complete with respective error code
              - feature: TASK_SR_ABORT code causes Tasks waiting on this Status Request to be aborted as well

v3.6.2:
   2022-10-04 - feature: added TScheduler.hpp and TSchedulerDeclarations.hpp - a workaround for conflicting declarations (e.g., nRF52840 using Adafruit Core).
                using namespace TS (credit: https://github.com/vortigont)

v3.7.0:
   2022-10-10 - feature: added ability for Task to "self-destruct" on disable. Useful for dynamic task management.
                Added updated example 19 for this functionality. Updated the Sketch Template
                (Thanks, https://github.com/vortigont for the idea).

v3.8.0:
   2023-01-24 - feature: added setIntervalNodelay() method to dynamically adjust current interval

v3.8.1:
   2023-05-11 - bug: conditional compile options missing from *.hpp files (Adafruit support)

v3.8.2:
   2023-09-27 - feature: _TASK_TICKLESS - support for tickless execution under FreeRTOS
              - feature: _TASK_DO_NOT_YIELD - ability to disable yield() in execute() method

v3.8.3:
   2023-09-29 - feature: _TASK_TICKLESS - change in approach for backwards compatibility
              - feature: added scheduler stats for total/active/invoked tasks per each pass

v3.8.4:
   2024-01-13 - bug: (git PR #180): the variables tStart and tFinish are required if sleep support is enabled,
                independent of _TASK_TIMECRITICAL. however, they were guarded by _TASK_TIMECRITICAL as well.
              - bug: (git PR #181): delete manually disable tasks with self-destruct flag
              - bug: (git PR #182): correct deletion of self-destruct 'current' task in disableAll()

v3.8.5:
   2024-06-17 - updated volatile compound statements after C++20 deprecated compound assignment on volatiles

v3.9.0:
   2024-08-14 - _TASK_ISR_SUPPORT compile option (espressif chips only) - allow placing a few control methods into IRAM to be used in ISRs
                list of IRAM-enabled methods:
                    StatusRequest::signal
                    StatusRequest::signalComplete
                    Task::enable
                    Task::enableIfNot
                    Task::enableDelayed
                    Task::restart
                    Task::restartDelayed
                    Task::delay
                    Task::forceNextIteration

v4.0.0:
    2024-10-26 - MAJOR UPDATE for use in pre-emptive environments (FreeRTOS or Zephyr)
        - list of IRAM-enabled methods extended:
            Task::disable
            Task::abort
            Task::cancel
        - _TASK_DEFINE_MILLIS - deprecated
        - _TASK_EXTERNAL_TIME - deprecated
        - New compile options:
            _TASK_NON_ARDUINO - does not include "Arduino.h"
                                target platform has to implement:
                                    unsigned long _task_millis();
                                    unsigned long _task_micros();
                                    void _task_yield();
            _TASK_HEADER_AND_CPP - enables compilation of TaskScheduler.cpp (non Arduino IDE use)

        - Major rework of the _TASK_THREAD_SAFE approach. Developers should only be calling Scheduler and Task methods
          directly from the thread where TaskScheduler execute() method runs.
          Calls from the other threads should be done via Scheduler::requestAction(...) methods.
          Target platform should implement an action queue and two methods:
             bool _task_enqueue_request(_task_request_t* req);  // puts _task_request_t object on the action queue
             bool _task_dequeue_request(_task_request_t* req);  // retrieves _task_request_t object from the action queue
             Please see examples folder for implementation folder.

    2025-09-13:
        - feature: allow _TASK_STD_FUNCTION for all platforms (you should know what you are doing)
        - feature: allow _TASK_DO_NOT_YIELD for all platforms (you should know what you are doing)

    2025-09-21:
        - bug: incorrect behavior of the StatusRequest timeout functionality due to uninitialized timeout variable
        - update: addedd Doxygen comments to all methods and variables
         -update: added a number of compile and unit tests for CI/CD

v4.0.1:
    2025-10-07:
        - bug: removed leftover iMutex variable
        - added example30: _TASK_THREAD_SAFE

v4.0.2:
    2025-10-09:
        - added unit test for _TASK_THREAD_SAFE functionality

v4.0.3:
    2025-11-02:
        - bug: next execution time with _TASK_TICKLESS did not take task timeout into account

v4.0.4:
    2025-11-15:
        - bug: set callbacks should not be available for _TASK_OO_CALLBACKS mode

v4.0.5:
    2026-04-16:
        - bug fix: use-after-free in Scheduler::execute() when a callback destroys
          the next task in the chain (e.g. painlessMesh BufferedConnection teardown).
          Promoted nextTask to scheduler member iNextExecute so deleteTask() can
          advance it before the memory is freed.

v4.0.6:
    2026-04-17:
        - bug fix: thread-safe setSelfDestruct request called wrong function
          (setSchedulingOption instead of setSelfDestruct)
        - bug fix: NULL pointer dereference in disable() when iScheduler is NULL
          (triggered when a still-enabled task is removed via deleteTask then destroyed)
        - bug fix: dangling pointer in disableAll() and enableAll() -- applied the
          same iNextExecute pattern used in execute() to protect against callbacks
          that destroy the next task in the chain
        - bug fix: missing NULL check on iStatusRequest in execute() status request
          handling -- could crash if waiting flag was set but no StatusRequest assigned
        - added NULL safety to currentLts() and isOverrun() public API methods
        - initialized iActiveTasks/iTotalTasks/iInvokedTasks in Scheduler::init()
        - clarified assignment-in-condition in StatusRequest::signal()
        - added documentation comments for known limitations: thread-safe request
          pointer lifetime, millis() rollover in tickless mode, signed overflow in
          overrun calculations, execute() re-entrancy, pointer truncation on >32-bit

v4.0.7:
    2026-04-17:
        - feature: deprecated currentTask() now returns a safe sentinel instead of
          crashing when called outside a task callback (iCurrent == NULL)
        - feature: added getChainLength() method to Scheduler -- returns the number
          of tasks in the chain at any time, without requiring an execute() pass.
          Maintained via O(1) counter in addTask/deleteTask.

v4.0.8:
    2026-04-20:
        - bug fix: Scheduler::currentTask() failed to compile under _TASK_OO_CALLBACKS
          because the static Task sDummy sentinel introduced in v4.0.7 cannot be
          instantiated (Task is abstract when OO callbacks are enabled -- Callback()
          is pure virtual). Replaced with a local concrete subclass that stubs out
          Callback() in OO mode; function-pointer mode path is unchanged.
        - example: Scheduler_example21_OO_Callbacks -- silenced signed/unsigned
          compare warning in SuperSensor::measurementReady() by casting iDelay to
          unsigned long.
*/

#include "TaskSchedulerDeclarations.h"

#ifndef _TASKSCHEDULER_H_
#define _TASKSCHEDULER_H_

// ----------------------------------------
// The following "defines" control library functionality at compile time,
// and should be used in the main sketch depending on the functionality required
//
// #define _TASK_TIMECRITICAL       // Enable monitoring scheduling overruns
// #define _TASK_SLEEP_ON_IDLE_RUN  // Enable 1 ms SLEEP_IDLE powerdowns between runs if no callback methods were invoked during the pass
// #define _TASK_STATUS_REQUEST     // Compile with support for StatusRequest functionality - triggering tasks on status change events in addition to time only
// #define _TASK_WDT_IDS            // Compile with support for wdt control points and task ids
// #define _TASK_LTS_POINTER        // Compile with support for local task storage pointer
// #define _TASK_PRIORITY           // Support for layered scheduling priority
// #define _TASK_MICRO_RES          // Support for microsecond resolution
// #define _TASK_STD_FUNCTION       // Support for std::function
// #define _TASK_DEBUG              // Make all methods and variables public for debug purposes
// #define _TASK_INLINE             // Make all methods "inline" - needed to support some multi-tab, multi-file implementations
// #define _TASK_TIMEOUT            // Support for overall task timeout
// #define _TASK_OO_CALLBACKS       // Support for callbacks via inheritance
// #define _TASK_EXPOSE_CHAIN       // Methods to access tasks in the task chain
// #define _TASK_SCHEDULING_OPTIONS // Support for multiple scheduling options
// #define _TASK_SELF_DESTRUCT      // Enable tasks to "self-destruct" after disable
// #define _TASK_TICKLESS           // Enable support for tickless sleep on FreeRTOS
// #define _TASK_DO_NOT_YIELD       // Disable yield() method in execute() for ESP chips
// #define _TASK_ISR_SUPPORT        // for esp chips - place control methods in IRAM
// #define _TASK_NON_ARDUINO        // for non-arduino use
// #define _TASK_HEADER_AND_CPP     // PlatformIO style: separate Header and CPP file
// #define _TASK_THREAD_SAFE        // Enable additional checking for thread safety

 #ifdef _TASK_MICRO_RES

 #undef _TASK_SLEEP_ON_IDLE_RUN     // SLEEP_ON_IDLE has only millisecond resolution
 #define __TASK_TIME_FUNCTION() _task_micros()

 #else
 #define __TASK_TIME_FUNCTION() _task_millis()

 #endif  // _TASK_MICRO_RES


#ifdef _TASK_SLEEP_ON_IDLE_RUN
#include "TaskSchedulerSleepMethods.h"

  Scheduler* iSleepScheduler;
  SleepCallback iSleepMethod;

#endif  // _TASK_SLEEP_ON_IDLE_RUN


// _TASK_STD_FUNCTION is allowed for all platforms - you should know what you are doing
// #if !defined (ARDUINO_ARCH_ESP8266) && !defined (ARDUINO_ARCH_ESP32) && !defined (ARDUINO_ARCH_STM32)
// #ifdef _TASK_STD_FUNCTION
//     #error Support for std::function only for ESP8266 or ESP32 architecture
// #undef _TASK_STD_FUNCTION
// #endif // _TASK_STD_FUNCTION
// #endif // ARDUINO_ARCH_ESP8266

#ifdef _TASK_WDT_IDS
    static unsigned int  __task_id_counter = 0; // global task ID counter for assiging task IDs automatically.
#endif  // _TASK_WDT_IDS

#ifdef _TASK_PRIORITY
    Scheduler* iCurrentScheduler;
#endif // _TASK_PRIORITY


#ifdef _TASK_THREAD_SAFE
__attribute__((weak)) bool _task_enqueue_request(_task_request_t* req) {return false; };
__attribute__((weak)) bool _task_dequeue_request(_task_request_t* req) {return false; };
#endif


// ------------------ TaskScheduler implementation --------------------

#ifndef _TASK_NON_ARDUINO

#ifdef _TASK_MICRO_RES
static unsigned long _task_micros() {return micros();}
#else
static unsigned long _task_millis() {return millis();}
#if defined(_TASK_SLEEP_ON_IDLE_RUN) || defined(_TASK_TIMECRITICAL)
static unsigned int _task_micros() {return micros();}
#endif  // _TASK_SLEEP_ON_IDLE_RUN
#endif  //  _TASK_MICRO_RES

#if !defined(_TASK_DO_NOT_YIELD)
static void _task_yield() { yield(); };
#endif

#else

#ifdef _TASK_MICRO_RES
static unsigned long _task_micros();
#else
static unsigned long _task_millis();
#if defined(_TASK_SLEEP_ON_IDLE_RUN) || defined(_TASK_TIMECRITICAL)
static unsigned long _task_micros();
#endif  // _TASK_SLEEP_ON_IDLE_RUN
#endif  //  _TASK_MICRO_RES

#if !defined(_TASK_DO_NOT_YIELD)
static void _task_yield();
#endif
#endif  // #ifndef _TASK_NON_ARDUINO

/** Constructor, uses default values for the parameters
 * so could be called with no parameters.
 */
#ifdef _TASK_OO_CALLBACKS
Task::Task( unsigned long aInterval, long aIterations, Scheduler* aScheduler, bool aEnable
#ifdef _TASK_SELF_DESTRUCT
, bool aSelfDestruct ) {
#else
  ) {
#endif  //  #ifdef _TASK_SELF_DESTRUCT
    reset();
    set(aInterval, aIterations);
#else
Task::Task( unsigned long aInterval, long aIterations, TaskCallback aCallback, Scheduler* aScheduler, bool aEnable, TaskOnEnable aOnEnable, TaskOnDisable aOnDisable
#ifdef _TASK_SELF_DESTRUCT
, bool aSelfDestruct ) {
#else
  ) {
#endif  //  #ifdef _TASK_SELF_DESTRUCT
    reset();
    set(aInterval, aIterations, aCallback, aOnEnable, aOnDisable);
#endif

#ifdef _TASK_SELF_DESTRUCT
    setSelfDestruct(aSelfDestruct);
#endif  //  #ifdef _TASK_SELF_DESTRUCT

    if (aScheduler) aScheduler->addTask(*this);

#ifdef _TASK_WDT_IDS
    iTaskID = ++ __task_id_counter;
#endif  // _TASK_WDT_IDS

    if (aEnable) enable();
}

/** Destructor.
 *  Makes sure the task disabled and deleted out of the chain
 *  prior to being deleted.
 */
Task::~Task() {
    if ( this->isEnabled() ) disable();
    if (iScheduler) iScheduler->deleteTask(*this);
}


#ifdef _TASK_STATUS_REQUEST

/** Constructor with reduced parameter list for tasks created for
 *  StatusRequest only triggering (always immediate and only 1 iteration)
 */


#ifdef _TASK_OO_CALLBACKS
Task::Task( Scheduler* aScheduler ) {
    reset();
    set(TASK_IMMEDIATE, TASK_ONCE);
#else
Task::Task( TaskCallback aCallback, Scheduler* aScheduler, TaskOnEnable aOnEnable, TaskOnDisable aOnDisable ) {
    reset();
    set(TASK_IMMEDIATE, TASK_ONCE, aCallback, aOnEnable, aOnDisable);
#endif // _TASK_OO_CALLBACKS

    if (aScheduler) aScheduler->addTask(*this);

#ifdef _TASK_WDT_IDS
    iTaskID = ++ __task_id_counter;
#endif  // _TASK_WDT_IDS
}


StatusRequest::StatusRequest()
{
    iCount = 0;
    iStatus = 0;
#ifdef _TASK_TIMEOUT
    iTimeout = 0;
    iStarttime = 0;
#endif  //  #ifdef _TASK_TIMEOUT
}

void StatusRequest::setWaiting(unsigned int aCount) {
  iCount = aCount;
  iStatus = 0;
#ifdef _TASK_TIMEOUT
  iStarttime = __TASK_TIME_FUNCTION();
#endif  //  #ifdef _TASK_TIMEOUT
}

bool StatusRequest::isPending() { return (iCount != 0); }
bool StatusRequest::isCompleted() { return (iCount == 0); }
int StatusRequest::getStatus() { return iStatus; }
int StatusRequest::getCount() { return iCount; }
StatusRequest* Task::getStatusRequest() { return iStatusRequest; }
StatusRequest* Task::getInternalStatusRequest() { return &iMyStatusRequest; }

/** Signals completion of the StatusRequest by one of the participating events
 *  @param: aStatus - if provided, sets the return code of the StatusRequest: negative = error, 0 (default) = OK, positive = OK with a specific status code
 *  Negative status will complete Status Request fully (since an error occured).
 *  @return: true, if StatusRequest is complete, false otherwise (still waiting for other events)
 */
bool __TASK_IRAM StatusRequest::signal(int aStatus) {
    if ( iCount) {  // do not update the status request if it was already completed
        if (iCount > 0)  --iCount;
        iStatus = aStatus;
        if ( iStatus < 0 ) iCount = 0;   // if an error is reported, the status is requested to be completed immediately
    }
    return (iCount == 0);
}

void __TASK_IRAM StatusRequest::signalComplete(int aStatus) {
    if (iCount) { // do not update the status request if it was already completed
        iCount = 0;
        iStatus = aStatus;
    }
}

/** Sets a Task to wait until a particular event completes
 *  @param: aStatusRequest - a pointer for the StatusRequest to wait for.
 *  If aStatusRequest is NULL, request for waiting is ignored, and the waiting task is not enabled.
 */
bool Task::waitFor(StatusRequest* aStatusRequest, unsigned long aInterval, long aIterations) {
    iStatusRequest = aStatusRequest;
    if ( iStatusRequest != NULL ) { // assign internal StatusRequest var and check if it is not NULL
        setIterations(aIterations);
        setInterval(aInterval);
        iStatus.waiting = _TASK_SR_NODELAY;  // no delay
        return enable();
    }
    return false;
}

bool Task::waitForDelayed(StatusRequest* aStatusRequest, unsigned long aInterval, long aIterations) {
    iStatusRequest = aStatusRequest;
    if ( iStatusRequest != NULL ) { // assign internal StatusRequest var and check if it is not NULL
        setIterations(aIterations);
        if ( aInterval ) setInterval(aInterval);  // For the delayed version only set the interval if it was not a zero
        iStatus.waiting = _TASK_SR_DELAY;  // with delay equal to the current interval
        return enable();
    }
    return false;
}

#ifdef _TASK_TIMEOUT
void StatusRequest::resetTimeout() {
    iStarttime = __TASK_TIME_FUNCTION();
}

long StatusRequest::untilTimeout() {
    if ( iTimeout ) {
        return ( (long) (iStarttime + iTimeout) - (long) __TASK_TIME_FUNCTION() );
    }
    return -1;
}
#endif  // _TASK_TIMEOUT
#endif  // _TASK_STATUS_REQUEST

bool Task::isEnabled() { return iStatus.enabled; }

unsigned long Task::getInterval() { return iInterval; }

long Task::getIterations() { return iIterations; }

unsigned long Task::getRunCounter() { return iRunCounter; }

#ifdef _TASK_OO_CALLBACKS

// bool Task::Callback() { return true; }
bool Task::OnEnable() { return true; }
void Task::OnDisable() { }

#else

void Task::setCallback(TaskCallback aCallback) { iCallback = aCallback; }
void Task::setOnEnable(TaskOnEnable aCallback) { iOnEnable = aCallback; }
void Task::setOnDisable(TaskOnDisable aCallback) { iOnDisable = aCallback; }

#endif // _TASK_OO_CALLBACKS


/** Resets (initializes) the task/
 * Task is not enabled and is taken out
 * out of the execution chain as a result
 */
void Task::reset() {

    iStatus.enabled = false;
    iStatus.inonenable = false;
    iStatus.canceled = false;
    iPreviousMillis = 0;
    iInterval = 0;
    iDelay = 0;
    iPrev = NULL;
    iNext = NULL;
    iScheduler = NULL;
    iRunCounter = 0;

#ifdef _TASK_SCHEDULING_OPTIONS
    iOption = TASK_SCHEDULE;
#endif  // _TASK_SCHEDULING_OPTIONS

#ifdef _TASK_TIMECRITICAL
    iOverrun = 0;
    iStartDelay = 0;
#endif  // _TASK_TIMECRITICAL

#ifdef _TASK_WDT_IDS
    iControlPoint = 0;
#endif  // _TASK_WDT_IDS

#ifdef _TASK_LTS_POINTER
    iLTS = NULL;
#endif  // _TASK_LTS_POINTER

#ifdef _TASK_STATUS_REQUEST
    iStatusRequest = NULL;
    iStatus.waiting = 0;
    iMyStatusRequest.signalComplete();
#endif  // _TASK_STATUS_REQUEST

#ifdef  _TASK_TIMEOUT
    iTimeout = 0;
    iStarttime = 0;
    iStatus.timeout = false;
#endif  // _TASK_TIMEOUT

#ifdef _TASK_SELF_DESTRUCT
    iStatus.sd_request = false;
#endif  //  #ifdef _TASK_SELF_DESTRUCT

}

/** Explicitly set Task execution parameters
 * @param aInterval - execution interval in ms
 * @param aIterations - number of iterations, use -1 for no limit
 * @param aCallback - pointer to the callback method which executes the task actions
 * @param aOnEnable - pointer to the callback method which is called on enable()
 * @param aOnDisable - pointer to the callback method which is called on disable()
 */

#ifdef _TASK_OO_CALLBACKS
void Task::set(unsigned long aInterval, long aIterations) {
#else
void Task::set(unsigned long aInterval, long aIterations, TaskCallback aCallback, TaskOnEnable aOnEnable, TaskOnDisable aOnDisable) {
    iCallback = aCallback;
    iOnEnable = aOnEnable;
    iOnDisable = aOnDisable;
#endif // _TASK_OO_CALLBACKS
    setInterval(aInterval);
    iSetIterations = aIterations;
    iIterations = aIterations;
}

/** Sets number of iterations for the task
 * if task is enabled, schedule for immediate execution
 * @param aIterations - number of iterations, use -1 for no limit
 */
void Task::setIterations(long aIterations) {
    iSetIterations = aIterations;
    iIterations = aIterations;
}

#ifndef _TASK_OO_CALLBACKS

/** Prepare task for next step iteration following yielding of control to the scheduler
 * @param aCallback - pointer to the callback method for the next step
 */
void Task::yield (TaskCallback aCallback) {
    iCallback = aCallback;
    forceNextIteration();

    // The next 2 lines adjust runcounter and number of iterations
    // as if it is the same run of the callback, just split between
    // a series of callback methods
    iRunCounter = iRunCounter - 1;
    if ( iIterations >= 0 ) iIterations = iIterations + 1;
}

/** Prepare task for next step iteration following yielding of control to the scheduler
 * @param aCallback - pointer to the callback method for the next step
 */
void Task::yieldOnce (TaskCallback aCallback) {
    yield(aCallback);
    iIterations = 1;
}
#endif // _TASK_OO_CALLBACKS


/** Enables the task
 *  schedules it for execution as soon as possible,
 *  and resets the RunCounter back to zero
 */
bool __TASK_IRAM Task::enable() {
    if (iScheduler) { // activation without active scheduler does not make sense
        iRunCounter = 0;
        iStatus.canceled = false;

#ifdef _TASK_STATUS_REQUEST
        iMyStatusRequest.setWaiting();
#endif // _TASK_STATUS_REQUEST]

#ifdef _TASK_OO_CALLBACKS
        if ( !iStatus.inonenable ) {
            Task *current = iScheduler->iCurrent;
            iScheduler->iCurrent = this;
            iStatus.inonenable = true;      // Protection against potential infinite loop
            iStatus.enabled = OnEnable();
            iStatus.inonenable = false;     // Protection against potential infinite loop
            iScheduler->iCurrent = current;
        }
#else
        if ( iOnEnable && !iStatus.inonenable ) {
            Task *current = iScheduler->iCurrent;
            iScheduler->iCurrent = this;
            iStatus.inonenable = true;      // Protection against potential infinite loop
            iStatus.enabled = iOnEnable();
            iStatus.inonenable = false;     // Protection against potential infinite loop
            iScheduler->iCurrent = current;
        }
        else {
            iStatus.enabled = true;
        }
#endif // _TASK_OO_CALLBACKS

        iDelay = iInterval;
        iPreviousMillis = __TASK_TIME_FUNCTION() - iDelay;

#ifdef _TASK_TIMEOUT
        resetTimeout();
#endif // _TASK_TIMEOUT

#ifdef _TASK_STATUS_REQUEST
        if ( !iStatus.enabled ) {
            iMyStatusRequest.signalComplete();
        }
#endif // _TASK_STATUS_REQUEST
        return iStatus.enabled;
    }
    return false;
}

/** Enables the task only if it was not enabled already
 * Returns previous state (true if was already enabled, false if was not)
 */
bool __TASK_IRAM Task::enableIfNot() {
    bool previousEnabled = iStatus.enabled;
    if ( !previousEnabled ) enable();
    return (previousEnabled);
}

/** Enables the task
 * and schedules it for execution after a delay = aInterval
 */
bool __TASK_IRAM Task::enableDelayed(unsigned long aDelay) {
    enable();
    delay(aDelay);
    return iStatus.enabled;
}

#ifdef _TASK_TIMEOUT
void Task::setTimeout(unsigned long aTimeout, bool aReset) {
    iTimeout = aTimeout;
    if (aReset) resetTimeout();
}

void Task::resetTimeout() {
    iStarttime = __TASK_TIME_FUNCTION();
    iStatus.timeout = false;
}

unsigned long Task::getTimeout() {
    return iTimeout;
}

long Task::untilTimeout() {
    if ( iTimeout ) {
        return ( (long) (iStarttime + iTimeout) - (long) __TASK_TIME_FUNCTION() );
    }
    return -1;
}

bool Task::isTimedOut() {
    return iStatus.timeout;
}

#endif // _TASK_TIMEOUT



/** Delays Task for execution after a delay = aInterval (if task is enabled).
 * leaves task enabled or disabled
 * if aDelay is zero, delays for the original scheduling interval from now
 */
void __TASK_IRAM Task::delay(unsigned long aDelay) {
    iDelay = aDelay ? aDelay : iInterval;
    iPreviousMillis = __TASK_TIME_FUNCTION();
}

/** Adjusts Task execution with aInterval (if task is enabled).
 */
void Task::adjust(long aInterval) {
    if ( aInterval == 0 ) return;  //  nothing to do for a zero
    if ( aInterval < 0 ) {
      iPreviousMillis = iPreviousMillis + aInterval;
    }
    else {
      iDelay = iDelay + aInterval;  //  we have to adjust delay because adjusting iPreviousMillis might push
                            //  it into the future beyond current millis() and cause premature trigger
    }
}


/** Schedules next iteration of Task for execution immediately (if enabled)
 * leaves task enabled or disabled
 * Task's original schedule is shifted, and all subsequent iterations will continue from this point in time
 */
void __TASK_IRAM Task::forceNextIteration() {
    iDelay = iInterval;
    iPreviousMillis = __TASK_TIME_FUNCTION() - iDelay;
}

/** Sets the execution interval.
 * Task execution is delayed for aInterval
 * Use  enable() to schedule execution ASAP
 * @param aInterval - new execution interval
 */
void Task::setInterval (unsigned long aInterval) {
    iInterval = aInterval;
    delay(); // iDelay will be updated by the delay() function
}

/** Sets the execution interval without delaying the task
 * Task state does not change
 * If Task is disabled, it would remain so
 * @param aInterval - new execution interval
 */
void Task::setIntervalNodelay (unsigned long aInterval, unsigned int aOption) {
// #define TASK_INTERVAL_KEEP      0
// #define TASK_INTERVAL_RECALC    1
// #define TASK_INTERVAL_RESET     2

    switch (aOption) {
      case TASK_INTERVAL_RECALC:
      {
          int32_t d = aInterval - iInterval;
          // change the delay proportionally
          iDelay = iDelay + d;
          iInterval = aInterval;
          break;
      }
      case TASK_INTERVAL_RESET:
          iInterval = aInterval;
          iDelay = aInterval;
          break;

      default:
//      case TASK_INTERVAL_KEEP:
          if ( iInterval == iDelay ) {
              iInterval = aInterval;
              iDelay = aInterval;
          }
          else {
              iInterval = aInterval;
          }
          break;
    }
}

/** Disables task
 * Task will no longer be executed by the scheduler
 * Returns status of the task before disable was called (i.e., if the task was already disabled)
 */

bool __TASK_IRAM Task::disable() {
    bool previousEnabled = iStatus.enabled;
    iStatus.enabled = false;
    iStatus.inonenable = false;

  if (iScheduler) {
#ifdef _TASK_OO_CALLBACKS
    if (previousEnabled) {
#else
    if (previousEnabled && iOnDisable) {
#endif // _TASK_OO_CALLBACKS

        Task *current = iScheduler->iCurrent;
        iScheduler->iCurrent = this;
#ifdef _TASK_OO_CALLBACKS
        OnDisable();
#else
        iOnDisable();
#endif // _TASK_OO_CALLBACKS

        iScheduler->iCurrent = current;
    }
  }
#ifdef _TASK_STATUS_REQUEST
    iMyStatusRequest.signalComplete();
#endif

#ifdef _TASK_SELF_DESTRUCT
    if ( getSelfDestruct() ) iStatus.sd_request = true;
#endif  //  #ifdef _TASK_SELF_DESTRUCT
    return (previousEnabled);
}

/** Aborts task execution
 * Task will no longer be executed by the scheduler AND ondisable method will not be called
 */
void __TASK_IRAM Task::abort() {
    iStatus.enabled = false;
    iStatus.inonenable = false;
    iStatus.canceled = true;
#ifdef _TASK_STATUS_REQUEST
    iMyStatusRequest.signalComplete(TASK_SR_ABORT);
#endif

#ifdef _TASK_SELF_DESTRUCT
    if ( getSelfDestruct() ) iStatus.sd_request = true;
#endif  //  #ifdef _TASK_SELF_DESTRUCT
}


/** Cancels task execution
 * Task will no longer be executed by the scheduler. Ondisable method will be called after 'canceled' flag is set
 */
void __TASK_IRAM Task::cancel() {
    iStatus.canceled = true;
#ifdef _TASK_STATUS_REQUEST
    iMyStatusRequest.signalComplete(TASK_SR_ABORT);
#endif
    disable();
}

bool Task::isCanceled() {
    return iStatus.canceled;
}

/** Restarts task
 * Task will run number of iterations again
 */

bool __TASK_IRAM Task::restart() {
    iIterations = iSetIterations;
    return enable();
}

/** Restarts task delayed
 * Task will run number of iterations again
 */
bool __TASK_IRAM Task::restartDelayed(unsigned long aDelay) {
    iIterations = iSetIterations;
    return enableDelayed(aDelay);
}

bool Task::isFirstIteration() { return (iRunCounter <= 1); }

bool Task::isLastIteration() { return (iIterations == 0); }

#ifdef _TASK_TIMECRITICAL

long Task::getOverrun() { return iOverrun; }
long Task::getStartDelay() { return iStartDelay; }

#endif  // _TASK_TIMECRITICAL


#ifdef _TASK_WDT_IDS

void Task::setId(unsigned int aID) { iTaskID = aID; }
unsigned int Task::getId() { return iTaskID; }
void Task::setControlPoint(unsigned int aPoint) { iControlPoint = aPoint; }
unsigned int Task::getControlPoint() { return iControlPoint; }

#endif  // _TASK_WDT_IDS

#ifdef _TASK_LTS_POINTER

void  Task::setLtsPointer(void *aPtr) { iLTS = aPtr; }
void* Task::getLtsPointer() { return iLTS; }

#endif  // _TASK_LTS_POINTER

// ------------------ Scheduler implementation --------------------

/** Default constructor.
 * Creates a scheduler with an empty execution chain.
 */
Scheduler::Scheduler() {
    init();
#ifdef _TASK_SLEEP_ON_IDLE_RUN
    setSleepMethod(&SleepMethod);
#endif // _TASK_SLEEP_ON_IDLE_RUN
}

/*
Scheduler::~Scheduler() {
#ifdef _TASK_SLEEP_ON_IDLE_RUN
#endif // _TASK_SLEEP_ON_IDLE_RUN
}
*/

/** Initializes all internal varaibles
 */
void Scheduler::init() {
    iEnabled = false;

    iFirst = NULL;
    iLast = NULL;
    iCurrent = NULL;
    iNextExecute = NULL;

    iPaused = false;
    iActiveTasks = 0;
    iTotalTasks = 0;
    iInvokedTasks = 0;
    iChainLength = 0;

#ifdef _TASK_PRIORITY
    iHighPriority = NULL;
#endif  // _TASK_PRIORITY

#ifdef _TASK_SLEEP_ON_IDLE_RUN
    allowSleep(true);
#endif  // _TASK_SLEEP_ON_IDLE_RUN

#ifdef _TASK_TIMECRITICAL
    cpuLoadReset();
#endif  // _TASK_TIMECRITICAL

    iEnabled = true;
}

/** Appends task aTask to the tail of the execution chain.
 * @param &aTask - reference to the Task to be appended.
 * @note Task can only be part of the chain once.
 */
 void Scheduler::addTask(Task& aTask) {
// If task already belongs to a scheduler, we should not be adding
// it to this scheduler. It should be deleted from the other scheduler first.
    if (aTask.iScheduler != NULL)
        return;

    iEnabled = false;

    aTask.iScheduler = this;
// First task situation:
    if (iFirst == NULL) {
        iFirst = &aTask;
        aTask.iPrev = NULL;
    }
    else {
// This task gets linked back to the previous last one
        aTask.iPrev = iLast;
        iLast->iNext = &aTask;
    }
// "Previous" last task gets linked to this one - as this one becomes the last one
    aTask.iNext = NULL;
    iLast = &aTask;
    iChainLength++;

    iEnabled = true;
}

/** Deletes specific Task from the execution chain
 * @param &aTask - reference to the task to be deleted from the chain
 */
void Scheduler::deleteTask(Task& aTask) {
// Can only delete own tasks
    if (aTask.iScheduler != this)
        return;

    iEnabled = false;
    iChainLength--;

    // If execute() is mid-iteration and this task is next in line,
    // advance the pointer before we unlink and the memory is freed.
    if (iNextExecute == &aTask)
        iNextExecute = aTask.iNext;

    aTask.iScheduler = NULL;
    if (aTask.iPrev == NULL) {
        if (aTask.iNext == NULL) {
            iFirst = NULL;
            iLast = NULL;
            iEnabled = true;
            return;
        }
        else {
            aTask.iNext->iPrev = NULL;
            iFirst = aTask.iNext;
            aTask.iNext = NULL;
            iEnabled = true;
            return;
        }
    }

    if (aTask.iNext == NULL) {
        aTask.iPrev->iNext = NULL;
        iLast = aTask.iPrev;
        aTask.iPrev = NULL;
        iEnabled = true;
        return;
    }

    aTask.iPrev->iNext = aTask.iNext;
    aTask.iNext->iPrev = aTask.iPrev;
    aTask.iPrev = NULL;
    aTask.iNext = NULL;

    iEnabled = true;
}

/** Disables all tasks in the execution chain
 * Convenient for error situations, when the only
 * task remaining active is an error processing task
 * @param aRecursive - if true, tasks of the higher priority chains are disabled as well recursively
 */
#ifdef _TASK_PRIORITY
void Scheduler::disableAll(bool aRecursive) {
#else
void Scheduler::disableAll() {
#endif

    iEnabled = false;

    Task*    current = iFirst;
    while (current) {
        iNextExecute = current->iNext;
        current->disable();
#ifdef _TASK_SELF_DESTRUCT
        if ( current->iStatus.sd_request ) delete current;
#endif  //  #ifdef _TASK_SELF_DESTRUCT
        current = iNextExecute;
    }

#ifdef _TASK_PRIORITY
    if (aRecursive && iHighPriority) iHighPriority->disableAll(true);
#endif  // _TASK_PRIORITY

    iEnabled = true;
}


/** Enables all the tasks in the execution chain
 * @param aRecursive - if true, tasks of the higher priority chains are enabled as well recursively
 */
#ifdef _TASK_PRIORITY
void Scheduler::enableAll(bool aRecursive) {
#else
void Scheduler::enableAll() {
#endif

    iEnabled = false;

    Task    *current = iFirst;
    while (current) {
        iNextExecute = current->iNext;
        current->enable();
        current = iNextExecute;
    }

#ifdef _TASK_PRIORITY
    if (aRecursive && iHighPriority) iHighPriority->enableAll(true);
#endif  // _TASK_PRIORITY

    iEnabled = true;
}

/** Sets scheduler for the higher priority tasks (support for layered task priority)
 * @param aScheduler - pointer to a scheduler for the higher priority tasks
 */
#ifdef _TASK_PRIORITY
void Scheduler::setHighPriorityScheduler(Scheduler* aScheduler) {
    if (aScheduler != this) iHighPriority = aScheduler;  // Setting yourself as a higher priority one will create infinite recursive call

#ifdef _TASK_SLEEP_ON_IDLE_RUN
    if (iHighPriority) {
        iHighPriority->allowSleep(false);       // Higher priority schedulers should not do power management
    }
#endif  // _TASK_SLEEP_ON_IDLE_RUN

};
#endif  // _TASK_PRIORITY


#ifdef _TASK_SLEEP_ON_IDLE_RUN
void Scheduler::allowSleep(bool aState) {
    iAllowSleep = aState;
}
#endif  // _TASK_SLEEP_ON_IDLE_RUN


#ifdef _TASK_PRIORITY
void Scheduler::startNow( bool aRecursive ) {
#else
void Scheduler::startNow() {
#endif
    unsigned long t = __TASK_TIME_FUNCTION();

    iEnabled = false;

    iCurrent = iFirst;
    while (iCurrent) {
        if ( iCurrent->iStatus.enabled ) iCurrent->iPreviousMillis = t - iCurrent->iDelay;
        iCurrent = iCurrent->iNext;
    }

#ifdef _TASK_PRIORITY
    if (aRecursive && iHighPriority) iHighPriority->startNow( true );
#endif  // _TASK_PRIORITY

    iEnabled = true;
}

/** Returns number millis or micros until next scheduled iteration of a given task
 *
 * @param aTask - reference to task which next iteration is in question
 */
long Scheduler::timeUntilNextIteration(Task& aTask) {

#ifdef _TASK_STATUS_REQUEST

    StatusRequest *s = aTask.getStatusRequest();
    if ( s != NULL && s->pending() )
        return (-1);    // cannot be determined
#endif
    if ( !aTask.isEnabled() )
        return (-1);    // cannot be determined

    long d = (long) aTask.iDelay - ( (long) (__TASK_TIME_FUNCTION() - aTask.iPreviousMillis) );

    if ( d < 0 )
        return (0); // Task will run as soon as possible
    return ( d );
}


// DEPRECATED: Unsafe outside task callbacks (iCurrent may be NULL).
// Use getCurrentTask() instead, which returns a pointer (NULL-safe).
Task& Scheduler::currentTask() {
#ifdef _TASK_OO_CALLBACKS
    // Task is abstract in OO mode (Callback() is pure virtual), so the
    // sentinel needs a concrete subclass that stubs it out.
    struct _DummyTask : public Task {
        _DummyTask() : Task() {}
        bool Callback() { return false; }
    };
    static _DummyTask sDummy;
#else
    static Task sDummy;
#endif
    return iCurrent ? *iCurrent : sDummy;
}
Task* Scheduler::getCurrentTask() { return iCurrent; }

#ifdef _TASK_LTS_POINTER
void* Scheduler::currentLts() { return iCurrent ? iCurrent->iLTS : NULL; }
#endif  // _TASK_LTS_POINTER

#ifdef _TASK_TIMECRITICAL
bool Scheduler::isOverrun() { return iCurrent ? (iCurrent->iOverrun < 0) : false; }

void Scheduler::cpuLoadReset() {
    iCPUStart = _task_micros();
    iCPUCycle = 0;
    iCPUIdle = 0;
}


unsigned long Scheduler::getCpuLoadTotal() {
    return (_task_micros() - iCPUStart);
}
#endif  // _TASK_TIMECRITICAL


#ifdef _TASK_SLEEP_ON_IDLE_RUN
void  Scheduler::setSleepMethod( SleepCallback aCallback ) {
    if ( aCallback != NULL ) {
        iSleepScheduler = this;
        iSleepMethod = aCallback;
    }
}
#endif  // _TASK_SLEEP_ON_IDLE_RUN

#ifdef  _TASK_THREAD_SAFE
bool Scheduler::requestAction(_task_request_t* aRequest) {
    if ( aRequest == NULL ) return false;
    return _task_enqueue_request(aRequest);
}

bool Scheduler::requestAction(void* aObject, _task_request_type_t aType, unsigned long aParam1, unsigned long aParam2, unsigned long aParam3, unsigned long aParam4, unsigned long aParam5) {
    if ( aObject == NULL ) return false;
    _task_request_t r = {
        .req_type = aType,
        .object_ptr = aObject,
        .param1 = aParam1,
        .param2 = aParam2,
        .param3 = aParam3,
        .param4 = aParam4,
        .param5 = aParam5
    };
    return _task_enqueue_request(&r);
}

void Scheduler::processRequests() {
    _task_request_t req;

// NOTE: Thread-safe requests carry raw pointers. If the target object is
// destroyed between enqueue and dequeue, the pointer is dangling.
// Callers must ensure object lifetime spans request processing.
// A future version may add generation counters to detect stale requests.
    while ( _task_dequeue_request(&req) ) {

        if ( req.object_ptr == NULL ) continue;

        switch (req.req_type ) {

#ifdef _TASK_STATUS_REQUEST
        case TASK_SR_REQUEST_SETWAITING_1: {
            StatusRequest* t = (StatusRequest*) req.object_ptr;
            t->setWaiting(req.param1);
        }
        break;

        case TASK_SR_REQUEST_SIGNAL_1: {
            StatusRequest* t = (StatusRequest*) req.object_ptr;
            t->signal((int)req.param1);
        }
        break;

        case TASK_SR_REQUEST_SIGNALCOMPLETE_1: {
            StatusRequest* t = (StatusRequest*) req.object_ptr;
            t->signalComplete((int) req.param1);
        }
        break;

#ifdef _TASK_TIMEOUT
        case TASK_SR_REQUEST_SETTIMEOUT_1: {
            StatusRequest* t = (StatusRequest*) req.object_ptr;
            t->setTimeout(req.param1);
        }
        break;

        case TASK_SR_REQUEST_RESETTIMEOUT_0: {
            StatusRequest* t = (StatusRequest*) req.object_ptr;
            t->resetTimeout();
        }
        break;
#endif  // _TASK_TIMEOUT

#endif

#ifdef _TASK_LTS_POINTER
        case TASK_REQUEST_SETLTSPOINTER_1: {
            Task* t = (Task*) req.object_ptr;
            t->setLtsPointer((void*) req.param1);
        }
        break;
#endif

#ifdef _TASK_SELF_DESTRUCT
        case TASK_REQUEST_SETSELFDESTRUCT_1: {
            Task* t = (Task*) req.object_ptr;
            t->setSelfDestruct((bool)req.param1);
        }
        break;
#endif

#ifdef _TASK_SCHEDULING_OPTIONS
        case TASK_REQUEST_SETSCHEDULINGOPTION_1: {
            Task* t = (Task*) req.object_ptr;
            t->setSchedulingOption((unsigned int)req.param1);
        }
        break;
#endif

#ifdef _TASK_TIMEOUT
        case TASK_REQUEST_SETTIMEOUT_2: {
            Task* t = (Task*) req.object_ptr;
            t->setTimeout(req.param1, (bool)req.param2);
        }
        break;

        case TASK_REQUEST_RESETTIMEOUT_0: {
            Task* t = (Task*) req.object_ptr;
            t->resetTimeout();
        }
        break;
#endif

#ifdef _TASK_STATUS_REQUEST
        case TASK_REQUEST_WAITFOR: {
            Task* t = (Task*) req.object_ptr;
            t->waitFor((StatusRequest*)req.param1, req.param2, req.param3);
        }
        break;

        case TASK_REQUEST_WAITFORDELAYED_3: {
            Task* t = (Task*) req.object_ptr;
            t->waitForDelayed((StatusRequest*)req.param1, req.param2, (long) req.param3);
        }
        break;
#endif

#ifdef _TASK_WDT_IDS
        case TASK_REQUEST_SETID_1: {
            Task* t = (Task*) req.object_ptr;
            t->setId((unsigned int)req.param1);
        }
        break;

        case TASK_REQUEST_SETCONTROLPOINT_1: {
            Task* t = (Task*) req.object_ptr;
            t->setControlPoint((unsigned int)req.param1);
        }
        break;
#endif

        case TASK_REQUEST_ENABLE_0: {
            Task* t = (Task*) req.object_ptr;
            t->enable();
        }
        break;

        case TASK_REQUEST_ENABLEIFNOT_0: {
            Task* t = (Task*) req.object_ptr;
            t->enableIfNot();
        }
        break;

        case TASK_REQUEST_ENABLEDELAYED_1: {
            Task* t = (Task*) req.object_ptr;
            t->enableDelayed(req.param1);
        }
        break;

        case TASK_REQUEST_RESTART_0: {
            Task* t = (Task*) req.object_ptr;
            t->restart();
        }
        break;

        case TASK_REQUEST_RESTARTDELAYED_1: {
            Task* t = (Task*) req.object_ptr;
            t->restartDelayed(req.param1);
        }
        break;

        case TASK_REQUEST_DELAY_1: {
            Task* t = (Task*) req.object_ptr;
            t->delay(req.param1);
        }
        break;

        case TASK_REQUEST_ADJUST_1: {
            Task* t = (Task*) req.object_ptr;
            t->adjust(req.param1);
        }
        break;

        case TASK_REQUEST_FORCENEXTITERATION_0: {
            Task* t = (Task*) req.object_ptr;
            t->forceNextIteration();
        }
        break;

        case TASK_REQUEST_DISABLE_0: {
            Task* t = (Task*) req.object_ptr;
            t->disable();
        }
        break;

        case TASK_REQUEST_ABORT_0: {
            Task* t = (Task*) req.object_ptr;
            t->abort();
        }
        break;

        case TASK_REQUEST_CANCEL_0: {
            Task* t = (Task*) req.object_ptr;
            t->cancel();
        }
        break;

        case TASK_REQUEST_SET_5:{
            Task* t = (Task*) req.object_ptr;
#ifdef _TASK_OO_CALLBACKS
            t->set(req.param1, (long)req.param2);
#else
            t->set(req.param1, (long)req.param2, (TaskCallback) req.param3, (TaskOnEnable) req.param4, (TaskOnDisable) req.param5);
#endif
        }
        break;

        case TASK_REQUEST_SETINTERVAL_1: {
            Task* t = (Task*) req.object_ptr;
            t->setInterval(req.param1);
        }
        break;

        case TASK_REQUEST_SETINTERVALNODELAY_2: {
            Task* t = (Task*) req.object_ptr;
            t->setIntervalNodelay(req.param1, (unsigned int)req.param2);
        }
        break;

        case TASK_REQUEST_SETITERATIONS_1: {
            Task* t = (Task*) req.object_ptr;
            t->setIterations((long)req.param1);
        }
        break;

#ifndef _TASK_OO_CALLBACKS
        case TASK_REQUEST_SETCALLBACK_1: {
            Task* t = (Task*) req.object_ptr;
            t->setCallback((TaskCallback)req.param1);
        }
        break;

        case TASK_REQUEST_SETONENABLE_1: {
            Task* t = (Task*) req.object_ptr;
            t->setOnEnable((TaskOnEnable)req.param1);
        }
        break;

        case TASK_REQUEST_SETONDISABLE_1: {
            Task* t = (Task*) req.object_ptr;
            t->setOnDisable((TaskOnDisable)req.param1);
        }
        break;
#endif // _TASK_OO_CALLBACKS

        default:
        break;
        }
    }
}
#endif  // _TASK_THREAD_SAFE


/** Makes one pass through the execution chain.
 * Tasks are executed in the order they were added to the chain
 * There is no concept of priority
 * Different pseudo "priority" could be achieved
 * by running task more frequently
 */

// NOTE: In _TASK_PRIORITY mode, this method is called recursively on the
// high-priority scheduler. Do not call execute() on the same scheduler
// instance from within a task callback -- this will corrupt iCurrent and
// iNextExecute.
bool Scheduler::execute() {

    bool     idleRun = true;
    unsigned long m, i;  // millis, interval;

#if defined(_TASK_TIMECRITICAL)
    unsigned long tPassStart;
    unsigned long tTaskStart, tTaskFinish;
#endif  // _TASK_TIMECRITICAL

#ifdef _TASK_SLEEP_ON_IDLE_RUN
    unsigned long tFinish;
    unsigned long tStart;
#ifdef _TASK_TIMECRITICAL
    unsigned long tIdleStart = 0;
#endif  // _TASK_TIMECRITICAL
#endif  // _TASK_SLEEP_ON_IDLE_RUN


    iCurrent = iFirst;

    iActiveTasks = 0;
    iTotalTasks = 0;
    iInvokedTasks = 0;

#ifdef _TASK_PRIORITY
    // If lower priority scheduler does not have a single task in the chain
    // the higher priority scheduler still has to have a chance to run
        if (!iCurrent && iHighPriority) iHighPriority->execute();
        iCurrentScheduler = this;
#endif  // _TASK_PRIORITY

    //  each scheduled is enabled/disabled individually, so check iEnabled only
    //  after the higher priority scheduler has been invoked.
    if ( !iEnabled ) return true; //  consider this to be an idle run

#ifdef _TASK_THREAD_SAFE
    // Process external requests for task updates
    // The requests are processed in bulk, in the order they were received
    processRequests();
#endif

#ifdef _TASK_SLEEP_ON_IDLE_RUN
    // scheduling pass starts
    tStart = _task_micros();
#endif

#ifdef _TASK_TICKLESS
    unsigned long nr = UINT32_MAX;  // we do not know yet if we can tell when next run will be
    unsigned int  nrd = _TASK_NEXTRUN_UNDEFINED;
#endif


    while (!iPaused && iCurrent) {

#ifdef _TASK_THREAD_SAFE
    // Process external requests for task on every pass
    // The requests are processed in bulk, in the order they were received
    // This should emulate a one-thread behaviour
        processRequests();
#endif

        iTotalTasks++;

#if defined(_TASK_TIMECRITICAL)
        tPassStart = _task_micros();
        tTaskStart = tTaskFinish = 0;
#endif  // _TASK_TIMECRITICAL

#ifdef _TASK_PRIORITY
    // If scheduler for higher priority tasks is set, it's entire chain is executed on every pass of the base scheduler
        if (iHighPriority) idleRun = iHighPriority->execute() && idleRun;
        iCurrentScheduler = this;
#endif  // _TASK_PRIORITY
        iNextExecute = iCurrent->iNext;
        do {
            if ( iCurrent->iStatus.enabled ) {
                iActiveTasks++;

#ifdef _TASK_WDT_IDS
    // For each task the control points are initialized to avoid confusion because of carry-over:
                iCurrent->iControlPoint = 0;
#endif  // _TASK_WDT_IDS

    // Disable task on last iteration:
                if (iCurrent->iIterations == 0) {
                    iCurrent->disable();
#ifdef _TASK_SELF_DESTRUCT
                    if ( iCurrent->iStatus.sd_request ) delete iCurrent;
#endif  //  #ifdef _TASK_SELF_DESTRUCT
                    break;
                }
                m = __TASK_TIME_FUNCTION();
                i = iCurrent->iInterval;

#ifdef _TASK_TIMEOUT
    // Disable task on a timeout
                if ( iCurrent->iTimeout && (m - iCurrent->iStarttime > iCurrent->iTimeout) ) {
                    iCurrent->iStatus.timeout = true;
                    iCurrent->disable();
#ifdef _TASK_SELF_DESTRUCT
                    if ( iCurrent->iStatus.sd_request ) delete iCurrent;
#endif  //  #ifdef _TASK_SELF_DESTRUCT
                    break;
                }
#endif // _TASK_TIMEOUT

#ifdef  _TASK_STATUS_REQUEST
    // If StatusRequest object was provided, and still pending, and task is waiting, this task should not run
    // Otherwise, continue with execution as usual.  Tasks waiting to StatusRequest need to be rescheduled according to
    // how they were placed into waiting state (waitFor or waitForDelayed)
                if ( iCurrent->iStatus.waiting ) {

#ifdef _TASK_TICKLESS
                    // if there is a task waiting on a status request we are obligated to run continously
                    // because event can trigger at any point at time.
                    nrd |= _TASK_NEXTRUN_IMMEDIATE; // immediate
#endif

                    StatusRequest *sr = iCurrent->iStatusRequest;
                    if ( sr ) {
#ifdef _TASK_TIMEOUT
                        if ( sr->iTimeout && (m - sr->iStarttime > sr->iTimeout) ) {
                          sr->signalComplete(TASK_SR_TIMEOUT);
                        }
#endif // _TASK_TIMEOUT
                        if ( sr->pending() ) break;
                        if ( sr->iStatus == TASK_SR_ABORT ) {
                          iCurrent->abort();
                          break;
                        }
                    }
                    else {
                        iCurrent->iStatus.waiting = 0;  // clear broken waiting state
                        break;
                    }
                    if (iCurrent->iStatus.waiting == _TASK_SR_NODELAY) {
                        iCurrent->iDelay = i;
                        iCurrent->iPreviousMillis = m - i;
                    }
                    else {
                        iCurrent->iPreviousMillis = m;
                    }
                    iCurrent->iStatus.waiting = 0;
                }
#endif  // _TASK_STATUS_REQUEST

                // this is the main scheduling decision point
                // if the interval between current time and previous invokation time is less than the current delay - task should NOT be activated yet.
                // this is millis-rollover-safe way of scheduling
                if ( m - iCurrent->iPreviousMillis < iCurrent->iDelay ) {
#ifdef _TASK_TICKLESS
                // catch the reamining time until invocation as next time this should run
                // WARNING: nextrun calculation does not survive millis() rollover (~49.7 days).
                // For the rollover case we fall back to immediate execution, which is safe
                // but suboptimal (one extra wakeup per rollover event).
                    unsigned long nextrun = iCurrent->iDelay + iCurrent->iPreviousMillis;
                    // nextrun should be after current millis() (except rollover)
                    // nextrun should be sooner than previously determined
#ifdef _TASK_TIMEOUT
                    // in case timeout is set - we have to consider it as well
                    unsigned long untilto = iCurrent->untilTimeout() + iCurrent->iPreviousMillis;
                    if ( untilto < nextrun ) nextrun = untilto;
#endif  // _TASK_TIMEOUT
                    if ( nextrun > m && nextrun < nr ) {
                        nr = nextrun;
                        nrd |= _TASK_NEXTRUN_TIMED; // next run timed
                    }
#endif  //  _TASK_TICKLESS
                    break;
                }


#ifdef _TASK_TICKLESS
                // if there is a task ready to run right now - we have to have another scheduler pass right after this one
                // this is especially important for the tasks that run out of iterations and should be disabled during next pass
                nrd |= _TASK_NEXTRUN_IMMEDIATE; // next run immediate
#endif

                if ( iCurrent->iIterations > 0 ) iCurrent->iIterations = iCurrent->iIterations - 1;  // do not decrement (-1) being a signal of never-ending task
                iCurrent->iRunCounter = iCurrent->iRunCounter + 1;
#ifdef _TASK_SCHEDULING_OPTIONS
                switch (iCurrent->iOption) {
                  case TASK_INTERVAL:
                    iCurrent->iPreviousMillis = m;
                    break;

                  case TASK_SCHEDULE_NC:
                    iCurrent->iPreviousMillis = iCurrent->iPreviousMillis + iCurrent->iDelay;
                    {
                        // NOTE: cast to long is valid only when (p + i - m) fits in LONG_MAX (~24.8 days).
                        // Tasks with intervals exceeding ~24 days may report incorrect overrun values.
                        long ov = (long) ( iCurrent->iPreviousMillis + i - m );
                        if ( ov < 0 ) {
                            long ii = (i == 0) ? 1 : i; // avoid division by zero
                            iCurrent->iPreviousMillis = iCurrent->iPreviousMillis + ((m - iCurrent->iPreviousMillis) / ii) * ii;
                        }
                    }
                    break;

                  default:
                    iCurrent->iPreviousMillis = iCurrent->iPreviousMillis + iCurrent->iDelay;
                }
#else
                iCurrent->iPreviousMillis = iCurrent->iPreviousMillis + iCurrent->iDelay;
#endif  // _TASK_SCHEDULING_OPTIONS

#ifdef _TASK_TIMECRITICAL
    // Updated_previous+current interval should put us into the future, so iOverrun should be positive or zero.
    // If negative - the task is behind (next execution time is already in the past)
    // NOTE: cast to long is valid only when (p + i - m) fits in LONG_MAX (~24.8 days).
                unsigned long p = iCurrent->iPreviousMillis;
                iCurrent->iOverrun = (long) ( p + i - m );
                iCurrent->iStartDelay = (long) ( m - p );
#endif  // _TASK_TIMECRITICAL

                iCurrent->iDelay = i;

#if defined(_TASK_TIMECRITICAL)
                tTaskStart = _task_micros();
#endif  // _TASK_TIMECRITICAL

#ifdef _TASK_OO_CALLBACKS
                idleRun = !iCurrent->Callback();
#else
                if ( iCurrent->iCallback ) {
                    iCurrent->iCallback();
                    idleRun = false;
                    iInvokedTasks++;
                }
#endif // _TASK_OO_CALLBACKS

#if defined(_TASK_TIMECRITICAL)
                tTaskFinish = _task_micros();
#endif  // _TASK_TIMECRITICAL

            }
#ifdef _TASK_SELF_DESTRUCT
            else if ( iCurrent->iStatus.sd_request ) delete iCurrent;
#endif  //  #ifdef _TASK_SELF_DESTRUCT
        } while (0);    //guaranteed single run - allows use of "break" to exit

        iCurrent = iNextExecute;

#ifdef _TASK_TIMECRITICAL
        iCPUCycle += ( (_task_micros() - tPassStart) - (tTaskFinish - tTaskStart) );
#endif  // _TASK_TIMECRITICAL

// _TASK_DO_NOT_YIELD is allowed for everyone - you should know what you are doing
// #if defined (ARDUINO_ARCH_ESP8266) || defined (ARDUINO_ARCH_ESP32)
#if !defined(_TASK_DO_NOT_YIELD)
        _task_yield();
#endif  //  _TASK_DO_NOT_YIELD
// #endif  //  ARDUINO_ARCH_ESPxx
    }

#ifdef _TASK_SLEEP_ON_IDLE_RUN
    tFinish = _task_micros(); // Scheduling pass end time in microseconds.
#endif

#ifdef _TASK_TICKLESS
    iNextRun = 0;  // next iteration should be immediate by default
    // if the pass was "idle" and there are tasks scheduled
    do {
        if ( !idleRun ) break;
        if ( (nrd & _TASK_NEXTRUN_IMMEDIATE) ) break;
        if ( nrd == _TASK_NEXTRUN_UNDEFINED ) break;
        m = __TASK_TIME_FUNCTION();
        if ( nr <= m) break;
        iNextRun = ( nr - m );
    } while (0);
#endif

#ifdef _TASK_SLEEP_ON_IDLE_RUN

    if (idleRun && iAllowSleep) {
        if ( iSleepScheduler == this ) { // only one scheduler should make the MC go to sleep.
            if ( iSleepMethod != NULL ) {

#ifdef _TASK_TIMECRITICAL
                tIdleStart = _task_micros();
#endif  // _TASK_TIMECRITICAL

                (*iSleepMethod)( tFinish-tStart );

#ifdef _TASK_TIMECRITICAL
                iCPUIdle += (_task_micros() - tIdleStart);
#endif  // _TASK_TIMECRITICAL
            }
        }
    }



#endif  // _TASK_SLEEP_ON_IDLE_RUN

    return (idleRun);
}



#endif /* _TASKSCHEDULER_H_ */
