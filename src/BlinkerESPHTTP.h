#ifndef BLINKER_ESP_MQTT_H
#define BLINKER_ESP_MQTT_H

#if defined(ESP8266) || defined(ESP32)

#ifndef BLINKER_ARDUINOJSON
    #define BLINKER_ARDUINOJSON
#endif

#include "Adapters/BlinkerMQTT.h"
#include "Blinker/BlinkerApi.h"
#ifndef ARDUINOJSON_VERSION_MAJOR
#include "modules/ArduinoJson/ArduinoJson.h"
#endif

#if defined(ESP8266)
    #include <ESP8266mDNS.h>
    #include <ESP8266WiFi.h>
    // #include <ESP8266WebServer.h>
#elif defined(ESP32)
    #include <ESPmDNS.h>
    #include <WiFi.h>
    // #include <WebServer.h>
#endif

typedef BlinkerApi BApi;

#endif

#endif
