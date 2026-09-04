#ifndef BLINKER_RUNTIME_GATEWAYPERMITJOINCOORDINATOR_H
#define BLINKER_RUNTIME_GATEWAYPERMITJOINCOORDINATOR_H

#include "../interface/IClock.h"
#include "../interface/IGatewayPermitJoinAdapter.h"
#include "../protocol/gateway/Contracts.h"
#include "IGatewayManagementControl.h"

namespace blinker {

// Executes one exact short-lived permit-join window. It owns only idempotency,
// expiry and management results; discovery and opaque bytes stay in Adapter.
class GatewayPermitJoinCoordinator final
    : public IGatewayManagementControl {
public:
    GatewayPermitJoinCoordinator(
        IGatewayPermitJoinAdapter& adapter,
        IClock& clock);
    ~GatewayPermitJoinCoordinator() override;

    Result handleCommand(ByteView encoded) override;
    void poll() override;
    ByteView pendingResult() const override;
    void markResultPublished() override;
    void reset() override;

    bool windowOpen() const { return active_; }
    uint16_t activeAdapterId() const;
    ByteView activeOperationId() const;
    uint64_t expiresAtUnixSeconds() const;

private:
    struct OwnedWindow {
        uint8_t operationId[gateway::kOperationIdSize];
        uint16_t adapterId;
        uint64_t expiresAtUnixSeconds;

        OwnedWindow()
            : operationId(), adapterId(0U),
              expiresAtUnixSeconds(0U) {}
    };

    bool sameWindow(
        const gateway::GatewayPermitJoinCommandView& command) const;
    void copyWindow(
        const gateway::GatewayPermitJoinCommandView& command);
    void reply(
        const gateway::GatewayPermitJoinCommandView& command,
        gateway::GatewayPermitJoinStatus status);
    gateway::GatewayPermitJoinStatus openFailureStatus(
        ErrorCode error) const;
    void expire(gateway::GatewayPermitJoinStatus status);
    void clearState();

    IGatewayPermitJoinAdapter& adapter_;
    IClock& clock_;
    OwnedWindow activeWindow_;
    OwnedWindow terminalWindow_;
    gateway::GatewayPermitJoinStatus terminalStatus_;
    uint8_t result_[gateway::kGatewayPermitJoinResultMaximumEncodedSize];
    size_t resultSize_;
    bool active_;
    bool hasTerminal_;
    bool publishPending_;

    GatewayPermitJoinCoordinator(const GatewayPermitJoinCoordinator&);
    GatewayPermitJoinCoordinator& operator=(
        const GatewayPermitJoinCoordinator&);
};

} // namespace blinker

#endif
