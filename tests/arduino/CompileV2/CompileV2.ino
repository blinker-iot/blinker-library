#include <BlinkerV2Advanced.h>

uint8_t frameHeader[blinker::bbp2::kBaseHeaderSize];
uint8_t protocolBuffer[160];
uint8_t reliableFrameBuffer[160];
static const blinker::StringView compileActionValues[] = {
    blinker::StringView("tap"),
    blinker::StringView("on"),
    blinker::StringView("off")
};
static const blinker::PropertyConstraints compileActionConstraints(
    blinker::ConstraintEnumText,
    0.0,
    0.0,
    0.0,
    0U,
    blinker::StringView(),
    compileActionValues,
    sizeof(compileActionValues) / sizeof(compileActionValues[0]));
static const blinker::FieldSpec canonicalFields[] = {
    blinker::FieldSpec(
        "canonical-switch",
        blinker::EndpointKind::Property,
        blinker::ValueType::Boolean,
        static_cast<uint8_t>(
            blinker::AccessRead | blinker::AccessWrite |
            blinker::AccessNotify)),
    blinker::FieldSpec(
        "canonical-command",
        blinker::EndpointKind::Action,
        blinker::ValueType::Text,
        blinker::AccessCommand,
        &compileActionConstraints),
    blinker::FieldSpec(
        "canonical-number",
        blinker::EndpointKind::Property,
        blinker::ValueType::UnsignedInteger,
        static_cast<uint8_t>(
            blinker::AccessRead | blinker::AccessNotify)),
    blinker::FieldSpec(
        "canonical-slider",
        blinker::EndpointKind::Property,
        blinker::ValueType::Float32,
        static_cast<uint8_t>(
            blinker::AccessRead | blinker::AccessWrite |
            blinker::AccessNotify)),
    blinker::FieldSpec(
        "canonical-text",
        blinker::EndpointKind::Property,
        blinker::ValueType::Text,
        static_cast<uint8_t>(
            blinker::AccessRead | blinker::AccessNotify))
};
static const blinker::FieldSpec secondCanonicalFields[] = {
    blinker::FieldSpec(
        "second-flag",
        blinker::EndpointKind::Property,
        blinker::ValueType::Boolean,
        static_cast<uint8_t>(
            blinker::AccessRead | blinker::AccessWrite |
            blinker::AccessNotify))
};
blinker::ClientStorage<5, 192, 64, 48> canonicalStorage;
blinker::ClientStorage<1, 96, 32, 16> secondCanonicalStorage;

BLINKER_PROPERTY(
    compilePublicPower,
    bool,
    blinker::readWrite());
BLINKER_ACTION(compilePublicRestart);
BLINKER_EVENT(compilePublicAlarm, uint32_t);
static const auto compilePublicSchema = blinker::makeDeviceSchema(
    compilePublicPower,
    compilePublicRestart,
    compilePublicAlarm);

class CompileClock : public blinker::IClock
{
public:
    uint32_t monotonicMillis() const override { return 0; }
    bool unixTime(uint64_t&) const override { return false; }
};

blinker::Result acceptCompileStateWrite(
    void*,
    blinker::ByteView,
    bool,
    const blinker::RxContext&)
{
    return blinker::Result::success();
}

