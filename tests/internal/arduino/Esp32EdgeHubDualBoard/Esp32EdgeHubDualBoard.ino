#include "../../../local/Esp32LocalGateway.h"
#if __has_include("../../../local/Esp32EdgeHubWifiUpdateConfig.h")
#include "../../../local/Esp32EdgeHubWifiUpdateConfig.h"
#endif
#include <BlinkerV2Advanced.h>
#include <BlinkerV2/arduino/ports/esp32/Esp32Storage.h>
#include <BlinkerV2/identity/GatewayCredentialRenewalStore.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <esp_heap_caps.h>
#include <esp_system.h>
#include <host/ble_att.h>
#include <host/ble_gap.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>
#if defined(BLINKER_TEST_ENABLE_TIME)
#include "../../support/ObservedTime.h"
#include <BlinkerV2/runtime/GatewayTimeResponder.h>
#endif
#if defined(BLINKER_TEST_NATIVE_RADIO)
#include "NativeRadioProbe.h"
#endif

namespace {

bool revocationCommitResetArmed = false;
bool renewalReceiptResetArmed = false;
WiFiClientSecure* mqttFaultNetwork = nullptr;
uint32_t mqttDisconnectDeadline = 0U;

unsigned int taskStackMinimum(const char* name) {
    const TaskHandle_t task = xTaskGetHandle(name);
    return task == nullptr
               ? 0U
               : static_cast<unsigned int>(
                     uxTaskGetStackHighWaterMark(task));
}

// Deliberately lies about at-rest protection so a disposable development
// board can exercise receipt -> proof -> Active without an encrypted NVS
// partition. It is local to this internal probe and cannot be selected by the
// published Edge Hub product alias.
class UnsafePlainEncryptedBlob
    : public blinker::Esp32NvsAtomicBlobStore {
public:
    explicit UnsafePlainEncryptedBlob(
        const blinker::Esp32NvsBlobStoreConfig& config)
        : blinker::Esp32NvsAtomicBlobStore(config) {}

    blinker::StorageProtection protection() const override {
        return blinker::StorageProtection::EncryptedFlash;
    }
};

class ResettableGatewayAccessBlob final
    : public UnsafePlainEncryptedBlob {
public:
    explicit ResettableGatewayAccessBlob(
        const blinker::Esp32NvsBlobStoreConfig& config)
        : UnsafePlainEncryptedBlob(config) {}

    blinker::Result replace(blinker::ByteView value) override {
        static const uint8_t revocationMagic[4] = {'B', 'G', 'R', '1'};
        if (revocationCommitResetArmed && value.data != nullptr &&
            value.size >= sizeof(revocationMagic) &&
            memcmp(value.data, revocationMagic, sizeof(revocationMagic)) == 0) {
            revocationCommitResetArmed = false;
            Serial.println("fault=pre_tombstone_reset");
            Serial.flush();
            delay(50U);
            esp_restart();
            return blinker::Result::failure(
                blinker::ErrorCode::InternalError);
        }
        return blinker::Esp32NvsAtomicBlobStore::replace(value);
    }
};

class ResettableGatewayRenewalBlob final
    : public UnsafePlainEncryptedBlob {
public:
    explicit ResettableGatewayRenewalBlob(
        const blinker::Esp32NvsBlobStoreConfig& config)
        : UnsafePlainEncryptedBlob(config) {}

    blinker::Result replace(blinker::ByteView value) override {
        blinker::GatewayCredentialRenewalRecord record;
        const blinker::Result decoded =
            blinker::GatewayCredentialRenewalStore::decode(value, record);
        const bool reset = decoded && renewalReceiptResetArmed &&
            record.state ==
                blinker::GatewayCredentialRenewalState::ReceiptStored;
        blinker::clearGatewayCredentialRenewalRecord(record);
        if (reset) {
            renewalReceiptResetArmed = false;
            Serial.println("fault=pre_receipt_store_reset");
            Serial.flush();
            delay(50U);
            esp_restart();
            return blinker::Result::failure(
                blinker::ErrorCode::InternalError);
        }
        return blinker::Esp32NvsAtomicBlobStore::replace(value);
    }
};

} // namespace

