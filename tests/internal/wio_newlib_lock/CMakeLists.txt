cmake_minimum_required(VERSION 3.13)
project(blinker_wio_newlib_lock_tests LANGUAGES CXX)

enable_testing()
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(BLINKER_SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../src")
set(WIO_GRAPH SEEED_WIO_TERMINAL=1 BLINKER_ENABLE_TASKS=1
    BLINKER_INTERNAL_WIO_TASKS=1 BLINKER_TASK_CAPACITY=4
    BLINKER_INTERNAL_WIO_TASK_REGION_RESERVED=1)

function(configure_lock_target target)
    target_include_directories(${target} PRIVATE "${BLINKER_SOURCE_ROOT}"
        "${CMAKE_CURRENT_SOURCE_DIR}/stubs")
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Werror -pedantic)
    endif()
endfunction()

add_executable(wio_newlib_lock fixture.cpp provider.cpp)
configure_lock_target(wio_newlib_lock)
target_compile_definitions(wio_newlib_lock PRIVATE ${WIO_GRAPH})
foreach(mode IN ITEMS cold running nested suspended)
    add_test(NAME wio_newlib_${mode} COMMAND wio_newlib_lock ${mode})
endforeach()
foreach(mode IN ITEMS irq-lock irq-unlock)
    add_test(NAME wio_newlib_${mode}
        COMMAND "${CMAKE_COMMAND}" "-DPROGRAM=$<TARGET_FILE:wio_newlib_lock>"
            "-DMODE=${mode}" -P "${CMAKE_CURRENT_SOURCE_DIR}/expect_irq_stop.cmake")
endforeach()

# Compile all disabled configurations in the ordinary build. If the real inc
# emits any of its C exports, these deliberate duplicate definitions fail link.
foreach(mode IN ITEMS task0 no-task-macro non-wio)
    add_executable(wio_newlib_off_${mode} off.cpp provider.cpp)
    configure_lock_target(wio_newlib_off_${mode})
    if(mode STREQUAL "task0")
        target_compile_definitions(wio_newlib_off_${mode} PRIVATE
            SEEED_WIO_TERMINAL=1 BLINKER_ENABLE_TASKS=0)
    elseif(mode STREQUAL "no-task-macro")
        target_compile_definitions(wio_newlib_off_${mode} PRIVATE SEEED_WIO_TERMINAL=1)
    else()
        target_compile_definitions(wio_newlib_off_${mode} PRIVATE
            BLINKER_ENABLE_TASKS=1 BLINKER_INTERNAL_WIO_TASKS=1
            BLINKER_TASK_CAPACITY=4 BLINKER_INTERNAL_WIO_TASK_REGION_RESERVED=1)
    endif()
    add_test(NAME wio_newlib_off_${mode} COMMAND wio_newlib_off_${mode})
endforeach()

function(add_invalid_graph name)
    add_library(wio_newlib_invalid_${name} OBJECT EXCLUDE_FROM_ALL provider.cpp)
    configure_lock_target(wio_newlib_invalid_${name})
    target_compile_definitions(wio_newlib_invalid_${name} PRIVATE ${ARGN})
    add_test(NAME wio_newlib_reject_${name}
        COMMAND "${CMAKE_COMMAND}" "-DBUILD_DIR=${CMAKE_BINARY_DIR}"
            "-DCONFIG=$<CONFIG>" "-DTARGET=wio_newlib_invalid_${name}"
            "-DEXPECT=graph" -P "${CMAKE_CURRENT_SOURCE_DIR}/expect_build_failure.cmake")
    set_tests_properties(wio_newlib_reject_${name} PROPERTIES RUN_SERIAL TRUE)
endfunction()
add_invalid_graph(enable2 SEEED_WIO_TERMINAL=1 BLINKER_ENABLE_TASKS=2
    BLINKER_INTERNAL_WIO_TASKS=1 BLINKER_TASK_CAPACITY=4
    BLINKER_INTERNAL_WIO_TASK_REGION_RESERVED=1)
add_invalid_graph(internal0 SEEED_WIO_TERMINAL=1 BLINKER_ENABLE_TASKS=1
    BLINKER_INTERNAL_WIO_TASKS=0 BLINKER_TASK_CAPACITY=4
    BLINKER_INTERNAL_WIO_TASK_REGION_RESERVED=1)
add_invalid_graph(capacity3 SEEED_WIO_TERMINAL=1 BLINKER_ENABLE_TASKS=1
    BLINKER_INTERNAL_WIO_TASKS=1 BLINKER_TASK_CAPACITY=3
    BLINKER_INTERNAL_WIO_TASK_REGION_RESERVED=1)
add_invalid_graph(region0 SEEED_WIO_TERMINAL=1 BLINKER_ENABLE_TASKS=1
    BLINKER_INTERNAL_WIO_TASKS=1 BLINKER_TASK_CAPACITY=4
    BLINKER_INTERNAL_WIO_TASK_REGION_RESERVED=0)
add_invalid_graph(internal-missing SEEED_WIO_TERMINAL=1 BLINKER_ENABLE_TASKS=1
    BLINKER_TASK_CAPACITY=4 BLINKER_INTERNAL_WIO_TASK_REGION_RESERVED=1)
add_invalid_graph(region-missing SEEED_WIO_TERMINAL=1 BLINKER_ENABLE_TASKS=1
    BLINKER_INTERNAL_WIO_TASKS=1 BLINKER_TASK_CAPACITY=4)

# The task-enabled Sketch's strong anchor must not be satisfied by task0 code.
add_executable(wio_newlib_mismatch EXCLUDE_FROM_ALL mismatch.cpp provider.cpp)
configure_lock_target(wio_newlib_mismatch)
target_compile_definitions(wio_newlib_mismatch PRIVATE
    SEEED_WIO_TERMINAL=1 BLINKER_ENABLE_TASKS=0)
add_test(NAME wio_newlib_reject_sketch_library_mismatch
    COMMAND "${CMAKE_COMMAND}" "-DBUILD_DIR=${CMAKE_BINARY_DIR}"
        "-DCONFIG=$<CONFIG>" "-DTARGET=wio_newlib_mismatch"
        "-DEXPECT=anchor" -P "${CMAKE_CURRENT_SOURCE_DIR}/expect_build_failure.cmake")
set_tests_properties(wio_newlib_reject_sketch_library_mismatch PROPERTIES
    RUN_SERIAL TRUE)
