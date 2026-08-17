#ifndef BLINKER_PROVISIONING_LOCALSETUPSAGA_H
#define BLINKER_PROVISIONING_LOCALSETUPSAGA_H

#include "../interface/IClock.h"
#include "../interface/IControllerCredentialStore.h"
#include "../interface/ILocalSetupOperationHandler.h"
#include "../interface/IOwnershipRecordStore.h"
#include "CloudCredentialEnrollmentCoordinator.h"
#include "LocalSetupSagaStore.h"
#include "OwnershipClaimCoordinator.h"
#include "WifiCredentialStore.h"

namespace blinker {

// One durable mutation coordinator behind LocalSetupApplication. It contains
// no BLE, WiFi SDK, HTTP or MQTT dependency. BLE-only passes null network/cloud
// dependencies; WiFi+BLE passes both.
class LocalSetupSaga final : public ILocalSetupOperationHandler {
public:
    LocalSetupSaga(const DeviceInstanceId& deviceInstanceId,
                   IOwnershipRecordStore& ownership,
                   OwnershipClaimCoordinator& claim,
                   IControllerCredentialStore& controllers,
                   LocalSetupSagaStore& journal,
                   IClock& clock,
                   WifiCredentialStore* network = nullptr,
                   CloudCredentialEnrollmentCoordinator* cloud = nullptr);
    ~LocalSetupSaga();
    LocalSetupSaga(const LocalSetupSaga&) = delete;
    LocalSetupSaga& operator=(const LocalSetupSaga&) = delete;

    Result prepareSetupSession(ByteView setupSessionLocator,
                               uint32_t& ownershipGeneration) override;
    Result beginAuthorizedSession(
        const LocalSetupAuthorization& authorization) override;
    Result handleAuthorizedOperation(
        const LocalSetupOperationContext& context,
        LocalSetupOperation operation,
        ByteView requestBody,
        MutableByteSpan operationScratch,
        MutableByteSpan responseBodyStorage,
        ByteView& responseBody) override;
    void endAuthorizedSession() override;

private:
    Result handleBeginClaim(LocalSetupSagaRecord& record,
                            MutableByteSpan output,
                            ByteView& response);
    Result handleApplyClaimGrant(LocalSetupSagaRecord& record,
                                 ByteView body,
                                 MutableByteSpan workspace,
                                 MutableByteSpan output,
                                 ByteView& response);
    Result handleCompleteClaim(LocalSetupSagaRecord& record,
                               ByteView body,
                               MutableByteSpan workspace,
                               ByteView& response);
    Result handleInstallController(LocalSetupSagaRecord& record,
                                   ByteView body,
                                   MutableByteSpan workspace,
                                   MutableByteSpan output,
                                   ByteView& response);
    Result handleConfigureNetwork(LocalSetupSagaRecord& record,
                                  ByteView body,
                                  MutableByteSpan output,
                                  ByteView& response);
    Result handleBeginCloud(LocalSetupSagaRecord& record,
                            ByteView body,
                            MutableByteSpan workspace,
                            MutableByteSpan output,
                            ByteView& response);
    Result handleApplyCloudGrant(LocalSetupSagaRecord& record,
                                 ByteView body,
                                 MutableByteSpan workspace,
                                 MutableByteSpan output,
                                 ByteView& response);
    Result handleCompleteCloud(LocalSetupSagaRecord& record,
                               ByteView body,
                               MutableByteSpan workspace,
                               ByteView& response);
    Result encodeClaimRequest(const LocalSetupSagaRecord& record,
                              MutableByteSpan output,
                              ByteView& response) const;
    Result encodeClaimReceipt(const LocalSetupSagaRecord& record,
                              ByteView encodedGrant,
                              MutableByteSpan output,
                              ByteView& response) const;
    Result installControllerAndEncodeReceipt(
        const LocalSetupSagaRecord& record,
        ByteView controllerSecret,
        MutableByteSpan workspace,
        MutableByteSpan output,
        ByteView& response);
    Result networkRecoveryAvailable(bool& available);
    Result loadCloud();
    Result validateContext(const LocalSetupSagaRecord& record,
                           const LocalSetupAuthorization& authorization) const;

    const DeviceInstanceId& deviceInstanceId_;
    IOwnershipRecordStore& ownership_;
    OwnershipClaimCoordinator& claim_;
    IControllerCredentialStore& controllers_;
    LocalSetupSagaStore& journal_;
    IClock& clock_;
    WifiCredentialStore* network_;
    CloudCredentialEnrollmentCoordinator* cloud_;
    bool authorizedSession_;
};

} // namespace blinker

#endif
