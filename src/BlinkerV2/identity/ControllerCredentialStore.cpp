#include "ControllerCredentialStore.h"

#include <string.h>

#include "../core/Crc32.h"
#include "../core/SecureMemory.h"

namespace blinker {

namespace {

const uint8_t kMagic[4] = {'B', 'C', 'T', '2'};
const uint8_t kSchemaVersion = 1U;
const size_t kHeaderSize = 8U;
const size_t kCrcOffset = ControllerCredentialStore::serializedSize - 4U;

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

Result encodeTable(
    const ControllerCredentialTable& table,
    MutableByteSpan output) {
    if (output.data == nullptr ||
        output.size < ControllerCredentialStore::serializedSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    memset(output.data, 0, ControllerCredentialStore::serializedSize);
    memcpy(output.data, kMagic, sizeof(kMagic));
    output.data[4] = kSchemaVersion;
    output.data[5] = static_cast<uint8_t>(
        ControllerCredentialTable::capacity);
    output.data[6] = static_cast<uint8_t>(table.count());
    Result result = table.encode(MutableByteSpan(
        output.data + kHeaderSize,
        ControllerCredentialTable::encodedSize));
    if (result) {
        writeU32(
            output.data + kCrcOffset,
            computeCrc32(ByteView(output.data, kCrcOffset)));
    }
    return result;
}

Result decodeTable(
    ByteView input,
    ControllerCredentialTable& table) {
    if (input.data == nullptr ||
        input.size != ControllerCredentialStore::serializedSize ||
        memcmp(input.data, kMagic, sizeof(kMagic)) != 0 ||
        input.data[4] != kSchemaVersion ||
        input.data[5] != static_cast<uint8_t>(
            ControllerCredentialTable::capacity) ||
        input.data[6] > ControllerCredentialTable::capacity ||
        input.data[7] != 0U ||
        readU32(input.data + kCrcOffset) !=
            computeCrc32(ByteView(input.data, kCrcOffset))) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    const Result result = table.decode(
        ByteView(
            input.data + kHeaderSize,
            ControllerCredentialTable::encodedSize),
        input.data[6]);
    return result.code() == ErrorCode::InvalidArgument
               ? Result::failure(ErrorCode::InvalidEncoding)
               : result;
}

} // namespace

Result ControllerCredentialStore::loadTable(
    ControllerCredentialTable& table) {
    uint8_t encoded[serializedSize] = {};
    size_t written = 0U;
    Result result = storage_.load(
        MutableByteSpan(encoded, sizeof(encoded)), written);
    if (result && written != sizeof(encoded)) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    if (result) result = decodeTable(ByteView(encoded, written), table);
    secureZero(MutableByteSpan(encoded, sizeof(encoded)));
    return result;
}

Result ControllerCredentialStore::replaceAndVerify(
    const ControllerCredentialTable& table) {
    uint8_t encoded[serializedSize] = {};
    Result result = encodeTable(
        table, MutableByteSpan(encoded, sizeof(encoded)));
    if (result) result = storage_.replace(ByteView(encoded, sizeof(encoded)));
    secureZero(MutableByteSpan(encoded, sizeof(encoded)));
    if (!result) return result;

    ControllerCredentialTable verified;
    result = loadTable(verified);
    bool same = result && verified.count() == table.count();
    for (size_t index = 0U; same && index < table.count(); ++index) {
        same = sameControllerCredential(
            verified.at(index), table.at(index));
    }
    return same ? Result::success()
                : (result ? Result::failure(ErrorCode::InternalError) : result);
}

Result ControllerCredentialStore::clearAndVerify() {
    Result result = storage_.clear();
    if (!result && result.code() != ErrorCode::NotFound) return result;
    ControllerCredentialTable ignored;
    result = loadTable(ignored);
    return result.code() == ErrorCode::NotFound
               ? Result::success()
               : (result ? Result::failure(ErrorCode::InternalError) : result);
}

Result ControllerCredentialStore::count(size_t& output) {
    output = 0U;
    ControllerCredentialTable table;
    Result result = loadTable(table);
    if (!result && result.code() == ErrorCode::NotFound) {
        return Result::success();
    }
    if (result) output = table.count();
    return result;
}

Result ControllerCredentialStore::loadAt(
    size_t index,
    ControllerCredential& output) {
    ControllerCredentialTable table;
    Result result = loadTable(table);
    if (result && index >= table.count()) {
        result = Result::failure(ErrorCode::NotFound);
    }
    if (result) output = table.at(index);
    return result;
}

Result ControllerCredentialStore::loadActive(
    ByteView controllerId,
    ControllerCredentialDomain domain,
    uint32_t accessEpoch,
    ControllerCredential& output) {
    ControllerCredentialTable table;
    Result result = loadTable(table);
    if (result) {
        result = table.loadActive(
            controllerId, domain, accessEpoch, output);
    }
    return result;
}

Result ControllerCredentialStore::installVerified(
    const ControllerCredential& credential) {
    ControllerCredentialTable table;
    Result result = loadTable(table);
    if (!result && result.code() == ErrorCode::NotFound) {
        result = Result::success();
    }
    bool changed = false;
    if (result) result = table.install(credential, changed);
    return result && changed ? replaceAndVerify(table) : result;
}

Result ControllerCredentialStore::rotateVerified(
    uint32_t expectedCredentialVersion,
    const ControllerCredential& credential) {
    ControllerCredentialTable table;
    Result result = loadTable(table);
    bool changed = false;
    if (result) {
        result = table.rotate(
            expectedCredentialVersion, credential, changed);
    }
    return result && changed ? replaceAndVerify(table) : result;
}

Result ControllerCredentialStore::revokeVerified(
    ByteView controllerId,
    ControllerCredentialDomain domain,
    uint32_t accessEpoch,
    uint32_t expectedCredentialVersion) {
    ControllerCredentialTable table;
    Result result = loadTable(table);
    if (!result && result.code() == ErrorCode::NotFound) {
        return Result::success();
    }
    bool changed = false;
    if (result) {
        result = table.revoke(
            controllerId,
            domain,
            accessEpoch,
            expectedCredentialVersion,
            changed);
    }
    if (!result || !changed) return result;
    return table.count() == 0U
               ? clearAndVerify()
               : replaceAndVerify(table);
}

Result ControllerCredentialStore::clearAll() {
    return clearAndVerify();
}

} // namespace blinker
