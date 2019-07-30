#ifndef _PAINLESS_MESH_PROTOCOL_HPP_
#define _PAINLESS_MESH_PROTOCOL_HPP_

#include <cmath>
#include <list>

#include <Arduino.h>
#include "configuration.hpp"

namespace painlessmesh {

namespace router {

/** Different ways to route packages
 *
 * NEIGHBOUR packages are send to the neighbour and will be immediately handled
 * there. The TIME_SYNC and NODE_SYNC packages are NEIGHBOUR. SINGLE messages
 * are meant for a specific node. When another node receives this message, it
 * will look in its routing information and send it on to the correct node,
 * withouth processing the message in any other way. Only the targetted node
 * will actually parse/handle this message (without sending it on). Finally,
 * BROADCAST message are send to every node and processed/handled by every node.
 * */
enum Type { ROUTING_ERROR = -1, NEIGHBOUR, SINGLE, BROADCAST };
}  // namespace router

namespace protocol {

enum Type {
  TIME_DELAY = 3,
  TIME_SYNC = 4,
  NODE_SYNC_REQUEST = 5,
  NODE_SYNC_REPLY = 6,
  CONTROL = 7,    // deprecated
  BROADCAST = 8,  // application data for everyone
  SINGLE = 9      // application data for a single node
};

enum TimeType {
  TIME_SYNC_ERROR = -1,
  TIME_SYNC_REQUEST,
  TIME_REQUEST,
  TIME_REPLY
};

class PackageInterface {
 public:
  virtual JsonObject addTo(JsonObject&& jsonObj) const = 0;
  virtual size_t jsonObjectSize() const = 0;
};

/**
 * Single package
 *
 * Message send to a specific node
 */
class Single : public PackageInterface {
 public:
  int type = SINGLE;
  uint32_t from;
  uint32_t dest;
  TSTRING msg = "";

  Single() {}
  Single(uint32_t fromID, uint32_t destID, TSTRING& message) {
    from = fromID;
    dest = destID;
    msg = message;
  }

  Single(JsonObject jsonObj) {
    dest = jsonObj["dest"].as<uint32_t>();
    from = jsonObj["from"].as<uint32_t>();
    msg = jsonObj["msg"].as<TSTRING>();
  }

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj["type"] = type;
    jsonObj["dest"] = dest;
    jsonObj["from"] = from;
    jsonObj["msg"] = msg;
    return jsonObj;
  }

  size_t jsonObjectSize() const {
    return JSON_OBJECT_SIZE(4) + round(1.1 * msg.length());
  }
};

/**
 * Broadcast package
 */
class Broadcast : public Single {
 public:
  int type = BROADCAST;

  using Single::Single;

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj = Single::addTo(std::move(jsonObj));
    jsonObj["type"] = type;
    return jsonObj;
  }

  size_t jsonObjectSize() const {
    return JSON_OBJECT_SIZE(4) + round(1.1 * msg.length());
  }
};

class NodeTree : public PackageInterface {
 public:
  uint32_t nodeId = 0;
  bool root = false;
  std::list<NodeTree> subs;

  NodeTree() {}

  NodeTree(uint32_t nodeID, bool iAmRoot) {
    nodeId = nodeID;
    root = iAmRoot;
  }

  NodeTree(JsonObject jsonObj) {
    if (jsonObj.containsKey("root")) root = jsonObj["root"].as<bool>();
    if (jsonObj.containsKey("nodeId"))
      nodeId = jsonObj["nodeId"].as<uint32_t>();
    else
      nodeId = jsonObj["from"].as<uint32_t>();

    if (jsonObj.containsKey("subs")) {
      auto jsonArr = jsonObj["subs"].as<JsonArray>();
      for (size_t i = 0; i < jsonArr.size(); ++i) {
        subs.push_back(NodeTree(jsonArr[i].as<JsonObject>()));
      }
    }
  }

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj["nodeId"] = nodeId;
    if (root) jsonObj["root"] = root;
    if (subs.size() > 0) {
      JsonArray subsArr = jsonObj.createNestedArray("subs");
      for (auto&& s : subs) {
        JsonObject subObj = subsArr.createNestedObject();
        subObj = s.addTo(std::move(subObj));
      }
    }
    return jsonObj;
  }

  bool operator==(const NodeTree& b) const {
    if (!(this->nodeId == b.nodeId && this->root == b.root &&
          this->subs.size() == b.subs.size()))
      return false;
    auto itA = this->subs.begin();
    auto itB = b.subs.begin();
    for (size_t i = 0; i < this->subs.size(); ++i) {
      if ((*itA) != (*itB)) {
        return false;
      }
      ++itA;
      ++itB;
    }
    return true;
  }

  bool operator!=(const NodeTree& b) const { return !this->operator==(b); }

  TSTRING toString(bool pretty = false);

  size_t jsonObjectSize() const {
    size_t base = 1;
    if (root) ++base;
    if (subs.size() > 0) ++base;
    size_t size = JSON_OBJECT_SIZE(base);
    if (subs.size() > 0) size += JSON_ARRAY_SIZE(subs.size());
    for (auto&& s : subs) size += s.jsonObjectSize();
    return size;
  }

  void clear() {
    nodeId = 0;
    subs.clear();
    root = false;
  }
};

