#include "BlinkerESP32_BLE.h"

void BlinkerESP32_BLE::begin()
{
    Base::begin();
    this->conn.begin();
    BLINKER_LOG1("ESP32_BLE initialized...");
}