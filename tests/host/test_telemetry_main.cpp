#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <string.h>

#include "../../src/BlinkerV2/model/EndpointRegistry.h"
#include "../../src/BlinkerV2/protocol/bbp2/Messages.h"
#include "../../src/BlinkerV2/runtime/TelemetryLease.h"

using namespace blinker;

namespace {

void expectBytes(ByteView actual, const uint8_t* expected, size_t size) {
    assert(actual.size == size);
    assert(memcmp(actual.data, expected, size) == 0);
}

bbp2::TelemetryControlBody openControl(
    uint32_t streamId,
    uint16_t first,
    uint16_t second = 0U) {
    bbp2::TelemetryControlBody control;
    control.operation = bbp2::TelemetryControlOperation::Open;
    control.streamId = streamId;
    control.leaseMillis = 30000U;
    control.intervalMillis = 100U;
    control.fieldIds[0] = first;
    control.fieldCount = 1U;
    if (second != 0U) {
        control.fieldIds[1] = second;
        control.fieldCount = 2U;
    }
    return control;
}

} // namespace

int main() {
    static const EndpointDescriptor fields[] = {
        EndpointDescriptor(
            StringView("temperature", 11U),
            EndpointKind::Property,
            ValueType::Float32,
            static_cast<uint8_t>(AccessRead | AccessNotify),
            nullptr,
            200U),
        EndpointDescriptor(
            StringView("power", 5U),
            EndpointKind::Property,
            ValueType::UnsignedInteger,
            static_cast<uint8_t>(AccessRead | AccessNotify),
            nullptr,
            1000U)
    };
    EndpointRegistry endpoints(fields, 2U);
    assert(endpoints.seal());

    uint8_t encodedStorage[128] = {};
    ByteView encoded;
    bbp2::TelemetryControlBody control = openControl(1U, 1U, 2U);
    assert(bbp2::encodeTelemetryControlBody(
        control,
        MutableByteSpan(encodedStorage, sizeof(encodedStorage)),
        encoded));
    static const uint8_t expectedControl[] = {
        0xA5, 0x00, 0x00, 0x01, 0x01, 0x03, 0x19, 0x75,
        0x30, 0x04, 0x18, 0x64, 0x05, 0x82, 0x01, 0x02
    };
    expectBytes(encoded, expectedControl, sizeof(expectedControl));
    bbp2::TelemetryControlBody decodedControl;
    assert(bbp2::decodeTelemetryControlBody(encoded, decodedControl));
    assert(decodedControl.streamId == 1U);
    assert(decodedControl.fieldCount == 2U);
    assert(decodedControl.fieldIds[0] == 1U);
    assert(decodedControl.fieldIds[1] == 2U);

    static const uint8_t valuesBytes[] = {0xA1, 0x01, 0xF5};
    bbp2::TelemetryDataBody data;
    data.streamId = 1U;
    data.streamEpoch = 1U;
    data.sampleSequence = 1U;
    data.monotonicMillis = 0U;
    data.values = ByteView(valuesBytes, sizeof(valuesBytes));
    assert(bbp2::encodeTelemetryDataBody(
        data,
        MutableByteSpan(encodedStorage, sizeof(encodedStorage)),
        encoded));
    static const uint8_t expectedData[] = {
        0xA5, 0x00, 0x01, 0x01, 0x01, 0x02, 0x01,
        0x03, 0x00, 0x04, 0xA1, 0x01, 0xF5
    };
    expectBytes(encoded, expectedData, sizeof(expectedData));
    bbp2::TelemetryDataBody decodedData;
    assert(bbp2::decodeTelemetryDataBody(encoded, decodedData));
    assert(decodedData.values.size == sizeof(valuesBytes));

    TelemetryLeaseTable leases;
    RxContext app;
    app.transportId = 0U;
    app.sessionId = 7U;
    RxContext service;
    service.transportId = 1U;
    service.sessionId = 0U;
    bbp2::TelemetryStatusBody status;
    assert(leases.apply(control, app, endpoints, 100U, status));
    assert(status.status == bbp2::TelemetryStatusCode::Opened);
    assert(status.effectiveIntervalMillis == 1000U);
    const uint32_t appEpoch = status.streamEpoch;
    // Routed retry is idempotent and retains the epoch.
    assert(leases.apply(control, app, endpoints, 200U, status));
    assert(status.streamEpoch == appEpoch);

    // A stream ID is session-local. Cloud and Direct may use the same value.
    bbp2::TelemetryControlBody second = openControl(1U, 1U);
    assert(leases.apply(second, service, endpoints, 200U, status));
    assert(leases.activeCount() == 2U);
    leases.deferSource(app, 200U, 1000U);
    assert(leases.firstDue(1199U) != nullptr);
    assert(leases.firstDue(1199U)->transportId == service.transportId);
    leases.sampled(*leases.firstDue(1199U), 1199U);
    assert(leases.firstDue(1199U) == nullptr);
    assert(leases.firstDue(1200U) != nullptr);
    assert(leases.firstDue(1200U)->transportId == app.transportId);
    bbp2::TelemetryControlBody overflow = openControl(3U, 1U);
    assert(!leases.apply(overflow, app, endpoints, 200U, status));

    TelemetryLeaseTable duplicateTable;
    bbp2::TelemetryControlBody duplicate = openControl(4U, 1U, 1U);
    assert(!bbp2::encodeTelemetryControlBody(
        duplicate,
        MutableByteSpan(encodedStorage, sizeof(encodedStorage)),
        encoded));
    assert(!duplicateTable.apply(duplicate, app, endpoints, 200U, status));

    TelemetryLeaseTable wrappingTable;
    bbp2::TelemetryControlBody wrapping = openControl(5U, 1U);
    wrapping.leaseMillis = 5000U;
    const uint32_t beforeWrap = UINT32_MAX - 1000U;
    assert(wrappingTable.apply(
        wrapping, app, endpoints, beforeWrap, status));
    assert(wrappingTable.firstExpired(3998U) == nullptr);
    assert(wrappingTable.firstExpired(3999U) != nullptr);

    bbp2::TelemetryControlBody renew;
    renew.operation = bbp2::TelemetryControlOperation::Renew;
    renew.streamId = 1U;
    renew.streamEpoch = appEpoch;
    renew.leaseMillis = 10000U;
    assert(leases.apply(renew, app, endpoints, 300U, status));
    assert(status.status == bbp2::TelemetryStatusCode::Renewed);
    renew.streamEpoch += 1U;
    assert(!leases.apply(renew, app, endpoints, 300U, status));

    leases.sessionClosed(app);
    assert(leases.activeCount() == 1U);
    assert(leases.firstExpired(30199U) == nullptr);
    TelemetryLeaseSlot* expired = leases.firstExpired(30200U);
    assert(expired != nullptr && expired->streamId == 1U);
    leases.close(*expired);
    assert(leases.activeCount() == 0U);

    assert(sizeof(TelemetryLeaseSlot) <= 48U);
    return 0;
}