/**
 * NodeSyncRequest package
 */
class NodeSyncRequest : public NodeTree {
 public:
  int type = NODE_SYNC_REQUEST;
  uint32_t from;
  uint32_t dest;

  NodeSyncRequest() {}
  NodeSyncRequest(uint32_t fromID, uint32_t destID, std::list<NodeTree> subTree,
                  bool iAmRoot = false) {
    from = fromID;
    dest = destID;
    subs = subTree;
    nodeId = fromID;
    root = iAmRoot;
  }

  NodeSyncRequest(JsonObject jsonObj) : NodeTree(jsonObj) {
    dest = jsonObj["dest"].as<uint32_t>();
    from = jsonObj["from"].as<uint32_t>();
  }

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj = NodeTree::addTo(std::move(jsonObj));
    jsonObj["type"] = type;
    jsonObj["dest"] = dest;
    jsonObj["from"] = from;
    return jsonObj;
  }

  bool operator==(const NodeSyncRequest& b) const {
    if (!(this->from == b.from && this->dest == b.dest)) return false;
    return NodeTree::operator==(b);
  }

  bool operator!=(const NodeSyncRequest& b) const {
    return !this->operator==(b);
  }

  size_t jsonObjectSize() const {
    size_t base = 4;
    if (root) ++base;
    if (subs.size() > 0) ++base;
    size_t size = JSON_OBJECT_SIZE(base);
    if (subs.size() > 0) size += JSON_ARRAY_SIZE(subs.size());
    for (auto&& s : subs) size += s.jsonObjectSize();
    return size;
  }
};

/**
 * NodeSyncReply package
 */
class NodeSyncReply : public NodeSyncRequest {
 public:
  int type = NODE_SYNC_REPLY;

  using NodeSyncRequest::NodeSyncRequest;

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj = NodeSyncRequest::addTo(std::move(jsonObj));
    jsonObj["type"] = type;
    return jsonObj;
  }
};

struct time_sync_msg_t {
  int type = TIME_SYNC_ERROR;
  uint32_t t0 = 0;
  uint32_t t1 = 0;
  uint32_t t2 = 0;
};

/**
 * TimeSync package
 */
class TimeSync : public PackageInterface {
 public:
  int type = TIME_SYNC;
  uint32_t dest;
  uint32_t from;
  time_sync_msg_t msg;

  TimeSync() {}

  TimeSync(uint32_t fromID, uint32_t destID) {
    from = fromID;
    dest = destID;
    msg.type = TIME_SYNC_REQUEST;
  }

  TimeSync(uint32_t fromID, uint32_t destID, uint32_t t0) {
    from = fromID;
    dest = destID;
    msg.type = TIME_REQUEST;
    msg.t0 = t0;
  }

  TimeSync(uint32_t fromID, uint32_t destID, uint32_t t0, uint32_t t1) {
    from = fromID;
    dest = destID;
    msg.type = TIME_REPLY;
    msg.t0 = t0;
    msg.t1 = t1;
  }

  TimeSync(uint32_t fromID, uint32_t destID, uint32_t t0, uint32_t t1,
           uint32_t t2) {
    from = fromID;
    dest = destID;
    msg.type = TIME_REPLY;
    msg.t0 = t0;
    msg.t1 = t1;
    msg.t2 = t2;
  }

