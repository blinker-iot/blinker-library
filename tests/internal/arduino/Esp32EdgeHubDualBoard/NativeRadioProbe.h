#ifndef BLINKER_TEST_NATIVE_RADIO_PROBE_H
#define BLINKER_TEST_NATIVE_RADIO_PROBE_H

#include <host/ble_hs.h>

// Read-only SDK event listener. No callback replacement, private SDK symbols,
// radio policy, credentials or serial output from the NimBLE task.
namespace native_radio_probe {
struct Event { uint32_t at; uint16_t handle; uint8_t kind; };
static Event events[4];
static portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
static uint8_t head, count;
static uint32_t dropped;
static ble_gap_event_listener listener;
static bool registered, attempted;

static int onGap(ble_gap_event* event, void*) {
    Event value = {};
    if (event->type == BLE_GAP_EVENT_CONNECT && event->connect.status == 0) {
        value.kind = 1U; value.handle = event->connect.conn_handle;
    } else if (event->type == BLE_GAP_EVENT_DISCONNECT) {
        value.kind = 2U; value.handle = event->disconnect.conn.conn_handle;
    } else return 0;
    value.at = micros();
    portENTER_CRITICAL(&lock);
    if (count < 4U) { events[(head + count) % 4U] = value; ++count; }
    else ++dropped;
    portEXIT_CRITICAL(&lock);
    return 0;
}

static void poll() {
    if (!registered) {
        if (attempted) return;
        if (xTaskGetHandle("nimble_host") == nullptr || !ble_hs_synced()) return;
        attempted = true;
        const int result = ble_gap_event_listener_register(&listener, onGap, nullptr);
        Serial.printf("radioProbe=registered result=%d us=%lu\n", result, static_cast<unsigned long>(micros()));
        registered = result == 0;
        return;
    }
    static uint16_t handle = BLE_HS_CONN_HANDLE_NONE;
    static uint32_t sequence, gapAt, previousBusySample;
    static bool awaitingIdle;
    for (uint8_t i = 0U; i < 4U; ++i) {
        Event event = {}; bool available = false;
        portENTER_CRITICAL(&lock);
        if (count) { event = events[head]; head = (head + 1U) % 4U; --count; available = true; }
        portEXIT_CRITICAL(&lock);
        if (!available) break;
        if (event.kind == 1U) {
            if (handle != BLE_HS_CONN_HANDLE_NONE) Serial.println("radioProbe=overlap");
            handle = event.handle; ++sequence; awaitingIdle = false;
            Serial.printf("radioProbe=connected seq=%lu us=%lu\n", static_cast<unsigned long>(sequence), static_cast<unsigned long>(event.at));
        } else if (event.handle == handle) {
            gapAt = event.at; awaitingIdle = true;
            Serial.printf("radioProbe=gap_disconnected seq=%lu us=%lu\n", static_cast<unsigned long>(sequence), static_cast<unsigned long>(gapAt));
        }
    }
    if (handle == BLE_HS_CONN_HANDLE_NONE) return;
    const uint32_t before = micros();
    const int found = ble_gap_conn_find(handle, nullptr);
    const bool scanning = ble_gap_disc_active() != 0;
    const bool connecting = ble_gap_conn_active() != 0;
    const uint32_t sampled = micros();
    if (awaitingIdle && found == BLE_HS_ENOTCONN && !scanning && !connecting) {
        uint32_t lost;
        portENTER_CRITICAL(&lock); lost = dropped; portEXIT_CRITICAL(&lock);
        Serial.printf("radioProbe=idle seq=%lu us=%lu gapUs=%lu sampleSpanUs=%lu sampleCostUs=%lu dropped=%lu\n",
            static_cast<unsigned long>(sequence), static_cast<unsigned long>(sampled),
            static_cast<unsigned long>(sampled - gapAt), static_cast<unsigned long>(sampled - previousBusySample),
            static_cast<unsigned long>(sampled - before), static_cast<unsigned long>(lost));
        handle = BLE_HS_CONN_HANDLE_NONE; awaitingIdle = false;
    }
    if (found == 0 || scanning || connecting) previousBusySample = before;
}
} // namespace native_radio_probe
#endif
