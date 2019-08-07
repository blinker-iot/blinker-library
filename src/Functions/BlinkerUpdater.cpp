#if defined(ESP8266) || defined(ESP32)

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerDebug.h"
#include "BlinkerUpdater.h"

#if defined(ESP8266)

// #include "ESP8266httpUpdate.h"
// #include <StreamString.h>

// extern "C" uint32_t _SPIFFS_start;
// extern "C" uint32_t _SPIFFS_end;

// BlinkerHTTPUpdate::BlinkerHTTPUpdate(void)
//         : _httpClientTimeout(8000), _followRedirects(false), _ledPin(-1)
// {
// }

// BlinkerHTTPUpdate::BlinkerHTTPUpdate(int httpClientTimeout)
//         : _httpClientTimeout(httpClientTimeout), _followRedirects(false), _ledPin(-1)
// {
// }

// BlinkerHTTPUpdate::~BlinkerHTTPUpdate(void)
// {
// }

// #ifdef HTTPUPDATE_1_2_COMPATIBLE
// BlinkerHTTPUpdateResult BlinkerHTTPUpdate::update(const String& url, const String& currentVersion,
//         const String& httpsFingerprint, bool reboot)
// {
//     rebootOnUpdate(reboot);
// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored  "-Wdeprecated-declarations"
//     return update(url, currentVersion, httpsFingerprint);
// #pragma GCC diagnostic pop
// }

// BlinkerHTTPUpdateResult BlinkerHTTPUpdate::update(const String& url, const String& currentVersion)
// {
//     HTTPClient http;
// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored  "-Wdeprecated-declarations"
//     http.begin(url);
// #pragma GCC diagnostic pop
//     return handleUpdate(http, currentVersion, false);
// }

// BlinkerHTTPUpdateResult BlinkerHTTPUpdate::update(const String& url, const String& currentVersion,
//         const String& httpsFingerprint)
// {
//     HTTPClient http;
// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored  "-Wdeprecated-declarations"
//     http.begin(url, httpsFingerprint);
// #pragma GCC diagnostic pop
//     return handleUpdate(http, currentVersion, false);
// }

// BlinkerHTTPUpdateResult BlinkerHTTPUpdate::update(const String& url, const String& currentVersion,
//         const uint8_t httpsFingerprint[20])
// {
//     HTTPClient http;
// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored  "-Wdeprecated-declarations"
//     http.begin(url, httpsFingerprint);
// #pragma GCC diagnostic pop
//     return handleUpdate(http, currentVersion, false);
// }
// #endif

// BlinkerHTTPUpdateResult BlinkerHTTPUpdate::update(WiFiClient& client, const String& url, const String& currentVersion)
// {
//     HTTPClient http;
//     http.begin(client, url);
//     return handleUpdate(http, currentVersion, false);
// }

// BlinkerHTTPUpdateResult BlinkerHTTPUpdate::update(WiFiClient& client, const String& url, const String& md5, const String& currentVersion)
// {
//     HTTPClient http;
//     http.begin(client, url);
//     return handleUpdate(http, currentVersion, false, md5);
// }

// #ifdef HTTPUPDATE_1_2_COMPATIBLE
// BlinkerHTTPUpdateResult BlinkerHTTPUpdate::updateSpiffs(const String& url, const String& currentVersion, const String& httpsFingerprint)
// {
//     HTTPClient http;
// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored  "-Wdeprecated-declarations"
//     http.begin(url, httpsFingerprint);
// #pragma GCC diagnostic pop
//     return handleUpdate(http, currentVersion, true);
// }

// BlinkerHTTPUpdateResult BlinkerHTTPUpdate::updateSpiffs(const String& url, const String& currentVersion, const uint8_t httpsFingerprint[20])
// {
//     HTTPClient http;
// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored  "-Wdeprecated-declarations"
//     http.begin(url, httpsFingerprint);
// #pragma GCC diagnostic pop
//     return handleUpdate(http, currentVersion, true);
// }

// BlinkerHTTPUpdateResult BlinkerHTTPUpdate::updateSpiffs(const String& url, const String& currentVersion)
// {
//     HTTPClient http;
// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored  "-Wdeprecated-declarations"
//     http.begin(url);
// #pragma GCC diagnostic pop
//     return handleUpdate(http, currentVersion, true);
// }
// #endif

// BlinkerHTTPUpdateResult BlinkerHTTPUpdate::updateSpiffs(WiFiClient& client, const String& url, const String& currentVersion)
// {
//     HTTPClient http;
//     http.begin(client, url);
//     return handleUpdate(http, currentVersion, true);
// }

// #ifdef HTTPUPDATE_1_2_COMPATIBLE
// BlinkerHTTPUpdateResult BlinkerHTTPUpdate::update(const String& host, uint16_t port, const String& uri, const String& currentVersion,
//         bool https, const String& httpsFingerprint, bool reboot)
// {
//     (void)https;
//     rebootOnUpdate(reboot);
//     if (httpsFingerprint.length() == 0) {
// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored  "-Wdeprecated-declarations"
//         return update(host, port, uri, currentVersion);
//     } else {
//         return update(host, port, uri, currentVersion, httpsFingerprint);
// #pragma GCC diagnostic pop
//     }
// }

// BlinkerHTTPUpdateResult BlinkerHTTPUpdate::update(const String& host, uint16_t port, const String& uri,
//         const String& currentVersion)
// {
//     HTTPClient http;
// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored  "-Wdeprecated-declarations"
//     http.begin(host, port, uri);
// #pragma GCC diagnostic pop
//     return handleUpdate(http, currentVersion, false);
// }

// BlinkerHTTPUpdateResult BlinkerHTTPUpdate::update(const String& host, uint16_t port, const String& url,
//         const String& currentVersion, const String& httpsFingerprint)
// {
//     HTTPClient http;
// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored  "-Wdeprecated-declarations"
//     http.begin(host, port, url, httpsFingerprint);
// #pragma GCC diagnostic pop
//     return handleUpdate(http, currentVersion, false);
// }

// BlinkerHTTPUpdateResult BlinkerHTTPUpdate::update(const String& host, uint16_t port, const String& url,
//         const String& currentVersion, const uint8_t httpsFingerprint[20])
// {
//     HTTPClient http;
// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored  "-Wdeprecated-declarations"
//     http.begin(host, port, url, httpsFingerprint);
// #pragma GCC diagnostic pop
//     return handleUpdate(http, currentVersion, false);
// }
// #endif