void setup()
{
    blinker::DeviceRuntimeConfig canonicalConfig;
    canonicalConfig.protocolFeatures = blinker::bbp2::FeatureStateRevision;
    canonicalConfig.maxFrameSize = 192;
    canonicalConfig.maxReassemblySize = 192;
    blinker::Client canonical(
        canonicalStorage.buffers(canonicalFields), canonicalConfig);
    blinker::EndpointHandle canonicalSwitch =
        canonical.endpoints().find("canonical-switch");
    blinker::EndpointHandle compileNumber =
        canonical.endpoints().find("canonical-number");
    blinker::EndpointHandle compileSlider =
        canonical.endpoints().find("canonical-slider");
    blinker::EndpointHandle compileText =
        canonical.endpoints().find("canonical-text");
    canonical.setStateWriteHandler(&acceptCompileStateWrite, 0);
    blinker::DeviceRuntimeConfig secondCanonicalConfig;
    secondCanonicalConfig.protocolFeatures =
        blinker::bbp2::FeatureStateRevision;
    secondCanonicalConfig.maxFrameSize = 96;
    secondCanonicalConfig.maxReassemblySize = 96;
    blinker::Client secondCanonical(
        secondCanonicalStorage.buffers(secondCanonicalFields),
        secondCanonicalConfig);
    blinker::EndpointHandle secondFlag =
        secondCanonical.endpoints().find("second-flag");
    if (!canonicalSwitch.valid())
    {
        frameHeader[0] = 0;
    }
    if (!secondFlag.valid())
    {
        frameHeader[0] = 0;
    }
    blinker::EndpointValueView currentValue;
    canonicalSwitch.current(currentValue);
    canonicalSwitch.set(true);
    compileNumber.setUnsigned(1);
    compileSlider.set(1.0f);
    compileText.setText(blinker::StringView("ok", 2));
    blinker::StateUpdate compileUpdate(canonical);
    compileUpdate.begin(1);
    compileUpdate.set(canonicalSwitch, true);
    compileUpdate.commit();

    blinker::bbp2::FrameHeader header;
    header.kind = static_cast<uint8_t>(blinker::bbp2::MessageKind::Hello);
    header.bodyLength = 0;

    const blinker::Result result = blinker::bbp2::encodeHeader(
        header,
        blinker::MutableByteSpan(frameHeader, sizeof(frameHeader)));

    if (!result)
    {
        frameHeader[0] = 0;
    }

    blinker::bbp2::HelloBody hello;
    hello.versions[0] = 2;
    hello.versionCount = 1;
    hello.features = blinker::bbp2::FeatureManifest;
    hello.maxFrameSize = 512;
    hello.maxReassemblySize = 1024;
    hello.manifestRevision = 1;
    hello.hasManifestRevision = true;
    static const uint8_t compileFingerprint[blinker::kSha256Size] = {};
    hello.manifestFingerprint = blinker::ByteView(
        compileFingerprint,
        sizeof(compileFingerprint));
    hello.hasManifestFingerprint = true;
    blinker::ByteView encoded;
    blinker::bbp2::encodeHelloBody(
        hello,
        blinker::MutableByteSpan(protocolBuffer, sizeof(protocolBuffer)),
        encoded);

    blinker::bbp2::KeyedBodyWriter keyed(
        blinker::MutableByteSpan(protocolBuffer, sizeof(protocolBuffer)));
    keyed.begin(1);
    keyed.writeBool(blinker::StringView("power", 5), true);
    keyed.finish(encoded);

    static const blinker::EndpointDescriptor endpoint(
        blinker::StringView("lamp", 4),
        blinker::EndpointKind::Property,
        blinker::ValueType::Boolean,
        static_cast<uint8_t>(
            blinker::AccessRead | blinker::AccessWrite));
    const blinker::EndpointDescriptor* endpointStorage[1] = {0};
    blinker::EndpointRegistry registry(endpointStorage, 1);
    registry.add(endpoint);
    blinker::encodeManifest(
        1,
        registry,
        blinker::MutableByteSpan(protocolBuffer, sizeof(protocolBuffer)),
        encoded);

    CompileClock clock;
    blinker::TransportHub hub;
    blinker::ReliableFrameSlot reliableSlots[1];
    reliableSlots[0].storage = blinker::MutableByteSpan(
        reliableFrameBuffer,
        sizeof(reliableFrameBuffer));
    blinker::ReliableOutbox reliableOutbox(
        hub,
        clock,
        reliableSlots,
        1);
    blinker::DeviceRuntimeConfig runtimeConfig;
    runtimeConfig.maxFrameSize = sizeof(protocolBuffer);
    runtimeConfig.maxReassemblySize = sizeof(protocolBuffer);
    blinker::DeviceRuntime runtime(
        hub,
        registry,
        blinker::MutableByteSpan(protocolBuffer, sizeof(protocolBuffer)),
        runtimeConfig);
    runtime.setReliableOutbox(&reliableOutbox);
}

void loop()
{
}
