#ifndef BLINKER_CORE_SPAN_H
#define BLINKER_CORE_SPAN_H

#include <stddef.h>
#include <stdint.h>

namespace blinker {

struct ByteView {
    const uint8_t* data;
    size_t size;

    constexpr ByteView() : data(nullptr), size(0) {}
    constexpr ByteView(const uint8_t* bytes, size_t length)
        : data(bytes), size(length) {}

    bool empty() const { return size == 0; }

    ByteView subspan(size_t offset, size_t length) const {
        if (offset > size || length > size - offset) {
            return ByteView();
        }
        return ByteView(data + offset, length);
    }
};

struct MutableByteSpan {
    uint8_t* data;
    size_t size;

    MutableByteSpan() : data(nullptr), size(0) {}
    MutableByteSpan(uint8_t* bytes, size_t length) : data(bytes), size(length) {}

    bool empty() const { return size == 0; }
};

struct StringView {
    const char* data;
    size_t size;

    constexpr StringView() : data(nullptr), size(0) {}
    constexpr StringView(const char* text, size_t length)
        : data(text), size(length) {}

    // String literals and const character arrays carry their byte length in
    // the type. Keep the pointer+length constructor for borrowed runtime text,
    // but do not make sketches or generated code count literal bytes by hand.
    template <size_t N>
    constexpr StringView(const char (&text)[N])
        : data(text), size(N > 0U ? N - 1U : 0U) {}

    bool empty() const { return size == 0; }
};

struct MutableCharSpan {
    char* data;
    size_t size;

    MutableCharSpan() : data(nullptr), size(0) {}
    MutableCharSpan(char* text, size_t length) : data(text), size(length) {}

    bool empty() const { return size == 0; }
};

} // namespace blinker

#endif
