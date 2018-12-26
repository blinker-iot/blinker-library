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

    #if defined(BLINKER_WIFI)
        #undef BLINKER_WIFI
        #define BLINKER_MQTT
    #endif

    #if defined(BLINKER_ALIGENIE_LIGHT)
        #if defined(BLINKER_ALIGENIE_OUTLET)
            #undef BLINKER_ALIGENIE_OUTLET
        #endif
        #if defined(BLINKER_ALIGENIE_SWITCH)
            #undef BLINKER_ALIGENIE_SWITCH
        #endif
        #if defined(BLINKER_ALIGENIE_SENSOR)
            #undef BLINKER_ALIGENIE_SENSOR
        #endif

        #define BLINKER_ALIGENIE
    #elif defined(BLINKER_ALIGENIE_OUTLET)
        #if defined(BLINKER_ALIGENIE_LIGHT)
            #undef BLINKER_ALIGENIE_LIGHT
        #endif
        #if defined(BLINKER_ALIGENIE_SWITCH)
            #undef BLINKER_ALIGENIE_SWITCH
        #endif
        #if defined(BLINKER_ALIGENIE_SENSOR)
            #undef BLINKER_ALIGENIE_SENSOR
        #endif

        #define BLINKER_ALIGENIE
    #elif defined(BLINKER_ALIGENIE_SWITCH)
        #if defined(BLINKER_ALIGENIE_LIGHT)
            #undef BLINKER_ALIGENIE_LIGHT
        #endif
        #if defined(BLINKER_ALIGENIE_OUTLET)
            #undef BLINKER_ALIGENIE_OUTLET
        #endif
        #if defined(BLINKER_ALIGENIE_SENSOR)
            #undef BLINKER_ALIGENIE_SENSOR
        #endif

        #define BLINKER_ALIGENIE
    #elif defined(BLINKER_ALIGENIE_SENSOR)
        #if defined(BLINKER_ALIGENIE_LIGHT)
            #undef BLINKER_ALIGENIE_LIGHT
        #endif
        #if defined(BLINKER_ALIGENIE_OUTLET)
            #undef BLINKER_ALIGENIE_OUTLET
        #endif
        #if defined(BLINKER_ALIGENIE_SWITCH)
            #undef BLINKER_ALIGENIE_SWITCH
        #endif
        
        #define BLINKER_ALIGENIE
    #endif

    #if defined(BLINKER_DUEROS_LIGHT)
        #if defined(BLINKER_DUEROS_OUTLET)
            #undef BLINKER_DUEROS_OUTLET
        #endif
        #if defined(BLINKER_DUEROS_SWITCH)
            #undef BLINKER_DUEROS_SWITCH
        #endif
        #if defined(BLINKER_DUEROS_SENSOR)
            #undef BLINKER_DUEROS_SENSOR
        #endif

        #define BLINKER_DUEROS
    #elif defined(BLINKER_DUEROS_OUTLET)
        #if defined(BLINKER_DUEROS_LIGHT)
            #undef BLINKER_DUEROS_LIGHT
        #endif
        #if defined(BLINKER_DUEROS_SWITCH)
            #undef BLINKER_DUEROS_SWITCH
        #endif
        #if defined(BLINKER_DUEROS_SENSOR)
            #undef BLINKER_DUEROS_SENSOR
        #endif

        #define BLINKER_DUEROS
    #elif defined(BLINKER_DUEROS_SENSOR)
        #if defined(BLINKER_DUEROS_LIGHT)
            #undef BLINKER_DUEROS_LIGHT
        #endif
        #if defined(BLINKER_DUEROS_SWITCH)
            #undef BLINKER_DUEROS_SWITCH
        #endif
        #if defined(BLINKER_DUEROS_OUTLET)
            #undef BLINKER_DUEROS_OUTLET
        #endif

        #define BLINKER_DUEROS
    #endif

    #if defined(ESP8266) || defined(ESP32)
        #include "BlinkerESPMQTT.h"

        BlinkerESPMQTT      Blinker;     
    #else
        #define BLINKER_ESP_AT

        #define BLINKER_MQTT_AT

        #undef BLINKER_MQTT
        
        #include "BlinkerSerialESPMQTT.h"

        BlinkerSerialESPMQTT      Blinker;  
    #endif

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

    #if defined(ESP8266) || defined(ESP32)
        #include "BlinkerESPPRO.h"
        
        BlinkerESPPRO       Blinker; 
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

#endif

#include "BlinkerWidgets.h"

// #if defined(BLINKER_MQTT)
#if defined(BLINKER_ESP_TASK)
#if defined(ESP8266)
    #include "Schedule.h"
    extern "C" {
    #include "ets_sys.h"
    #include "user_interface.h"
    #include "cont.h"
    }

    #define blinker_procTaskPrio        0
    #define blinker_procTaskQueueLen    1
    os_event_t    blinker_procTaskQueue[blinker_procTaskQueueLen];
    // cont_t* blinker_g_pcont __attribute__((section(".noinit")));

    // uint32_t oldtime = 0;
    // static uint32_t s_micros_at_task_start;

    // static uint32_t oldtime = 0;

    void preloop_update_frequency() __attribute__((weak));
    void preloop_update_frequency() {
    #if defined(F_CPU) && (F_CPU == 160000000L)
        REG_SET_BIT(0x3ff00014, BIT(0));
        ets_update_cpu_frequency(160);
    #endif
    }

    static void blinker_loop_wrapper() {
        preloop_update_frequency();
        Blinker.run();
        run_scheduled_functions();
        // esp_schedule();
    }

    static void blinker_run(os_event_t *events)
    {
        cont_run(g_pcont, &blinker_loop_wrapper);
        system_os_post(blinker_procTaskPrio, 0, 0 );
    }

    void BLINKER_TAST_INIT()
    {
        // ets_task
        #if defined(BLINKER_MQTT)
            Blinker.beginMQTT();
        #endif
        system_os_task(blinker_run, blinker_procTaskPrio, blinker_procTaskQueue, blinker_procTaskQueueLen);
        system_os_post(blinker_procTaskPrio, 0, 0 );
    }
// #endif

// #endif

// #if defined(ESP32)
#elif defined(ESP32)
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "Arduino.h"

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
        #if defined(BLINKER_MQTT)
            Blinker.beginMQTT();
        #endif
        xTaskCreatePinnedToCore(blinkerLoopTask, "blinkerLoopTask", 4096, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
    }

    // #endif
#else
    #error This code is intended to run on the ESP8266/ESP32 platform! Please check your Tools->Board setting.
#endif

#endif

#endif
