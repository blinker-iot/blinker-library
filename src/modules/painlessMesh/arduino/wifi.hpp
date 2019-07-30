#ifndef _PAINLESS_MESH_ARDUINO_WIFI_HPP_
#define _PAINLESS_MESH_ARDUINO_WIFI_HPP_

#include "../painlessmesh/configuration.hpp"

#ifdef PAINLESSMESH_ENABLE_ARDUINO_WIFI
#include "../painlessMeshConnection.h"
#include "../painlessMeshSTA.h"

#include "../painlessmesh/logger.hpp"
#include "../painlessmesh/mesh.hpp"
#include "../painlessmesh/router.hpp"
#include "../painlessmesh/tcp.hpp"

extern painlessmesh::logger::LogClass Log;

namespace painlessmesh {
namespace wifi {
class Mesh : public painlessmesh::Mesh<MeshConnection> {
 public:
  /** Initialize the mesh network
   *
   * Add this to your setup() function. This routine does the following things:
   *
   * - Starts a wifi network
   * - Begins searching for other wifi networks that are part of the mesh
   * - Logs on to the best mesh network node it finds… if it doesn’t find
   * anything, it starts a new search in 5 seconds.
   *
   * @param ssid The name of your mesh.  All nodes share same AP ssid. They are
   * distinguished by BSSID.
   * @param password Wifi password to your mesh.
   * @param port the TCP port that you want the mesh server to run on. Defaults
   * to 5555 if not specified.
   * @param connectMode Switch between WIFI_AP, WIFI_STA and WIFI_AP_STA
   * (default) mode
   */
  void init(TSTRING ssid, TSTRING password, uint16_t port = 5555,
            WiFiMode_t connectMode = WIFI_AP_STA, uint8_t channel = 1,
            uint8_t hidden = 0, uint8_t maxconn = MAX_CONN) {
    using namespace logger;
    // Init random generator seed to generate delay variance
    randomSeed(millis());

    // Shut Wifi down and start with a blank slage
    if (WiFi.status() != WL_DISCONNECTED) WiFi.disconnect();

    Log(STARTUP, "init(): %d\n",
        WiFi.setAutoConnect(false));  // Disable autoconnect
    WiFi.persistent(false);

    // start configuration
    if (!WiFi.mode(connectMode)) {
      Log(GENERAL, "WiFi.mode() false");
    }

    _meshSSID = ssid;
    _meshPassword = password;
    _meshChannel = channel;
    _meshHidden = hidden;
    _meshMaxConn = maxconn;
    _meshPort = port;

    uint8_t MAC[] = {0, 0, 0, 0, 0, 0};
    if (WiFi.softAPmacAddress(MAC) == 0) {
      Log(ERROR, "init(): WiFi.softAPmacAddress(MAC) failed.\n");
    }
    uint32_t nodeId = encodeNodeId(MAC);

    this->init(nodeId, port);

    tcpServerInit();
    eventHandleInit();

    _apIp = IPAddress(0, 0, 0, 0);

    if (connectMode & WIFI_AP) {
      apInit(nodeId);  // setup AP
    }
    if (connectMode & WIFI_STA) {
      this->initStation();
    }
  }

  /** Initialize the mesh network
   *
   * Add this to your setup() function. This routine does the following things:
   *
   * - Starts a wifi network
   * - Begins searching for other wifi networks that are part of the mesh
   * - Logs on to the best mesh network node it finds… if it doesn’t find
   * anything, it starts a new search in 5 seconds.
   *
   * @param ssid The name of your mesh.  All nodes share same AP ssid. They are
   * distinguished by BSSID.
   * @param password Wifi password to your mesh.
   * @param port the TCP port that you want the mesh server to run on. Defaults
   * to 5555 if not specified.
   * @param connectMode Switch between WIFI_AP, WIFI_STA and WIFI_AP_STA
   * (default) mode
   */
  void init(TSTRING ssid, TSTRING password, Scheduler *baseScheduler,
            uint16_t port = 5555, WiFiMode_t connectMode = WIFI_AP_STA,
            uint8_t channel = 1, uint8_t hidden = 0,
            uint8_t maxconn = MAX_CONN) {
    this->setScheduler(baseScheduler);
    init(ssid, password, port, connectMode, channel, hidden, maxconn);
  }

  /**
   * Connect (as a station) to a specified network and ip
   *
   * You can pass {0,0,0,0} as IP to have it connect to the gateway
   *
   * This stops the node from scanning for other (non specified) nodes
   * and you should probably also use this node as an anchor: `setAnchor(true)`
   */
  void stationManual(TSTRING ssid, TSTRING password, uint16_t port = 0,
                     IPAddress remote_ip = IPAddress(0, 0, 0, 0)) {
    // Set station config
    stationScan.manualIP = remote_ip;

    // Start scan
    stationScan.init(this, ssid, password, port);
    stationScan.manual = true;
  }