// BlinkerHTTPUpdateResult BlinkerHTTPUpdate::update(WiFiClient& client, const String& host, uint16_t port, const String& uri,
//         const String& currentVersion)
// {
//     HTTPClient http;
//     http.begin(client, host, port, uri);
//     return handleUpdate(http, currentVersion, false);
// }

// /**
//  * return error code as int
//  * @return int error code
//  */
// int BlinkerHTTPUpdate::getLastError(void)
// {
//     return _lastError;
// }

// /**
//  * return error code as String
//  * @return String error
//  */
// String BlinkerHTTPUpdate::getLastErrorString(void)
// {

//     if(_lastError == 0) {
//         return String(); // no error
//     }

//     // error from Update class
//     if(_lastError > 0) {
//         StreamString error;
//         Update.printError(error);
//         error.trim(); // remove line ending
//         return String(F("Update error: ")) + error;
//     }

//     // error from http client
//     if(_lastError > -100) {
//         return String(F("HTTP error: ")) + HTTPClient::errorToString(_lastError);
//     }

//     switch(_lastError) {
//     case HTTP_UE_TOO_LESS_SPACE:
//         return F("Not Enough space");
//     case HTTP_UE_SERVER_NOT_REPORT_SIZE:
//         return F("Server Did Not Report Size");
//     case HTTP_UE_SERVER_FILE_NOT_FOUND:
//         return F("File Not Found (404)");
//     case HTTP_UE_SERVER_FORBIDDEN:
//         return F("Forbidden (403)");
//     case HTTP_UE_SERVER_WRONG_HTTP_CODE:
//         return F("Wrong HTTP Code");
//     case HTTP_UE_SERVER_FAULTY_MD5:
//         return F("Wrong MD5");
//     case HTTP_UE_BIN_VERIFY_HEADER_FAILED:
//         return F("Verify Bin Header Failed");
//     case HTTP_UE_BIN_FOR_WRONG_FLASH:
//         return F("New Binary Does Not Fit Flash Size");
//     }

//     return String();
// }


// /**
//  *
//  * @param http HTTPClient *
//  * @param currentVersion const char *
//  * @return BlinkerHTTPUpdateResult
//  */
// BlinkerHTTPUpdateResult BlinkerHTTPUpdate::handleUpdate(HTTPClient& http, const String& currentVersion, bool spiffs, const String& md5)
// {

//     BlinkerHTTPUpdateResult ret = HTTP_UPDATE_FAILED;

//     // use HTTP/1.0 for update since the update handler not support any transfer Encoding
//     http.useHTTP10(true);
//     http.setTimeout(_httpClientTimeout);
//     http.setFollowRedirects(_followRedirects);
//     http.setUserAgent(F("ESP8266-http-Update"));
//     http.addHeader(F("x-ESP8266-STA-MAC"), WiFi.macAddress());
//     http.addHeader(F("x-ESP8266-AP-MAC"), WiFi.softAPmacAddress());
//     http.addHeader(F("x-ESP8266-free-space"), String(ESP.getFreeSketchSpace()));
//     http.addHeader(F("x-ESP8266-sketch-size"), String(ESP.getSketchSize()));
//     http.addHeader(F("x-ESP8266-sketch-md5"), String(ESP.getSketchMD5()));
//     http.addHeader(F("x-ESP8266-chip-size"), String(ESP.getFlashChipRealSize()));
//     http.addHeader(F("x-ESP8266-sdk-version"), ESP.getSdkVersion());

//     if(spiffs) {
//         http.addHeader(F("x-ESP8266-mode"), F("spiffs"));
//     } else {
//         http.addHeader(F("x-ESP8266-mode"), F("sketch"));
//     }

//     if(currentVersion && currentVersion[0] != 0x00) {
//         http.addHeader(F("x-ESP8266-version"), currentVersion);
//     }

//     const char * headerkeys[] = { "x-MD5" };
//     size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);

//     // track these headers
//     http.collectHeaders(headerkeys, headerkeyssize);


//     int code = http.GET();
//     int len = http.getSize();

//     if(code <= 0) {
//         BLINKER_LOG_ALL(BLINKER_F("[httpUpdate] HTTP error: "), http.errorToString(code).c_str());
//         _lastError = code;
//         http.end();
//         return HTTP_UPDATE_FAILED;
//     }


//     BLINKER_LOG_ALL(BLINKER_F("[httpUpdate] Header read fin."));
//     BLINKER_LOG_ALL(BLINKER_F("[httpUpdate] Server header:"));
//     BLINKER_LOG_ALL(BLINKER_F("[httpUpdate]  - code: "), code);
//     BLINKER_LOG_ALL(BLINKER_F("[httpUpdate]  - len: "), len);

//     if(http.hasHeader("x-MD5")) {
//         BLINKER_LOG_ALL(BLINKER_F("[httpUpdate]  - MD5: "), http.header("x-MD5").c_str());
//     }
//     if(md5.length() > 1) {
//         BLINKER_LOG_ALL(BLINKER_F("[httpUpdate]  - MD5: "), md5);
//     }

//     BLINKER_LOG_ALL(BLINKER_F("[httpUpdate] ESP8266 info:"));
//     BLINKER_LOG_ALL(BLINKER_F("[httpUpdate]  - free Space: "), ESP.getFreeSketchSpace());
//     BLINKER_LOG_ALL(BLINKER_F("[httpUpdate]  - current Sketch Size: "), ESP.getSketchSize());

//     if(currentVersion && currentVersion[0] != 0x00) {
//         BLINKER_LOG_ALL(BLINKER_F("[httpUpdate]  - current version: "), currentVersion.c_str() );
//     }

