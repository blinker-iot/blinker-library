#ifndef BLINKER_BASE64_COMPAT_H
#define BLINKER_BASE64_COMPAT_H

#include "BlinkerPlatformDetector.h"

#if defined(BLINKER_NATIVE_WIFI) && !defined(BLINKER_PLATFORM_ESP32)

#include <Arduino.h>
#include "../../modules/base64/Base64.h"

namespace base64 {
    static inline String encode(const String& input)
    {
        int encodedLen = base64_enc_len(input.length());
        char* encoded = (char*)malloc((encodedLen + 1) * sizeof(char));
        if (!encoded) return String();
        base64_encode(encoded, (char*)input.c_str(), input.length());
        String output(encoded);
        free(encoded);
        return output;
    }
}

#endif

#endif
