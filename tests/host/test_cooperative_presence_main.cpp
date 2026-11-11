// Reuse the original in-memory atomic Store and root fixtures without rerunning
// their suite. The regression keeps the exact durable Store implementation.
#define main device_access_store_suite_main
#include "test_device_access_store_main.cpp"
#undef main
#include <BlinkerV2/provisioning/ControllerControlCoordinator.h>

namespace {
class CooperativePresenceObserver final : public blinker::IDeviceAccessChangeObserver {
public:
    CooperativePresenceObserver()
        : ownerHeld(true), admitted(false), refuse(false), poisoned(false),
          ordinaryCalls(0U), cooperativeCalls(0U), completedCalls(0U),
          outcome(blinker::ErrorCode::Ok) {}
    blinker::Result beginDeviceAccessChange() override {
        ++ordinaryCalls;
        return blinker::Result::failure(blinker::ErrorCode::WouldBlock);
    }
    void endDeviceAccessChange(blinker::Result) override { CHECK_TRUE(false); }
    blinker::Result beginCooperativePresenceChange() override {
        ++cooperativeCalls;
        CHECK_TRUE(ownerHeld && !admitted);
        if (refuse || poisoned)
            return blinker::Result::failure(blinker::ErrorCode::WouldBlock);
        admitted = true;
        return blinker::Result::success();
    }
    void endCooperativePresenceChange(blinker::Result result) override {
        CHECK_TRUE(ownerHeld && admitted);
        admitted = false;
        ++completedCalls;
        outcome = result.code();
        if (!result) poisoned = true;
    }
    bool ownerHeld, admitted, refuse, poisoned;
    uint32_t ordinaryCalls, cooperativeCalls, completedCalls;
    blinker::ErrorCode outcome;
};

class RejectSignature final : public blinker::IServerSignatureVerifier {
public:
    blinker::Result verifyDigest(uint32_t, blinker::ServerSignatureAlgorithm,
                                blinker::ByteView, blinker::ByteView) override {
        return blinker::Result::failure(blinker::ErrorCode::AuthenticationRequired);
    }
};

void defaultFenceAndCasRemainUnchanged() {
    MemoryStore memory;
    blinker::DeviceAccessStore store(memory);
    CHECK_TRUE(store.bootstrap(deviceKey(1U), 1U,
        credential(0x10U, 0x20U, 1U, 1U, blinker::kControllerPermissionAll)));
    ChangeObserver observer;
    store.setChangeObserver(&observer);
    memory.fence = &observer.held;
    blinker::IDeviceAccessStore& port = store;
    CHECK_TRUE(port.replacePresenceVerifiedCooperatively(1U, 0U, presenceKey(1U, 0x40U)));
    CHECK_EQ(1U, observer.before);
    CHECK_EQ(1U, observer.after);
    const uint32_t writes = memory.replaceCalls;
    CHECK_TRUE(port.replacePresenceVerifiedCooperatively(1U, 0U, presenceKey(1U, 0x40U)));
    CHECK_EQ(writes, memory.replaceCalls);
    CHECK_EQ(2U, observer.before);
    CHECK_EQ(2U, observer.after);
    CHECK_EQ(blinker::ErrorCode::StateConflict,
        port.replacePresenceVerifiedCooperatively(2U, 1U, presenceKey(2U, 0x50U)).code());
    CHECK_EQ(blinker::ErrorCode::StateConflict,
        port.replacePresenceVerifiedCooperatively(1U, 0U, presenceKey(2U, 0x50U)).code());
    CHECK_EQ(writes, memory.replaceCalls);
    store.setChangeObserver(nullptr);
    memory.fence = nullptr;
}

void cooperativePresenceNeverUnlocksOwnerOrAdmitsOrdinaryWrites() {
    MemoryStore memory;
    blinker::DeviceAccessStore store(memory);
    CHECK_TRUE(store.bootstrap(deviceKey(1U), 1U,
        credential(0x10U, 0x20U, 1U, 1U, blinker::kControllerPermissionAll)));
    CooperativePresenceObserver observer;
    store.setChangeObserver(&observer);
    memory.fence = &observer.ownerHeld;
    const uint32_t writes = memory.replaceCalls;
    CHECK_EQ(blinker::ErrorCode::WouldBlock,
        store.replacePresenceVerified(1U, 0U, presenceKey(1U, 0x40U)).code());
    CHECK_EQ(writes, memory.replaceCalls);
    CHECK_TRUE(store.replacePresenceVerifiedCooperatively(1U, 0U, presenceKey(1U, 0x40U)));
    CHECK_TRUE(observer.ownerHeld && !observer.admitted);
    CHECK_EQ(1U, observer.cooperativeCalls);
    CHECK_EQ(1U, observer.completedCalls);
    CHECK_EQ(blinker::ErrorCode::WouldBlock, store.replace(deviceKey(2U)).code());
    CHECK_EQ(2U, observer.ordinaryCalls);
    observer.refuse = true;
    CHECK_EQ(blinker::ErrorCode::WouldBlock,
        store.replacePresenceVerifiedCooperatively(1U, 1U, presenceKey(2U, 0x50U)).code());
    CHECK_EQ(1U, observer.completedCalls);
    observer.refuse = false;
    memory.failAfterReplace = true;
    CHECK_EQ(blinker::ErrorCode::WouldBlock,
        store.replacePresenceVerifiedCooperatively(1U, 1U, presenceKey(2U, 0x50U)).code());
    CHECK_TRUE(observer.ownerHeld && observer.poisoned && !observer.admitted);
    CHECK_EQ(2U, observer.completedCalls);
    CHECK_EQ(blinker::ErrorCode::WouldBlock, observer.outcome);
    // The same handle exposes the attempted bytes despite unknown durability.
    // An exact retry must consult the poison fence before returning a receipt.
    const uint32_t unknownWrites = memory.replaceCalls;
    CHECK_EQ(blinker::ErrorCode::WouldBlock,
        store.replacePresenceVerifiedCooperatively(1U, 1U, presenceKey(2U, 0x50U)).code());
    CHECK_EQ(unknownWrites, memory.replaceCalls);
    CHECK_EQ(4U, observer.cooperativeCalls);
    CHECK_EQ(2U, observer.completedCalls);
    store.setChangeObserver(nullptr);
    memory.fence = nullptr;
}

void presenceCoordinatorUsesOnlyCooperativeMutation() {
    MemoryStore memory;
    blinker::DeviceAccessStore store(memory);
    CHECK_TRUE(store.bootstrap(deviceKey(1U), 1U,
        credential(0x10U, 0x20U, 1U, 1U, blinker::kControllerPermissionAll)));
    CooperativePresenceObserver observer;
    store.setChangeObserver(&observer);
    memory.fence = &observer.ownerHeld;
    blinker::DeviceInstanceId instance;
    for (size_t i = 0U; i < sizeof(instance.bytes); ++i) instance.bytes[i] = static_cast<uint8_t>(i + 1U);
    RejectSignature signatures;
    blinker::ControllerGrantVerifier verifier(signatures);
    blinker::ControllerControlCoordinator coordinator(instance, store, store, verifier, &store);
    blinker::PresenceKeyMutation mutation;
    mutation.accessEpoch = 1U;
    mutation.expectedVersion = 0U;
    mutation.presence = presenceKey(1U, 0x40U);
    uint8_t encodedStorage[blinker::kPresenceKeyMutationMaxEncodedSize] = {};
    blinker::ByteView encoded;
    CHECK_TRUE(blinker::encodePresenceKeyMutation(mutation,
        blinker::MutableByteSpan(encodedStorage, sizeof(encodedStorage)), encoded));
    uint8_t output[blinker::kPresenceKeyReceiptMaxEncodedSize] = {};
    blinker::ByteView receipt;
    CHECK_TRUE(coordinator.applyPresence(encoded,
        blinker::MutableByteSpan(output, sizeof(output)), receipt));
    CHECK_EQ(0U, observer.ordinaryCalls);
    CHECK_EQ(1U, observer.cooperativeCalls);
    CHECK_EQ(1U, observer.completedCalls);
    blinker::PresenceKeyReceipt decoded;
    CHECK_TRUE(blinker::decodePresenceKeyReceipt(receipt, decoded));
    CHECK_TRUE(blinker::verifyPresenceKeyReceiptProof(mutation.presence, decoded));
    blinker::DevicePresenceKey installed;
    CHECK_TRUE(store.loadPresence(1U, installed));
    CHECK_TRUE(blinker::sameDevicePresenceKey(installed, mutation.presence));
    CHECK_TRUE(observer.ownerHeld && !observer.admitted);
    store.setChangeObserver(nullptr);
    memory.fence = nullptr;
}
} // namespace

int main() {
    defaultFenceAndCasRemainUnchanged();
    cooperativePresenceNeverUnlocksOwnerOrAdmitsOrdinaryWrites();
    presenceCoordinatorUsesOnlyCooperativeMutation();
    std::cout << "Cooperative Presence fence, CAS, exclusion, uncertainty and receipt checks passed" << std::endl;
}
