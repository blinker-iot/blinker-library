#include <cerrno>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "BlinkerV2/core/Sha256.h"
#include "BlinkerV2/model/Manifest.h"
#include "BlinkerV2/protocol/bbp2/Frame.h"
#include "BlinkerV2/protocol/bbp2/Messages.h"
#include "BlinkerV2/protocol/ble/Fragment.h"
#include "BlinkerV2/protocol/cbor/Cbor.h"

namespace {

typedef std::map<std::string, std::string> Properties;

struct OwnedField {
    std::string key;
    std::string unit;
    std::vector<std::string> enumValues;
    std::vector<blinker::StringView> enumViews;
    blinker::PropertyConstraints constraints;
    bool hasConstraints;

    OwnedField() : hasConstraints(false) {}
};

void fail(const std::string& message) {
    std::cerr << "golden C++: " << message << std::endl;
    std::exit(1);
}

Properties loadProperties(const char* path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) fail(std::string("cannot open fixture: ") + path);
    Properties properties;
    std::string line;
    size_t lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        if (!line.empty() && line[line.size() - 1U] == '\r') {
            line.erase(line.size() - 1U);
        }
        if (line.empty() || line[0] == '#') continue;
        const size_t separator = line.find('=');
        if (separator == std::string::npos || separator == 0U) {
            std::ostringstream message;
            message << "invalid fixture line " << lineNumber;
            fail(message.str());
        }
        const std::string key = line.substr(0, separator);
        if (!properties.insert(
                std::make_pair(key, line.substr(separator + 1U))).second) {
            fail(std::string("duplicate fixture key: ") + key);
        }
    }
    return properties;
}

const std::string& require(const Properties& properties, const std::string& key) {
    const Properties::const_iterator found = properties.find(key);
    if (found == properties.end()) fail(std::string("missing fixture key: ") + key);
    return found->second;
}

bool has(const Properties& properties, const std::string& key) {
    return properties.find(key) != properties.end();
}

uint64_t parseUnsigned(const std::string& value, const std::string& key) {
    if (value.empty() || value[0] == '-') fail(std::string("invalid unsigned: ") + key);
    char* end = nullptr;
    errno = 0;
    const unsigned long long parsed = std::strtoull(value.c_str(), &end, 10);
    if (errno != 0 || end == nullptr || *end != '\0') {
        fail(std::string("invalid unsigned: ") + key);
    }
    return static_cast<uint64_t>(parsed);
}

int64_t parseSigned(const std::string& value, const std::string& key) {
    char* end = nullptr;
    errno = 0;
    const long long parsed = std::strtoll(value.c_str(), &end, 10);
    if (errno != 0 || end == nullptr || *end != '\0') {
        fail(std::string("invalid signed: ") + key);
    }
    return static_cast<int64_t>(parsed);
}

double parseDouble(const std::string& value, const std::string& key) {
    char* end = nullptr;
    errno = 0;
    const double parsed = std::strtod(value.c_str(), &end);
    if (errno != 0 || end == nullptr || *end != '\0' || !std::isfinite(parsed)) {
        fail(std::string("invalid double: ") + key);
    }
    return parsed;
}

std::vector<std::string> split(const std::string& value, char separator) {
    std::vector<std::string> parts;
    size_t start = 0;
    for (;;) {
        const size_t end = value.find(separator, start);
        parts.push_back(value.substr(start, end - start));
        if (end == std::string::npos) break;
        start = end + 1U;
    }
    return parts;
}

std::vector<uint8_t> fromHex(const std::string& value) {
    if ((value.size() & 1U) != 0U) fail("odd hex length");
    std::vector<uint8_t> output(value.size() / 2U);
    for (size_t index = 0; index < output.size(); ++index) {
        const std::string byte = value.substr(index * 2U, 2U);
        char* end = nullptr;
        const unsigned long parsed = std::strtoul(byte.c_str(), &end, 16);
        if (end == nullptr || *end != '\0' || parsed > 0xFFUL) fail("invalid hex");
        output[index] = static_cast<uint8_t>(parsed);
    }
    return output;
}

