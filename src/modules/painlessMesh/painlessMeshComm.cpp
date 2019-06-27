//
//  painlessMeshComm.cpp
//
//
//  Created by Bill Gray on 7/26/16.
//
//
#include "painlessMesh.h"

extern LogClass Log;
extern painlessMesh* staticThis;

bool ICACHE_FLASH_ATTR
painlessMesh::broadcastMessage(painlessmesh::protocol::Broadcast pkg,
                               std::shared_ptr<MeshConnection> exclude) {
  // send a message to every node on the mesh
  bool errCode = false;

  if (exclude != NULL)
    Log(COMMUNICATION,
        "broadcastMessage(): from=%u type=%d, msg=%s exclude=%u\n", pkg.from,
        pkg.type, pkg.msg.c_str(), exclude->nodeId);
  else
    Log(COMMUNICATION,
        "broadcastMessage(): from=%u type=%d, msg=%s exclude=NULL\n", pkg.from,
        pkg.type, pkg.msg.c_str());

  if (this->subs.size() > 0)
    errCode = true;  // Assume true if at least one connections
  for (auto&& connection : this->subs) {
    if (!exclude || connection->nodeId != exclude->nodeId) {
      pkg.dest = connection->nodeId;
      if (!send<painlessmesh::protocol::Broadcast>(connection, pkg))
        errCode = false;  // If any error return 0
    }
  }
  return errCode;
}

