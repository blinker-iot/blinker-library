#ifndef _PAINLESS_MESH_MESH_HPP_
#define _PAINLESS_MESH_MESH_HPP_

#include "configuration.hpp"

#include "ntp.hpp"
#include "plugin.hpp"
#include "tcp.hpp"

#ifdef PAINLESSMESH_ENABLE_OTA
#include "ota.hpp"
#endif

namespace painlessmesh {
typedef std::function<void(uint32_t nodeId)> newConnectionCallback_t;
typedef std::function<void(uint32_t nodeId)> droppedConnectionCallback_t;
typedef std::function<void(uint32_t from, TSTRING &msg)> receivedCallback_t;
typedef std::function<void()> changedConnectionsCallback_t;
typedef std::function<void(int32_t offset)> nodeTimeAdjustedCallback_t;
typedef std::function<void(uint32_t nodeId, int32_t delay)> nodeDelayCallback_t;

/**
 * Main api class for the mesh
 *
 * Brings all the functions together except for the WiFi functions
 */
template <class T>
class Mesh : public ntp::MeshTime, public plugin::PackageHandler<T> {
 public:
  void init(uint32_t id, uint16_t port) {
    if (!isExternalScheduler) {
      mScheduler = new Scheduler();
    }
    this->nodeId = id;

#ifdef ESP32
    xSemaphore = xSemaphoreCreateMutex();
#endif

    mScheduler->enableAll();

    // Add package handlers
    this->callbackList = painlessmesh::ntp::addPackageCallback(
        std::move(this->callbackList), (*this));
    this->callbackList = painlessmesh::router::addPackageCallback(
        std::move(this->callbackList), (*this));
  }

  void init(Scheduler *scheduler, uint32_t id, uint16_t port) {
    this->setScheduler(scheduler);
    this->init(id, port);
  }

#ifdef PAINLESSMESH_ENABLE_OTA
  void initOTA(TSTRING role = "") {
    painlessmesh::plugin::ota::addPackageCallback(*this->mScheduler, (*this),
                                                  role);
  }
#endif

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
  void setRoot(bool on = true) { this->root = on; };

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
  bool isRoot() { return this->root; };

  void setDebugMsgTypes(uint16_t types) { Log.setLogLevel(types); }

  /**
   * Disconnect and stop this node
   */
  void stop() {
    using namespace logger;
    // Close all connections
    while (this->subs.size() > 0) {
      auto conn = this->subs.begin();
      (*conn)->close();
      this->eraseClosedConnections();
    }
    plugin::PackageHandler<T>::stop();
  }

  /** Perform crucial maintenance task
   *
   * Add this to your loop() function. This routine runs various maintenance
   * tasks.
   */
  void update(void) {
    if (semaphoreTake()) {
      mScheduler->execute();
      semaphoreGive();
    }
    return;
  }

  /** Send message to a specific node
   *
   * @param destId The nodeId of the node to send it to.
   * @param msg The message to send
   *
   * @return true if everything works, false if not.
   */
  bool sendSingle(uint32_t destId, TSTRING msg) {
    Log(logger::COMMUNICATION, "sendSingle(): dest=%u msg=%s\n", destId,
        msg.c_str());
    auto single = painlessmesh::protocol::Single(this->nodeId, destId, msg);
    return painlessmesh::router::send<T>(single, (*this));
  }

  /** Broadcast a message to every node on the mesh network.
   *
   * @param includeSelf Send message to myself as well. Default is false.
   *
   * @return true if everything works, false if not
   */
  bool sendBroadcast(TSTRING msg, bool includeSelf = false) {
    using namespace logger;
    Log(COMMUNICATION, "sendBroadcast(): msg=%s\n", msg.c_str());
    auto pkg = painlessmesh::protocol::Broadcast(this->nodeId, 0, msg);
    auto success = router::broadcast<protocol::Broadcast, T>(pkg, (*this), 0);
    if (success && includeSelf) {
      auto variant = protocol::Variant(pkg);
      this->callbackList.execute(pkg.type, pkg, NULL, 0);
    }
    if (success > 0) return true;
    return false;
  }

  /** Sends a node a packet to measure network trip delay to that node.
   *
   * After calling this function, user program have to wait to the response in
   * the form of a callback specified by onNodeDelayReceived().
   *
   * @return true if nodeId is connected to the mesh, false otherwise
   */
  bool startDelayMeas(uint32_t id) {
    using namespace logger;
    Log(S_TIME, "startDelayMeas(): NodeId %u\n", id);
    auto conn = painlessmesh::router::findRoute<T>((*this), id);
    if (!conn) return false;
    return router::send<protocol::TimeDelay, T>(
        protocol::TimeDelay(this->nodeId, id, this->getNodeTime()), conn);
  }

