#ifndef BLINKER_UPDATER_H
#define BLINKER_UPDATER_H

#if defined(ESP8266) || defined(ESP32)

#if defined(ESP8266)

// #define HTTPUPDATE_1_2_COMPATIBLE

// #include <Arduino.h>
// #include <ESP8266WiFi.h>
// #include <WiFiClient.h>
// #include <WiFiUdp.h>
// #include <ESP8266HTTPClient.h>

// #ifdef DEBUG_ESP_HTTP_UPDATE
// #ifdef DEBUG_ESP_PORT
// #define DEBUG_HTTP_UPDATE(fmt, ...) DEBUG_ESP_PORT.printf_P( (PGM_P)PSTR(fmt), ## __VA_ARGS__ )
// #endif
// #endif

// #ifndef DEBUG_HTTP_UPDATE
// #define DEBUG_HTTP_UPDATE(...) do { (void)0; } while(0)
// #endif

// /// note we use HTTP client errors too so we start at 100
// #define HTTP_UE_TOO_LESS_SPACE              (-100)
// #define HTTP_UE_SERVER_NOT_REPORT_SIZE      (-101)
// #define HTTP_UE_SERVER_FILE_NOT_FOUND       (-102)
// #define HTTP_UE_SERVER_FORBIDDEN            (-103)
// #define HTTP_UE_SERVER_WRONG_HTTP_CODE      (-104)
// #define HTTP_UE_SERVER_FAULTY_MD5           (-105)
// #define HTTP_UE_BIN_VERIFY_HEADER_FAILED    (-106)
// #define HTTP_UE_BIN_FOR_WRONG_FLASH         (-107)

// enum BlinkerHTTPUpdateResult {
//     HTTP_UPDATE_FAILED,
//     HTTP_UPDATE_NO_UPDATES,
//     HTTP_UPDATE_OK
// };

// typedef BlinkerHTTPUpdateResult t_httpUpdate_return; // backward compatibility

// class BlinkerHTTPUpdate
// {
// public:
//     BlinkerHTTPUpdate(void);
//     BlinkerHTTPUpdate(int httpClientTimeout);
//     ~BlinkerHTTPUpdate(void);

//     void rebootOnUpdate(bool reboot)
//     {
//         _rebootOnUpdate = reboot;
//     }

//     void followRedirects(bool follow)
//     {
//         _followRedirects = follow;
//     }

//     void setLedPin(int ledPin = -1, uint8_t ledOn = HIGH)
//     {
//         _ledPin = ledPin;
//         _ledOn = ledOn;
//     }

// #ifdef HTTPUPDATE_1_2_COMPATIBLE
//     // This function is deprecated, use rebootOnUpdate and the next one instead
//     t_httpUpdate_return update(const String& url, const String& currentVersion,
//                                const String& httpsFingerprint, bool reboot) __attribute__((deprecated));
//     t_httpUpdate_return update(const String& url, const String& currentVersion = "") __attribute__((deprecated));
//     t_httpUpdate_return update(const String& url, const String& currentVersion,
//                                const String& httpsFingerprint) __attribute__((deprecated));
//     t_httpUpdate_return update(const String& url, const String& currentVersion,
//                                const uint8_t httpsFingerprint[20]) __attribute__((deprecated)); // BearSSL
// #endif
//     t_httpUpdate_return update(WiFiClient& client, const String& url, const String& currentVersion = "");
//     t_httpUpdate_return update(WiFiClient& client, const String& url, const String& md5, const String& currentVersion);

// #ifdef HTTPUPDATE_1_2_COMPATIBLE
//     // This function is deprecated, use one of the overloads below along with rebootOnUpdate
//     t_httpUpdate_return update(const String& host, uint16_t port, const String& uri, const String& currentVersion,
//                                bool https, const String& httpsFingerprint, bool reboot) __attribute__((deprecated));

