#ifndef BLINKER_RUNTIME_GATEWAYMANAGEMENTCONTROLMUX_H
#define BLINKER_RUNTIME_GATEWAYMANAGEMENTCONTROLMUX_H

#include "../protocol/gateway/Contracts.h"
#include "IGatewayManagementControl.h"

namespace blinker {

// Dispatches one child's proof/revoke/renew without coupling its coordinators.
// Hub permit-join is owned only by GatewayChildManagementRouter.
// Work must finish AND its result be published before another control family
// can take ownership. A Ready reply alone does not release a live port lease.
class GatewayManagementControlMux final : public IGatewayManagementControl {
public:
    GatewayManagementControlMux(
        IGatewayManagementControl& proof,
        IGatewayManagementControl& revocation,
        IGatewayManagementControl& renewal)
        : proof_(proof), revocation_(revocation),
          renewal_(renewal), admission_(nullptr), active_(None) {}

    void setAdmission(IGatewayManagementAdmission& admission) { admission_ = &admission; }

    bool continuesCommand(ByteView encoded) const override {
        return active_ != None && activeControl().continuesCommand(encoded);
    }

    Result handleCommand(ByteView encoded) override {
        gateway::GatewayProofCommandView proofCommand;
        const Result proofDecoded =
            gateway::decodeGatewayProofCommand(encoded, proofCommand);
        gateway::GatewayRevocationCommandView revocationCommand;
        const Result revocationDecoded = proofDecoded
            ? Result::failure(ErrorCode::InvalidEncoding)
            : gateway::decodeGatewayRevocationCommand(
                  encoded, revocationCommand);
        gateway::GatewayCredentialRenewalCommandView renewalCommand;
        const Result renewalDecoded =
            (proofDecoded || revocationDecoded)
                ? Result::failure(ErrorCode::InvalidEncoding)
                : gateway::decodeGatewayCredentialRenewalCommand(
                      encoded, renewalCommand);
        const Active requested = proofDecoded ? Proof :
            (revocationDecoded ? Revocation : (renewalDecoded ? Renewal : None));
        if (requested == None) {
            return Result::failure(ErrorCode::InvalidEncoding);
        }
        // Admission belongs to this child only. A blocked command stays in the
        // Broker's bounded ledger. Same-family alone is not a continuation.
        if (admission_ != nullptr && !admission_->allowNewManagementWork() &&
            !(active_ == requested && activeControl().continuesCommand(encoded))) {
            return Result::failure(ErrorCode::WouldBlock);
        }
        if (active_ != None && active_ != requested) {
            IGatewayManagementControl& current = activeControl();
            if (current.busy() || current.ownsChildSession() ||
                !current.pendingResult().empty()) {
                return Result::failure(ErrorCode::StateConflict);
            }
            current.reset();
            if (current.busy() || current.ownsChildSession()) {
                return Result::failure(ErrorCode::StateConflict);
            }
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

    bool busy() const override {
        return active_ != None && activeControl().busy();
    }

    ByteView pendingResult() const override {
        return active_ != None ? activeControl().pendingResult() : ByteView();
    }

    bool ownsChildSession() const override {
        return active_ != None && activeControl().ownsChildSession();
    }

    void markResultPublished() override {
        if (active_ != None) activeControl().markResultPublished();
    }

    void reset() override {
        proof_.reset();
        revocation_.reset();
        renewal_.reset();
        // Forgetting a command is not proof of asynchronous physical release.
        if (active_ != None && !activeControl().busy() && !activeControl().ownsChildSession()) active_ = None;
    }

private:
    enum Active : uint8_t {
        None = 0U,
        Proof = 1U,
        Revocation = 2U,
        Renewal = 3U
    };

    IGatewayManagementControl& activeControl() const {
        if (active_ == Proof) return proof_;
        if (active_ == Revocation) return revocation_;
        return renewal_;
    }

    IGatewayManagementControl& proof_;
    IGatewayManagementControl& revocation_;
    IGatewayManagementControl& renewal_;
    IGatewayManagementAdmission* admission_;
    Active active_;
};

} // namespace blinker

#endif