//     switch(code) {
//     case HTTP_CODE_OK:  ///< OK (Start Update)
//         if(len > 0) {
//             bool startUpdate = true;
//             if(spiffs) {
//                 size_t spiffsSize = ((size_t) &_SPIFFS_end - (size_t) &_SPIFFS_start);
//                 if(len > (int) spiffsSize) {
//                     BLINKER_LOG_ALL(BLINKER_F("[httpUpdate] spiffsSize to low "), spiffsSize, BLINKER_F(" needed: "), len);
//                     startUpdate = false;
//                 }
//             } else {
//                 if(len > (int) ESP.getFreeSketchSpace()) {
//                     BLINKER_LOG_ALL(BLINKER_F("[httpUpdate] FreeSketchSpace to low "), ESP.getFreeSketchSpace(), BLINKER_F(" needed: "), len);
//                     startUpdate = false;
//                 }
//             }

//             if(!startUpdate) {
//                 _lastError = HTTP_UE_TOO_LESS_SPACE;
//                 ret = HTTP_UPDATE_FAILED;
//             } else {

//                 WiFiClient * tcp = http.getStreamPtr();

//                 WiFiUDP::stopAll();
//                 WiFiClient::stopAllExcept(tcp);

//                 delay(100);

//                 int command;

//                 if(spiffs) {
//                     command = U_SPIFFS;
//                     BLINKER_LOG_ALL(BLINKER_F("[httpUpdate] runUpdate spiffs..."));
//                 } else {
//                     command = U_FLASH;
//                     BLINKER_LOG_ALL(BLINKER_F("[httpUpdate] runUpdate flash..."));
//                 }

//                 if(!spiffs) {
//                     uint8_t buf[4];
//                     if(tcp->peekBytes(&buf[0], 4) != 4) {
//                         BLINKER_LOG_ALL(BLINKER_F("[httpUpdate] peekBytes magic header failed"));
//                         _lastError = HTTP_UE_BIN_VERIFY_HEADER_FAILED;
//                         http.end();
//                         return HTTP_UPDATE_FAILED;
//                     }

//                     // check for valid first magic byte
//                     if(buf[0] != 0xE9) {
//                         BLINKER_LOG_ALL(BLINKER_F("[httpUpdate] Magic header does not start with 0xE9"));
//                         _lastError = HTTP_UE_BIN_VERIFY_HEADER_FAILED;
//                         http.end();
//                         return HTTP_UPDATE_FAILED;

//                     }

//                     uint32_t bin_flash_size = ESP.magicFlashChipSize((buf[3] & 0xf0) >> 4);

//                     // check if new bin fits to SPI flash
//                     if(bin_flash_size > ESP.getFlashChipRealSize()) {
//                         BLINKER_LOG_ALL(BLINKER_F("[httpUpdate] New binary does not fit SPI Flash size"));
//                         _lastError = HTTP_UE_BIN_FOR_WRONG_FLASH;
//                         http.end();
//                         return HTTP_UPDATE_FAILED;
//                     }
//                 }
//                 if (md5.length() > 1)
//                 {
//                     if(runUpdate(*tcp, len, md5, command)) {
//                         ret = HTTP_UPDATE_OK;
//                         BLINKER_LOG_ALL(BLINKER_F("[httpUpdate] Update ok"));
//                         http.end();

//                         if(_rebootOnUpdate && !spiffs) {
//                             ESP.restart();
//                         }

//                     } else {
//                         ret = HTTP_UPDATE_FAILED;
//                         BLINKER_LOG_ALL(BLINKER_F("[httpUpdate] Update failed"));
//                     }
//                 }
//                 else
//                 {
//                     if(runUpdate(*tcp, len, http.header("x-MD5"), command)) {
//                         ret = HTTP_UPDATE_OK;
//                         BLINKER_LOG_ALL(BLINKER_F("[httpUpdate] Update ok"));
//                         http.end();

//                         if(_rebootOnUpdate && !spiffs) {
//                             ESP.restart();
//                         }

//                     } else {
//                         ret = HTTP_UPDATE_FAILED;
//                         BLINKER_LOG_ALL(BLINKER_F("[httpUpdate] Update failed"));
//                     }
//                 }
//             }
//         } else {
//             _lastError = HTTP_UE_SERVER_NOT_REPORT_SIZE;
//             ret = HTTP_UPDATE_FAILED;
//             BLINKER_LOG_ALL(BLINKER_F("[httpUpdate] Content-Length was 0 or wasn't set by Server?!"));
//         }
//         break;
//     case HTTP_CODE_NOT_MODIFIED:
//         ///< Not Modified (No updates)
//         ret = HTTP_UPDATE_NO_UPDATES;
//         break;
//     case HTTP_CODE_NOT_FOUND:
//         _lastError = HTTP_UE_SERVER_FILE_NOT_FOUND;
//         ret = HTTP_UPDATE_FAILED;
//         break;
//     case HTTP_CODE_FORBIDDEN:
//         _lastError = HTTP_UE_SERVER_FORBIDDEN;
//         ret = HTTP_UPDATE_FAILED;
//         break;
//     default:
//         _lastError = HTTP_UE_SERVER_WRONG_HTTP_CODE;
//         ret = HTTP_UPDATE_FAILED;
//         BLINKER_LOG_ALL(BLINKER_F("[httpUpdate] HTTP Code is "), code);
//         //http.writeToStream(&Serial1);
//         break;
//     }

//     http.end();
//     return ret;
// }

// /**
//  * write Update to flash
//  * @param in Stream&
//  * @param size uint32_t
//  * @param md5 String
//  * @return true if Update ok
//  */
// bool BlinkerHTTPUpdate::runUpdate(Stream& in, uint32_t size, String md5, int command)
// {

//     StreamString error;

//     if(!Update.begin(size, command, _ledPin, _ledOn)) {
//         _lastError = Update.getError();
//         Update.printError(error);
//         error.trim(); // remove line ending
//         BLINKER_LOG_ALL(BLINKER_F("[httpUpdate] Update.begin failed! "), error.c_str());
//         return false;
//     }

//     if(md5.length()) {
//         if(!Update.setMD5(md5.c_str())) {
//             _lastError = HTTP_UE_SERVER_FAULTY_MD5;
//             BLINKER_LOG_ALL(BLINKER_F("[httpUpdate] Update.setMD5 failed! "), md5.c_str());
//             return false;
//         }
//     }

//     if(Update.writeStream(in) != size) {
//         _lastError = Update.getError();
//         Update.printError(error);
//         error.trim(); // remove line ending
//         BLINKER_LOG_ALL(BLINKER_F("[httpUpdate] Update.writeStream failed! "), error.c_str());
//         return false;
//     }

