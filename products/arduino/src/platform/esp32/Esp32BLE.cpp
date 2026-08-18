#include "BleProduct.h"
#include "Esp32BLEPlatform.h"
#include <BlinkerV2/api/BlinkerFacade.h>

namespace blinker {
namespace integration {

IProductLifecycle& lifecycle(BleTag) {
    static official_detail::PlatformBleOnlyComposition<
        official_detail::Esp32PlatformBlePlatform>
        instance;
    return instance;
}

} // namespace integration
} // namespace blinker
