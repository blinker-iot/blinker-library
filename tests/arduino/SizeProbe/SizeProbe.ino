#include <BlinkerV2Advanced.h>
#include <BlinkerV2/provisioning/BleNoiseModePreparation.h>
#include <BlinkerV2/provisioning/BleNoiseProvisioningChannel.h>
#include <BlinkerV2/transport/LocalAccessFrameTransport.h>
#include <BlinkerV2/security/LocalAccessNoiseSession.h>
#include <BlinkerV2/security/LocalAccessPlainSession.h>

// Link-visible 32-bit ABI probes for the active public/runtime graph. Retired
// onboarding types must not return here.
extern "C" {
volatile uint8_t blinker_size_endpoint_slot[sizeof(blinker::EndpointSlot)];
volatile uint8_t blinker_size_field_spec[sizeof(blinker::FieldSpec)];
volatile uint8_t blinker_size_state_slot[sizeof(blinker::EndpointStateSlot)];
volatile uint8_t blinker_size_bool_property[sizeof(blinker::Property<bool>)];
volatile uint8_t blinker_size_void_action[sizeof(blinker::Action<void>)];
volatile uint8_t blinker_size_uint_event[sizeof(blinker::Event<uint32_t>)];
volatile uint8_t blinker_size_schema[
    sizeof(blinker::DeviceSchema<
        blinker::Property<bool>,
        blinker::Property<float>,
        blinker::Action<void>,
        blinker::Event<uint32_t> >)];
volatile uint8_t blinker_size_client[sizeof(blinker::Client)];
volatile uint8_t blinker_size_device[sizeof(blinker::Device)];
volatile uint8_t blinker_size_product[sizeof(blinker::Product)];
volatile uint8_t blinker_size_runtime[sizeof(blinker::DeviceRuntime)];
volatile uint8_t blinker_size_storage_24[
    sizeof(blinker::ClientStorage<24, 256, 192, 32>)];
volatile uint8_t blinker_size_device_instance_id[
    sizeof(blinker::DeviceInstanceId)];
volatile uint8_t blinker_size_device_instance_store[
    sizeof(blinker::DeviceInstanceIdStore)];
volatile uint8_t blinker_size_device_key[sizeof(blinker::DeviceKey)];
volatile uint8_t blinker_size_configured_key_source[
    sizeof(blinker::ConfiguredDeviceKeySource)];
volatile uint8_t blinker_size_session_arena[
    sizeof(blinker::DeviceKeySessionArena)];
volatile uint8_t blinker_size_session_endpoint[
    sizeof(blinker::DeviceKeyProvisioningEndpoint)];
volatile uint8_t blinker_size_controller_credential[
    sizeof(blinker::ControllerCredential)];
volatile uint8_t blinker_size_device_access_store[
    sizeof(blinker::DeviceAccessStore)];
volatile uint8_t blinker_size_controller_coordinator[
    sizeof(blinker::ControllerControlCoordinator)];
volatile uint8_t blinker_size_ble_grant[
    sizeof(blinker::BleEnrollmentGrant)];
volatile uint8_t blinker_size_ble_grant_context[
    sizeof(blinker::BleEnrollmentGrantContext)];
volatile uint8_t blinker_size_ble_grant_verifier[
    sizeof(blinker::BleEnrollmentGrantVerifier)];
volatile uint8_t blinker_size_ble_request[
    sizeof(blinker::BleEnrollmentRequest)];
volatile uint8_t blinker_size_ble_response[
    sizeof(blinker::BleEnrollmentResponse)];
volatile uint8_t blinker_size_ble_application[
    sizeof(blinker::BleEnrollmentApplication)];
volatile uint8_t blinker_size_wifi_profile[
    sizeof(blinker::WifiCredentialProfile)];
volatile uint8_t blinker_size_wifi_store[
    sizeof(blinker::WifiCredentialStore)];
volatile uint8_t blinker_size_wifi_connection[
    sizeof(blinker::WifiConnectionLifecycle)];
volatile uint8_t blinker_size_wifi_cloud[
    sizeof(blinker::WifiCloudLifecycle)];
volatile uint8_t blinker_size_http_session[
    sizeof(blinker::HttpDeviceKeySessionProvider)];
volatile uint8_t blinker_size_ble_mode[sizeof(blinker::ble::ModeProfile)];
volatile uint8_t blinker_size_ble_coordinator[
    sizeof(blinker::BleModeCoordinator)];
volatile uint8_t blinker_size_ble_lifecycle[
    sizeof(blinker::BleOnlyLifecycle)];
volatile uint8_t blinker_size_ble_completion[
    sizeof(blinker::DeviceAccessSetupCompletion)];
volatile uint8_t blinker_size_ble_transport[
    sizeof(blinker::BleFrameTransport)];
volatile uint8_t blinker_size_direct_secure_material[
    sizeof(blinker::security::DirectSecureKeyMaterial)];
volatile uint8_t blinker_size_direct_secure_session[
    sizeof(blinker::security::DirectSecureSession)];
volatile uint8_t blinker_size_noise_session[
    sizeof(blinker::security::NoiseNnSession)];
volatile uint8_t blinker_size_noise_channel[
    sizeof(blinker::BleNoiseProvisioningChannel)];
volatile uint8_t blinker_size_noise_preparation[
    sizeof(blinker::BleNoiseModePreparation)];
volatile uint8_t blinker_size_local_access_lease[
    sizeof(blinker::LocalAccessLease)];
volatile uint8_t blinker_size_local_access_admission[
    sizeof(blinker::LocalAccessAdmission)];
volatile uint8_t blinker_size_local_access_authority_2[
    sizeof(blinker::LocalAccessAuthority<2U>)];
volatile uint8_t blinker_size_local_access_challenge[
    sizeof(blinker::LocalAccessChallengeSession<2U>)];
volatile uint8_t blinker_size_local_access_control[
    sizeof(blinker::LocalAccessControlReceiver<2U>)];
volatile uint8_t blinker_size_local_access_transport[
    sizeof(blinker::LocalAccessFrameTransport<1U, blinker::security::LocalAccessNoiseSession>)];
volatile uint8_t blinker_size_local_access_plain_records[sizeof(blinker::security::LocalAccessPlainSession)];
volatile uint8_t blinker_size_local_access_noise_records[sizeof(blinker::security::LocalAccessNoiseSession)];
}

