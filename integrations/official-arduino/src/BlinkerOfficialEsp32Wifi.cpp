#include "BlinkerOfficialEsp32WifiPlatform.h"
#include "BlinkerOfficialWifiComposition.h"

namespace blinker {
namespace integration {
namespace {

typedef official_detail::WifiComposition<
    official_detail::Esp32WifiPlatform>
    Esp32WifiComposition;

Esp32WifiComposition& composition() {
    static Esp32WifiComposition instance;
    return instance;
}

} // namespace

IProductLifecycle& lifecycle(WiFiTag) {
    return composition();
}

IProductLifecycle& lifecycle(const WifiOnboardingProfile& profile) {
    Esp32WifiComposition& selected = composition();
    selected.configure(profile);
    return selected;
}

} // namespace integration
} // namespace blinker