  TimeSync(JsonObject jsonObj) {
    dest = jsonObj["dest"].as<uint32_t>();
    from = jsonObj["from"].as<uint32_t>();
    msg.type = jsonObj["msg"]["type"].as<int>();
    if (jsonObj["msg"].containsKey("t0"))
      msg.t0 = jsonObj["msg"]["t0"].as<uint32_t>();
    if (jsonObj["msg"].containsKey("t1"))
      msg.t1 = jsonObj["msg"]["t1"].as<uint32_t>();
    if (jsonObj["msg"].containsKey("t2"))
      msg.t2 = jsonObj["msg"]["t2"].as<uint32_t>();
  }

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj["type"] = type;
    jsonObj["dest"] = dest;
    jsonObj["from"] = from;
    auto msgObj = jsonObj.createNestedObject("msg");
    msgObj["type"] = msg.type;
    if (msg.type >= 1) msgObj["t0"] = msg.t0;
    if (msg.type >= 2) {
      msgObj["t1"] = msg.t1;
      msgObj["t2"] = msg.t2;
    }
    return jsonObj;
  }

  /**
   * Create a reply to the current message with the new time set
   */
  void reply(uint32_t newT0) {
    msg.t0 = newT0;
    ++msg.type;
    std::swap(from, dest);
  }

  /**
   * Create a reply to the current message with the new time set
   */
  void reply(uint32_t newT1, uint32_t newT2) {
    msg.t1 = newT1;
    msg.t2 = newT2;
    ++msg.type;
    std::swap(from, dest);
  }

  size_t jsonObjectSize() const {
    return JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(4);
  }
};

/**
 * TimeDelay package
 */
class TimeDelay : public TimeSync {
 public:
  int type = TIME_DELAY;
  using TimeSync::TimeSync;

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj = TimeSync::addTo(std::move(jsonObj));
    jsonObj["type"] = type;
    return jsonObj;
  }
};

/**
 * Can store any package variant
 *
 * Internally stores packages as a JsonObject. Main use case is to convert
 * different packages from and to Json (using ArduinoJson).
 */
class Variant {
 public:
#ifdef ARDUINOJSON_ENABLE_STD_STRING
  /**
   * Create Variant object from a json string
   *
   * @param json The json string containing a package
   */
  Variant(std::string json)
      : jsonBuffer(JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(4) +
                   2 * json.length()) {
    error = deserializeJson(jsonBuffer, json,
                            DeserializationOption::NestingLimit(255));
    if (!error) jsonObj = jsonBuffer.as<JsonObject>();
  }

  /**
   * Create Variant object from a json string
   *
   * @param json The json string containing a package
   * @param capacity The capacity to reserve for parsing the string
   */
  Variant(std::string json, size_t capacity) : jsonBuffer(capacity) {
    error = deserializeJson(jsonBuffer, json,
                            DeserializationOption::NestingLimit(255));
    if (!error) jsonObj = jsonBuffer.as<JsonObject>();
  }
#endif

#ifdef ARDUINOJSON_ENABLE_ARDUINO_STRING
  /**
   * Create Variant object from a json string
   *
   * @param json The json string containing a package
   */
  Variant(String json)
      : jsonBuffer(JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(4) +
                   2 * json.length()) {
    error = deserializeJson(jsonBuffer, json,
                            DeserializationOption::NestingLimit(255));
    if (!error) jsonObj = jsonBuffer.as<JsonObject>();
  }

  /**
   * Create Variant object from a json string
   *
   * @param json The json string containing a package
   * @param capacity The capacity to reserve for parsing the string
   */
  Variant(String json, size_t capacity) : jsonBuffer(capacity) {
    error = deserializeJson(jsonBuffer, json,
                            DeserializationOption::NestingLimit(255));
    if (!error) jsonObj = jsonBuffer.as<JsonObject>();
  }
#endif
  /**
   * Create Variant object from any package implementing PackageInterface
   */
  Variant(const PackageInterface* pkg) : jsonBuffer(pkg->jsonObjectSize()) {
    jsonObj = jsonBuffer.to<JsonObject>();
    jsonObj = pkg->addTo(std::move(jsonObj));
  }

  /**
   * Create Variant object from a Single package
   *
   * @param single The single package
   */
  Variant(Single single) : jsonBuffer(single.jsonObjectSize()) {
    jsonObj = jsonBuffer.to<JsonObject>();
    jsonObj = single.addTo(std::move(jsonObj));
  }

  /**
   * Create Variant object from a Broadcast package
   *
   * @param broadcast The broadcast package
   */
  Variant(Broadcast broadcast) : jsonBuffer(broadcast.jsonObjectSize()) {
    jsonObj = jsonBuffer.to<JsonObject>();
    jsonObj = broadcast.addTo(std::move(jsonObj));
  }

  /**
   * Create Variant object from a NodeTree
   *
   * @param nodeTree The NodeTree
   */
  Variant(NodeTree nodeTree) : jsonBuffer(nodeTree.jsonObjectSize()) {
    jsonObj = jsonBuffer.to<JsonObject>();
    jsonObj = nodeTree.addTo(std::move(jsonObj));
  }

