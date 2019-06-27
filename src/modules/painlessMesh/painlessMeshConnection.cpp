//
//  painlessMeshConnection.cpp
//
//
//  Created by Bill Gray on 7/26/16.
//
//

#include "painlessMesh.h"

//#include "lwip/priv/tcpip_priv.h"

extern LogClass Log;
extern painlessMesh* staticThis;

static painlessmesh::buffer::temp_buffer_t shared_buffer;

void meshRecvCb(void * arg, AsyncClient *, void * data, size_t len);
void tcpSentCb(void * arg, AsyncClient * tpcb, size_t len, uint32_t time);

ICACHE_FLASH_ATTR MeshConnection::MeshConnection(AsyncClient *client_ptr, painlessMesh *pMesh, bool is_station) {
  using namespace painlessmesh;
  station = is_station;
  mesh = pMesh;
  client = client_ptr;

  client->setNoDelay(true);
  client->setRxTimeout(NODE_TIMEOUT / TASK_SECOND);

  // tcp_arg(pcb, static_cast<void*>(this));
  auto arg = static_cast<void *>(this);
  client->onData(meshRecvCb, arg);

  client->onAck(tcpSentCb, arg);

  if (station) {  // we are the station, start nodeSync
    Log(CONNECTION, "meshConnectedCb(): we are STA\n");
    } else {
      Log(CONNECTION, "meshConnectedCb(): we are AP\n");
    }

    client->onError([](void * arg, AsyncClient *client, int8_t err) {
        if (staticThis->semaphoreTake()) {
          // When AsyncTCP gets an error it will call both
          // onError and onDisconnect
          // so we handle this in the onDisconnect callback
          Log(CONNECTION, "tcp_err(): MeshConnection %s\n",
              client->errorToString(err));
          staticThis->semaphoreGive();
        }
    }, arg);

    client->onDisconnect([](void *arg, AsyncClient *client) {
        if (staticThis->semaphoreTake()) {
            if (arg == NULL) {
              Log(CONNECTION, "onDisconnect(): MeshConnection NULL\n");
              if (client->connected()) client->close(true);
              return;
            }
            auto conn = static_cast<MeshConnection*>(arg);
            Log(CONNECTION, "onDisconnect():\n");
            Log(CONNECTION, "onDisconnect(): dropping %u now= %u\n",
                conn->nodeId, staticThis->getNodeTime());
            conn->close();
            staticThis->semaphoreGive();
        }
    }, arg);

    auto syncInterval = NODE_TIMEOUT * 0.75;
    if (!station) syncInterval = syncInterval * 4;

    this->nodeSyncTask.set(syncInterval, TASK_FOREVER, [this]() {
      Log(SYNC, "nodeSyncTask(): request with %u\n", this->nodeId);
      // Need to find by client, because nodeId might not be set yet
      // TODO pass the shared_from_this route instead of finding it
      // using saveConn
      auto saveConn = layout::findRoute<MeshConnection>(
          (*staticThis),
          [client = this->client](std::shared_ptr<MeshConnection> s) {
            return (*s->client) == (*client);
          });
      staticThis->send<protocol::NodeSyncRequest>(
          saveConn, this->request(staticThis->asNodeTree()));
    });
    staticThis->_scheduler.addTask(this->nodeSyncTask);
    if (station)
        this->nodeSyncTask.enable();
    else
        this->nodeSyncTask.enableDelayed();

    receiveBuffer = painlessmesh::buffer::ReceiveBuffer<String>();
    readBufferTask.set(100*TASK_MILLISECOND, TASK_FOREVER, [this]() {
        if (!this->receiveBuffer.empty()) {
            String frnt = this->receiveBuffer.front();
            this->receiveBuffer.pop_front();
            if (!this->receiveBuffer.empty())
                this->readBufferTask.forceNextIteration();
            // handleMessage can invalidate this, (when closing connection)
            // so this should be the final action in this function
            this->handleMessage(frnt, staticThis->getNodeTime());
        }
    });
    staticThis->_scheduler.addTask(readBufferTask);
    readBufferTask.enableDelayed();

    sentBufferTask.set(500 * TASK_MILLISECOND, TASK_FOREVER, [this]() {
      Log(GENERAL, "sentBufferTask()\n");
      if (!this->sentBuffer.empty() && this->client->canSend()) {
        this->writeNext();
        this->sentBufferTask.forceNextIteration();
      }
    });
    staticThis->_scheduler.addTask(sentBufferTask);
    sentBufferTask.enableDelayed();

    Log(GENERAL, "MeshConnection(): leaving\n");
}

