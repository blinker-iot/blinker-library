#pragma once
#include "FakeNimBle.h"

// SDK boundary fake; tests compile the production port, not a copied algorithm.
enum {
    BLE_GAP_SCAN_FAST_INTERVAL_MIN = 48, BLE_GAP_SCAN_FAST_WINDOW = 48,
    BLE_GAP_INITIAL_SUPERVISION_TIMEOUT = 256, BLE_GAP_INITIAL_CONN_LATENCY = 0,
    BLE_GAP_INITIAL_CONN_MIN_CE_LEN = 0, BLE_GAP_INITIAL_CONN_MAX_CE_LEN = 0,
    BLE_GATT_CHR_PROP_WRITE = 8, BLE_GATT_CHR_PROP_INDICATE = 32,
    BLE_GATT_DSC_CLT_CFG_UUID16 = 0x2902
};
struct ble_gap_disc_params {
    uint16_t itvl, window;
    uint8_t filter_policy, limited, passive, filter_duplicates, disable_observer_mode;
};
struct ble_gap_conn_params {
    uint16_t scan_itvl, scan_window, itvl_min, itvl_max, latency,
             supervision_timeout, min_ce_len, max_ce_len;
};
struct ble_gatt_error { int status; };
struct ble_gatt_svc { uint16_t start_handle, end_handle; };
struct ble_gatt_chr { uint8_t properties; uint16_t val_handle; };
struct ble_gatt_dsc { uint16_t handle; ble_uuid128_t uuid; };
struct ble_gatt_attr {};
namespace fake_nimble {
struct GattCallbacks {
    int (*service)(uint16_t, const ble_gatt_error*, const ble_gatt_svc*, void*) = nullptr;
    int (*characteristic)(uint16_t, const ble_gatt_error*, const ble_gatt_chr*, void*) = nullptr;
    int (*descriptor)(uint16_t, const ble_gatt_error*, uint16_t, const ble_gatt_dsc*, void*) = nullptr;
    int (*write)(uint16_t, const ble_gatt_error*, ble_gatt_attr*, void*) = nullptr;
    void* context = nullptr;
};
inline GattCallbacks* gatts() { static GattCallbacks callbacks[2]; return callbacks; }
inline GattCallbacks& gatt(uint16_t handle = 7) {
    auto* connection = native(handle);
    if (connection == nullptr) std::abort();
    return gatts()[connection - state.connections];
}
inline void resetGatt() { gatts()[0] = GattCallbacks(); gatts()[1] = GattCallbacks(); }
}
inline uint16_t ble_uuid_u16(const ble_uuid_t*) { return BLE_GATT_DSC_CLT_CFG_UUID16; }
inline int ble_gap_disc_active() { return fake_nimble::state.scanning ? 1 : 0; }
inline int ble_gap_conn_active() { return fake_nimble::state.connecting ? 1 : 0; }
inline int ble_gap_disc(uint8_t, int32_t, const ble_gap_disc_params*,
                        int (*callback)(ble_gap_event*, void*), void* context) {
    auto& s = fake_nimble::state;
    ++s.scanCalls;
    s.central = true;
    if (s.scanning || s.connecting) return BLE_HS_EBUSY;
    if (s.scanResult) return s.scanResult;
    s.scanning = true; s.gap = callback; s.context = context; return 0;
}
inline int ble_gap_disc_cancel() {
    ++fake_nimble::state.scanCancelCalls;
    if (fake_nimble::state.cancelResult) return fake_nimble::state.cancelResult;
    fake_nimble::state.scanning = false; return 0;
}
inline int ble_gap_connect(uint8_t, const ble_addr_t*, int32_t,
                           const ble_gap_conn_params*,
                           int (*callback)(ble_gap_event*, void*), void* context) {
    auto& s = fake_nimble::state;
    if (s.scanning || s.connecting) return BLE_HS_EBUSY;
    if (s.connectResult) return s.connectResult;
    s.connecting = true; s.gap = callback; s.context = context; return 0;
}
inline int ble_gap_conn_cancel() {
    auto& s = fake_nimble::state;
    if (s.cancelResult) return s.cancelResult;
    if (!s.connecting) return BLE_HS_EALREADY;
    if (!s.deferCancel) {
        s.connecting = false;
        ble_gap_event event = {}; event.type = BLE_GAP_EVENT_CONNECT;
        event.connect.status = BLE_HS_ENOTCONN;
        event.connect.conn_handle = BLE_HS_CONN_HANDLE_NONE;
        fake_nimble::emit(event);
    }
    return 0;
}
inline int ble_hs_adv_parse_fields(ble_hs_adv_fields* out, const uint8_t* data, uint8_t size) {
    *out = {}; out->svc_data_uuid128 = data; out->svc_data_uuid128_len = size; return 0;
}
inline int ble_gattc_disc_svc_by_uuid(uint16_t handle, const ble_uuid_t*,
    int (*callback)(uint16_t, const ble_gatt_error*, const ble_gatt_svc*, void*), void* context) {
    fake_nimble::gatt(handle) = fake_nimble::GattCallbacks();
    fake_nimble::gatt(handle).service = callback; fake_nimble::gatt(handle).context = context;
    return fake_nimble::state.discoveryResult;
}
inline int ble_gattc_disc_chrs_by_uuid(uint16_t handle, uint16_t, uint16_t, const ble_uuid_t*,
    int (*callback)(uint16_t, const ble_gatt_error*, const ble_gatt_chr*, void*), void* context) {
    fake_nimble::gatt(handle).characteristic = callback; fake_nimble::gatt(handle).context = context; return 0;
}
inline int ble_gattc_disc_all_dscs(uint16_t handle, uint16_t, uint16_t,
    int (*callback)(uint16_t, const ble_gatt_error*, uint16_t, const ble_gatt_dsc*, void*), void* context) {
    fake_nimble::gatt(handle).descriptor = callback; fake_nimble::gatt(handle).context = context; return 0;
}
inline int ble_gattc_write_flat(uint16_t handle, uint16_t, const void*, uint16_t,
    int (*callback)(uint16_t, const ble_gatt_error*, ble_gatt_attr*, void*), void* context) {
    if (!fake_nimble::alive(handle)) return BLE_HS_ENOTCONN;
    auto* connection = fake_nimble::native(handle); ++connection->writeCalls;
    if (connection->writeResult) return connection->writeResult;
    fake_nimble::gatt(handle).write = callback; fake_nimble::gatt(handle).context = context; return 0;
}
