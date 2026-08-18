#include "BlinkerOfficialRenesasUnoWifiPlatform.h"
#include "BlinkerOfficialWifiComposition.h"

namespace blinker {
namespace integration {

namespace {

typedef official_detail::WifiComposition<
    official_detail::RenesasUnoWifiPlatform>
    RenesasUnoWifiComposition;

RenesasUnoWifiComposition& composition() {
    static RenesasUnoWifiComposition instance;
    return instance;
}

} // namespace

IProductLifecycle& lifecycle(WiFiTag) {
    return composition();
}

IProductLifecycle& lifecycle(const WifiOnboardingProfile& profile) {
    RenesasUnoWifiComposition& selected = composition();
    selected.configure(profile);
    return selected;
}

} // namespace integration
} // namespace blinker