//     if(!Update.end()) {
//         _lastError = Update.getError();
//         Update.printError(error);
//         error.trim(); // remove line ending
//         BLINKER_LOG_ALL(BLINKER_F("[httpUpdate] Update.end failed! "), error.c_str());
//         return false;
//     }

//     return true;
// }

// #if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_HTTPUPDATE)
// BlinkerHTTPUpdate BlinkerhttpUpdate;
// #endif

#include <Updater.h>
#include <Arduino.h>
#include <eboot_command.h>
#include <interrupts.h>
#include <esp8266_peri.h>

//#define DEBUG_UPDATER Serial

extern "C" {
    #include <c_types.h>
    #include <spi_flash.h>
    #include <user_interface.h>
}

// #if defined _SPIFFS_start
extern "C" uint32_t _SPIFFS_start;
extern "C" uint32_t _SPIFFS_end;
// #elif defined _FS_start
// extern "C" uint32_t _FS_start;
// extern "C" uint32_t _FS_end;
// #endif

BlinkerUpdaterClass::BlinkerUpdaterClass()
: _async(false)
, _error(0)
, _buffer(0)
, _bufferLen(0)
, _size(0)
, _progress_callback(NULL)
, _startAddress(0)
, _currentAddress(0)
, _command(U_FLASH)
{
}

BlinkerUpdaterClass& BlinkerUpdaterClass::onProgress(THandlerFunction_Progress fn) {
    _progress_callback = fn;
    return *this;
}

void BlinkerUpdaterClass::_reset() {
    if (_buffer)
        delete[] _buffer;
    _buffer = 0;
    _bufferLen = 0;
    _startAddress = 0;
    _currentAddress = 0;
    _size = 0;
    _command = U_FLASH;

    // if(_ledPin != -1) {
    //     digitalWrite(_ledPin, !_ledOn); // off
    // }
}

bool BlinkerUpdaterClass::begin(size_t size, int command, int ledPin, uint8_t ledOn) {
    if(_size > 0){
    // #ifdef DEBUG_UPDATER
    //     DEBUG_UPDATER.println(F("[begin] already running"));
    // #endif
        BLINKER_LOG_ALL(F("[begin] already running"));
        return false;
    }

    // _ledPin = ledPin;
    // _ledOn = !!ledOn; // 0(LOW) or 1(HIGH)

    /* Check boot mode; if boot mode is 1 (UART download mode),
        we will not be able to reset into normal mode once update is done.
        Fail early to avoid frustration.
        https://github.com/esp8266/Arduino/issues/1017#issuecomment-200605576
    */
    int boot_mode = (GPI >> 16) & 0xf;
    if (boot_mode == 1) {
        _setError(UPDATE_ERROR_BOOTSTRAP);
        return false;
    }

    // #ifdef DEBUG_UPDATER
    if (command == U_SPIFFS) {
        // DEBUG_UPDATER.println(F("[begin] Update SPIFFS."));
        BLINKER_LOG_ALL(F("[begin] Update SPIFFS."));
    }
    // #endif

    if(size == 0) {
        _setError(UPDATE_ERROR_SIZE);
        return false;
    }

    if(!ESP.checkFlashConfig(false)) {
        _setError(UPDATE_ERROR_FLASH_CONFIG);
        return false;
    }

    _reset();
    clearError(); //  _error = 0

    wifi_set_sleep_type(NONE_SLEEP_T);

    uintptr_t updateStartAddress = 0;
    if (command == U_FLASH) {
        //size of current sketch rounded to a sector
        size_t currentSketchSize = (ESP.getSketchSize() + FLASH_SECTOR_SIZE - 1) & (~(FLASH_SECTOR_SIZE - 1));
        //address of the end of the space available for sketch and update
        uintptr_t updateEndAddress;
        // #if defined _SPIFFS_start
        updateEndAddress = (uintptr_t)&_SPIFFS_start - 0x40200000;
        // #elif defined _FS_start
        // updateEndAddress = (uintptr_t)&_FS_start - 0x40200000;
        // #endif

        // if ((uintptr_t)&_SPIFFS_start > (uintptr_t)&_SPIFFS_end)
        // {
        //     updateEndAddress = (uintptr_t)&_SPIFFS_end - 0x40200000;
        // }
        // else
        // {
        //     updateEndAddress = (uintptr_t)&_SPIFFS_start - 0x40200000;
        // }
        //size of the update rounded to a sector
        size_t roundedSize = (size + FLASH_SECTOR_SIZE - 1) & (~(FLASH_SECTOR_SIZE - 1));
        //address where we will start writing the update
        updateStartAddress = (updateEndAddress > roundedSize)? (updateEndAddress - roundedSize) : 0;

    // #ifdef DEBUG_UPDATER
    //     DEBUG_UPDATER.printf("[begin] roundedSize:       0x%08zX (%zd)\n", roundedSize, roundedSize);
    //     DEBUG_UPDATER.printf("[begin] updateEndAddress:  0x%08zX (%zd)\n", updateEndAddress, updateEndAddress);
    //     DEBUG_UPDATER.printf("[begin] currentSketchSize: 0x%08zX (%zd)\n", currentSketchSize, currentSketchSize);
    // #endif
        // BLINKER_LOG_ALL(F("[begin] _SPIFFS_start: "), _SPIFFS_start, F(" "), _SPIFFS_start);
        BLINKER_LOG_ALL(F("[begin] roundedSize: "), roundedSize, F(" "), roundedSize);
        BLINKER_LOG_ALL(F("[begin] updateEndAddress: "), updateEndAddress, F(" "), updateEndAddress);
        BLINKER_LOG_ALL(F("[begin] currentSketchSize: "), currentSketchSize, F(" "), currentSketchSize);

        //make sure that the size of both sketches is less than the total space (updateEndAddress)
        if(updateStartAddress < currentSketchSize) {
        _setError(UPDATE_ERROR_SPACE);
        return false;
        }
    }
    else if (command == U_SPIFFS) {
        // #if defined _SPIFFS_start
        updateStartAddress = (uintptr_t)&_SPIFFS_start - 0x40200000;
        // #elif defined _FS_start
        // updateStartAddress = (uintptr_t)&_FS_start - 0x40200000;
        // #endif
    }
    else {
        // unknown command
    // #ifdef DEBUG_UPDATER
    //     DEBUG_UPDATER.println(F("[begin] Unknown update command."));
    // #endif
        BLINKER_LOG_ALL(F("[begin] Unknown update command."));
        return false;
    }

    //initialize
    _startAddress = updateStartAddress;
    _currentAddress = _startAddress;
    _size = size;
    if (ESP.getFreeHeap() > 2 * FLASH_SECTOR_SIZE) {
        _bufferSize = FLASH_SECTOR_SIZE;
    } else {
        _bufferSize = 256;
    }
    _buffer = new uint8_t[_bufferSize];
    _command = command;

    // #ifdef DEBUG_UPDATER
    //     DEBUG_UPDATER.printf("[begin] _startAddress:     0x%08X (%d)\n", _startAddress, _startAddress);
    //     DEBUG_UPDATER.printf("[begin] _currentAddress:   0x%08X (%d)\n", _currentAddress, _currentAddress);
    //     DEBUG_UPDATER.printf("[begin] _size:             0x%08zX (%zd)\n", _size, _size);
    // #endif
    BLINKER_LOG_ALL(F("[begin] _startAddress: "), _startAddress, F(" "), _startAddress);
    BLINKER_LOG_ALL(F("[begin] _currentAddress: "), _currentAddress, F(" "), _currentAddress);
    BLINKER_LOG_ALL(F("[begin] _size: "), _size, F(" "), _size);

    _md5.begin();
    return true;
}

