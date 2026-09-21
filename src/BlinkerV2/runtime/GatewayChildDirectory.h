#ifndef BLINKER_RUNTIME_GATEWAYCHILDDIRECTORY_H
#define BLINKER_RUNTIME_GATEWAYCHILDDIRECTORY_H

#include "../identity/GatewayChildRecords.h"
#include "IGatewayManagementDelivery.h"
#include "IGatewayManagementControl.h"

namespace blinker {

struct GatewayChildDirectoryEntry {
    GatewayChildRecords& records;
    IGatewayManagementDelivery& delivery;
    IGatewayManagementControl* control;
    GatewayChildDirectoryEntry(GatewayChildRecords& recordOwner, IGatewayManagementDelivery& deliveryHandler,
                               IGatewayManagementControl* controlHandler = nullptr)
        : records(recordOwner), delivery(deliveryHandler), control(controlHandler) {}
};

// Bounded caller-owned durable record directory; not a native connection pool.
// Routing never reserves a slot or writes metadata. Only the selected crypto
// processor commits authenticated material. No duplicate durable index/codec.
class GatewayChildDirectory final : public IGatewayManagementDelivery {
public:
    GatewayChildDirectory(GatewayChildDirectoryEntry* entries, size_t count)
        : entries_(entries), count_(count) {}
    Result process(const GatewayAccessDeliveryContext& context, ByteView exactEnvelope,
                   MutableByteSpan ackOutput, size_t& ackSize) override;
    void invalidate();
    size_t size() const { return count_; }
    GatewayChildDirectoryEntry* entry(size_t index) {
        return entries_ != nullptr && index < count_ ? &entries_[index] : nullptr;
    }
private:
    Result select(StringView logicalId, ByteView instance, ByteView operation,
                  bool initial, GatewayChildDirectoryEntry*& output);
    GatewayChildDirectoryEntry* entries_;
    size_t count_;
    GatewayChildDirectory(const GatewayChildDirectory&) = delete;
    GatewayChildDirectory& operator=(const GatewayChildDirectory&) = delete;
};

} // namespace blinker
#endif