  void initStation() {
    stationScan.init(this, _meshSSID, _meshPassword, _meshPort);
    mScheduler->addTask(stationScan.task);
    stationScan.task.enable();
  }

  void tcpServerInit() {
    using namespace logger;
    Log(GENERAL, "tcpServerInit():\n");
    _tcpListener = new AsyncServer(_meshPort);
    painlessmesh::tcp::initServer<MeshConnection,
                                  painlessmesh::Mesh<MeshConnection>>(
        (*_tcpListener), (*this));
    Log(STARTUP, "AP tcp server established on port %d\n", _meshPort);
    return;
  }

  void tcpConnect() {
    using namespace logger;
    // TODO: move to Connection or StationConnection?
    Log(GENERAL, "tcpConnect():\n");
    if (stationScan.manual && stationScan.port == 0)
      return;  // We have been configured not to connect to the mesh

    // TODO: We could pass this to tcpConnect instead of loading it here

    if (_station_got_ip && WiFi.localIP()) {
      AsyncClient *pConn = new AsyncClient();

      IPAddress ip = WiFi.gatewayIP();
      if (stationScan.manualIP) {
        ip = stationScan.manualIP;
      }

      painlessmesh::tcp::connect<MeshConnection,
                                 painlessmesh::Mesh<MeshConnection>>(
          (*pConn), ip, stationScan.port, (*this));
    } else {
      Log(ERROR, "tcpConnect(): err Something un expected in tcpConnect()\n");
    }
  }

  bool setHostname(const char *hostname) {
#ifdef ESP8266
    return WiFi.hostname(hostname);
#elif defined(ESP32)
    if (strlen(hostname) > 32) {
      return false;
    }
    return WiFi.setHostname(hostname);
#endif  // ESP8266
  }

  IPAddress getStationIP() { return WiFi.localIP(); }
  IPAddress getAPIP() { return _apIp; }

  void stop() {
    // remove all WiFi events
#ifdef ESP32
    WiFi.removeEvent(eventScanDoneHandler);
    WiFi.removeEvent(eventSTAStartHandler);
    WiFi.removeEvent(eventSTADisconnectedHandler);
    WiFi.removeEvent(eventSTAGotIPHandler);
#elif defined(ESP8266)
    eventSTAConnectedHandler = WiFiEventHandler();
    eventSTADisconnectedHandler = WiFiEventHandler();
    eventSTAAuthChangeHandler = WiFiEventHandler();
    eventSTAGotIPHandler = WiFiEventHandler();
    eventSoftAPConnectedHandler = WiFiEventHandler();
    eventSoftAPDisconnectedHandler = WiFiEventHandler();
#endif  // ESP32
    // Stop scanning task
    stationScan.task.setCallback(NULL);
    mScheduler->deleteTask(stationScan.task);
    painlessmesh::Mesh<MeshConnection>::stop();

    // Shutdown wifi hardware
    if (WiFi.status() != WL_DISCONNECTED) WiFi.disconnect();
  }

 protected:
  friend class ::StationScan;
  TSTRING _meshSSID;
  TSTRING _meshPassword;
  uint8_t _meshChannel;
  uint8_t _meshHidden;
  uint8_t _meshMaxConn;
  uint16_t _meshPort;

  IPAddress _apIp;
  StationScan stationScan;

  void init(Scheduler *scheduler, uint32_t id, uint16_t port) {
    painlessmesh::Mesh<MeshConnection>::init(scheduler, id, port);
  }

  void init(uint32_t id, uint16_t port) {
    painlessmesh::Mesh<MeshConnection>::init(id, port);
  }

  void apInit(uint32_t nodeId) {
    _apIp = IPAddress(10, (nodeId & 0xFF00) >> 8, (nodeId & 0xFF), 1);
    IPAddress netmask(255, 255, 255, 0);

    WiFi.softAPConfig(_apIp, _apIp, netmask);
    WiFi.softAP(_meshSSID.c_str(), _meshPassword.c_str(), _meshChannel,
                _meshHidden, _meshMaxConn);
  }

  uint32_t encodeNodeId(const uint8_t *hwaddr) {
    using namespace painlessmesh::logger;
    Log(GENERAL, "encodeNodeId():\n");
    uint32_t value = 0;

    value |= hwaddr[2] << 24;  // Big endian (aka "network order"):
    value |= hwaddr[3] << 16;
    value |= hwaddr[4] << 8;
    value |= hwaddr[5];
    return value;
  }

