#ifndef _PAINLESS_MESH_CONNECTION_H_
#define _PAINLESS_MESH_CONNECTION_H_

#if defined(ESP8266) || defined(ESP32)

#define _TASK_PRIORITY  // Support for layered scheduling priority
#define _TASK_STD_FUNCTION
#include "../TaskScheduler/TaskSchedulerDeclarations.h"

#ifdef ESP32
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif  // ESP32

#include "painlessmesh/buffer.hpp"
#include "painlessmesh/configuration.hpp"
#include "painlessmesh/layout.hpp"
#include "painlessmesh/mesh.hpp"

#ifndef PAINLESSMESH_ENABLE_ARDUINO_WIFI
class MeshConnection;
using painlessMesh = painlessmesh::Mesh<MeshConnection>;
#endif

class MeshConnection : public painlessmesh::layout::Neighbour,
                       public std::enable_shared_from_this<MeshConnection> {
 public:
  AsyncClient *client;
  painlessmesh::Mesh<MeshConnection> *mesh;

  bool newConnection = true;
  bool connected = true;
  bool station = true;

  // Timestamp to be compared in manageConnections() to check response
  // for timeout
  uint32_t timeDelayLastRequested = 0;

  bool addMessage(TSTRING &message, bool priority = false);
  bool writeNext();
  painlessmesh::buffer::ReceiveBuffer<TSTRING> receiveBuffer;
  painlessmesh::buffer::SentBuffer<TSTRING> sentBuffer;

  Task nodeSyncTask;
  Task timeSyncTask;
  Task readBufferTask;
  Task sentBufferTask;
  Task timeOutTask;

  MeshConnection(AsyncClient *client, painlessmesh::Mesh<MeshConnection> *pMesh,
                 bool station);
  ~MeshConnection();

  void initTCPCallbacks();
  void initTasks();

  void handleMessage(TSTRING msg, uint32_t receivedAt);

  void close();

  friend painlessmesh::Mesh<MeshConnection>;
};
#endif
#endif
