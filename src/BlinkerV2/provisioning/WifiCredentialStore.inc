#include "WifiCredentialStore.h"

#include <limits.h>
#include <string.h>

#include "../core/Crc32.h"
#include "../core/SecureMemory.h"

namespace blinker {

namespace {

const uint8_t kProfileMagic[4] = {'B', 'W', 'F', '2'};
const uint8_t kSelectorMagic[4] = {'B', 'W', 'S', '2'};
const uint8_t kSchemaVersion = 1U;
const uint8_t kNoActiveSlot = 0xFFU;

void wifiStoreWriteU32(uint8_t* output, uint32_t value) {
    output[0] = static_cast<uint8_t>(value >> 24U);
    output[1] = static_cast<uint8_t>(value >> 16U);
    output[2] = static_cast<uint8_t>(value >> 8U);
    output[3] = static_cast<uint8_t>(value);
}

uint32_t wifiStoreReadU32(const uint8_t* input) {
    return (static_cast<uint32_t>(input[0]) << 24U) |
           (static_cast<uint32_t>(input[1]) << 16U) |
           (static_cast<uint32_t>(input[2]) << 8U) |
           static_cast<uint32_t>(input[3]);
}

bool allZero(const uint8_t* data, size_t size) {
    uint8_t combined = 0U;
    for (size_t index = 0U; index < size; ++index) {
        combined = static_cast<uint8_t>(combined | data[index]);
    }
    return combined == 0U;
}

bool erasedResult(const Result& result) {
    return result.ok() || result.code() == ErrorCode::NotFound;
}

} // namespace

WifiCredentialProfile::WifiCredentialProfile()
    : generation(0U),
      authentication(WifiAuthentication::Open),
      ssidLength(0U),
      credentialLength(0U),
      flags(0U),
      ssid(),
      credential() {}

void clearWifiCredentialProfile(WifiCredentialProfile& profile) {
    secureZero(MutableByteSpan(
        reinterpret_cast<uint8_t*>(&profile),
        sizeof(profile)));
    profile.authentication = WifiAuthentication::Open;
}

WifiCredentialStore::WifiCredentialStore(
    IAtomicBlobStore& selector,
    IAtomicBlobStore& slot0,
    IAtomicBlobStore& slot1,
    const WifiCredentialStoreConfig& config)
    : selector_(selector),
      slot0_(slot0),
      slot1_(slot1),
      config_(config),
      activeGeneration_(0U),
      activeSlot_(kNoActiveSlot),
      ready_(false),
      hasActive_(false) {}

bool WifiCredentialStore::protectedStorage() const {
    return config_.allowPlainFlash ||
           (selector_.protection() != StorageProtection::PlainFlash &&
            slot0_.protection() != StorageProtection::PlainFlash &&
            slot1_.protection() != StorageProtection::PlainFlash);
}

Result WifiCredentialStore::begin() {
    if (ready_) return Result::success();
    if (!protectedStorage()) {
        return Result::failure(ErrorCode::NotConfigured);
    }

    uint8_t encoded[serializedSelectorSize] = {};
    size_t written = 0U;
    Result result = selector_.load(
        MutableByteSpan(encoded, sizeof(encoded)),
        written);
    if (result.code() == ErrorCode::NotFound) {
        activeGeneration_ = 0U;
        activeSlot_ = kNoActiveSlot;
        hasActive_ = false;
        ready_ = true;
        secureZero(MutableByteSpan(encoded, sizeof(encoded)));
        return Result::success();
    }
    if (result && written != sizeof(encoded)) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    Selector selector;
    if (result) {
        result = decodeSelector(ByteView(encoded, written), selector);
    }
    secureZero(MutableByteSpan(encoded, sizeof(encoded)));
    if (!result) return result;

    if (selector.activeSlot == kNoActiveSlot) {
        activeGeneration_ = 0U;
        activeSlot_ = kNoActiveSlot;
        hasActive_ = false;
        ready_ = true;
        return Result::success();
    }

    WifiCredentialProfile profile;
    result = loadSlot(
        selector.activeSlot,
        selector.generation,
        profile);
    clearWifiCredentialProfile(profile);
    if (!result) return result;
    activeGeneration_ = selector.generation;
    activeSlot_ = selector.activeSlot;
    hasActive_ = true;
    ready_ = true;
    return Result::success();
}

void WifiCredentialStore::end() {
    activeGeneration_ = 0U;
    activeSlot_ = kNoActiveSlot;
    ready_ = false;
    hasActive_ = false;
}

IAtomicBlobStore& WifiCredentialStore::slot(uint8_t index) {
    return index == 0U ? slot0_ : slot1_;
}

Result WifiCredentialStore::loadSlot(
    uint8_t index,
    uint32_t expectedGeneration,
    WifiCredentialProfile& profile) {
    if (index > 1U || expectedGeneration == 0U) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    uint8_t encoded[serializedProfileSize] = {};
    size_t written = 0U;
    Result result = slot(index).load(
        MutableByteSpan(encoded, sizeof(encoded)),
        written);
    if (result && written != sizeof(encoded)) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    WifiCredentialProfile decoded;
    if (result) result = decodeProfile(ByteView(encoded, written), decoded);
    if (result && decoded.generation != expectedGeneration) {
        result = Result::failure(ErrorCode::SequenceConflict);
    }
    if (result) profile = decoded;
    clearWifiCredentialProfile(decoded);
    secureZero(MutableByteSpan(encoded, sizeof(encoded)));
    return result;
}

Result WifiCredentialStore::load(WifiCredentialProfile& profile) {
    if (!ready_) return Result::failure(ErrorCode::NotConfigured);
    if (!hasActive_) return Result::failure(ErrorCode::NotFound);
    return loadSlot(activeSlot_, activeGeneration_, profile);
}

Result WifiCredentialStore::loadPrevious(WifiCredentialProfile& profile) {
    if (!ready_) return Result::failure(ErrorCode::NotConfigured);
    if (!hasActive_ || activeGeneration_ <= 1U) {
        return Result::failure(ErrorCode::NotFound);
    }
    WifiCredentialProfile previous;
    Result result = loadSlot(
        static_cast<uint8_t>(activeSlot_ ^ 1U),
        activeGeneration_ - 1U,
        previous);
    if (result) profile = previous;
    clearWifiCredentialProfile(previous);
    return result;
}

bool WifiCredentialStore::sameNetwork(
    const WifiCredentialProfile& profile,
    const WifiNetworkConfig& config) {
    return profile.authentication == config.authentication &&
           profile.hidden() == config.hidden &&
           profile.ssidLength == config.ssid.size &&
           profile.credentialLength == config.credential.size &&
           memcmp(profile.ssid, config.ssid.data, config.ssid.size) == 0 &&
           constantTimeEqual(
               ByteView(profile.credential, profile.credentialLength),
               config.credential);
}

Result WifiCredentialStore::commit(const WifiNetworkConfig& config) {
    if (!ready_) return Result::failure(ErrorCode::NotConfigured);
    Result result = validateWifiNetworkConfig(config);
    if (!result) return result;

    WifiCredentialProfile current;
    if (hasActive_) {
        result = load(current);
        if (!result) {
            clearWifiCredentialProfile(current);
            return result;
        }
        if (sameNetwork(current, config)) {
            clearWifiCredentialProfile(current);
            return Result::success();
        }
        clearWifiCredentialProfile(current);
        if (activeGeneration_ == UINT32_MAX) {
            return Result::failure(ErrorCode::SequenceConflict);
        }
    }

    const uint32_t generation = hasActive_ ? activeGeneration_ + 1U : 1U;
    const uint8_t targetSlot = hasActive_
                                   ? static_cast<uint8_t>(activeSlot_ ^ 1U)
                                   : 0U;
    WifiCredentialProfile candidate;
    candidate.generation = generation;
    candidate.authentication = config.authentication;
    candidate.ssidLength = static_cast<uint8_t>(config.ssid.size);
    candidate.credentialLength =
        static_cast<uint8_t>(config.credential.size);
    candidate.setHidden(config.hidden);
    candidate.setConfirmed(false);
    memcpy(candidate.ssid, config.ssid.data, config.ssid.size);
    if (!config.credential.empty()) {
        memcpy(
            candidate.credential,
            config.credential.data,
            config.credential.size);
    }

    uint8_t encoded[serializedProfileSize] = {};
    result = encodeProfile(
        candidate,
        MutableByteSpan(encoded, sizeof(encoded)));
    if (result) {
        result = slot(targetSlot).replace(ByteView(encoded, sizeof(encoded)));
    }
    secureZero(MutableByteSpan(encoded, sizeof(encoded)));
    clearWifiCredentialProfile(candidate);
    if (!result) return result;

    result = writeSelector(targetSlot, generation);
    if (!result) return result;
    activeSlot_ = targetSlot;
    activeGeneration_ = generation;
    hasActive_ = true;
    return Result::success();
}

Result WifiCredentialStore::rollbackToPrevious() {
    if (!ready_) return Result::failure(ErrorCode::NotConfigured);
    if (!hasActive_ || activeGeneration_ <= 1U) {
        return Result::failure(ErrorCode::NotFound);
    }
    WifiCredentialProfile active;
    Result result = load(active);
    const bool activeConfirmed = result && active.confirmed();
    clearWifiCredentialProfile(active);
    if (!result) return result;
    if (activeConfirmed) {
        return Result::failure(ErrorCode::StateConflict);
    }

    const uint8_t previousSlot = static_cast<uint8_t>(activeSlot_ ^ 1U);
    WifiCredentialProfile previous;
    result = loadSlot(
        previousSlot,
        activeGeneration_ - 1U,
        previous);
    const uint32_t previousGeneration = previous.generation;
    const bool previousConfirmed = result && previous.confirmed();
    clearWifiCredentialProfile(previous);
    if (!result) return result;
    if (!previousConfirmed) {
        return Result::failure(ErrorCode::StateConflict);
    }
    result = writeSelector(previousSlot, previousGeneration);
    if (!result) return result;
    activeSlot_ = previousSlot;
    activeGeneration_ = previousGeneration;
    return Result::success();
}

Result WifiCredentialStore::confirmActive() {
    if (!ready_) return Result::failure(ErrorCode::NotConfigured);
    if (!hasActive_) return Result::failure(ErrorCode::NotFound);

    WifiCredentialProfile profile;
    Result result = load(profile);
    if (!result) {
        clearWifiCredentialProfile(profile);
        return result;
    }
    if (profile.confirmed()) {
        clearWifiCredentialProfile(profile);
        return Result::success();
    }
    profile.setConfirmed(true);

    uint8_t encoded[serializedProfileSize] = {};
    result = encodeProfile(
        profile,
        MutableByteSpan(encoded, sizeof(encoded)));
    if (result) {
        result = slot(activeSlot_).replace(ByteView(encoded, sizeof(encoded)));
    }
    secureZero(MutableByteSpan(encoded, sizeof(encoded)));
    clearWifiCredentialProfile(profile);
    return result;
}

Result WifiCredentialStore::installConfirmed(
    const WifiCredentialProfile& profile) {
    if (!ready_) return Result::failure(ErrorCode::NotConfigured);
    Result result = validateWifiNetworkConfig(profile.networkConfig());
    if (!result || profile.generation == 0U || !profile.confirmed() ||
        (profile.flags & 0xFCU) != 0U) {
        return result ? Result::failure(ErrorCode::InvalidArgument) : result;
    }

    if (hasActive_) {
        WifiCredentialProfile existing;
        result = load(existing);
        const bool same = result && existing.generation == profile.generation &&
                          existing.confirmed() &&
                          sameNetwork(existing, profile.networkConfig());
        clearWifiCredentialProfile(existing);
        if (!result) return result;
        return same ? Result::success()
                    : Result::failure(ErrorCode::StateConflict);
    }

    uint8_t encoded[serializedProfileSize] = {};
    result = encodeProfile(profile, MutableByteSpan(encoded, sizeof(encoded)));
    if (result) result = slot0_.replace(ByteView(encoded, sizeof(encoded)));
    secureZero(MutableByteSpan(encoded, sizeof(encoded)));
    if (!result) return result;

    result = writeSelector(0U, profile.generation);
    if (!result) return result;
    activeSlot_ = 0U;
    activeGeneration_ = profile.generation;
    hasActive_ = true;

    WifiCredentialProfile verified;
    result = load(verified);
    const bool same = result && verified.generation == profile.generation &&
                      verified.confirmed() &&
                      sameNetwork(verified, profile.networkConfig());
    clearWifiCredentialProfile(verified);
    return same ? Result::success()
                : (result ? Result::failure(ErrorCode::InternalError) : result);
}

Result WifiCredentialStore::clear() {
    if (!ready_) return Result::failure(ErrorCode::NotConfigured);
    Result result = writeSelector(kNoActiveSlot, 0U);
    if (!result) return result;
    activeSlot_ = kNoActiveSlot;
    activeGeneration_ = 0U;
    hasActive_ = false;

    const Result first = slot0_.clear();
    const Result second = slot1_.clear();
    if (!erasedResult(first)) return first;
    if (!erasedResult(second)) return second;
    return Result::success();
}

Result WifiCredentialStore::writeSelector(
    uint8_t activeSlot,
    uint32_t generation) {
    Selector selector;
    selector.activeSlot = activeSlot;
    selector.generation = generation;
    uint8_t encoded[serializedSelectorSize] = {};
    Result result = encodeSelector(
        selector,
        MutableByteSpan(encoded, sizeof(encoded)));
    if (result) {
        result = selector_.replace(ByteView(encoded, sizeof(encoded)));
    }
    secureZero(MutableByteSpan(encoded, sizeof(encoded)));
    return result;
}

Result WifiCredentialStore::encodeProfile(
    const WifiCredentialProfile& profile,
    MutableByteSpan output) {
    const WifiNetworkConfig config = profile.networkConfig();
    Result result = validateWifiNetworkConfig(config);
    if (!result || profile.generation == 0U) {
        return result ? Result::failure(ErrorCode::InvalidArgument) : result;
    }
    if (output.data == nullptr || output.size < serializedProfileSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    memset(output.data, 0, serializedProfileSize);
    memcpy(output.data, kProfileMagic, sizeof(kProfileMagic));
    output.data[4] = kSchemaVersion;
    output.data[5] = static_cast<uint8_t>(profile.authentication);
    output.data[6] = profile.flags;
    output.data[7] = profile.ssidLength;
    output.data[8] = profile.credentialLength;
    wifiStoreWriteU32(output.data + 12U, profile.generation);
    memcpy(output.data + 16U, profile.ssid, profile.ssidLength);
    if (profile.credentialLength != 0U) {
        memcpy(
            output.data + 48U,
            profile.credential,
            profile.credentialLength);
    }
    wifiStoreWriteU32(
        output.data + 112U,
        computeCrc32(ByteView(output.data, 112U)));
    return Result::success();
}

Result WifiCredentialStore::decodeProfile(
    ByteView encoded,
    WifiCredentialProfile& profile) {
    if (encoded.data == nullptr || encoded.size != serializedProfileSize ||
        memcmp(encoded.data, kProfileMagic, sizeof(kProfileMagic)) != 0 ||
        encoded.data[4] != kSchemaVersion ||
        (encoded.data[6] & 0xFCU) != 0U ||
        !allZero(encoded.data + 9U, 3U) ||
        wifiStoreReadU32(encoded.data + 112U) !=
            computeCrc32(ByteView(encoded.data, 112U))) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    const uint8_t ssidLength = encoded.data[7];
    const uint8_t credentialLength = encoded.data[8];
    if (ssidLength > kWifiSsidMaxSize ||
        credentialLength > kWifiCredentialMaxSize ||
        !allZero(
            encoded.data + 16U + ssidLength,
            kWifiSsidMaxSize - ssidLength) ||
        !allZero(
            encoded.data + 48U + credentialLength,
            kWifiCredentialMaxSize - credentialLength)) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    WifiCredentialProfile decoded;
    decoded.generation = wifiStoreReadU32(encoded.data + 12U);
    decoded.authentication =
        static_cast<WifiAuthentication>(encoded.data[5]);
    decoded.ssidLength = ssidLength;
    decoded.credentialLength = credentialLength;
    decoded.flags = encoded.data[6];
    memcpy(decoded.ssid, encoded.data + 16U, kWifiSsidMaxSize);
    memcpy(
        decoded.credential,
        encoded.data + 48U,
        kWifiCredentialMaxSize);
    Result result = decoded.generation == 0U
                        ? Result::failure(ErrorCode::InvalidEncoding)
                        : validateWifiNetworkConfig(decoded.networkConfig());
    if (!result) {
        clearWifiCredentialProfile(decoded);
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    profile = decoded;
    clearWifiCredentialProfile(decoded);
    return Result::success();
}

Result WifiCredentialStore::encodeSelector(
    const Selector& selector,
    MutableByteSpan output) {
    if ((selector.activeSlot > 1U && selector.activeSlot != kNoActiveSlot) ||
        ((selector.activeSlot == kNoActiveSlot) !=
         (selector.generation == 0U))) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (output.data == nullptr || output.size < serializedSelectorSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    memset(output.data, 0, serializedSelectorSize);
    memcpy(output.data, kSelectorMagic, sizeof(kSelectorMagic));
    output.data[4] = kSchemaVersion;
    output.data[5] = selector.activeSlot;
    wifiStoreWriteU32(output.data + 8U, selector.generation);
    wifiStoreWriteU32(
        output.data + 12U,
        computeCrc32(ByteView(output.data, 12U)));
    return Result::success();
}

Result WifiCredentialStore::decodeSelector(
    ByteView encoded,
    Selector& selector) {
    if (encoded.data == nullptr || encoded.size != serializedSelectorSize ||
        memcmp(encoded.data, kSelectorMagic, sizeof(kSelectorMagic)) != 0 ||
        encoded.data[4] != kSchemaVersion ||
        !allZero(encoded.data + 6U, 2U) ||
        wifiStoreReadU32(encoded.data + 12U) !=
            computeCrc32(ByteView(encoded.data, 12U))) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    Selector decoded;
    decoded.activeSlot = encoded.data[5];
    decoded.generation = wifiStoreReadU32(encoded.data + 8U);
    if ((decoded.activeSlot > 1U && decoded.activeSlot != kNoActiveSlot) ||
        ((decoded.activeSlot == kNoActiveSlot) !=
         (decoded.generation == 0U))) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    selector = decoded;
    return Result::success();
}

} // namespace blinker
