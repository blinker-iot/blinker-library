// Internal, physical-USB-only storage maintenance gate. Never a user example.
// No firmware starts automatically and no secrets are printed as text.
#include <Arduino.h>

#if defined(ARDUINO_UNOR4_WIFI)
#include "R4StorageGate.h"
#elif defined(SEEED_WIO_TERMINAL) || defined(WIO_TERMINAL)
#include "WioStorageGate.h"
#else
#error "PlatformStorageBackupGate supports only UNO R4 WiFi and Wio Terminal"
#endif

#include "StorageGateDriver.h"

void setup() {
    Serial.begin(115200U);
}

void loop() {
    storage_gate::service();
}