ICACHE_FLASH_ATTR MeshConnection::~MeshConnection() {
  Log(CONNECTION, "~MeshConnection():\n");
  this->close();
  if (!client->freeable()) {
    Log(CONNECTION, "~MeshConnection(): Closing pcb\n");
    client->close(true);
    }
    client->abort();
    delete client;
}

void ICACHE_FLASH_ATTR MeshConnection::close() {
    if (!connected)
        return;

    Log(CONNECTION, "MeshConnection::close().\n");
    this->connected = false;

    this->timeSyncTask.setCallback(NULL);
    this->nodeSyncTask.setCallback(NULL);
    this->readBufferTask.setCallback(NULL);
    this->sentBufferTask.setCallback(NULL);
    this->client->onDisconnect(NULL, NULL);
    this->client->onError(NULL, NULL);

    auto nodeId = this->nodeId;

    mesh->droppedConnectionTask.set(
        TASK_SECOND, TASK_ONCE, [nodeId]() {
          Log(CONNECTION, "closingTask():\n");
          Log(CONNECTION, "closingTask(): dropping %u now= %u\n", nodeId,
              staticThis->getNodeTime());
          if (staticThis->changedConnectionsCallback)
            staticThis->changedConnectionsCallback(); // Connection dropped. Signal user
          if (staticThis->droppedConnectionCallback)
            staticThis->droppedConnectionCallback(nodeId); // Connection dropped. Signal user
          staticThis->syncSubConnections(nodeId);
        });
    mesh->_scheduler.addTask(staticThis->droppedConnectionTask);
    mesh->droppedConnectionTask.enable();

    if (client->connected()) {
      Log(CONNECTION, "close(): Closing pcb\n");
      client->close();
    }

    if (station && WiFi.status() == WL_CONNECTED) {
      Log(CONNECTION, "close(): call WiFi.disconnect().\n");
      WiFi.disconnect();
    }

    receiveBuffer.clear();
    sentBuffer.clear();

    if (station && mesh->_station_got_ip)
        mesh->_station_got_ip = false;

    this->nodeId = 0;
    mesh->eraseClosedConnections();
    Log(CONNECTION, "MeshConnection::close() Done.\n");
}


bool ICACHE_FLASH_ATTR MeshConnection::addMessage(String &message, bool priority) {
    if (ESP.getFreeHeap() - message.length() >= MIN_FREE_MEMORY) { // If memory heap is enough, queue the message
        if (priority) {
            sentBuffer.push(message, priority);
            Log(COMMUNICATION,
                "addMessage(): Package sent to queue beginning -> %d , "
                "FreeMem: %d\n",
                sentBuffer.size(), ESP.getFreeHeap());
        } else {
          if (sentBuffer.size() < MAX_MESSAGE_QUEUE) {
            sentBuffer.push(message, priority);
            Log(COMMUNICATION,
                "addMessage(): Package sent to queue end -> %d , FreeMem: "
                "%d\n",
                sentBuffer.size(), ESP.getFreeHeap());
            } else {
              Log(ERROR,
                  "addMessage(): Message queue full -> %d , FreeMem: %d\n",
                  sentBuffer.size(), ESP.getFreeHeap());
              sentBufferTask.forceNextIteration();
              return false;
            }
        }
        sentBufferTask.forceNextIteration();
        return true;
    } else {
        //connection->sendQueue.clear(); // Discard all messages if free memory is low
        Log(DEBUG, "addMessage(): Memory low, message was discarded\n");
        sentBufferTask.forceNextIteration();
        return false;
    }
}

