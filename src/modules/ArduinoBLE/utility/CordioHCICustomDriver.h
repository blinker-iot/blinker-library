#if defined(CORE_CM4)

#include "CyH4TransportDriver.h"

ble::vendor::cypress_ble::CyH4TransportDriver& ble_cordio_get_h4_transport_driver()
{
    static  ble::vendor::cypress_ble::CyH4TransportDriver s_transport_driver(
        /* TX */ CYBSP_BT_UART_TX, /* RX */ CYBSP_BT_UART_RX,
        /* cts */ CYBSP_BT_UART_CTS, /* rts */ CYBSP_BT_UART_RTS, NC, DEF_BT_BAUD_RATE,
        CYBSP_BT_HOST_WAKE, CYBSP_BT_DEVICE_WAKE
    );
    return s_transport_driver;
}

#define CUSTOM_HCI_DRIVER

#endif