// Compile every control-receiver method on the real MCU ABI, even though the
// size sketch only retains its measurement symbol at link time.
template class blinker::LocalAccessControlReceiver<2U>;
template class blinker::LocalAccessFrameTransport<1U, blinker::security::LocalAccessNoiseSession>;
template class blinker::LocalAccessFrameTransport<1U, blinker::security::LocalAccessPlainSession>;

// Device.h owns the exact default Device ABI ceilings. Do not duplicate
// those numbers here; this probe still emits the actual object-size symbol.
#if BLINKER_RESOURCE_PROFILE == BLINKER_RESOURCE_PROFILE_SMALL
static_assert(sizeof(blinker::BleFrameTransport) <= 320U,
              "Small BLE transport exceeds 320 bytes");
#elif BLINKER_RESOURCE_PROFILE == BLINKER_RESOURCE_PROFILE_STANDARD
static_assert(sizeof(blinker::BleFrameTransport) <= 520U,
              "Standard BLE transport exceeds 520 bytes");
#elif BLINKER_RESOURCE_PROFILE == BLINKER_RESOURCE_PROFILE_LARGE
static_assert(sizeof(blinker::BleFrameTransport) <= 888U,
              "Large BLE transport exceeds 888 bytes");
#endif

static_assert(sizeof(blinker::Property<bool>) <= 16U,
              "Property runtime exceeds 16 bytes");
static_assert(sizeof(blinker::Action<void>) <= 16U,
              "Action runtime exceeds 16 bytes");
static_assert(sizeof(blinker::Event<uint32_t>) <= 8U,
              "Event runtime exceeds 8 bytes");
static_assert(sizeof(blinker::DeviceInstanceId) <= 16U,
              "DeviceInstanceId exceeds 16 bytes");
static_assert(sizeof(blinker::DeviceInstanceIdStore) <= 8U,
              "DeviceInstanceIdStore exceeds 8 bytes");
static_assert(sizeof(blinker::DeviceKey) <= 32U,
              "DeviceKey exceeds 32 bytes");
static_assert(sizeof(blinker::ConfiguredDeviceKeySource) == 40U,
              "ConfiguredDeviceKeySource changed its RAM budget");
static_assert(sizeof(blinker::DeviceKeySessionArena) == 1260U,
              "DeviceKeySessionArena changed its RAM budget");
static_assert(sizeof(blinker::DeviceKeyProvisioningEndpoint) <= 16U,
              "DeviceKeyProvisioningEndpoint exceeds 16 bytes");
static_assert(sizeof(blinker::ControllerCredential) <= 64U,
              "ControllerCredential exceeds 64 bytes");
