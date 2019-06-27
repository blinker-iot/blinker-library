#ifndef   _PAINLESS_MESH_CONNECTION_H_
#define   _PAINLESS_MESH_CONNECTION_H_

#define _TASK_PRIORITY // Support for layered scheduling priority
#define _TASK_STD_FUNCTION
#include <TaskSchedulerDeclarations.h>

#ifdef ESP32
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif // ESP32

#define ARDUINOJSON_USE_LONG_LONG 1
typedef String TSTRING;
#undef ARDUINOJSON_ENABLE_STD_STRING
#include "painlessmesh/buffer.hpp"
#include "painlessmesh/layout.hpp"

#include <string>
class MeshConnection : public painlessmesh::layout::Neighbour {
 public:
  AsyncClient *client;
  painlessMesh *mesh;

  bool newConnection = true;
  bool connected = true;
  bool station = true;

  // Timestamp to be compared in manageConnections() to check response
  // for timeout
  uint32_t timeDelayLastRequested = 0;

  bool addMessage(String &message, bool priority = false);
  bool writeNext();
  painlessmesh::buffer::ReceiveBuffer<String> receiveBuffer;
  painlessmesh::buffer::SentBuffer<String> sentBuffer;

  Task nodeSyncTask;
  Task timeSyncTask;
  Task readBufferTask;
  Task sentBufferTask;

#ifdef UNITY // Facilitate testing
        MeshConnection() {};
#endif
        MeshConnection(AsyncClient *client, painlessMesh *pMesh, bool station);
#ifndef UNITY
        ~MeshConnection();
#endif

        void handleMessage(String &msg, uint32_t receivedAt);

        void close();
        
        friend class painlessMesh;
};
#endif
