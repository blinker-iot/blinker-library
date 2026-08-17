#ifndef BLINKER_PROVISIONING_LOCALSETUPOPERATIONCONTRACT_H
#define BLINKER_PROVISIONING_LOCALSETUPOPERATIONCONTRACT_H

#include "../identity/ControllerCredential.h"
#include "../interface/IWifiCredentialSink.h"

namespace blinker {

static const uint8_t kLocalSetupOperationContractVersion = 1U;

enum : size_t {
    kInitialControllerInstallMaxEncodedSize = 38U,
    kConfigureNetworkRequestMaxEncodedSize = 110U,
    kConfigureNetworkResponseMaxEncodedSize = 10U
};

enum class ConfigureNetworkDisposition : uint8_t {
    CandidateStored = 1U
};

struct InitialControllerInstall {
    ByteView controllerSecret;
};

struct ConfigureNetworkRequest {
    WifiNetworkConfig network;
};

struct ConfigureNetworkResponse {
    uint32_t networkGeneration;
    ConfigureNetworkDisposition disposition;

    ConfigureNetworkResponse()
        : networkGeneration(0U),
          disposition(ConfigureNetworkDisposition::CandidateStored) {}
};

Result encodeInitialControllerInstall(
    const InitialControllerInstall& body,
    MutableByteSpan output,
    ByteView& encoded);
Result decodeInitialControllerInstall(
    ByteView encoded,
    InitialControllerInstall& body);

Result encodeConfigureNetworkRequest(
    const ConfigureNetworkRequest& body,
    MutableByteSpan output,
    ByteView& encoded);
Result decodeConfigureNetworkRequest(
    ByteView encoded,
    ConfigureNetworkRequest& body);

Result encodeConfigureNetworkResponse(
    const ConfigureNetworkResponse& body,
    MutableByteSpan output,
    ByteView& encoded);
Result decodeConfigureNetworkResponse(
    ByteView encoded,
    ConfigureNetworkResponse& body);

} // namespace blinker

#endif
