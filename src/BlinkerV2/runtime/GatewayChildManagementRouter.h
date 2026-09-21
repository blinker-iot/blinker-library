#ifndef BLINKER_RUNTIME_GATEWAYCHILDMANAGEMENTROUTER_H
#define BLINKER_RUNTIME_GATEWAYCHILDMANAGEMENTROUTER_H

#include "GatewayChildDirectory.h"

namespace blinker {

// One bounded management owner above per-child coordinators. Permit-join is
// Hub-scoped; controller responses go DIRECTLY to their originating child mux,
// never through this router's mutable current-owner pointer.
class GatewayChildManagementRouter final : public IGatewayManagementControl {
public:
    GatewayChildManagementRouter(GatewayChildDirectory& directory, IGatewayManagementControl& permit)
        : directory_(directory), permit_(permit), permitAdmission_(nullptr), active_(nullptr), terminal_(), terminalSize_(0U) {}
    // Child admission is checked by its own mux after exact directory routing.
    void setPermitAdmission(IGatewayManagementAdmission& admission) { permitAdmission_ = &admission; }
    Result handleCommand(ByteView encoded) override;
    bool continuesCommand(ByteView encoded) const override {
        return active_ != nullptr && active_->continuesCommand(encoded);
    }
    void poll() override;
    bool busy() const override { return active_ != nullptr && active_->busy(); }
    bool ownsChildSession() const override;
    ByteView pendingResult() const override {
        return terminalSize_ != 0U ? ByteView(terminal_, terminalSize_) :
            (active_ != nullptr ? active_->pendingResult() : ByteView());
    }
    void markResultPublished() override;
    void reset() override;
private:
    Result select(ByteView encoded, IGatewayManagementControl*& selected, bool& absentTerminal);
    Result finishAbsent(ByteView encoded);
    GatewayChildDirectory& directory_;
    IGatewayManagementControl& permit_;
    IGatewayManagementAdmission* permitAdmission_;
    IGatewayManagementControl* active_;
    // Finalize/Supersede have no payload: bounded exact result, not a ledger.
    uint8_t terminal_[64U];
    size_t terminalSize_;
    GatewayChildManagementRouter(const GatewayChildManagementRouter&) = delete;
    GatewayChildManagementRouter& operator=(const GatewayChildManagementRouter&) = delete;
};

} // namespace blinker
#endif