std::string toHex(blinker::ByteView bytes) {
    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for (size_t index = 0; index < bytes.size; ++index) {
        output << std::setw(2) << static_cast<unsigned int>(bytes.data[index]);
    }
    return output.str();
}

std::string errorName(blinker::ErrorCode code) {
    switch (code) {
        case blinker::ErrorCode::Ok: return "ok";
        case blinker::ErrorCode::InvalidArgument: return "invalid_argument";
        case blinker::ErrorCode::InvalidMagic: return "invalid_magic";
        case blinker::ErrorCode::UnsupportedVersion: return "unsupported_version";
        case blinker::ErrorCode::UnsupportedFeature: return "unsupported_feature";
        case blinker::ErrorCode::InvalidHeader: return "invalid_header";
        case blinker::ErrorCode::InvalidEncoding: return "invalid_encoding";
        case blinker::ErrorCode::NonCanonicalEncoding:
            return "non_canonical_encoding";
        case blinker::ErrorCode::TruncatedInput: return "truncated_input";
        case blinker::ErrorCode::TrailingData: return "trailing_data";
        case blinker::ErrorCode::DuplicateField: return "duplicate_field";
        default: return "other";
    }
}

void requireOk(const blinker::Result& result, const std::string& operation) {
    if (!result) fail(operation + " failed: " + errorName(result.code()));
}

std::string fieldKey(size_t index, const char* suffix) {
    std::ostringstream key;
    key << "manifest." << index << '.' << suffix;
    return key.str();
}

void buildFields(
    const Properties& properties,
    std::vector<OwnedField>& owned,
    std::vector<blinker::FieldSpec>& fields) {
    const size_t count = static_cast<size_t>(parseUnsigned(
        require(properties, "manifest.count"), "manifest.count"));
    owned.resize(count);
    fields.reserve(count);
    for (size_t index = 0; index < count; ++index) {
        OwnedField& field = owned[index];
        field.key = require(properties, fieldKey(index, "key"));
        const uint64_t kind = parseUnsigned(
            require(properties, fieldKey(index, "kind")), fieldKey(index, "kind"));
        const uint64_t type = parseUnsigned(
            require(properties, fieldKey(index, "type")), fieldKey(index, "type"));
        const uint64_t access = parseUnsigned(
            require(properties, fieldKey(index, "access")), fieldKey(index, "access"));
        if (kind > 2U || type > 9U || access > 0xFFU) fail("field enum out of range");

        const std::string minimumKey = fieldKey(index, "minimum");
        const std::string maximumKey = fieldKey(index, "maximum");
        const std::string stepKey = fieldKey(index, "step");
        const std::string maxLengthKey = fieldKey(index, "max_length");
        const std::string unitKey = fieldKey(index, "unit");
        const std::string enumKey = fieldKey(index, "enum");
        uint8_t flags = blinker::ConstraintNone;
        if (has(properties, minimumKey)) {
            flags = static_cast<uint8_t>(flags | blinker::ConstraintMinimum);
            field.constraints.minimum = parseDouble(require(properties, minimumKey), minimumKey);
        }
        if (has(properties, maximumKey)) {
            flags = static_cast<uint8_t>(flags | blinker::ConstraintMaximum);
            field.constraints.maximum = parseDouble(require(properties, maximumKey), maximumKey);
        }
        if (has(properties, stepKey)) {
            flags = static_cast<uint8_t>(flags | blinker::ConstraintStep);
            field.constraints.step = parseDouble(require(properties, stepKey), stepKey);
        }
        if (has(properties, maxLengthKey)) {
            flags = static_cast<uint8_t>(flags | blinker::ConstraintMaxLength);
            const uint64_t maximumLength = parseUnsigned(
                require(properties, maxLengthKey), maxLengthKey);
            if (maximumLength > UINT32_MAX) fail("max_length out of range");
            field.constraints.maxLength = static_cast<uint32_t>(maximumLength);
        }
        if (has(properties, unitKey)) {
            flags = static_cast<uint8_t>(flags | blinker::ConstraintUnit);
            field.unit = require(properties, unitKey);
            field.constraints.unit = blinker::StringView(field.unit.data(), field.unit.size());
        }
        if (has(properties, enumKey)) {
            flags = static_cast<uint8_t>(flags | blinker::ConstraintEnumText);
            field.enumValues = split(require(properties, enumKey), '|');
            field.enumViews.reserve(field.enumValues.size());
            for (size_t enumIndex = 0; enumIndex < field.enumValues.size(); ++enumIndex) {
                const std::string& item = field.enumValues[enumIndex];
                field.enumViews.push_back(blinker::StringView(item.data(), item.size()));
            }
            field.constraints.enumTextValues = field.enumViews.data();
            field.constraints.enumTextCount = field.enumViews.size();
        }
        field.constraints.flags = flags;
        field.hasConstraints = flags != blinker::ConstraintNone;
        fields.push_back(blinker::FieldSpec(
            blinker::StringView(field.key.data(), field.key.size()),
            static_cast<blinker::EndpointKind>(kind),
            static_cast<blinker::ValueType>(type),
            static_cast<uint8_t>(access),
            field.hasConstraints ? &field.constraints : nullptr));
    }
}

