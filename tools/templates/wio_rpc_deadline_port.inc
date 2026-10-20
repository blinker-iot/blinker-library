// Internal, fixed Wio RPC graph only. Included once after ble_uart/clientThread.
static_assert(kErpcStatus_Success == 0 && kErpcStatus_Fail == 1 && kErpcStatus_Timeout == 5,
              "Pinned native eRPC status values must match the shared boundary");
namespace blinker { namespace wio_rpc { Boundary boundary; } }

extern "C" uint32_t blinker_wio_rpc_enter_critical()
{
    const uint32_t previous = __get_PRIMASK();
    __disable_irq();
    return previous;
}

extern "C" void blinker_wio_rpc_leave_critical(uint32_t previous)
{
    __set_PRIMASK(previous);
}

extern "C" uint32_t blinker_wio_rpc_now()
{
    return millis();
}

extern "C" void blinker_wio_rpc_pause()
{
    vTaskDelay(1);
}

extern "C" bool blinker_wio_rpc_caller_allowed()
{
    return __get_IPSR() == 0 && __get_PRIMASK() == 0 && __get_BASEPRI() == 0 &&
        xTaskGetSchedulerState() == taskSCHEDULER_RUNNING &&
        erpc::Thread::getCurrentThreadId() == clientThread.getThreadId();
}

extern "C" void blinker_wio_rpc_stop_uart()
{
    ble_uart.quarantine();
}

extern "C" int blinker_wio_rpc_faulted()
{
    return blinker::wio_rpc::boundary.faulted() ? 1 : 0;
}

extern "C" int blinker_wio_rpc_finalize(uint32_t sequence, int status)
{
    return blinker::wio_rpc::boundary.finalize(sequence, status);
}

extern "C" void blinker_wio_rpc_invalid_output()
{
    blinker::wio_rpc::boundary.trip(blinker::wio_rpc::Fault::Codec);
}
