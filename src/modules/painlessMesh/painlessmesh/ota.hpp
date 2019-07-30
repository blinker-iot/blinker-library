#ifndef _PAINLESS_MESH_PLUGIN_OTA_HPP_
#define _PAINLESS_MESH_PLUGIN_OTA_HPP_

#include "configuration.hpp"

#include "base64.hpp"
#include "logger.hpp"
#include "plugin.hpp"

#if defined(ESP32) || defined(ESP8266)
#ifdef ESP32
#include <SPIFFS.h>
#include <Update.h>
#else
#include <FS.h>
#endif
#endif

namespace painlessmesh {
namespace plugin {

/** OTA over the mesh
 *
 * OTA is implemented as a painlessmesh::plugin.
 *
 * The protocol consists of three message types: ota::Announce, ota::DataRequest
 * and ota::Data. The first message is generally send by the node that
 * distributes the firmware and announces the current version of firmware
 * available for each hardware and role. Firmware version is determined by
 * its MD5 signature. See
 * [painlessMeshBoost](http://gitlab.com/painlessMesh/painlessMeshBoost) for a
 * possible implementation of a distribution node.
 *
 * Once a node receives a announce message it will check it against its own role
 * and hardware to discover if it is suitable this node. If that checks out and
 * the MD5 is different than its own MD5 it will send a data request back to the
 * firmware distribution node. This request also includes a partNo, to determine
 * which part of the data it needs (starting from zero).
 *
 * When the distribution node receives a data request, it sends the data back to
 * the node (with a data message). The node will then write this data and
 * request the next part of the data. This exchange continuous until the node
 * has all the data, written it and reboots into the new firmware.
 */
namespace ota {

/** Package used by the firmware distribution node to announce new version
 * available
 *
 * This is based on the general BroadcastPackage to ensure it is being
 * broadcasted. It is possible to define a Announce::role, which defines the
 * node role the firmware is meant for.
 *
 * The package type/identifier is set to 10.
 */
class Announce : public BroadcastPackage {
 public:
  TSTRING md5;
  TSTRING hardware;

  /**
   * \brief The type of node the firmware is meant for
   *
   * Nodes can fulfill different roles, which require specific firmware. E.g a
   * node can be a sensor and therefore needs the firmware meant for sensor
   * nodes. This allows one to set the type of node (role) this firmware is
   * aimed at.
   *
   * Note that the role should not contain underscores or dots.
   */
  TSTRING role;

  /** Force an update even if the node already has this firmware version
   *
   * Mainly usefull when testing updates etc.
   */
  bool forced = false;
  size_t noPart;

  Announce() : BroadcastPackage(10) {}

  Announce(JsonObject jsonObj) : BroadcastPackage(jsonObj) {
    md5 = jsonObj["md5"].as<TSTRING>();
    hardware = jsonObj["hardware"].as<TSTRING>();
    role = jsonObj["role"].as<TSTRING>();
    if (jsonObj.containsKey("forced")) forced = jsonObj["forced"];
    noPart = jsonObj["noPart"];
  }

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj = BroadcastPackage::addTo(std::move(jsonObj));
    jsonObj["md5"] = md5;
    jsonObj["hardware"] = hardware;
    jsonObj["role"] = role;
    if (forced) jsonObj["forced"] = forced;
    jsonObj["noPart"] = noPart;
    return jsonObj;
  }

  size_t jsonObjectSize() const {
    return JSON_OBJECT_SIZE(noJsonFields + 5) +
           round(1.1 * (md5.length() + hardware.length() + role.length()));
  }

 protected:
  Announce(int type, router::Type routing) : BroadcastPackage(type) {
    this->routing = routing;
  }
};

class Data;

/** Request (part of) the firmware update
 *
 * This is send by the node needing the new firmware, to the firmware
 * distribution node to request a part (DataRequest::partNo) of the data.
 *
 * The package type/identifier is set to 11.
 */
class DataRequest : public Announce {
 public:
  size_t partNo = 0;
  uint32_t dest = 0;

  DataRequest() : Announce(11, router::SINGLE) {}

  DataRequest(JsonObject jsonObj) : Announce(jsonObj) {
    dest = jsonObj["dest"];
    partNo = jsonObj["partNo"];
  }

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj = Announce::addTo(std::move(jsonObj));
    jsonObj["dest"] = dest;
    jsonObj["partNo"] = partNo;
    return jsonObj;
  }