bool ICACHE_FLASH_ATTR MeshConnection::writeNext() {
    if (sentBuffer.empty()) {
      Log(COMMUNICATION, "writeNext(): sendQueue is empty\n");
      return false;
    }
    auto len = sentBuffer.requestLength(shared_buffer.length);
    auto snd_len = client->space();
    if (len > snd_len)
        len = snd_len;
    if (len > 0) {
      // sentBuffer.read(len, shared_buffer);
      // auto written = client->write(shared_buffer.buffer, len, 1);
      auto data_ptr = sentBuffer.readPtr(len);
      auto written = client->write(data_ptr, len, 1);
      if (written == len) {
        Log(COMMUNICATION, "writeNext(): Package sent = %s\n",
            shared_buffer.buffer);
        client->send();  // TODO only do this for priority messages
        sentBuffer.freeRead();
        sentBufferTask.forceNextIteration();
        return true;
        } else if (written == 0) {
          Log(COMMUNICATION,
              "writeNext(): tcp_write Failed node=%u. Resending later\n",
              nodeId);
          return false;
        } else {
          Log(ERROR,
              "writeNext(): Less written than requested. Please report bug on "
              "the issue tracker\n");
          return false;
        }
    } else {
      Log(COMMUNICATION, "writeNext(): tcp_sndbuf not enough space\n");
      return false;
    }

}

// connection managment functions
//***********************************************************************
void ICACHE_FLASH_ATTR painlessMesh::onReceive(receivedCallback_t  cb) {
  Log(GENERAL, "onReceive():\n");
  receivedCallback = cb;
}

//***********************************************************************
void ICACHE_FLASH_ATTR painlessMesh::onNewConnection(newConnectionCallback_t cb) {
  Log(GENERAL, "onNewConnection():\n");
  newConnectionCallback = cb;
}

void ICACHE_FLASH_ATTR painlessMesh::onDroppedConnection(droppedConnectionCallback_t cb) {
  Log(GENERAL, "onDroppedConnection():\n");
  droppedConnectionCallback = cb;
}

//***********************************************************************
void ICACHE_FLASH_ATTR painlessMesh::onChangedConnections(changedConnectionsCallback_t cb) {
  Log(GENERAL, "onChangedConnections():\n");
  changedConnectionsCallback = cb;
}

//***********************************************************************
void ICACHE_FLASH_ATTR painlessMesh::onNodeTimeAdjusted(nodeTimeAdjustedCallback_t cb) {
  Log(GENERAL, "onNodeTimeAdjusted():\n");
  nodeTimeAdjustedCallback = cb;
}

//***********************************************************************
void ICACHE_FLASH_ATTR painlessMesh::onNodeDelayReceived(nodeDelayCallback_t cb) {
  Log(GENERAL, "onNodeDelayReceived():\n");
  nodeDelayReceivedCallback = cb;
}

void ICACHE_FLASH_ATTR painlessMesh::eraseClosedConnections() {
  Log(CONNECTION, "eraseClosedConnections():\n");
  this->subs.remove_if([](const std::shared_ptr<MeshConnection> &conn) {
    return !conn->connected;
  });
}

