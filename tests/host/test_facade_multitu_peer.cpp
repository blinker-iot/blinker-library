// Deliberately model a library implementation TU: no public product entry and
// no build-wide product macro. The facade must be the same type and instance.
#include "BlinkerV2/api/BlinkerFacade.h"

#if defined(BLINKER_PRODUCT_WIFI) || defined(BLINKER_PRODUCT_BLE) || \
    defined(BLINKER_PRODUCT_WIFI_BLE)
#error "The multi-TU peer must not inherit Sketch product selection"
#endif

blinker::BlinkerClass& facadeFromCoreTranslationUnit() { return Blinker; }
size_t facadeSizeFromCoreTranslationUnit() { return sizeof(blinker::BlinkerClass); }
bool facadeStartedFromCoreTranslationUnit() { return Blinker.status().started; }
blinker::ErrorCode facadeErrorFromCoreTranslationUnit() { return Blinker.lastError(); }
void facadePollFromCoreTranslationUnit(uint32_t budget) { Blinker.run(budget); }
void facadeEndFromCoreTranslationUnit() { Blinker.end(); }
