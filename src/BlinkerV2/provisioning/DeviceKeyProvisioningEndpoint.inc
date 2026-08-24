#include "DeviceKeyProvisioningEndpoint.h"

#include <string.h>

#include "../core/SecureMemory.h"

namespace blinker {

namespace {

void writeU32(uint8_t* output, uint32_t value) {
    output[0] = static_cast<uint8_t>(value >> 24U);
    output[1] = static_cast<uint8_t>(value >> 16U);
    output[2] = static_cast<uint8_t>(value >> 8U);
    output[3] = static_cast<uint8_t>(value);
}

uint32_t readU32(const uint8_t* input) {
    return (static_cast<uint32_t>(input[0]) << 24U) |
           (static_cast<uint32_t>(input[1]) << 16U) |
           (static_cast<uint32_t>(input[2]) << 8U) |
           static_cast<uint32_t>(input[3]);
}

DeviceKeyProvisioningStatus storageStatus(Result result) {
    return result.code() == ErrorCode::StateConflict
               ? DeviceKeyProvisioningStatus::Conflict
               : DeviceKeyProvisioningStatus::StorageFailure;
}

} // namespace

Result DeviceKeyProvisioningEndpoint::writeStatus(
    uint8_t operation,
    DeviceKeyProvisioningStatus status,
    MutableByteSpan response,
    size_t& written) {
    written = 0U;
    if (response.data == nullptr ||
        response.size < kDeviceKeyProvisioningStatusResponseSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    response.data[0] = kDeviceKeyProvisioningVersion;
    response.data[1] = operation;
    response.data[2] = static_cast<uint8_t>(status);
    written = kDeviceKeyProvisioningStatusResponseSize;
    return Result::success();
}

Result DeviceKeyProvisioningEndpoint::getInfo(
    MutableByteSpan response,
    size_t& written) {
    const uint8_t operation = static_cast<uint8_t>(
        DeviceKeyProvisioningOperation::GetInfo);
    if (!isValidDeviceInstanceId(instance_)) {
        return writeStatus(
            operation,
            DeviceKeyProvisioningStatus::StorageFailure,
            response,
            written);
    }
    if (!validComposition_) {
        return writeStatus(
            operation,
            DeviceKeyProvisioningStatus::StorageFailure,
            response,
            written);
    }
    if (response.data == nullptr ||
        response.size < kDeviceKeyProvisioningInfoResponseSize) {
        written = 0U;
        return Result::failure(ErrorCode::BufferTooSmall);
    }

    DeviceKey current;
    const Result loaded = keys_.load(current);
    clearDeviceKey(current);
    if (!loaded && loaded.code() != ErrorCode::NotFound) {
        return writeStatus(
            operation,
            DeviceKeyProvisioningStatus::StorageFailure,
            response,
            written);
    }

    uint8_t flags = loaded
                        ? static_cast<uint8_t>(
                              kDeviceKeyProvisioningHasDeviceKey)
                        : 0U;
    uint32_t accessEpoch = 0U;
    if (access_ != nullptr) {
        flags = static_cast<uint8_t>(
            flags | kDeviceKeyProvisioningSupportsAccessBootstrap);
        const Result accessResult =
            access_->loadAccessEpoch(accessEpoch);
        if (accessResult) {
            flags = static_cast<uint8_t>(
                flags | kDeviceKeyProvisioningHasAccessState);
        } else if (accessResult.code() != ErrorCode::NotFound) {
            return writeStatus(
                operation,
                DeviceKeyProvisioningStatus::StorageFailure,
                response,
                written);
        }
    }

    response.data[0] = kDeviceKeyProvisioningVersion;
    response.data[1] = operation;
    response.data[2] = static_cast<uint8_t>(
        DeviceKeyProvisioningStatus::Success);
    response.data[3] = flags;
    memcpy(response.data + 4U, instance_.bytes, sizeof(instance_.bytes));
    writeU32(response.data + 4U + sizeof(instance_.bytes), accessEpoch);
    written = kDeviceKeyProvisioningInfoResponseSize;
    return Result::success();
}

Result DeviceKeyProvisioningEndpoint::commitKey(
    uint8_t operation,
    const DeviceKey& candidate,
    MutableByteSpan response,
    size_t& written) {
    Result result;
    if (installed_) {
        DeviceKey current;
        result = keys_.load(current);
        const bool same = result && sameDeviceKey(current, candidate);
        clearDeviceKey(current);
        return writeStatus(
            operation,
            !result
                ? DeviceKeyProvisioningStatus::StorageFailure
                : same ? DeviceKeyProvisioningStatus::Success
                       : DeviceKeyProvisioningStatus::Conflict,
            response,
            written);
    }

    result = keys_.replace(candidate);
    if (!result) {
        return writeStatus(
            operation,
            DeviceKeyProvisioningStatus::StorageFailure,
            response,
            written);
    }
    installed_ = true;
    return writeStatus(
        operation,
        DeviceKeyProvisioningStatus::Success,
        response,
        written);
}

Result DeviceKeyProvisioningEndpoint::install(
    ByteView encodedKey,
    MutableByteSpan response,
    size_t& written) {
    const uint8_t operation = static_cast<uint8_t>(
        DeviceKeyProvisioningOperation::Install);
    if (response.data == nullptr ||
        response.size < kDeviceKeyProvisioningStatusResponseSize) {
        written = 0U;
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    if (!validComposition_) {
        return writeStatus(
            operation,
            DeviceKeyProvisioningStatus::StorageFailure,
            response,
            written);
    }

    DeviceKey candidate;
    Result result = decodeDeviceKey(
        StringView(
            reinterpret_cast<const char*>(encodedKey.data),
            encodedKey.size),
        candidate);
    if (!result) {
        clearDeviceKey(candidate);
        return writeStatus(
            operation,
            DeviceKeyProvisioningStatus::InvalidDeviceKey,
            response,
            written);
    }

    if (access_ != nullptr && !installed_) {
        uint32_t accessEpoch = 0U;
        result = access_->loadAccessEpoch(accessEpoch);
        if (!result) {
            clearDeviceKey(candidate);
            return writeStatus(
                operation,
                result.code() == ErrorCode::NotFound
                    ? DeviceKeyProvisioningStatus::AccessRequired
                    : DeviceKeyProvisioningStatus::StorageFailure,
                response,
                written);
        }
    }

    result = commitKey(operation, candidate, response, written);
    clearDeviceKey(candidate);
    return result;
}

Result DeviceKeyProvisioningEndpoint::bootstrap(
    ByteView request,
    MutableByteSpan response,
    size_t& written) {
    const uint8_t operation = static_cast<uint8_t>(
        DeviceKeyProvisioningOperation::Bootstrap);
    if (access_ == nullptr) {
        return writeStatus(
            operation,
            DeviceKeyProvisioningStatus::UnsupportedOperation,
            response,
            written);
    }
    if (response.data == nullptr ||
        response.size < kDeviceKeyProvisioningStatusResponseSize) {
        written = 0U;
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    if (!validComposition_) {
        return writeStatus(
            operation,
            DeviceKeyProvisioningStatus::StorageFailure,
            response,
            written);
    }

    DeviceKey candidate;
    Result result = decodeDeviceKey(
        StringView(
            reinterpret_cast<const char*>(request.data + 2U),
            kDeviceKeyTextSize),
        candidate);
    if (!result) {
        clearDeviceKey(candidate);
        return writeStatus(
            operation,
            DeviceKeyProvisioningStatus::InvalidDeviceKey,
            response,
            written);
    }

    size_t offset = 2U + kDeviceKeyTextSize;
    const uint32_t accessEpoch = readU32(request.data + offset);
    offset += 4U;

    ControllerCredential controller;
    controller.domain = ControllerCredentialDomain::PlatformAccess;
    controller.ownershipGeneration = accessEpoch;
    memcpy(controller.controllerId, request.data + offset, kControllerIdSize);
    offset += kControllerIdSize;
    controller.credentialVersion = readU32(request.data + offset);
    offset += 4U;
    controller.permissions = kControllerPermissionAll;
    memcpy(
        controller.secret,
        request.data + offset,
        kControllerCredentialSecretSize);

    if (accessEpoch == 0U) {
        result = Result::failure(ErrorCode::InvalidArgument);
    }
    if (result) result = validateControllerCredential(controller);
    if (!result) {
        clearControllerCredential(controller);
        clearDeviceKey(candidate);
        return writeStatus(
            operation,
            DeviceKeyProvisioningStatus::InvalidController,
            response,
            written);
    }

    result = access_->bootstrap(candidate, accessEpoch, controller);
    clearControllerCredential(controller);
    if (!result) {
        clearDeviceKey(candidate);
        return writeStatus(
            operation,
            storageStatus(result),
            response,
            written);
    }

    installed_ = true;
    result = writeStatus(
        operation,
        DeviceKeyProvisioningStatus::Success,
        response,
        written);
    clearDeviceKey(candidate);
    return result;
}

Result DeviceKeyProvisioningEndpoint::handle(
    ByteView request,
    MutableByteSpan response,
    size_t& written) {
    written = 0U;
    if (request.data == nullptr || request.size < 2U) {
        return writeStatus(
            0U,
            DeviceKeyProvisioningStatus::Malformed,
            response,
            written);
    }
    const uint8_t operation = request.data[1];
    if (request.data[0] != kDeviceKeyProvisioningVersion) {
        return writeStatus(
            operation,
            DeviceKeyProvisioningStatus::UnsupportedVersion,
            response,
            written);
    }
    if (operation == static_cast<uint8_t>(
            DeviceKeyProvisioningOperation::GetInfo)) {
        if (request.size != kDeviceKeyProvisioningInfoRequestSize) {
            return writeStatus(
                operation,
                DeviceKeyProvisioningStatus::Malformed,
                response,
                written);
        }
        return getInfo(response, written);
    }
    if (operation == static_cast<uint8_t>(
            DeviceKeyProvisioningOperation::Install)) {
        if (request.size != kDeviceKeyProvisioningInstallRequestSize) {
            return writeStatus(
                operation,
                DeviceKeyProvisioningStatus::Malformed,
                response,
                written);
        }
        return install(
            ByteView(request.data + 2U, kDeviceKeyTextSize),
            response,
            written);
    }
    if (operation == static_cast<uint8_t>(
            DeviceKeyProvisioningOperation::Bootstrap)) {
        if (request.size != kDeviceKeyProvisioningBootstrapRequestSize) {
            return writeStatus(
                operation,
                DeviceKeyProvisioningStatus::Malformed,
                response,
                written);
        }
        return bootstrap(request, response, written);
    }
    return writeStatus(
        operation,
        DeviceKeyProvisioningStatus::UnsupportedOperation,
        response,
        written);
}

} // namespace blinker
