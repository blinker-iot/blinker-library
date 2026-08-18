#include <BlinkerV2/api/Client.h>
#include <BlinkerOfficialDeviceKeyWifi.h>

namespace {

const blinker::EndpointDescriptor kFields[] = {
    blinker::EndpointDescriptor(
        blinker::StringView("online"),
        blinker::EndpointKind::Property,
        blinker::ValueType::Boolean,
        static_cast<uint8_t>(
            blinker::AccessRead | blinker::AccessNotify))
};

blinker::DeviceRuntimeConfig runtimeConfig() {
    blinker::DeviceRuntimeConfig config;
    config.maxFrameSize = 400U;
    config.maxReassemblySize = 400U;
    return config;
}

blinker::ClientStorage<1U, 400U, 96U, 8U> storage;
blinker::Client device(storage.buffers(kFields), runtimeConfig());
blinker::IProductLifecycle* product = nullptr;
bool started = false;
uint32_t nextStatusAt = 0U;

} // namespace

void setup() {
    Serial.begin(115200);
    product = &blinker::integration::official_detail::
        esp32DeviceKeyWifiLifecycle(
        blinker::StringView("YOUR_WIFI_SSID"),
        blinker::StringView("YOUR_WIFI_PASSWORD"),
        blinker::StringView(
            "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8"));

    blinker::Result result = product->attach(device);
    if (result) result = product->start();
    started = result.ok();
    if (!started) {
        Serial.print("Blinker start failed: ");
        Serial.println(static_cast<unsigned>(result.code()));
    }
}

void loop() {
    if (!started) return;
    product->poll(5000U);
    const uint32_t now = millis();
    if (static_cast<int32_t>(now - nextStatusAt) >= 0) {
        const blinker::ProductLifecycleStatus status = product->status();
        Serial.print("Blinker state/error/network: ");
        Serial.print(static_cast<unsigned>(status.state));
        Serial.print('/');
        Serial.print(static_cast<unsigned>(status.lastError));
        Serial.print('/');
        Serial.println(status.networkReady ? 1 : 0);
        nextStatusAt = now + 2000U;
    }
}