bool BlinkerUpdaterClass::setMD5(const char * expected_md5){
    if(strlen(expected_md5) != 32)
    {
        return false;
    }
    _target_md5 = expected_md5;
    return true;
}

bool BlinkerUpdaterClass::end(bool evenIfRemaining){
    if(_size == 0){
    // #ifdef DEBUG_UPDATER
    //     DEBUG_UPDATER.println(F("no update"));
    // #endif
        BLINKER_LOG_ALL(F("no update"));
        return false;
    }

    if(hasError() || (!isFinished() && !evenIfRemaining)){
    // #ifdef DEBUG_UPDATER
    //     DEBUG_UPDATER.printf("premature end: res:%u, pos:%zu/%zu\n", getError(), progress(), _size);
    // #endif
        BLINKER_LOG_ALL(F("premature end: res: "), getError(), F(", pos:"), progress(), _size);

        _reset();
        return false;
    }

    if(evenIfRemaining) {
        if(_bufferLen > 0) {
        _writeBuffer();
        }
        _size = progress();
    }

    _md5.calculate();
    if(_target_md5.length()) {
        if(strcasecmp(_target_md5.c_str(), _md5.toString().c_str()) != 0){
        _setError(UPDATE_ERROR_MD5);
        _reset();
        return false;
        }
        // #ifdef DEBUG_UPDATER
        //     else DEBUG_UPDATER.printf("MD5 Success: %s\n", _target_md5.c_str());
        // #endif
        BLINKER_LOG_ALL(F("MD5 Success: "), _target_md5.c_str());
    }

    if(!_verifyEnd()) {
        _reset();
        return false;
    }

    if (_command == U_FLASH) {
        eboot_command ebcmd;
        ebcmd.action = ACTION_COPY_RAW;
        ebcmd.args[0] = _startAddress;
        ebcmd.args[1] = 0x00000;
        ebcmd.args[2] = _size;
        eboot_command_write(&ebcmd);

    // #ifdef DEBUG_UPDATER
    //     DEBUG_UPDATER.printf("Staged: address:0x%08X, size:0x%08zX\n", _startAddress, _size);
        BLINKER_LOG_ALL(F("Staged: address: "), _startAddress, F(", size: "), _size);
    }
    else if (_command == U_SPIFFS) {
    //     DEBUG_UPDATER.printf("SPIFFS: address:0x%08X, size:0x%08zX\n", _startAddress, _size);
    // #endif
        BLINKER_LOG_ALL(F("SPIFFS: address: "), _startAddress, F(", size: "), _size);
    }

    _reset();
    return true;
}

bool BlinkerUpdaterClass::_writeBuffer(){
    #define FLASH_MODE_PAGE  0
    #define FLASH_MODE_OFFSET  2

    bool eraseResult = true, writeResult = true;
    if (_currentAddress % FLASH_SECTOR_SIZE == 0) {
        if(!_async) yield();
        eraseResult = ESP.flashEraseSector(_currentAddress/FLASH_SECTOR_SIZE);
    }

    // If the flash settings don't match what we already have, modify them.
    // But restore them after the modification, so the hash isn't affected.
    // This is analogous to what esptool.py does when it receives a --flash_mode argument.
    bool modifyFlashMode = false;
    FlashMode_t flashMode = FM_QIO;
    FlashMode_t bufferFlashMode = FM_QIO;
    if (_currentAddress == _startAddress + FLASH_MODE_PAGE) {
        flashMode = ESP.getFlashChipMode();
        // #ifdef DEBUG_UPDATER
        // DEBUG_UPDATER.printf("Header: 0x%1X %1X %1X %1X\n", _buffer[0], _buffer[1], _buffer[2], _buffer[3]);
        // #endif
        BLINKER_LOG_ALL(F("Header: "), _buffer[0], F(" "), _buffer[1], F(" "), _buffer[2], F(" "), _buffer[3]);

        bufferFlashMode = ESP.magicFlashChipMode(_buffer[FLASH_MODE_OFFSET]);
        if (bufferFlashMode != flashMode) {
        // #ifdef DEBUG_UPDATER
        //     DEBUG_UPDATER.printf("Set flash mode from 0x%1X to 0x%1X\n", bufferFlashMode, flashMode);
        // #endif
        BLINKER_LOG_ALL(F("Set flash mode from "), bufferFlashMode, " to ", flashMode);

        _buffer[FLASH_MODE_OFFSET] = flashMode;
        modifyFlashMode = true;
        }
    }

    if (eraseResult) {
        if(!_async) yield();
        writeResult = ESP.flashWrite(_currentAddress, (uint32_t*) _buffer, _bufferLen);
    } else { // if erase was unsuccessful
        _currentAddress = (_startAddress + _size);
        _setError(UPDATE_ERROR_ERASE);
        return false;
    }

    // Restore the old flash mode, if we modified it.
    // Ensures that the MD5 hash will still match what was sent.
    if (modifyFlashMode) {
        _buffer[FLASH_MODE_OFFSET] = bufferFlashMode;
    }

    if (!writeResult) {
        _currentAddress = (_startAddress + _size);
        _setError(UPDATE_ERROR_WRITE);
        return false;
    }
    _md5.add(_buffer, _bufferLen);
    _currentAddress += _bufferLen;
    _bufferLen = 0;
    return true;
}

