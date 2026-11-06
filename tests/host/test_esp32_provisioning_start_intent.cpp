#include <BlinkerV2/runtime/InitialProvisioningWindowIntent.h>

#include <cstdlib>
#include <iostream>

namespace {

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            std::cerr << "Initial provisioning intent check failed at line "   \
                      << __LINE__ << ": " << #condition << std::endl;           \
            std::exit(EXIT_FAILURE);                                           \
        }                                                                      \
    } while (false)

using blinker::ErrorCode;
using blinker::InitialProvisioningWindowIntent;
static_assert(sizeof(InitialProvisioningWindowIntent) == 1U,
              "Startup intent must remain a one-byte RAM-only policy");

void defaultBootHasNoMaintenanceWindow() {
    InitialProvisioningWindowIntent intent;
    CHECK(!intent.consume());
    CHECK(!intent.consume());
}

void explicitStartupRequestIsConsumedExactlyOnce() {
    InitialProvisioningWindowIntent intent;
    CHECK(intent.request(true, true, false).ok());
    CHECK(intent.consume());
    CHECK(!intent.consume());
    CHECK(!intent.consume());
}

void duplicatePendingRequestDoesNotReplaceOrConsumeTheIntent() {
    InitialProvisioningWindowIntent intent;
    CHECK(intent.request(true, true, false).ok());
    CHECK(intent.request(true, true, false).code() == ErrorCode::AlreadyExists);
    CHECK(intent.consume());
    CHECK(!intent.consume());
}

void allInvalidStartupStatesAreRejectedWithoutCreatingAnIntent() {
    for (unsigned bits = 0U; bits < 8U; ++bits) {
        const bool unattached = (bits & 1U) != 0U;
        const bool stopped = (bits & 2U) != 0U;
        const bool stopPending = (bits & 4U) != 0U;
        if (unattached && stopped && !stopPending) continue;
        InitialProvisioningWindowIntent intent;
        CHECK(intent.request(unattached, stopped, stopPending).code() == ErrorCode::StateConflict);
        CHECK(!intent.consume());
    }
}

void aRejectedStartupRequestDoesNotPoisonAFutureExplicitRequest() {
    InitialProvisioningWindowIntent intent;
    CHECK(intent.request(false, true, false).code() == ErrorCode::StateConflict);
    CHECK(intent.request(true, false, false).code() == ErrorCode::StateConflict);
    CHECK(intent.request(true, true, true).code() == ErrorCode::StateConflict);
    CHECK(intent.request(true, true, false).ok());
    CHECK(intent.consume());
}

void aPendingIntentCannotBypassTheCurrentStartupBoundary() {
    InitialProvisioningWindowIntent intent;
    CHECK(intent.request(true, true, false).ok());
    CHECK(intent.request(false, true, false).code() == ErrorCode::StateConflict);
    CHECK(intent.request(true, false, false).code() == ErrorCode::StateConflict);
    CHECK(intent.request(true, true, true).code() == ErrorCode::StateConflict);
    CHECK(intent.consume());
    CHECK(!intent.consume());
}

void consumedIntentCannotBeRearmed() {
    InitialProvisioningWindowIntent intent;
    CHECK(intent.request(true, true, false).ok());
    CHECK(intent.consume());
    CHECK(intent.request(true, true, false).code() == ErrorCode::StateConflict);
    CHECK(!intent.consume());
}

void anOrdinaryStartupCannotBeTurnedIntoALaterMaintenanceRequest() {
    InitialProvisioningWindowIntent intent;
    CHECK(!intent.consume());
    CHECK(intent.request(true, true, false).code() == ErrorCode::StateConflict);
    CHECK(!intent.consume());
}

} // namespace

int main() {
    defaultBootHasNoMaintenanceWindow();
    explicitStartupRequestIsConsumedExactlyOnce();
    duplicatePendingRequestDoesNotReplaceOrConsumeTheIntent();
    allInvalidStartupStatesAreRejectedWithoutCreatingAnIntent();
    aRejectedStartupRequestDoesNotPoisonAFutureExplicitRequest();
    aPendingIntentCannotBypassTheCurrentStartupBoundary();
    consumedIntentCannotBeRearmed();
    anOrdinaryStartupCannotBeTurnedIntoALaterMaintenanceRequest();
    std::cout << "Initial provisioning window intent: 8 cases passed" << std::endl;
    return EXIT_SUCCESS;
}
