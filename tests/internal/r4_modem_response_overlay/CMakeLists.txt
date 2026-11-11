cmake_minimum_required(VERSION 3.13)
project(blinker_internal_r4_modem_response LANGUAGES CXX)
enable_testing()

if(NOT DEFINED R4_MODEM_OVERLAY OR NOT EXISTS "${R4_MODEM_OVERLAY}/overlay.json")
    message(FATAL_ERROR "Supply a fresh hash-bound prepared R4_MODEM_OVERLAY")
endif()
if(NOT DEFINED R4_MODEM_SOURCE)
    message(FATAL_ERROR "R4_MODEM_SOURCE must name the original SDK libraries/WiFiS3/src")
endif()
foreach(bound COMMAND RESPONSE)
    if(NOT DEFINED R4_MODEM_${bound}_BYTES)
        message(FATAL_ERROR "Both internal bounds must be explicit; there is no default")
    endif()
endforeach()
if(NOT DEFINED R4_MODEM_RPC_MS)
    message(FATAL_ERROR "The internal whole-RPC budget must be explicit")
endif()
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(overlay_verifier "${CMAKE_CURRENT_SOURCE_DIR}/../../../tools/prepare_r4_modem_response_overlay.py")
set(overlay_check "${Python3_EXECUTABLE}" "${overlay_verifier}"
    --modem-dir "${R4_MODEM_SOURCE}" --output "${R4_MODEM_OVERLAY}" --verify-only)
execute_process(COMMAND ${overlay_check} RESULT_VARIABLE overlay_result)
if(NOT overlay_result EQUAL 0)
    message(FATAL_ERROR "Prepared R4 Modem overlay verification failed")
endif()

add_executable(r4_modem_response_tests test_main.cpp "${R4_MODEM_OVERLAY}/Modem.cpp")
add_custom_target(r4_modem_overlay_verify COMMAND ${overlay_check} VERBATIM)
add_dependencies(r4_modem_response_tests r4_modem_overlay_verify)
target_compile_features(r4_modem_response_tests PRIVATE cxx_std_11)
target_compile_definitions(r4_modem_response_tests PRIVATE ARDUINO_UNOWIFIR4
    BLINKER_INTERNAL_R4_MODEM_COMMAND_BYTES=${R4_MODEM_COMMAND_BYTES}
    BLINKER_INTERNAL_R4_MODEM_RESPONSE_BYTES=${R4_MODEM_RESPONSE_BYTES}
    BLINKER_INTERNAL_R4_MODEM_RPC_MS=${R4_MODEM_RPC_MS})
target_include_directories(r4_modem_response_tests PRIVATE
    stubs "${R4_MODEM_OVERLAY}" "${CMAKE_CURRENT_SOURCE_DIR}/../../../src")
if(MSVC)
    target_compile_options(r4_modem_response_tests PRIVATE /W4)
else()
    target_compile_options(r4_modem_response_tests PRIVATE -Wall -Wextra)
endif()
# The two limits are a synthetic ordinary-manual-WiFi experiment, not scan/TLS
# or task admission. Keep the inherited parser warnings visible, not -Werror.
foreach(case normal binary-exact sized-zero command-exact long-command
        unterminated-data untrimmed-exact untrimmed-over-cap
        untrimmed-sized-no-space untrimmed-sized-leading-space sized-prefix-boundaries
        header-payload-normal header-payload-over-cap
        sized-negative sized-overflow sized-over-cap sized-nondigit wrong-command-data
        zero-budget owner-deadline tx-deadline rx-deadline rx-owner-fault final-byte-deadline short-write
        header-payload-deadline header-rpc-interleave format-overflow)
    add_test(NAME r4_modem_${case} COMMAND r4_modem_response_tests "${case}")
endforeach()
