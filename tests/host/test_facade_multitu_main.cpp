#if defined(BLINKER_TEST_FACADE_WIFI_BLE)
// This is an entry/API linkage test, not a native ESP32 stack test. Supply the
// test lifecycle below instead of including an ESP32 SDK on the host.
#define ARDUINO_ARCH_ESP32 1
#define BLINKER_V2_ARDUINO_ESP32_WIFI_BLE_PRODUCT_H
#include "BlinkerWiFiBLE.h"
#elif defined(BLINKER_TEST_FACADE_BLE)
#include "BlinkerBLE.h"
#else
#include "BlinkerWiFi.h"
#endif

#include <cstdlib>
#include <iostream>

using namespace blinker;

BlinkerClass& facadeFromCoreTranslationUnit();
size_t facadeSizeFromCoreTranslationUnit();
bool facadeStartedFromCoreTranslationUnit();
ErrorCode facadeErrorFromCoreTranslationUnit();
void facadePollFromCoreTranslationUnit(uint32_t budget);
void facadeEndFromCoreTranslationUnit();

#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " << #x << '\n'; std::exit(1); } } while (0)

namespace {

template <typename... Args>
struct CanBegin {
    template <typename Facade>
    static auto check(int) -> decltype(
        std::declval<Facade&>().begin(std::declval<Args>()...), char());
    template <typename>
    static long check(...);
    enum { value = sizeof(check<BlinkerClass>(0)) == sizeof(char) };
};

struct Lifecycle : IProductLifecycle {
    unsigned starts = 0U;
    unsigned polls = 0U;
    unsigned stops = 0U;
    uint32_t budget = 0U;
    Result attach(Client&) override { return Result::success(); }
    Result start() override { ++starts; return Result::success(); }
    void poll(uint32_t selected) override { ++polls; budget = selected; }
    void stop() override { ++stops; }
    ProductLifecycleStatus status() const override {
        return ProductLifecycleStatus(ProductLifecycleState::Active);
    }
    ProductCapabilities capabilities() const override { return ProductCapabilities(); }
} testLifecycle;

BLINKER_PROPERTY(power, bool, readWrite());
BLINKER_PROPERTY(duty, uint32_t, readWrite());

typedef Property<bool>& PropertyArgument;
typedef StringView CredentialArgument;
typedef char (&StringArrayArgument)[4];

static_assert(!CanBegin<>::value, "An empty schema must not begin");
static_assert(!CanBegin<StringArrayArgument>::value, "A string is not an interaction");

#if defined(BLINKER_TEST_FACADE_BLE) || defined(BLINKER_TEST_FACADE_WIFI_BLE)
static_assert(CanBegin<PropertyArgument>::value, "BLE interaction begin rejected");
static_assert(!CanBegin<CredentialArgument, CredentialArgument, CredentialArgument,
                       PropertyArgument>::value, "BLE accepted WiFi credentials");
static_assert(!CanBegin<StringArrayArgument, StringArrayArgument, StringArrayArgument,
                       PropertyArgument>::value, "BLE accepted string credentials");
#else
static_assert(!CanBegin<PropertyArgument>::value, "WiFi accepted local-only begin");
static_assert(CanBegin<CredentialArgument, CredentialArgument, CredentialArgument,
                      PropertyArgument>::value, "WiFi credential begin rejected");
static_assert(CanBegin<StringArrayArgument, StringArrayArgument, StringArrayArgument,
                      PropertyArgument>::value, "Strings stole the WiFi begin overload");
#endif

} // namespace

namespace blinker { namespace integration {
#if defined(BLINKER_TEST_FACADE_WIFI_BLE)
IProductLifecycle& lifecycle(facade_detail::WifiBleProfile) { return testLifecycle; }
#elif defined(BLINKER_TEST_FACADE_BLE)
IProductLifecycle& lifecycle(facade_detail::BleProfile) { return testLifecycle; }
#else
IProductLifecycle& lifecycle(const WifiDeviceKeyProfile&) { return testLifecycle; }
#endif
} }

int main() {
    CHECK(&facadeFromCoreTranslationUnit() == &Blinker);
    CHECK(facadeSizeFromCoreTranslationUnit() == sizeof(BlinkerClass));
    CHECK(!facadeStartedFromCoreTranslationUnit());
    CHECK(!Blinker.enableTasks(power));
    CHECK(facadeErrorFromCoreTranslationUnit() == ErrorCode::UnsupportedFeature);
#if defined(BLINKER_TEST_FACADE_BLE) || defined(BLINKER_TEST_FACADE_WIFI_BLE)
    CHECK(Blinker.begin(power, duty));
#else
    CHECK(Blinker.begin("key", "ssid", "password", power, duty));
#endif
    CHECK(testLifecycle.starts == 1U && facadeStartedFromCoreTranslationUnit());
    CHECK(Blinker.report(power, true, duty, 25U));
    bool powerValue = false;
    uint32_t dutyValue = 0U;
    CHECK(power.current(powerValue) && powerValue);
    CHECK(duty.current(dutyValue) && dutyValue == 25U);
    facadePollFromCoreTranslationUnit(321U);
    CHECK(testLifecycle.polls == 1U && testLifecycle.budget == 321U);
    facadeEndFromCoreTranslationUnit();
    CHECK(testLifecycle.stops == 1U && !Blinker.status().started);
    CHECK(facadeErrorFromCoreTranslationUnit() == ErrorCode::Ok);
    std::cout << "facade multi-TU: shared global, entry selection, overload rejection and lifecycle passed\n";
}
