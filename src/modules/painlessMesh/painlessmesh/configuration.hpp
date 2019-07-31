#include <Arduino.h>
#ifndef _PAINLESS_MESH_CONFIGURATION_HPP_
#define _PAINLESS_MESH_CONFIGURATION_HPP_

#include <list>

#define _TASK_PRIORITY // Support for layered scheduling priority
#define _TASK_STD_FUNCTION

#include "../../TaskScheduler/TaskSchedulerDeclarations.h"

#define ARDUINOJSON_USE_LONG_LONG 1
#undef ARDUINOJSON_ENABLE_STD_STRING
// #include <ArduinoJson.h>
#ifndef ARDUINOJSON_VERSION_MAJOR
#include "../../ArduinoJson/ArduinoJson.h"
#endif
#undef ARDUINOJSON_ENABLE_STD_STRING

// Enable (arduino) wifi support
#define PAINLESSMESH_ENABLE_ARDUINO_WIFI

// Enable OTA support
#define PAINLESSMESH_ENABLE_OTA

#define NODE_TIMEOUT 5 * TASK_SECOND

#ifdef ESP32
#include <WiFi.h>
// #include <AsyncTCP.h>
#include "../../AsyncTCP/AsyncTCP.h"
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
// #include <ESPAsyncTCP.h>
#include "../../ESPAsyncTCP/ESPAsyncTCP.h"
#endif // ESP32

typedef String TSTRING;

// backward compatibility
template <typename T>
using SimpleList = std::list<T>;

namespace painlessmesh {
namespace wifi {
class Mesh;
};
};  // namespace painlessmesh

/** A convenience typedef to access the mesh class*/
using painlessMesh = painlessmesh::wifi::Mesh;

#ifdef ESP32
#define MAX_CONN 10
#else
#define MAX_CONN 4
#endif // DEBUG

#endif