void encodeValue(
    blinker::cbor::Writer& writer,
    const std::string& type,
    const std::string& value,
    const std::string& key) {
    blinker::Result result;
    if (type == "bool") {
        if (value != "true" && value != "false") fail("invalid bool: " + key);
        result = writer.writeBool(value == "true");
    } else if (type == "int") {
        result = writer.writeInt(parseSigned(value, key));
    } else if (type == "uint") {
        result = writer.writeUnsigned(parseUnsigned(value, key));
    } else if (type == "f32") {
        result = writer.writeFloat32(static_cast<float>(parseDouble(value, key)));
    } else if (type == "f64") {
        result = writer.writeFloat64(parseDouble(value, key));
    } else if (type == "text") {
        result = writer.writeText(blinker::StringView(value.data(), value.size()));
    } else if (type == "bytes") {
        const std::vector<uint8_t> bytes = fromHex(value);
        result = writer.writeBytes(blinker::ByteView(bytes.data(), bytes.size()));
    } else if (type == "int_array") {
        const std::vector<std::string> items = split(value, '|');
        result = writer.beginArray(items.size());
        for (size_t index = 0; result && index < items.size(); ++index) {
            result = writer.writeInt(parseSigned(items[index], key));
        }
    } else {
        fail("unknown state type: " + type);
    }
    requireOk(result, "encode " + key);
}

std::vector<uint8_t> encodeStateValues(
    const Properties& properties,
    const std::vector<OwnedField>& fields,
    bool idMode) {
    const size_t count = static_cast<size_t>(parseUnsigned(
        require(properties, "state.count"), "state.count"));
    if (count != fields.size()) fail("state.count must equal manifest.count");
    std::vector<uint8_t> storage(4096U);
    blinker::cbor::Writer writer(blinker::MutableByteSpan(storage.data(), storage.size()));
    requireOk(writer.beginMap(count), "begin state map");
    for (size_t index = 0; index < count; ++index) {
        if (idMode) {
            requireOk(writer.writeUnsigned(index + 1U), "write state id");
        } else {
            requireOk(
                writer.writeText(blinker::StringView(
                    fields[index].key.data(), fields[index].key.size())),
                "write state key");
        }
        std::ostringstream prefix;
        prefix << "state." << index;
        encodeValue(
            writer,
            require(properties, prefix.str() + ".type"),
            require(properties, prefix.str() + ".value"),
            prefix.str() + ".value");
    }
    storage.resize(writer.size());
    return storage;
}