  static DataRequest replyTo(const Announce& ann, uint32_t from,
                             size_t partNo) {
    DataRequest req;
    req.dest = ann.from;
    req.md5 = ann.md5;
    req.hardware = ann.hardware;
    req.role = ann.role;
    req.forced = ann.forced;
    req.noPart = ann.noPart;
    req.partNo = partNo;
    req.from = from;
    return req;
  }

  static DataRequest replyTo(const Data& d, size_t partNo);

  size_t jsonObjectSize() const {
    return JSON_OBJECT_SIZE(noJsonFields + 5 + 2) +
           round(1.1 * (md5.length() + hardware.length() + role.length()));
  }

 protected:
  DataRequest(int type) : Announce(type, router::SINGLE) {}
};

/** Package containing part of the firmware
 *
 * The package type/identifier is set to 12.
 */
class Data : public DataRequest {
 public:
  TSTRING data;

  Data() : DataRequest(12) {}

  Data(JsonObject jsonObj) : DataRequest(jsonObj) {
    data = jsonObj["data"].as<TSTRING>();
  }

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj = DataRequest::addTo(std::move(jsonObj));
    jsonObj["data"] = data;
    return jsonObj;
  }

  static Data replyTo(const DataRequest& req, TSTRING data, size_t partNo) {
    Data d;
    d.from = req.dest;
    d.dest = req.from;
    d.md5 = req.md5;
    d.hardware = req.hardware;
    d.role = req.role;
    d.forced = req.forced;
    d.noPart = req.noPart;
    d.partNo = partNo;
    d.data = data;
    return d;
  }

  size_t jsonObjectSize() const {
    return JSON_OBJECT_SIZE(noJsonFields + 5 + 2 + 1) +
           round(1.1 * (md5.length() + hardware.length() + role.length() +
                        data.length()));
  }
};

inline DataRequest DataRequest::replyTo(const Data& d, size_t partNo) {
  DataRequest req;
  req.from = d.dest;
  req.dest = d.from;
  req.md5 = d.md5;
  req.hardware = d.hardware;
  req.role = d.role;
  req.forced = d.forced;
  req.noPart = d.noPart;
  req.partNo = partNo;
  return req;
}

/** Data related to the current state of the node update
 *
 * This class is used by the OTA algorithm to keep track of both the current
 * version of the software and the ongoing update.
 *
 * The firmware md5 uniquely identifies each firmware version
 */
class State : public protocol::PackageInterface {
 public:
  TSTRING md5;
#ifdef ESP32
  TSTRING hardware = "ESP32";
#else
  TSTRING hardware = "ESP8266";
#endif
  TSTRING role;
  size_t noPart = 0;
  size_t partNo = 0;
  TSTRING ota_fn = "/ota_fw.json";

  State() {}

  State(JsonObject jsonObj) {
    md5 = jsonObj["md5"].as<TSTRING>();
    hardware = jsonObj["hardware"].as<TSTRING>();
    role = jsonObj["role"].as<TSTRING>();
  }

  State(const Announce& ann) {
    md5 = ann.md5;
    hardware = ann.hardware;
    role = ann.role;
    noPart = ann.noPart;
  }

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj["role"] = role;
    jsonObj["md5"] = md5;
    jsonObj["hardware"] = hardware;
    return jsonObj;
  }

  size_t jsonObjectSize() const {
    return JSON_OBJECT_SIZE(3) +
           round(1.1 * (md5.length() + hardware.length() + role.length()));
  }

  std::shared_ptr<Task> task;
};

