// An ordinary library TU has no Sketch product selection or separate facade.
#include "BlinkerV2/api/BlinkerFacade.h"

#if defined(BLINKER_PRODUCT_WIFI) || defined(BLINKER_PRODUCT_BLE) || defined(BLINKER_PRODUCT_WIFI_BLE)
#error "Location peer must not inherit a Sketch product macro"
#endif

bool observeLocationFromCoreTu(blinker::Location& target, const blinker::LocationFix& observation,
    uint32_t sampledAtMillis) {
    return Blinker.location(target, observation, sampledAtMillis);
}
size_t locationSizeFromCoreTu() { return sizeof(blinker::Location); }