//     t_httpUpdate_return update(const String& host, uint16_t port, const String& uri = "/",
//                                const String& currentVersion = "") __attribute__((deprecated));
//     t_httpUpdate_return update(const String& host, uint16_t port, const String& url,
//                                const String& currentVersion, const String& httpsFingerprint) __attribute__((deprecated));
//     t_httpUpdate_return update(const String& host, uint16_t port, const String& url,
//                                const String& currentVersion, const uint8_t httpsFingerprint[20]) __attribute__((deprecated)); // BearSSL
// #endif
//     t_httpUpdate_return update(WiFiClient& client, const String& host, uint16_t port, const String& uri = "/",
//                                const String& currentVersion = "");

// #ifdef HTTPUPDATE_1_2_COMPATIBLE
//     // This function is deprecated, use rebootOnUpdate and the next one instead
//     t_httpUpdate_return updateSpiffs(const String& url, const String& currentVersion,
//                                      const String& httpsFingerprint, bool reboot) __attribute__((deprecated));
//     t_httpUpdate_return updateSpiffs(const String& url, const String& currentVersion = "") __attribute__((deprecated));
//     t_httpUpdate_return updateSpiffs(const String& url, const String& currentVersion, const String& httpsFingerprint) __attribute__((deprecated));
//     t_httpUpdate_return updateSpiffs(const String& url, const String& currentVersion, const uint8_t httpsFingerprint[20]) __attribute__((deprecated)); // BearSSL
// #endif
//     t_httpUpdate_return updateSpiffs(WiFiClient& client, const String& url, const String& currentVersion = "");


//     int getLastError(void);
//     String getLastErrorString(void);

// protected:
//     t_httpUpdate_return handleUpdate(HTTPClient& http, const String& currentVersion, bool spiffs = false, const String& md5 = "");
//     bool runUpdate(Stream& in, uint32_t size, String md5, int command = U_FLASH);

//     int _lastError;
//     bool _rebootOnUpdate = true;
// private:
//     int _httpClientTimeout;
//     bool _followRedirects;

//     int _ledPin;
//     uint8_t _ledOn;
// };

// #if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_HTTPUPDATE)
// extern BlinkerHTTPUpdate BlinkerhttpUpdate;
// #endif

#include <Arduino.h>
#include <flash_utils.h>
#include <MD5Builder.h>

#define UPDATE_ERROR_OK                 (0)
#define UPDATE_ERROR_WRITE              (1)
#define UPDATE_ERROR_ERASE              (2)
#define UPDATE_ERROR_READ               (3)
#define UPDATE_ERROR_SPACE              (4)
#define UPDATE_ERROR_SIZE               (5)
#define UPDATE_ERROR_STREAM             (6)
#define UPDATE_ERROR_MD5                (7)
#define UPDATE_ERROR_FLASH_CONFIG       (8)
#define UPDATE_ERROR_NEW_FLASH_CONFIG   (9)
#define UPDATE_ERROR_MAGIC_BYTE         (10)
#define UPDATE_ERROR_BOOTSTRAP          (11)

#define U_FLASH   0
#define U_SPIFFS  100
#define U_AUTH    200

// #ifdef DEBUG_ESP_UPDATER
// #ifdef DEBUG_ESP_PORT
// #define DEBUG_UPDATER DEBUG_ESP_PORT
// #endif
// #endif

class BlinkerUpdaterClass {
  public:
    typedef std::function<void(unsigned int, unsigned int)> THandlerFunction_Progress;
    
    BlinkerUpdaterClass();

    /*
      This callback will be called when Update is receiving data
    */
    BlinkerUpdaterClass& onProgress(THandlerFunction_Progress fn);

    /*
      Call this to check the space needed for the update
      Will return false if there is not enough space
    */
    bool begin(size_t size, int command = U_FLASH, int ledPin = -1, uint8_t ledOn = LOW);

    /*
      Run Updater from asynchronous callbacs
    */
    void runAsync(bool async){ _async = async; }

    /*
      Writes a buffer to the flash and increments the address
      Returns the amount written
    */
    size_t write(uint8_t *data, size_t len);

    /*
      Writes the remaining bytes from the Stream to the flash
      Uses readBytes() and sets UPDATE_ERROR_STREAM on timeout
      Returns the bytes written
      Should be equal to the remaining bytes when called
      Usable for slow streams like Serial
    */
    size_t writeStream(Stream &data);

