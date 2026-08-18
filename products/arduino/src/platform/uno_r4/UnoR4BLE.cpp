#include "BleProduct.h"
#include "UnoR4BLEPlatform.h"
#include <BlinkerV2/api/BlinkerFacade.h>

namespace blinker {
namespace integration {

IProductLifecycle& lifecycle(BleTag) {
    static official_detail::PlatformBleOnlyComposition<
        official_detail::RenesasUnoPlatformBlePlatform>
        instance;
    return instance;
}

} // namespace integration
} // namespace blinker