bool ICACHE_FLASH_ATTR painlessMesh::closeConnectionSTA()
{
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

//***********************************************************************
std::list<uint32_t> ICACHE_FLASH_ATTR
painlessMesh::getNodeList(bool includeSelf) {
  return painlessmesh::layout::asList(this->asNodeTree(), includeSelf);
}

//***********************************************************************
void ICACHE_FLASH_ATTR tcpSentCb(void * arg, AsyncClient * client, size_t len, uint32_t time) {
    if (staticThis->semaphoreTake()) {
        if (arg == NULL) {
          Log(COMMUNICATION, "tcpSentCb(): no valid connection found\n");
        }
        auto conn = static_cast<MeshConnection*>(arg);
        conn->sentBufferTask.forceNextIteration();
        staticThis->semaphoreGive();
    }
}

void ICACHE_FLASH_ATTR meshRecvCb(void * arg, AsyncClient *client, void * data, size_t len) {
    if (staticThis->semaphoreTake()) {
        if (arg == NULL) {
          Log(COMMUNICATION, "meshRecvCb(): no valid connection found\n");
        }
        auto receiveConn = static_cast<MeshConnection*>(arg);

        Log(COMMUNICATION, "meshRecvCb(): fromId=%u\n",
            receiveConn ? receiveConn->nodeId : 0);

        receiveConn->receiveBuffer.push(static_cast<const char*>(data), len, shared_buffer);

        // Signal that we are done
        client->ack(len); // ackLater?
        //client->ackLater();
        //tcp_recved(receiveConn->pcb, total_length);

        receiveConn->readBufferTask.forceNextIteration(); 
        staticThis->semaphoreGive();
    }
}

void ICACHE_FLASH_ATTR MeshConnection::handleMessage(String &buffer,
                                                     uint32_t receivedAt) {
  using namespace painlessmesh;
  Log(COMMUNICATION, "meshRecvCb(): Recvd from %u-->%s<--\n", this->nodeId,
      buffer.c_str());

  // TODO use shared_from_this
  auto rConn = layout::findRoute<MeshConnection>(
      (*staticThis),
      [client = this->client](std::shared_ptr<MeshConnection> s) {
        return (*s->client) == (*client);
      });

  auto variant = painlessmesh::protocol::Variant(buffer);
  if (variant.error) {
    Log(ERROR,
        "handleMessage(): parseObject() failed. total_length=%d, data=%s<--\n",
        buffer.length(), buffer.c_str());
    return;
  }

  if (variant.is<painlessmesh::protocol::Broadcast>()) {
    auto pkg = variant.to<painlessmesh::protocol::Broadcast>();
    staticThis->broadcastMessage(pkg, rConn);
    if (staticThis->receivedCallback)
      staticThis->receivedCallback(pkg.from, pkg.msg);
    return;
  }

  if (variant.is<painlessmesh::protocol::TimeSync>()) {
    auto pkg = variant.to<painlessmesh::protocol::TimeSync>();
    staticThis->handleTimeSync(rConn, pkg, receivedAt);
    return;
  }

  if (variant.is<painlessmesh::protocol::Single>()) {
    auto pkg = variant.to<painlessmesh::protocol::Single>();
    if (pkg.dest == staticThis->getNodeId()) {
      if (staticThis->receivedCallback)
        staticThis->receivedCallback(pkg.from, pkg.msg);
    } else {
      staticThis->send<painlessmesh::protocol::Single>(pkg);
    }
    return;
  }

  if (variant.is<painlessmesh::protocol::TimeDelay>()) {
    auto pkg = variant.to<painlessmesh::protocol::TimeDelay>();
    if (pkg.dest == staticThis->getNodeId())
      staticThis->handleTimeDelay(rConn, pkg, receivedAt);
    else
      staticThis->send<painlessmesh::protocol::TimeDelay>(pkg);
    return;
  }

  if (variant.is<painlessmesh::protocol::NodeSyncRequest>()) {
    auto request = variant.to<protocol::NodeSyncReply>();
    staticThis->handleNodeSync(rConn, request);
    staticThis->send<protocol::NodeSyncReply>(
        rConn, rConn->reply(std::move(staticThis->asNodeTree())), true);
    return;
  }

  if (variant.is<painlessmesh::protocol::NodeSyncReply>()) {
    auto reply = variant.to<protocol::NodeSyncReply>();
    staticThis->handleNodeSync(rConn, reply);
    return;
  }

  Log(ERROR, "meshRecvCb(): unexpected json\n");
  return;
}

//***********************************************************************
// WiFi event handler
void ICACHE_FLASH_ATTR painlessMesh::eventHandleInit() {
#ifdef ESP32
  eventScanDoneHandler =
      WiFi.onEvent(
          [](WiFiEvent_t event, WiFiEventInfo_t info) {
            Log(CONNECTION, "eventScanDoneHandler: SYSTEM_EVENT_SCAN_DONE\n");
            staticThis->stationScan.task.setCallback(
                []() { staticThis->stationScan.scanComplete(); });
            staticThis->stationScan.task.forceNextIteration();
          },
          WiFiEvent_t::SYSTEM_EVENT_SCAN_DONE);

  eventSTAStartHandler = WiFi.onEvent(
      [](WiFiEvent_t event, WiFiEventInfo_t info) {
        // staticThis->stationScan.task.forceNextIteration();
        Log(CONNECTION, "eventSTAStartHandler: SYSTEM_EVENT_STA_START\n");
      },
      WiFiEvent_t::SYSTEM_EVENT_STA_START);

  eventSTADisconnectedHandler = WiFi.onEvent(
      [](WiFiEvent_t event, WiFiEventInfo_t info) {
        staticThis->_station_got_ip = false;
        Log(CONNECTION,
            "eventSTADisconnectedHandler: SYSTEM_EVENT_STA_DISCONNECTED\n");
        // staticThis->stationScan.task.forceNextIteration();
        WiFi.disconnect();
        // Search for APs and connect to the best one
        staticThis->stationScan.connectToAP();
      },
      WiFiEvent_t::SYSTEM_EVENT_STA_DISCONNECTED);

  eventSTAGotIPHandler = WiFi.onEvent(
      [](WiFiEvent_t event, WiFiEventInfo_t info) {
        staticThis->_station_got_ip = true;
        Log(CONNECTION, "eventSTAGotIPHandler: SYSTEM_EVENT_STA_GOT_IP\n");
        staticThis->tcpConnect();  // Connect to TCP port
      },
      WiFiEvent_t::SYSTEM_EVENT_STA_GOT_IP);
#elif defined(ESP8266)
  eventSTAConnectedHandler = WiFi.onStationModeConnected(
      [&](const WiFiEventStationModeConnected &event) {
        // Log(CONNECTION, "Event: Station Mode Connected to \"%s\"\n",
        // event.ssid.c_str());
        Log(CONNECTION, "Event: Station Mode Connected\n");
      });

  eventSTADisconnectedHandler =
      WiFi.onStationModeDisconnected(
          [&](const WiFiEventStationModeDisconnected &event) {
            staticThis->_station_got_ip = false;
            // Log(CONNECTION, "Event: Station Mode
            // Disconnected from %s\n", event.ssid.c_str());
            Log(CONNECTION, "Event: Station Mode Disconnected\n");
            WiFi.disconnect();
            staticThis->stationScan
                .connectToAP();  // Search for APs and connect to the best one
          });

  eventSTAAuthChangeHandler = WiFi.onStationModeAuthModeChanged(
      [&](const WiFiEventStationModeAuthModeChanged &event) {
        Log(CONNECTION, "Event: Station Mode Auth Mode Change\n");
      });

  eventSTAGotIPHandler =
      WiFi.onStationModeGotIP([&](const WiFiEventStationModeGotIP &event) {
        staticThis->_station_got_ip = true;
        Log(CONNECTION,
            "Event: Station Mode Got IP (IP: %s  Mask: %s  Gateway: %s)\n",
            event.ip.toString().c_str(), event.mask.toString().c_str(),
            event.gw.toString().c_str());
        staticThis->tcpConnect(); // Connect to TCP port
      });

  eventSoftAPConnectedHandler = WiFi.onSoftAPModeStationConnected(
      [&](const WiFiEventSoftAPModeStationConnected &event) {
        Log(CONNECTION, "Event: %lu Connected to AP Mode Station\n",
            staticThis->encodeNodeId(event.mac));
      });

  eventSoftAPDisconnectedHandler = WiFi.onSoftAPModeStationDisconnected(
      [&](const WiFiEventSoftAPModeStationDisconnected &event) {
        Log(CONNECTION, "Event: %lu Disconnected from AP Mode Station\n",
            staticThis->encodeNodeId(event.mac));
      });
#endif // ESP32
    return;
}
