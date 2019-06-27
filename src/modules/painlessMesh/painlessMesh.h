#ifndef   _EASY_MESH_H_
#define   _EASY_MESH_H_

#define _TASK_PRIORITY // Support for layered scheduling priority
#define _TASK_STD_FUNCTION

#include <TaskSchedulerDeclarations.h>
#include <Arduino.h>
#include <list>
#define ARDUINOJSON_USE_LONG_LONG 1
// #include <ArduinoJson.h>
#include "modules/ArduinoJson/ArduinoJson.h"
#include <functional>
#include <memory>
using namespace std;
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
// #include <ESPAsyncTCP.h>
#include "modules/ESPAsyncTCP/ESPAsyncTCP.h"
#endif // ESP32

#include "painlessMeshSTA.h"
#include "painlessMeshConnection.h"

typedef String TSTRING;
#undef ARDUINOJSON_ENABLE_STD_STRING
#include "painlessmesh/buffer.hpp"
#include "painlessmesh/layout.hpp"
#include "painlessmesh/logger.hpp"
#include "painlessmesh/ntp.hpp"
#include "painlessmesh/protocol.hpp"
using namespace painlessmesh::logger;

#define NODE_TIMEOUT         10*TASK_SECOND
#define MIN_FREE_MEMORY      4000 // Minimum free memory, besides here all packets in queue are discarded.
#define MAX_MESSAGE_QUEUE    50 // MAX number of unsent messages in queue. Newer messages are discarded
#define MAX_CONSECUTIVE_SEND 5 // Max message burst

template<typename T>
using SimpleList = std::list<T>; // backward compatibility

#ifdef ESP32
#define MAX_CONN 10
#else
#define MAX_CONN 4
#endif // DEBUG

using ConnectionList = std::list<std::shared_ptr<MeshConnection>>;

typedef std::function<void(uint32_t nodeId)> newConnectionCallback_t;
typedef std::function<void(uint32_t nodeId)> droppedConnectionCallback_t;
typedef std::function<void(uint32_t from, String &msg)> receivedCallback_t;
typedef std::function<void()> changedConnectionsCallback_t;
typedef std::function<void(int32_t offset)> nodeTimeAdjustedCallback_t;
typedef std::function<void(uint32_t nodeId, int32_t delay)> nodeDelayCallback_t;

class painlessMesh : public painlessmesh::layout::Layout<MeshConnection> {
 public:
  // inline functions
  uint32_t getNodeId(void) { return nodeId; };

  /**
   * Set the node as an root/master node for the mesh
   *
   * This is an optional setting that can speed up mesh formation.
   * At most one node in the mesh should be a root, or you could
   * end up with multiple subMeshes.
   *
   * We recommend any AP_ONLY nodes (e.g. a bridgeNode) to be set
   * as a root node.
   *
   * If one node is root, then it is also recommended to call
   * painlessMesh::setContainsRoot() on all the nodes in the mesh.
   */
  void setRoot(bool on = true) { root = on; };

  /**
   * The mesh should contains a root node
   *
   * This will cause the mesh to restructure more quickly around the root node.
   * Note that this could have adverse effects if set, while there is no root
   * node present. Also see painlessMesh::setRoot().
   */
  void setContainsRoot(bool on = true) { shouldContainRoot = on; };

  /**
   * Check whether this node is a root node.
   */
  bool isRoot() { return root; };

  // in painlessMeshDebug.cpp
  void setDebugMsgTypes(uint16_t types);

  // in painlessMesh.cpp
  painlessMesh();
  void init(String ssid, String password, Scheduler *baseScheduler,
            uint16_t port = 5555, WiFiMode_t connectMode = WIFI_AP_STA,
            uint8_t channel = 1, uint8_t hidden = 0,
            uint8_t maxconn = MAX_CONN);
  void init(String ssid, String password, uint16_t port = 5555,
            WiFiMode_t connectMode = WIFI_AP_STA, uint8_t channel = 1,
            uint8_t hidden = 0, uint8_t maxconn = MAX_CONN);
  /**
   * Disconnect and stop this node
   */
  void stop();
  void update(void);
  bool sendSingle(uint32_t &destId, String &msg);
  bool sendBroadcast(String &msg, bool includeSelf = false);
  bool startDelayMeas(uint32_t nodeId);

  // in painlessMeshConnection.cpp
  void onReceive(receivedCallback_t onReceive);
  void onNewConnection(newConnectionCallback_t onNewConnection);
  void onDroppedConnection(droppedConnectionCallback_t onDroppedConnection);
  void onChangedConnections(changedConnectionsCallback_t onChangedConnections);
  void onNodeTimeAdjusted(nodeTimeAdjustedCallback_t onTimeAdjusted);
  void onNodeDelayReceived(nodeDelayCallback_t onDelayReceived);

  bool isConnected(uint32_t nodeId) {
    return painlessmesh::layout::findRoute<MeshConnection>((*this), nodeId) !=
           NULL;
  }

  std::list<uint32_t> getNodeList(bool includeSelf = false);

  /**
   * Return a json representation of the current mesh layout
   */
  inline TSTRING subConnectionJson(bool pretty = false) {
    return this->asNodeTree().toString(pretty);
  }

  // in painlessMeshSync.cpp
  uint32_t getNodeTime(void);

