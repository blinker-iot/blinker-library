#include "painlessMesh.h"

#include "time.h"

extern painlessMesh* staticThis;
extern LogClass Log;
uint32_t timeAdjuster = 0;

// timeSync Functions
//***********************************************************************
uint32_t ICACHE_FLASH_ATTR painlessMesh::getNodeTime(void) {
    auto base_time = micros();
    uint32_t ret = base_time + timeAdjuster;
    Log(GENERAL, "getNodeTime(): time=%u\n", ret);
    return ret;
}

//***********************************************************************
void ICACHE_FLASH_ATTR
painlessMesh::handleNodeSync(std::shared_ptr<MeshConnection> conn,
                             painlessmesh::protocol::NodeTree newTree) {
  using namespace painlessmesh;
  Log(SYNC, "handleNodeSync(): with %u\n", conn->nodeId);

  if (!conn->validSubs(newTree)) {
    Log(SYNC, "handleNodeSync(): invalid new connection\n");
    conn->close();
    return;
  }

  if (conn->newConnection) {
    auto oldConnection =
        layout::findRoute<MeshConnection>((*this), newTree.nodeId);
    if (oldConnection) {
      Log(SYNC,
          "handleNodeSync(): already connected to %u. Closing the new "
          "connection \n",
          conn->nodeId);
      conn->close();
      return;
    }

    // TODO: Move this to its own function
    newConnectionTask.set(
        TASK_SECOND, TASK_ONCE, [remoteNodeId = newTree.nodeId]() {
          Log(CONNECTION, "newConnectionTask():\n");
          Log(CONNECTION, "newConnectionTask(): adding %u now= %u\n",
              remoteNodeId, staticThis->getNodeTime());
          if (staticThis->newConnectionCallback)
            staticThis->newConnectionCallback(
                remoteNodeId);  // Connection dropped. Signal user
        });

    _scheduler.addTask(newConnectionTask);
    newConnectionTask.enable();

    // Initially interval is every 10 seconds,
    // this will slow down to TIME_SYNC_INTERVAL
    // after first succesfull sync
    conn->timeSyncTask.set(10 * TASK_SECOND, TASK_FOREVER, [conn]() {
      Log(S_TIME, "timeSyncTask(): %u\n", conn->nodeId);
      staticThis->startTimeSync(conn);
    });
    _scheduler.addTask(conn->timeSyncTask);
    if (conn->station)
      // We are STA, request time immediately
      conn->timeSyncTask.enable();
    else
      // We are the AP, give STA the change to initiate time sync
      conn->timeSyncTask.enableDelayed();
    conn->newConnection = false;
  }

  if (conn->updateSubs(newTree)) {
    if (changedConnectionsCallback) changedConnectionsCallback();
    staticThis->syncSubConnections(conn->nodeId);
  } else {
    conn->nodeSyncTask.delay();
    stability += min(1000 - stability, (size_t)25);
  }
}

void ICACHE_FLASH_ATTR
painlessMesh::startTimeSync(std::shared_ptr<MeshConnection> conn) {
  Log(S_TIME, "startTimeSync(): with %u, local port: %d\n", conn->nodeId,
      conn->client->getLocalPort());
  auto adopt = adoptionCalc(conn);
  painlessmesh::protocol::TimeSync timeSync;
  if (adopt) {
    timeSync =
        painlessmesh::protocol::TimeSync(nodeId, conn->nodeId, getNodeTime());
    Log(S_TIME, "startTimeSync(): Requesting %u to adopt our time\n",
        conn->nodeId);
  } else {
    timeSync = painlessmesh::protocol::TimeSync(nodeId, conn->nodeId);
    Log(S_TIME, "startTimeSync(): Requesting time from %u\n", conn->nodeId);
  }
  send<painlessmesh::protocol::TimeSync>(conn, timeSync, true);
}

//***********************************************************************
bool ICACHE_FLASH_ATTR painlessMesh::adoptionCalc(std::shared_ptr<MeshConnection> conn) {
  using namespace painlessmesh;
  if (conn == NULL)  // Missing connection
    return false;
  // make the adoption calulation. Figure out how many nodes I am connected to
  // exclusive of this connection.

  // We use length as an indicator for how many subconnections both nodes have
  uint16_t mySubCount = layout::size(layout::excludeRoute(
      this->asNodeTree(), conn->nodeId));           // exclude this connection.
  uint16_t remoteSubCount = layout::size((*conn));  // exclude this connection.
  bool ap = conn->client->getLocalPort() == _meshPort;

  // ToDo. Simplify this logic
  bool ret = (mySubCount > remoteSubCount) ? false : true;
  if (mySubCount == remoteSubCount && ap) {  // in case of draw, ap wins
    ret = false;
    }

    Log(S_TIME,
        "adoptionCalc(): mySubCount=%d remoteSubCount=%d role=%s adopt=%s\n",
        mySubCount, remoteSubCount, ap ? "AP" : "STA", ret ? "true" : "false");

    return ret;
}

