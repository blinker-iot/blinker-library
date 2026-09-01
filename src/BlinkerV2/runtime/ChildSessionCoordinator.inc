#include "ChildSessionCoordinator.h"

#include <limits.h>
#include <string.h>

#include "../core/SecureMemory.h"

namespace blinker {

namespace {

bool childSupportsVersion(const bbp2::HelloBody& hello) {
    for (uint8_t index = 0U; index < hello.versionCount; ++index) {
        if (hello.versions[index] == bbp2::kVersion) return true;
    }
    return false;
}

bool childSupportsMethod2(const bbp2::HelloBody& hello) {
    for (uint8_t index = 0U;
         index < hello.authorizationMethodCount;
         ++index) {
        if (hello.authorizationMethods[index] ==
            security::kControllerHmacSha256Method) {
            return true;
        }
    }
    return false;
}

} // namespace

ChildSessionCoordinator::ChildSessionCoordinator(
    IRandom& random,
    const ChildSessionConfig& config)
    : config_(config),
      initiator_(random),
      authPayload_(),
      manifestFingerprint_(),
      deadlineMillis_(0U),
      negotiatedFeatures_(0U),
      manifestRevision_(0U),
      negotiatedMaxFrameSize_(0U),
      sequence_(0U),
      remoteReliableReceiveWindow_(0U),
      hasManifest_(false),
      state_(ChildSessionState::Idle) {}

ChildSessionCoordinator::~ChildSessionCoordinator() {
    reset();
}

Result ChildSessionCoordinator::validateConfig() const {
    if ((config_.features & kChildSessionRequiredFeatures) !=
            kChildSessionRequiredFeatures ||
        (config_.features & ~kChildSessionKnownFeatures) != 0U ||
        config_.authenticationTimeoutMillis == 0U ||
        config_.authenticationTimeoutMillis > INT32_MAX ||
        config_.maxFrameSize < kChildSessionMinimumFrameSize ||
        config_.reliableReceiveWindow == 0U ||
        config_.reliableReceiveWindow > bbp2::kMaxReliableReceiveWindow) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    return Result::success();
}

void ChildSessionCoordinator::reset() {
    initiator_.reset();
    secureZero(MutableByteSpan(authPayload_, sizeof(authPayload_)));
    secureZero(MutableByteSpan(
        manifestFingerprint_, sizeof(manifestFingerprint_)));
    deadlineMillis_ = 0U;
    negotiatedFeatures_ = 0U;
    manifestRevision_ = 0U;
    negotiatedMaxFrameSize_ = 0U;
    sequence_ = 0U;
    remoteReliableReceiveWindow_ = 0U;
    hasManifest_ = false;
    state_ = ChildSessionState::Idle;
}

Result ChildSessionCoordinator::fail(ErrorCode error) {
    reset();
    return Result::failure(error);
}

bool ChildSessionCoordinator::deadlineReached(uint32_t nowMillis) const {
    return state_ != ChildSessionState::Idle &&
           state_ != ChildSessionState::Secure &&
           static_cast<int32_t>(nowMillis - deadlineMillis_) >= 0;
}

Result ChildSessionCoordinator::poll(uint32_t nowMillis) {
    return deadlineReached(nowMillis)
               ? fail(ErrorCode::NotConnected)
               : Result::success();
}

Result ChildSessionCoordinator::writeFrameHeader(
    bbp2::MessageKind kind,
    uint8_t flags,
    uint16_t sequence,
    size_t bodySize,
    MutableByteSpan output,
    ByteView& frame) const {
    frame = ByteView();
    if (bodySize > UINT16_MAX ||
        output.data == nullptr ||
        output.size < bbp2::kBaseHeaderSize + bodySize ||
        bbp2::kBaseHeaderSize + bodySize > config_.maxFrameSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    bbp2::FrameHeader header;
    header.kind = static_cast<uint8_t>(kind);
    header.flags = flags;
    header.sequence = sequence;
    header.bodyLength = static_cast<uint16_t>(bodySize);
    Result result = bbp2::encodeHeader(header, output);
    if (result) {
        frame = ByteView(
            output.data,
            bbp2::kBaseHeaderSize + bodySize);
    }
    return result;
}

Result ChildSessionCoordinator::writeHello(
    MutableByteSpan output,
    ByteView& frame) {
    frame = ByteView();
    if (output.data == nullptr || output.size <= bbp2::kBaseHeaderSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    bbp2::HelloBody hello;
    // Gateway remains an App-like controller on wire. Gateway authorization
    // is carried by its exact credential and cloud topology, not PeerRole.
    hello.role = bbp2::PeerRole::App;
    hello.versions[0] = bbp2::kVersion;
    hello.versionCount = 1U;
    hello.features = config_.features;
    hello.maxFrameSize = config_.maxFrameSize;
    hello.maxReassemblySize = config_.maxFrameSize;
    hello.authorizationMethods[0] =
        security::kControllerHmacSha256Method;
    hello.authorizationMethodCount = 1U;
    hello.reliableReceiveWindow = config_.reliableReceiveWindow;
    ByteView body;
    Result result = bbp2::encodeHelloBody(
        hello,
        MutableByteSpan(
            output.data + bbp2::kBaseHeaderSize,
            output.size - bbp2::kBaseHeaderSize),
        body);
    return result
               ? writeFrameHeader(
                     bbp2::MessageKind::Hello,
                     bbp2::FlagNone,
                     sequence_,
                     body.size,
                     output,
                     frame)
               : result;
}

Result ChildSessionCoordinator::writeAuthenticate(
    ByteView payload,
    MutableByteSpan output,
    ByteView& frame) {
    frame = ByteView();
    if (output.data == nullptr || output.size <= bbp2::kBaseHeaderSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    bbp2::AuthRequestBody request;
    request.method = security::kControllerHmacSha256Method;
    request.payload = payload;
    ByteView body;
    Result result = bbp2::encodeAuthRequestBody(
        request,
        MutableByteSpan(
            output.data + bbp2::kBaseHeaderSize,
            output.size - bbp2::kBaseHeaderSize),
        body);
    return result
               ? writeFrameHeader(
                     bbp2::MessageKind::Authenticate,
                     bbp2::FlagNone,
                     sequence_,
                     body.size,
                     output,
                     frame)
               : result;
}

Result ChildSessionCoordinator::begin(
    const ControllerCredential& credential,
    uint32_t nowMillis,
    MutableByteSpan output,
    ByteView& frame) {
    frame = ByteView();
    reset();
    Result result = validateConfig();
    ByteView authPayload;
    if (result) {
        result = initiator_.begin(
            credential,
            MutableByteSpan(authPayload_, sizeof(authPayload_)),
            authPayload);
    }
    if (result && authPayload.size !=
                      security::kControllerAuthInitPayloadSize) {
        result = Result::failure(ErrorCode::InternalError);
    }
    if (!result) return fail(result.code());

    sequence_ = 1U;
    deadlineMillis_ = nowMillis + config_.authenticationTimeoutMillis;
    result = writeHello(output, frame);
    if (!result) return fail(result.code());
    state_ = ChildSessionState::AwaitingHello;
    return Result::success();
}

Result ChildSessionCoordinator::handleHello(
    const bbp2::FrameView& frame,
    MutableByteSpan output,
    ByteView& responseFrame) {
    bbp2::HelloBody hello;
    Result result = bbp2::decodeHelloBody(frame.body, hello);
    if (!result) return result;
    if (hello.role != bbp2::PeerRole::Device ||
        !childSupportsVersion(hello) ||
        !childSupportsMethod2(hello) ||
        (hello.features & kChildSessionRequiredFeatures) !=
            kChildSessionRequiredFeatures ||
        !hello.hasManifestRevision ||
        !hello.hasManifestFingerprint ||
        hello.maxFrameSize < kChildSessionMinimumFrameSize) {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }

    negotiatedFeatures_ = hello.features & config_.features;
    negotiatedMaxFrameSize_ =
        hello.maxFrameSize < config_.maxFrameSize
            ? hello.maxFrameSize
            : config_.maxFrameSize;
    remoteReliableReceiveWindow_ = hello.reliableReceiveWindow;
    manifestRevision_ = hello.manifestRevision;
    memcpy(
        manifestFingerprint_,
        hello.manifestFingerprint.data,
        sizeof(manifestFingerprint_));
    hasManifest_ = true;

    ++sequence_;
    result = writeAuthenticate(
        ByteView(authPayload_, security::kControllerAuthInitPayloadSize),
        output,
        responseFrame);
    if (result) state_ = ChildSessionState::AwaitingChallenge;
    return result;
}

Result ChildSessionCoordinator::handleChallenge(
    const bbp2::FrameView& frame,
    MutableByteSpan output,
    ByteView& responseFrame) {
    bbp2::AuthResultBody resultBody;
    Result result = bbp2::decodeAuthResultBody(frame.body, resultBody);
    ByteView proof;
    if (result) {
        result = initiator_.handleChallenge(
            resultBody,
            MutableByteSpan(authPayload_, sizeof(authPayload_)),
            proof);
    }
    if (!result) return result;
    ++sequence_;
    result = writeAuthenticate(proof, output, responseFrame);
    if (result) state_ = ChildSessionState::AwaitingAuthorization;
    return result;
}

Result ChildSessionCoordinator::handleAuthorization(
    const bbp2::FrameView& frame) {
    bbp2::AuthResultBody resultBody;
    Result result = bbp2::decodeAuthResultBody(frame.body, resultBody);
    if (result) result = initiator_.handleAuthorization(resultBody);
    if (result) {
        secureZero(MutableByteSpan(authPayload_, sizeof(authPayload_)));
        deadlineMillis_ = 0U;
        state_ = ChildSessionState::Secure;
    }
    return result;
}

Result ChildSessionCoordinator::handleFrame(
    ByteView input,
    uint32_t nowMillis,
    MutableByteSpan output,
    ByteView& responseFrame) {
    responseFrame = ByteView();
    if (state_ == ChildSessionState::Idle ||
        state_ == ChildSessionState::Secure) {
        return fail(ErrorCode::SequenceConflict);
    }
    if (deadlineReached(nowMillis)) {
        return fail(ErrorCode::NotConnected);
    }
    const size_t inboundLimit = negotiatedMaxFrameSize_ == 0U
                                    ? config_.maxFrameSize
                                    : negotiatedMaxFrameSize_;
    if (input.size > inboundLimit) {
        return fail(ErrorCode::CapacityExceeded);
    }
    bbp2::FrameView frame;
    Result result = bbp2::parseFrame(input, frame);
    const bbp2::MessageKind expected =
        state_ == ChildSessionState::AwaitingHello
            ? bbp2::MessageKind::Hello
            : bbp2::MessageKind::AuthResult;
    if (!result ||
        frame.header.kind != static_cast<uint8_t>(expected) ||
        frame.header.flags != bbp2::FlagIsResponse ||
        frame.header.sequence != sequence_) {
        return fail(result ? ErrorCode::SequenceConflict : result.code());
    }

    if (state_ == ChildSessionState::AwaitingHello) {
        result = handleHello(frame, output, responseFrame);
    } else if (state_ == ChildSessionState::AwaitingChallenge) {
        result = handleChallenge(frame, output, responseFrame);
    } else {
        result = handleAuthorization(frame);
    }
    return result ? result : fail(result.code());
}

} // namespace blinker
