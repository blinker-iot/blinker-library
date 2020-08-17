// Cooperative multitasking library for Arduino
// Copyright (c) 2015-2017 Anatoli Arkhipenko
//
// Changelog:
// v1.0.0:
//     2015-02-24 - Initial release
//     2015-02-28 - added delay() and disableOnLastIteration() methods
//     2015-03-25 - changed scheduler execute() method for a more precise delay calculation:
//                  1. Do not delay if any of the tasks ran (making request for immediate execution redundant)
//                  2. Delay is invoked only if none of the tasks ran
//                  3. Delay is based on the min anticipated wait until next task _AND_ the runtime of execute method itself.
//     2015-05-11 - added  restart() and restartDelayed() methods to restart tasks which are on hold after running all iterations
//     2015-05-19 - completely removed  delay from the scheduler since there are no power saving there. using 1 ms sleep instead
//
// v1.4.1:
//     2015-09-15 - more careful placement of AVR-specific includes for sleep method (compatibility with DUE)
//                  sleep on idle run is no longer a default and should be explicitly compiled with
//                 _TASK_SLEEP_ON_IDLE_RUN defined
//
// v1.5.0:
//     2015-09-20 - access to currently executing task (for callback methods)
//     2015-09-20 - pass scheduler as a parameter to the task constructor to append the task to the end of the chain
//     2015-09-20 - option to create a task already enabled
//
// v1.5.1:
//     2015-09-21 - bug fix: incorrect handling of active tasks via set() and setIterations().
//                  Thanks to Hannes Morgenstern for catching this one
//
// v1.6.0:
//     2015-09-22 - revert back to having all tasks disable on last iteration.
//     2015-09-22 - deprecated disableOnLastIteration method as a result
//     2015-09-22 - created a separate branch 'disable-on-last-iteration' for this
//     2015-10-01 - made version numbers semver compliant (documentation only)
//
// v1.7.0:
//    2015-10-08 - introduced callback run counter - callback methods can branch on the iteration number.
//    2015-10-11 - enableIfNot() - enable a task only if it is not already enabled. Returns true if was already enabled,
//                 false if was disabled.
//    2015-10-11 - disable() returns previous enable state (true if was enabled, false if was already disabled)
//    2015-10-11 - introduced callback methods "on enable" and "on disable". On enable runs every time enable is called,
//                 on disable runs only if task was enabled
//    2015-10-12 - new Task method: forceNextIteration() - makes next iteration happen immediately during the next pass
//                 regardless how much time is left
//
// v1.8.0:
//    2015-10-13 - support for status request objects allowing tasks waiting on requests
//    2015-10-13 - moved to a single header file to allow compilation control via #defines from the main sketch
//
// v1.8.1:
//    2015-10-22 - implement Task id and control points to support identification of failure points for watchdog timer logging
//
// v1.8.2:
//    2015-10-27 - implement Local Task Storage Pointer (allow use of same callback code for different tasks)
//    2015-10-27 - bug: currentTask() method returns incorrect Task reference if called within OnEnable and OnDisable methods
//    2015-10-27 - protection against infinite loop in OnEnable (if enable() methods are called within OnEnable)
//    2015-10-29 - new currentLts() method in the scheduler class returns current task's LTS pointer in one call
//
// v1.8.3:
//    2015-11-05 - support for task activation on a status request with arbitrary interval and number of iterations
//                (0 and 1 are still default values)
//    2015-11-05 - implement waitForDelayed() method to allow task activation on the status request completion
//                 delayed for one current interval
//    2015-11-09 - added callback methods prototypes to all examples for Arduino IDE 1.6.6 compatibility
//    2015-11-14 - added several constants to be used as task parameters for readability (e.g, TASK_FOREVER, TASK_SECOND, etc.)
//    2015-11-14 - significant optimization of the scheduler's execute loop, including millis() rollover fix option
//
// v1.8.4:
//    2015-11-15 - bug fix: Task alignment with millis() for scheduling purposes should be done after OnEnable, not before.
//                 Especially since OnEnable method can change the interval
//    2015-11-16 - further optimizations of the task scheduler execute loop
//
// v1.8.5:
//    2015-11-23 - bug fix: incorrect calculation of next task invocation in case callback changed the interval
//    2015-11-23 - bug fix: Task::set() method calls setInterval() explicitly, therefore delaying the task in the same manner
//
// v1.9.0:
//    2015-11-24 - packed three byte-long status variables into bit array structure data type - saving 2 bytes per each task instance
//
// v1.9.2:
//    2015-11-28 - _TASK_ROLLOVER_FIX is deprecated (not necessary)
//    2015-12-16 - bug fixes: automatic millis rollover support for delay methods
//    2015-12-17 - new method for _TASK_TIMECRITICAL option: getStartDelay()
//
// v2.0.0:
//    2015-12-22 - _TASK_PRIORITY - support for layered task prioritization
//
// v2.0.1:
//    2016-01-02 - bug fix: issue#11 Xtensa compiler (esp8266): Declaration of constructor does not match implementation
//
// v2.0.2:
//    2016-01-05 - bug fix: time constants wrapped inside compile option
//    2016-01-05 - support for ESP8266 wifi power saving mode for _TASK_SLEEP_ON_IDLE_RUN compile option
//
// v2.1.0:
//    2016-02-01 - support for microsecond resolution
//    2016-02-02 - added Scheduler baseline start time reset method: startNow()
//
// v2.2.0:
//    2016-11-17 - all methods made 'inline' to support inclusion of TaskSchedule.h file into other header files
//
// v2.2.1:
//    2016-11-30 - inlined constructors. Added "yield()" and "yieldOnce()" functions to easily break down and chain
//                 back together long running callback methods
//    2016-12-16 - added "getCount()" to StatusRequest objects, made every task StatusRequest enabled.
//                 Internal StatusRequest objects are accessible via "getInternalStatusRequest()" method.
//
// v2.3.0:
//    2017-02-24 - new timeUntilNextIteration() method within Scheduler class - inquire when a particlar task is
//                 scheduled to run next time
//
// v2.4.0:
//    2017-04-27 - added destructor to the Task class to ensure tasks are disables and taken off the execution chain
//                 upon destruction. (Contributed by Edwin van Leeuwen [BlackEdder - https://github.com/BlackEdder)
//
// v2.7.2:
//    2017-04-27 - ESP8266 ONLY: added optional support for std::functions via _TASK_STD_FUNCTION compilation option
//                 (Contributed by Edwin van Leeuwen [BlackEdder - https://github.com/BlackEdder)
//    2017-08-30 - add _TASK_DEBUG making all methods and variables public FOR DEBUGGING PURPOSES ONLY!
//                 Use at your own risk!
//    2017-08-30 - bug fix: Scheduler::addTask() checks if task is already part of an execution chain (github issue #37)
//    2017-08-30 - support for multi-tab sketches (Contributed by Adam Ryczkowski - https://github.com/adamryczkowski)
//
// v2.5.1:
//    2018-01-06 - support for IDLE sleep on Teensy boards (tested on Teensy 3.5)
//
// v2.5.2:
//    2018-01-09 - _TASK_INLINE compilation directive making all methods declared "inline" (issue #42)
//
// v2.6.0:
//    2018-01-30 - _TASK_TIMEOUT compilation directive: Task overall timeout functionality
//    2018-01-30 - ESP32 support (experimental)
//                 (Contributed by Marco Tombesi: https://github.com/baggior)
//
// v2.6.1:
//    2018-02-13 - Bug: support for task self-destruction in the OnDisable method
//                 Example 19: dynamic tasks creation and destruction
//    2018-03-14 - Bug: high level scheduler ignored if lower level chain is empty
//                 Example 20: use of local task storage to work with task-specific class objects
//
// v3.0.0:
//    2018-03-15 - Major Release: Support for dynamic callback methods binding via compilation parameter _TASK_OO_CALLBACKS
//
// v3.0.1:
//    2018-11-09 - bug: task deleted from the execution chain cannot be added back (github issue #67)
//
// v3.0.2:
//    2018-11-11 - bug: default constructor is ambiguous when Status Request objects are enabled (github issue #65 & #68)
#if defined(ESP8266) || defined(ESP32)
#include <Arduino.h>
#include "TaskSchedulerDeclarations.h"