template <class T>
void addPackageCallback(Scheduler& scheduler, plugin::PackageHandler<T>& mesh,
                        TSTRING role = "") {
  using namespace logger;
#if defined(ESP32) || defined(ESP8266)
  auto currentFW = std::make_shared<State>();
  currentFW->role = role;
  auto updateFW = std::make_shared<State>();
  updateFW->role = role;
#ifdef ESP32
  SPIFFS.begin(true);  // Start the SPI Flash Files System
#else
  SPIFFS.begin();  // Start the SPI Flash Files System
#endif
  if (SPIFFS.exists(currentFW->ota_fn)) {
    auto file = SPIFFS.open(currentFW->ota_fn, "r");
    TSTRING msg = "";
    while (file.available()) {
      msg += (char)file.read();
    }
    auto var = protocol::Variant(msg);
    auto fw = var.to<State>();
    if (fw.role == role && fw.hardware == currentFW->hardware) {
      Log(DEBUG, "MD5 found %s\n", fw.md5.c_str());
      currentFW->md5 = fw.md5;
    }
  }

  mesh.onPackage(10, [currentFW, updateFW, &mesh,
                      &scheduler](protocol::Variant variant) {
    // convert variant to Announce
    auto pkg = variant.to<Announce>();
    // Check if we want the update
    if (currentFW->role == pkg.role && currentFW->hardware == pkg.hardware) {
      if ((currentFW->md5 == pkg.md5 && !pkg.forced) ||
          updateFW->md5 == pkg.md5)
        // Either already have it, or already updating to it
        return false;
      else {
        auto request = DataRequest::replyTo(pkg, mesh.getNodeId(), updateFW->partNo);
        updateFW->md5 = pkg.md5;
        // enable the request task
        updateFW->task =
            mesh.addTask(scheduler, 30 * TASK_SECOND, 10,
                         [request, &mesh]() { mesh.sendPackage(&request); });
        updateFW->task->setOnDisable([updateFW]() {
          Log(ERROR, "OTA: Did not receive the requested data.\n");
          updateFW->md5 = "";
        });
      }
    }
    return false;
  });

  mesh.onPackage(11, [currentFW](protocol::Variant variant) {
    Log(ERROR, "Data request should not be send to this node\n");
    return false;
  });

  mesh.onPackage(12, [currentFW, updateFW, &mesh,
                      &scheduler](protocol::Variant variant) {
    auto pkg = variant.to<Data>();
    // Check whether it is a new part, of correct md5 role etc etc
    if (updateFW->partNo == pkg.partNo && updateFW->md5 == pkg.md5 &&
        updateFW->role == pkg.role && updateFW->hardware == pkg.hardware) {
      // If so write
      if (pkg.partNo == 0) {
#ifdef ESP32
        uint32_t maxSketchSpace = UPDATE_SIZE_UNKNOWN;
#else
        uint32_t maxSketchSpace =
                (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
#endif
        Log(DEBUG, "Sketch size %d\n", maxSketchSpace);
        if (Update.isRunning()) {
          Update.end(false);
        }
        if (!Update.begin(maxSketchSpace)) {  // start with max available size
          Log(DEBUG, "handleOTA(): OTA start failed!");
          Update.printError(Serial);
          Update.end();
        } else {
          Update.setMD5(pkg.md5.c_str());
        }
      }

      //    write data
      auto b64Data = base64::decode(pkg.data);
      if (Update.write((uint8_t*)b64Data.c_str(), b64Data.length()) !=
          b64Data.length()) {
        Log(ERROR, "handleOTA(): OTA write failed!");
        Update.printError(Serial);
        Update.end();
        updateFW->md5 = "";
        updateFW->partNo = 0;
        return false;
      }

      // If last part then write ota_fn and reboot
      if (pkg.partNo == pkg.noPart - 1) {
        //       check md5, reboot
        if (Update.end(true)) {  // true to set the size to the
                                 // current progress
          auto file = SPIFFS.open(updateFW->ota_fn, "w");
          String msg;
          auto var = protocol::Variant(updateFW.get());
          var.printTo(msg);
          file.print(msg);
          file.close();

          Log(DEBUG, "handleOTA(): OTA Success! %s, %s\n", msg.c_str(),
              updateFW->role.c_str());
          ESP.restart();
        } else {
          Log(DEBUG, "handleOTA(): OTA failed!\n");
          Update.printError(Serial);
          updateFW->md5 = "";
          updateFW->partNo = 0;
        }
        updateFW->task->setOnDisable(NULL);
        updateFW->task->disable();
      } else {
        // else request more
        ++updateFW->partNo;
        auto request = DataRequest::replyTo(pkg, updateFW->partNo);
        updateFW->task->setCallback(
            [request, &mesh]() { mesh.sendPackage(&request); });
        updateFW->task->disable();
        updateFW->task->restart();
      }
    }
    return false;
  });
#endif
}

}  // namespace ota
}  // namespace plugin
}  // namespace painlessmesh

#endif

