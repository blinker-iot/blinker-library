// Compile/link probe, not a public gateway example or a hardware test.
// Uses the genuine protected-NVS/TLS product; no storage/security override.
#include <BlinkerV2Advanced.h>
#include <BlinkerV2/api/Product.h>
#include <BlinkerV2/arduino/platform/esp32/Esp32EdgeHubProduct.h>

#if defined(BLINKER_TEST_ENABLE_TIME)
#include <BlinkerV2/runtime/TimeSyncService.h>
blinker::TimeSyncService businessTime;
#endif

#if defined(BLINKER_TEST_CHILD_TIME)
#include <BlinkerV2/runtime/GatewayTimeResponder.h>
blinker::GatewayTimeResponder firstChildTime(businessTime);
blinker::GatewayTimeResponder secondChildTime(businessTime);
#endif

BLINKER_PROPERTY(hubReady, bool, blinker::readOnly());
blinker::DeviceSchema<decltype(hubReady)> schema(hubReady);
blinker::Device device(schema);
blinker::integration::official_detail::Esp32EdgeHubProduct lifecycle;
blinker::Product product(device, lifecycle);

void setup() {
#if defined(BLINKER_TEST_CHILD_TIME)
    if (!lifecycle.setChildRequestHandler(0U, &firstChildTime) ||
        !lifecycle.setChildRequestHandler(1U, &secondChildTime)) return;
#endif
    if (!lifecycle.configureProvisioning()) return;
#if defined(BLINKER_TEST_ENABLE_TIME)
    product.begin(&businessTime);
#else
    product.begin();
#endif
}

void loop() { product.run(); }