static_assert(sizeof(blinker::DeviceAccessStore) <= 16U,
              "DeviceAccessStore exceeds 16 bytes");
static_assert(sizeof(blinker::ControllerControlCoordinator) <= 48U,
              "ControllerControlCoordinator exceeds 48 bytes");
static_assert(sizeof(blinker::BleEnrollmentGrant) <= 112U,
              "BleEnrollmentGrant exceeds 112 bytes");
static_assert(sizeof(blinker::BleEnrollmentGrantContext) <= 48U,
              "BleEnrollmentGrantContext exceeds 48 bytes");
static_assert(sizeof(blinker::BleEnrollmentGrantVerifier) <= 8U,
              "BleEnrollmentGrantVerifier exceeds 8 bytes");
static_assert(sizeof(blinker::BleEnrollmentRequest) <= 32U,
              "BleEnrollmentRequest exceeds 32 bytes");
static_assert(sizeof(blinker::BleEnrollmentResponse) <= 56U,
              "BleEnrollmentResponse exceeds 56 bytes");
static_assert(sizeof(blinker::BleEnrollmentApplication) <= 160U,
              "BleEnrollmentApplication exceeds 160 bytes");
static_assert(sizeof(blinker::WifiCredentialProfile) <= 104U,
              "WifiCredentialProfile exceeds 104 bytes");
static_assert(sizeof(blinker::WifiCredentialStore) <= 32U,
              "WifiCredentialStore exceeds 32 bytes");
static_assert(sizeof(blinker::WifiConnectionLifecycle) <= 64U,
              "WifiConnectionLifecycle exceeds 64 bytes");
static_assert(sizeof(blinker::WifiCloudLifecycle) == 20U,
              "WifiCloudLifecycle changed its RAM budget");
// Successful login proof now survives HTTP scratch cleanup: +16 B, no wire change.
static_assert(sizeof(blinker::HttpDeviceKeySessionProvider) <= 296U,
              "HttpDeviceKeySessionProvider exceeds 296 bytes");
static_assert(sizeof(blinker::ble::ModeProfile) <= 12U,
              "BLE ModeProfile exceeds 12 bytes");
static_assert(sizeof(blinker::BleModeCoordinator) <= 40U,
              "BleModeCoordinator exceeds 40 bytes");
static_assert(sizeof(blinker::security::NoiseNnSession) <= 272U,
              "NoiseNnSession exceeds 272 bytes");
#if BLINKER_RESOURCE_PROFILE == BLINKER_RESOURCE_PROFILE_SMALL
static_assert(sizeof(blinker::BleNoiseProvisioningChannel) <= 520U,
              "Small provisioning channel exceeds 520 bytes");
#elif BLINKER_RESOURCE_PROFILE == BLINKER_RESOURCE_PROFILE_STANDARD
static_assert(sizeof(blinker::BleNoiseProvisioningChannel) <= 600U,
              "Standard provisioning channel exceeds 600 bytes");
#else
static_assert(sizeof(blinker::BleNoiseProvisioningChannel) <= 728U,
              "Large provisioning channel exceeds 728 bytes");
#endif
static_assert(sizeof(blinker::BleNoiseModePreparation) <= 16U,
              "BleNoiseModePreparation exceeds 16 bytes");
static_assert(sizeof(blinker::BleOnlyLifecycle) <= 48U,
              "BleOnlyLifecycle exceeds 48 bytes");
static_assert(sizeof(blinker::DeviceAccessSetupCompletion) <= 8U,
              "DeviceAccessSetupCompletion exceeds 8 bytes");
static_assert(sizeof(blinker::security::DirectSecureKeyMaterial) == 96U,
              "Direct secure key material changed its RAM budget");
static_assert(sizeof(blinker::security::DirectSecureSession) <= 112U,
              "Direct secure session exceeds 112 bytes");
static_assert(sizeof(blinker::LocalAccessLease) <= 160U,
              "Local access lease exceeds 160 bytes");
static_assert(sizeof(blinker::LocalAccessAdmission) <= 184U,
              "Local access admission exceeds 184 bytes");
static_assert(sizeof(blinker::LocalAccessAuthority<2U>) <= 328U,
              "Two-target local access authority exceeds 328 bytes");
static_assert(sizeof(blinker::LocalAccessChallengeSession<2U>) <= 52U,
              "Local access challenge session exceeds 52 bytes");
