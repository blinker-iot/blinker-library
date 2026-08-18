#ifndef BLINKER_ARDUINO_CLIENT_HTTP_ADAPTER_H
#define BLINKER_ARDUINO_CLIENT_HTTP_ADAPTER_H

#include <BlinkerV2/core/SecureMemory.h>
#include <BlinkerV2/interface/IClock.h>
#include <BlinkerV2/interface/IHttpClient.h>

#include <limits.h>
#include <string.h>

namespace blinker {

template <
    typename TClient,
    size_t HostCapacity = 128,
    size_t RequestCapacity = 512,
    size_t HeaderLineCapacity = 128>
class ArduinoClientHttpAdapter : public IHttpClient {
public:
    ArduinoClientHttpAdapter(
        TClient& client,
        HttpSecurity actualSecurity,
        IClock& clock)
        : client_(client),
          actualSecurity_(actualSecurity),
          clock_(clock),
          request_(),
          response_(),
          currentState_(HttpClientState::Idle),
          phase_(PhaseIdle),
          currentError_(ErrorCode::Ok),
          requestHeaderSize_(0),
          requestHeaderSent_(0),
          requestBodySent_(0),
          responseSize_(0),
          headerLineSize_(0),
          contentLength_(0),
          startedAt_(0U),
          hasContentLength_(false),
          statusParsed_(false) {
        host_[0] = '\0';
    }

    Result start(const HttpRequest& request) override {
        stop();
        if (request.security != actualSecurity_) {
            return fail(ErrorCode::InvalidArgument);
        }
        if ((request.method != HttpMethod::Get &&
             request.method != HttpMethod::Post) ||
            (request.method == HttpMethod::Get && request.body.size != 0U)) {
            return fail(ErrorCode::InvalidArgument);
        }
        if (request.host.empty() || request.host.data == nullptr ||
            request.host.size >= HostCapacity || request.port == 0U ||
            request.target.empty() || request.target.data == nullptr ||
            request.target.data[0] != '/' ||
            request.responseBuffer.data == nullptr ||
            request.responseBuffer.size == 0U ||
            request.timeoutMillis == 0U ||
            request.timeoutMillis > INT32_MAX ||
            (request.body.size != 0U && request.body.data == nullptr) ||
            (request.headerCount != 0U && request.headers == nullptr)) {
            return fail(ErrorCode::InvalidArgument);
        }
        if (containsNul(request.host) || containsNul(request.target) ||
            containsNul(request.contentType)) {
            return fail(ErrorCode::InvalidArgument);
        }
        for (uint8_t index = 0U; index < request.headerCount; ++index) {
            if (!validHeaderName(request.headers[index].name) ||
                !validHeaderValue(request.headers[index].value) ||
                reservedHeader(request.headers[index].name)) {
                return fail(ErrorCode::InvalidArgument);
            }
        }
        memcpy(host_, request.host.data, request.host.size);
        host_[request.host.size] = '\0';
        request_ = request;
        Result result = buildRequestHeader();
        if (!result) return result;
        currentState_ = HttpClientState::InProgress;
        phase_ = PhaseConnect;
        startedAt_ = clock_.monotonicMillis();
        return Result::success();
    }

