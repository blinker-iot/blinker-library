#ifndef TEST_R4_RPC_FAULT_MODEM_H
#define TEST_R4_RPC_FAULT_MODEM_H

#include <stdint.h>
#include <stddef.h>
#include <string>

#define MAX_BUFF_SIZE 128U
#define _PREF_PUT "+PREFPUT"
#define CMD_WRITE(x) "AT" x "="

class ModemClass {
public:
    unsigned timeoutCalls;
    uint32_t timeoutMillis;
    ModemClass() : timeoutCalls(0U), timeoutMillis(10000U) {}
    void timeout(size_t value) { ++timeoutCalls; timeoutMillis = static_cast<uint32_t>(value); }
};

uint32_t micros();
#endif
