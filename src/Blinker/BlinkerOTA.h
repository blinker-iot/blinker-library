#ifndef BLINKER_OTA_H
#define BLINKER_OTA_H

#if (defined(ESP8266) || defined(ESP32))

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerDebug.h"
#include <EEPROM.h>
#if defined(ESP8266)
    #include <ESP8266HTTPClient.h>
    #include <ESP8266httpUpdate.h>

    #ifndef BLINKER_WITHOUT_SSL
    extern BearSSL::WiFiClientSecure client_mqtt;
    #endif
#elif defined(ESP32)
    #include <WiFi.h>
    #include <Update.h>

    // extern WiFiClient client_s;
    extern WiFiClientSecure client_s;
#endif

#include "BlinkerUpdater.h"

// extern WiFiClientSecure client_s;

enum bota_status_t{
    BLINKER_UPGRADE_DISABLE,
    BLINKER_UPGRADE_BEGIN,
    BLINKER_UPGRADE_START,
    BLINKER_UPGRADE_FAIL,
    BLINKER_UPGRADE_LOAD_FAIL,
    BLINKER_UPGRADE_VERI_FAIL,
    BLINKER_UPGRADE_SUCCESS
};
// #define	VERSIONPARAM					"1.0.1"

// if (loadOTACheck()) {
// 	if (!loadVersion()) {
// 		saveVersion();
// 		UPGRADE_STATUS = UPGRADE_SUCCESS;
// 	}
// 	else {
// 		UPGRADE_STATUS = UPGRADE_FAIL;
// 	}
// }

class BlinkerOTA
{
    public :
        BlinkerOTA() {}
        // void parseData(char *OTAdata);
        void host(String _host) { ota_host = _host; }
        void url(String _url) { ota_url = _url; }
        void setURL(String urlstr);
        void config(String _host, String _url, String _fingerPrint, String _md5)
        {
            // #if defined(ESP8266)
                BLINKER_LOG_ALL(_host.indexOf("https"));
                if (_host.indexOf("https") != -1)
                {
                    ota_host = _host.substring(8);
                }
                else
                {
                    ota_host = _host;
                }                
            // #else
            // ota_host = _host;
            // #endif
            ota_url = _url;
            ota_fingerPrint = _fingerPrint;
            ota_md5 = _md5;
        }
        bool update();
        // bool run();
        bota_status_t status() {
            return _status;
        }
        uint8_t loadOTACheck();
        void saveOTARun();
        void saveOTACheck();
        void clearOTACheck();
        bool loadVersion();
        void saveVersion();

    private :
        String getHeaderValue(String header, String headerName) {
            return header.substring(strlen(headerName.c_str()));
        }

    protected :
        // #if defined(ESP32)
        //     uint8_t OTACheck;
        // #endif
        String ota_host;
        String ota_url;
        String ota_fingerPrint;
        String ota_md5;
        uint16_t ota_port = 443;
        char *otaUrl;
        bota_status_t _status;
        // int
        size_t contentLength = 0;
        bool isValidContentType = false;
};

void BlinkerOTA::setURL(String url) {
    otaUrl = (char*)malloc(url.length()*sizeof(char));
    strcpy(otaUrl,url.c_str());

    BLINKER_LOG_ALL(BLINKER_F("ota url: "), otaUrl);

    _status = BLINKER_UPGRADE_START;

    update();
}

