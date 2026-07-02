#ifndef BLINKER_H
#define BLINKER_H

#include "Blinker/BlinkerPlatform.h"

#if defined(BLINKER_BLE)

    #if defined(ESP32)
        #include "BlinkerESP32BLE.h"

        BlinkerESP32BLE     Blinker;
    #else
        #include "BlinkerSerialBLE.h"

        BlinkerSerialBLE    Blinker;
    #endif

#elif defined(BLINKER_WIFI) || defined(BLINKER_MQTT)

    //     #define BLINKER_WITHOUT_WS_REG
    // #endif

    #if defined(BLINKER_WIFI)
        #undef BLINKER_WIFI
        #define BLINKER_MQTT
    #endif

    #define BLINKER_PROTOCOL_HTTP_SERVER


    #if defined(BLINKER_NATIVE_WIFI)
        #include "BlinkerESPMQTT.h"

        BlinkerESPMQTT      Blinker; 

    #else
        #error BLINKER_WIFI/BLINKER_MQTT requires ESP32, Arduino UNO R4 WiFi, or Raspberry Pi Pico W.
    #endif

#elif defined(BLINKER_HTTP)

    #if defined(BLINKER_NATIVE_WIFI)
        #include "BlinkerESPHTTP.h"

        BlinkerESPHTTP      Blinker;
    #else
        #error BLINKER_HTTP requires ESP32, Arduino UNO R4 WiFi, or Raspberry Pi Pico W.
    #endif

#else

    #error Please set a mode BLINKER_BLE/BLINKER_WIFI/BLINKER_MQTT ! Please check your mode setting.

#endif

#include "BlinkerWidgets.h"

// #if defined(BLINKER_MQTT)
#if defined(BLINKER_ESP_TASK)
#if defined(ESP32)
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
            // esp_task_wdt_feed();
            vTaskDelay(1);
            // esp_task_wdt_feed();
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
    #error BLINKER_ESP_TASK is intended to run on the ESP32 platform! Please check your Tools->Board setting.
#endif

#endif

#ifndef LED_BUILTIN
    #if defined(ESP32)
        #define LED_BUILTIN 2
    #else
        #define LED_BUILTIN 13
    #endif
#endif

#endif
