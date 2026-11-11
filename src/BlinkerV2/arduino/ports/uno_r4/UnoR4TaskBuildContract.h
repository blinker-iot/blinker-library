#ifndef BLINKER_UNO_R4_TASK_BUILD_CONTRACT_H
#define BLINKER_UNO_R4_TASK_BUILD_CONTRACT_H

#include "../../config/TaskConfig.h"

#if defined(BLINKER_ENABLE_TASKS) && BLINKER_ENABLE_TASKS
#include <BlinkerV2/core/ResourceProfile.h>
#include <BlinkerV2/core/Result.h>
#include <stddef.h>
#include <stdint.h>

#if !defined(ARDUINO_UNOWIFIR4) || ARDUINO_UNOWIFIR4 != 1 || \
    !defined(ARDUINO_ARCH_RENESAS_UNO) || ARDUINO_ARCH_RENESAS_UNO != 1 || !defined(NO_USB)
#error "UNO R4 tasks require the reviewed UNO R4 WiFi NO_USB target"
#endif
#if !defined(BLINKER_INTERNAL_UNO_R4_TASKS) || BLINKER_INTERNAL_UNO_R4_TASKS != 1
#error "UNO R4 tasks require internal storage and signing-profile admission"
#endif
#if !defined(BLINKER_INTERNAL_R4_LIBRARY_BINDING) || BLINKER_INTERNAL_R4_LIBRARY_BINDING != 1
#error "UNO R4 tasks require the matching maintained library/profile binding"
#endif
#if BLINKER_TASK_CAPACITY != 4
#error "UNO R4 task candidate admits four slots only"
#endif
#if defined(BLINKER_PRODUCT_BLE) || defined(BLINKER_PRODUCT_WIFI_BLE) || \
    (defined(BLINKER_INTERNAL_LOCAL_ACCESS_CONTROL) && BLINKER_INTERNAL_LOCAL_ACCESS_CONTROL) || \
    (defined(BLINKER_INTERNAL_LAN_WEBSOCKET) && BLINKER_INTERNAL_LAN_WEBSOCKET) || \
    (defined(BLINKER_INTERNAL_RPC_FAULT_GATE) && BLINKER_INTERNAL_RPC_FAULT_GATE)
#error "UNO R4 task candidate admits manual WiFi only"
#endif
#if !defined(BLINKER_OFFICIAL_CLOUD_SESSION_TLS) || BLINKER_OFFICIAL_CLOUD_SESSION_TLS != 0 || \
    !defined(BLINKER_OFFICIAL_MQTT_TLS) || BLINKER_OFFICIAL_MQTT_TLS != 0
#error "UNO R4 task candidate admits the existing plain WiFi profile only"
#endif
#if BLINKER_RESOURCE_PROFILE != BLINKER_RESOURCE_PROFILE_SMALL || BLINKER_DEVICE_FRAME_SIZE != 256
#error "UNO R4 tasks require the reviewed Small 256 B frame profile"
#endif
#if BLINKER_TASK_WINDOW_MS != 60000 || BLINKER_TASK_WRITES != 10 || \
    BLINKER_TASK_MIN_START_MS != 30000 || BLINKER_TASK_MAX_COMMIT_MS != 1000
#error "UNO R4 tasks require the reviewed four-slot storage policy"
#endif
#if !defined(MQTT_MAX_PACKET_SIZE) || MQTT_MAX_PACKET_SIZE != 361
#error "UNO R4 tasks require the reviewed 361 B initial MQTT capacity"
#endif
#if !defined(BLINKER_INTERNAL_R4_MODEM_COMMAND_BYTES) || BLINKER_INTERNAL_R4_MODEM_COMMAND_BYTES != 32 || \
    !defined(BLINKER_INTERNAL_R4_MODEM_RESPONSE_BYTES) || BLINKER_INTERNAL_R4_MODEM_RESPONSE_BYTES != 1024 || \
    !defined(BLINKER_INTERNAL_R4_MODEM_RPC_MS) || BLINKER_INTERNAL_R4_MODEM_RPC_MS != 1000 || \
    !defined(BLINKER_INTERNAL_R4_HEAP_BOUNDARY) || BLINKER_INTERNAL_R4_HEAP_BOUNDARY != 1
#error "UNO R4 tasks require the reviewed RPC and heap protection configuration"
#endif

// Existing strong entries supplied by the protected SDK overlay and the one
// heap adapter TU. No weak fallback: flags alone cannot supply these bindings.
// Final recipe/ELF verification still owns whole-program wrap and SDK identity.
extern "C" bool blinker_r4_rpc_faulted();
extern "C" void* __wrap__sbrk(ptrdiff_t increment);

namespace blinker {
namespace integration {
namespace official_detail {

inline Result renesasUnoTaskBuildReady() {
    if (blinker_r4_rpc_faulted()) return Result::failure(ErrorCode::StateConflict);
    // A zero increment only validates the original heap adapter's boundaries;
    // it neither allocates nor changes the SDK-owned break.
    if (__wrap__sbrk(0) == reinterpret_cast<void*>(UINTPTR_MAX))
        return Result::failure(ErrorCode::NotConfigured);
    return Result::success();
}

} // namespace official_detail
} // namespace integration
} // namespace blinker
#endif

#endif