#ifndef _TASKSCHEDULER_H_
#define _TASKSCHEDULER_H_

// ----------------------------------------
// The following "defines" control library functionality at compile time,
// and should be used in the main sketch depending on the functionality required
//
// #define _TASK_TIMECRITICAL      // Enable monitoring scheduling overruns
// #define _TASK_SLEEP_ON_IDLE_RUN // Enable 1 ms SLEEP_IDLE powerdowns between tasks if no callback methods were invoked during the pass
// #define _TASK_STATUS_REQUEST    // Compile with support for StatusRequest functionality - triggering tasks on status change events in addition to time only
// #define _TASK_WDT_IDS           // Compile with support for wdt control points and task ids
// #define _TASK_LTS_POINTER       // Compile with support for local task storage pointer
// #define _TASK_PRIORITY          // Support for layered scheduling priority
// #define _TASK_MICRO_RES         // Support for microsecond resolution
// #define _TASK_STD_FUNCTION      // Support for std::function (ESP8266 ONLY)
// #define _TASK_DEBUG             // Make all methods and variables public for debug purposes
// #define _TASK_INLINE		       // Make all methods "inline" - needed to support some multi-tab, multi-file implementations
// #define _TASK_TIMEOUT           // Support for overall task timeout
// #define _TASK_OO_CALLBACKS      // Support for callbacks via inheritance

 #ifdef _TASK_MICRO_RES

 #undef _TASK_SLEEP_ON_IDLE_RUN     // SLEEP_ON_IDLE has only millisecond resolution
 #define _TASK_TIME_FUNCTION() micros()

 #else
 #define _TASK_TIME_FUNCTION() millis()

 #endif  // _TASK_MICRO_RES


