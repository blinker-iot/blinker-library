#ifndef BLINKER_ESP32_WIFIPROV_ADAPTER_H
#define BLINKER_ESP32_WIFIPROV_ADAPTER_H

#if !defined(ARDUINO_ARCH_ESP32)
#error "BlinkerEsp32WifiProvAdapter requires an ESP32 Arduino target"
#endif

#include <BlinkerV2/interface/IWifiCredentialSink.h>
#include <BlinkerV2/provisioning/DeviceKeyProvisioningEndpoint.h>

#include <network_provisioning/manager.h>

#include <stdint.h>

namespace blinker {

enum class Esp32WifiProvState : uint8_t {
    Stopped = 0U,
    Running,
    Complete,
    Fault
};

struct Esp32WifiProvConfig {
    typedef esp_err_t (*Prepare)(const Esp32WifiProvConfig& config);

    const network_prov_scheme_t* scheme;
    network_prov_event_handler_t schemeEventHandler;
    Prepare prepare;
    const char* serviceName;
    const char* proofOfPossession;
    const char* serviceKey;
    uint8_t* serviceUuid;

    Esp32WifiProvConfig()
        : scheme(nullptr),
          schemeEventHandler(),
          prepare(nullptr),
          serviceName(nullptr),
          proofOfPossession(nullptr),
          serviceKey(nullptr),
          serviceUuid(nullptr) {}
};

// Scheme factories live in separate translation units. Arduino's archive
// linker can therefore select SoftAP without pulling the BLE provisioning
// scheme (and its Bluedroid host) into the final firmware.
Esp32WifiProvConfig esp32BleWifiProvConfig(
    const char* serviceName,
    const char* proofOfPossession,
    uint8_t* serviceUuid = nullptr);
Esp32WifiProvConfig esp32SoftApWifiProvConfig(
    const char* serviceName,
    const char* proofOfPossession,
    const char* serviceKey);

// Thin lifecycle around Espressif Network Provisioning. Wi-Fi credentials are
// copied once into the injected Blinker journal; the ESP32 WiFi driver remains
// in RAM-persistence mode so it does not create a second durable copy.
class Esp32WifiProvAdapter {
public:
    Esp32WifiProvAdapter(
        DeviceKeyProvisioningEndpoint& deviceKey,
        IWifiCredentialSink& wifiCredentials);
    ~Esp32WifiProvAdapter();

    Result begin(const Esp32WifiProvConfig& config);
    void poll();
    void end();

    Esp32WifiProvState state() const {
        return static_cast<Esp32WifiProvState>(state_);
    }
    ErrorCode lastError() const {
        return static_cast<ErrorCode>(lastError_);
    }
    bool deviceKeyInstalled() const { return keyInstalled_; }
    bool wifiCredentialsCommitted() const {
        return credentialsCommitted_;
    }

private:
    static void onEvent(
        void* context,
        network_prov_cb_event_t event,
        void* eventData);
    static esp_err_t onRequest(
        uint32_t sessionId,
        const uint8_t* input,
        ssize_t inputSize,
        uint8_t** output,
        ssize_t* outputSize,
        void* context);

    void handleEvent(
        network_prov_cb_event_t event,
        void* eventData);
    Result commitCredentials(const void* eventData);
    void fail(ErrorCode error);
    static ErrorCode mapPlatformError(int error);
    static bool validText(
        const char* value,
        size_t minimum,
        size_t maximum);

    DeviceKeyProvisioningEndpoint& deviceKey_;
    IWifiCredentialSink& wifiCredentials_;
    volatile uint8_t state_;
    volatile uint8_t lastError_;
    volatile bool keyInstalled_;
    volatile bool credentialsCommitted_;
    volatile bool wifiSucceeded_;
    volatile bool ended_;
    bool managerInitialized_;
    bool stopRequested_;

    Esp32WifiProvAdapter(const Esp32WifiProvAdapter&);
    Esp32WifiProvAdapter& operator=(const Esp32WifiProvAdapter&);
};

#if UINTPTR_MAX <= UINT32_MAX
static_assert(
    sizeof(Esp32WifiProvAdapter) <= 20U,
    "ESP32 WiFiProv adapter exceeds its 32-bit ABI gate");
#endif

} // namespace blinker

#endif
