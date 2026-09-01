#ifndef BLINKER_RUNTIME_GATEWAYMANAGEMENTCONTROLMUX_H
#define BLINKER_RUNTIME_GATEWAYMANAGEMENTCONTROLMUX_H

#include "../protocol/gateway/Contracts.h"
#include "IGatewayManagementControl.h"

namespace blinker {

// Dispatches exact management wire kinds without coupling their coordinators.
// A terminal result must be published before another control family can take
// ownership, which prevents proof and credential mutation from overlapping.
class GatewayManagementControlMux final : public IGatewayManagementControl {
public:
    GatewayManagementControlMux(
        IGatewayManagementControl& proof,
        IGatewayManagementControl& revocation)
        : proof_(proof), revocation_(revocation), active_(None) {}

    Result handleCommand(ByteView encoded) override {
        gateway::GatewayProofCommandView proofCommand;
        const Result proofDecoded =
            gateway::decodeGatewayProofCommand(encoded, proofCommand);
        gateway::GatewayRevocationCommandView revocationCommand;
        const Result revocationDecoded = proofDecoded
            ? Result::failure(ErrorCode::InvalidEncoding)
            : gateway::decodeGatewayRevocationCommand(
                  encoded, revocationCommand);
        const Active requested = proofDecoded
                                     ? Proof
                                     : (revocationDecoded
                                            ? Revocation
                                            : None);
        if (requested == None) {
            return Result::failure(ErrorCode::InvalidEncoding);
        }
        if (active_ != None && active_ != requested) {
            IGatewayManagementControl& current = activeControl();
            if (!current.pendingResult().empty()) {
                return Result::failure(ErrorCode::StateConflict);
            }
            current.reset();
        }
        active_ = requested;
        return activeControl().handleCommand(encoded);
    }

    void poll() override {
        if (active_ != None) activeControl().poll();
    }

    ByteView pendingResult() const override {
        if (active_ == Proof) return proof_.pendingResult();
        if (active_ == Revocation) return revocation_.pendingResult();
        return ByteView();
    }

    void markResultPublished() override {
        if (active_ != None) activeControl().markResultPublished();
    }

    void reset() override {
        proof_.reset();
        revocation_.reset();
        active_ = None;
    }

private:
    enum Active : uint8_t { None = 0U, Proof = 1U, Revocation = 2U };

    IGatewayManagementControl& activeControl() {
        return active_ == Proof ? proof_ : revocation_;
    }

    IGatewayManagementControl& proof_;
    IGatewayManagementControl& revocation_;
    Active active_;
};

} // namespace blinker

#endif