#ifdef _TASK_SLEEP_ON_IDLE_RUN

#ifdef ARDUINO_ARCH_AVR
#include <avr/sleep.h>
#include <avr/power.h>
#endif  // ARDUINO_ARCH_AVR

#ifdef ARDUINO_ARCH_ESP8266
#define _TASK_ESP8266_DLY_THRESHOLD 200L
extern "C" {
#include "user_interface.h"
}
#endif //ARDUINO_ARCH_ESP8266

#ifdef ARDUINO_ARCH_ESP32
#define _TASK_ESP8266_DLY_THRESHOLD 200L
#warning _TASK_SLEEP_ON_IDLE_RUN for ESP32 cannot use light sleep mode but a standard delay for 1 ms
#endif  // ARDUINO_ARCH_ESP32

#ifdef ARDUINO_ARCH_STM32F1
#include <libmaple/pwr.h>
#include <libmaple/scb.h>
#endif  // ARDUINO_ARCH_STM32F1

#endif  // _TASK_SLEEP_ON_IDLE_RUN


#if !defined (ARDUINO_ARCH_ESP8266) && !defined (ARDUINO_ARCH_ESP32)
#ifdef _TASK_STD_FUNCTION
    #error Support for std::function only for ESP8266 or ESP32 architecture
#undef _TASK_STD_FUNCTION
#endif // _TASK_STD_FUNCTION
#endif // ARDUINO_ARCH_ESP8266

#ifdef _TASK_WDT_IDS
    static unsigned int __task_id_counter = 0; // global task ID counter for assiging task IDs automatically.
#endif  // _TASK_WDT_IDS

#ifdef _TASK_PRIORITY
    Scheduler* iCurrentScheduler;
#endif // _TASK_PRIORITY


// ------------------ TaskScheduler implementation --------------------


/** Constructor, uses default values for the parameters
 * so could be called with no parameters.
 */