static_assert(sizeof(blinker::LocalAccessFrameTransport<1U, blinker::security::LocalAccessNoiseSession>) +
                  sizeof(blinker::security::LocalAccessNoiseSession) <= 464U,
              "Local transport plus Noise state exceeds 464 bytes, excluding external buffers");
static_assert(sizeof(blinker::security::LocalAccessPlainSession) <= 68U,
              "Plain session must not reserve Noise key/handshake storage");
static_assert(sizeof(blinker::Product) <= 16U,
              "Product exceeds 16 bytes");

#define BLINKER_TOUCH_SIZE(symbol) symbol[0] = 1U

void setup() {
    BLINKER_TOUCH_SIZE(blinker_size_endpoint_slot);
    BLINKER_TOUCH_SIZE(blinker_size_field_spec);
    BLINKER_TOUCH_SIZE(blinker_size_state_slot);
    BLINKER_TOUCH_SIZE(blinker_size_bool_property);
    BLINKER_TOUCH_SIZE(blinker_size_void_action);
    BLINKER_TOUCH_SIZE(blinker_size_uint_event);
    BLINKER_TOUCH_SIZE(blinker_size_schema);
    BLINKER_TOUCH_SIZE(blinker_size_client);
    BLINKER_TOUCH_SIZE(blinker_size_device);
    BLINKER_TOUCH_SIZE(blinker_size_product);
    BLINKER_TOUCH_SIZE(blinker_size_runtime);
    BLINKER_TOUCH_SIZE(blinker_size_storage_24);
    BLINKER_TOUCH_SIZE(blinker_size_device_instance_id);
    BLINKER_TOUCH_SIZE(blinker_size_device_instance_store);
    BLINKER_TOUCH_SIZE(blinker_size_device_key);
    BLINKER_TOUCH_SIZE(blinker_size_configured_key_source);
    BLINKER_TOUCH_SIZE(blinker_size_session_arena);
    BLINKER_TOUCH_SIZE(blinker_size_session_endpoint);
    BLINKER_TOUCH_SIZE(blinker_size_controller_credential);
    BLINKER_TOUCH_SIZE(blinker_size_device_access_store);
    BLINKER_TOUCH_SIZE(blinker_size_controller_coordinator);
    BLINKER_TOUCH_SIZE(blinker_size_ble_grant);
    BLINKER_TOUCH_SIZE(blinker_size_ble_grant_context);
    BLINKER_TOUCH_SIZE(blinker_size_ble_grant_verifier);
    BLINKER_TOUCH_SIZE(blinker_size_ble_request);
    BLINKER_TOUCH_SIZE(blinker_size_ble_response);
    BLINKER_TOUCH_SIZE(blinker_size_ble_application);
    BLINKER_TOUCH_SIZE(blinker_size_wifi_profile);
    BLINKER_TOUCH_SIZE(blinker_size_wifi_store);
    BLINKER_TOUCH_SIZE(blinker_size_wifi_connection);
    BLINKER_TOUCH_SIZE(blinker_size_wifi_cloud);
    BLINKER_TOUCH_SIZE(blinker_size_http_session);
    BLINKER_TOUCH_SIZE(blinker_size_ble_mode);
    BLINKER_TOUCH_SIZE(blinker_size_ble_coordinator);
    BLINKER_TOUCH_SIZE(blinker_size_ble_lifecycle);
    BLINKER_TOUCH_SIZE(blinker_size_ble_completion);
    BLINKER_TOUCH_SIZE(blinker_size_ble_transport);
    BLINKER_TOUCH_SIZE(blinker_size_direct_secure_material);
    BLINKER_TOUCH_SIZE(blinker_size_direct_secure_session);
    BLINKER_TOUCH_SIZE(blinker_size_noise_session);
    BLINKER_TOUCH_SIZE(blinker_size_noise_channel);
    BLINKER_TOUCH_SIZE(blinker_size_noise_preparation);
    BLINKER_TOUCH_SIZE(blinker_size_local_access_lease);
    BLINKER_TOUCH_SIZE(blinker_size_local_access_admission);
    BLINKER_TOUCH_SIZE(blinker_size_local_access_authority_2);
    BLINKER_TOUCH_SIZE(blinker_size_local_access_challenge);
    BLINKER_TOUCH_SIZE(blinker_size_local_access_control);
    BLINKER_TOUCH_SIZE(blinker_size_local_access_transport);
    BLINKER_TOUCH_SIZE(blinker_size_local_access_plain_records);
    BLINKER_TOUCH_SIZE(blinker_size_local_access_noise_records);
}

void loop() {}
