#include "ControllerControlEndpoint.h"

#include "../core/SecureMemory.h"

namespace blinker {

ControllerControlEndpoint::ControllerControlEndpoint(
    ControllerControlCoordinator& coordinator,
    IRandom& random,
    MutableByteSpan workspace)
    : coordinator_(coordinator),
      random_(random),
      workspace_(workspace),
      sessionId_(0U),
      transportId_(0U),
      active_(false) {}

ControllerControlEndpoint::~ControllerControlEndpoint() {
    reset();
}

Result ControllerControlEndpoint::beginControlWindow(
    const RxContext& rx,
    ByteView& controlNonce) {
    controlNonce = ByteView();
    if (!rx.encrypted || rx.sessionId == 0U) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    if (workspace_.data == nullptr ||
        workspace_.size < kControllerControlWorkspaceSize) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    if (active_) {
        if (!owns(rx)) return Result::failure(ErrorCode::StateConflict);
        controlNonce = coordinator_.controlNonce();
        return controlNonce.size == kControllerControlNonceSize
                   ? Result::success()
                   : Result::failure(ErrorCode::InternalError);
    }

    uint8_t nonce[kControllerControlNonceSize] = {};
    Result result = random_.fill(MutableByteSpan(nonce, sizeof(nonce)));
    if (result) {
        result = coordinator_.beginControlWindow(
            ByteView(nonce, sizeof(nonce)));
    }
    secureZero(MutableByteSpan(nonce, sizeof(nonce)));
    if (!result) return result;

    transportId_ = rx.transportId;
    sessionId_ = rx.sessionId;
    active_ = true;
    controlNonce = coordinator_.controlNonce();
    return Result::success();
}

Result ControllerControlEndpoint::applyControllerMutation(
    const RxContext& rx,
    ByteView encodedGrant,
    ByteView controllerSecret,
    MutableByteSpan output,
    ByteView& receipt) {
    receipt = ByteView();
    if (!rx.encrypted || !active_ || !owns(rx)) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    return coordinator_.apply(
        encodedGrant,
        controllerSecret,
        workspace_,
        output,
        receipt);
}

Result ControllerControlEndpoint::applyPresenceKeyMutation(
    const RxContext& rx,
    ByteView encodedMutation,
    MutableByteSpan output,
    ByteView& receipt) {
    (void)rx;
    receipt = ByteView();
    return coordinator_.applyPresence(encodedMutation, output, receipt);
}

void ControllerControlEndpoint::controllerSessionClosed(
    const RxContext& rx) {
    if (active_ && owns(rx)) reset();
}

void ControllerControlEndpoint::reset() {
    coordinator_.endControlWindow();
    secureZero(workspace_);
    sessionId_ = 0U;
    transportId_ = 0U;
    active_ = false;
}

bool ControllerControlEndpoint::owns(const RxContext& rx) const {
    return rx.transportId == transportId_ && rx.sessionId == sessionId_;
}

} // namespace blinker
