#include "EnrollmentKeyHttpExchange.h"

namespace blinker {

namespace {

const char kAuthorizationHeaderName[] = "Blinker-Enrollment-Key";
const char kCborContentType[] = "application/cbor";

} // namespace

EnrollmentKeyHttpExchange::EnrollmentKeyHttpExchange(
    IHttpClient& http,
    const EnrollmentKeyHttpConfig& config,
    StringView enrollmentKey)
    : http_(http),
      config_(config),
      authorizationHeader_(
          StringView(
              kAuthorizationHeaderName,
              sizeof(kAuthorizationHeaderName) - 1U),
          enrollmentKey) {}

Result EnrollmentKeyHttpExchange::setEnrollmentKey(
    StringView enrollmentKey) {
    if (http_.state() == HttpClientState::InProgress) {
        return Result::failure(ErrorCode::StateConflict);
    }
    if (!enrollmentKey.empty() && !validateEnrollmentKey(enrollmentKey)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    authorizationHeader_.value = enrollmentKey;
    return Result::success();
}

Result EnrollmentKeyHttpExchange::start(
    StringView target,
    ByteView exactBody,
    MutableByteSpan responseBuffer) {
    if (!validateEnrollmentKey(authorizationHeader_.value) ||
        config_.host.data == nullptr || config_.host.empty() ||
        config_.port == 0U || target.data == nullptr || target.empty() ||
        target.data[0] != '/' || exactBody.data == nullptr ||
        exactBody.empty() || responseBuffer.data == nullptr ||
        responseBuffer.empty() || config_.timeoutMillis == 0U) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (http_.state() == HttpClientState::InProgress) {
        return Result::failure(ErrorCode::StateConflict);
    }

    HttpRequest request;
    request.method = HttpMethod::Post;
    request.host = config_.host;
    request.port = config_.port;
    request.security = config_.security;
    request.target = target;
    request.contentType = StringView(
        kCborContentType,
        sizeof(kCborContentType) - 1U);
    request.headers = &authorizationHeader_;
    request.headerCount = 1U;
    request.body = exactBody;
    request.responseBuffer = responseBuffer;
    request.timeoutMillis = config_.timeoutMillis;
    return http_.start(request);
}

} // namespace blinker