  // in painlessMeshSTA.cpp
  uint32_t encodeNodeId(const uint8_t *hwaddr);
  /**
   * Connect (as a station) to a specified network and ip
   *
   * You can pass {0,0,0,0} as IP to have it connect to the gateway
   *
   * This stops the node from scanning for other (non specified) nodes
   * and you should probably also use this node as an anchor: `setAnchor(true)`
   */
  void stationManual(String ssid, String password, uint16_t port = 0,
                     IPAddress remote_ip = IPAddress(0, 0, 0, 0));
  bool setHostname(const char *hostname);
  IPAddress getStationIP();

  StationScan stationScan;

  // Rough estimate of the mesh stability (goes from 0-1000)
  size_t stability = 0;

  // in painlessMeshAP.cpp
  IPAddress getAPIP();

#if __cplusplus > 201103L
    [[deprecated("Use of the internal scheduler will be deprecated, please use an user provided scheduler instead (See the startHere example).")]]
#endif
    Scheduler &scheduler = _scheduler;

#ifndef UNITY // Make everything public in unit test mode
protected:
#endif
 template <typename T>
 bool send(std::shared_ptr<MeshConnection> conn, T package,
           bool priority = false) {
   auto variant = painlessmesh::protocol::Variant(package);
   String msg;
   variant.printTo(msg);
   // Log(COMMUNICATION, "send<>(conn): conn-nodeId=%u pkg=%s\n",
   //         conn->nodeId, msg.c_str());
   return conn->addMessage(msg, priority);
    }

    template <typename T>
    bool send(T package, bool priority = false) {
      std::shared_ptr<MeshConnection> conn =
          painlessmesh::layout::findRoute<MeshConnection>((*this),
                                                          package.dest);
      if (conn) {
        return send<T>(conn, package, priority);
      } else {
        // Log(ERROR, "In sendMessage(destId): findConnection( %u ) failed\n",
        //         package.dest);
        return false;
      }
    }

    bool broadcastMessage(painlessmesh::protocol::Broadcast pkg,
                          std::shared_ptr<MeshConnection> exclude = NULL);

    void handleNodeSync(std::shared_ptr<MeshConnection> conn,
                        painlessmesh::protocol::NodeTree newTree);

    void                startTimeSync(std::shared_ptr<MeshConnection> conn);
    void handleTimeSync(std::shared_ptr<MeshConnection> conn,
                        painlessmesh::protocol::TimeSync, uint32_t receivedAt);
    void handleTimeDelay(std::shared_ptr<MeshConnection> conn,
                         painlessmesh::protocol::TimeDelay timeDelay,
                         uint32_t receivedAt);

    bool                adoptionCalc(std::shared_ptr<MeshConnection> conn);

    // Update other connections of a change
    void                syncSubConnections(uint32_t changedId);

    // in painlessMeshConnection.cpp
    //void                cleanDeadConnections(void); // Not implemented. Needed?
    void                tcpConnect(void);
    bool                closeConnectionSTA(); 

    void                eraseClosedConnections();

    std::list<uint32_t> getNodeList(String &subConnections);

    // in painlessMeshAP.cpp
    void                apInit(void);

    void                tcpServerInit();

    // callbacks
    // in painlessMeshConnection.cpp
    void                eventHandleInit();

    // Callback functions
    newConnectionCallback_t         newConnectionCallback;
    droppedConnectionCallback_t     droppedConnectionCallback;
    receivedCallback_t              receivedCallback;
    changedConnectionsCallback_t    changedConnectionsCallback;
    nodeTimeAdjustedCallback_t      nodeTimeAdjustedCallback;
    nodeDelayCallback_t             nodeDelayReceivedCallback;
#ifdef ESP32
    SemaphoreHandle_t xSemaphore = NULL;

    WiFiEventId_t eventScanDoneHandler;
    WiFiEventId_t eventSTAStartHandler;
    WiFiEventId_t eventSTADisconnectedHandler;
    WiFiEventId_t eventSTAGotIPHandler;
#elif defined(ESP8266)
    WiFiEventHandler  eventSTAConnectedHandler;
    WiFiEventHandler  eventSTADisconnectedHandler;
    WiFiEventHandler  eventSTAAuthChangeHandler;
    WiFiEventHandler  eventSTAGotIPHandler;
    WiFiEventHandler  eventSoftAPConnectedHandler;
    WiFiEventHandler  eventSoftAPDisconnectedHandler;
#endif // ESP8266
    String            _meshSSID;
    String            _meshPassword;
    uint16_t          _meshPort;
    uint8_t           _meshChannel;
    uint8_t           _meshHidden;
    uint8_t           _meshMaxConn;

    IPAddress         _apIp;

    AsyncServer       *_tcpListener;

    bool              _station_got_ip = false;

    bool              isExternalScheduler = false;

    /// Is the node a root node
    bool shouldContainRoot;

    Scheduler         _scheduler;
    Task              droppedConnectionTask;
    Task              newConnectionTask;

    /**
     * Wrapper function for ESP32 semaphore function
     *
     * Waits for the semaphore to be available and then returns true
     *
     * Always return true on ESP8266
     */
    bool semaphoreTake();
    /**
     * Wrapper function for ESP32 semaphore give function
     *
     * Does nothing on ESP8266 hardware
     */
    void semaphoreGive();

    friend class StationScan;
    friend class MeshConnection;
    friend void onDataCb(void * arg, AsyncClient *client, void *data, size_t len);
    friend void tcpSentCb(void * arg, AsyncClient * tpcb, size_t len, uint32_t time);
    friend void meshRecvCb(void * arg, AsyncClient *, void * data, size_t len);
};

#endif //   _EASY_MESH_H_
