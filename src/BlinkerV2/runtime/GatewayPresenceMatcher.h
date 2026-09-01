#ifndef BLINKER_RUNTIME_GATEWAYPRESENCEMATCHER_H
#define BLINKER_RUNTIME_GATEWAYPRESENCEMATCHER_H

#include "../identity/GatewayAccessStore.h"
#include "../interface/IBleCentralPort.h"
#include "../protocol/ble/Presence.h"

namespace blinker {

// Exact single-child discovery matcher. It holds only the durable identity
// binding and the bounded active+pending locators; MAC and device name are
// never inputs. Pending is tried first so a staged PresenceKey can be proven
// without losing the active recovery path.
class GatewayPresenceMatcher {
public:
    GatewayPresenceMatcher();
    ~GatewayPresenceMatcher();

    Result configure(const GatewayAccessRecord& access);
    void clear();
    bool match(const BleCentralAdvertisement& advertisement);

    // Bearer-specific discovery stays behind the Selector seam. The native
    // secure session invokes this operation without depending on BLE types.
    template <typename Link>
    Result connect(Link& link) {
        if (!configured_) {
            return Result::failure(ErrorCode::NotConfigured);
        }
        return link.connect(&GatewayPresenceMatcher::matchThunk, this);
    }

    bool configured() const { return configured_; }
    bool matchedPending() const { return matchedPending_; }
    uint32_t matchedPresenceVersion() const { return matchedVersion_; }

    static bool matchThunk(
        void* context,
        const BleCentralAdvertisement& advertisement);

private:
    bool matchLocator(
        const GatewayPresenceLocator& locator,
        ByteView advertisedLocator) const;

    DeviceInstanceId child_;
    GatewayPresenceLocator active_;
    GatewayPresenceLocator pending_;
    uint32_t accessEpoch_;
    uint32_t matchedVersion_;
    bool configured_;
    bool matchedPending_;
};

} // namespace blinker

#endif
