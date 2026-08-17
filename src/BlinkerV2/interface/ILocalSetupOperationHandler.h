#ifndef BLINKER_INTERFACE_ILOCALSETUPOPERATIONHANDLER_H
#define BLINKER_INTERFACE_ILOCALSETUPOPERATIONHANDLER_H

#include "../provisioning/LocalSetupContract.h"

namespace blinker {

// Verified, session-scoped authority borrowed from LocalSetupApplication.
// Views remain valid until endAuthorizedSession() and must not be retained.
struct LocalSetupAuthorization {
    ByteView ticketId;
    ByteView controllerId;
    ByteView controllerSecretDigest;
    uint32_t ownershipGeneration;
    uint32_t controllerPermissions;
    uint32_t authorizedOperations;

    LocalSetupAuthorization()
        : ownershipGeneration(0U), controllerPermissions(0U),
          authorizedOperations(0U) {}
};

struct LocalSetupOperationContext {
    LocalSetupAuthorization authorization;
    ByteView requestDigest;
    uint32_t requestId;
    bool exactReplay;

    LocalSetupOperationContext() : requestId(0U), exactReplay(false) {}
};

// Product-owned mutation seam. Implementations bind the existing ownership,
// controller and WiFi coordinators without exposing stores to BLE or Noise.
// A request identified by ticketId + requestId + requestDigest is idempotent;
// exactReplay asks the handler to return the same logical result.
class ILocalSetupOperationHandler {
public:
    virtual ~ILocalSetupOperationHandler() {}

    // Read-only preflight before the secure channel starts. A fresh device
    // returns its target generation; an interrupted saga may return the same
    // generation so a new Noise/Ticket session can resume it. A fully active
    // device without a resumable journal returns StateConflict. This method
    // must not create, advance or erase a durable transaction.
    virtual Result prepareSetupSession(
        ByteView setupSessionLocator,
        uint32_t& ownershipGeneration) = 0;

    virtual Result
    beginAuthorizedSession(const LocalSetupAuthorization& authorization) = 0;

    virtual Result
    handleAuthorizedOperation(const LocalSetupOperationContext& context,
                              LocalSetupOperation operation,
                              ByteView requestBody,
                              MutableByteSpan operationScratch,
                              MutableByteSpan responseBodyStorage,
                              ByteView& responseBody) = 0;

    virtual void endAuthorizedSession() = 0;
};

} // namespace blinker

#endif