  /**
   * Create Variant object from a NodeSyncReply package
   *
   * @param nodeSyncReply The nodeSyncReply package
   */
  Variant(NodeSyncReply nodeSyncReply)
      : jsonBuffer(nodeSyncReply.jsonObjectSize()) {
    jsonObj = jsonBuffer.to<JsonObject>();
    jsonObj = nodeSyncReply.addTo(std::move(jsonObj));
  }

  /**
   * Create Variant object from a NodeSyncRequest package
   *
   * @param nodeSyncRequest The nodeSyncRequest package
   */
  Variant(NodeSyncRequest nodeSyncRequest)
      : jsonBuffer(nodeSyncRequest.jsonObjectSize()) {
    jsonObj = jsonBuffer.to<JsonObject>();
    jsonObj = nodeSyncRequest.addTo(std::move(jsonObj));
  }

  /**
   * Create Variant object from a TimeSync package
   *
   * @param timeSync The timeSync package
   */
  Variant(TimeSync timeSync) : jsonBuffer(timeSync.jsonObjectSize()) {
    jsonObj = jsonBuffer.to<JsonObject>();
    jsonObj = timeSync.addTo(std::move(jsonObj));
  }

  /**
   * Create Variant object from a TimeDelay package
   *
   * @param timeDelay The timeDelay package
   */
  Variant(TimeDelay timeDelay) : jsonBuffer(timeDelay.jsonObjectSize()) {
    jsonObj = jsonBuffer.to<JsonObject>();
    jsonObj = timeDelay.addTo(std::move(jsonObj));
  }

  /**
   * Whether this package is of the given type
   */
  template <typename T>
  inline bool is() {
    return false;
  }

  /**
   * Convert Variant to the given type
   */
  template <typename T>
  inline T to() {
    return T(jsonObj);
  }

  /**
   * Return package type
   */
  int type() { return jsonObj["type"].as<int>(); }

  /**
   * Package routing method
   */
  router::Type routing() {
    if (jsonObj.containsKey("routing"))
      return (router::Type)jsonObj["routing"].as<int>();

    auto type = this->type();
    if (type == SINGLE || type == TIME_DELAY) return router::SINGLE;
    if (type == BROADCAST) return router::BROADCAST;
    if (type == NODE_SYNC_REQUEST || type == NODE_SYNC_REPLY ||
        type == TIME_SYNC)
      return router::NEIGHBOUR;
    return router::ROUTING_ERROR;
  }

  /**
   * Destination node of the package
   */
  uint32_t dest() {
    if (jsonObj.containsKey("dest")) return jsonObj["dest"].as<uint32_t>();
    return 0;
  }

#ifdef ARDUINOJSON_ENABLE_STD_STRING
  /**
   * Print a variant to a string
   *
   * @return A json representation of the string
   */
  void printTo(std::string& str, bool pretty = false) {
    if (pretty)
      serializeJsonPretty(jsonObj, str);
    else
      serializeJson(jsonObj, str);
  }
#endif

#ifdef ARDUINOJSON_ENABLE_ARDUINO_STRING
  /**
   * Print a variant to a string
   *
   * @return A json representation of the string
   */
  void printTo(String& str, bool pretty = false) {
    if (pretty)
      serializeJsonPretty(jsonObj, str);
    else
      serializeJson(jsonObj, str);
  }
#endif

  DeserializationError error = DeserializationError::Ok;

 private:
  DynamicJsonDocument jsonBuffer;
  JsonObject jsonObj;
};

template <>
inline bool Variant::is<Single>() {
  return jsonObj["type"].as<int>() == SINGLE;
}

template <>
inline bool Variant::is<Broadcast>() {
  return jsonObj["type"].as<int>() == BROADCAST;
}

template <>
inline bool Variant::is<NodeSyncReply>() {
  return jsonObj["type"].as<int>() == NODE_SYNC_REPLY;
}

template <>
inline bool Variant::is<NodeSyncRequest>() {
  return jsonObj["type"].as<int>() == NODE_SYNC_REQUEST;
}

template <>
inline bool Variant::is<TimeSync>() {
  return jsonObj["type"].as<int>() == TIME_SYNC;
}

template <>
inline bool Variant::is<TimeDelay>() {
  return jsonObj["type"].as<int>() == TIME_DELAY;
}

template <>
inline JsonObject Variant::to<JsonObject>() {
  return jsonObj;
}

inline TSTRING NodeTree::toString(bool pretty) {
  TSTRING str;
  auto variant = Variant(*this);
  variant.printTo(str, pretty);
  return str;
}

}  // namespace protocol
}  // namespace painlessmesh
#endif