std::vector<uint8_t> encodeStatePage(
    const Properties& properties,
    const std::vector<uint8_t>& values) {
    const uint64_t revision = parseUnsigned(
        require(properties, "state.revision"), "state.revision");
    const uint64_t count = parseUnsigned(
        require(properties, "state.count"), "state.count");
    if (revision > UINT32_MAX || count > UINT16_MAX) fail("state page input out of range");
    blinker::bbp2::StatePageBody body;
    body.revision = static_cast<uint32_t>(revision);
    body.cursor = 0;
    body.nextCursor = static_cast<uint16_t>(count);
    body.totalFields = static_cast<uint16_t>(count);
    body.values = blinker::ByteView(values.data(), values.size());
    std::vector<uint8_t> output(4096U);
    blinker::ByteView encoded;
    requireOk(
        blinker::bbp2::encodeStatePageBody(
            body, blinker::MutableByteSpan(output.data(), output.size()), encoded),
        "encode StatePage");
    output.resize(encoded.size);
    return output;
}

std::vector<uint8_t> encodeFrame(
    const Properties& properties,
    const std::vector<uint8_t>& body,
    bool idMode) {
    const uint64_t sequence = parseUnsigned(
        require(properties, "frame.sequence"), "frame.sequence");
    if (sequence + (idMode ? 1U : 0U) > UINT16_MAX || body.size() > UINT16_MAX) {
        fail("frame input out of range");
    }
    blinker::bbp2::FrameHeader header;
    header.kind = static_cast<uint8_t>(blinker::bbp2::MessageKind::StatePage);
    header.flags = static_cast<uint8_t>(
        blinker::bbp2::FlagIsResponse |
        (idMode ? blinker::bbp2::FlagIdMode : blinker::bbp2::FlagNone));
    header.sequence = static_cast<uint16_t>(sequence + (idMode ? 1U : 0U));
    header.bodyLength = static_cast<uint16_t>(body.size());
    std::vector<uint8_t> output(blinker::bbp2::kBaseHeaderSize + body.size());
    requireOk(
        blinker::bbp2::encodeHeader(
            header, blinker::MutableByteSpan(output.data(), output.size())),
        "encode frame header");
    std::memcpy(output.data() + blinker::bbp2::kBaseHeaderSize, body.data(), body.size());
    return output;
}

std::vector<std::vector<uint8_t> > fragment(
    const std::vector<uint8_t>& frame,
    uint8_t frameId,
    size_t packetSize) {
    if (packetSize <= blinker::ble::kFragmentHeaderSize) fail("invalid packet size");
    const size_t capacity = packetSize - blinker::ble::kFragmentHeaderSize;
    const size_t count = (frame.size() + capacity - 1U) / capacity;
    if (count == 0U || count > blinker::ble::kMaximumFragmentCount) {
        fail("fragment count out of range");
    }
    std::vector<std::vector<uint8_t> > packets;
    packets.reserve(count);
    size_t offset = 0;
    for (size_t index = 0; index < count; ++index) {
        const size_t remaining = frame.size() - offset;
        const size_t payload = remaining < capacity ? remaining : capacity;
        std::vector<uint8_t> packet(blinker::ble::kFragmentHeaderSize + payload);
        blinker::ble::FragmentHeader header;
        header.frameId = frameId;
        header.index = static_cast<uint8_t>(index);
        header.flags = blinker::ble::FragmentFlagNone;
        if (index == 0U) {
            header.flags = static_cast<uint8_t>(
                header.flags | blinker::ble::FragmentFlagStart);
        }
        if (index + 1U == count) {
            header.flags = static_cast<uint8_t>(
                header.flags | blinker::ble::FragmentFlagEnd);
        }
        requireOk(
            blinker::ble::encodeFragmentHeader(
                header, blinker::MutableByteSpan(packet.data(), packet.size())),
            "encode BLE fragment");
        std::memcpy(packet.data() + blinker::ble::kFragmentHeaderSize,
                    frame.data() + offset,
                    payload);
        packets.push_back(packet);
        offset += payload;
    }
    return packets;
}