#include <BlinkerV2/arduino/platform/esp32/Esp32EdgeHubProduct.h>
#if defined(BLINKER_TEST_INGRESS_CAPACITY)
#include "IngressCapacityProbe.h"
#endif

namespace {

blinker::EndpointDescriptor descriptor(
    blinker::StringView("hubReady"),
    blinker::EndpointKind::Property,
    blinker::ValueType::Boolean,
    static_cast<uint8_t>(blinker::AccessRead | blinker::AccessNotify));
blinker::ClientStorage<1U, 256U, 64U, 8U> storage;

blinker::DeviceRuntimeConfig runtimeConfig() {
    blinker::DeviceRuntimeConfig config;
    // F4 engineering gate: this exact composition owns the complete radio
    // policy. Do not advertise from ordinary devices or the synthetic child.
    config.protocolFeatures |= blinker::bbp2::FeatureGatewayDirectYield;
    config.maxFrameSize = BLINKER_DEVICE_FRAME_SIZE;
    config.maxReassemblySize = BLINKER_DEVICE_FRAME_SIZE;
    return config;
}

blinker::Client client(
    storage.buffers(&descriptor, 1U),
    runtimeConfig());

// This explicit weak-storage type exists only to exercise the complete
// receipt -> proof -> Active flow on disposable development boards. The
// production Esp32EdgeHubProduct remains bound to protected NVS.
typedef blinker::integration::official_detail::
    BasicEsp32EdgeHubPlatform<
        false,
        ResettableGatewayAccessBlob,
        ResettableGatewayRenewalBlob> TestPlatformBase;
// The existing platform seam keeps socket fault injection out of the product
// API. Only this probe can close MQTT without also resetting WiFi / BLE.
class TestPlatform final : public TestPlatformBase {
public:
    TestPlatform() { mqttFaultNetwork = &mqttNetwork(); }
};
#if defined(BLINKER_TEST_ENABLE_TIME)
// Test-only observers and responders borrow one real business service. The
// production protected-storage alias and security/SNTP clock stay untouched.
blinker_test::ObservedTime businessTime;
// Bounded observation of the real optional handler, outside radio callbacks.
class ObservedChildTime final : public blinker::IGatewayChildRequestHandler {
public:
    explicit ObservedChildTime(blinker::IWallTimeSource& source) : responder_(source) {}
    uint32_t features() const override { return responder_.features(); }
    bool handles(uint8_t kind) const override { return responder_.handles(kind); }
    blinker::Result reply(const blinker::GatewayChildRequestContext& context,
                         const blinker::bbp2::FrameView& request,
                         blinker::MutableByteSpan output, blinker::ByteView& response) override {
        ++requests; permissions = context.permissions; sequence = request.header.sequence;
        const auto result = responder_.reply(context, request, output, response);
        error = static_cast<unsigned>(result.code()); bytes = response.size;
        if (result && !response.empty()) ++replies;
        return result;
    }
    void reset() override { responder_.reset(); }
    uint32_t requests = 0U, replies = 0U;
    unsigned permissions = 0U, sequence = 0U, error = 0U, bytes = 0U;
private:
    blinker::GatewayTimeResponder responder_;
};
#if defined(BLINKER_TEST_TIME_SOURCE_OUTAGE)
// Test-only provider outage, not a fake UTC, credential or protocol reply.
class ChildTimeSource final : public blinker::IWallTimeSource {
public:
    void pauseOnce() {
        if (issued_) return;
        issued_ = true;
        until_ = millis() + 45000U;
        Serial.println("B3_SOURCE_PAUSE durationMs=45000");
    }
    bool readTime(blinker::WallClockSnapshot& snapshot) override {
        if (issued_ && static_cast<int32_t>(until_ - millis()) > 0) {
            snapshot = blinker::WallClockSnapshot();
            return false;
        }
        return businessTime.readTime(snapshot);
    }
private:
    bool issued_ = false;
    uint32_t until_ = 0U;
} childTimeSource;
ObservedChildTime firstChildTime(childTimeSource);
ObservedChildTime secondChildTime(childTimeSource);
#else
ObservedChildTime firstChildTime(businessTime);
ObservedChildTime secondChildTime(businessTime);
#endif
#endif
blinker::integration::official_detail::
#if defined(BLINKER_TEST_INGRESS_CAPACITY)
    BasicEsp32EdgeHubProduct<TestPlatform, IngressCapacityProbe> edgeHub;
#else
    BasicEsp32EdgeHubProduct<TestPlatform> edgeHub;
#endif
uint8_t lastState = 0xFFU;
uint16_t lastError = 0xFFFFU;
uint8_t lastChildState = 0xFFU;
uint8_t selectedChild = 0U;
uint16_t lastChildError = 0xFFFFU;
uint16_t lastRouteError = 0xFFFFU;
bool lastManagementSubscribed = false;
bool lastExecutionWanted = false;
uint32_t lastDiagnosticMs = 0U;
volatile uint8_t lastWifiDisconnectReason = 0U;

void printResult(const char* stage, blinker::Result result) {
    Serial.print(stage);
    Serial.print('=');
    Serial.println(static_cast<unsigned int>(result.code()));
}

void onWifiEvent(arduino_event_id_t event, arduino_event_info_t info) {
    if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
        lastWifiDisconnectReason = info.wifi_sta_disconnected.reason;
    }
}

