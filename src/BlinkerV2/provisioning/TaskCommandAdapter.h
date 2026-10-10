#ifndef BLINKER_PROVISIONING_TASKCOMMANDADAPTER_H
#define BLINKER_PROVISIONING_TASKCOMMANDADAPTER_H

#include "TaskManagementSession.h"
#include "../protocol/bbp2/Messages.h"

namespace blinker {

// Trusted transport dispatch metadata, NOT a decoded authorization claim.
// generation changes before a transport/session number can be reused. For
// routed traffic peerId is the authenticated envelope's actual source, not a
// nested payload field. For direct/unrouted traffic it is empty (all zero).
// Role is fixed by the installed path; the sender cannot select a reply key.
struct TaskCommandSource {
    uint8_t peerId[16];
    uint32_t generation;
    uint32_t session;
    uint8_t transport;
    TaskResultKey role;
    bbp2::RoutePeerKind peerKind;
    bool routed;
    TaskCommandSource() : peerId(), generation(0U), session(0U), transport(0U),
        role(TaskResultKey::DeviceKey), peerKind(bbp2::RoutePeerKind::LogicalDevice), routed(false) {}
};

// One selector definition; response operations are rejected at admission.
using TaskCommand = TaskWireOperation;

// One adapter per Engine/Session, shared by every enabled transport. No RX or
// reply queue, retained secret, I/O, BBP registry or platform dependency. Call
// from the cooperative management point, outside borrowed native RX callbacks.
class TaskCommandAdapter {
public:
    TaskCommandAdapter(TaskManagementSession& session, TaskAuthorityCache& authority,
        TaskEngine& engine, const DeviceInstanceId& instance, const TaskRootSources& roots);
    Result start(); // Engine already begun; independently refresh/restore roots.
    void stop(); // Cancel pending only; preserve installed tasks and scope.
    void poll(); // Age the same Session even with no incoming traffic.
    void disconnect(const TaskCommandSource& source);
    void disconnectTransport(uint8_t transport, uint32_t generation);
    void invalidateRoots(); // BEFORE changing any actual root.
    Result refreshRoots(); // AFTER serialized mutation; never from an ISR.
    void retire(); // Terminal authority retirement; erasure belongs to product.
    uint32_t rootGeneration() const { return authority_.generation(); }
    Result receive(const TaskCommandSource& source, TaskCommand command, ByteView body,
        MutableByteSpan output, ByteView& response);

private:
    void cancel();
    bool matches(const TaskCommandSource& source) const;
    TaskManagementSession& session_;
    TaskAuthorityCache& authority_;
    TaskEngine& engine_;
    const DeviceInstanceId& instance_;
    const TaskRootSources& roots_;
    TaskCommandSource source_;
    uint64_t channel_;
    bool active_, started_, busy_;
    TaskCommandAdapter(const TaskCommandAdapter&);
    TaskCommandAdapter& operator=(const TaskCommandAdapter&);
};

#if UINTPTR_MAX <= UINT32_MAX
static_assert(sizeof(TaskCommandAdapter) <= 64U, "Task dispatch binding exceeds its 32-bit budget");
#endif
} // namespace blinker
#endif
