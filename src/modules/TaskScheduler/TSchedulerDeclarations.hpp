// Cooperative multitasking library for Arduino
// Copyright (c) 2015-2019 Anatoli Arkhipenko

/*
    This is a namespace wrapper to avoid collision with frameworks having their
    own class named Scheduler (i.e. nordicnrf52)
    include it instead of <TaskSchedulerDeclarations.h> into your sketch and
    use Class name wrapper - 'TaskScheduler'
    i.e.
    TaskScheduler ts;
*/

#pragma once

namespace TS{
#include "TaskSchedulerDeclarations.h"
}

using TsScheduler = TS::Scheduler;
using TsTask = TS::Task;
#ifdef _TASK_STATUS_REQUEST
using TsStatusRequest = TS::StatusRequest;
#endif
#ifdef _TASK_SLEEP_ON_IDLE_RUN
using TsSleepCallback = TS::SleepCallback;
#endif
using TsTaskCallback = TS::TaskCallback;
using TsTaskOnDisable = TS::TaskOnDisable;
using TsTaskOnEnable = TS::TaskOnEnable;
