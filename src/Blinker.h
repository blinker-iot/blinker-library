#ifndef BLINKER_H
#define BLINKER_H

#if defined(BLINKER_BLE)

    #if defined(BLINKER_ALIGENIE_LIGHT) || defined(BLINKER_ALIGENIE_OUTLET) || \
        defined(BLINKER_ALIGENIE_SWITCH)|| defined(BLINKER_ALIGENIE_SENSOR)
        #error This code is intended to run on the BLINKER_MQTT mode! Please check your mode setting.    
    #endif

    #if defined(BLINKER_DUEROS_LIGHT) || defined(BLINKER_DUEROS_OUTLET) || \
        defined(BLINKER_DUEROS_SWITCH)|| defined(BLINKER_DUEROS_SENSOR)
        #error This code is intended to run on the BLINKER_MQTT mode! Please check your mode setting.
    #endif

    #if defined(ESP32)
        #include "BlinkerESP32BLE.h"

        BlinkerESP32BLE     Blinker;
    #else
        #include "BlinkerSerialBLE.h"

        BlinkerSerialBLE    Blinker;
    #endif

#elif defined(BLINKER_WIFI) || defined(BLINKER_MQTT)

    // #if defined(BLINKER_APCONFIG_V2)
    //     #define BLINKER_APCONFIG
    //     #define BLINKER_WITHOUT_WS_REG
    // #endif

    #if defined(BLINKER_WIFI)
        #undef BLINKER_WIFI
        #define BLINKER_MQTT
    #endif

    #define BLINKER_PROTOCOL_HTTP_SERVER

    #include "BlinkerAssistant.h"

    #if (defined(ESP8266) || defined(ESP32)) && !defined(BLINKER_MQTT_AT)
        #include "BlinkerESPMQTT.h"

        BlinkerESPMQTT      Blinker; 

    #else
        #define BLINKER_ESP_AT

        #define BLINKER_MQTT_AT

        #undef BLINKER_MQTT
        
        #include "BlinkerSerialESPMQTT.h"

        BlinkerSerialESPMQTT      Blinker;  
    #endif

// #elif defined(BLINKER_WIFI_LOWPOWER)

//     #if defined(ESP8266) || defined(ESP32)
//         #include "BlinkerESPMQTTLP.h"

//         BlinkerESPMQTTLP    Blinker;
//     #endif

#elif defined(BLINKER_PRO)

    #if defined(BLINKER_ALIGENIE_LIGHT) || defined(BLINKER_ALIGENIE_OUTLET) || \
        defined(BLINKER_ALIGENIE_SWITCH)|| defined(BLINKER_ALIGENIE_SENSOR)
        #error This code is intended to run on the BLINKER_MQTT mode! Please check your mode setting.
    #endif    

    #if defined(BLINKER_DUEROS_LIGHT) || defined(BLINKER_DUEROS_OUTLET) || \
        defined(BLINKER_DUEROS_SWITCH)|| defined(BLINKER_DUEROS_SENSOR)
        #error This code is intended to run on the BLINKER_MQTT mode! Please check your mode setting.
    #endif

    #define BLINKER_ALIGENIE

    #define BLINKER_DUEROS

    #ifndef BLINKER_ESP_SMARTCONFIG
        #ifndef BLINKER_APCONFIG
            #define BLINKER_ESP_SMARTCONFIG
        #endif
    #endif

    #define BLINKER_PROTOCOL_HTTP_SERVER

    #if defined(ESP8266) || defined(ESP32)
        #include "BlinkerESPPRO.h"
        
        BlinkerESPPRO       Blinker; 
    #else
        #error This code is intended to run on the ESP8266/ESP32 platform! Please check your Tools->Board setting.
    #endif