bool BlinkerOTA::update() {
    saveOTACheck();

#if defined(ESP8266)
    // #ifndef BLINKER_WITHOUT_SSL
    BearSSL::WiFiClientSecure client_s;

    BLINKER_LOG_FreeHeap();
    #ifndef BLINKER_WITHOUT_SSL
        client_mqtt.stop();
    #endif
    ::delay(100);

    bool mfln = client_s.probeMaxFragmentLength(ota_host, ota_port, 1024);
    if (mfln) {
        client_s.setBufferSizes(1024, 1024);
    }

    // client_s.setFingerprint(ota_fingerPrint.c_str());

    BLINKER_LOG_FreeHeap();

    client_s.setInsecure();

    // BLINKER_LOG_ALL(BLINKER_F("Connecting to: "), ota_host);

    // t_httpUpdate_return ret = BlinkerhttpUpdate.update(client_s, ota_host + ota_url, ota_md5, "");

    // switch (ret) {
    //     case HTTP_UPDATE_FAILED:
    //         // USE_SERIAL.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
    //         // break;
    //         BLINKER_LOG_ALL(BLINKER_F("HTTP_UPDATE_FAILD Error : "), BlinkerhttpUpdate.getLastError());
    //         return false;

    //     case HTTP_UPDATE_NO_UPDATES:
    //         // USE_SERIAL.println("HTTP_UPDATE_NO_UPDATES");
    //         // break;
    //         BLINKER_LOG_ALL(BLINKER_F("HTTP_UPDATE_NO_UPDATES"));
    //         return false;

    //     case HTTP_UPDATE_OK:
    //         // USE_SERIAL.println("HTTP_UPDATE_OK");
    //         // break;
    //         BLINKER_LOG_ALL(BLINKER_F("HTTP_UPDATE_OK"));
    //         return true;
    // }

    // #else
    //     WiFiClient               client_s;
    // #endif

#elif defined(ESP32)
    client_s.stop();
#endif

    BlinkerUpdater.setMD5(ota_md5.c_str());

    while (1) {
        if (!client_s.connect(ota_host.c_str(), ota_port)) {
            BLINKER_ERR_LOG(BLINKER_F("server connection failed"));
            BLINKER_LOG_FreeHeap();
            ::delay(1000);
        }
        else {
            BLINKER_LOG_ALL(BLINKER_F("connection succeed"));
            BLINKER_LOG_FreeHeap();
            BLINKER_LOG_ALL(BLINKER_F("Fetching Bin: "), ota_url);

            String _ota_url = BLINKER_F("GET ");
            _ota_url += ota_url;
            _ota_url += BLINKER_F(" HTTP/1.1\r\nHost: ");
            _ota_url += ota_host;
            _ota_url += BLINKER_F("\r\nCache-Control: no-cache\r\nConnection: close\r\n\r\n");

            // client_s.print(String("GET ") + ota_url + " HTTP/1.1\r\n" +
            //             "Host: " + ota_host + "\r\n" +
            //             "Cache-Control: no-cache\r\n" +
            //             "Connection: close\r\n\r\n");

            client_s.print(_ota_url);

            BLINKER_LOG_ALL(BLINKER_F("_ota_url: "), _ota_url);

            unsigned long timeout = millis();
            while (client_s.available() == 0) {
                if (millis() - timeout > 5000) {
                    BLINKER_LOG_ALL(BLINKER_F("Client Timeout !"));
                    client_s.stop();

                    _status = BLINKER_UPGRADE_LOAD_FAIL;
                    return false;
                }
            }
            // Once the response is available,
            // check stuff

            while (client_s.available()) {
                // read line till /n
                String line = client_s.readStringUntil('\n');
                // remove space, to check if the line is end of headers
                line.trim();

                // if the the line is empty,
                // this is end of headers
                // break the while and feed the
                // remaining `client_s` to the
                // Update.writeStream();
                if (!line.length()) {
                    //headers ended
                    break; // and get the OTA started
                }

                BLINKER_LOG_ALL(BLINKER_F("line: "), line);

                // Check if the HTTP Response is 200
                // else break and Exit Update
                if (line.startsWith("HTTP/1.1")) {
                    if (line.indexOf("200") < 0) {
                        BLINKER_LOG_ALL(BLINKER_F("Got a non 200 status code from server. Exiting OTA Update."));
                        break;
                    }
                }

                // extract headers here
                // Start with content length
                if (line.startsWith("Content-Length: ")) {
                    contentLength = atoi((getHeaderValue(line, "Content-Length: ")).c_str());
                    BLINKER_LOG_ALL(BLINKER_F("Got "), contentLength, BLINKER_F(" bytes from server"));
                }

                // Next, the content type
                if (line.startsWith("Content-Type: ")) {
                    String contentType = getHeaderValue(line, "Content-Type: ");
                    BLINKER_LOG_ALL(BLINKER_F("Got "), contentType, BLINKER_F(" payload."));
                    if (contentType == "application/octet-stream") {
                        isValidContentType = true;
                    }
                }
            }

            // Check what is the contentLength and if content type is `application/octet-stream`
            BLINKER_LOG_ALL(BLINKER_F("contentLength : "),
                            contentLength,
                            BLINKER_F(", isValidContentType : "),
                            isValidContentType);

            // check contentLength and content type
            if (contentLength && isValidContentType)
            {
                // Check if there is enough to OTA Update
                bool canBegin = BlinkerUpdater.begin(contentLength);

                // If yes, begin
                if (canBegin) {
                    BLINKER_LOG(BLINKER_F("Begin OTA. This may take 2 - 5 mins to complete. Things might be quite for a while.. Patience!"));
                    // No activity would appear on the Serial monitor
                    // So be patient. This may take 2 - 5mins to complete
                    size_t written = BlinkerUpdater.writeStream(client_s);

                    if (written == contentLength) {
                        BLINKER_LOG(BLINKER_F("Written : "), written, BLINKER_F(" successfully"));
                    }
                    else {
                        BLINKER_LOG(BLINKER_F("Written only : "), written,
                                        BLINKER_F("/"), contentLength, BLINKER_F(". Retry?"));
                    }

                    if (BlinkerUpdater.end()) {
                        BLINKER_LOG(BLINKER_F("OTA done!"));
                        if (BlinkerUpdater.isFinished()) {
                            BLINKER_LOG(BLINKER_F("Update successfully completed. Rebooting."));
                            _status = BLINKER_UPGRADE_SUCCESS;
                            // ESP.restart();

                            return true;
                        } else {
                            BLINKER_LOG(BLINKER_F("Update not finished? Something went wrong!"));
                            _status = BLINKER_UPGRADE_FAIL;

                            return false;
                        }
                    }
                    else {
                        BLINKER_LOG(BLINKER_F("Error Occurred. Error #: "), BlinkerUpdater.getError());
                        _status = BLINKER_UPGRADE_FAIL;

                        // return false;
                    }
                }
                else {
                    // not enough space to begin OTA
                    // Understand the partitions and
                    // space availability
                    BLINKER_LOG(BLINKER_F("Not enough space to begin OTA"));
                    client_s.flush();
                    _status = BLINKER_UPGRADE_FAIL;

                    return false;
                }
            }
            else {
                BLINKER_LOG(BLINKER_F("There was no content in the response"));
                client_s.flush();
                _status = BLINKER_UPGRADE_FAIL;

                return false;
            }
        }
    }
// #endif
}

