// Task wire budgets: physical carrier plus historical Route/Delivery estimates.
// The latter do NOT make tasks routable or prove end-to-end authorization.
#include <cstdlib>
#include <cstring>
#include <iostream>
#include "BlinkerV2/core/TaskTiming.h"
#include "BlinkerV2/protocol/bbp2/Messages.h"
#include "BlinkerV2/provisioning/TaskManagementGrant.h"
#include "BlinkerV2/provisioning/TaskAuthorityContract.h"
#include "BlinkerV2/provisioning/TaskManagementSession.h"

using namespace blinker;
#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " << #x << '\n'; std::exit(1); } } while (0)

namespace {
const uint8_t identity[16] = {1U};
const uint8_t nonce[16] = {2U};
const uint8_t requestId[16] = {3U};
const uint8_t digest[32] = {4U};
const uint8_t signature[64] = {5U}; // Length placeholder, NOT a valid signature.

ByteView operation(MutableByteSpan output, TaskTimingKind kind, bool floating) {
    TaskMutation request;
    request.kind = TaskMutationKind::Replace;
    request.taskId = UINT32_MAX;
    request.expectedRevision = UINT32_MAX;
    memcpy(request.requestId, requestId, sizeof(requestId));
    TaskTiming& timing = request.definition.timing;
    timing.kind = kind;
    timing.utcMillis = kind == TaskTimingKind::UtcOnce ? kWallTimeEndMillis - 1U : 0U;
    timing.durationMillis = kind == TaskTimingKind::Countdown ? 0x7FFFFFFFU : 0U;
    timing.minuteOfDay = kind == TaskTimingKind::Weekly ? 1439U : 0U;
    timing.utcOffsetMinutes = kind == TaskTimingKind::Weekly ? -720 : 0;
    timing.weekdays = kind == TaskTimingKind::Weekly ? 127U : 0U;
    CHECK(validateTaskTiming(timing)); // Measure valid definitions, not just valid CBOR.
    TaskSnapshotRecord& value = request.definition;
    value.enabled = true;
    memset(value.key, 'k', sizeof(value.key));
    value.keyLength = static_cast<uint8_t>(sizeof(value.key));
    value.valueType = floating ? ValueType::Float64 : ValueType::UnsignedInteger;
    cbor::Writer w(MutableByteSpan(value.value, sizeof(value.value)));
    if (floating) CHECK(w.writeFloat64(0.1));
    else CHECK(w.writeUnsigned(9007199254740991ULL));
    value.valueLength = static_cast<uint8_t>(w.size());
    ByteView encoded;
    CHECK(encodeTaskMutationPrepare(request, output, encoded));
    CHECK(cbor::validate(encoded));
    return encoded;
}

ByteView grant(MutableByteSpan output) {
    TaskManagementGrant value;
    value.binding.deviceInstanceId = ByteView(identity, sizeof(identity));
    value.binding.taskScope = ByteView(nonce, sizeof(nonce));
    value.binding.nonce = ByteView(nonce, sizeof(nonce));
    value.binding.requestId = ByteView(requestId, sizeof(requestId));
    value.binding.operationDigest = ByteView(digest, sizeof(digest));
    value.serverKeyId = UINT32_MAX;
    value.signature = ByteView(signature, sizeof(signature));
    ByteView encoded;
    CHECK(encodeTaskManagementGrant(value, output, encoded));
    CHECK(encoded.size == kTaskManagementGrantMaxSize);
    TaskManagementGrant decoded;
    CHECK(decodeTaskManagementGrant(encoded, decoded));
    return encoded;
}

size_t routedFrameSize(ByteView body, bool delivery) {
    bbp2::RoutedMessageBody route;
    route.peerKind = delivery ? bbp2::RoutePeerKind::Account : bbp2::RoutePeerKind::LogicalDevice;
    route.peerId = ByteView(identity, sizeof(identity));
    route.requestId = ByteView(requestId, sizeof(requestId)); route.hasRequestId = true;
    // Existing two-byte CBOR kind width, solely to measure the envelope.
    // Does NOT make a task routable or encode it as telemetry in production.
    route.messageKind = static_cast<uint8_t>(bbp2::MessageKind::TelemetryData);
    route.messageFlags = bbp2::FlagAckRequired;
    route.messageBody = body;
    uint8_t output[512]; ByteView encoded;
    CHECK(delivery ? bbp2::encodeDeliveryBody(route, MutableByteSpan(output, sizeof(output)), encoded)
                   : bbp2::encodeRouteBody(route, MutableByteSpan(output, sizeof(output)), encoded));
    CHECK(cbor::validate(encoded));
    bbp2::FrameHeader header;
    header.bodyLength = static_cast<uint16_t>(encoded.size);
    header.flags = bbp2::FlagHasCrc32c; // Include the optional outer CRC in the budget.
    return bbp2::encodedFrameSize(header);
}
}