    void poll(uint32_t budgetMicros) override {
        if (currentState_ != HttpClientState::InProgress) return;
        size_t operations = budgetMicros == 0U
                                ? 64U
                                : static_cast<size_t>(budgetMicros);
        if (operations > 1024U) operations = 1024U;

        while (operations-- != 0U &&
               currentState_ == HttpClientState::InProgress) {
            if (static_cast<uint32_t>(
                    clock_.monotonicMillis() - startedAt_) >=
                request_.timeoutMillis) {
                fail(ErrorCode::NotConnected);
                return;
            }
            if (phase_ == PhaseConnect) {
                if (!client_.connect(host_, request_.port)) {
                    fail(ErrorCode::NotConnected);
                    return;
                }
                phase_ = PhaseSendHeader;
                continue;
            }
            if (phase_ == PhaseSendHeader) {
                const size_t remaining =
                    requestHeaderSize_ - requestHeaderSent_;
                if (remaining == 0U) {
                    phase_ = request_.body.size == 0U
                                 ? PhaseStatusLine
                                 : PhaseSendBody;
                    continue;
                }
                const size_t written = client_.write(
                    reinterpret_cast<const uint8_t*>(requestHeader_) +
                        requestHeaderSent_,
                    remaining);
                if (written == 0U) return;
                if (written > remaining) {
                    fail(ErrorCode::InternalError);
                    return;
                }
                requestHeaderSent_ += written;
                continue;
            }
            if (phase_ == PhaseSendBody) {
                const size_t remaining = request_.body.size - requestBodySent_;
                if (remaining == 0U) {
                    phase_ = PhaseStatusLine;
                    continue;
                }
                const size_t written = client_.write(
                    request_.body.data + requestBodySent_,
                    remaining);
                if (written == 0U) return;
                if (written > remaining) {
                    fail(ErrorCode::InternalError);
                    return;
                }
                requestBodySent_ += written;
                continue;
            }

            if (phase_ == PhaseBody) {
                if (client_.available() <= 0) {
                    if (!client_.connected()) {
                        if (!hasContentLength_) {
                            complete();
                        } else {
                            fail(ErrorCode::TruncatedInput);
                        }
                    }
                    return;
                }
                const int readValue = client_.read();
                if (readValue < 0) return;
                if (responseSize_ >= request_.responseBuffer.size) {
                    fail(ErrorCode::BufferTooSmall);
                    return;
                }
                request_.responseBuffer.data[responseSize_++] =
                    static_cast<uint8_t>(readValue);
                if (hasContentLength_ && responseSize_ == contentLength_) {
                    complete();
                }
                continue;
            }

            if (client_.available() <= 0) {
                if (!client_.connected()) {
                    fail(ErrorCode::TruncatedInput);
                }
                return;
            }
            const int readValue = client_.read();
            if (readValue < 0) return;
            const Result result = consumeHeaderByte(
                static_cast<uint8_t>(readValue));
            if (!result) return;
        }
    }

    HttpClientState state() const override { return currentState_; }

    Result response(HttpResponse& responseValue) const override {
        if (currentState_ != HttpClientState::Complete) {
            return Result::failure(ErrorCode::WouldBlock);
        }
        responseValue = response_;
        return Result::success();
    }

    ErrorCode lastError() const override { return currentError_; }

    void stop() override {
        client_.stop();
        secureClear(host_, sizeof(host_));
        secureClear(requestHeader_, sizeof(requestHeader_));
        request_ = HttpRequest();
        response_ = HttpResponse();
        currentState_ = HttpClientState::Idle;
        phase_ = PhaseIdle;
        currentError_ = ErrorCode::Ok;
        requestHeaderSize_ = 0;
        requestHeaderSent_ = 0;
        requestBodySent_ = 0;
        responseSize_ = 0;
        headerLineSize_ = 0;
        contentLength_ = 0;
        startedAt_ = 0U;
        hasContentLength_ = false;
        statusParsed_ = false;
    }

private:
    enum Phase : uint8_t {
        PhaseIdle = 0,
        PhaseConnect,
        PhaseSendHeader,
        PhaseSendBody,
        PhaseStatusLine,
        PhaseHeaders,
        PhaseBody
    };

    static bool containsNul(StringView value) {
        if (value.size == 0U) return false;
        if (value.data == nullptr) return true;
        return memchr(value.data, '\0', value.size) != nullptr;
    }

    static bool validHeaderName(StringView value) {
        if (value.data == nullptr || value.empty()) return false;
        for (size_t index = 0U; index < value.size; ++index) {
            const char byte = value.data[index];
            if (!((byte >= 'A' && byte <= 'Z') ||
                  (byte >= 'a' && byte <= 'z') ||
                  (byte >= '0' && byte <= '9') || byte == '-')) {
                return false;
            }
        }
        return true;
    }

    static bool validHeaderValue(StringView value) {
        if (value.data == nullptr || value.empty()) return false;
        for (size_t index = 0U; index < value.size; ++index) {
            const unsigned char byte =
                static_cast<unsigned char>(value.data[index]);
            if (byte < 0x20U || byte > 0x7EU) return false;
        }
        return true;
    }

    static bool asciiEqualIgnoreCase(char left, char right) {
        if (left >= 'A' && left <= 'Z') left = static_cast<char>(left + 32);
        if (right >= 'A' && right <= 'Z') right = static_cast<char>(right + 32);
        return left == right;
    }

