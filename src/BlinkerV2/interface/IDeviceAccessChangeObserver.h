#ifndef BLINKER_INTERFACE_IDEVICEACCESSCHANGEOBSERVER_H
#define BLINKER_INTERFACE_IDEVICEACCESSCHANGEOBSERVER_H

#include "../core/Result.h"

namespace blinker {

// Optional product-owned fence around the actual atomic access-store commit.
// A callback can originate in WiFiProv: it must not refresh task roots, perform
// I/O, or wait for the cooperative owner. A busy owner rejects the mutation;
// it does not permit a write followed by delayed invalidation.
class IDeviceAccessChangeObserver {
public:
    virtual ~IDeviceAccessChangeObserver() {}
    virtual Result beginDeviceAccessChange() = 0;
    // Invoked after every admitted write attempt, including uncertain failure.
    // Failure is not rollback: the same handle can expose an uncommitted view.
    // Consumers must latch tasks closed until a fresh storage lifecycle; do not
    // reload that handle on the next tick and call the root durable. Success
    // permits deferred owner refresh only, never I/O from this callback.
    virtual void endDeviceAccessChange(Result outcome) = 0;

    // Only an explicitly wired synchronous PresenceKey control mutation may
    // borrow a product's current cooperative owner. The default is the same
    // exclusive fence as every ordinary/native root write, never reentrancy.
    virtual Result beginCooperativePresenceChange() {
        return beginDeviceAccessChange();
    }
    virtual void endCooperativePresenceChange(Result outcome) {
        endDeviceAccessChange(outcome);
    }

    // Internal first-Cloud-Key transaction only, never a native setup callback
    // or a substitute for authorization. The product must explicitly admit its
    // held owner and original TaskStore boundary; ordinary profiles stay closed.
    virtual Result beginCooperativeInitialCloudKeyChange() {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }
    virtual void endCooperativeInitialCloudKeyChange(Result outcome) {
        (void)outcome;
    }
};

} // namespace blinker
#endif
