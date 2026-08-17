#include "OwnershipReleaseCleanup.h"

namespace blinker {

Result OwnershipReleaseCleanup::finalize(
    OwnershipReleaseNetworkPolicy networkPolicy) {
    if (networkPolicy != OwnershipReleaseNetworkPolicy::Preserve &&
        networkPolicy != OwnershipReleaseNetworkPolicy::Clear) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (networkPolicy == OwnershipReleaseNetworkPolicy::Clear &&
        network_ == nullptr) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    Result result = ownershipClaim_.eraseAll();
    if (result) result = cloudEnrollment_.eraseAll();
    if (result) result = cloud_.clear();
    if (result) result = controllers_.clearAll();
    if (result && networkPolicy == OwnershipReleaseNetworkPolicy::Clear) {
        result = network_->clear();
    }
    return result;
}

} // namespace blinker
