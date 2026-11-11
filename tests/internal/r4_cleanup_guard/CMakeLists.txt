cmake_minimum_required(VERSION 3.13)
project(blinker_internal_r4_cleanup_guard LANGUAGES CXX)
enable_testing()

foreach(input R4_CORE PUBSUB_SOURCE)
    if(NOT DEFINED ${input})
        message(FATAL_ERROR "Supply the untouched, hash-bound ${input}")
    endif()
endforeach()
set(wifi "${R4_CORE}/libraries/WiFiS3/src")
foreach(input "${wifi}/WiFiClient.cpp" "${wifi}/WiFi.cpp" "${PUBSUB_SOURCE}/PubSubClient.cpp")
    if(NOT EXISTS "${input}")
        message(FATAL_ERROR "Missing original dependency ${input}")
    endif()
endforeach()

# Compile complete original SDK/MQTT sources. Only the Arduino ABI and Modem
# implementation are Host seams; Modem.h/commands/types/FifoBuffer stay original.
add_executable(r4_cleanup_guard test_main.cpp modem_seam.cpp portable_support.cpp
    "${wifi}/WiFiClient.cpp" "${wifi}/WiFi.cpp" "${PUBSUB_SOURCE}/PubSubClient.cpp")
target_compile_features(r4_cleanup_guard PRIVATE cxx_std_11)
target_compile_definitions(r4_cleanup_guard PRIVATE ARDUINO_ARCH_RENESAS_UNO ARDUINO_UNOWIFIR4)
target_include_directories(r4_cleanup_guard PRIVATE stubs "${wifi}"
    "${PUBSUB_SOURCE}" "${R4_CORE}/cores/arduino" "${CMAKE_CURRENT_SOURCE_DIR}/../../../src")
if(MSVC)
    target_compile_options(r4_cleanup_guard PRIVATE /W4 /showIncludes)
else()
    target_compile_options(r4_cleanup_guard PRIVATE -Wall -Wextra -H)
endif()
foreach(case native-positive client-fault station-connecting-fault station-connected-fault
        http-stop-fault http-fail-fault mqtt-disconnect-fault client-destruction-fault)
    add_test(NAME r4_cleanup_${case} COMMAND r4_cleanup_guard "${case}")
endforeach()
foreach(case ipv4-format ipv6-route ipv4-busy ipv4-fault)
    add_test(NAME r4_connect_${case} COMMAND r4_cleanup_guard "${case}")
endforeach()
