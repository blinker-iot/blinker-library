#include "BlinkerOfficialBleOnlyComposition.h"
#include "BlinkerOfficialRenesasUnoBle.h"
#include "BlinkerOfficialRenesasUnoBlePlatform.h"

namespace blinker {
namespace integration {
namespace official_detail {

ILocalBleComposition& renesasUnoLocalBleLifecycle() {
    static LocalBleComposition<RenesasUnoLocalBlePlatform> instance;
    return instance;
}

} // namespace official_detail
} // namespace integration
} // namespace blinker
