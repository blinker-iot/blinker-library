#ifndef BLINKER_SECURITY_INITIALCLOUDWIFIACCESS_H
#define BLINKER_SECURITY_INITIALCLOUDWIFIACCESS_H

#include "InitialCloudWifiHandoff.h"
#include "../core/HmacSha256.h"

namespace blinker {
namespace security {

// Synchronous, temporary carrier material. Never stored or part of a receipt.
struct InitialCloudWifiAccess {
    char ssid[21];
    char password[33];
    char pop[33];

    InitialCloudWifiAccess() : ssid(), password(), pop() {}
    ~InitialCloudWifiAccess() { clear(); }
    InitialCloudWifiAccess(const InitialCloudWifiAccess&) = delete;
    InitialCloudWifiAccess& operator=(const InitialCloudWifiAccess&) = delete;

    void clear() {
        secureZero(MutableByteSpan(reinterpret_cast<uint8_t*>(ssid), sizeof(ssid)));
        secureZero(MutableByteSpan(reinterpret_cast<uint8_t*>(password), sizeof(password)));
        secureZero(MutableByteSpan(reinterpret_cast<uint8_t*>(pop), sizeof(pop)));
    }
};

// Pure cross-language contract; a matching digest is not write authorization.
inline Result deriveInitialCloudWifiAccess(ByteView adminSecret, const InitialCloudKeyHandoff& handoff,
    InitialCloudWifiAccess& output) {
    output.clear();
    const ByteView instance(handoff.authority.instance.bytes, sizeof(handoff.authority.instance.bytes));
    const ByteView binding(handoff.sessionBinding, sizeof(handoff.sessionBinding));
    const ByteView intent(handoff.intent, sizeof(handoff.intent));
    const ByteView locator(handoff.locator.bytes, sizeof(handoff.locator.bytes));
    if (!bbp2::exactInitialCloudKeyBytes(adminSecret, kControllerCredentialSecretSize) ||
        !isValidDeviceInstanceId(handoff.authority.instance) ||
        !bbp2::exactInitialCloudKeyBytes(binding, kDirectSecureBindingSize) ||
        !bbp2::exactInitialCloudKeyBytes(intent, bbp2::kInitialCloudKeyInstallIntentSize) ||
        !bbp2::exactInitialCloudKeyBytes(locator, sizeof(handoff.locator.bytes)))
        return Result::failure(ErrorCode::InvalidArgument);

    // Purpose separation prevents reusing an installation handoff's AP/PoP
    // for a new installed-Key network recovery, even with equal coordinates.
    static const uint8_t initialDomain[] = "blinker.initial-cloud-wifi-handoff.v1";
    static const uint8_t networkDomain[] = "blinker.initial-cloud-network-handoff.v1";
    ByteView domain;
    if (handoff.purpose == InitialCloudWifiHandoffPurpose::KeyInstallation)
        domain = ByteView(initialDomain, sizeof(initialDomain));
    else if (handoff.purpose == InitialCloudWifiHandoffPurpose::InstalledKeyNetwork)
        domain = ByteView(networkDomain, sizeof(networkDomain));
    else return Result::failure(ErrorCode::InvalidArgument);
    static const char hex[] = "0123456789abcdef";
    memcpy(output.ssid, "BLK-", 4U);
    char* const destinations[] = {output.ssid + 4U, output.password, output.pop};
    for (uint8_t purpose = 1U; purpose <= 3U; ++purpose) {
        HmacSha256 hmac;
        Result result = hmac.begin(adminSecret);
        if (result) result = hmac.update(domain);
        if (result) result = hmac.update(ByteView(&purpose, 1U));
        if (result) result = hmac.update(instance);
        if (result) result = hmac.update(binding);
        if (result) result = hmac.update(intent);
        if (result) result = hmac.update(locator);
        uint8_t digest[kSha256Size] = {};
        if (result) result = hmac.finish(MutableByteSpan(digest, sizeof(digest)));
        if (result) {
            char* const destination = destinations[purpose - 1U];
            for (size_t i = 0U; i < (purpose == 1U ? 8U : 16U); ++i) {
                destination[2U * i] = hex[digest[i] >> 4U];
                destination[2U * i + 1U] = hex[digest[i] & 0x0fU];
            }
        }
        secureZero(MutableByteSpan(digest, sizeof(digest)));
        if (!result) { output.clear(); return result; }
    }
    return Result::success();
}

// Reuses the installed Admin; the supervisor independently owns the physical
// instance and Key/root checks, and is the only caller allowed to start an AP.
class InitialCloudWifiAccessSource {
public:
    explicit InitialCloudWifiAccessSource(IControllerCredentialSource& credentials)
        : credentials_(credentials) {}

    Result derive(const InitialCloudKeyHandoff& handoff, InitialCloudWifiAccess& output) {
        output.clear();
        const InitialCloudKeyHandoffAuthority& authority = handoff.authority;
        const ByteView id(authority.adminId, sizeof(authority.adminId));
        if (!bbp2::exactInitialCloudKeyBytes(id, kControllerIdSize) ||
            authority.epoch == 0U || authority.credentialVersion == 0U)
            return Result::failure(ErrorCode::InvalidArgument);
        ControllerCredential admin;
        Result result = credentials_.loadActive(id, ControllerCredentialDomain::PlatformAccess, authority.epoch, admin);
        if (result && (!validateControllerCredential(admin) ||
            admin.suite != ControllerCredentialSuite::HmacSha256_32 ||
            admin.domain != ControllerCredentialDomain::PlatformAccess || admin.permissions != kControllerPermissionAll ||
            admin.ownershipGeneration != authority.epoch || admin.credentialVersion != authority.credentialVersion ||
            !constantTimeEqual(admin.id(), id))) result = Result::failure(ErrorCode::AuthenticationRequired);
        if (result) result = deriveInitialCloudWifiAccess(admin.authenticationSecret(), handoff, output);
        clearControllerCredential(admin);
        return result;
    }

private:
    IControllerCredentialSource& credentials_;
};

} // namespace security
} // namespace blinker
#endif