// bool BlinkerOTA::run()
// {
//     while (1) {
//         if (!client_s.connect(ota_host.c_str(), ota_port)) {
//             BLINKER_ERR_LOG(BLINKER_F("server connection failed"));
//             BLINKER_LOG_FreeHeap();
//             ::delay(1000);
//         }
//         else {
//             BLINKER_LOG_ALL(BLINKER_F("connection succeed"));
//             BLINKER_LOG_FreeHeap();
//             BLINKER_LOG_ALL(BLINKER_F("Fetching Bin: "), ota_url);

//             client_s.print(String("GET ") + ota_url + " HTTP/1.1\r\n" +
//                         "Host: " + ota_host + "\r\n" +
//                         "Cache-Control: no-cache\r\n" +
//                         "Connection: close\r\n\r\n");

//             unsigned long timeout = millis();
//             while (client_s.available() == 0) {
//                 if (millis() - timeout > 5000) {
//                     BLINKER_LOG_ALL(BLINKER_F("Client Timeout !"));
//                     client_s.stop();

//                     _status = BLINKER_UPGRADE_LOAD_FAIL;
//                     return false;
//                 }
//             }
//             // Once the response is available,
//             // check stuff

//             while (client_s.available()) {
//                 // read line till /n
//                 String line = client_s.readStringUntil('\n');
//                 // remove space, to check if the line is end of headers
//                 line.trim();

//                 // if the the line is empty,
//                 // this is end of headers
//                 // break the while and feed the
//                 // remaining `client_s` to the
//                 // Update.writeStream();
//                 if (!line.length()) {
//                     //headers ended
//                     break; // and get the OTA started
//                 }

