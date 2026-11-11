execute_process(COMMAND "${CMAKE_COMMAND}" --build "${BUILD_DIR}"
    --config "${CONFIG}" --target "${TARGET}" --parallel 1
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
set(log "${output}\n${error}")
file(WRITE "${BUILD_DIR}/${TARGET}.expected-failure.log" "${log}")
if(NOT result MATCHES "^[1-9][0-9]*$" OR NOT log MATCHES "${EXPECT}")
    message(FATAL_ERROR "Missing expected rejection ${EXPECT}: ${result}\n${log}")
endif()
if(EXPECT MATCHES "^(blinker_r4_rpc_faulted|__wrap__sbrk)$" AND
   NOT log MATCHES "(LNK2019|LNK2001|undefined reference|undefined symbol)")
    message(FATAL_ERROR "Missing strong-symbol linker rejection: ${log}")
endif()
message(STATUS "${TARGET}: rejected for ${EXPECT}")