void printLocalWifiProfile() {
    blinker::Esp32NvsWifiCredentialSink storage(
        blinker::integration::official_detail::
            edgeHubWifiStorageConfig(false));
    blinker::Result result = storage.begin();
    blinker::WifiCredentialProfile profile;
    if (result) result = storage.load(profile);
    Serial.print("wifiProfile=");
    Serial.print(static_cast<unsigned int>(result.code()));
    if (result) {
        Serial.print(" generation=");
        Serial.print(static_cast<unsigned long>(profile.generation));
        Serial.print(" confirmed=");
        Serial.print(profile.confirmed() ? 1 : 0);
        Serial.print(" auth=");
        Serial.print(static_cast<unsigned int>(profile.authentication));
        Serial.print(" ssid=");
        Serial.write(profile.ssid, profile.ssidLength);
    }
    Serial.println();
    blinker::clearWifiCredentialProfile(profile);
    storage.end();
}

blinker::Result updateLocalWifiProfile() {
#if defined(BLINKER_INTERNAL_WIFI_SSID) && \
    defined(BLINKER_INTERNAL_WIFI_PASSWORD)
    blinker::Esp32NvsWifiCredentialSink storage(
        blinker::integration::official_detail::
            edgeHubWifiStorageConfig(false));
    blinker::Result result = storage.begin();
    blinker::WifiNetworkConfig network;
    network.ssid = blinker::ByteView(
        reinterpret_cast<const uint8_t*>(BLINKER_INTERNAL_WIFI_SSID),
        strlen(BLINKER_INTERNAL_WIFI_SSID));
    network.authentication = BLINKER_INTERNAL_WIFI_PASSWORD[0] == '\0'
                                 ? blinker::WifiAuthentication::Open
                                 : blinker::WifiAuthentication::Wpa2Personal;
    network.credential = blinker::ByteView(
        reinterpret_cast<const uint8_t*>(BLINKER_INTERNAL_WIFI_PASSWORD),
        strlen(BLINKER_INTERNAL_WIFI_PASSWORD));
    if (result) result = blinker::validateWifiNetworkConfig(network);
    if (result) {
        blinker::WifiCredentialProfile current;
        const blinker::Result loaded = storage.load(current);
        const bool unchanged = loaded &&
            current.authentication == network.authentication &&
            current.hidden() == network.hidden &&
            current.ssidLength == network.ssid.size &&
            current.credentialLength == network.credential.size &&
            memcmp(current.ssid, network.ssid.data, network.ssid.size) == 0 &&
            (network.credential.empty() ||
             memcmp(
                 current.credential,
                 network.credential.data,
                 network.credential.size) == 0);
        blinker::clearWifiCredentialProfile(current);
        if (!unchanged) {
            result = !loaded && loaded.code() != blinker::ErrorCode::NotFound
                         ? loaded
                         : storage.commit(network);
        }
    }
    storage.end();
    return result;
#else
    return blinker::Result::success();
#endif
}

