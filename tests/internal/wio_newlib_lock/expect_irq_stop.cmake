execute_process(COMMAND "${PROGRAM}" "${MODE}" TIMEOUT 1
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT "${result}" MATCHES "timeout" OR
   NOT "${output}" MATCHES "irq-context-entered" OR
   NOT "${output}" MATCHES "irq-mask-disabled" OR
   NOT "${output}" MATCHES "irq-failstop-loop" OR
   "${output}${error}" MATCHES "scheduler-call-in-irq")
    message(FATAL_ERROR "IRQ did not fail-stop before scheduler use: ${result}; ${output}; ${error}")
endif()
message(STATUS "${MODE}: production hook stopped before scheduler access; child terminated by test timeout")
