#include "painlessMesh.h"

#include "lwip/init.h"

painlessMesh* staticThis;
LogClass Log;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
ICACHE_FLASH_ATTR painlessMesh::painlessMesh() {}
#pragma GCC diagnostic pop

// general functions
//***********************************************************************
void ICACHE_FLASH_ATTR painlessMesh::init(String ssid, String password, Scheduler *baseScheduler, uint16_t port, WiFiMode_t connectMode, uint8_t channel, uint8_t hidden, uint8_t maxconn) {

#ifdef ESP8266
    // Due to ESP32 multithreaded we need to protect the internal scheduler
    // so currently we use a separate scheduler for internal and external for
    // ESP32
    baseScheduler->setHighPriorityScheduler(&this->_scheduler);
    isExternalScheduler = true;
#endif

    init(ssid, password, port, connectMode, channel, hidden, maxconn);
}

void ICACHE_FLASH_ATTR painlessMesh::init(String ssid, String password, uint16_t port, WiFiMode_t connectMode, uint8_t channel, uint8_t hidden, uint8_t maxconn) {
    // Init random generator seed to generate delay variance
    randomSeed(millis());

    // Shut Wifi down and start with a blank slage
    if (WiFi.status() != WL_DISCONNECTED) WiFi.disconnect();

    Log(STARTUP, "init(): %d\n",
        WiFi.setAutoConnect(false));  // Disable autoconnect
    WiFi.persistent(false);

    staticThis = this;  // provides a way for static callback methods to access "this" object;

    // start configuration
    if(!WiFi.mode(connectMode)) {
      Log(GENERAL, "WiFi.mode() false");
    }

    _meshSSID     = ssid;
    _meshPassword = password;
    _meshPort     = port;
    _meshChannel  = channel;
    _meshHidden   = hidden;
    _meshMaxConn  = maxconn;

    uint8_t MAC[] = {0, 0, 0, 0, 0, 0};
    if (WiFi.softAPmacAddress(MAC) == 0) {
      Log(ERROR, "init(): WiFi.softAPmacAddress(MAC) failed.\n");
    }
    nodeId = encodeNodeId(MAC);

    _apIp = IPAddress(0, 0, 0, 0);

    if (connectMode & WIFI_AP) {
        apInit();       // setup AP
    }
    if (connectMode & WIFI_STA) {
        stationScan.init(this, ssid, password, port);
        _scheduler.addTask(stationScan.task);
    }

#ifdef ESP32
    xSemaphore = xSemaphoreCreateMutex();
#endif

    eventHandleInit();
    
    _scheduler.enableAll();
}

void ICACHE_FLASH_ATTR painlessMesh::stop() {

    // remove all WiFi events
#ifdef ESP32
    WiFi.removeEvent(eventScanDoneHandler);
    WiFi.removeEvent(eventSTAStartHandler);
    WiFi.removeEvent(eventSTADisconnectedHandler);
    WiFi.removeEvent(eventSTAGotIPHandler);
#elif defined(ESP8266)
    eventSTAConnectedHandler       = WiFiEventHandler();
    eventSTADisconnectedHandler    = WiFiEventHandler();
    eventSTAAuthChangeHandler      = WiFiEventHandler();
    eventSTAGotIPHandler           = WiFiEventHandler();
    eventSoftAPConnectedHandler    = WiFiEventHandler();
    eventSoftAPDisconnectedHandler = WiFiEventHandler();
#endif // ESP32

    // Close all connections
    while (subs.size() > 0) {
      auto connection = subs.begin();
      (*connection)->close();
    }

    // Stop scanning task
    stationScan.task.setCallback(NULL);
    _scheduler.deleteTask(stationScan.task);

    // Note that this results in the droppedConnections not to be signalled
    // We might want to change this later
    newConnectionTask.setCallback(NULL);
    _scheduler.deleteTask(newConnectionTask);
    droppedConnectionTask.setCallback(NULL);
    _scheduler.deleteTask(droppedConnectionTask);

    // Shutdown wifi hardware
    if (WiFi.status() != WL_DISCONNECTED)
        WiFi.disconnect();
}

//***********************************************************************
// do nothing if user have other Scheduler, they have to run their scheduler in loop not this library
void ICACHE_FLASH_ATTR painlessMesh::update(void) {
    if ( isExternalScheduler == false ) {
        if( semaphoreTake() )
        {
            _scheduler.execute();
            semaphoreGive();
        }
    }
    return;
}

bool ICACHE_FLASH_ATTR painlessMesh::semaphoreTake(void) {
#ifdef ESP32
    return xSemaphoreTake( xSemaphore, ( TickType_t ) 10 ) == pdTRUE;
#else
    return true;
#endif
}


void ICACHE_FLASH_ATTR painlessMesh::semaphoreGive(void) {
#ifdef ESP32
    xSemaphoreGive( xSemaphore );
#endif
}


//***********************************************************************
bool ICACHE_FLASH_ATTR painlessMesh::sendSingle(uint32_t &destId, String &msg) {
  Log(COMMUNICATION, "sendSingle(): dest=%u msg=%s\n", destId, msg.c_str());
  auto single = painlessmesh::protocol::Single(this->nodeId, destId, msg);
  return send<painlessmesh::protocol::Single>(single);
}

//***********************************************************************
bool ICACHE_FLASH_ATTR painlessMesh::sendBroadcast(String &msg,
                                                   bool includeSelf) {
  Log(COMMUNICATION, "sendBroadcast(): msg=%s\n", msg.c_str());
  auto pkg = painlessmesh::protocol::Broadcast(this->nodeId, 0, msg);
  bool success = broadcastMessage(pkg);
  if (success && includeSelf && this->receivedCallback)
    this->receivedCallback(this->getNodeId(), pkg.msg);
  return success;
}

bool ICACHE_FLASH_ATTR painlessMesh::startDelayMeas(uint32_t nodeID) {
  Log(S_TIME, "startDelayMeas(): NodeId %u\n", nodeID);
  auto conn = painlessmesh::layout::findRoute<MeshConnection>((*this), nodeID);
  if (!conn) return false;
  return send<painlessmesh::protocol::TimeDelay>(
      conn,
      painlessmesh::protocol::TimeDelay(this->nodeId, nodeID, getNodeTime()));
}

void ICACHE_FLASH_ATTR painlessMesh::setDebugMsgTypes(uint16_t newTypes) {
  Log.setLogLevel(newTypes);
}
