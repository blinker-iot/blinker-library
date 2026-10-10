#pragma once

#include <host/ble_gap.h>
#include <host/ble_hs.h>

// Internal probe only: -DBLINKER_TEST_GAP_WITNESS and -Wl,--wrap=ble_gap_init.
// GAP init clears the SDK listener list on every Host start. Register after
// successful init, before the Host task runs; never race its listener traversal
// by re-registering from loop(). No SDK patch or product callback is required.
#if defined(BLINKER_TEST_GAP_WITNESS)
struct BleGapWitness {
    uint32_t hostStarts = 0, connects = 0, disconnects = 0, indicationErrors = 0;
    uint16_t lastHandle = BLE_HS_CONN_HANDLE_NONE;
    int initResult = 0, listenerResult = 0;
    int lastDisconnect = 0, lastIndicationError = 0, lastAdvComplete = 0;
};
BleGapWitness bleGapWitness;
portMUX_TYPE witnessLock = portMUX_INITIALIZER_UNLOCKED;
ble_gap_event_listener witnessListener;

int observeBle(ble_gap_event* event, void*) {
    portENTER_CRITICAL(&witnessLock);
    if (event->type == BLE_GAP_EVENT_CONNECT && event->connect.status == 0) {
        ++bleGapWitness.connects;
        bleGapWitness.lastHandle = event->connect.conn_handle;
    } else if (event->type == BLE_GAP_EVENT_DISCONNECT) {
        ++bleGapWitness.disconnects;
        bleGapWitness.lastHandle = BLE_HS_CONN_HANDLE_NONE;
        bleGapWitness.lastDisconnect = event->disconnect.reason;
    } else if (event->type == BLE_GAP_EVENT_NOTIFY_TX && event->notify_tx.indication &&
               event->notify_tx.status != 0 && event->notify_tx.status != BLE_HS_EDONE) {
        ++bleGapWitness.indicationErrors;
        bleGapWitness.lastIndicationError = event->notify_tx.status;
    } else if (event->type == BLE_GAP_EVENT_ADV_COMPLETE) {
        bleGapWitness.lastAdvComplete = event->adv_complete.reason;
    }
    portEXIT_CRITICAL(&witnessLock);
    return 0;
}

extern "C" int __real_ble_gap_init(void);
extern "C" int __wrap_ble_gap_init(void) {
    const int result = __real_ble_gap_init();
    // Previous Host has stopped; the new Host task has not been started yet.
    ++bleGapWitness.hostStarts;
    bleGapWitness.lastHandle = BLE_HS_CONN_HANDLE_NONE;
    bleGapWitness.lastDisconnect = 0;
    bleGapWitness.lastIndicationError = 0;
    bleGapWitness.lastAdvComplete = 0;
    bleGapWitness.initResult = result;
    bleGapWitness.listenerResult = result == 0
        ? ble_gap_event_listener_register(&witnessListener, &observeBle, nullptr)
        : BLE_HS_ENOTSYNCED;
    return result; // Observations never change SDK admission or callbacks.
}
#endif

void printBleWitness() {
    Serial.printf("bleWitness uptime=%lu synced=%d adv=%d heap=%u heapMin=%u",
                  static_cast<unsigned long>(millis()), ble_hs_synced(), ble_gap_adv_active(),
                  ESP.getFreeHeap(), ESP.getMinFreeHeap());
#if defined(BLINKER_TEST_GAP_WITNESS)
    portENTER_CRITICAL(&witnessLock);
    const BleGapWitness snapshot = bleGapWitness;
    portEXIT_CRITICAL(&witnessLock);
    ble_gap_conn_desc connection = {};
    const bool connected = snapshot.lastHandle != BLE_HS_CONN_HANDLE_NONE &&
        ble_gap_conn_find(snapshot.lastHandle, &connection) == 0;
    Serial.printf(" observed=%u hostStarts=%lu init=%d listener=%d acl=%u connects=%lu disconnects=%lu "
                  "indicationErrors=%lu lastIndicationError=%d lastDisconnect=%d advComplete=%d",
                  snapshot.hostStarts != 0U && snapshot.initResult == 0 && snapshot.listenerResult == 0 ? 1U : 0U,
                  static_cast<unsigned long>(snapshot.hostStarts), snapshot.initResult, snapshot.listenerResult,
                  connected ? 1U : 0U, static_cast<unsigned long>(snapshot.connects),
                  static_cast<unsigned long>(snapshot.disconnects),
                  static_cast<unsigned long>(snapshot.indicationErrors), snapshot.lastIndicationError,
                  snapshot.lastDisconnect, snapshot.lastAdvComplete);
#else
    Serial.print(" observed=0"); // No stale one-shot listener in ordinary builds.
#endif
    Serial.println();
}
