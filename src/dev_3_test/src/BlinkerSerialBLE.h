#ifndef BLINKER_SERIAL_BLE_H
#define BLINKER_SERIAL_BLE_H

#include "Adapters/BlinkerSerial.h"
#include "Blinker/BlinkerProtocol.h"

class BlinkerSerialBLE : public BlinkerProtocol<BlinkerSerial>
{
    typedef BlinkerProtocol<BlinkerSerial> Base;

    public :
        BlinkerSerialBLE(BlinkerSerial& transp)
            : Base(transp)
        {}

        void begin( uint8_t ss_rx_pin = 2,
                    uint8_t ss_tx_pin = 3,
                    uint32_t ss_baud = 9600);
        
};

#endif