  /** Set a callback routine for any messages that are addressed to this node.
   *
   * Every time this node receives a message, this callback routine will the
   * called.  “from” is the id of the original sender of the message, and “msg”
   * is a string that contains the message.  The message can be anything.  A
   * JSON, some other text string, or binary data.
   *
   * \code
   * mesh.onReceive([](auto nodeId, auto msg) {
   *    // Do something with the message
   *    Serial.println(msg);
   * });
   * \endcode
   */
  void onReceive(receivedCallback_t onReceive) {
    using namespace painlessmesh;
    this->callbackList.onPackage(
        protocol::SINGLE,
        [onReceive](protocol::Variant variant, std::shared_ptr<T>, uint32_t) {
          auto pkg = variant.to<protocol::Single>();
          onReceive(pkg.from, pkg.msg);
          return false;
        });
    this->callbackList.onPackage(
        protocol::BROADCAST,
        [onReceive](protocol::Variant variant, std::shared_ptr<T>, uint32_t) {
          auto pkg = variant.to<protocol::Broadcast>();
          onReceive(pkg.from, pkg.msg);
          return false;
        });
  }

  /** Callback that gets called every time the local node makes a new
   * connection.
   *
   * \code
   * mesh.onNewConnection([](auto nodeId) {
   *    // Do something with the event
   *    Serial.println(String(nodeId));
   * });
   * \endcode
   */
  void onNewConnection(newConnectionCallback_t onNewConnection) {
    Log(logger::GENERAL, "onNewConnection():\n");
    newConnectionCallback = onNewConnection;
  }

  /** Callback that gets called every time the local node drops a connection.
   *
   * \code
   * mesh.onDroppedConnection([](auto nodeId) {
   *    // Do something with the event
   *    Serial.println(String(nodeId));
   * });
   * \endcode
   */
  void onDroppedConnection(droppedConnectionCallback_t onDroppedConnection) {
    Log(logger::GENERAL, "onDroppedConnection():\n");
    droppedConnectionCallback = onDroppedConnection;
  }

  /** Callback that gets called every time the layout of the mesh changes
   *
   * \code
   * mesh.onChangedConnections([]() {
   *    // Do something with the event
   * });
   * \endcode
   */
  void onChangedConnections(changedConnectionsCallback_t onChangedConnections) {
    Log(logger::GENERAL, "onChangedConnections():\n");
    changedConnectionsCallback = onChangedConnections;
  }

  /** Callback that gets called every time node time gets adjusted
   *
   * Node time is automatically kept in sync in the mesh. This gets called
   * whenever the time is to far out of sync with the rest of the mesh and gets
   * adjusted.
   *
   * \code
   * mesh.onNodeTimeAdjusted([](auto offset) {
   *    // Do something with the event
   *    Serial.println(String(offset));
   * });
   * \endcode
   */
  void onNodeTimeAdjusted(nodeTimeAdjustedCallback_t onTimeAdjusted) {
    Log(logger::GENERAL, "onNodeTimeAdjusted():\n");
    nodeTimeAdjustedCallback = onTimeAdjusted;
  }

  /** Callback that gets called when a delay measurement is received.
   *
   * This fires when a time delay masurement response is received, after a
   * request was sent.
   *
   * \code
   * mesh.onNodeDelayReceived([](auto nodeId, auto delay) {
   *    // Do something with the event
   *    Serial.println(String(delay));
   * });
   * \endcode
   */
  void onNodeDelayReceived(nodeDelayCallback_t onDelayReceived) {
    Log(logger::GENERAL, "onNodeDelayReceived():\n");
    nodeDelayReceivedCallback = onDelayReceived;
  }

  /**
   * Are we connected/know a route to the given node?
   *
   * @param nodeId The nodeId we are looking for
   */
  bool isConnected(uint32_t nodeId) {
    return painlessmesh::router::findRoute<T>((*this), nodeId) != NULL;
  }

  /** Get a list of all known nodes.
   *
   * This includes nodes that are both directly and indirectly connected to the
   * current node.
   */
  std::list<uint32_t> getNodeList(bool includeSelf = false) {
    return painlessmesh::layout::asList(this->asNodeTree(), includeSelf);
  }

