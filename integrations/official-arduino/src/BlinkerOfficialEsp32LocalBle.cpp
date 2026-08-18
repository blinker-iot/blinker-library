#include "BlinkerOfficialBleOnlyComposition.h"
#include "BlinkerOfficialEsp32Ble.h"
#include "BlinkerOfficialEsp32BlePlatform.h"

namespace blinker {
namespace integration {
namespace official_detail {

ILocalBleComposition& esp32LocalBleLifecycle() {
    static LocalBleComposition<Esp32LocalBlePlatform> instance;
    return instance;
}

} // namespace official_detail
} // namespace integration
} // namespace blinker
