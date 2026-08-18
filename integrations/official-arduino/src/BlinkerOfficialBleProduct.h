#ifndef BLINKER_OFFICIAL_BLE_PRODUCT_H
#define BLINKER_OFFICIAL_BLE_PRODUCT_H

#include <BlinkerV2/interface/IProductLifecycle.h>

namespace blinker {
namespace integration {
namespace official_detail {

class ILocalBleComposition : public IProductLifecycle {
public:
    virtual Result openPairingMode() = 0;
    virtual Result cancelPairingMode() = 0;
    virtual Result confirmPhysicalPresence() = 0;
};

} // namespace official_detail
} // namespace integration
} // namespace blinker

#endif
