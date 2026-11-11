#include <BlinkerV2/arduino/ports/esp32/Esp32WiFiProvSecurity1Owner.h>

#include <cstdlib>
#include <iostream>

namespace {
using namespace blinker;
using Kind = Esp32WiFiProvRekeyEndpointKind;

#define CHECK(expr) do { if (!(expr)) { std::cerr << "Owner check: " << __LINE__ << '\n'; std::exit(1); } } while (false)

Esp32WiFiProvSecurity1Owner nativeOwner;
int realHandle = 0;
bool decryptOk = true;
bool onCustomCalled = false;
bool openOk = true;
Esp32WiFiProvRekeyNativeOwner observed{};

esp_err_t realOpen(void*, uint32_t) { return openOk ? ESP_OK : ESP_ERR_INVALID_STATE; }
esp_err_t realClose(void*, uint32_t) { return ESP_OK; }
esp_err_t realHandshake(void*, const void*, uint32_t, const uint8_t*, ssize_t, uint8_t**, ssize_t*, void*) { return ESP_OK; }
esp_err_t realDecrypt(void*, uint32_t, const uint8_t*, ssize_t, uint8_t**, ssize_t*) {
    return decryptOk ? ESP_OK : ESP_ERR_INVALID_STATE;
}

struct Binding {
    static Esp32WiFiProvSecurity1Owner* owner() { return &nativeOwner; }
    static const protocomm_security_t& originalSecurity1() {
        static const protocomm_security_t real{1, 2U, nullptr, nullptr, &realOpen, &realClose, &realHandshake, nullptr, &realDecrypt};
        return real;
    }
};

using Hook = Esp32WiFiProvSecurity1OwnerHook<Binding>;
const protocomm_security_t security = Hook::wrap();

esp_err_t dispatch(void*, const char* name, uint32_t session, const uint8_t*, ssize_t, uint8_t**, ssize_t*) {
    const auto kind = Hook::endpoint(name);
    CHECK(!nativeOwner.current(kind, session, observed));
    const esp_err_t result = security.decrypt(&realHandle, session, nullptr, 0, nullptr, nullptr);
    if (result == ESP_OK && kind != Kind::None) {
        CHECK(nativeOwner.current(kind, session, observed));
        CHECK(!nativeOwner.current(kind == Kind::SdkConfig ? Kind::BlinkerConfig : Kind::SdkConfig, session, observed));
        onCustomCalled = true;
    }
    return result;
}

esp_err_t request(const char* name, uint32_t session) {
    void* context = nullptr;
    return Hook::request(&dispatch, context, name, session, nullptr, 0, nullptr, nullptr);
}

} // namespace

int main() {
    const uint8_t pass = 1U;
    const protocomm_security1_params_t pop{&pass, 1U};
    const protocomm_security1_params_t empty{&pass, 0U};
    CHECK(security.ver == 1 && security.patch_ver == 2U);
    nativeOwner.arm();
    openOk = false;
    CHECK(security.new_transport_session(&realHandle, 0U) != ESP_OK);
    CHECK(request("blinker-config", 0U) != ESP_OK);
    openOk = true;
    CHECK(security.new_transport_session(&realHandle, 0U) == ESP_OK);
    CHECK(request("blinker-config", 0U) != ESP_OK);
    CHECK(security.security_req_handler(&realHandle, &empty, 0U, nullptr, 0, nullptr, nullptr, nullptr) == ESP_OK);
    CHECK(request("blinker-config", 0U) != ESP_OK);
    CHECK(security.security_req_handler(&realHandle, &pop, 0U, nullptr, 0, nullptr, nullptr, nullptr) == ESP_OK);
    CHECK(request("blinker-config", 1U) != ESP_OK);
    decryptOk = false;
    CHECK(request("blinker-config", 0U) != ESP_OK && !onCustomCalled);
    decryptOk = true;
    CHECK(request("prov-scan", 0U) == ESP_OK && !onCustomCalled);
    CHECK(request("blinker-config", 0U) == ESP_OK && onCustomCalled);
    const uint32_t first = observed.generation;
    CHECK(observed.session == 0U && first > 0U);
    CHECK(!nativeOwner.current(Kind::BlinkerConfig, 0U, observed));
    CHECK(request("prov-config", 0U) == ESP_OK && observed.generation == first);
    CHECK(security.security_req_handler(&realHandle, &pop, 0U, nullptr, 0, nullptr, nullptr, nullptr) == ESP_OK);
    CHECK(request("blinker-config", 0U) == ESP_OK && observed.generation > first);
    const uint32_t second = observed.generation;
    CHECK(security.close_transport_session(&realHandle, 0U) == ESP_OK);
    CHECK(request("prov-config", 0U) != ESP_OK);
    CHECK(security.new_transport_session(&realHandle, 0U) == ESP_OK); // SDK may reuse the numeric id.
    CHECK(request("prov-config", 0U) != ESP_OK);
    CHECK(security.security_req_handler(&realHandle, &pop, 0U, nullptr, 0, nullptr, nullptr, nullptr) == ESP_OK);
    CHECK(request("prov-config", 0U) == ESP_OK && observed.generation > second);
    nativeOwner.disarm();
    CHECK(request("prov-config", 0U) != ESP_OK);
}
