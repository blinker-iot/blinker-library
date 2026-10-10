#include <BlinkerV2/core/SecureMemory.h>
#include <BlinkerV2/provisioning/PresenceKeyControlContract.h>
#include <BlinkerV2/protocol/bbp2/Messages.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

namespace {

static_assert(
    blinker::kPresenceKeyMutationMaxEncodedSize ==
        blinker::bbp2::kPresenceKeyMutationMaximumSize,
    "PresenceKey mutation bounds drifted between contract and BBP/2");
static_assert(
    blinker::kPresenceKeyReceiptMaxEncodedSize ==
        blinker::bbp2::kPresenceKeyReceiptMaximumSize,
    "PresenceKey receipt bounds drifted between contract and BBP/2");

#define CHECK_TRUE(condition)                                                   \
    do {                                                                        \
        if (!(condition)) {                                                     \
            std::cerr << "PresenceKey control check failed at line "          \
                      << __LINE__ << ": " << #condition << std::endl;          \
            std::exit(1);                                                       \
        }                                                                       \
    } while (false)

const uint8_t kMutation[] = {
    0xA5, 0x00, 0x01, 0x01, 0x1A, 0x01, 0x02, 0x03, 0x04, 0x02,
    0x02, 0x03, 0x03, 0x04, 0x50, 0x01, 0x02, 0x03, 0x04, 0x05,
    0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10};

const uint8_t kDigest[] = {
    0x5D, 0xFB, 0xAB, 0xEE, 0xDF, 0x31, 0x8B, 0xF3,
    0x3C, 0x09, 0x27, 0xC4, 0x3D, 0x76, 0x30, 0xF5,
    0x1B, 0x82, 0xF3, 0x51, 0x74, 0x03, 0x01, 0x35,
    0x4F, 0xA3, 0xD7, 0xFC, 0x51, 0xF0, 0x13, 0x2E};

const uint8_t kProof[] = {
    0xD1, 0x86, 0x31, 0xE4, 0x0B, 0x5B, 0x48, 0x56,
    0xA7, 0x99, 0x05, 0x51, 0x10, 0x8E, 0x45, 0xE4,
    0xA5, 0x8C, 0xAA, 0xC1, 0x6C, 0xD8, 0x57, 0x9F,
    0x3D, 0xB3, 0xA0, 0x54, 0x52, 0x33, 0x1E, 0xA6};

void mutationMatchesNodeGolden() {
    blinker::PresenceKeyMutation mutation;
    CHECK_TRUE(blinker::decodePresenceKeyMutation(
                   blinker::ByteView(kMutation, sizeof(kMutation)), mutation)
                   .ok());
    CHECK_TRUE(mutation.accessEpoch == 0x01020304U);
    CHECK_TRUE(mutation.expectedVersion == 2U);
    CHECK_TRUE(mutation.presence.version == 3U);

    uint8_t output[blinker::kPresenceKeyMutationMaxEncodedSize] = {};
    blinker::ByteView encoded;
    CHECK_TRUE(blinker::encodePresenceKeyMutation(
                   mutation,
                   blinker::MutableByteSpan(output, sizeof(output)),
                   encoded)
                   .ok());
    CHECK_TRUE(encoded.size == sizeof(kMutation));
    CHECK_TRUE(std::memcmp(encoded.data, kMutation, sizeof(kMutation)) == 0);
    blinker::clearDevicePresenceKey(mutation.presence);
}

void receiptMatchesNodeGolden() {
    blinker::DevicePresenceKey presence;
    presence.version = 3U;
    for (size_t index = 0U; index < sizeof(presence.bytes); ++index) {
        presence.bytes[index] = static_cast<uint8_t>(index + 1U);
    }

    blinker::PresenceKeyReceipt receipt;
    receipt.accessEpoch = 0x01020304U;
    receipt.expectedVersion = 2U;
    receipt.presenceKeyVersion = 3U;
    receipt.keyDigest = blinker::ByteView(kDigest, sizeof(kDigest));
    receipt.proofKind = 1U;

    uint8_t digest[blinker::kPresenceKeyDigestSize] = {};
    uint8_t proof[blinker::kPresenceKeyReceiptProofSize] = {};
    CHECK_TRUE(blinker::computePresenceKeyReceiptProof(
                   presence,
                   receipt,
                   blinker::MutableByteSpan(digest, sizeof(digest)),
                   blinker::MutableByteSpan(proof, sizeof(proof)))
                   .ok());
    CHECK_TRUE(std::memcmp(digest, kDigest, sizeof(kDigest)) == 0);
    CHECK_TRUE(std::memcmp(proof, kProof, sizeof(kProof)) == 0);

    receipt.proof = blinker::ByteView(proof, sizeof(proof));
    uint8_t encodedStorage[blinker::kPresenceKeyReceiptMaxEncodedSize] = {};
    blinker::ByteView encoded;
    CHECK_TRUE(blinker::encodePresenceKeyReceipt(
                   receipt,
                   blinker::MutableByteSpan(
                       encodedStorage,
                       sizeof(encodedStorage)),
                   encoded)
                   .ok());
    CHECK_TRUE(encoded.size == 85U);

    blinker::PresenceKeyReceipt decoded;
    CHECK_TRUE(blinker::decodePresenceKeyReceipt(encoded, decoded).ok());
    CHECK_TRUE(blinker::verifyPresenceKeyReceiptProof(presence, decoded).ok());
    proof[0] ^= 0x01U;
    CHECK_TRUE(!blinker::verifyPresenceKeyReceiptProof(presence, receipt).ok());

    blinker::clearDevicePresenceKey(presence);
    blinker::secureZero(blinker::MutableByteSpan(digest, sizeof(digest)));
    blinker::secureZero(blinker::MutableByteSpan(proof, sizeof(proof)));
}

} // namespace

int main() {
    mutationMatchesNodeGolden();
    receiptMatchesNodeGolden();
    std::cout << "PresenceKey control contract checks passed" << std::endl;
    return 0;
}