    static bool startsWithIgnoreCase(
        const char* value,
        size_t valueSize,
        const char* prefix) {
        const size_t prefixSize = strlen(prefix);
        if (valueSize < prefixSize) return false;
        for (size_t index = 0; index < prefixSize; ++index) {
            if (!asciiEqualIgnoreCase(value[index], prefix[index])) {
                return false;
            }
        }
        return true;
    }

    static bool equalsIgnoreCase(StringView value, const char* expected) {
        const size_t expectedSize = strlen(expected);
        if (value.size != expectedSize) return false;
        for (size_t index = 0U; index < expectedSize; ++index) {
            if (!asciiEqualIgnoreCase(value.data[index], expected[index])) {
                return false;
            }
        }
        return true;
    }

    static bool reservedHeader(StringView name) {
        return equalsIgnoreCase(name, "Host") ||
               equalsIgnoreCase(name, "Content-Type") ||
               equalsIgnoreCase(name, "Content-Length") ||
               equalsIgnoreCase(name, "Connection") ||
               equalsIgnoreCase(name, "Transfer-Encoding");
    }

    static bool containsIgnoreCase(
        const char* value,
        size_t valueSize,
        const char* needle) {
        const size_t needleSize = strlen(needle);
        if (needleSize == 0U) return true;
        if (needleSize > valueSize) return false;
        for (size_t start = 0; start <= valueSize - needleSize; ++start) {
            size_t index = 0;
            for (; index < needleSize; ++index) {
                if (!asciiEqualIgnoreCase(value[start + index], needle[index])) {
                    break;
                }
            }
            if (index == needleSize) return true;
        }
        return false;
    }

    Result appendHeader(StringView value) {
        if (value.size > RequestCapacity - requestHeaderSize_) {
            return fail(ErrorCode::BufferTooSmall);
        }
        if (value.size != 0U) {
            memcpy(requestHeader_ + requestHeaderSize_, value.data, value.size);
            requestHeaderSize_ += value.size;
        }
        return Result::success();
    }

    Result appendLiteral(const char* value) {
        return appendHeader(StringView(value, strlen(value)));
    }

    Result appendUnsigned(size_t value) {
        char digits[20];
        size_t count = 0;
        do {
            digits[count++] = static_cast<char>('0' + (value % 10U));
            value /= 10U;
        } while (value != 0U);
        char ordered[20];
        for (size_t index = 0; index < count; ++index) {
            ordered[index] = digits[count - index - 1U];
        }
        return appendHeader(StringView(ordered, count));
    }

    Result buildRequestHeader() {
        Result result = appendLiteral(
            request_.method == HttpMethod::Get ? "GET " : "POST ");
        if (result) result = appendHeader(request_.target);
        if (result) result = appendLiteral(" HTTP/1.1\r\nHost: ");
        if (result) result = appendHeader(request_.host);
        const bool defaultPort =
            (request_.security == HttpSecurity::Tls && request_.port == 443U) ||
            (request_.security == HttpSecurity::PlainTcp && request_.port == 80U);
        if (result && !defaultPort) result = appendLiteral(":");
        if (result && !defaultPort) result = appendUnsigned(request_.port);
        if (result) result = appendLiteral("\r\n");
        for (uint8_t index = 0U;
             result && index < request_.headerCount;
             ++index) {
            result = appendHeader(request_.headers[index].name);
            if (result) result = appendLiteral(": ");
            if (result) result = appendHeader(request_.headers[index].value);
            if (result) result = appendLiteral("\r\n");
        }
        if (result) result = appendLiteral("Connection: close\r\n");
        if (result && request_.method == HttpMethod::Post) {
            result = appendLiteral("Content-Type: ");
            if (result) {
                result = request_.contentType.empty()
                             ? appendLiteral("application/octet-stream")
                             : appendHeader(request_.contentType);
            }
            if (result) result = appendLiteral("\r\nContent-Length: ");
            if (result) result = appendUnsigned(request_.body.size);
            if (result) result = appendLiteral("\r\n");
        }
        if (result) result = appendLiteral("\r\n");
        return result;
    }

