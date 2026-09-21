#ifndef BLINKER_RUNTIME_GATEWAYPERMITJOINCOORDINATOR_H
#define BLINKER_RUNTIME_GATEWAYPERMITJOINCOORDINATOR_H

#include "../interface/IClock.h"
#include "../interface/IGatewayPermitJoinAdapter.h"
#include "../protocol/gateway/Contracts.h"
#include "IGatewayManagementControl.h"
#include "GatewayManagementBudget.h"

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
    bool continuesCommand(ByteView encoded) const override;
    void poll() override;
    bool busy() const override { return phase_ != Idle; }
    bool ownsChildSession() const override {
        return adapter_.state() != GatewayPermitJoinAdapterState::Stopped;
    }
    ByteView pendingResult() const override;
    void markResultPublished() override;
    void reset() override;
    // End the active window while retaining its exact terminal replay/result.
    // Unlike reset(), this does not forget that the operation was rejected.
    void rejectWindow();

    bool windowOpen() const {
        return phase_ == Ready &&
               adapter_.state() == GatewayPermitJoinAdapterState::Ready;
    }
    uint16_t activeAdapterId() const;
    ByteView activeOperationId() const;
    uint64_t expiresAtUnixSeconds() const;

private:
    enum Phase : uint8_t { Idle, Opening, Ready, Closing };
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
    void replyWindow(const OwnedWindow& window,
                     gateway::GatewayPermitJoinStatus status);
    void finishClose();
    void clearState();

    IGatewayPermitJoinAdapter& adapter_;
    IClock& clock_;
    OwnedWindow activeWindow_;
    OwnedWindow terminalWindow_;
    gateway::GatewayPermitJoinStatus terminalStatus_;
    uint8_t result_[gateway::kGatewayPermitJoinResultMaximumEncodedSize];
    size_t resultSize_;
    GatewayManagementBudget budget_;
    Phase phase_;
    bool hasTerminal_;
    bool publishPending_;

    GatewayPermitJoinCoordinator(const GatewayPermitJoinCoordinator&);
    GatewayPermitJoinCoordinator& operator=(
        const GatewayPermitJoinCoordinator&);
};

} // namespace blinker

#endif