void pollFaultControl() {
    while (Serial.available() > 0) {
        const int command = Serial.read();
#if defined(BLINKER_TEST_ENABLE_TIME)
#if defined(BLINKER_TEST_TIME_SOURCE_OUTAGE)
        if (command == 'v') childTimeSource.pauseOnce();
#endif
        if (command == 'u') {
            blinker::WallClockSnapshot wall;
            const bool valid = businessTime.readTime(wall);
            Serial.printf("B3_HUB_TIME valid=%u utc=%lu source=%u ageMs=%lu uncertaintyMs=%lu requests=%lu samples=%lu serverSamples=%lu\n",
                valid, static_cast<unsigned long>(wall.utcMillis / 1000U), static_cast<unsigned>(wall.source),
                static_cast<unsigned long>(wall.ageMillis), static_cast<unsigned long>(wall.uncertaintyMillis),
                static_cast<unsigned long>(businessTime.requests), static_cast<unsigned long>(businessTime.samples),
                static_cast<unsigned long>(businessTime.serverSamples));
        }
        if (command == 'y') {
            const auto& trace = selectedChild == 0U ? firstChildTime : secondChildTime;
            Serial.printf("B3_HANDLER requests=%lu replies=%lu permissions=%u sequence=%u error=%u bytes=%u\n",
                static_cast<unsigned long>(trace.requests), static_cast<unsigned long>(trace.replies),
                trace.permissions, trace.sequence, trace.error, trace.bytes);
        }
#endif
        if (command == '0' || command == '1') {
            selectedChild = static_cast<uint8_t>(command - '0');
            lastChildState = 0xFFU;
        } else if (command == 'g') {
            // Exact single-ACL probe; never print addresses, records or keys.
            for (uint16_t handle = 0U; handle < 4U; ++handle) {
                ble_gap_conn_desc link = {};
                if (ble_gap_conn_find(handle, &link) != 0) continue;
                Serial.printf("gattLink intervalUnits=%u latency=%u supervisionUnits=%u mtu=%u\n",
                    link.conn_itvl, link.conn_latency, link.supervision_timeout, ble_att_mtu(handle));
            }
#if defined(BLINKER_TEST_INGRESS_CAPACITY)
        } else if (command == 'q') {
            IngressCapacityProbe::instance->arm();
#endif
        } else if (command == 'R' || command == 'r') {
            revocationCommitResetArmed = true;
            Serial.println("fault=pre_tombstone_armed");
        } else if (command == 'N' || command == 'n') {
            renewalReceiptResetArmed = true;
            Serial.println("fault=pre_receipt_store_armed");
        } else if (command == 'C' || command == 'c') {
            mqttDisconnectDeadline = millis() + 30000U;
            Serial.println("fault=mqtt_socket_close_armed");
        } else if (command == 'D' || command == 'd') {
            // Normal bearer disconnect: preserve child/Hub credentials and MQTT.
            printResult("childDisconnect", edgeHub.child(selectedChild)->session.disconnect());
#if defined(BLINKER_TEST_NATIVE_RADIO)
        } else if (command == 'T' || command == 't') {
            Serial.printf("radioProbe=clock us=%lu\n", static_cast<unsigned long>(micros()));
#endif
        } else if (command == 'A' || command == 'a') {
            edgeHub.stop();
            printResult("resetAccess", edgeHub.resetAccess());
            Serial.flush();
            delay(100U);
            esp_restart();
        }
    }
}

} // namespace

