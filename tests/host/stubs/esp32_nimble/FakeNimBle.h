#pragma once

// Narrow SDK boundary fake: compile the actual peripheral adapter, not a copy
// of its state machine. No radio, scheduler or controller behavior is simulated.
#include <cstdint>
#include <cstring>
#include <cstdlib>

#define CONFIG_BT_NIMBLE_ENABLED 1
#define CONFIG_BT_NIMBLE_ROLE_CENTRAL 1
#define CONFIG_BT_NIMBLE_ROLE_OBSERVER 1
#ifndef CONFIG_BT_NIMBLE_MAX_CONNECTIONS
#define CONFIG_BT_NIMBLE_MAX_CONNECTIONS 3
#endif
#define portMUX_INITIALIZER_UNLOCKED 0
using portMUX_TYPE = int;
using esp_err_t = int;
enum {
    ESP_OK = 0, ESP_ERR_NO_MEM = 0x101,
    BLE_HS_CONN_HANDLE_NONE = 0xffff, BLE_HS_FOREVER = -1,
    BLE_HS_ENOTCONN = 7, BLE_HS_EDONE = 14, BLE_HS_ETIMEOUT = 13,
    BLE_HS_EALREADY = 2, BLE_HS_EBUSY = 15,
    BLE_UUID_TYPE_128 = 128, BLE_HS_IO_NO_INPUT_OUTPUT = 3,
    BLE_ERR_REM_USER_CONN_TERM = 0x13,
    BLE_GATT_CHR_F_READ = 2, BLE_GATT_CHR_F_WRITE_NO_RSP = 4,
    BLE_GATT_CHR_F_WRITE = 8, BLE_GATT_CHR_F_NOTIFY = 16,
    BLE_GATT_CHR_F_INDICATE = 32, BLE_GATT_SVC_TYPE_PRIMARY = 1,
    BLE_GATT_ACCESS_OP_READ_CHR = 0, BLE_GATT_ACCESS_OP_WRITE_CHR = 1,
    BLE_ATT_ERR_UNLIKELY = 14, BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN = 13,
    BLE_ATT_ERR_INSUFFICIENT_RES = 17,
    BLE_HS_ADV_F_DISC_GEN = 2, BLE_HS_ADV_F_BREDR_UNSUP = 4,
    BLE_GAP_CONN_MODE_UND = 2, BLE_GAP_DISC_MODE_GEN = 2,
    BLE_GAP_EVENT_CONNECT = 0, BLE_GAP_EVENT_DISCONNECT = 1, BLE_GAP_EVENT_DISC = 2,
    BLE_GAP_EVENT_NOTIFY_RX = 12,
    BLE_GAP_EVENT_ENC_CHANGE = 10, BLE_GAP_EVENT_NOTIFY_TX = 13,
    BLE_GAP_EVENT_SUBSCRIBE = 14
};
struct ble_uuid_t { uint8_t type; };
struct ble_uuid128_t { ble_uuid_t u; uint8_t value[16]; };
struct os_mbuf { uint16_t size; uint8_t data[20]; };
#define OS_MBUF_PKTLEN(buffer) ((buffer)->size)
struct ble_gatt_access_ctxt { uint8_t op; os_mbuf* om; };
struct ble_gatt_chr_def {
    const ble_uuid_t* uuid;
    int (*access_cb)(uint16_t, uint16_t, ble_gatt_access_ctxt*, void*);
    void* arg; uint16_t flags; uint16_t* val_handle;
};
struct ble_gatt_svc_def {
    uint8_t type; const ble_uuid_t* uuid;
    const ble_gatt_chr_def* characteristics;
};
struct ble_gap_conn_desc {
    uint16_t conn_handle;
    struct { uint8_t encrypted; uint8_t bonded; } sec_state;
};
struct ble_addr_t { uint8_t type; uint8_t val[6]; };
struct ble_gap_disc_desc {
    ble_addr_t addr; int8_t rssi; const uint8_t* data; uint8_t length_data;
};
struct ble_gap_event {
    uint8_t type;
    struct { int status; uint16_t conn_handle; } connect;
    struct { int reason; ble_gap_conn_desc conn; } disconnect;
    struct { int status; uint16_t conn_handle; } enc_change;
    struct { uint16_t conn_handle, attr_handle; uint8_t cur_notify, cur_indicate; } subscribe;
    struct { int status; uint16_t conn_handle, attr_handle; uint8_t indication; } notify_tx;
    ble_gap_disc_desc disc;
    struct { uint16_t conn_handle, attr_handle; uint8_t indication; os_mbuf* om; } notify_rx;
};
struct ble_hs_adv_fields {
    uint8_t flags; const ble_uuid128_t* uuids128;
    uint8_t num_uuids128, uuids128_is_complete;
    const uint8_t* name; uint8_t name_len, name_is_complete;
    const uint8_t* svc_data_uuid128; uint8_t svc_data_uuid128_len;
};
struct ble_gap_adv_params { uint8_t conn_mode, disc_mode; };
struct FakeHostConfig {
    void (*reset_cb)(int); void (*sync_cb)(); void* store_status_cb;
    uint8_t sm_io_cap, sm_bonding, sm_mitm, sm_sc, sm_sc_only;
    uint8_t sm_our_key_dist, sm_their_key_dist;
};
extern FakeHostConfig ble_hs_cfg;
namespace fake_nimble {
struct NativeConnection {
    uint16_t handle = BLE_HS_CONN_HANDLE_NONE;
    bool live = false;
    int (*gap)(ble_gap_event*, void*) = nullptr;
    void* context = nullptr;
    int writeResult = 0;
    unsigned terminateCalls = 0, writeCalls = 0;
    uint8_t encrypted = 0, bonded = 0;
};
struct State {
    uint32_t now = 0;
    bool advertising = false;
    bool deferTermination = false;
    bool scanning = false, connecting = false, deferCancel = false;
    bool central = false;
    NativeConnection connections[2];
    int cancelResult = 0, discoveryResult = 0;
    int scanResult = 0, connectResult = 0;
    uint16_t connected = BLE_HS_CONN_HANDLE_NONE;
    uint16_t receiveHandle = 1, transmitHandle = 2;
    unsigned advertiseCalls = 0, terminateCalls = 0, initCalls = 0;
    unsigned stopCalls = 0, deinitCalls = 0, scanCalls = 0, scanCancelCalls = 0;
    int initResult = 0, stopResult = 0, deinitResult = 0;
    bool autoSync = true;
    int terminateResult = 0, advertiseResult = 0;
    int (*gap)(ble_gap_event*, void*) = nullptr;
    void* context = nullptr;
    const ble_gatt_chr_def* receive = nullptr;
    void (*onExit)() = nullptr;
    void (*onDelay)() = nullptr;
    os_mbuf buffer = {};
};
extern State state;
inline NativeConnection* native(uint16_t handle) {
    for (auto& connection : state.connections) if (connection.handle == handle) return &connection;
    return nullptr;
}
inline bool alive(uint16_t handle) { auto* connection = native(handle); return connection != nullptr && connection->live; }
inline unsigned connectionCount() { unsigned count = 0; for (auto& c : state.connections) if (c.live) ++count; return count; }
inline void allocateConnection(uint16_t handle) {
    NativeConnection* connection = native(handle);
    if (connection == nullptr) for (auto& c : state.connections) if (!c.live) { connection = &c; break; }
    if (connection == nullptr) std::abort(); // Tests may not silently overwrite an active ACL.
    *connection = NativeConnection(); connection->handle = handle; connection->live = true;
    connection->gap = state.gap; connection->context = state.context;
}
inline void emit(ble_gap_event event) {
    if (state.central && event.type != BLE_GAP_EVENT_CONNECT && event.type != BLE_GAP_EVENT_DISC) {
        const uint16_t handle = event.type == BLE_GAP_EVENT_DISCONNECT ? event.disconnect.conn.conn_handle :
            event.type == BLE_GAP_EVENT_ENC_CHANGE ? event.enc_change.conn_handle : event.notify_rx.conn_handle;
        auto* connection = native(handle);
        if (connection != nullptr && connection->gap != nullptr) connection->gap(&event, connection->context);
    } else state.gap(&event, state.context);
}
inline void disconnect(uint16_t handle) {
    if (state.central) { if (native(handle)) native(handle)->live = false; }
    else if (state.connected == handle) state.connected = BLE_HS_CONN_HANDLE_NONE;
    ble_gap_event event = {}; event.type = BLE_GAP_EVENT_DISCONNECT;
    event.disconnect.conn.conn_handle = handle; emit(event);
}
inline void connect(uint16_t handle = 7) {
    state.advertising = false; state.connecting = false;
    if (state.central) allocateConnection(handle); else state.connected = handle;
    ble_gap_event event = {}; event.type = BLE_GAP_EVENT_CONNECT;
    event.connect.conn_handle = handle; emit(event);
}
inline void subscribe(bool indicate = true) {
    ble_gap_event event = {}; event.type = BLE_GAP_EVENT_SUBSCRIBE;
    event.subscribe.conn_handle = state.connected;
    event.subscribe.attr_handle = state.transmitHandle;
    event.subscribe.cur_indicate = indicate ? 1U : 0U;
    event.subscribe.cur_notify = indicate ? 0U : 1U; emit(event);
}
inline void tx(int status, uint16_t handle = 7, uint16_t attribute = 2, bool indication = true) {
    ble_gap_event event = {}; event.type = BLE_GAP_EVENT_NOTIFY_TX;
    event.notify_tx.status = status; event.notify_tx.conn_handle = handle;
    event.notify_tx.attr_handle = attribute;
    event.notify_tx.indication = indication ? 1U : 0U; emit(event);
}
}
inline void portENTER_CRITICAL(portMUX_TYPE*) {}
inline void portEXIT_CRITICAL(portMUX_TYPE*) {
    if (fake_nimble::state.onExit) fake_nimble::state.onExit();
}
inline uint32_t millis() { return fake_nimble::state.now; }
inline void delay(uint32_t value) {
    fake_nimble::state.now += value;
    if (fake_nimble::state.onDelay) fake_nimble::state.onDelay();
}
inline int nimble_port_init() { ++fake_nimble::state.initCalls; return fake_nimble::state.initResult; }
inline int nimble_port_stop() { ++fake_nimble::state.stopCalls; return fake_nimble::state.stopResult; }
inline int nimble_port_deinit() { ++fake_nimble::state.deinitCalls; return fake_nimble::state.deinitResult; }
inline void nimble_port_run() {}
inline void nimble_port_freertos_deinit() {}
inline void nimble_port_freertos_init(void (*)(void*)) { if (fake_nimble::state.autoSync) ble_hs_cfg.sync_cb(); }
inline void ble_svc_gap_init() {}
inline void ble_svc_gatt_init() {}
inline int ble_svc_gap_device_name_set(const char*) { return 0; }
inline int ble_gatts_count_cfg(const ble_gatt_svc_def*) { return 0; }
inline int ble_gatts_add_svcs(const ble_gatt_svc_def* services) {
    fake_nimble::state.receive = &services[0].characteristics[0];
    *services[0].characteristics[0].val_handle = fake_nimble::state.receiveHandle;
    *services[0].characteristics[1].val_handle = fake_nimble::state.transmitHandle;
    return 0;
}
inline int ble_hs_util_ensure_addr(int) { return 0; }
inline int ble_hs_id_infer_auto(int, uint8_t* type) { *type = 0; return 0; }
inline int ble_gap_adv_active() { return fake_nimble::state.advertising ? 1 : 0; }
inline int ble_gap_adv_stop() { fake_nimble::state.advertising = false; return 0; }
inline int ble_gap_adv_set_fields(const ble_hs_adv_fields*) { return 0; }
inline int ble_gap_adv_rsp_set_fields(const ble_hs_adv_fields*) { return 0; }
inline int ble_gap_adv_start(uint8_t, const void*, int32_t, const ble_gap_adv_params*,
                             int (*callback)(ble_gap_event*, void*), void* context) {
    auto& s = fake_nimble::state; ++s.advertiseCalls;
    s.gap = callback; s.context = context;
    if (s.advertiseResult) return s.advertiseResult;
    s.advertising = true; return 0;
}
inline int ble_gap_conn_find(uint16_t handle, ble_gap_conn_desc* output) {
    if (fake_nimble::state.central) {
        auto* c = fake_nimble::native(handle);
        if (c == nullptr || !c->live) return BLE_HS_ENOTCONN;
        *output = {}; output->conn_handle = handle;
        output->sec_state.encrypted = c->encrypted; output->sec_state.bonded = c->bonded;
        return 0;
    }
    if (fake_nimble::state.connected != handle) return BLE_HS_ENOTCONN;
    *output = {}; output->conn_handle = handle; return 0;
}
inline int ble_gap_terminate(uint16_t handle, uint8_t) {
    ++fake_nimble::state.terminateCalls;
    if (fake_nimble::state.central) {
        auto* c = fake_nimble::native(handle);
        if (c == nullptr || !c->live) return BLE_HS_ENOTCONN;
        ++c->terminateCalls;
    } else if (fake_nimble::state.connected != handle) return BLE_HS_ENOTCONN;
    if (fake_nimble::state.terminateResult) return fake_nimble::state.terminateResult;
    if (fake_nimble::state.deferTermination) return 0;
    fake_nimble::disconnect(handle); return 0;
}
inline os_mbuf* ble_hs_mbuf_from_flat(const void* data, uint16_t size) {
    auto& b = fake_nimble::state.buffer; b.size = size;
    std::memcpy(b.data, data, size); return &b;
}
inline int ble_hs_mbuf_to_flat(const os_mbuf* b, void* data, uint16_t capacity, uint16_t* size) {
    if (b->size > capacity) return -1;
    *size = b->size; std::memcpy(data, b->data, b->size); return 0;
}
inline void os_mbuf_free_chain(os_mbuf*) {}
inline int ble_gatts_indicate_custom(uint16_t, uint16_t, os_mbuf*) { return 0; }
inline int ble_gatts_notify_custom(uint16_t, uint16_t, os_mbuf*) { return 0; }
