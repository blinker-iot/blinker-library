//
//  painlessMeshAP.cpp
//  
//
//  Created by Bill Gray on 7/26/16.
// 
//

#include <Arduino.h>

#include "painlessMesh.h"

extern LogClass Log;
extern painlessMesh* staticThis;

// AP functions
//***********************************************************************
void ICACHE_FLASH_ATTR painlessMesh::apInit(void) {
  _apIp = IPAddress(10, (nodeId & 0xFF00) >> 8, (nodeId & 0xFF), 1);
  IPAddress netmask(255, 255, 255, 0);

  WiFi.softAPConfig(_apIp, _apIp, netmask);
  WiFi.softAP(_meshSSID.c_str(), _meshPassword.c_str(), _meshChannel,
              _meshHidden, _meshMaxConn);

  // establish AP tcpServers
  tcpServerInit();
}

IPAddress ICACHE_FLASH_ATTR painlessMesh::getAPIP()
{
    return _apIp;
}

//***********************************************************************
void ICACHE_FLASH_ATTR painlessMesh::tcpServerInit() {
  Log(GENERAL, "tcpServerInit():\n");

  _tcpListener = new AsyncServer(_meshPort);
  _tcpListener->setNoDelay(true);

  _tcpListener->onClient(
      [](void* arg, AsyncClient* client) {
        if (staticThis->semaphoreTake()) {
          Log(CONNECTION, "New AP connection incoming\n");
          auto conn =
              std::make_shared<MeshConnection>(client, staticThis, false);
          staticThis->subs.push_back(conn);
          staticThis->semaphoreGive();
        }
      },
      NULL);

  _tcpListener->begin();

  Log(STARTUP, "AP tcp server established on port %d\n", _meshPort);
  return;
}
