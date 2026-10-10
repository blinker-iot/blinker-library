#include "TaskAuthorityContract.h"
#include <string.h>
#include "../runtime/TaskEngine.h"

#include "../core/SecureMemory.h"
#include "../core/Sha256.h"
#include "../protocol/cbor/Cbor.h"

namespace blinker {
namespace task_authority_detail {

bool nonzero(ByteView value, size_t size) {
    if (value.data == nullptr || value.size != size) return false;
    uint8_t bits = 0U;
    for (size_t i = 0U; i < size; ++i) bits |= value.data[i];
    return bits != 0U;
}

Result sink(void* context, ByteView bytes) { return static_cast<Sha256*>(context)->update(bytes); }

struct RootSecrets {
    DeviceKey key;
    ControllerCredential admin;
    ~RootSecrets() { clearDeviceKey(key); clearControllerCredential(admin); }
};

// Existing App/Service Admin fingerprint convention. Current enrollment uses
// version 1; later versions require verified server metadata to advance too.
// Output may alias secret: all input is consumed before finish writes output.
Result adminFingerprint(const ControllerCredential& admin, MutableByteSpan output) {
    Sha256 hash;
    static const char domain[] = "blinker/direct-admin/fingerprint/v1";
    const uint8_t version[] = {static_cast<uint8_t>(admin.credentialVersion >> 24U),
        static_cast<uint8_t>(admin.credentialVersion >> 16U),
        static_cast<uint8_t>(admin.credentialVersion >> 8U), static_cast<uint8_t>(admin.credentialVersion)};
    Result result = hash.update(ByteView(reinterpret_cast<const uint8_t*>(domain), sizeof(domain))); // Includes NUL separator.
    if (result) result = hash.update(admin.id());
    if (result) result = hash.update(ByteView(version, sizeof(version)));
    if (result) result = hash.update(admin.authenticationSecret());
    return result ? hash.finish(output) : result;
}

bool valid(const TaskAuthorityChange& value) {
    return nonzero(value.requestId, 16U) && nonzero(value.deviceInstanceId, 16U) &&
        nonzero(value.taskScope, 16U) && nonzero(value.currentRoot, 32U) &&
        (value.previousRoot.empty() || (nonzero(value.previousRoot, 32U) &&
         !constantTimeEqual(value.previousRoot, value.currentRoot)));
}

Result writeChange(cbor::Writer& writer, const TaskAuthorityChange& change) {
    Result result = writer.writeUnsigned(1U);
    if (result) result = writer.writeBytes(change.requestId);
    if (result) result = writer.writeBytes(change.deviceInstanceId);
    if (result) result = writer.writeBytes(change.taskScope);
    if (result) result = writer.writeBytes(change.previousRoot);
    if (result) result = writer.writeBytes(change.currentRoot);
    return result;
}

Result loadRoots(const DeviceInstanceId& instance, const TaskRootSources& sources, RootSecrets& roots) {
    if (!isValidDeviceInstanceId(instance) || (sources.deviceKey == nullptr && sources.controllers == nullptr) ||
        (sources.controllers == nullptr && (!sources.adminControllerId.empty() || sources.accessEpoch != 0U)) ||
        (sources.controllers != nullptr && (!task_authority_detail::nonzero(sources.adminControllerId, 16U) || sources.accessEpoch == 0U)))
        return Result::failure(ErrorCode::InvalidArgument);
    Result result;
    if (sources.deviceKey != nullptr) {
        result = sources.deviceKey->load(roots.key);
        if (result) result = validateDeviceKey(roots.key);
    }
    if (result && sources.controllers != nullptr) {
        result = sources.controllers->loadActive(sources.adminControllerId,
            ControllerCredentialDomain::PlatformAccess, sources.accessEpoch, roots.admin);
        if (result) result = validateControllerCredential(roots.admin);
        if (result && (roots.admin.domain != ControllerCredentialDomain::PlatformAccess ||
            roots.admin.ownershipGeneration != sources.accessEpoch ||
            roots.admin.permissions != kControllerPermissionAll ||
            !constantTimeEqual(roots.admin.id(), sources.adminControllerId)))
            result = Result::failure(ErrorCode::AuthenticationRequired);
    }
    return result;
}

Result rootDigest(const DeviceInstanceId& instance, const TaskRootSources& sources,
    const RootSecrets& roots, MutableByteSpan digest) {
    uint8_t fingerprint[32] = {};
    Result result;
    if (sources.controllers != nullptr) {
        result = adminFingerprint(roots.admin, MutableByteSpan(fingerprint, sizeof(fingerprint)));
        if (!result) return result;
    }
    Sha256 hash;
    cbor::Writer writer(task_authority_detail::sink, &hash);
    result = writer.beginArray(5U);
    if (result) result = writer.writeText(StringView("blinker.task-root.v1"));
    if (result) result = writer.writeUnsigned(1U);
    if (result) result = writer.writeBytes(ByteView(instance.bytes, sizeof(instance.bytes)));
    if (result) result = sources.deviceKey != nullptr
        ? writer.writeBytes(ByteView(roots.key.bytes, sizeof(roots.key.bytes))) : writer.writeNull();
    if (result && sources.controllers != nullptr) {
        result = writer.beginArray(7U);
        if (result) result = writer.writeUnsigned(static_cast<uint8_t>(roots.admin.suite));
        if (result) result = writer.writeUnsigned(static_cast<uint8_t>(roots.admin.domain));
        if (result) result = writer.writeUnsigned(roots.admin.ownershipGeneration);
        if (result) result = writer.writeUnsigned(roots.admin.credentialVersion);
        if (result) result = writer.writeUnsigned(roots.admin.permissions);
        if (result) result = writer.writeBytes(roots.admin.id());
        if (result) result = writer.writeBytes(ByteView(fingerprint, sizeof(fingerprint)));
    } else if (result) result = writer.writeNull();
    if (result) result = hash.finish(digest);
    secureZero(MutableByteSpan(fingerprint, sizeof(fingerprint)));
    if (!result) secureZero(digest);
    return result;
}

} // namespace task_authority_detail

Result taskAuthorityRootDigest(const DeviceInstanceId& instance,
    const TaskRootSources& sources, MutableByteSpan digest) {
    if (digest.data == nullptr || digest.size != kSha256Size)
        return Result::failure(ErrorCode::InvalidArgument);
    secureZero(digest);
    task_authority_detail::RootSecrets roots;
    const Result result = task_authority_detail::loadRoots(instance, sources, roots);
    return result ? task_authority_detail::rootDigest(instance, sources, roots, digest) : result;
}

Result encodeTaskManagementReceipt(const DeviceInstanceId& instance,
    const TaskRootSources& sources, TaskResultKey role,
    const TaskManagementChallenge& completed, const TaskManagementResult& value,
    MutableByteSpan output, ByteView& encoded) {
    encoded = ByteView();
    if (output.data == nullptr || !completed.authority.valid() ||
        !constantTimeEqual(ByteView(instance.bytes, 16U), ByteView(completed.authority.deviceInstanceId, 16U)) ||
        (role != TaskResultKey::DeviceKey && role != TaskResultKey::Admin) ||
        (role == TaskResultKey::DeviceKey ? sources.deviceKey == nullptr : sources.controllers == nullptr))
        return Result::failure(ErrorCode::InvalidArgument);
    task_authority_detail::RootSecrets roots;
    Result result = task_authority_detail::loadRoots(instance, sources, roots);
    uint8_t digest[32] = {};
    if (result) result = task_authority_detail::rootDigest(instance, sources, roots, MutableByteSpan(digest, sizeof(digest)));
    if (result && !constantTimeEqual(ByteView(digest, sizeof(digest)), ByteView(completed.authority.rootDigest, 32U)))
        result = Result::failure(ErrorCode::AuthenticationRequired);
    secureZero(MutableByteSpan(digest, sizeof(digest)));
    if (result) result = encodeTaskManagementResult(completed, value, role,
        role == TaskResultKey::DeviceKey ? ByteView(roots.key.bytes, sizeof(roots.key.bytes)) : roots.admin.authenticationSecret(),
        output, encoded);
    return result;
}

Result encodeTaskAuthorityChange(const TaskAuthorityChange& change,
    MutableByteSpan output, ByteView& encoded) {
    encoded = ByteView();
    if (!task_authority_detail::valid(change) || output.data == nullptr)
        return Result::failure(ErrorCode::InvalidArgument);
    cbor::Writer writer(output);
    Result result = writer.beginArray(6U);
    if (result) result = task_authority_detail::writeChange(writer, change);
    if (result) encoded = writer.view();
    return result;
}

Result decodeTaskAuthorityChange(ByteView encoded, TaskAuthorityChange& change) {
    change = TaskAuthorityChange();
    if (encoded.data == nullptr || encoded.empty()) return Result::failure(ErrorCode::InvalidArgument);
    if (encoded.size > kTaskAuthorityChangeMaxSize) return Result::failure(ErrorCode::CapacityExceeded);
    cbor::Limits limits;
    limits.maxDepth = 1U; limits.maxContainerItems = 6U;
    limits.maxTextLength = 0U; limits.maxByteStringLength = 32U;
    Result result = cbor::validate(encoded, limits);
    if (!result) return result;
    cbor::Reader reader(encoded, limits);
    size_t count = 0U;
    result = reader.readArraySize(count);
    if (!result) return result;
    if (count != 6U) return Result::failure(ErrorCode::InvalidEncoding);
    uint64_t version = 0U;
    result = reader.readUnsigned(version);
    if (!result) return result;
    if (version != 1U) return Result::failure(ErrorCode::UnsupportedVersion);
    TaskAuthorityChange value;
    result = reader.readBytes(value.requestId);
    if (result) result = reader.readBytes(value.deviceInstanceId);
    if (result) result = reader.readBytes(value.taskScope);
    if (result) result = reader.readBytes(value.previousRoot);
    if (result) result = reader.readBytes(value.currentRoot);
    if (!result) return result;
    if (!reader.finished()) return Result::failure(ErrorCode::TrailingData);
    if (!task_authority_detail::valid(value)) return Result::failure(ErrorCode::InvalidEncoding);
    change = value;
    return Result::success();
}

Result taskAuthorityChangeDigest(const TaskAuthorityChange& change, MutableByteSpan digest) {
    if (digest.data == nullptr || digest.size != kSha256Size)
        return Result::failure(ErrorCode::InvalidArgument);
    secureZero(digest);
    if (!task_authority_detail::valid(change)) return Result::failure(ErrorCode::InvalidArgument);
    Sha256 hash;
    cbor::Writer writer(task_authority_detail::sink, &hash);
    Result result = writer.beginArray(7U);
    if (result) result = writer.writeText(StringView("blinker.task-authority-change.v1"));
    if (result) result = task_authority_detail::writeChange(writer, change);
    if (result) result = hash.finish(digest);
    if (!result) secureZero(digest);
    return result;
}

Result verifyTaskAuthorityChangeGrant(ByteView encodedGrant,
    const TaskAuthorityChange& expected, ByteView nonce,
    const DeviceInstanceId& currentInstance, ByteView currentRoot,
    IServerSignatureVerifier& verifier, TaskManagementGrant& grant) {
    grant = TaskManagementGrant();
    if (!task_authority_detail::valid(expected) || !isValidDeviceInstanceId(currentInstance) ||
        !task_authority_detail::nonzero(nonce, 16U) || !task_authority_detail::nonzero(currentRoot, 32U))
        return Result::failure(ErrorCode::InvalidArgument);
    if (!constantTimeEqual(expected.deviceInstanceId, ByteView(currentInstance.bytes, sizeof(currentInstance.bytes))) ||
        !constantTimeEqual(expected.currentRoot, currentRoot))
        return Result::failure(ErrorCode::AuthenticationRequired);
    uint8_t digest[kSha256Size] = {};
    Result result = taskAuthorityChangeDigest(expected, MutableByteSpan(digest, sizeof(digest)));
    if (result) {
        TaskGrantBinding binding;
        binding.deviceInstanceId = expected.deviceInstanceId; binding.taskScope = expected.taskScope;
        binding.nonce = nonce; binding.requestId = expected.requestId;
        binding.operationDigest = ByteView(digest, sizeof(digest));
        result = verifyTaskManagementGrant(encodedGrant, binding, verifier, grant);
    }
    secureZero(MutableByteSpan(digest, sizeof(digest)));
    return result;
}

TaskAuthorityCache::TaskAuthorityCache()
    : observation_(), generation_(0U), rootsReady_(false), refreshing_(false) {}

void TaskAuthorityCache::invalidate() {
    const bool terminal = observation_.state == TaskAuthorityState::Retired || generation_ == UINT32_MAX;
    observation_ = TaskAuthorityObservation();
    rootsReady_ = false;
    if (generation_ != UINT32_MAX) ++generation_;
    if (terminal) observation_.state = TaskAuthorityState::Retired;
}

void TaskAuthorityCache::retire() {
    invalidate();
    observation_.state = TaskAuthorityState::Retired;
}

void TaskAuthorityCache::clearScope() {
    secureZero(MutableByteSpan(observation_.binding.taskScope, 16U));
    if (observation_.state != TaskAuthorityState::Retired) observation_.state = TaskAuthorityState::Pending;
}

Result TaskAuthorityCache::refresh(const DeviceInstanceId& instance, const TaskRootSources& sources) {
    if (refreshing_) return Result::failure(ErrorCode::WouldBlock);
    invalidate();
    if (observation_.state == TaskAuthorityState::Retired)
        return Result::failure(ErrorCode::AuthenticationRequired);
    const uint32_t generation = generation_;
    refreshing_ = true;
    uint8_t digest[32] = {};
    Result result = taskAuthorityRootDigest(instance, sources, MutableByteSpan(digest, sizeof(digest)));
    refreshing_ = false;
    if (generation != generation_ || observation_.state == TaskAuthorityState::Retired)
        result = Result::failure(ErrorCode::AuthenticationRequired);
    if (result) {
        memcpy(observation_.binding.deviceInstanceId, instance.bytes, sizeof(instance.bytes));
        memcpy(observation_.binding.rootDigest, digest, sizeof(digest));
        rootsReady_ = true;
    }
    secureZero(MutableByteSpan(digest, sizeof(digest)));
    return result;
}

Result TaskAuthorityCache::root(TaskAuthorityBinding& output) const {
    output = TaskAuthorityBinding();
    if (!rootsReady_ || observation_.state == TaskAuthorityState::Retired)
        return Result::failure(ErrorCode::AuthenticationRequired);
    output = observation_.binding;
    return Result::success();
}

Result TaskAuthorityCache::selectVerified(const TaskAuthorityBinding& binding, uint32_t generation) {
    if (!rootsReady_ || generation != generation_ || observation_.state == TaskAuthorityState::Retired ||
        !binding.valid() ||
        !constantTimeEqual(ByteView(binding.deviceInstanceId, 16U), ByteView(observation_.binding.deviceInstanceId, 16U)) ||
        !constantTimeEqual(ByteView(binding.rootDigest, 32U), ByteView(observation_.binding.rootDigest, 32U)))
        return Result::failure(ErrorCode::AuthenticationRequired);
    observation_.binding = binding;
    observation_.state = TaskAuthorityState::Current;
    return Result::success();
}

Result TaskAuthorityCache::restore(const TaskEngine& engine) {
    clearScope();
    TaskAuthorityBinding stored;
    const Result result = engine.authorityBinding(stored);
    return result ? selectVerified(stored, generation_) : result;
}

void TaskAuthorityCache::read(void* context, TaskAuthorityObservation& output) {
    output = TaskAuthorityObservation();
    if (context != nullptr) static_cast<TaskAuthorityCache*>(context)->observe(output);
}

} // namespace blinker