#elif defined(BLINKER_PRO_ESP) || defined(BLINKER_WIFI_AUTO)

    #include "BlinkerAssistant.h"    

    #if defined(BLINKER_APCONFIG_V2)
        #define BLINKER_APCONFIG
        #define BLINKER_WITHOUT_WS_REG
    #endif
    
    #define BLINKER_WITHOUT_WS_REG

    #if defined(BLINKER_WIFI_AUTO)
        #define BLINKER_PRO_ESP
    #endif

    // #ifndef BLINKER_ESP_SMARTCONFIG
    //     #ifndef BLINKER_APCONFIG
    //         #define BLINKER_ESP_SMARTCONFIG
    //     #endif
    // #endif

    #if defined(ESP8266) || defined(ESP32)
        #include "BlinkerESPPROESP.h"
        
        BlinkerESPPROESP    Blinker; 
    #else
        #error This code is intended to run on the ESP8266/ESP32 platform! Please check your Tools->Board setting.
    #endif

#elif defined(BLINKER_AT_MQTT)

    #define BLINKER_ESP_AT

    #if defined(ESP8266) || defined(ESP32)
        #include "BlinkerESPMQTTAT.h"

        BlinkerESPMQTTAT    Blinker; 
    #else
        #error This code is intended to run on the ESP8266/ESP32 platform! Please check your Tools->Board setting.
    #endif

#elif defined(BLINKER_WIFI_GATEWAY)

    #include "BlinkerAssistant.h"

    #if defined(ESP8266) || defined(ESP32)
        #include "BlinkerESPGateway.h"

        BlinkerESPGateway    Blinker; 
    #else
        #error This code is intended to run on the ESP8266/ESP32 platform! Please check your Tools->Board setting.
    #endif

#elif defined(BLINKER_WIFI_SUBDEVICE)

    #include "BlinkerAssistant.h"

    #if defined(ESP8266) || defined(ESP32)
        #include "BlinkerESPSubDevice.h"

        BlinkerESPSubDevice     Blinker;
    #else
        #error This code is intended to run on the ESP8266/ESP32 platform! Please check your Tools->Board setting.
    #endif

#elif defined(BLINKER_NBIOT_WH)

    #ifndef BLINKER_NB73_NBIOT
    #define BLINKER_NB73_NBIOT
    #endif

    #include "BlinkerSerialWHNBIoT.h"

    BlinkerSerialWHNBIoT    Blinker;

#elif defined(BLINKER_NBIOT_SIM7020)

    // #ifndef BLINKER_SIM7020C_NBIOT
    // #define BLINKER_SIM7020C_NBIOT
    // #endif

    #include "BlinkerSerialSIMNBIoT.h"

    BlinkerSerialSIMNBIoT   Blinker;

#elif defined(BLINKER_NBIOT_SIM7000)

    // #ifndef BLINKER_SIM7020C_NBIOT
    // #define BLINKER_SIM7020C_NBIOT
    // #endif

    #ifndef BLINKER_WITHOUT_SSL
        #define BLINKER_WITHOUT_SSL
    #endif

    #include "BlinkerSerialSIM7000NBIoT.h"

    BlinkerSerialSIM7000NBIoT   Blinker;

#elif defined(BLINKER_PRO_SIM7020)

    #include "BlinkerSIMPRO.h"

    BlinkerSIMPRO   Blinker;

#elif defined(BLINKER_GPRS_AIR202)

    // #ifndef BLINKER_AIR202_GPRS
    // #define BLINKER_AIR202_GPRS
    // #endif

    #include "BlinkerSerialLUATGPRS.h"

    BlinkerSerialLUATGPRS   Blinker;

#elif defined(BLINKER_PRO_AIR202)

    #include "BlinkerLUATPRO.h"

    BlinkerLUATPRO  Blinker;

// #elif defined(BLINKER_WIFI_AUTO)

//     #define BLINKER_MQTT_AUTO

//     #if !defined(BLINKER_ESP_SMARTCONFIG) && !defined(BLINKER_APCONFIG)
//         #define BLINKER_ESP_SMARTCONFIG
//     #endif

//     #include "BlinkerESPMQTTAUTO.h"

//     BlinkerESPMQTTAUTO  Blinker;

#elif defined(BLINKER_LOWPOWER_AIR202)
    
    #include "BlinkerLowPowerGPRS.h"

    BlinkerLowPowerGPRS Blinker;

#elif defined(BLINKER_QRCODE_NBIOT_SIM7020)
    
    #include "BlinkerQRCodeSIMNBIOT.h"

    BlinkerQRCodeSerialSIMNBIoT Blinker;

