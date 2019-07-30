#ifndef _PAINLESS_MESH_PLUGIN_PERFORMANCE_HPP_
#define _PAINLESS_MESH_PLUGIN_PERFORMANCE_HPP_

#include "../painlessmesh/configuration.hpp"

#include "../painlessmesh/logger.hpp"
#include "../painlessmesh/plugin.hpp"

namespace painlessmesh {
namespace plugin {
/** Add performance tracking to the mesh
 *
 * Nodes will send out special packages to each other, allowing each node to
 * keep track of different performance measures
 */
namespace performance {

/** Track mean and variance of the given value
 *
 * Older values are discounted, so that you keep a rolling average
 */
class Stats {
 public:
  void update(double v, double alpha = 0.1) {
    if (!init) {
      mu = v;
      init = true;
      return;
    }
    auto d = v - mu;
    mu = mu + alpha * d;
    var = (1 - alpha) * (var + alpha * pow(d, 2));
  }

  // Returns the mean and 95% interval based on 1.96*sd? or 2.96*sd
  TSTRING toString() const {
#ifdef PAINLESSMESH_ENABLE_STD_STRING
    std::stringstream ss;
    ss << mu << "[" << mu - 1.96 * sqrt(var) << "," << mu + 1.96 * sqrt(var)
       << "]";
    return ss.str();
#else
    return TSTRING(mu) + TSTRING("[") + TSTRING(mu - 1.96 * sqrt(var)) +
           TSTRING(",") + TSTRING(mu + 1.96 * sqrt(var)) + TSTRING("]");
#endif
  }

 protected:
  double mu = 0;
  double var = 0;
  bool init = false;
};

class PerformancePackage : public plugin::BroadcastPackage {
 public:
  int id = 0;      // Can see if we missed values
  int time;        // Get an idea of the delay, by comparing it with nodetime
  int stability;   // stability of the sending node
  int freeMemory;  // memory of the sending node
#ifdef ESP32
  TSTRING hardware = "ESP32";
#else
  TSTRING hardware = "ESP8266";
#endif
  PerformancePackage() : plugin::BroadcastPackage(13) {}

  PerformancePackage(JsonObject jsonObj) : plugin::BroadcastPackage(jsonObj) {
    id = jsonObj["id"];
    time = jsonObj["time"];
    stability = jsonObj["stability"];
    freeMemory = jsonObj["freeMemory"];
    hardware = jsonObj["hardware"].as<TSTRING>();
  }

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj = BroadcastPackage::addTo(std::move(jsonObj));
    jsonObj["type"] = type;
    jsonObj["id"] = id;
    jsonObj["time"] = time;
    jsonObj["stability"] = stability;
    jsonObj["freeMemory"] = freeMemory;
    jsonObj["hardware"] = hardware;
    return jsonObj;
  }

  size_t jsonObjectSize() const {
    return JSON_OBJECT_SIZE(4 + 5) + round(2 * (hardware.length()));
  }
};

/// Numbers to track for each node we receive PerformancePackages from
class Track {
 public:
  uint32_t nodeId;
  TSTRING hardware;
  uint32_t hits = 0;
  uint32_t misses = 0;
  int lastId = 0;
  Stats delay;
  Stats stability;
  Stats freeMemory;
  uint32_t present = 0;  // Every so often check if each node is absent or
                         // present in the layout
  uint32_t absent = 0;
  Track() {}

  void addTo(JsonObject& jsonObj) const {
    jsonObj["nodeId"] = nodeId;
    jsonObj["hardware"] = hardware;
    jsonObj["hits"] = hits;
    jsonObj["misses"] = misses;
    jsonObj["delay"] = delay.toString();
    jsonObj["stability"] = stability.toString();
    jsonObj["freeMemory"] = freeMemory.toString();
    jsonObj["present"] = present;
    jsonObj["absent"] = absent;
  }
};

/// Holds resulst from all the nodes
class TrackMap : public protocol::PackageInterface,
                 public std::map<uint32_t, Track> {
 public:
  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj["event"] = "performance";
    // Start array
    auto jsonArr = jsonObj.createNestedArray("nodes");
    // for each in map do
    for (auto&& pair : (*this)) {
      auto obj = jsonArr.createNestedObject();
      pair.second.addTo(obj);
    }
    return jsonObj;
  }  // namespace performance

  size_t jsonObjectSize() const {
    return JSON_OBJECT_SIZE(2 + 15) + JSON_ARRAY_SIZE(this->size()) +
           this->size()*(JSON_OBJECT_SIZE(9) + 4 * 100);
  }
};  // namespace plugin

template <class T>
void begin(T& mesh, size_t frequency = 2) {
  auto tracker = std::make_shared<TrackMap>();
  auto sendPkg = std::make_shared<PerformancePackage>();

  mesh.onPackage(sendPkg->type, [&mesh, tracker](protocol::Variant var) {
    auto pkg = var.to<PerformancePackage>();
    // if not in tracker, add it
    if (!tracker->count(pkg.from)) {
      tracker->operator[](pkg.from) = Track();
      if (pkg.id > 0) tracker->operator[](pkg.from).lastId = pkg.id - 1;
    }
    // update all the values in the trackmap
    tracker->operator[](pkg.from).nodeId = pkg.from;
    tracker->operator[](pkg.from).hardware = pkg.hardware;
    ++tracker->operator[](pkg.from).hits;
    if (pkg.id < tracker->operator[](pkg.from).lastId)  // Node was reset?
      tracker->operator[](pkg.from).lastId = pkg.id;
    else {
      tracker->operator[](pkg.from).misses +=
          (pkg.id - tracker->operator[](pkg.from).lastId) - 1;
    }
    tracker->operator[](pkg.from).lastId = pkg.id;
    tracker->operator[](pkg.from).delay.update(
        ((int)mesh.getNodeTime() - pkg.time) / 1000);
    tracker->operator[](pkg.from).stability.update(pkg.stability);
    tracker->operator[](pkg.from).freeMemory.update(pkg.freeMemory);
    return false;
  });

  sendPkg->from = mesh.getNodeId();
  mesh.addTask(TASK_SECOND / frequency, TASK_FOREVER, [sendPkg, &mesh]() {
    ++sendPkg->id;
    sendPkg->time = mesh.getNodeTime();
    sendPkg->stability = mesh.stability;
    sendPkg->freeMemory = ESP.getFreeHeap();
    mesh.sendPackage(sendPkg.get());
  });

  mesh.addTask(TASK_MINUTE, TASK_FOREVER, [tracker, &mesh]() {
    for (auto&& pair : (*tracker)) {
      if (mesh.isConnected(pair.first))
        ++pair.second.present;
      else
        ++pair.second.absent;
    }
    protocol::Variant var(tracker.get());
    TSTRING str;
    var.printTo(str);
#ifdef PAINLESSMESH_ENABLE_STD_STRING
    std::cout << str << std::endl;
#else
    Serial.println(str);
#endif
  });
}

}  // namespace performance
}  // namespace plugin
}  // namespace painlessmesh
#endif