size_t BlinkerUpdaterClass::write(uint8_t *data, size_t len) {
    if(hasError() || !isRunning())
        return 0;

    if(len > remaining()){
        //len = remaining();
        //fail instead
        _setError(UPDATE_ERROR_SPACE);
        return 0;
    }

    size_t left = len;

    while((_bufferLen + left) > _bufferSize) {
        size_t toBuff = _bufferSize - _bufferLen;
        memcpy(_buffer + _bufferLen, data + (len - left), toBuff);
        _bufferLen += toBuff;
        if(!_writeBuffer()){
        return len - left;
        }
        left -= toBuff;
        if(!_async) yield();
    }
    //lets see whats left
    memcpy(_buffer + _bufferLen, data + (len - left), left);
    _bufferLen += left;
    if(_bufferLen == remaining()){
        //we are at the end of the update, so should write what's left to flash
        if(!_writeBuffer()){
        return len - left;
        }
    }
    return len;
}

bool BlinkerUpdaterClass::_verifyHeader(uint8_t data) {
    if(_command == U_FLASH) {
        // check for valid first magic byte (is always 0xE9)
        if(data != 0xE9) {
            _currentAddress = (_startAddress + _size);
            _setError(UPDATE_ERROR_MAGIC_BYTE);
            return false;
        }
        return true;
    } else if(_command == U_SPIFFS) {
        // no check of SPIFFS possible with first byte.
        return true;
    }
    return false;
}

bool BlinkerUpdaterClass::_verifyEnd() {
    if(_command == U_FLASH) {

        uint8_t buf[4];
        if(!ESP.flashRead(_startAddress, (uint32_t *) &buf[0], 4)) {
            _currentAddress = (_startAddress);
            _setError(UPDATE_ERROR_READ);
            return false;
        }

        // check for valid first magic byte
        if(buf[0] != 0xE9) {
            _currentAddress = (_startAddress);
            _setError(UPDATE_ERROR_MAGIC_BYTE);
            return false;
        }

        uint32_t bin_flash_size = ESP.magicFlashChipSize((buf[3] & 0xf0) >> 4);

        // check if new bin fits to SPI flash
        if(bin_flash_size > ESP.getFlashChipRealSize()) {
            _currentAddress = (_startAddress);
            _setError(UPDATE_ERROR_NEW_FLASH_CONFIG);
            return false;
        }

        return true;
    } else if(_command == U_SPIFFS) {
        // SPIFFS is already over written checks make no sense any more.
        return true;
    }
    return false;
}

size_t BlinkerUpdaterClass::writeStream(Stream &data) {
    size_t written = 0;
    size_t toRead = 0;
    if(hasError() || !isRunning())
        return 0;

    if(!_verifyHeader(data.peek())) {
// #ifdef DEBUG_UPDATER
//         printError(DEBUG_UPDATER);
// #endif
        BLINKER_LOG(printError());
        _reset();
        return 0;
    }

    // if(_ledPin != -1) {
    //     pinMode(_ledPin, OUTPUT);
    // }
    
    if (_progress_callback) {
        _progress_callback(0, _size);
    }
    while(remaining()) {
        // if(_ledPin != -1) {
        //     digitalWrite(_ledPin, _ledOn); // Switch LED on
        // }
        size_t bytesToRead = _bufferSize - _bufferLen;
        if(bytesToRead > remaining()) {
            bytesToRead = remaining();
        }
        toRead = data.readBytes(_buffer + _bufferLen,  bytesToRead);
        if(toRead == 0) { //Timeout
            delay(100);
            toRead = data.readBytes(_buffer + _bufferLen, bytesToRead);
            if(toRead == 0) { //Timeout
                _currentAddress = (_startAddress + _size);
                _setError(UPDATE_ERROR_STREAM);
                _reset();
                return written;
            }
        }
        // if(_ledPin != -1) {
        //     digitalWrite(_ledPin, !_ledOn); // Switch LED off
        // }
        _bufferLen += toRead;
        if((_bufferLen == remaining() || _bufferLen == _bufferSize) && !_writeBuffer())
            return written;
        written += toRead;

        if(_progress_callback) {
            _progress_callback(progress(), _size);
        }
        yield();
    }
    if(_progress_callback) {
        _progress_callback(_size, _size);
    }
    return written;
}

void BlinkerUpdaterClass::_setError(int error){
    _error = error;
    // #ifdef DEBUG_UPDATER
    // printError(DEBUG_UPDATER);
    // #endif
    BLINKER_LOG(printError());
}