    Result consumeHeaderByte(uint8_t byte) {
        if (byte == static_cast<uint8_t>('\n')) {
            size_t size = headerLineSize_;
            if (size != 0U && headerLine_[size - 1U] == '\r') --size;
            const Result result = consumeHeaderLine(headerLine_, size);
            headerLineSize_ = 0;
            return result;
        }
        if (headerLineSize_ >= HeaderLineCapacity) {
            return fail(ErrorCode::CapacityExceeded);
        }
        headerLine_[headerLineSize_++] = static_cast<char>(byte);
        return Result::success();
    }

    Result consumeHeaderLine(const char* line, size_t size) {
        if (phase_ == PhaseStatusLine) {
            if (size < 12U || memcmp(line, "HTTP/1.", 7U) != 0 ||
                (line[7] != '0' && line[7] != '1') ||
                line[8] != ' ' || line[9] < '1' || line[9] > '5' ||
                line[10] < '0' || line[10] > '9' ||
                line[11] < '0' || line[11] > '9') {
                return fail(ErrorCode::InvalidEncoding);
            }
            response_.statusCode = static_cast<uint16_t>(
                (line[9] - '0') * 100 +
                (line[10] - '0') * 10 +
                (line[11] - '0'));
            statusParsed_ = true;
            phase_ = PhaseHeaders;
            return Result::success();
        }
        if (phase_ != PhaseHeaders || !statusParsed_) {
            return fail(ErrorCode::ProtocolError);
        }
        if (size == 0U) {
            if (hasContentLength_ &&
                contentLength_ > request_.responseBuffer.size) {
                return fail(ErrorCode::BufferTooSmall);
            }
            if (hasContentLength_ && contentLength_ == 0U) {
                complete();
            } else {
                phase_ = PhaseBody;
            }
            return Result::success();
        }
        if (startsWithIgnoreCase(line, size, "Content-Length:")) {
            if (hasContentLength_) return fail(ErrorCode::DuplicateField);
            size_t offset = strlen("Content-Length:");
            while (offset < size && (line[offset] == ' ' || line[offset] == '\t')) {
                ++offset;
            }
            if (offset == size) return fail(ErrorCode::InvalidEncoding);
            size_t length = 0;
            for (; offset < size; ++offset) {
                if (line[offset] < '0' || line[offset] > '9') {
                    return fail(ErrorCode::InvalidEncoding);
                }
                const size_t digit = static_cast<size_t>(line[offset] - '0');
                if (length > (SIZE_MAX - digit) / 10U) {
                    return fail(ErrorCode::ValueOutOfRange);
                }
                length = length * 10U + digit;
            }
            contentLength_ = length;
            hasContentLength_ = true;
        } else if (startsWithIgnoreCase(line, size, "Transfer-Encoding:") &&
                   containsIgnoreCase(line, size, "chunked")) {
            return fail(ErrorCode::UnsupportedFeature);
        }
        return Result::success();
    }

    void complete() {
        client_.stop();
        response_.body = ByteView(request_.responseBuffer.data, responseSize_);
        currentError_ = ErrorCode::Ok;
        currentState_ = HttpClientState::Complete;
        phase_ = PhaseIdle;
    }

    Result fail(ErrorCode error) {
        client_.stop();
        currentError_ = error;
        currentState_ = HttpClientState::Failed;
        phase_ = PhaseIdle;
        return Result::failure(error);
    }

    static void secureClear(void* data, size_t size) {
        volatile uint8_t* bytes = static_cast<volatile uint8_t*>(data);
        for (size_t index = 0; index < size; ++index) bytes[index] = 0U;
    }

    TClient& client_;
    HttpSecurity actualSecurity_;
    IClock& clock_;
    HttpRequest request_;
    HttpResponse response_;
    HttpClientState currentState_;
    Phase phase_;
    ErrorCode currentError_;
    char host_[HostCapacity];
    char requestHeader_[RequestCapacity];
    char headerLine_[HeaderLineCapacity];
    size_t requestHeaderSize_;
    size_t requestHeaderSent_;
    size_t requestBodySent_;
    size_t responseSize_;
    size_t headerLineSize_;
    size_t contentLength_;
    uint32_t startedAt_;
    bool hasContentLength_;
    bool statusParsed_;
};

} // namespace blinker

#endif
