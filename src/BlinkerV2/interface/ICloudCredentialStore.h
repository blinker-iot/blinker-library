#ifndef BLINKER_INTERFACE_ICLOUDCREDENTIALSTORE_H
#define BLINKER_INTERFACE_ICLOUDCREDENTIALSTORE_H

#include "ICloudCredentialSource.h"

namespace blinker {

// Mutation boundary used only by cloud-enrollment and authorized reset.
class ICloudCredentialStore : public ICloudCredentialSource {
public:
    virtual Result installFresh(const CloudCredential& credential) = 0;
    virtual Result clear() = 0;
};

} // namespace blinker

#endif
