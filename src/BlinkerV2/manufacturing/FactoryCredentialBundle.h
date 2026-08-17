#ifndef BLINKER_MANUFACTURING_FACTORYCREDENTIALBUNDLE_H
#define BLINKER_MANUFACTURING_FACTORYCREDENTIALBUNDLE_H

#include "../identity/CloudCredentialStore.h"
#include "../provisioning/WifiCredentialStore.h"

namespace blinker {

struct FactoryCredentialBundle {
    CloudCredential cloud;
    WifiCredentialProfile wifi;
};

void clearFactoryCredentialBundle(FactoryCredentialBundle& bundle);

// Platform-neutral, fixed-size manufacturing record. It deliberately contains
// no platform storage layout and no enrollment-only claim/activation fields.
class FactoryCredentialInstaller {
public:
    enum : size_t { encodedSize = 228U };

    static Result decode(ByteView encoded, FactoryCredentialBundle& output);
    static Result install(
        const FactoryCredentialBundle& bundle,
        CloudCredentialStore& cloudStore,
        WifiCredentialStore& wifiStore);
    static Result installEncoded(
        ByteView encoded,
        CloudCredentialStore& cloudStore,
        WifiCredentialStore& wifiStore);
};

} // namespace blinker

#endif