#elif defined(BLINKER_QRCODE_NBIOT_SIM7000)

    #ifndef BLINKER_WITHOUT_SSL
        #define BLINKER_WITHOUT_SSL
    #endif
    
    #include "BlinkerQRCodeSIM7000NBIOT.h"

    BlinkerQRCodeSerialSIM7000NBIoT Blinker;

#elif defined(BLINKER_HTTP)

    #include "BlinkerESPHTTP.h"

    BlinkerESPHTTP      Blinker;   

#else

    #error Please set a mode BLINKER_BLE/BLINKER_WIFI/BLINKER_MQTT ! Please check your mode setting.

#endif

#include "BlinkerWidgets.h"

// #if defined(BLINKER_MQTT)
#if defined(BLINKER_ESP_TASK)
#if defined(ESP8266)
    #error ESP8266 TASK NOT SUPPORT!
    // #include "Schedule.h"
    // extern "C" {
    // #include "ets_sys.h"
    // #include "user_interface.h"
    // #include "cont.h"
    // }

    // #define blinker_procTaskPrio        1
    // #define blinker_procTaskQueueLen    1
    // os_event_t    blinker_procTaskQueue[blinker_procTaskQueueLen];
    // cont_t* blinker_g_pcont __attribute__((section(".noinit")));

    // // uint32_t oldtime = 0;
    // // static uint32_t s_micros_at_task_start;

    // // static uint32_t oldtime = 0;

    // void preloop_update_frequency() __attribute__((weak));
    // void preloop_update_frequency() {
    // #if defined(F_CPU) && (F_CPU == 160000000L)
    //     REG_SET_BIT(0x3ff00014, BIT(0));
    //     ets_update_cpu_frequency(160);
    // #endif
    // }

    // static bool isInit = false;

    // static void blinker_loop_wrapper() {
    //     preloop_update_frequency();
    //     if (!isInit)
    //     {
    //         Blinker.beginMQTT();
    //         isInit = true;
    //     }
    //     else
    //     {
    //         Blinker.run();
    //     }
    //     // Blinker.run();
    //     run_scheduled_functions();
    //     // esp_schedule();
    // }

    // static void blinker_run(os_event_t *events)
    // {
    //     cont_run(blinker_g_pcont, &blinker_loop_wrapper);
    //     system_os_post(blinker_procTaskPrio, 0, (os_param_t)blinker_g_pcont);
    // }

    // void BLINKER_TAST_INIT()
    // {
    //     // ets_task
    //     // #if defined(BLINKER_MQTT)
    //     //     Blinker.beginMQTT();
    //     // #endif
    //     system_os_task(blinker_run, blinker_procTaskPrio, blinker_procTaskQueue, blinker_procTaskQueueLen);
    //     system_os_post(blinker_procTaskPrio, 0, (os_param_t)blinker_g_pcont);
    // }
// #endif

// #endif

// #if defined(ESP32)
#elif defined(ESP32)
    #include <freertos/FreeRTOS.h>
    #include <freertos/task.h>
    #include <Arduino.h>

    // #if CONFIG_AUTOSTART_ARDUINO

    #if CONFIG_FREERTOS_UNICORE
    #define ARDUINO_RUNNING_CORE 0
    #else
    #define ARDUINO_RUNNING_CORE 1
    #endif

    void blinkerLoopTask(void *pvParameters)
    {
        for(;;) {
            Blinker.run();
            vTaskDelay(1);
        }
    }

    extern "C" void BLINKER_TAST_INIT()
    {
        // initArduino();
        // #if defined(BLINKER_MQTT)
        //     Blinker.beginMQTT();
        // #endif
        xTaskCreatePinnedToCore(blinkerLoopTask, 
                                "blinkerLoopTask", 
                                8192, 
                                NULL, 
                                3, 
                                NULL, 
                                ARDUINO_RUNNING_CORE);
    }

    // #endif
#else
    #error This code is intended to run on the ESP8266/ESP32 platform! Please check your Tools->Board setting.
#endif

#endif

#ifndef LED_BUILTIN
    #if defined(ESP8266) || defined(ESP32)
        #define LED_BUILTIN 2
    #else
        #define LED_BUILTIN 13
    #endif
#endif

#endif
