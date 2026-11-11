cmake_minimum_required(VERSION 3.13)
project(blinker_wio_rpc_deadline_tests LANGUAGES C CXX)
enable_testing()
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(WIO_RPC_DIR "" CACHE PATH "Read-only pinned rpcUnified 2.1.4 library")
set(WIO_RPC_OVERLAY "" CACHE PATH "Fresh verified deadline-overlay directory")
set(WIO_CORE_ROOT "" CACHE PATH "Read-only original SAMD1.8.5 core root")
if(NOT EXISTS "${WIO_RPC_DIR}/src/erpc/erpc_client_manager.cpp" OR
   NOT EXISTS "${WIO_RPC_OVERLAY}/overlay.json" OR
   NOT EXISTS "${WIO_CORE_ROOT}/cores/arduino/RingBuffer.h")
    message(FATAL_ERROR "Explicit original SDK/dependency and verified overlay required")
endif()

# Whole original source projection prevents quoted includes escaping back into
# the SDK. The pending/client/transport algorithms are the generated real units.
# Only threading, registers, Arduino scheduling and physical I/O are Host seams.
set(PROJECTION "${CMAKE_BINARY_DIR}/projection")
file(COPY "${WIO_RPC_DIR}/src/" DESTINATION "${PROJECTION}")
file(GLOB_RECURSE generated RELATIVE "${WIO_RPC_OVERLAY}" "${WIO_RPC_OVERLAY}/*")
foreach(name IN LISTS generated)
    if(NOT IS_DIRECTORY "${WIO_RPC_OVERLAY}/${name}" AND NOT name STREQUAL "overlay.json")
        configure_file("${WIO_RPC_OVERLAY}/${name}" "${PROJECTION}/${name}" COPYONLY)
    endif()
endforeach()
foreach(name IN ITEMS Arduino.h HardwareSerial.h SERCOM.h erpc_threading.h wiring_private.h)
    configure_file("stubs/${name}" "${PROJECTION}/erpc/${name}" COPYONLY)
endforeach()
configure_file("${WIO_CORE_ROOT}/cores/arduino/RingBuffer.h" "${PROJECTION}/erpc/RingBuffer.h" COPYONLY)

# Compile the exact generated runServer function against a counter-only server
# seam, not the complete init TU or a simulated firmware runtime.
file(READ "${WIO_RPC_OVERLAY}/erpc_Unified_init.cpp" init_text)
string(FIND "${init_text}" "void runServer(void *arg)" server_start)
string(FIND "${init_text}" "Thread serverThread(" server_end)
if(server_start LESS 0 OR server_end LESS server_start)
    message(FATAL_ERROR "Generated runServer excerpt anchors missing")
endif()
math(EXPR server_size "${server_end} - ${server_start}")
string(SUBSTRING "${init_text}" ${server_start} ${server_size} server_excerpt)
file(WRITE "${PROJECTION}/erpc/run_server_excerpt.inc" "${server_excerpt}")
execute_process(COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/extract_bridges.py"
    --projection "${PROJECTION}" --client-tool "${REPO_ROOT}/tools/wio_rpc_deadline_client.py"
    RESULT_VARIABLE bridge_result OUTPUT_VARIABLE bridge_excerpt ERROR_VARIABLE bridge_stderr)
file(WRITE "${CMAKE_BINARY_DIR}/bridge-extraction.log" "${bridge_stderr}")
if(NOT bridge_result EQUAL 0)
    message(FATAL_ERROR "Actual C bridge extraction failed; preserve bridge-extraction.log")
endif()
file(WRITE "${PROJECTION}/erpc/actual_bridge_excerpt.inc" "${bridge_excerpt}")

add_executable(wio_rpc_transport transport_fixture.cpp host_runtime.cpp
    "${PROJECTION}/erpc/erpc_arduino_uart_transport.cpp"
    "${PROJECTION}/erpc/erpc_framed_transport.cpp"
    "${PROJECTION}/erpc/erpc_message_buffer.cpp"
    "${PROJECTION}/erpc/erpc_crc16.cpp")
add_executable(wio_rpc_client client_fixture.cpp host_runtime.cpp
    "${PROJECTION}/erpc/erpc_client_manager.cpp"
    "${PROJECTION}/erpc/erpc_transport_arbitrator.cpp"
    "${PROJECTION}/erpc/erpc_arbitrated_client_manager.cpp"
    "${PROJECTION}/erpc/erpc_basic_codec.cpp"
    "${PROJECTION}/erpc/erpc_message_buffer.cpp"
    "${PROJECTION}/erpc/erpc_message_loggers.cpp"
    "${PROJECTION}/erpc/rpc_wifi_api_client.cpp"
    "${PROJECTION}/erpc/rpc_system_client.cpp")
foreach(target IN ITEMS wio_rpc_transport wio_rpc_client)
    if(TARGET ${target})
        target_include_directories(${target} PRIVATE "${PROJECTION}" "${PROJECTION}/erpc")
        target_compile_definitions(${target} PRIVATE SEEED_WIO_TERMINAL=1
            BLINKER_ENABLE_TASKS=1 BLINKER_INTERNAL_WIO_TASKS=1
            BLINKER_INTERNAL_WIO_TASK_REGION_RESERVED=1 BLINKER_TASK_CAPACITY=4)
        if(MSVC)
            target_compile_options(${target} PRIVATE /W3)
        else()
            target_compile_options(${target} PRIVATE -Wall -Wextra)
            target_link_libraries(${target} PRIVATE pthread)
        endif()
    endif()
endforeach()
foreach(case IN ITEMS normal-generated whole-budget-final-err encode-expired allocation-failed
        postfault-no-allocate binary-failure string-failure caller-server caller-irq caller-masked
        reentry bridge-getpeer-short bridge-sockopt-short bridge-read-oversize bridge-dns-short bridge-errno
        exact-deadline-late-reply sequence-wrap)
    add_test(NAME wio_client_${case} COMMAND wio_rpc_client ${case})
    set_tests_properties(wio_client_${case} PROPERTIES TIMEOUT 15)
endforeach()
foreach(case IN ITEMS wire-normal idle-first-byte partial-frame-deadline whole-call-deadline
        root-lock-budget local-lock-budget tx-full raw-tx-full zero-write short-write
        crc-fault oversized-frame quarantine-order late-irq park caller-context clock-wrap)
    add_test(NAME wio_transport_${case} COMMAND wio_rpc_transport ${case})
    set_tests_properties(wio_transport_${case} PROPERTIES TIMEOUT 15)
endforeach()