//                 // Check if the HTTP Response is 200
//                 // else break and Exit Update
//                 if (line.startsWith("HTTP/1.1")) {
//                     if (line.indexOf("200") < 0) {
//                         BLINKER_LOG_ALL(BLINKER_F("Got a non 200 status code from server. Exiting OTA Update."));
//                         break;
//                     }
//                 }

//                 // extract headers here
//                 // Start with content length
//                 if (line.startsWith("Content-Length: ")) {
//                     contentLength = atoi((getHeaderValue(line, "Content-Length: ")).c_str());
//                     BLINKER_LOG_ALL(BLINKER_F("Got "), contentLength, BLINKER_F(" bytes from server"));
//                 }

//                 // Next, the content type
//                 if (line.startsWith("Content-Type: ")) {
//                     String contentType = getHeaderValue(line, "Content-Type: ");
//                     BLINKER_LOG_ALL(BLINKER_F("Got "), contentType, BLINKER_F(" payload."));
//                     if (contentType == "application/octet-stream") {
//                         isValidContentType = true;
//                     }
//                 }
//             }

//             // Check what is the contentLength and if content type is `application/octet-stream`
//             BLINKER_LOG_ALL(BLINKER_F("contentLength : "),
//                             contentLength,
//                             BLINKER_F(", isValidContentType : "),
//                             isValidContentType);

//             // check contentLength and content type
//             if (contentLength && isValidContentType)
//             {
//                 // Check if there is enough to OTA Update
//                 bool canBegin = BlinkerUpdater.begin(contentLength);

//                 // If yes, begin
//                 if (canBegin) {
//                     BLINKER_LOG(BLINKER_F("Begin OTA. This may take 2 - 5 mins to complete. Things might be quite for a while.. Patience!"));
//                     // No activity would appear on the Serial monitor
//                     // So be patient. This may take 2 - 5mins to complete
//                     size_t written = BlinkerUpdater.writeStream(client_s);

//                     if (written == contentLength) {
//                         BLINKER_LOG(BLINKER_F("Written : "), written, BLINKER_F(" successfully"));
//                     }
//                     else {
//                         BLINKER_LOG(BLINKER_F("Written only : "), written,
//                                         BLINKER_F("/"), contentLength, BLINKER_F(". Retry?"));
//                     }

//                     if (BlinkerUpdater.end()) {
//                         BLINKER_LOG(BLINKER_F("OTA done!"));
//                         if (BlinkerUpdater.isFinished()) {
//                             BLINKER_LOG(BLINKER_F("Update successfully completed. Rebooting."));
//                             _status = BLINKER_UPGRADE_SUCCESS;
//                             // ESP.restart();

//                             return true;
//                         } else {
//                             BLINKER_LOG(BLINKER_F("Update not finished? Something went wrong!"));
//                             _status = BLINKER_UPGRADE_FAIL;

//                             return false;
//                         }
//                     }
//                     else {
//                         BLINKER_LOG(BLINKER_F("Error Occurred. Error #: "), BlinkerUpdater.getError());
//                         _status = BLINKER_UPGRADE_FAIL;

//                         // return false;
//                     }
//                 }
//                 else {
//                     // not enough space to begin OTA
//                     // Understand the partitions and
//                     // space availability
//                     BLINKER_LOG(BLINKER_F("Not enough space to begin OTA"));
//                     client_s.flush();
//                     _status = BLINKER_UPGRADE_FAIL;

//                     return false;
//                 }
//             }
//             else {
//                 BLINKER_LOG(BLINKER_F("There was no content in the response"));
//                 client_s.flush();
//                 _status = BLINKER_UPGRADE_FAIL;

//                 return false;
//             }
//         }
//     }
// }

uint8_t BlinkerOTA::loadOTACheck() {
    // #if defined(ESP8266)
    static uint8_t OTACheck;
    // #endif
    EEPROM.begin(BLINKER_EEP_SIZE);
    EEPROM.get(BLINKER_EEP_ADDR_OTA_CHECK, OTACheck);
    EEPROM.commit();
    EEPROM.end();

    BLINKER_LOG_ALL(BLINKER_F("OTA Check: "), OTACheck);
    // BLINKER_LOG_ALL(BLINKER_F("BLINKER_EEP_ADDR_OTA_CHECK: "), BLINKER_EEP_ADDR_OTA_CHECK);

    return OTACheck;

    // if (OTACheck != BLINKER_OTA_START) {
    //     BLINKER_LOG_ALL(BLINKER_F("OTA NOT START"));
    //     return false;
    // }
    // else {
    //     BLINKER_LOG_ALL(BLINKER_F("OTA START"));
    //     return true;
    // }
}