String BlinkerUpdaterClass::printError(){
    String errData = BLINKER_F("ERROR: ");
    errData += String(_error);
    errData += BLINKER_F(" ");
    // out.printf_P(PSTR("ERROR[%u]: "), _error);
    if(_error == UPDATE_ERROR_OK){
        // out.println(F("No Error"));
        errData += F("No Error");
    } else if(_error == UPDATE_ERROR_WRITE){
        // out.println(F("Flash Write Failed"));
        errData += F("Flash Write Failed");
    } else if(_error == UPDATE_ERROR_ERASE){
        // out.println(F("Flash Erase Failed"));
        errData += F("Flash Erase Failed");
    } else if(_error == UPDATE_ERROR_READ){
        // out.println(F("Flash Read Failed"));
        errData += F("Flash Read Failed");
    } else if(_error == UPDATE_ERROR_SPACE){
        // out.println(F("Not Enough Space"));
        errData += F("Not Enough Space");
    } else if(_error == UPDATE_ERROR_SIZE){
        // out.println(F("Bad Size Given"));
        errData += F("Bad Size Given");
    } else if(_error == UPDATE_ERROR_STREAM){
        // out.println(F("Stream Read Timeout"));
        errData += F("Stream Read Timeout");
    } else if(_error == UPDATE_ERROR_MD5){
        //out.println(F("MD5 Check Failed"));
        // out.printf("MD5 Failed: expected:%s, calculated:%s\n", _target_md5.c_str(), _md5.toString().c_str());
        errData += F("MD5 Check Failed ");
        errData += F("MD5 Failed: expected: ");
        errData += _target_md5;
        errData += F(" , calculated: ");
        errData += _md5.toString();
    } else if(_error == UPDATE_ERROR_FLASH_CONFIG){
        // out.printf_P(PSTR("Flash config wrong real: %d IDE: %d\n"), ESP.getFlashChipRealSize(), ESP.getFlashChipSize());
        errData += F("Flash config wrong real: ");
        errData += String(ESP.getFlashChipRealSize());
        errData += F(" IDE: ");
        errData += String(ESP.getFlashChipSize());
    } else if(_error == UPDATE_ERROR_NEW_FLASH_CONFIG){
        // out.printf_P(PSTR("new Flash config wrong real: %d\n"), ESP.getFlashChipRealSize());
        errData += F("new Flash config wrong real: ");
        errData += String(ESP.getFlashChipRealSize());
    } else if(_error == UPDATE_ERROR_MAGIC_BYTE){
        // out.println(F("Magic byte is wrong, not 0xE9"));
        errData += F("Magic byte is wrong, not 0xE9");
    } else if (_error == UPDATE_ERROR_BOOTSTRAP){
        // out.println(F("Invalid bootstrapping state, reset ESP8266 before updating"));
        errData += F("Invalid bootstrapping state, reset ESP8266 before updating");
    } else {
        // out.println(F("UNKNOWN"));
        errData += F("UNKNOWN");
    }
    return errData;
}

BlinkerUpdaterClass BlinkerUpdater;

#elif defined(ESP32)

#include "Update.h"
#include "Arduino.h"
#include "esp_spi_flash.h"
#include "esp_ota_ops.h"
#include "esp_image_format.h"

static const char * _err2str(uint8_t _error){
    if(_error == UPDATE_ERROR_OK){
        return ("No Error");
    } else if(_error == UPDATE_ERROR_WRITE){
        return ("Flash Write Failed");
    } else if(_error == UPDATE_ERROR_ERASE){
        return ("Flash Erase Failed");
    } else if(_error == UPDATE_ERROR_READ){
        return ("Flash Read Failed");
    } else if(_error == UPDATE_ERROR_SPACE){
        return ("Not Enough Space");
    } else if(_error == UPDATE_ERROR_SIZE){
        return ("Bad Size Given");
    } else if(_error == UPDATE_ERROR_STREAM){
        return ("Stream Read Timeout");
    } else if(_error == UPDATE_ERROR_MD5){
        return ("MD5 Check Failed");
    } else if(_error == UPDATE_ERROR_MAGIC_BYTE){
        return ("Wrong Magic Byte");
    } else if(_error == UPDATE_ERROR_ACTIVATE){
        return ("Could Not Activate The Firmware");
    } else if(_error == UPDATE_ERROR_NO_PARTITION){
        return ("Partition Could Not be Found");
    } else if(_error == UPDATE_ERROR_BAD_ARGUMENT){
        return ("Bad Argument");
    } else if(_error == UPDATE_ERROR_ABORT){
        return ("Aborted");
    }
    return ("UNKNOWN");
}

static bool _partitionIsBootable(const esp_partition_t* partition){
    uint8_t buf[4];
    if(!partition){
        return false;
    }
    if(!ESP.flashRead(partition->address, (uint32_t*)buf, 4)) {
        return false;
    }

    if(buf[0] != ESP_IMAGE_HEADER_MAGIC) {
        return false;
    }
    return true;
}

static bool _enablePartition(const esp_partition_t* partition){
    uint8_t buf[4];
    if(!partition){
        return false;
    }
    if(!ESP.flashRead(partition->address, (uint32_t*)buf, 4)) {
        return false;
    }
    buf[0] = ESP_IMAGE_HEADER_MAGIC;

    return ESP.flashWrite(partition->address, (uint32_t*)buf, 4);
}

BlinkerUpdaterClass::BlinkerUpdaterClass()
: _error(0)
, _buffer(0)
, _bufferLen(0)
, _size(0)
, _progress_callback(NULL)
, _progress(0)
, _command(U_FLASH)
, _partition(NULL)
{
}

BlinkerUpdaterClass& BlinkerUpdaterClass::onProgress(THandlerFunction_Progress fn) {
    _progress_callback = fn;
    return *this;
}

void BlinkerUpdaterClass::_reset() {
    if (_buffer)
        delete[] _buffer;
    _buffer = 0;
    _bufferLen = 0;
    _progress = 0;
    _size = 0;
    _command = U_FLASH;
}

bool BlinkerUpdaterClass::canRollBack(){
    if(_buffer){ //Update is running
        return false;
    }
    const esp_partition_t* partition = esp_ota_get_next_update_partition(NULL);
    return _partitionIsBootable(partition);
}

bool BlinkerUpdaterClass::rollBack(){
    if(_buffer){ //Update is running
        return false;
    }
    const esp_partition_t* partition = esp_ota_get_next_update_partition(NULL);
    return _partitionIsBootable(partition) && !esp_ota_set_boot_partition(partition);
}

bool BlinkerUpdaterClass::begin(size_t size, int command) {
    if(_size > 0){
        // log_w("already running");
        BLINKER_LOG_ALL(F("already running"));
        return false;
    }

    _reset();
    _error = 0;

    if(size == 0) {
        _error = UPDATE_ERROR_SIZE;
        return false;
    }

    if (command == U_FLASH) {
        _partition = esp_ota_get_next_update_partition(NULL);
        if(!_partition){
            _error = UPDATE_ERROR_NO_PARTITION;
            return false;
        }
        // log_d("OTA Partition: %s", _partition->label);
        BLINKER_LOG_ALL(F("OTA Partition: "), _partition->label);
    }
    else if (command == U_SPIFFS) {
        _partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, NULL);
        if(!_partition){
            _error = UPDATE_ERROR_NO_PARTITION;
            return false;
        }
    }
    else {
        _error = UPDATE_ERROR_BAD_ARGUMENT;
        // log_e("bad command %u", command);
        BLINKER_LOG_ALL(F("bad command "), command);
        return false;
    }

    if(size == UPDATE_SIZE_UNKNOWN){
        size = _partition->size;
    } else if(size > _partition->size){
        _error = UPDATE_ERROR_SIZE;
        // log_e("too large %u > %u", size, _partition->size);
        BLINKER_LOG_ALL(F("too large "), size, F(" > "), _partition->size);
        return false;
    }

    //initialize
    _buffer = (uint8_t*)malloc(SPI_FLASH_SEC_SIZE);
    if(!_buffer){
        // log_e("malloc failed");
        BLINKER_LOG_ALL(F("malloc failed"));
        return false;
    }
    _size = size;
    _command = command;
    _md5.begin();
    return true;
}

