#ifndef _PAINLESS_MESH_TCP_HPP_
#define _PAINLESS_MESH_TCP_HPP_

#include <list>

#include <Arduino.h>
#include "configuration.hpp"

#include "logger.hpp"

namespace painlessmesh {
namespace tcp {
template <class T, class M>
void initServer(AsyncServer &server, M &mesh) {
  using namespace logger;
  server.setNoDelay(true);

  server.onClient(
      [&mesh](void *arg, AsyncClient *client) {
        if (mesh.semaphoreTake()) {
          Log(CONNECTION, "New AP connection incoming\n");
          auto conn = std::make_shared<T>(client, &mesh, false);
          conn->initTasks();
          conn->initTCPCallbacks();
          mesh.subs.push_back(conn);
          mesh.semaphoreGive();
        }
      },
      NULL);
  server.begin();
}

template <class T, class M>
void connect(AsyncClient &client, IPAddress ip, uint16_t port, M &mesh) {
  using namespace logger;
  client.onError([&mesh](void *, AsyncClient *client, int8_t err) {
    if (mesh.semaphoreTake()) {
      Log(CONNECTION, "tcp_err(): error trying to connect %d\n", err);
      if (client->connected()) client->close();
      // TODO: can we make this platform indepenedent?
      WiFi.disconnect();
      mesh.semaphoreGive();
    }
  });

  client.onConnect(
      [&mesh](void *, AsyncClient *client) {
        if (mesh.semaphoreTake()) {
          Log(CONNECTION, "New STA connection incoming\n");
          auto conn = std::make_shared<T>(client, &mesh, true);
          conn->initTasks();
          conn->initTCPCallbacks();
          mesh.subs.push_back(conn);
          mesh.semaphoreGive();
        }
      },
      NULL);

  client.connect(ip, port);
}
}  // namespace tcp
}  // namespace painlessmesh
#endif
