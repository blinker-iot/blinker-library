#include <BlinkerV2/api/BlinkerFacade.h>

#if defined(BLINKER_PRODUCT_WIFI) || defined(BLINKER_PRODUCT_BLE) || \
    defined(BLINKER_PRODUCT_WIFI_BLE)
#error "The ordinary facade peer must not require a Sketch product macro"
#endif

bool publicFacadePeerMatches(const blinker::BlinkerClass* selected) {
    return selected == &Blinker;
}