void setup() {
    Serial.begin(115200);
#if defined(ARDUINO_ARCH_ESP32)
    // Diagnostics must never stall the product loop when the USB host closes
    // its serial monitor or stops draining CDC data.
    Serial.setTxTimeoutMs(0U);
#endif
    delay(200U);
    WiFi.onEvent(onWifiEvent);
    printResult("wifiUpdate", updateLocalWifiProfile());
    printLocalWifiProfile();
#if defined(BLINKER_TEST_INSTALL_DEVICE_KEY) && defined(BLINKER_INTERNAL_EDGE_HUB_DEVICE_KEY) && \
    defined(BLINKER_INTERNAL_WIFI_SSID) && \
    defined(BLINKER_INTERNAL_WIFI_PASSWORD)
    const blinker::Result configured = edgeHub.configure(
        blinker::StringView(BLINKER_INTERNAL_EDGE_HUB_DEVICE_KEY),
        blinker::StringView(BLINKER_INTERNAL_WIFI_SSID),
        blinker::StringView(BLINKER_INTERNAL_WIFI_PASSWORD));
#else
    // Reuses the DeviceKey and WiFi profile already stored by the previous
    // control-plane run. This path never prints or recompiles either secret.
    const blinker::Result configured = edgeHub.configureProvisioning();
#endif
    printResult("configure", configured);
    blinker::Result attached = configured
                                         ? edgeHub.attach(client)
                                         : configured;
#if defined(BLINKER_TEST_ENABLE_TIME)
    if (attached) attached = edgeHub.attachTime(client, businessTime);
    if (attached) attached = edgeHub.setChildRequestHandler(0U, &firstChildTime);
    if (attached) attached = edgeHub.setChildRequestHandler(1U, &secondChildTime);
#endif
    printResult("attach", attached);
    const blinker::Result started = attached
                                        ? edgeHub.start()
                                        : attached;
    printResult("start", started);
}