void addOutput(
    std::map<std::string, std::string>& outputs,
    const std::string& key,
    const std::vector<uint8_t>& value) {
    outputs[key] = toHex(blinker::ByteView(value.data(), value.size()));
}

void checkInvalid(
    const Properties& properties,
    const std::string& prefix,
    const blinker::Result& result) {
    const std::string expected = require(properties, prefix + ".error");
    const std::string actual = errorName(result.code());
    if (actual != expected) {
        fail(prefix + " expected " + expected + ", got " + actual);
    }
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2 || argc > 3) {
        std::cerr << "usage: golden_main <fixture> [--emit]" << std::endl;
        return 2;
    }
    const bool emit = argc == 3 && std::string(argv[2]) == "--emit";
    const Properties properties = loadProperties(argv[1]);
    if (require(properties, "fixture.version") != "1") fail("unsupported fixture version");

    std::vector<OwnedField> owned;
    std::vector<blinker::FieldSpec> fields;
    buildFields(properties, owned, fields);
    blinker::EndpointRegistry registry(fields.data(), fields.size());
    requireOk(registry.seal(), "seal registry");

    const uint64_t revisionValue = parseUnsigned(
        require(properties, "manifest.revision"), "manifest.revision");
    if (revisionValue > UINT32_MAX) fail("manifest revision out of range");
    const uint32_t revision = static_cast<uint32_t>(revisionValue);
    std::map<std::string, std::string> outputs;

    std::vector<uint8_t> manifestStorage(8192U);
    blinker::ByteView manifest;
    requireOk(
        blinker::encodeManifest(
            revision,
            registry,
            blinker::MutableByteSpan(manifestStorage.data(), manifestStorage.size()),
            manifest),
        "encode Manifest");
    manifestStorage.resize(manifest.size);
    addOutput(outputs, "expected.manifest", manifestStorage);

    std::vector<uint8_t> fingerprint(blinker::kSha256Size);
    requireOk(
        blinker::computeManifestFingerprint(
            revision,
            registry,
            blinker::MutableByteSpan(fingerprint.data(), fingerprint.size())),
        "fingerprint Manifest");
    addOutput(outputs, "expected.manifest_sha256", fingerprint);

    const size_t pageCapacity = static_cast<size_t>(parseUnsigned(
        require(properties, "manifest.page_body_capacity"),
        "manifest.page_body_capacity"));
    uint16_t cursor = 0;
    size_t pageIndex = 0;
    while (cursor < fields.size()) {
        std::vector<uint8_t> pageStorage(pageCapacity);
        blinker::ByteView page;
        uint16_t nextCursor = 0;
        requireOk(
            blinker::encodeManifestPage(
                revision,
                registry,
                blinker::ByteView(fingerprint.data(), fingerprint.size()),
                cursor,
                blinker::MutableByteSpan(pageStorage.data(), pageStorage.size()),
                page,
                nextCursor),
            "encode Manifest page");
        if (nextCursor <= cursor) fail("Manifest page did not advance");
        pageStorage.resize(page.size);
        std::ostringstream key;
        key << "expected.manifest_page." << pageIndex;
        addOutput(outputs, key.str(), pageStorage);
        cursor = nextCursor;
        ++pageIndex;
    }
    {
        std::ostringstream count;
        count << pageIndex;
        outputs["expected.manifest_page.count"] = count.str();
    }

    blinker::bbp2::HelloBody deviceHello;
    deviceHello.role = blinker::bbp2::PeerRole::Device;
    deviceHello.versions[0] = blinker::bbp2::kVersion;
    deviceHello.versionCount = 1U;
    deviceHello.features =
        blinker::bbp2::FeatureManifest |
        blinker::bbp2::FeatureEndpointIds;
    const uint64_t helloMaxFrame = parseUnsigned(
        require(properties, "hello.max_frame"), "hello.max_frame");
    const uint64_t helloMaxReassembly = parseUnsigned(
        require(properties, "hello.max_reassembly"),
        "hello.max_reassembly");
    if (helloMaxFrame > UINT16_MAX || helloMaxReassembly > UINT32_MAX) {
        fail("HELLO size out of range");
    }
    deviceHello.maxFrameSize = static_cast<uint16_t>(helloMaxFrame);
    deviceHello.maxReassemblySize =
        static_cast<uint32_t>(helloMaxReassembly);
    deviceHello.manifestRevision = revision;
    deviceHello.manifestFingerprint = blinker::ByteView(
        fingerprint.data(), fingerprint.size());
    deviceHello.hasManifestRevision = true;
    deviceHello.hasManifestFingerprint = true;
    std::vector<uint8_t> helloStorage(256U);
    blinker::ByteView helloBody;
    requireOk(
        blinker::bbp2::encodeHelloBody(
            deviceHello,
            blinker::MutableByteSpan(
                helloStorage.data(), helloStorage.size()),
            helloBody),
        "encode device HELLO");
    helloStorage.resize(helloBody.size);
    addOutput(outputs, "expected.hello.device", helloStorage);
    std::vector<uint8_t> requestStorage(64U);
    blinker::ByteView request;
    blinker::bbp2::StateRequestBody initialRequest;
    requireOk(
        blinker::bbp2::encodeStateRequestBody(
            initialRequest,
            blinker::MutableByteSpan(requestStorage.data(), requestStorage.size()),
            request),
        "encode initial StateRequest");
    outputs["expected.state_request.initial"] = toHex(request);
    blinker::bbp2::StateRequestBody continuationRequest;
    const uint64_t requestCursor = parseUnsigned(
        require(properties, "state.request.cursor"), "state.request.cursor");
    const uint64_t observedRevision = parseUnsigned(
        require(properties, "state.request.observed_revision"),
        "state.request.observed_revision");
    if (requestCursor == 0U || requestCursor > UINT16_MAX || observedRevision > UINT32_MAX) {
        fail("continuation StateRequest input out of range");
    }
    continuationRequest.cursor = static_cast<uint16_t>(requestCursor);
    continuationRequest.observedRevision = static_cast<uint32_t>(observedRevision);
    continuationRequest.hasObservedRevision = true;
    requireOk(
        blinker::bbp2::encodeStateRequestBody(
            continuationRequest,
            blinker::MutableByteSpan(requestStorage.data(), requestStorage.size()),
            request),
        "encode continuation StateRequest");
    outputs["expected.state_request.continuation"] = toHex(request);

    const std::vector<uint8_t> keyedValues = encodeStateValues(properties, owned, false);
    const std::vector<uint8_t> idValues = encodeStateValues(properties, owned, true);
    addOutput(outputs, "expected.state_values.keyed", keyedValues);
    addOutput(outputs, "expected.state_values.id", idValues);
    const std::vector<uint8_t> keyedPage = encodeStatePage(properties, keyedValues);
    const std::vector<uint8_t> idPage = encodeStatePage(properties, idValues);
    addOutput(outputs, "expected.state_page.keyed", keyedPage);
    addOutput(outputs, "expected.state_page.id", idPage);
    const std::vector<uint8_t> keyedFrame = encodeFrame(properties, keyedPage, false);
    const std::vector<uint8_t> idFrame = encodeFrame(properties, idPage, true);
    addOutput(outputs, "expected.frame.state_page_keyed", keyedFrame);
    addOutput(outputs, "expected.frame.state_page_id", idFrame);

    const uint64_t frameIdValue = parseUnsigned(
        require(properties, "ble.frame_id"), "ble.frame_id");
    if (frameIdValue == 0U || frameIdValue > 0xFFU) fail("BLE frame_id out of range");
    const uint8_t frameId = static_cast<uint8_t>(frameIdValue);
    const size_t packetSizes[] = {20U, 244U};
    for (size_t sizeIndex = 0; sizeIndex < 2U; ++sizeIndex) {
        const std::vector<std::vector<uint8_t> > packets =
            fragment(keyedFrame, frameId, packetSizes[sizeIndex]);
        std::ostringstream prefix;
        prefix << "expected.ble.att" << packetSizes[sizeIndex];
        std::ostringstream count;
        count << packets.size();
        outputs[prefix.str() + ".count"] = count.str();
        for (size_t index = 0; index < packets.size(); ++index) {
            std::ostringstream key;
            key << prefix.str() << '.' << index;
            addOutput(outputs, key.str(), packets[index]);
        }
    }

    if (emit) {
        for (std::map<std::string, std::string>::const_iterator item = outputs.begin();
             item != outputs.end();
             ++item) {
            std::cout << item->first << '=' << item->second << '\n';
        }
    } else {
        for (std::map<std::string, std::string>::const_iterator item = outputs.begin();
             item != outputs.end();
             ++item) {
            const std::string actual = item->second;
            const std::string expected = require(properties, item->first);
            if (actual != expected) fail(item->first + " byte mismatch");
        }
    }

    const std::vector<uint8_t> unknownFlags = fromHex(
        require(properties, "invalid.frame.unknown_flags.hex"));
    blinker::bbp2::FrameView frameView;
    checkInvalid(
        properties,
        "invalid.frame.unknown_flags",
        blinker::bbp2::parseFrame(
            blinker::ByteView(unknownFlags.data(), unknownFlags.size()), frameView));
    const std::vector<uint8_t> truncated = fromHex(
        require(properties, "invalid.frame.truncated.hex"));
    checkInvalid(
        properties,
        "invalid.frame.truncated",
        blinker::bbp2::parseFrame(
            blinker::ByteView(truncated.data(), truncated.size()), frameView));

    blinker::bbp2::StateRequestBody decodedRequest;
    const std::vector<uint8_t> duplicate = fromHex(
        require(properties, "invalid.state_request.duplicate.hex"));
    checkInvalid(
        properties,
        "invalid.state_request.duplicate",
        blinker::bbp2::decodeStateRequestBody(
            blinker::ByteView(duplicate.data(), duplicate.size()), decodedRequest));
    const std::vector<uint8_t> noncanonical = fromHex(
        require(properties, "invalid.state_request.noncanonical.hex"));
    checkInvalid(
        properties,
        "invalid.state_request.noncanonical",
        blinker::bbp2::decodeStateRequestBody(
            blinker::ByteView(noncanonical.data(), noncanonical.size()), decodedRequest));

    blinker::ble::FragmentView fragmentView;
    const std::vector<uint8_t> unknownBleFlags = fromHex(
        require(properties, "invalid.ble.unknown_flags.hex"));
    checkInvalid(
        properties,
        "invalid.ble.unknown_flags",
        blinker::ble::parseFragment(
            blinker::ByteView(unknownBleFlags.data(), unknownBleFlags.size()), fragmentView));
    const std::vector<uint8_t> startMismatch = fromHex(
        require(properties, "invalid.ble.start_index_mismatch.hex"));
    checkInvalid(
        properties,
        "invalid.ble.start_index_mismatch",
        blinker::ble::parseFragment(
            blinker::ByteView(startMismatch.data(), startMismatch.size()), fragmentView));

    std::cout << "golden C++: " << outputs.size()
              << " exact outputs and 6 invalid vectors passed" << std::endl;
    return 0;
}