    /*
      If all bytes are written
      this call will write the config to eboot
      and return true
      If there is already an update running but is not finished and !evenIfRemaining
      or there is an error
      this will clear everything and return false
      the last error is available through getError()
      evenIfRemaining is helpful when you update without knowing the final size first
    */
    bool end(bool evenIfRemaining = false);

    /*
      Prints the last error to an output stream
    */
    String printError();

    /*
      sets the expected MD5 for the firmware (hexString)
    */
    bool setMD5(const char * expected_md5);

    /*
      returns the MD5 String of the sucessfully ended firmware
    */
    String md5String(void){ return _md5.toString(); }

    /*
      populated the result with the md5 bytes of the sucessfully ended firmware
    */
    void md5(uint8_t * result){ return _md5.getBytes(result); }

    //Helpers
    uint8_t getError(){ return _error; }
    void clearError(){ _error = UPDATE_ERROR_OK; }
    bool hasError(){ return _error != UPDATE_ERROR_OK; }
    bool isRunning(){ return _size > 0; }
    bool isFinished(){ return _currentAddress == (_startAddress + _size); }
    size_t size(){ return _size; }
    size_t progress(){ return _currentAddress - _startAddress; }
    size_t remaining(){ return _size - (_currentAddress - _startAddress); }

    /*
      Template to write from objects that expose
      available() and read(uint8_t*, size_t) methods
      faster than the writeStream method
      writes only what is available
    */
    template<typename T>
    size_t write(T &data){
      size_t written = 0;
      if (hasError() || !isRunning())
        return 0;

      size_t available = data.available();
      while(available) {
        if(_bufferLen + available > remaining()){
            available = remaining() - _bufferLen;
        }
        if(_bufferLen + available > _bufferSize) {
            size_t toBuff = _bufferSize - _bufferLen;
            data.read(_buffer + _bufferLen, toBuff);
            _bufferLen += toBuff;
            if(!_writeBuffer())
                return written;
            written += toBuff;
        } else {
            data.read(_buffer + _bufferLen, available);
            _bufferLen += available;
            written += available;
            if(_bufferLen == remaining()) {
                if(!_writeBuffer()) {
                    return written;
                }
            }
        }
        if(remaining() == 0)
            return written;
        delay(1);
        available = data.available();
        }
        return written;
    }

  private:
    void _reset();
    bool _writeBuffer();

    bool _verifyHeader(uint8_t data);
    bool _verifyEnd();

    void _setError(int error);    

    bool _async;
    uint8_t _error;
    uint8_t *_buffer;
    size_t _bufferLen; // amount of data written into _buffer
    size_t _bufferSize; // total size of _buffer
    size_t _size;
    THandlerFunction_Progress _progress_callback;
    uint32_t _startAddress;
    uint32_t _currentAddress;
    uint32_t _command;

    String _target_md5;
    MD5Builder _md5;

    // int _ledPin;
    // uint8_t _ledOn;
};

extern BlinkerUpdaterClass BlinkerUpdater;

#elif defined(ESP32)

#include <Arduino.h>
#include <MD5Builder.h>
#include <functional>
#include "esp_partition.h"

#define UPDATE_ERROR_OK                 (0)
#define UPDATE_ERROR_WRITE              (1)
#define UPDATE_ERROR_ERASE              (2)
#define UPDATE_ERROR_READ               (3)
#define UPDATE_ERROR_SPACE              (4)
#define UPDATE_ERROR_SIZE               (5)
#define UPDATE_ERROR_STREAM             (6)
#define UPDATE_ERROR_MD5                (7)
#define UPDATE_ERROR_MAGIC_BYTE         (8)
#define UPDATE_ERROR_ACTIVATE           (9)
#define UPDATE_ERROR_NO_PARTITION       (10)
#define UPDATE_ERROR_BAD_ARGUMENT       (11)
#define UPDATE_ERROR_ABORT              (12)

#define UPDATE_SIZE_UNKNOWN 0xFFFFFFFF

