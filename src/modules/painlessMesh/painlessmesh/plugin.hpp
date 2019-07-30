#ifndef _PAINLESS_MESH_PLUGIN_HPP_
#define _PAINLESS_MESH_PLUGIN_HPP_

#include <Arduino.h>
#include "configuration.hpp"

#include "router.hpp"

namespace painlessmesh {

/** Plugin interface for painlessMesh packages/messages
 *
 * This interface allows one to design their own messages types/packages, and
 * add handlers that are called when the new package type arrives at a node.
 * Here you can think of things like sensor packages, which hold the
 * measurements done by the sensors. The packages related to OTA updates are
 * also implemented as a plugin system (see plugin::ota). Each package type is
 * uniquely identified using the protocol::PackageInterface::type. Currently
 * default package types use numbers up to 12, so to be on the safe side we
 * recommend your own packages to use higher type values, e.g. start counting at
 * 20 at the lowest.
 *
 * An important piece of information is how a package should be routed.
 * Currently we have three main routing algorithms (router::Type).
 *
 * \code
 * using namespace painlessmesh;
 *
 * // Inherit from SinglePackage, the most basic package with
 * router::Type::SINGLE class SensorPackage : public plugin::SinglePackage {
 *
 * };
 *
 * \endcode
 */
namespace plugin {

class SinglePackage : public protocol::PackageInterface {
 public:
  uint32_t from;
  uint32_t dest;
  router::Type routing;
  int type;
  int noJsonFields = 4;

  SinglePackage(int type) : routing(router::SINGLE), type(type) {}

  SinglePackage(JsonObject jsonObj) {
    from = jsonObj["from"];
    dest = jsonObj["dest"];
    type = jsonObj["type"];
    routing = static_cast<router::Type>(jsonObj["routing"].as<int>());
  }

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj["from"] = from;
    jsonObj["dest"] = dest;
    jsonObj["routing"] = static_cast<int>(routing);
    jsonObj["type"] = type;
    return jsonObj;
  }
};

class BroadcastPackage : public protocol::PackageInterface {
 public:
  uint32_t from;
  router::Type routing;
  int type;
  int noJsonFields = 3;

  BroadcastPackage(int type) : routing(router::BROADCAST), type(type) {}

  BroadcastPackage(JsonObject jsonObj) {
    from = jsonObj["from"];
    type = jsonObj["type"];
    routing = static_cast<router::Type>(jsonObj["routing"].as<int>());
  }

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj["from"] = from;
    jsonObj["routing"] = static_cast<int>(routing);
    jsonObj["type"] = type;
    return jsonObj;
  }
};

class NeighbourPackage : public plugin::SinglePackage {
 public:
  NeighbourPackage(int type) : SinglePackage(type) {
    routing = router::NEIGHBOUR;
  }

  NeighbourPackage(JsonObject jsonObj) : SinglePackage(jsonObj) {}
};

/**
 * Handle different plugins
 *
 * Responsible for
 * - having a list of plugin types
 * - the functions defined to handle the different plugin types
 * - tasks?
 */
template <typename T>
class PackageHandler : public layout::Layout<T> {
 public:
  void stop() {
    for (auto&& task : taskList) {
      task->disable();
      task->setCallback(NULL);
    }
    taskList.clear();
  }

  ~PackageHandler() {
    if (taskList.size() > 0)
      Log(logger::ERROR,
          "~PackageHandler(): Always call PackageHandler::stop(scheduler) "
          "before calling this destructor");
  }

  bool sendPackage(const protocol::PackageInterface* pkg) {
    auto variant = protocol::Variant(pkg);
    // if single or neighbour with direction
    if (variant.routing() == router::SINGLE ||
        (variant.routing() == router::NEIGHBOUR && variant.dest() != 0)) {
      return router::send(variant, (*this));
    }

    // if broadcast or neighbour without direction
    if (variant.routing() == router::BROADCAST ||
        (variant.routing() == router::NEIGHBOUR && variant.dest() == 0)) {
      auto i = router::broadcast(variant, (*this), 0);
      if (i > 0) return true;
      return false;
    }
    return false;
  }

  void onPackage(int type, std::function<bool(protocol::Variant)> function) {
    auto func = [function](protocol::Variant var, std::shared_ptr<T>,
                           uint32_t) { return function(var); };
    this->callbackList.onPackage(type, func);
  }

  /**
   * Add a task to the scheduler
   *
   * The task will be stored in a list and a shared_ptr to the task will be
   * returned. If the task is anonymous (i.e. no shared_ptr to it is held
   * anywhere else) and disabled then it will be reused when a new task is
   * added.
   */
  std::shared_ptr<Task> addTask(Scheduler& scheduler, unsigned long aInterval,
                                long aIterations,
                                std::function<void()> aCallback) {
    using namespace painlessmesh::logger;
    for (auto&& task : taskList) {
      if (task.use_count() == 1 && !task->isEnabled()) {
        task->set(aInterval, aIterations, aCallback, NULL, NULL);
        task->enable();
        return task;
      }
    }

    std::shared_ptr<Task> task =
        std::make_shared<Task>(aInterval, aIterations, aCallback);
    scheduler.addTask((*task));
    task->enable();
    taskList.push_front(task);
    return task;
  }

  std::shared_ptr<Task> addTask(Scheduler& scheduler,
                                std::function<void()> aCallback) {
    return this->addTask(scheduler, 0, TASK_ONCE, aCallback);
  }

 protected:
  router::MeshCallbackList<T> callbackList;
  std::list<std::shared_ptr<Task> > taskList = {};
};

}  // namespace plugin
}  // namespace painlessmesh
#endif