void BlinkerOTA::saveOTARun() {
    EEPROM.begin(BLINKER_EEP_SIZE);
    EEPROM.put(BLINKER_EEP_ADDR_OTA_CHECK, BLINKER_OTA_RUN);
    EEPROM.commit();
    EEPROM.end();

    BLINKER_LOG_ALL(BLINKER_F("OTA RUN: "), BLINKER_OTA_RUN);
}

void BlinkerOTA::saveOTACheck() {
    EEPROM.begin(BLINKER_EEP_SIZE);
    EEPROM.put(BLINKER_EEP_ADDR_OTA_CHECK, BLINKER_OTA_START);
    EEPROM.commit();
    EEPROM.end();

    BLINKER_LOG_ALL(BLINKER_F("OTA START: "), BLINKER_OTA_START);
}

void BlinkerOTA::clearOTACheck() {
    EEPROM.begin(BLINKER_EEP_SIZE);
    EEPROM.put(BLINKER_EEP_ADDR_OTA_CHECK, BLINKER_OTA_CLEAR);
    EEPROM.commit();
    EEPROM.end();

    BLINKER_LOG_ALL(BLINKER_F("OTA CLEAR: "), BLINKER_OTA_CLEAR);
    _status = BLINKER_UPGRADE_DISABLE;
}

bool BlinkerOTA::loadVersion() {
// #if defined(BLINKER_PRO)
//     uint32_t versionCheck;
//     EEPROM.begin(BLINKER_EEP_SIZE);
//     EEPROM.get(BLINKER_EEP_ADDR_OTA_INFO, versionCheck);//+BUNDLINGSIZE+isBundling
//     EEPROM.commit();
//     EEPROM.end();

//     BLINKER_LOG_ALL(BLINKER_F("loadVersion: "), versionCheck);

//     if (versionCheck != BLINKER_OTA_VERSION_CODE) {
//         BLINKER_LOG_ALL(BLINKER_F("OTA SUCCESS BLINKER_OTA_VERSION_CODE NOT UPGRADE"));
//         _status = BLINKER_UPGRADE_SUCCESS;
//         return false;
//     }
//     else {
//         BLINKER_LOG_ALL(BLINKER_F("OTA FAIL OR NOT START OTA"));
//         _status = BLINKER_UPGRADE_FAIL;
//         return true;
//     }
// #else
    char versionCheck[11];

    EEPROM.begin(BLINKER_EEP_SIZE);
    EEPROM.get(BLINKER_EEP_ADDR_OTA_INFO, versionCheck);//+BUNDLINGSIZE+isBundling
    EEPROM.commit();
    EEPROM.end();

    BLINKER_LOG_ALL(BLINKER_F("loadVersion: "), versionCheck);

    if (strcmp(versionCheck, BLINKER_OTA_VERSION_CODE)) {
        BLINKER_LOG_ALL(BLINKER_F("OTA SUCCESS BLINKER_OTA_VERSION_CODE NOT UPGRADE"));
        _status = BLINKER_UPGRADE_SUCCESS;
        return false;
    }
    else {
        BLINKER_LOG_ALL(BLINKER_F("OTA FAIL OR NOT START OTA"));
        _status = BLINKER_UPGRADE_FAIL;
        return true;
    }
// #endif
}

void BlinkerOTA::saveVersion() {
    EEPROM.begin(BLINKER_EEP_SIZE);
    EEPROM.put(BLINKER_EEP_ADDR_OTA_INFO, BLINKER_OTA_VERSION_CODE);//+BUNDLINGSIZE+isBundling
    EEPROM.commit();
    EEPROM.end();

    BLINKER_LOG_ALL(BLINKER_F("SAVE BLINKER_OTA_VERSION_CODE"));
}

#endif

#endif
