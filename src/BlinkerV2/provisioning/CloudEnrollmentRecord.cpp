#include "CloudEnrollmentRecord.h"

#include <string.h>

#include "../core/SecureMemory.h"

namespace blinker {

namespace {

bool nonZero(ByteView value) {
    if (value.data == nullptr || value.empty()) return false;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined != 0U;
}

bool allZero(ByteView value) {
    if (value.data == nullptr) return false;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined == 0U;
}

} // namespace

CloudEnrollmentRecord::CloudEnrollmentRecord()
    : state(CloudEnrollmentRecordState::Prepared),
      ownershipGeneration(0U),
      credentialVersion(0U),
      encryptionKeyId(0U),
      requestId(),
      ephemeralPrivateKey(),
      cloudSecret(),
      grantDigest() {}

Result validateCloudEnrollmentRecord(const CloudEnrollmentRecord& record) {
    if ((record.state != CloudEnrollmentRecordState::Prepared &&
         record.state != CloudEnrollmentRecordState::GrantAccepted) ||
        record.ownershipGeneration == 0U ||
        record.credentialVersion == 0U || record.encryptionKeyId == 0U ||
        !nonZero(ByteView(record.requestId, sizeof(record.requestId))) ||
        !nonZero(ByteView(
            record.ephemeralPrivateKey,
            sizeof(record.ephemeralPrivateKey))) ||
        !nonZero(ByteView(record.cloudSecret, sizeof(record.cloudSecret)))) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    const ByteView digest(record.grantDigest, sizeof(record.grantDigest));
    if ((record.state == CloudEnrollmentRecordState::Prepared &&
         !allZero(digest)) ||
        (record.state == CloudEnrollmentRecordState::GrantAccepted &&
         !nonZero(digest))) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    return Result::success();
}

bool sameCloudEnrollmentRecord(
    const CloudEnrollmentRecord& first,
    const CloudEnrollmentRecord& second) {
    return first.state == second.state &&
           first.ownershipGeneration == second.ownershipGeneration &&
           first.credentialVersion == second.credentialVersion &&
           first.encryptionKeyId == second.encryptionKeyId &&
           constantTimeEqual(
               ByteView(first.requestId, sizeof(first.requestId)),
               ByteView(second.requestId, sizeof(second.requestId))) &&
           constantTimeEqual(
               ByteView(
                   first.ephemeralPrivateKey,
                   sizeof(first.ephemeralPrivateKey)),
               ByteView(
                   second.ephemeralPrivateKey,
                   sizeof(second.ephemeralPrivateKey))) &&
           constantTimeEqual(
               ByteView(first.cloudSecret, sizeof(first.cloudSecret)),
               ByteView(second.cloudSecret, sizeof(second.cloudSecret))) &&
           constantTimeEqual(
               ByteView(first.grantDigest, sizeof(first.grantDigest)),
               ByteView(second.grantDigest, sizeof(second.grantDigest)));
}

void clearCloudEnrollmentRecord(CloudEnrollmentRecord& record) {
    secureZero(MutableByteSpan(
        reinterpret_cast<uint8_t*>(&record),
        sizeof(record)));
    record.state = CloudEnrollmentRecordState::Prepared;
}

} // namespace blinker