  void eventHandleInit() {
    using namespace logger;
#ifdef ESP32
    eventScanDoneHandler = WiFi.onEvent(
        [this](WiFiEvent_t event, WiFiEventInfo_t info) {
          if (this->semaphoreTake()) {
            Log(CONNECTION, "eventScanDoneHandler: SYSTEM_EVENT_SCAN_DONE\n");
            this->stationScan.task.setCallback(
                [this]() { this->stationScan.scanComplete(); });
            this->stationScan.task.forceNextIteration();
            this->semaphoreGive();
          }
        },
        WiFiEvent_t::SYSTEM_EVENT_SCAN_DONE);

    eventSTAStartHandler = WiFi.onEvent(
        [this](WiFiEvent_t event, WiFiEventInfo_t info) {
          if (this->semaphoreTake()) {
            Log(CONNECTION, "eventSTAStartHandler: SYSTEM_EVENT_STA_START\n");
            this->semaphoreGive();
          }
        },
        WiFiEvent_t::SYSTEM_EVENT_STA_START);

    eventSTADisconnectedHandler = WiFi.onEvent(
        [this](WiFiEvent_t event, WiFiEventInfo_t info) {
          if (this->semaphoreTake()) {
            this->_station_got_ip = false;
            Log(CONNECTION,
                "eventSTADisconnectedHandler: SYSTEM_EVENT_STA_DISCONNECTED\n");
            // WiFi.disconnect();
            // Search for APs and connect to the best one
            this->stationScan.connectToAP();
            this->semaphoreGive();
          }
        },
        WiFiEvent_t::SYSTEM_EVENT_STA_DISCONNECTED);

    eventSTAGotIPHandler = WiFi.onEvent(
        [this](WiFiEvent_t event, WiFiEventInfo_t info) {
          if (this->semaphoreTake()) {
            this->_station_got_ip = true;
            Log(CONNECTION, "eventSTAGotIPHandler: SYSTEM_EVENT_STA_GOT_IP\n");
            this->tcpConnect();  // Connect to TCP port
            this->semaphoreGive();
          }
        },
        WiFiEvent_t::SYSTEM_EVENT_STA_GOT_IP);
#elif defined(ESP8266)
    eventSTAConnectedHandler = WiFi.onStationModeConnected(
        [&](const WiFiEventStationModeConnected &event) {
          // Log(CONNECTION, "Event: Station Mode Connected to \"%s\"\n",
          // event.ssid.c_str());
          Log(CONNECTION, "Event: Station Mode Connected\n");
        });

    eventSTADisconnectedHandler = WiFi.onStationModeDisconnected(
        [&](const WiFiEventStationModeDisconnected &event) {
          this->_station_got_ip = false;
          // Log(CONNECTION, "Event: Station Mode
          // Disconnected from %s\n", event.ssid.c_str());
          Log(CONNECTION, "Event: Station Mode Disconnected\n");
          WiFi.disconnect();
          this->stationScan
              .connectToAP();  // Search for APs and connect to the best one
        });
    eventSTAAuthChangeHandler = WiFi.onStationModeAuthModeChanged(
        [&](const WiFiEventStationModeAuthModeChanged &event) {
          Log(CONNECTION, "Event: Station Mode Auth Mode Change\n");
        });

    eventSTAGotIPHandler =
        WiFi.onStationModeGotIP([&](const WiFiEventStationModeGotIP &event) {
          this->_station_got_ip = true;
          Log(CONNECTION,
              "Event: Station Mode Got IP (IP: %s  Mask: %s  Gateway: %s)\n",
              event.ip.toString().c_str(), event.mask.toString().c_str(),
              event.gw.toString().c_str());
          this->tcpConnect();  // Connect to TCP port
        });

    eventSoftAPConnectedHandler = WiFi.onSoftAPModeStationConnected(
        [&](const WiFiEventSoftAPModeStationConnected &event) {
          Log(CONNECTION, "Event: %lu Connected to AP Mode Station\n",
              this->encodeNodeId(event.mac));
        });

    eventSoftAPDisconnectedHandler = WiFi.onSoftAPModeStationDisconnected(
        [&](const WiFiEventSoftAPModeStationDisconnected &event) {
          Log(CONNECTION, "Event: %lu Disconnected from AP Mode Station\n",
              this->encodeNodeId(event.mac));
        });
#endif  // ESP32
    return;
  }

#ifdef ESP32
  WiFiEventId_t eventScanDoneHandler;
  WiFiEventId_t eventSTAStartHandler;
  WiFiEventId_t eventSTADisconnectedHandler;
  WiFiEventId_t eventSTAGotIPHandler;
#elif defined(ESP8266)
  WiFiEventHandler eventSTAConnectedHandler;
  WiFiEventHandler eventSTADisconnectedHandler;
  WiFiEventHandler eventSTAAuthChangeHandler;
  WiFiEventHandler eventSTAGotIPHandler;
  WiFiEventHandler eventSoftAPConnectedHandler;
  WiFiEventHandler eventSoftAPDisconnectedHandler;
#endif  // ESP8266
  AsyncServer *_tcpListener;
  bool _station_got_ip = false;
};
}  // namespace wifi
};  // namespace painlessmesh

#endif

#endif

