#ifndef _EASY_MESH_H_
#define _EASY_MESH_H_

#if defined(ESP8266) || defined(ESP32)

#define _TASK_PRIORITY  // Support for layered scheduling priority
#define _TASK_STD_FUNCTION

#include <Arduino.h>
#include <functional>
#include <list>
#include <memory>
#include "painlessmesh/configuration.hpp"
using namespace std;
#ifdef ESP32
// #include <AsyncTCP.h>
#include "modules/AsyncTCP/AsyncTCP.h"
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
// #include <ESPAsyncTCP.h>
#include "modules/ESPAsyncTCP/ESPAsyncTCP.h"
#endif  // ESP32

#ifdef PAINLESSMESH_ENABLE_ARDUINO_WIFI
#include "modules/painlessmesh/painlessMeshConnection.h"
#include "modules/painlessmesh/painlessMeshSTA.h"

#include "arduino/wifi.hpp"
#endif

#ifdef PAINLESSMESH_ENABLE_OTA
#include "modules/painlessmesh/painlessmesh/ota.hpp"
#endif

#include "modules/painlessmesh/painlessmesh/buffer.hpp"
#include "modules/painlessmesh/painlessmesh/layout.hpp"
#include "modules/painlessmesh/painlessmesh/logger.hpp"
#include "modules/painlessmesh/painlessmesh/mesh.hpp"
#include "modules/painlessmesh/painlessmesh/ntp.hpp"
#include "modules/painlessmesh/painlessmesh/plugin.hpp"
#include "modules/painlessmesh/painlessmesh/protocol.hpp"
#include "modules/painlessmesh/painlessmesh/router.hpp"
#include "modules/painlessmesh/painlessmesh/tcp.hpp"
using namespace painlessmesh::logger;

#define MIN_FREE_MEMORY \
  4000  // Minimum free memory, besides here all packets in queue are discarded.
#define MAX_MESSAGE_QUEUE \
  50  // MAX number of unsent messages in queue. Newer messages are discarded
#define MAX_CONSECUTIVE_SEND 5  // Max message burst

/*! \mainpage painlessMesh: A painless way to setup a mesh.
 *
 * painlessMesh is designed in a modular way, with many parent classes. The best
 * place to get started with the documentation is to have a look at
 * painlessmesh::wifi::Mesh (the main painlessMesh class is an alias (typedef)
 * of the painlessmesh::wifi::Mesh class). Make sure to also explore the public
 * member functions inherited from other classes, to get full information on the
 * functions available to you.
 */

#ifndef PAINLESSMESH_ENABLE_ARDUINO_WIFI
class MeshConnection;
using painlessMesh = painlessmesh::Mesh<MeshConnection>;
#endif

#endif  //   _EASY_MESH_H_
#endif
