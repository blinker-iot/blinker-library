#ifndef BlinkerSerialESP_H
#define BlinkerSerialESP_H

#if defined(BLINKER_MQTT_AT)
    #include <Adapters/BlinkerSerialMQTT.h>

    static BlinkerTransportStream _blinkerTransport;
    BlinkerSerialMQTT Blinker(_blinkerTransport);
#endif

#include <BlinkerWidgets.h>

#endif