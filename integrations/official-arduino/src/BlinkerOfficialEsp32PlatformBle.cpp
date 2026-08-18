#include "BlinkerOfficialBleOnlyComposition.h"
#include "BlinkerOfficialEsp32Ble.h"
#include "BlinkerOfficialEsp32BlePlatform.h"

namespace blinker {
namespace integration {
namespace official_detail {

IProductLifecycle& esp32PlatformBleLifecycle() {
    static PlatformBleOnlyComposition<Esp32PlatformBlePlatform> instance;
    return instance;
}

} // namespace official_detail
} // namespace integration
} // namespace blinker