#ifdef _TASK_OO_CALLBACKS
Task::Task( unsigned long aInterval, long aIterations, Scheduler* aScheduler, bool aEnable ) {
    reset();
    set(aInterval, aIterations);
#else
Task::Task( unsigned long aInterval, long aIterations, TaskCallback aCallback, Scheduler* aScheduler, bool aEnable, TaskOnEnable aOnEnable, TaskOnDisable aOnDisable ) {
    reset();
    set(aInterval, aIterations, aCallback, aOnEnable, aOnDisable);
#endif

    if (aScheduler) aScheduler->addTask(*this);

#ifdef _TASK_WDT_IDS
    iTaskID = ++__task_id_counter;
#endif  // _TASK_WDT_IDS

    if (aEnable) enable();
}

/** Destructor.
 *  Makes sure the task disabled and deleted out of the chain
 *  prior to being deleted.
 */
Task::~Task() {
    disable();
    if (iScheduler)
        iScheduler->deleteTask(*this);
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
    iTaskID = ++__task_id_counter;
#endif  // _TASK_WDT_IDS
}


StatusRequest::StatusRequest()
{
    iCount = 0;
    iStatus = 0;
}

void StatusRequest::setWaiting(unsigned int aCount) { iCount = aCount; iStatus = 0; }
bool StatusRequest::pending() { return (iCount != 0); }
bool StatusRequest::completed() { return (iCount == 0); }
int StatusRequest::getStatus() { return iStatus; }
int StatusRequest::getCount() { return iCount; }
StatusRequest* Task::getStatusRequest() { return iStatusRequest; }
StatusRequest* Task::getInternalStatusRequest() { return &iMyStatusRequest; }

/** Signals completion of the StatusRequest by one of the participating events
 *  @param: aStatus - if provided, sets the return code of the StatusRequest: negative = error, 0 (default) = OK, positive = OK with a specific status code
 *  Negative status will complete Status Request fully (since an error occured).
 *  @return: true, if StatusRequest is complete, false otherwise (still waiting for other events)
 */
bool StatusRequest::signal(int aStatus) {
    if ( iCount) {  // do not update the status request if it was already completed
        if (iCount > 0)  --iCount;
        if ( (iStatus = aStatus) < 0 ) iCount = 0;   // if an error is reported, the status is requested to be completed immediately
    }
    return (iCount == 0);
}

void StatusRequest::signalComplete(int aStatus) {
    if (iCount) { // do not update the status request if it was already completed
        iCount = 0;
        iStatus = aStatus;
    }
}

/** Sets a Task to wait until a particular event completes
 *  @param: aStatusRequest - a pointer for the StatusRequest to wait for.
 *  If aStatusRequest is NULL, request for waiting is ignored, and the waiting task is not enabled.
 */
void Task::waitFor(StatusRequest* aStatusRequest, unsigned long aInterval, long aIterations) {
	iStatusRequest = aStatusRequest;
    if ( iStatusRequest != NULL ) { // assign internal StatusRequest var and check if it is not NULL
        setIterations(aIterations);
        setInterval(aInterval);
        iStatus.waiting = _TASK_SR_NODELAY;  // no delay
        enable();
    }
}

void Task::waitForDelayed(StatusRequest* aStatusRequest, unsigned long aInterval, long aIterations) {
	iStatusRequest = aStatusRequest;
    if ( iStatusRequest != NULL ) { // assign internal StatusRequest var and check if it is not NULL
        setIterations(aIterations);
        if ( aInterval ) setInterval(aInterval);  // For the dealyed version only set the interval if it was not a zero
        iStatus.waiting = _TASK_SR_DELAY;  // with delay equal to the current interval
        enable();
    }
}
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
    iPreviousMillis = 0;
    iInterval = iDelay = 0;
    iPrev = NULL;
    iNext = NULL;
    iScheduler = NULL;
    iRunCounter = 0;

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

#ifdef	_TASK_TIMEOUT
	iTimeout = 0;
	iStarttime = 0;
	iStatus.timeout = false;
#endif  // _TASK_TIMEOUT
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
    iSetIterations = iIterations = aIterations;
}

/** Sets number of iterations for the task
 * if task is enabled, schedule for immediate execution
 * @param aIterations - number of iterations, use -1 for no limit
 */