  /**
   * Return a json representation of the current mesh layout
   */
  inline TSTRING subConnectionJson(bool pretty = false) {
    return this->asNodeTree().toString(pretty);
  }

  inline std::shared_ptr<Task> addTask(unsigned long aInterval, long aIterations,
                                std::function<void()> aCallback) {
    return plugin::PackageHandler<T>::addTask((*this->mScheduler), aInterval, aIterations, aCallback);
  }

  inline std::shared_ptr<Task> addTask(std::function<void()> aCallback) {
    return plugin::PackageHandler<T>::addTask((*this->mScheduler), aCallback);
  }

  ~Mesh() {
    this->stop();
    if (!isExternalScheduler) delete mScheduler;
  }

 protected:
  void setScheduler(Scheduler *baseScheduler) {
    this->mScheduler = baseScheduler;
    isExternalScheduler = true;
  }

  void startTimeSync(std::shared_ptr<T> conn) {
    using namespace logger;
    Log(S_TIME, "startTimeSync(): from %u with %u\n", this->nodeId,
        conn->nodeId);
    painlessmesh::protocol::TimeSync timeSync;
    if (ntp::adopt(this->asNodeTree(), (*conn))) {
      timeSync = painlessmesh::protocol::TimeSync(this->nodeId, conn->nodeId,
                                                  this->getNodeTime());
      Log(S_TIME, "startTimeSync(): Requesting time from %u\n", conn->nodeId);
    } else {
      timeSync = painlessmesh::protocol::TimeSync(this->nodeId, conn->nodeId);
      Log(S_TIME, "startTimeSync(): Requesting %u to adopt our time\n",
          conn->nodeId);
    }
    router::send<protocol::TimeSync, T>(timeSync, conn, true);
  }

  bool closeConnectionSTA() {
    auto connection = this->subs.begin();
    while (connection != this->subs.end()) {
      if ((*connection)->station) {
        // We found the STA connection, close it
        (*connection)->close();
        return true;
      }
      ++connection;
    }
    return false;
  }

  void eraseClosedConnections() {
    using namespace logger;
    Log(CONNECTION, "eraseClosedConnections():\n");
    this->subs.remove_if(
        [](const std::shared_ptr<T> &conn) { return !conn->connected; });
  }

  // Callback functions
  newConnectionCallback_t newConnectionCallback;
  droppedConnectionCallback_t droppedConnectionCallback;
  changedConnectionsCallback_t changedConnectionsCallback;
  nodeTimeAdjustedCallback_t nodeTimeAdjustedCallback;
  nodeDelayCallback_t nodeDelayReceivedCallback;
#ifdef ESP32
  SemaphoreHandle_t xSemaphore = NULL;
#endif

  bool isExternalScheduler = false;

  /// Is the node a root node
  bool shouldContainRoot;

  Scheduler *mScheduler;

  /**
   * Wrapper function for ESP32 semaphore function
   *
   * Waits for the semaphore to be available and then returns true
   *
   * Always return true on ESP8266
   */
  bool semaphoreTake() {
#ifdef ESP32
    return xSemaphoreTake(xSemaphore, (TickType_t)10) == pdTRUE;
#else
    return true;
#endif
  }

  /**
   * Wrapper function for ESP32 semaphore give function
   *
   * Does nothing on ESP8266 hardware
   */
  void semaphoreGive() {
#ifdef ESP32
    xSemaphoreGive(xSemaphore);
#endif
  }

  friend T;
  friend void onDataCb(void *, AsyncClient *, void *, size_t);
  friend void tcpSentCb(void *, AsyncClient *, size_t, uint32_t);
  friend void meshRecvCb(void *, AsyncClient *, void *, size_t);
  friend void painlessmesh::ntp::handleTimeSync<Mesh, T>(
      Mesh &, painlessmesh::protocol::TimeSync, std::shared_ptr<T>, uint32_t);
  friend void painlessmesh::ntp::handleTimeDelay<Mesh, T>(
      Mesh &, painlessmesh::protocol::TimeDelay, std::shared_ptr<T>, uint32_t);
  friend void painlessmesh::router::handleNodeSync<Mesh, T>(
      Mesh &, protocol::NodeTree, std::shared_ptr<T> conn);
  friend void painlessmesh::tcp::initServer<T, Mesh>(AsyncServer &, Mesh &);
  friend void painlessmesh::tcp::connect<T, Mesh>(AsyncClient &, IPAddress,
                                                  uint16_t, Mesh &);
};  // namespace painlessmesh
};  // namespace painlessmesh
#endif
