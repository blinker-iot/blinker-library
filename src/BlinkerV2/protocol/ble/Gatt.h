#ifndef BLINKER_PROTOCOL_BLE_GATT_H
#define BLINKER_PROTOCOL_BLE_GATT_H

#include <stdint.h>

namespace blinker {
namespace ble {

static const char kServiceUuid[] =
    "5f6d0001-3f5b-4e4f-9f4d-626c696e6b32";
static const char kReceiveUuid[] =
    "5f6d0002-3f5b-4e4f-9f4d-626c696e6b32";
static const char kTransmitUuid[] =
    "5f6d0003-3f5b-4e4f-9f4d-626c696e6b32";

// Bluetooth advertising carries 128-bit UUIDs least-significant byte first.
// Kept beside the string form so raw scan-response encoders cannot silently
// disagree with the GATT service registered by platform adapters.
static const uint8_t kServiceUuidLittleEndian[16] = {
    0x32U, 0x6BU, 0x6EU, 0x69U,
    0x6CU, 0x62U, 0x4DU, 0x9FU,
    0x4FU, 0x4EU, 0x5BU, 0x3FU,
    0x01U, 0x00U, 0x6DU, 0x5FU
};

} // namespace ble
} // namespace blinker

#endif