#define U_FLASH   0
#define U_SPIFFS  100
#define U_AUTH    200

class BlinkerUpdaterClass {
  public:
    typedef std::function<void(size_t, size_t)> THandlerFunction_Progress;

    BlinkerUpdaterClass();

    /*
      This callback will be called when Update is receiving data
    */
    BlinkerUpdaterClass& onProgress(THandlerFunction_Progress fn);

    /*
      Call this to check the space needed for the update
      Will return false if there is not enough space
    */
    bool begin(size_t size=UPDATE_SIZE_UNKNOWN, int command = U_FLASH);

    /*
      Writes a buffer to the flash and increments the address
      Returns the amount written
    */
    size_t write(uint8_t *data, size_t len);

    /*
      Writes the remaining bytes from the Stream to the flash
      Uses readBytes() and sets UPDATE_ERROR_STREAM on timeout
      Returns the bytes written
      Should be equal to the remaining bytes when called
      Usable for slow streams like Serial
    */
    size_t writeStream(Stream &data);

    /*
      If all bytes are written
      this call will write the config to eboot
      and return true
      If there is already an update running but is not finished and !evenIfRemainanig
      or there is an error
      this will clear everything and return false
      the last error is available through getError()
      evenIfRemaining is helpfull when you update without knowing the final size first
    */
    bool end(bool evenIfRemaining = false);

    /*
      Aborts the running update
    */
    void abort();

    /*
      Prints the last error to an output stream
    */
    void printError(Stream &out);

    /*
      sets the expected MD5 for the firmware (hexString)
    */
    bool setMD5(const char * expected_md5);

    /*
      returns the MD5 String of the sucessfully ended firmware
    */
    String md5String(void){ return _md5.toString(); }

    /*
      populated the result with the md5 bytes of the sucessfully ended firmware
    */
    void md5(uint8_t * result){ return _md5.getBytes(result); }

    //Helpers
    uint8_t getError(){ return _error; }
    void clearError(){ _error = UPDATE_ERROR_OK; }
    bool hasError(){ return _error != UPDATE_ERROR_OK; }
    bool isRunning(){ return _size > 0; }
    bool isFinished(){ return _progress == _size; }
    size_t size(){ return _size; }
    size_t progress(){ return _progress; }
    size_t remaining(){ return _size - _progress; }

    /*
      Template to write from objects that expose
      available() and read(uint8_t*, size_t) methods
      faster than the writeStream method
      writes only what is available
    */
    template<typename T>
    size_t write(T &data){
      size_t written = 0;
      if (hasError() || !isRunning())
        return 0;

      size_t available = data.available();
      while(available) {
        if(_bufferLen + available > remaining()){
          available = remaining() - _bufferLen;
        }
        if(_bufferLen + available > 4096) {
          size_t toBuff = 4096 - _bufferLen;
          data.read(_buffer + _bufferLen, toBuff);
          _bufferLen += toBuff;
          if(!_writeBuffer())
            return written;
          written += toBuff;
        } else {
          data.read(_buffer + _bufferLen, available);
          _bufferLen += available;
          written += available;
          if(_bufferLen == remaining()) {
            if(!_writeBuffer()) {
              return written;
            }
          }
        }
        if(remaining() == 0)
          return written;
        available = data.available();
      }
      return written;
    }

    /*
      check if there is a firmware on the other OTA partition that you can bootinto
    */
    bool canRollBack();
    /*
      set the other OTA partition as bootable (reboot to enable)
    */
    bool rollBack();

  private:
    void _reset();
    void _abort(uint8_t err);
    bool _writeBuffer();
    bool _verifyHeader(uint8_t data);
    bool _verifyEnd();


    uint8_t _error;
    uint8_t *_buffer;
    size_t _bufferLen;
    size_t _size;
    THandlerFunction_Progress _progress_callback;
    uint32_t _progress;
    uint32_t _command;
    const esp_partition_t* _partition;

    String _target_md5;
    MD5Builder _md5;
};

extern BlinkerUpdaterClass BlinkerUpdater;

#endif

#endif

#endif