//***********************************************************************
void ICACHE_FLASH_ATTR painlessMesh::handleTimeSync(
    std::shared_ptr<MeshConnection> conn,
    painlessmesh::protocol::TimeSync timeSync, uint32_t receivedAt) {
  switch (timeSync.msg.type) {
    case (painlessmesh::protocol::TIME_SYNC_ERROR):
      Log(ERROR,
          "handleTimeSync(): Received time sync error. Restarting time "
          "sync.\n");
      conn->timeSyncTask.forceNextIteration();
      break;
    case (painlessmesh::protocol::TIME_SYNC_REQUEST):  // Other party request me
                                                       // to ask it for time
      Log(S_TIME,
          "handleTimeSync(): Received requesto to start TimeSync with "
          "node: %u\n",
          conn->nodeId);
      timeSync.reply(getNodeTime());
      staticThis->send<painlessmesh::protocol::TimeSync>(conn, timeSync, true);
      break;

    case (painlessmesh::protocol::TIME_REQUEST):
      timeSync.reply(receivedAt, getNodeTime());
      staticThis->send<painlessmesh::protocol::TimeSync>(conn, timeSync, true);

      Log(S_TIME, "handleTimeSync(): timeSyncStatus with %u completed\n",
          conn->nodeId);

      // After response is sent I assume sync is completed
      conn->timeSyncTask.delay(TIME_SYNC_INTERVAL);
      break;

    case (painlessmesh::protocol::TIME_REPLY): {
      Log(S_TIME, "handleTimeSync(): TIME RESPONSE received.\n");
      int32_t offset = painlessmesh::ntp::clockOffset(
          timeSync.msg.t0, timeSync.msg.t1, timeSync.msg.t2, receivedAt);
      timeAdjuster += offset;  // Accumulate offset

      // flag all connections for re-timeSync
      if (nodeTimeAdjustedCallback) {
        nodeTimeAdjustedCallback(offset);
      }

      if (offset < TIME_SYNC_ACCURACY && offset > -TIME_SYNC_ACCURACY) {
        // mark complete only if offset was less than 10 ms
        conn->timeSyncTask.delay(TIME_SYNC_INTERVAL);
        Log(S_TIME, "handleTimeSync(): timeSyncStatus with %u completed\n",
            conn->nodeId);

        // Time has changed, update other nodes
        for (auto&& connection : this->subs) {
          if (connection->nodeId != conn->nodeId) {  // exclude this connection
            connection->timeSyncTask.forceNextIteration();
            Log(S_TIME,
                "handleTimeSync(): timeSyncStatus with %u brought forward\n",
                connection->nodeId);
          }
        }
      } else {
        // Iterate sync procedure if accuracy was not enough
        conn->timeSyncTask.delay(200 * TASK_MILLISECOND);  // Small delay
        Log(S_TIME,
            "handleTimeSync(): timeSyncStatus with %u needs further tries\n",
            conn->nodeId);
      }
      break;
    }
    default:
      Log(ERROR, "handleTimeSync(): unkown type %u, %u\n", timeSync.msg.type,
          painlessmesh::protocol::TIME_SYNC_REQUEST);
      break;
  }

  Log(S_TIME, "handleTimeSync(): ----------------------------------\n");
}

void ICACHE_FLASH_ATTR painlessMesh::handleTimeDelay(
    std::shared_ptr<MeshConnection> conn,
    painlessmesh::protocol::TimeDelay timeDelay, uint32_t receivedAt) {
  Log(S_TIME, "handleTimeDelay(): from %u in timestamp\n", timeDelay.from);

  switch (timeDelay.msg.type) {
    case (painlessmesh::protocol::TIME_SYNC_ERROR):
      Log(ERROR,
          "handleTimeDelay(): Error in requesting time delay. Please try "
          "again.\n");
      break;

    case (painlessmesh::protocol::TIME_REQUEST):
      // conn->timeSyncStatus == IN_PROGRESS;
      Log(S_TIME, "handleTimeDelay(): TIME REQUEST received.\n");

      // Build time response
      timeDelay.reply(receivedAt, getNodeTime());
      staticThis->send<painlessmesh::protocol::TimeDelay>(conn, timeDelay);
      break;

    case (painlessmesh::protocol::TIME_REPLY): {
      Log(S_TIME, "handleTimeDelay(): TIME RESPONSE received.\n");
      int32_t delay = painlessmesh::ntp::tripDelay(
          timeDelay.msg.t0, timeDelay.msg.t1, timeDelay.msg.t2, receivedAt);
      Log(S_TIME, "handleTimeDelay(): Delay is %d\n", delay);

      // conn->timeSyncStatus == COMPLETE;

      if (nodeDelayReceivedCallback)
        nodeDelayReceivedCallback(timeDelay.from, delay);
    } break;

    default:
      Log(ERROR,
          "handleTimeDelay(): Unknown timeSyncMessageType received. "
          "Ignoring for now.\n");
  }

  Log(S_TIME, "handleTimeSync(): ----------------------------------\n");
}

void ICACHE_FLASH_ATTR painlessMesh::syncSubConnections(uint32_t changedId) {
  Log(SYNC, "syncSubConnections(): changedId = %u\n", changedId);
  for (auto&& connection : this->subs) {
    if (connection->connected && !connection->newConnection &&
        connection->nodeId != 0 &&
        connection->nodeId != changedId) {  // Exclude current
      connection->nodeSyncTask.forceNextIteration();
    }
    }
    staticThis->stability /= 2;
}
