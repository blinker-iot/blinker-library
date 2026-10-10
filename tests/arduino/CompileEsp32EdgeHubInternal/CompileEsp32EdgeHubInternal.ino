#include <BlinkerV2Advanced.h>
#include <BlinkerV2/arduino/platform/esp32/Esp32EdgeHubProduct.h>
#include <BlinkerV2/runtime/GatewayExecutionLease.h>

// Compile the portable execution state on the actual MCU ABI without enabling
// demand scheduling or allocating an additional production lease instance.
static_assert(sizeof(blinker::GatewayExecutionLease) == 40U, "Execution lease ABI budget");

// Object-file size witnesses for the MCU ABI (nm -S). Unreferenced readonly
// sections are discarded by the linker; no runtime RAM/probe logic is added.
extern "C" const uint8_t blinker_size_shared_route[sizeof(blinker::GatewayRouteClient)] = {};
extern "C" const uint8_t blinker_size_child_ingress[sizeof(blinker::GatewayRouteEndpoint)] = {};
extern "C" const uint8_t blinker_size_gatt_child[sizeof(blinker::BasicGatewayGattChildContext<>)] = {};
extern "C" const uint8_t blinker_size_child_executor[sizeof(blinker::BasicGatewayGattChildContext<>::Executor)] = {};
extern "C" const uint8_t blinker_size_child_records[sizeof(blinker::GatewayChildRecords)] = {};
extern "C" const uint8_t blinker_size_child_directory[sizeof(blinker::GatewayChildDirectory)] = {};
extern "C" const uint8_t blinker_size_management_router[sizeof(blinker::GatewayChildManagementRouter)] = {};
extern "C" const uint8_t blinker_size_binding[sizeof(blinker::GatewayChildBinding)] = {};
extern "C" const uint8_t blinker_size_gateway_storage[sizeof(blinker::BasicEsp32GatewayStorage<>)] = {};
extern "C" const uint8_t blinker_size_central_host[sizeof(blinker::Esp32NimBleCentralHost)] = {};
extern "C" const uint8_t blinker_size_central_port[sizeof(blinker::Esp32NimBleCentralPort)] = {};
extern "C" const uint8_t blinker_size_gatt_pool[sizeof(blinker::integration::official_detail::Esp32EdgeHubProduct::GattPool)] = {};
extern "C" const uint8_t blinker_size_gatt_radio[sizeof(blinker::integration::official_detail::Esp32EdgeHubProduct::ChildRadio)] = {};
extern "C" const uint8_t blinker_size_gatt_boot_guard[sizeof(blinker::GatewayGattBootGuard)] = {};
// Target-specific SDK pool admission, not a runtime multi-child readiness claim.
static_assert(blinker::Esp32NimBleCentralHost::capacity == 2U, "This two-slot probe needs an SDK with two connections");

namespace {

blinker::EndpointDescriptor descriptor(
    blinker::StringView("power"),
    blinker::EndpointKind::Property,
    blinker::ValueType::Boolean,
    static_cast<uint8_t>(blinker::AccessRead | blinker::AccessNotify));
blinker::ClientStorage<1U, 256U, 64U, 8U> storage;

blinker::DeviceRuntimeConfig runtimeConfig() {
    blinker::DeviceRuntimeConfig config;
    config.maxFrameSize = BLINKER_DEVICE_FRAME_SIZE;
    config.maxReassemblySize = BLINKER_DEVICE_FRAME_SIZE;
    return config;
}

blinker::Client client(
    storage.buffers(&descriptor, 1U),
    runtimeConfig());
blinker::integration::official_detail::Esp32EdgeHubProduct edgeHub;

} // namespace

void setup() {
    (void)edgeHub.configureProvisioning();
    (void)edgeHub.attach(client);
    (void)edgeHub.start();
}

void loop() {
    edgeHub.poll(1000U);
}