void BlinkerUpdaterClass::_abort(uint8_t err){
    _reset();
    _error = err;
}

void BlinkerUpdaterClass::abort(){
    _abort(UPDATE_ERROR_ABORT);
}

bool BlinkerUpdaterClass::_writeBuffer(){
    //first bytes of new firmware
    if(!_progress && _command == U_FLASH){
        //check magic
        if(_buffer[0] != ESP_IMAGE_HEADER_MAGIC){
            _abort(UPDATE_ERROR_MAGIC_BYTE);
            return false;
        }
        //remove magic byte from the firmware now and write it upon success
        //this ensures that partially written firmware will not be bootable
        _buffer[0] = 0xFF;
    }
    if(!ESP.flashEraseSector((_partition->address + _progress)/SPI_FLASH_SEC_SIZE)){
        _abort(UPDATE_ERROR_ERASE);
        return false;
    }
    if (!ESP.flashWrite(_partition->address + _progress, (uint32_t*)_buffer, _bufferLen)) {
        _abort(UPDATE_ERROR_WRITE);
        return false;
    }
    //restore magic or md5 will fail
    if(!_progress && _command == U_FLASH){
        _buffer[0] = ESP_IMAGE_HEADER_MAGIC;
    }
    _md5.add(_buffer, _bufferLen);
    _progress += _bufferLen;
    _bufferLen = 0;
    return true;
}

bool BlinkerUpdaterClass::_verifyHeader(uint8_t data) {
    if(_command == U_FLASH) {
        if(data != ESP_IMAGE_HEADER_MAGIC) {
            _abort(UPDATE_ERROR_MAGIC_BYTE);
            return false;
        }
        return true;
    } else if(_command == U_SPIFFS) {
        return true;
    }
    return false;
}

bool BlinkerUpdaterClass::_verifyEnd() {
    if(_command == U_FLASH) {
        if(!_enablePartition(_partition) || !_partitionIsBootable(_partition)) {
            _abort(UPDATE_ERROR_READ);
            return false;
        }

        if(esp_ota_set_boot_partition(_partition)){
            _abort(UPDATE_ERROR_ACTIVATE);
            return false;
        }
        _reset();
        return true;
    } else if(_command == U_SPIFFS) {
        _reset();
        return true;
    }
    return false;
}

bool BlinkerUpdaterClass::setMD5(const char * expected_md5){
    if(strlen(expected_md5) != 32)
    {
        return false;
    }
    _target_md5 = expected_md5;
    return true;
}

bool BlinkerUpdaterClass::end(bool evenIfRemaining){
    if(hasError() || _size == 0){
        return false;
    }

    if(!isFinished() && !evenIfRemaining){
        // log_e("premature end: res:%u, pos:%u/%u\n", getError(), progress(), _size);
        BLINKER_LOG_ALL(F("premature end: res: "), getError(), \
                        F(", pos:"), progress(), _size);
        _abort(UPDATE_ERROR_ABORT);
        return false;
    }

    if(evenIfRemaining) {
        if(_bufferLen > 0) {
            _writeBuffer();
        }
        _size = progress();
    }

    _md5.calculate();
    if(_target_md5.length()) {
        if(_target_md5 != _md5.toString()){
            _abort(UPDATE_ERROR_MD5);
            return false;
        }
    }

    return _verifyEnd();
}

size_t BlinkerUpdaterClass::write(uint8_t *data, size_t len) {
    if(hasError() || !isRunning()){
        return 0;
    }

    if(len > remaining()){
        _abort(UPDATE_ERROR_SPACE);
        return 0;
    }

    size_t left = len;

    while((_bufferLen + left) > SPI_FLASH_SEC_SIZE) {
        size_t toBuff = SPI_FLASH_SEC_SIZE - _bufferLen;
        memcpy(_buffer + _bufferLen, data + (len - left), toBuff);
        _bufferLen += toBuff;
        if(!_writeBuffer()){
            return len - left;
        }
        left -= toBuff;
    }
    memcpy(_buffer + _bufferLen, data + (len - left), left);
    _bufferLen += left;
    if(_bufferLen == remaining()){
        if(!_writeBuffer()){
            return len - left;
        }
    }
    return len;
}

size_t BlinkerUpdaterClass::writeStream(Stream &data) {
    size_t written = 0;
    size_t toRead = 0;
    if(hasError() || !isRunning())
        return 0;

    if(!_verifyHeader(data.peek())) {
        _reset();
        return 0;
    }
    if (_progress_callback) {
        _progress_callback(0, _size);
    }
    while(remaining()) {
        toRead = data.readBytes(_buffer + _bufferLen,  (SPI_FLASH_SEC_SIZE - _bufferLen));
        if(toRead == 0) { //Timeout
            delay(100);
            toRead = data.readBytes(_buffer + _bufferLen, (SPI_FLASH_SEC_SIZE - _bufferLen));
            if(toRead == 0) { //Timeout
                _abort(UPDATE_ERROR_STREAM);
                return written;
            }
        }
        _bufferLen += toRead;
        if((_bufferLen == remaining() || _bufferLen == SPI_FLASH_SEC_SIZE) && !_writeBuffer())
            return written;
        written += toRead;
        if(_progress_callback) {
            _progress_callback(_progress, _size);
        }
    }
    if(_progress_callback) {
        _progress_callback(_size, _size);
    }
    return written;
}

void BlinkerUpdaterClass::printError(Stream &out){
    out.println(_err2str(_error));
}

BlinkerUpdaterClass BlinkerUpdater;

#endif

#endif