void loop() {
#if defined(BLINKER_TEST_NATIVE_RADIO)
    native_radio_probe::poll();
#endif
    pollFaultControl();
    edgeHub.poll(1000U);
#if defined(BLINKER_TEST_INGRESS_CAPACITY)
    IngressCapacityProbe::instance->observe();
#endif
#if defined(BLINKER_TEST_NATIVE_RADIO)
    native_radio_probe::poll();
#endif
    if (mqttDisconnectDeadline != 0U) {
        if (static_cast<int32_t>(millis() - mqttDisconnectDeadline) >= 0) {
            mqttDisconnectDeadline = 0U;
            Serial.println("fault=mqtt_socket_close_expired");
        } else if (mqttFaultNetwork != nullptr &&
                   edgeHub.permitJoin().windowOpen() &&
                   edgeHub.permitJoinRelay().waitingUpstreamAck()) {
            mqttDisconnectDeadline = 0U;
            mqttFaultNetwork->stop();
            Serial.println("fault=mqtt_socket_closed_upstream_pending");
        }
    }
    const blinker::ProductLifecycleStatus status = edgeHub.status();
    const uint8_t state = static_cast<uint8_t>(status.state);
    const uint16_t error = static_cast<uint16_t>(status.lastError);
    if (state != lastState || error != lastError) {
        Serial.print("state=");
        Serial.print(static_cast<unsigned int>(state));
        Serial.print(" error=");
        Serial.print(static_cast<unsigned int>(error));
        Serial.print(" networkReady=");
        Serial.println(status.networkReady ? 1 : 0);
        lastState = state;
        lastError = error;
    }
    const uint8_t childState = static_cast<uint8_t>(
        edgeHub.child(selectedChild)->session.state());
    const uint16_t childError = static_cast<uint16_t>(
        edgeHub.child(selectedChild)->session.lastError());
    const uint16_t routeError = static_cast<uint16_t>(
        edgeHub.child(selectedChild)->bridge.lastError());
    const bool managementSubscribed = edgeHub.management().subscribed();
    const bool executionWanted = edgeHub.child(selectedChild)->bridge.executionWanted();
    const uint32_t now = millis();
    if (childState != lastChildState || childError != lastChildError ||
        routeError != lastRouteError ||
        executionWanted != lastExecutionWanted ||
        managementSubscribed != lastManagementSubscribed ||
        static_cast<uint32_t>(now - lastDiagnosticMs) >= 10000U) {
        Serial.print("childState=");
        Serial.print(static_cast<unsigned int>(childState));
        Serial.print(" childSlot=");
        Serial.print(selectedChild);
        Serial.print(" secure=");
        Serial.print(edgeHub.child(selectedChild)->session.secure() ? 1 : 0);
        Serial.print(" error=");
        Serial.print(static_cast<unsigned int>(childError));
        Serial.print(" authState=");
        Serial.print(static_cast<unsigned int>(
            edgeHub.child(selectedChild)->session.lastAuthenticationState()));
        Serial.print(" routeError=");
        Serial.print(static_cast<unsigned int>(routeError));
        Serial.print(" executionWanted=");
        Serial.print(executionWanted ? 1 : 0);
        Serial.print(" controlPending=");
        Serial.print(edgeHub.child(selectedChild)->executor.controllerControlPending() ? 1 : 0);
        Serial.print(" revokeActive=");
        Serial.print(edgeHub.child(selectedChild)->revocation.active() ? 1 : 0);
        Serial.print(" revokeWaiting=");
        Serial.print(edgeHub.child(selectedChild)->revocation.waitingResponse() ? 1 : 0);
        Serial.print(" revokeReady=");
        Serial.print(edgeHub.child(selectedChild)->revocation.resultReady() ? 1 : 0);
        Serial.print(" managementError=");
        Serial.print(static_cast<unsigned int>(
            edgeHub.management().lastError()));
        Serial.print(" managementSubscribed=");
        Serial.print(managementSubscribed ? 1 : 0);
        Serial.print(" permitState=");
        Serial.print(static_cast<unsigned int>(
            edgeHub.permitJoinAdapter().state()));
        Serial.print(" permitError=");
        Serial.print(static_cast<unsigned int>(
            edgeHub.permitJoinAdapter().lastError()));
        Serial.print(" permitConnected=");
        Serial.print(edgeHub.permitJoinAdapter().relayConnected() ? 1 : 0);
        Serial.print(" permitDropped=");
        Serial.print(static_cast<unsigned long>(
            edgeHub.permitJoinAdapter().droppedPacketCount()));
        Serial.print(" relayActive=");
        Serial.print(edgeHub.permitJoinRelay().active() ? 1 : 0);
        Serial.print(" relayDown=");
        Serial.print(static_cast<unsigned long>(
            edgeHub.permitJoinRelay().downFragmentCount()));
        Serial.print(" relayUp=");
        Serial.print(static_cast<unsigned long>(
            edgeHub.permitJoinRelay().upFragmentCount()));
        Serial.print(" relayWaitingUpAck=");
        Serial.print(edgeHub.permitJoinRelay().waitingUpstreamAck() ? 1 : 0);
        Serial.print(" relayError=");
        Serial.print(static_cast<unsigned int>(
            edgeHub.permitJoinRelay().lastError()));
        Serial.print(" wifiStatus=");
        Serial.print(static_cast<unsigned int>(WiFi.status()));
        Serial.print(" wifiMode=");
        Serial.print(static_cast<unsigned int>(WiFi.getMode()));
        Serial.print(" wifiLifecycle=");
        Serial.print(static_cast<unsigned int>(
            edgeHub.wifiLifecycle().state()));
        Serial.print(" wifiError=");
        Serial.print(static_cast<unsigned int>(
            edgeHub.wifiLifecycle().lastError()));
        Serial.print(" disconnectReason=");
        Serial.print(static_cast<unsigned int>(lastWifiDisconnectReason));
        Serial.print(" heapFree=");
        Serial.print(static_cast<unsigned long>(ESP.getFreeHeap()));
        Serial.print(" heapMin=");
        Serial.print(static_cast<unsigned long>(ESP.getMinFreeHeap()));
        Serial.print(" heapLargest=");
        Serial.print(static_cast<unsigned long>(
            heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)));
        Serial.print(" loopStackMin=");
        Serial.print(taskStackMinimum("loopTask"));
        Serial.print(" ipc0StackMin=");
        Serial.print(taskStackMinimum("ipc0"));
        Serial.print(" nimbleStackMin=");
        Serial.print(taskStackMinimum("nimble_host"));
        Serial.print(" ip=");
        Serial.println(WiFi.localIP());
        lastChildState = childState;
        lastChildError = childError;
        lastRouteError = routeError;
        lastExecutionWanted = executionWanted;
        lastManagementSubscribed = managementSubscribed;
        lastDiagnosticMs = now;
    }
}
