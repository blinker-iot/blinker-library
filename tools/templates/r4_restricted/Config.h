// All C/C++/assembly consumers force-include this file through the build plan.
#include "CandidateConfig.h"
#if defined(BLINKER_PRODUCT_BLE) || defined(BLINKER_PRODUCT_WIFI_BLE) || \
    defined(BLINKER_INTERNAL_LOCAL_ACCESS_CONTROL) || \
    defined(BLINKER_INTERNAL_LAN_WEBSOCKET) || defined(BLINKER_INTERNAL_RPC_FAULT_GATE)
#error "Restricted R4 graph excludes BLE, LAN and fault injection"
#endif
@FIXED_DEFINES@