void Task::setIterations(long aIterations) {
    iSetIterations = iIterations = aIterations;
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
    iRunCounter--;
    if ( iIterations >= 0 ) iIterations++;
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
void Task::enable() {
    if (iScheduler) { // activation without active scheduler does not make sense
        iRunCounter = 0;

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

        iPreviousMillis = _TASK_TIME_FUNCTION() - (iDelay = iInterval);

#ifdef _TASK_TIMEOUT
			resetTimeout();
#endif // _TASK_TIMEOUT

#ifdef _TASK_STATUS_REQUEST
        if ( iStatus.enabled ) {
            iMyStatusRequest.setWaiting();
        }
#endif // _TASK_STATUS_REQUEST
    }
}

/** Enables the task only if it was not enabled already
 * Returns previous state (true if was already enabled, false if was not)
 */
bool Task::enableIfNot() {
    bool previousEnabled = iStatus.enabled;
    if ( !previousEnabled ) enable();
    return (previousEnabled);
}

/** Enables the task
 * and schedules it for execution after a delay = aInterval
 */
void Task::enableDelayed(unsigned long aDelay) {
    enable();
    delay(aDelay);
}

#ifdef _TASK_TIMEOUT
void Task::setTimeout(unsigned long aTimeout, bool aReset) {
	iTimeout = aTimeout;
	if (aReset) resetTimeout();
}

void Task::resetTimeout() {
	iStarttime = _TASK_TIME_FUNCTION();
	iStatus.timeout = false;
}

unsigned long Task::getTimeout() {
	return iTimeout;
}

long Task::untilTimeout() {
    if ( iTimeout ) {
        return ( (long) (iStarttime + iTimeout) - (long) _TASK_TIME_FUNCTION() );
	}
	return -1;
}

bool Task::timedOut() {
	return iStatus.timeout;
}

#endif // _TASK_TIMEOUT



/** Delays Task for execution after a delay = aInterval (if task is enabled).
 * leaves task enabled or disabled
 * if aDelay is zero, delays for the original scheduling interval from now
 */
void Task::delay(unsigned long aDelay) {
//  if (!aDelay) aDelay = iInterval;
    iDelay = aDelay ? aDelay : iInterval;
    iPreviousMillis = _TASK_TIME_FUNCTION(); // - iInterval + aDelay;
}

/** Schedules next iteration of Task for execution immediately (if enabled)
 * leaves task enabled or disabled
 * Task's original schedule is shifted, and all subsequent iterations will continue from this point in time
 */
void Task::forceNextIteration() {
    iPreviousMillis = _TASK_TIME_FUNCTION() - (iDelay = iInterval);
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

/** Disables task
 * Task will no longer be executed by the scheduler
 * Returns status of the task before disable was called (i.e., if the task was already disabled)
 */

bool Task::disable() {
    bool previousEnabled = iStatus.enabled;
    iStatus.enabled = false;
    iStatus.inonenable = false;

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
#ifdef _TASK_STATUS_REQUEST
    iMyStatusRequest.signalComplete();
#endif
    return (previousEnabled);
}

/** Restarts task
 * Task will run number of iterations again
 */

void Task::restart() {
    enable();
	iIterations = iSetIterations;
}

/** Restarts task delayed
 * Task will run number of iterations again
 */
void Task::restartDelayed(unsigned long aDelay) {
    enableDelayed(aDelay);
	iIterations = iSetIterations;
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
    iFirst = NULL;
    iLast = NULL;
    iCurrent = NULL;

#ifdef _TASK_PRIORITY
    iHighPriority = NULL;
#endif  // _TASK_PRIORITY

#ifdef _TASK_SLEEP_ON_IDLE_RUN
    allowSleep(true);
#endif  // _TASK_SLEEP_ON_IDLE_RUN
}

/** Appends task aTask to the tail of the execution chain.
 * @param &aTask - reference to the Task to be appended.
 * @note Task can only be part of the chain once.
 */
 void Scheduler::addTask(Task& aTask) {

// Avoid adding task twice to the same scheduler
    if (aTask.iScheduler == this)
        return;

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
}

/** Deletes specific Task from the execution chain
 * @param &aTask - reference to the task to be deleted from the chain
 */
void Scheduler::deleteTask(Task& aTask) {
	aTask.iScheduler = NULL;
    if (aTask.iPrev == NULL) {
        if (aTask.iNext == NULL) {
            iFirst = NULL;
            iLast = NULL;
            return;
        }
        else {
            aTask.iNext->iPrev = NULL;
            iFirst = aTask.iNext;
            aTask.iNext = NULL;
            return;
        }
    }

    if (aTask.iNext == NULL) {
        aTask.iPrev->iNext = NULL;
        iLast = aTask.iPrev;
        aTask.iPrev = NULL;
        return;
    }

    aTask.iPrev->iNext = aTask.iNext;
    aTask.iNext->iPrev = aTask.iPrev;
    aTask.iPrev = NULL;
    aTask.iNext = NULL;
}

/** Disables all tasks in the execution chain
 * Convenient for error situations, when the only
 * task remaining active is an error processing task
 * @param aRecursive - if true, tasks of the higher priority chains are disabled as well recursively
 */
void Scheduler::disableAll(bool aRecursive) {
    Task    *current = iFirst;
    while (current) {
        current->disable();
        current = current->iNext;
    }

#ifdef _TASK_PRIORITY
    if (aRecursive && iHighPriority) iHighPriority->disableAll(true);
#endif  // _TASK_PRIORITY
}


/** Enables all the tasks in the execution chain
 * @param aRecursive - if true, tasks of the higher priority chains are enabled as well recursively
 */
 void Scheduler::enableAll(bool aRecursive) {
    Task    *current = iFirst;
    while (current) {
        current->enable();
        current = current->iNext;
    }

#ifdef _TASK_PRIORITY
    if (aRecursive && iHighPriority) iHighPriority->enableAll(true);
#endif  // _TASK_PRIORITY

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

#ifdef ARDUINO_ARCH_ESP8266
    wifi_set_sleep_type( iAllowSleep ? LIGHT_SLEEP_T : NONE_SLEEP_T );
#endif  // ARDUINO_ARCH_ESP8266

#ifdef ARDUINO_ARCH_ESP32
	// TO-DO; find a suitable replacement for ESP32 if possible.
#endif  // ARDUINO_ARCH_ESP32
}
#endif  // _TASK_SLEEP_ON_IDLE_RUN


void Scheduler::startNow( bool aRecursive ) {
    unsigned long t = _TASK_TIME_FUNCTION();

    iCurrent = iFirst;
    while (iCurrent) {
        if ( iCurrent->iStatus.enabled ) iCurrent->iPreviousMillis = t - iCurrent->iDelay;
        iCurrent = iCurrent->iNext;
    }

#ifdef _TASK_PRIORITY
    if (aRecursive && iHighPriority) iHighPriority->startNow( true );
#endif  // _TASK_PRIORITY
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

    long d = (long) aTask.iDelay - ( (long) ((_TASK_TIME_FUNCTION() - aTask.iPreviousMillis)) );

    if ( d < 0 )
        return (0); // Task will run as soon as possible
    return ( d );
}

Task& Scheduler::currentTask() { return *iCurrent; }

#ifdef _TASK_LTS_POINTER
void* Scheduler::currentLts() { return iCurrent->iLTS; }
#endif  // _TASK_LTS_POINTER
#ifdef _TASK_TIMECRITICAL
bool Scheduler::isOverrun() { return (iCurrent->iOverrun < 0); }
#endif  // _TASK_TIMECRITICAL



/** Makes one pass through the execution chain.
 * Tasks are executed in the order they were added to the chain
 * There is no concept of priority
 * Different pseudo "priority" could be achieved
 * by running task more frequently
 */
bool Scheduler::execute() {
    bool     idleRun = true;
    register unsigned long m, i;  // millis, interval;

#ifdef _TASK_SLEEP_ON_IDLE_RUN
#if defined (ARDUINO_ARCH_ESP8266) || defined (ARDUINO_ARCH_ESP32)
      unsigned long t1 = micros();
      unsigned long t2 = 0;
#endif  // ARDUINO_ARCH_ESP8266
#endif // _TASK_SLEEP_ON_IDLE_RUN

	Task *nextTask;  // support for deleting the task in the onDisable method
    iCurrent = iFirst;

#ifdef _TASK_PRIORITY
    // If lower priority scheduler does not have a single task in the chain
    // the higher priority scheduler still has to have a chance to run
        if (!iCurrent && iHighPriority) iHighPriority->execute();
        iCurrentScheduler = this;
#endif  // _TASK_PRIORITY


    while (iCurrent) {

#ifdef _TASK_PRIORITY
    // If scheduler for higher priority tasks is set, it's entire chain is executed on every pass of the base scheduler
        if (iHighPriority) idleRun = iHighPriority->execute() && idleRun;
        iCurrentScheduler = this;
#endif  // _TASK_PRIORITY
		nextTask = iCurrent->iNext;
        do {
            if ( iCurrent->iStatus.enabled ) {

#ifdef _TASK_WDT_IDS
    // For each task the control points are initialized to avoid confusion because of carry-over:
                iCurrent->iControlPoint = 0;
#endif  // _TASK_WDT_IDS

    // Disable task on last iteration:
                if (iCurrent->iIterations == 0) {
                    iCurrent->disable();
                    break;
                }
                m = _TASK_TIME_FUNCTION();
                i = iCurrent->iInterval;

#ifdef _TASK_TIMEOUT
	// Disable task on a timeout
				if ( iCurrent->iTimeout && (m - iCurrent->iStarttime > iCurrent->iTimeout) ) {
					iCurrent->iStatus.timeout = true;
					iCurrent->disable();
                    break;
				}
#endif // _TASK_TIMEOUT

#ifdef  _TASK_STATUS_REQUEST
    // If StatusRequest object was provided, and still pending, and task is waiting, this task should not run
    // Otherwise, continue with execution as usual.  Tasks waiting to StatusRequest need to be rescheduled according to
    // how they were placed into waiting state (waitFor or waitForDelayed)
                if ( iCurrent->iStatus.waiting ) {
                    if ( (iCurrent->iStatusRequest)->pending() ) break;
                    if (iCurrent->iStatus.waiting == _TASK_SR_NODELAY) {
                        iCurrent->iPreviousMillis = m - (iCurrent->iDelay = i);
                    }
                    else {
                        iCurrent->iPreviousMillis = m;
                    }
                    iCurrent->iStatus.waiting = 0;
                }
#endif  // _TASK_STATUS_REQUEST

                if ( m - iCurrent->iPreviousMillis < iCurrent->iDelay ) break;

                if ( iCurrent->iIterations > 0 ) iCurrent->iIterations--;  // do not decrement (-1) being a signal of never-ending task
                iCurrent->iRunCounter++;
                iCurrent->iPreviousMillis += iCurrent->iDelay;

#ifdef _TASK_TIMECRITICAL
    // Updated_previous+current interval should put us into the future, so iOverrun should be positive or zero.
    // If negative - the task is behind (next execution time is already in the past)
                unsigned long p = iCurrent->iPreviousMillis;
                iCurrent->iOverrun = (long) ( p + i - m );
                iCurrent->iStartDelay = (long) ( m - p );
#endif  // _TASK_TIMECRITICAL

                iCurrent->iDelay = i;

#ifdef _TASK_OO_CALLBACKS
                idleRun = !iCurrent->Callback();
#else
                if ( iCurrent->iCallback ) {
                    iCurrent->iCallback();
                    idleRun = false;
                }
#endif // _TASK_OO_CALLBACKS

            }
        } while (0);    //guaranteed single run - allows use of "break" to exit
        iCurrent = nextTask;
#if defined (ARDUINO_ARCH_ESP8266) || defined (ARDUINO_ARCH_ESP32)
        yield();
#endif  // ARDUINO_ARCH_ESP8266
    }

#ifdef _TASK_SLEEP_ON_IDLE_RUN
    if (idleRun && iAllowSleep) {

#ifdef ARDUINO_ARCH_AVR // Could be used only for AVR-based boards.
      set_sleep_mode(SLEEP_MODE_IDLE);
      sleep_enable();
      /* Now enter sleep mode. */
      sleep_mode();

      /* The program will continue from here after the timer timeout ~1 ms */
      sleep_disable(); /* First thing to do is disable sleep. */
#endif // ARDUINO_ARCH_AVR

#ifdef CORE_TEENSY
    asm("wfi");
#endif //CORE_TEENSY

#ifdef ARDUINO_ARCH_ESP8266
// to do: find suitable sleep function for esp8266
      t2 = micros() - t1;
      if (t2 < _TASK_ESP8266_DLY_THRESHOLD) delay(1);   // ESP8266 implementation of delay() uses timers and yield
#endif  // ARDUINO_ARCH_ESP8266

#ifdef ARDUINO_ARCH_ESP32
//TODO: find a correct light sleep implementation for ESP32
    // esp_sleep_enable_timer_wakeup(1000); //1 ms
    // int ret= esp_light_sleep_start();
      t2 = micros() - t1;
      if (t2 < _TASK_ESP8266_DLY_THRESHOLD) delay(1);
#endif  // ARDUINO_ARCH_ESP32

#ifdef ARDUINO_ARCH_STM32F1
	  // Now go into stop mode, wake up on interrupt.
	  // Systick interrupt will run every 1 milliseconds.
	  asm("    wfi");
#endif  // ARDUINO_ARCH_STM32

    }
#endif  // _TASK_SLEEP_ON_IDLE_RUN

    return (idleRun);
}



#endif /* _TASKSCHEDULER_H_ */
#endif
