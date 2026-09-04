#ifndef BLINKER_RUNTIME_GATEWAYMANAGEMENTCONTROLMUX_H
#define BLINKER_RUNTIME_GATEWAYMANAGEMENTCONTROLMUX_H

#include "../protocol/gateway/Contracts.h"
#include "IGatewayManagementControl.h"

namespace blinker {

// Dispatches exact management wire kinds without coupling their coordinators.
// A terminal result must be published before another control family can take
// ownership, so proof, revocation and permit-join never overlap.
class GatewayManagementControlMux final : public IGatewayManagementControl {
public:
    GatewayManagementControlMux(
        IGatewayManagementControl& proof,
        IGatewayManagementControl& revocation,
        IGatewayManagementControl& permitJoin)
        : proof_(proof), revocation_(revocation),
          permitJoin_(permitJoin), renewal_(nullptr), active_(None) {}

    GatewayManagementControlMux(
        IGatewayManagementControl& proof,
        IGatewayManagementControl& revocation,
        IGatewayManagementControl& permitJoin,
        IGatewayManagementControl& renewal)
        : proof_(proof), revocation_(revocation),
          permitJoin_(permitJoin), renewal_(&renewal), active_(None) {}

    Result handleCommand(ByteView encoded) override {
        gateway::GatewayProofCommandView proofCommand;
        const Result proofDecoded =
            gateway::decodeGatewayProofCommand(encoded, proofCommand);
        gateway::GatewayRevocationCommandView revocationCommand;
        const Result revocationDecoded = proofDecoded
            ? Result::failure(ErrorCode::InvalidEncoding)
            : gateway::decodeGatewayRevocationCommand(
                  encoded, revocationCommand);
        gateway::GatewayPermitJoinCommandView permitJoinCommand;
        const Result permitJoinDecoded =
            (proofDecoded || revocationDecoded)
                ? Result::failure(ErrorCode::InvalidEncoding)
                : gateway::decodeGatewayPermitJoinCommand(
                      encoded, permitJoinCommand);
        gateway::GatewayCredentialRenewalCommandView renewalCommand;
        const Result renewalDecoded =
            (proofDecoded || revocationDecoded || permitJoinDecoded)
                ? Result::failure(ErrorCode::InvalidEncoding)
                : gateway::decodeGatewayCredentialRenewalCommand(
                      encoded, renewalCommand);
        const Active requested = proofDecoded
                                     ? Proof
                                     : (revocationDecoded
                                            ? Revocation
                                            : (permitJoinDecoded
                                                   ? PermitJoin
                                                   : (renewalDecoded &&
                                                              renewal_ != nullptr
                                                          ? Renewal
                                                          : None)));
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

    void handleControllerControlResponse(ByteView encoded) override {
        if (active_ == Revocation || active_ == Renewal) {
            activeControl().handleControllerControlResponse(encoded);
        }
    }

    void poll() override {
        if (active_ != None) activeControl().poll();
    }

    ByteView pendingResult() const override {
        if (active_ == Proof) return proof_.pendingResult();
        if (active_ == Revocation) return revocation_.pendingResult();
        if (active_ == PermitJoin) return permitJoin_.pendingResult();
        if (active_ == Renewal && renewal_ != nullptr) {
            return renewal_->pendingResult();
        }
        return ByteView();
    }

    void markResultPublished() override {
        if (active_ != None) activeControl().markResultPublished();
    }

    void reset() override {
        proof_.reset();
        revocation_.reset();
        permitJoin_.reset();
        if (renewal_ != nullptr) renewal_->reset();
        active_ = None;
    }

private:
    enum Active : uint8_t {
        None = 0U,
        Proof = 1U,
        Revocation = 2U,
        PermitJoin = 3U,
        Renewal = 4U
    };

    IGatewayManagementControl& activeControl() {
        if (active_ == Proof) return proof_;
        if (active_ == Revocation) return revocation_;
        if (active_ == PermitJoin) return permitJoin_;
        return *renewal_;
    }

    IGatewayManagementControl& proof_;
    IGatewayManagementControl& revocation_;
    IGatewayManagementControl& permitJoin_;
    IGatewayManagementControl* renewal_;
    Active active_;
};

} // namespace blinker

#endif