int main() {
    for (unsigned op = 0U; op <= 8U; ++op) {
        CHECK(taskWireRequest(static_cast<TaskWireOperation>(op)) == (op <= 3U || op >= 7U));
        for (size_t length : {1U, 23U, 24U, 123U, 138U, 176U, 197U}) {
            uint8_t bytes[kTaskCarrierMaxSize]; memset(bytes, 0x81, sizeof(bytes)); ByteView encoded;
            const auto operation = static_cast<TaskWireOperation>(op);
            CHECK(encodeTaskCarrier(operation, ByteView(bytes + 5U, length), MutableByteSpan(bytes, sizeof(bytes)), encoded));
            TaskWireOperation decoded; ByteView inner;
            CHECK(decodeTaskCarrier(encoded, decoded, inner)); CHECK(decoded == operation && inner.size == length);
            for (size_t i = 0; i < length; ++i) CHECK(inner.data[i] == 0x81U);
            for (size_t i = 0; i < encoded.size; ++i) CHECK(!decodeTaskCarrier(ByteView(bytes, i), decoded, inner) && inner.empty());
        }
    }
    {
        const uint8_t failure[] = {0x82U, 3U, 2U};
        const uint8_t golden[] = {0x83U, 1U, 6U, 0x43U, 0x82U, 3U, 2U}; // Shared Service golden.
        uint8_t bytes[32]; ByteView wire;
        CHECK(encodeTaskCarrier(TaskWireOperation::Failure, ByteView(failure, sizeof(failure)), MutableByteSpan(bytes, sizeof(bytes)), wire));
        CHECK(wire.size == sizeof(golden) && memcmp(wire.data, golden, sizeof(golden)) == 0);
        for (const size_t bad : {0U, 1U, 2U, 3U}) {
            memcpy(bytes, golden, sizeof(golden)); bytes[bad] = 0xFFU;
            TaskWireOperation operation; ByteView inner;
            CHECK(!decodeTaskCarrier(ByteView(bytes, sizeof(golden)), operation, inner) && inner.empty());
        }
    }
    size_t maximumOperation = 0U;
    size_t maximumGrant = 0U;
    for (TaskTimingKind kind : {TaskTimingKind::Countdown, TaskTimingKind::UtcOnce, TaskTimingKind::Weekly}) {
        for (bool floating : {false, true}) {
            uint8_t opBytes[256]; const ByteView op = operation(MutableByteSpan(opBytes, sizeof(opBytes)), kind, floating);
            for (bool delivery : {false, true}) {
                const size_t opSize = routedFrameSize(op, delivery);
                if (opSize > maximumOperation) maximumOperation = opSize;
                CHECK(opSize <= 256U);
                uint8_t grantBytes[kTaskManagementGrantMaxSize];
                const ByteView encodedGrant = grant(MutableByteSpan(grantBytes, sizeof(grantBytes)));
                const size_t grantSize = routedFrameSize(encodedGrant, delivery);
                CHECK(grantSize <= 256U);
                if (grantSize > maximumGrant) maximumGrant = grantSize;
            }
        }
    }
    CHECK(maximumOperation == 182U);
    CHECK(maximumGrant == 235U);
    TaskManagementChallenge challenge;
    memcpy(challenge.authority.deviceInstanceId, identity, 16U);
    memcpy(challenge.authority.taskScope, nonce, 16U);
    memcpy(challenge.authority.rootDigest, digest, 32U);
    memcpy(challenge.nonce, nonce, 16U); memcpy(challenge.requestId, requestId, 16U);
    memcpy(challenge.operationDigest, digest, 32U);
    uint8_t replyBytes[256]; ByteView reply;
    TaskRetirementPrepare retirement;
    retirement.authority = challenge.authority;
    memcpy(retirement.requestId, requestId, 16U);
    CHECK(encodeTaskRetirementPrepare(retirement, MutableByteSpan(replyBytes, sizeof(replyBytes)), reply));
    CHECK(reply.size == kTaskRetirementPrepareMaxSize && reply.size + 15U == 102U);
    TaskManagementResult retired;
    retired.kind = TaskManagementResultKind::Retired; retired.tableRevision = UINT32_MAX;
    memcpy(retired.retirementId, identity, 16U);
    for (TaskResultKey role : {TaskResultKey::DeviceKey, TaskResultKey::Admin}) {
        CHECK(encodeTaskManagementResult(challenge, retired, role, ByteView(digest, sizeof(digest)),
            MutableByteSpan(replyBytes, sizeof(replyBytes)), reply));
        CHECK(reply.size == 78U && reply.size + 15U == 93U);
    }
    CHECK(encodeTaskManagementChallenge(challenge, MutableByteSpan(replyBytes, sizeof(replyBytes)), reply));
    CHECK(reply.size == kTaskManagementChallengeMaxSize);
    for (bool delivery : {false, true}) CHECK(routedFrameSize(reply, delivery) == 197U);
    TaskReadPrepare catalogRead; catalogRead.catalog = true;
    memcpy(catalogRead.requestId, requestId, 16U);
    for (uint32_t index : {UINT32_MAX, 31U}) {
        catalogRead.index = index; memcpy(catalogRead.catalogFingerprint, digest, 32U);
        CHECK(encodeTaskReadPrepare(catalogRead, MutableByteSpan(replyBytes, sizeof(replyBytes)), reply));
        CHECK(reply.size <= kTaskReadPrepareMaxSize);
        TaskReadPrepare decodedRead; CHECK(decodeTaskReadPrepare(reply, decodedRead));
        CHECK(decodedRead.catalog && decodedRead.index == index);
        TaskManagementResult page; page.kind = index == UINT32_MAX ? TaskManagementResultKind::Catalog : TaskManagementResultKind::Target;
        page.index = index; page.tableRevision = UINT32_MAX; page.count = 10U;
        page.catalog.capacity = 10U; page.catalog.total = 32U; memcpy(page.catalog.fingerprint, digest, 32U);
        if (index != UINT32_MAX) { page.catalog.keyLength = 64U; memset(page.catalog.key, 'k', 64U); page.catalog.valueType = ValueType::Float64; }
        for (TaskResultKey role : {TaskResultKey::DeviceKey, TaskResultKey::Admin}) {
            CHECK(encodeTaskManagementResult(challenge, page, role, ByteView(digest, sizeof(digest)), MutableByteSpan(replyBytes, sizeof(replyBytes)), reply));
            CHECK(reply.size + 15U <= kTaskCarrierFrameMaxSize);
            TaskManagementResult verified;
            CHECK(verifyTaskManagementResult(reply, challenge, role, ByteView(digest, sizeof(digest)), verified));
            CHECK(verified.catalog.total == 32U && verified.catalog.capacity == 10U && verified.index == index);
            std::cout << "catalog index=" << index << " physical frame=" << reply.size + 15U << " bytes\n";
        }
    }
    size_t maximumResult = 0U;
    for (TaskMutationRejection reason : {TaskMutationRejection::WriteBudget, TaskMutationRejection::StartCooldown}) {
        TaskManagementResult result; result.kind = TaskManagementResultKind::Rejected;
        result.rejection = reason; result.tableRevision = UINT32_MAX; result.count = 10U;
        CHECK(encodeTaskManagementResult(challenge, result, TaskResultKey::DeviceKey, ByteView(digest, sizeof(digest)),
            MutableByteSpan(replyBytes, sizeof(replyBytes)), reply));
        CHECK(reply.size == 62U); // uint32 revision worst case, no retry-after guess.
        std::cout << "authenticated rejection: inner=" << reply.size << " physical frame=" << reply.size + 15U << " bytes\n";
    }
    for (TaskTimingKind kind : {TaskTimingKind::Countdown, TaskTimingKind::UtcOnce, TaskTimingKind::Weekly}) {
        for (bool floating : {false, true}) {
            uint8_t opBytes[256]; const ByteView op = operation(MutableByteSpan(opBytes, sizeof(opBytes)), kind, floating);
            TaskMutation decoded;
            CHECK(decodeTaskMutationPrepare(op, decoded));
            TaskManagementResult result;
            result.kind = TaskManagementResultKind::Record; result.hasRecord = true;
            result.tableRevision = UINT32_MAX; result.count = 10U; result.index = 9U;
            result.record = decoded.definition;
            result.record.taskId = UINT32_MAX - 1U; result.record.definitionRevision = UINT32_MAX;
            result.record.progress = TaskProgress::Completed;
            if (kind == TaskTimingKind::Countdown) result.record.startSequence = UINT32_MAX;
            else if (kind == TaskTimingKind::UtcOnce) result.record.consumedUtcMillis = result.record.timing.utcMillis;
            else CHECK(nextTaskUtc(result.record.timing, kWallTimeEndMillis - 8ULL * 86400000ULL, result.record.consumedUtcMillis));
            for (TaskResultKey role : {TaskResultKey::DeviceKey, TaskResultKey::Admin}) {
                CHECK(encodeTaskManagementResult(challenge, result, role, ByteView(digest, sizeof(digest)),
                    MutableByteSpan(replyBytes, sizeof(replyBytes)), reply)); // Dummy test key, NOT a root-derived production key.
                TaskManagementResult verified;
                CHECK(verifyTaskManagementResult(reply, challenge, role, ByteView(digest, sizeof(digest)), verified));
                CHECK(verified.record.taskId == result.record.taskId);
                for (bool delivery : {false, true}) {
                    const size_t size = routedFrameSize(reply, delivery);
                    CHECK(size <= 256U);
                    if (size > maximumResult) maximumResult = size;
                }
            }
        }
    }
    for (bool initialize : {true, false}) {
        uint8_t changedRoot[32]; memset(changedRoot, 7U, sizeof(changedRoot));
        TaskAuthorityChange change;
        change.requestId = ByteView(requestId, sizeof(requestId));
        change.deviceInstanceId = ByteView(identity, sizeof(identity)); change.taskScope = ByteView(nonce, sizeof(nonce));
        change.previousRoot = initialize ? ByteView() : ByteView(digest, sizeof(digest));
        change.currentRoot = ByteView(changedRoot, sizeof(changedRoot));
        uint8_t buffer[kTaskAuthorityChangeMaxSize]; ByteView encoded;
        CHECK(encodeTaskAuthorityChange(change, MutableByteSpan(buffer, sizeof(buffer)), encoded));
        CHECK(encoded.size == (initialize ? 88U : 121U));
        for (bool delivery : {false, true}) CHECK(routedFrameSize(encoded, delivery) == (initialize ? 147U : 180U));
    }
    std::cout << "routed frame maxima (including 10B BBP header + 4B CRC): typed-prepare=" << maximumOperation
              << ", grant-v1=" << maximumGrant
              << ", initialize=147, rebind=180 bytes; NOT end-to-end protocol/authentication admission\n";
    std::cout << "challenge=197, maximum authenticated single-record result=" << maximumResult << " bytes\n";
    // Current carrier is Service <-> physical device, NOT Route/Delivery. Its
    // five-byte envelope preserves the independently authenticated inner wire.
    CHECK(maximumOperation - 59U + 15U == 138U);
    CHECK(maximumGrant - 59U + 15U == 191U);
    CHECK(maximumResult - 59U + 15U == 192U);
    static_assert(kTaskCarrierFrameMaxSize == bbp2::kBaseHeaderSize + kTaskCarrierMaxSize, "Carrier capacity mismatch");
    std::cout << "physical task carrier: prepare=138 grant=191 challenge=153 max-record=192; retire/query=102 retired-result=93; conservative cap=212 bytes\n";
    std::cout << "owned read bytes=" << sizeof(TaskReadPrepare) << ", result bytes=" << sizeof(TaskManagementResult)
              << "; transient objects, not per-task storage or measured call-stack peak\n";
}
