cmake_minimum_required(VERSION 3.13)
project(blinker_device_v2_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Defaults preserve the Standard integration suite. Capacity probes configure
# a separate build tree so every core/test translation unit uses the same ABI.
set(BLINKER_HOST_RESOURCE_PROFILE 2 CACHE STRING "Resource profile for this build")
add_compile_definitions(BLINKER_RESOURCE_PROFILE=${BLINKER_HOST_RESOURCE_PROFILE})
foreach(capacity IN ITEMS MAX_TRANSPORTS MAX_PEER_SESSIONS)
    set(BLINKER_HOST_${capacity} "" CACHE STRING "Optional build-wide ${capacity}")
    if(NOT "${BLINKER_HOST_${capacity}}" STREQUAL "")
        add_compile_definitions(BLINKER_${capacity}=${BLINKER_HOST_${capacity}})
    endif()
endforeach()

option(BLINKER_ENABLE_ASAN "Enable AddressSanitizer for host targets" OFF)
option(BLINKER_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer for host targets" OFF)
option(BLINKER_BUILD_FUZZ_SMOKE "Build the deterministic protocol fuzz smoke target" OFF)
option(BLINKER_USE_LIBFUZZER "Build the protocol fuzz target with Clang libFuzzer" OFF)

set(BLINKER_SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../src")
set(BLINKER_IMPLEMENTATION_ROOT
    "${BLINKER_SOURCE_ROOT}/BlinkerV2/implementation")

function(blinker_enable_sanitizers target)
    if(MSVC)
        if(BLINKER_ENABLE_UBSAN)
            message(FATAL_ERROR
                "MSVC does not provide UndefinedBehaviorSanitizer; use Clang or GCC")
        endif()
        if(BLINKER_ENABLE_ASAN)
            target_compile_options(${target} PRIVATE /fsanitize=address /Zi)
            target_link_options(${target} PRIVATE /INCREMENTAL:NO)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        set(sanitizers "")
        if(BLINKER_ENABLE_ASAN)
            list(APPEND sanitizers "address")
        endif()
        if(BLINKER_ENABLE_UBSAN)
            list(APPEND sanitizers "undefined")
        endif()
        if(sanitizers)
            list(JOIN sanitizers "," sanitizer_list)
            target_compile_options(${target} PRIVATE
                -fsanitize=${sanitizer_list} -fno-omit-frame-pointer)
            target_link_options(${target} PRIVATE
                -fsanitize=${sanitizer_list} -fno-omit-frame-pointer)
        endif()
    elseif(BLINKER_ENABLE_ASAN OR BLINKER_ENABLE_UBSAN)
        message(FATAL_ERROR
            "Sanitizers are unsupported by ${CMAKE_CXX_COMPILER_ID}")
    endif()
endfunction()

function(blinker_strict_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX)
    else()
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic -Werror)
    endif()
endfunction()

function(blinker_test target)
    add_executable(${target} ${ARGN})
    target_include_directories(${target} PRIVATE "${BLINKER_SOURCE_ROOT}")
    target_link_libraries(${target} PRIVATE blinker_device_v2_core)
    blinker_strict_warnings(${target})
    blinker_enable_sanitizers(${target})
endfunction()

# Arduino Builder compiles these same twelve portable responsibility units.
# Host tests build them once and reuse the resulting archive.
add_library(blinker_device_v2_core STATIC
    "${BLINKER_IMPLEMENTATION_ROOT}/BlinkerApi.cpp"
    "${BLINKER_IMPLEMENTATION_ROOT}/BlinkerControl.cpp"
    "${BLINKER_IMPLEMENTATION_ROOT}/BlinkerCore.cpp"
    "${BLINKER_IMPLEMENTATION_ROOT}/BlinkerIdentity.cpp"
    "${BLINKER_IMPLEMENTATION_ROOT}/BlinkerModel.cpp"
    "${BLINKER_IMPLEMENTATION_ROOT}/BlinkerProtocol.cpp"
    "${BLINKER_IMPLEMENTATION_ROOT}/BlinkerProvisioningBle.cpp"
    "${BLINKER_IMPLEMENTATION_ROOT}/BlinkerProvisioningController.cpp"
    "${BLINKER_IMPLEMENTATION_ROOT}/BlinkerProvisioningWifi.cpp"
    "${BLINKER_IMPLEMENTATION_ROOT}/BlinkerRuntime.cpp"
    "${BLINKER_IMPLEMENTATION_ROOT}/BlinkerSecurity.cpp"
    "${BLINKER_IMPLEMENTATION_ROOT}/BlinkerTransport.cpp")
target_include_directories(blinker_device_v2_core PUBLIC "${BLINKER_SOURCE_ROOT}")
blinker_strict_warnings(blinker_device_v2_core)
blinker_enable_sanitizers(blinker_device_v2_core)

add_library(blinker_interaction_compile OBJECT compile_interactions.cpp)
target_include_directories(blinker_interaction_compile PRIVATE
    "${BLINKER_SOURCE_ROOT}")
blinker_strict_warnings(blinker_interaction_compile)

blinker_test(blinker_diagnostics_output_tests test_diagnostics_output_main.cpp)
target_include_directories(blinker_diagnostics_output_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/stubs/diagnostics")

blinker_test(blinker_device_v2_tests
    test_main.cpp test_wifi_ble_lifecycle.cpp)
target_include_directories(blinker_device_v2_tests PRIVATE
    stubs "${BLINKER_SOURCE_ROOT}/BlinkerV2/arduino/ports/arduino")

blinker_test(blinker_telemetry_tests test_telemetry_main.cpp)
blinker_test(blinker_endpoint_integer_step_tests test_endpoint_integer_step_main.cpp)
blinker_test(blinker_wall_clock_tests test_wall_clock_main.cpp)
blinker_test(blinker_task_timing_tests test_task_timing_main.cpp)
blinker_test(blinker_task_calendar_cursor_tests test_task_calendar_cursor_main.cpp)
blinker_test(blinker_task_snapshot_store_tests test_task_snapshot_store_main.cpp)
blinker_test(blinker_task_engine_tests test_task_engine_main.cpp)
blinker_test(blinker_task_management_budget_tests test_task_management_budget_main.cpp)
blinker_test(blinker_task_management_grant_tests test_task_management_grant_main.cpp)
blinker_test(blinker_task_management_session_tests test_task_management_session_main.cpp)
blinker_test(blinker_time_sync_tests test_time_sync_main.cpp)
blinker_test(blinker_time_sync_runtime_tests test_time_sync_runtime_main.cpp)
blinker_test(blinker_time_sync_responder_tests test_time_sync_responder_main.cpp)
blinker_test(blinker_resource_profile_tests test_resource_profile_main.cpp)
blinker_test(blinker_noise_golden_tests ../golden/noise_golden_main.cpp)
blinker_test(blinker_controller_auth_golden_tests
    ../golden/controller_auth_golden_main.cpp)
blinker_test(blinker_controller_control_golden_tests
    ../golden/controller_control_golden_main.cpp)
blinker_test(blinker_ble_enrollment_golden_tests
    ../golden/ble_enrollment_golden_main.cpp)
blinker_test(blinker_wifi_credential_store_tests
    test_wifi_credential_store_main.cpp)
blinker_test(blinker_device_access_store_tests
    test_device_access_store_main.cpp)
blinker_test(blinker_ble_enrollment_tests test_ble_enrollment_main.cpp)
blinker_test(blinker_controller_authorizer_tests
    test_controller_authorizer_main.cpp)
blinker_test(blinker_controller_initiator_tests
    test_controller_initiator_main.cpp)
blinker_test(blinker_child_session_coordinator_tests
    test_child_session_coordinator_main.cpp)
blinker_test(blinker_gatt_direct_child_link_tests
    test_gatt_direct_child_link_main.cpp)
blinker_test(blinker_direct_secure_session_tests
    test_direct_secure_session_main.cpp)
blinker_test(blinker_direct_secure_golden_tests
    ../golden/direct_secure_golden_main.cpp)
blinker_test(blinker_local_access_tests test_local_access_main.cpp)
blinker_test(blinker_local_access_transport_tests test_local_access_transport_main.cpp)
blinker_test(blinker_websocket_ingress_tests test_websocket_ingress_main.cpp)
blinker_test(blinker_esp32_local_websocket_tests test_esp32_local_websocket_main.cpp)
target_include_directories(blinker_esp32_local_websocket_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/stubs/esp32_local_ws")
target_compile_definitions(blinker_esp32_local_websocket_tests PRIVATE ARDUINO_ARCH_ESP32=1 BLINKER_INTERNAL_LAN_WEBSOCKET=1)
target_compile_definitions(blinker_local_access_tests PRIVATE
    BLINKER_LOCAL_ACCESS_GOLDEN="${CMAKE_CURRENT_SOURCE_DIR}/../golden/local_access_golden_vectors.properties")
blinker_test(blinker_controller_control_tests test_controller_control_main.cpp)
blinker_test(blinker_presence_key_control_tests
    test_presence_key_control_main.cpp)
blinker_test(blinker_device_key_session_tests test_device_key_session_main.cpp)
blinker_test(blinker_device_key_provisioning_tests
    test_device_key_provisioning_main.cpp)
blinker_test(blinker_device_key_session_golden_tests
    ../golden/device_key_session_golden_main.cpp)
blinker_test(blinker_http_device_key_session_tests
    test_http_device_key_session_main.cpp)
blinker_test(blinker_arduino_client_http_tests
    test_arduino_client_http_main.cpp)
target_include_directories(blinker_arduino_client_http_tests PRIVATE stubs
    "${BLINKER_SOURCE_ROOT}/BlinkerV2/arduino/ports/arduino")
blinker_test(blinker_wifi_connection_lifecycle_tests
    test_wifi_connection_lifecycle_main.cpp)
blinker_test(blinker_esp32_wifi_adapter_tests test_esp32_wifi_adapter_main.cpp)
blinker_test(blinker_esp32_nimble_adapter_tests test_esp32_nimble_main.cpp)
target_compile_definitions(blinker_esp32_nimble_adapter_tests PRIVATE
    ARDUINO_ARCH_ESP32=1)
target_include_directories(blinker_esp32_nimble_adapter_tests PRIVATE
    stubs/esp32_nimble)
blinker_test(blinker_esp32_nimble_central_tests test_esp32_nimble_central_main.cpp)
target_compile_definitions(blinker_esp32_nimble_central_tests PRIVATE
    ARDUINO_ARCH_ESP32=1)
target_include_directories(blinker_esp32_nimble_central_tests PRIVATE
    stubs/esp32_nimble)
blinker_test(blinker_esp32_nimble_capacity_tests test_esp32_nimble_capacity_main.cpp)
target_compile_definitions(blinker_esp32_nimble_capacity_tests PRIVATE
    ARDUINO_ARCH_ESP32=1 CONFIG_BT_NIMBLE_MAX_CONNECTIONS=1)
target_include_directories(blinker_esp32_nimble_capacity_tests PRIVATE
    stubs/esp32_nimble)
target_compile_definitions(blinker_esp32_wifi_adapter_tests PRIVATE
    ARDUINO_ARCH_ESP32=1)
target_include_directories(blinker_esp32_wifi_adapter_tests PRIVATE
    stubs/esp32_wifi "${BLINKER_SOURCE_ROOT}/BlinkerV2/arduino/ports/esp32")
blinker_test(blinker_renesas_wifi_adapter_tests
    test_renesas_wifi_adapter_main.cpp)
target_compile_definitions(blinker_renesas_wifi_adapter_tests PRIVATE
    ARDUINO_ARCH_RENESAS=1)
target_include_directories(blinker_renesas_wifi_adapter_tests PRIVATE
    stubs/renesas_wifi
    "${BLINKER_SOURCE_ROOT}/BlinkerV2/arduino/ports/uno_r4")
blinker_test(blinker_identity_tests test_identity_main.cpp)
blinker_test(blinker_noise_tests test_noise_main.cpp)
blinker_test(blinker_ble_mode_tests test_ble_mode_main.cpp)
blinker_test(blinker_ble_direct_profile_tests
    test_ble_direct_profile_main.cpp)
blinker_test(blinker_device_v2_golden_tests ../golden/golden_main.cpp)
blinker_test(blinker_edge_gateway_golden_tests
    ../golden/edge_gateway_golden_main.cpp)
blinker_test(blinker_gateway_access_delivery_tests
    test_gateway_access_delivery_main.cpp)
blinker_test(blinker_gateway_presence_matcher_tests
    test_gateway_presence_matcher_main.cpp)
blinker_test(blinker_gateway_child_session_tests
    test_gateway_child_session_main.cpp)
blinker_test(blinker_gateway_child_route_bridge_tests
    test_gateway_child_route_bridge_main.cpp)
blinker_test(blinker_gateway_route_client_tests
    test_gateway_route_client_main.cpp)
blinker_test(blinker_gateway_execution_lease_tests
    test_gateway_execution_lease_main.cpp)
blinker_test(blinker_gateway_direct_yield_tests
    test_gateway_direct_yield_main.cpp)
blinker_test(blinker_gateway_gatt_radio_tests
    test_gateway_gatt_radio_main.cpp)
blinker_test(blinker_gateway_proof_coordinator_tests
    test_gateway_proof_coordinator_main.cpp)
blinker_test(blinker_gateway_revocation_coordinator_tests
    test_gateway_revocation_coordinator_main.cpp)
blinker_test(blinker_gateway_credential_renewal_tests
    test_gateway_credential_renewal_main.cpp)
blinker_test(blinker_gateway_credential_renewal_coordinator_tests
    test_gateway_credential_renewal_coordinator_main.cpp)
blinker_test(blinker_gateway_permit_join_tests
    test_gateway_permit_join_main.cpp)

# Platform storage fragments are compiled directly only by their adapter
# tests. Arduino builds use the platform responsibility units in src/.
set(ESP32_STORAGE_FRAGMENT
    "${BLINKER_SOURCE_ROOT}/BlinkerV2/arduino/ports/esp32/Esp32Storage.inc")
set(ESP32_SECURITY_FRAGMENT
    "${BLINKER_SOURCE_ROOT}/BlinkerV2/arduino/ports/esp32/Esp32Security.inc")
set(UNO_R4_STORAGE_FRAGMENT
    "${BLINKER_SOURCE_ROOT}/BlinkerV2/arduino/ports/uno_r4/UnoR4Storage.inc")
set_source_files_properties(
    "${ESP32_STORAGE_FRAGMENT}" "${ESP32_SECURITY_FRAGMENT}"
    "${UNO_R4_STORAGE_FRAGMENT}"
    PROPERTIES LANGUAGE CXX)

blinker_test(blinker_esp32_security_adapter_tests
    test_esp32_security_main.cpp "${ESP32_SECURITY_FRAGMENT}")
target_compile_definitions(blinker_esp32_security_adapter_tests PRIVATE
    ARDUINO_ARCH_ESP32=1)
target_include_directories(blinker_esp32_security_adapter_tests PRIVATE
    stubs "${BLINKER_SOURCE_ROOT}/BlinkerV2/arduino/ports/esp32")

blinker_test(blinker_esp32_storage_adapter_tests
    test_esp32_storage_main.cpp "${ESP32_STORAGE_FRAGMENT}")
target_compile_definitions(blinker_esp32_storage_adapter_tests PRIVATE
    ARDUINO_ARCH_ESP32=1)
target_include_directories(blinker_esp32_storage_adapter_tests PRIVATE
    stubs "${BLINKER_SOURCE_ROOT}/BlinkerV2/arduino/ports/esp32")

blinker_test(blinker_task_product_tests test_task_product_main.cpp "${ESP32_STORAGE_FRAGMENT}")
blinker_test(blinker_task_module_tests test_task_module_main.cpp)
blinker_test(blinker_portable_ed25519_adapter_tests test_portable_ed25519_main.cpp)
target_include_directories(blinker_portable_ed25519_adapter_tests PRIVATE
    stubs/portable_ed25519)
blinker_test(blinker_task_facade_tests test_task_facade_main.cpp)
blinker_test(blinker_wifi_facade_multitu_tests
    test_facade_multitu_main.cpp test_facade_multitu_peer.cpp)
blinker_test(blinker_ble_facade_multitu_tests
    test_facade_multitu_main.cpp test_facade_multitu_peer.cpp)
target_compile_definitions(blinker_ble_facade_multitu_tests PRIVATE
    BLINKER_TEST_FACADE_BLE=1)
blinker_test(blinker_wifi_ble_facade_multitu_tests
    test_facade_multitu_main.cpp test_facade_multitu_peer.cpp)
target_compile_definitions(blinker_wifi_ble_facade_multitu_tests PRIVATE
    BLINKER_TEST_FACADE_WIFI_BLE=1)
blinker_test(blinker_sketch_timer_tests test_sketch_timer_main.cpp)
target_compile_definitions(blinker_task_product_tests PRIVATE ARDUINO_ARCH_ESP32=1)
target_include_directories(blinker_task_product_tests PRIVATE stubs)

blinker_test(blinker_esp32_gateway_storage_tests
    test_esp32_gateway_storage_main.cpp "${ESP32_STORAGE_FRAGMENT}")
target_compile_definitions(blinker_esp32_gateway_storage_tests PRIVATE ARDUINO_ARCH_ESP32=1)
target_include_directories(blinker_esp32_gateway_storage_tests PRIVATE stubs)

blinker_test(blinker_renesas_storage_adapter_tests
    test_renesas_storage_main.cpp "${UNO_R4_STORAGE_FRAGMENT}")
target_compile_definitions(blinker_renesas_storage_adapter_tests PRIVATE
    ARDUINO_ARCH_RENESAS_UNO=1)
target_include_directories(blinker_renesas_storage_adapter_tests PRIVATE
    stubs/renesas_preferences
    "${BLINKER_SOURCE_ROOT}/BlinkerV2/arduino/ports/uno_r4")

blinker_test(blinker_uno_r4_task_storage_tests
    test_uno_r4_task_storage_main.cpp "${UNO_R4_STORAGE_FRAGMENT}")
target_compile_definitions(blinker_uno_r4_task_storage_tests PRIVATE
    ARDUINO_ARCH_RENESAS_UNO=1)
target_include_directories(blinker_uno_r4_task_storage_tests PRIVATE
    stubs/renesas_preferences
    "${BLINKER_SOURCE_ROOT}/BlinkerV2/arduino/ports/uno_r4")

# The station shares the same singleton as its real storage implementation.
target_sources(blinker_renesas_wifi_adapter_tests PRIVATE "${UNO_R4_STORAGE_FRAGMENT}")
target_include_directories(blinker_renesas_wifi_adapter_tests PRIVATE stubs/renesas_preferences)

blinker_test(blinker_uno_r4_modem_owner_tests test_uno_r4_modem_owner_main.cpp)
target_include_directories(blinker_uno_r4_modem_owner_tests PRIVATE stubs/renesas_modem_client)
add_test(NAME blinker_uno_r4_modem_owner_tests COMMAND blinker_uno_r4_modem_owner_tests)

blinker_test(blinker_uno_r4_heap_boundary_tests test_uno_r4_heap_boundary_main.cpp)
add_test(NAME blinker_uno_r4_heap_boundary_tests COMMAND blinker_uno_r4_heap_boundary_tests)

blinker_test(blinker_uno_r4_modem_rpc_boundary_tests test_uno_r4_modem_rpc_boundary_main.cpp)
add_test(NAME blinker_uno_r4_modem_rpc_boundary_tests COMMAND blinker_uno_r4_modem_rpc_boundary_tests)

blinker_test(blinker_task_rpc_fault_observer_tests test_task_rpc_fault_observer_main.cpp)
target_include_directories(blinker_task_rpc_fault_observer_tests PRIVATE stubs/r4_rpc_fault_observer)
add_test(NAME blinker_task_rpc_fault_observer_tests COMMAND blinker_task_rpc_fault_observer_tests)

blinker_test(blinker_wio_qspi_owner_tests test_wio_qspi_owner_main.cpp)
add_test(NAME blinker_wio_qspi_owner_tests COMMAND blinker_wio_qspi_owner_tests)
blinker_test(blinker_wio_task_storage_tests test_wio_task_storage_main.cpp)
add_test(NAME blinker_wio_task_storage_tests COMMAND blinker_wio_task_storage_tests)

blinker_test(blinker_wio_task_probe_tests test_wio_task_probe_main.cpp)
add_test(NAME blinker_wio_task_probe_tests COMMAND blinker_wio_task_probe_tests)
blinker_test(blinker_wio_task_cold_recovery_tests test_wio_task_cold_recovery_main.cpp)
add_test(NAME blinker_wio_task_cold_recovery_tests COMMAND blinker_wio_task_cold_recovery_tests)
blinker_test(blinker_wio_identity_transition_tests test_wio_identity_transition_main.cpp)
add_test(NAME blinker_wio_identity_transition_tests COMMAND blinker_wio_identity_transition_tests)
blinker_test(blinker_wio_transition_serial_tests test_wio_transition_serial_main.cpp)
add_test(NAME blinker_wio_transition_serial_tests COMMAND blinker_wio_transition_serial_tests)
foreach(mode IN ITEMS 0 1 2)
    set(target blinker_wio_storage_gate_mode${mode}_tests)
    blinker_test(${target} test_wio_storage_gate_main.cpp)
    target_include_directories(${target} PRIVATE stubs/diagnostics)
    target_compile_definitions(${target} PRIVATE BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE=${mode})
    add_test(NAME ${target} COMMAND ${target})
endforeach()

if(BLINKER_BUILD_FUZZ_SMOKE OR BLINKER_USE_LIBFUZZER)
    blinker_test(blinker_device_v2_protocol_fuzz fuzz_protocol_main.cpp)
    if(BLINKER_USE_LIBFUZZER)
        if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            message(FATAL_ERROR "BLINKER_USE_LIBFUZZER requires Clang")
        endif()
        target_compile_definitions(blinker_device_v2_protocol_fuzz PRIVATE
            BLINKER_USE_LIBFUZZER=1)
        target_compile_options(blinker_device_v2_protocol_fuzz PRIVATE
            -fsanitize=fuzzer)
        target_link_options(blinker_device_v2_protocol_fuzz PRIVATE
            -fsanitize=fuzzer)
    endif()
endif()

enable_testing()
foreach(target IN ITEMS
    blinker_device_v2_tests
    blinker_resource_profile_tests
    blinker_telemetry_tests
    blinker_esp32_storage_adapter_tests
    blinker_esp32_gateway_storage_tests
    blinker_esp32_security_adapter_tests
    blinker_wifi_credential_store_tests
    blinker_device_access_store_tests
    blinker_ble_enrollment_tests
    blinker_controller_authorizer_tests
    blinker_controller_initiator_tests
    blinker_child_session_coordinator_tests
    blinker_gatt_direct_child_link_tests
    blinker_direct_secure_session_tests
    blinker_local_access_tests
    blinker_local_access_transport_tests
    blinker_websocket_ingress_tests
    blinker_esp32_local_websocket_tests
    blinker_controller_control_tests
    blinker_presence_key_control_tests
    blinker_device_key_session_tests
    blinker_device_key_provisioning_tests
    blinker_http_device_key_session_tests
    blinker_arduino_client_http_tests
    blinker_wifi_connection_lifecycle_tests
    blinker_esp32_wifi_adapter_tests
    blinker_esp32_nimble_adapter_tests
    blinker_esp32_nimble_central_tests
    blinker_esp32_nimble_capacity_tests
    blinker_renesas_wifi_adapter_tests
    blinker_renesas_storage_adapter_tests
    blinker_identity_tests
    blinker_noise_tests
    blinker_ble_mode_tests
    blinker_ble_direct_profile_tests)
    add_test(NAME ${target} COMMAND ${target})
endforeach()

add_test(NAME blinker_diagnostics_output_tests COMMAND blinker_diagnostics_output_tests)
add_test(NAME blinker_endpoint_integer_step_tests COMMAND blinker_endpoint_integer_step_tests)
add_test(NAME blinker_wall_clock_tests COMMAND blinker_wall_clock_tests)
add_test(NAME blinker_task_timing_tests COMMAND blinker_task_timing_tests)
add_test(NAME blinker_task_calendar_cursor_tests COMMAND blinker_task_calendar_cursor_tests)
add_test(NAME blinker_task_snapshot_store_tests COMMAND blinker_task_snapshot_store_tests)
add_test(NAME blinker_task_engine_tests COMMAND blinker_task_engine_tests)
add_test(NAME blinker_task_management_budget_tests COMMAND blinker_task_management_budget_tests)
add_test(NAME blinker_task_management_session_tests COMMAND blinker_task_management_session_tests)
add_test(NAME blinker_task_management_grant_tests COMMAND blinker_task_management_grant_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/../golden/task_management_grant_vectors.properties)
add_test(NAME blinker_task_product_tests COMMAND blinker_task_product_tests)
add_test(NAME blinker_task_module_tests COMMAND blinker_task_module_tests)
add_test(NAME blinker_portable_ed25519_adapter_tests COMMAND blinker_portable_ed25519_adapter_tests)
add_test(NAME blinker_uno_r4_task_storage_tests COMMAND blinker_uno_r4_task_storage_tests)
add_test(NAME blinker_task_facade_tests COMMAND blinker_task_facade_tests)
add_test(NAME blinker_wifi_facade_multitu_tests COMMAND blinker_wifi_facade_multitu_tests)
add_test(NAME blinker_ble_facade_multitu_tests COMMAND blinker_ble_facade_multitu_tests)
add_test(NAME blinker_wifi_ble_facade_multitu_tests COMMAND blinker_wifi_ble_facade_multitu_tests)
add_test(NAME blinker_sketch_timer_tests COMMAND blinker_sketch_timer_tests)
add_test(NAME blinker_time_sync_runtime_tests COMMAND blinker_time_sync_runtime_tests)
add_test(NAME blinker_time_sync_responder_tests COMMAND blinker_time_sync_responder_tests)
add_test(NAME blinker_time_sync_tests COMMAND blinker_time_sync_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/../golden/time_sync_golden_vectors.properties)

add_test(NAME blinker_device_key_session_golden_tests
    COMMAND blinker_device_key_session_golden_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/../golden/device_key_session_golden_vectors.properties)
add_test(NAME blinker_device_v2_golden_tests
    COMMAND blinker_device_v2_golden_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/../golden/bbp2_golden_vectors.properties)
add_test(NAME blinker_noise_golden_tests
    COMMAND blinker_noise_golden_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/../golden/portable_noise_golden_vectors.properties)
add_test(NAME blinker_controller_auth_golden_tests
    COMMAND blinker_controller_auth_golden_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/../golden/controller_auth_golden_vectors.properties)
add_test(NAME blinker_controller_control_golden_tests
    COMMAND blinker_controller_control_golden_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/../golden/controller_control_golden_vectors.properties)
add_test(NAME blinker_ble_enrollment_golden_tests
    COMMAND blinker_ble_enrollment_golden_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/../golden/ble_enrollment_golden_vectors.properties)
add_test(NAME blinker_direct_secure_golden_tests
    COMMAND blinker_direct_secure_golden_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/../golden/direct_secure_golden_vectors.properties)
add_test(NAME blinker_edge_gateway_golden_tests
    COMMAND blinker_edge_gateway_golden_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/../golden/edge_gateway_golden_vectors.properties)
add_test(NAME blinker_gateway_access_delivery_tests
    COMMAND blinker_gateway_access_delivery_tests)
add_test(NAME blinker_gateway_presence_matcher_tests
    COMMAND blinker_gateway_presence_matcher_tests)
add_test(NAME blinker_gateway_child_session_tests
    COMMAND blinker_gateway_child_session_tests)
add_test(NAME blinker_gateway_child_route_bridge_tests
    COMMAND blinker_gateway_child_route_bridge_tests)
add_test(NAME blinker_gateway_route_client_tests
    COMMAND blinker_gateway_route_client_tests)
add_test(NAME blinker_gateway_execution_lease_tests
    COMMAND blinker_gateway_execution_lease_tests)
add_test(NAME blinker_gateway_direct_yield_tests
    COMMAND blinker_gateway_direct_yield_tests)
add_test(NAME blinker_gateway_gatt_radio_tests
    COMMAND blinker_gateway_gatt_radio_tests)
add_test(NAME blinker_gateway_proof_coordinator_tests
    COMMAND blinker_gateway_proof_coordinator_tests)
add_test(NAME blinker_gateway_revocation_coordinator_tests
    COMMAND blinker_gateway_revocation_coordinator_tests)
add_test(NAME blinker_gateway_credential_renewal_tests
    COMMAND blinker_gateway_credential_renewal_tests)
add_test(NAME blinker_gateway_credential_renewal_coordinator_tests
    COMMAND blinker_gateway_credential_renewal_coordinator_tests)
add_test(NAME blinker_gateway_permit_join_tests
    COMMAND blinker_gateway_permit_join_tests)
if(BLINKER_BUILD_FUZZ_SMOKE AND NOT BLINKER_USE_LIBFUZZER)
    add_test(NAME blinker_device_v2_protocol_fuzz_smoke
        COMMAND blinker_device_v2_protocol_fuzz --iterations 20000)
endif()
