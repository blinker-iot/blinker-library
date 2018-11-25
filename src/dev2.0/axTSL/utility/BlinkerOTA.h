#ifndef BLINKER_OTA_H
#define BLINKER_OTA_H

// #if defined(BLINKER_PRO)

#include "Blinker/BlinkerConfig.h"
#include <EEPROM.h>
#if defined(ESP8266)
    #include <ESP8266HTTPClient.h>
    #include <ESP8266httpUpdate.h>
#elif defined(ESP32)
    #include <WiFi.h>
    #include <Update.h>

    // extern WiFiClient client_s;
#endif

extern WiFiClientSecure client_s;

enum bota_status_t{
    BLINKER_UPGRADE_DISABLE,
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
        void config(String _host, String _url, String _fingerPrint)
        {
            ota_host = _host;
            ota_url = _url;
            ota_fingerPrint = _fingerPrint;
        }
        void update();
        bota_status_t status() {
            return _status;
        }
        bool loadOTACheck();
        void saveOTACheck();
        void clearOTACheck();
        bool loadVersion();
        void saveVersion();

    private :
        String getHeaderValue(String header, String headerName) {
            return header.substring(strlen(headerName.c_str()));
        }

    protected :
        String ota_host;
        String ota_url;
        String ota_fingerPrint;
        uint16_t ota_port = 443;
        char *otaUrl;
        bota_status_t _status;
        int contentLength = 0;
        bool isValidContentType = false;
};

void BlinkerOTA::setURL(String url) {
    otaUrl = (char*)malloc(url.length()*sizeof(char));
    strcpy(otaUrl,url.c_str());

    BLINKER_LOG_ALL(("ota url: "), otaUrl);
        
    _status = BLINKER_UPGRADE_START;

    update();
}

void BlinkerOTA::update() {
    saveOTACheck();
    //Serial.println(F("LOAD OTA URL"));
// #if defined(ESP8266)
//     t_httpUpdate_return ret = ESPhttpUpdate.update(otaUrl);

//     free(otaUrl);
//     //t_httpUpdate_return  ret = ESPhttpUpdate.update("https://server/file.bin");
//     // pubOTAsteptest();
//     // delay(500);

//     switch(ret) {
//         case HTTP_UPDATE_FAILED:
//             //Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s \r\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
//             // pubOTAerrortest();
//             // delay(500);
//             // pubOTAtest();
//             // break;
//             // return UPGRADE_FAIL;
//             _status = BLINKER_UPGRADE_FAIL;
//             return;

//         case HTTP_UPDATE_NO_UPDATES:
//             //Serial.println(F("HTTP_UPDATE_NO_UPDATES"));
//             // pubOTAerrortest();
//             // delay(500);
//             // pubOTAtest();
//             // break;
//             // return UPGRADE_LOAD_FAIL;
//             _status = BLINKER_UPGRADE_LOAD_FAIL;
//             return;

//         case HTTP_UPDATE_OK:
//             //Serial.println(F("HTTP_UPDATE_OK"));
//             // break;
//             // return UPGRADE_SUCCESS;
//             _status = BLINKER_UPGRADE_SUCCESS;
//             return;

//         default :
//             _status = BLINKER_UPGRADE_FAIL;
//             return;
//             // return UPGRADE_FAIL;
//             // break;
//     }
// #elif defined(ESP32)
    BLINKER_LOG_ALL("Connecting to: " + String(ota_host));
    
    if (client_s.connect(ota_host.c_str(), ota_port)) {
        // Connection Succeed.
        // Fecthing the bin
        BLINKER_LOG_ALL("Fetching Bin: " + String(ota_url));
// #if defined(ESP32)
        if (client_s.verify(ota_fingerPrint.c_str(), ota_host.c_str())) {
            BLINKER_LOG_ALL("Fingerprint verified");
        }
        else {
            BLINKER_LOG_ALL("Fingerprint verification failed!");
            return;
        }
// #endif
        // Get the contents of the bin file
        client_s.print(String("GET ") + ota_url + " HTTP/1.1\r\n" +
                    "Host: " + ota_host + "\r\n" +
                    "Cache-Control: no-cache\r\n" +
                    "Connection: close\r\n\r\n");

        unsigned long timeout = millis();
        while (client_s.available() == 0) {
            if (millis() - timeout > 5000) {
                BLINKER_LOG_ALL("Client Timeout !");
                client_s.stop();

                _status = BLINKER_UPGRADE_LOAD_FAIL;
                return;
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

            // Check if the HTTP Response is 200
            // else break and Exit Update
            if (line.startsWith("HTTP/1.1")) {
                if (line.indexOf("200") < 0) {
                    BLINKER_LOG_ALL("Got a non 200 status code from server. Exiting OTA Update.");
                    break;
                }
            }

            // extract headers here
            // Start with content length
            if (line.startsWith("Content-Length: ")) {
                contentLength = atoi((getHeaderValue(line, "Content-Length: ")).c_str());
                BLINKER_LOG_ALL("Got " + String(contentLength) + " bytes from server");
            }

            // Next, the content type
            if (line.startsWith("Content-Type: ")) {
                String contentType = getHeaderValue(line, "Content-Type: ");
                BLINKER_LOG_ALL("Got " + contentType + " payload.");
                if (contentType == "application/octet-stream") {
                    isValidContentType = true;
                }
            }
        }
    }
    else {
        BLINKER_LOG_ALL("Connection to " + String(ota_host) + " failed. Please check your setup");
    }

    // Check what is the contentLength and if content type is `application/octet-stream`
    BLINKER_LOG_ALL("contentLength : " + String(contentLength) + ", isValidContentType : " + String(isValidContentType));

    // check contentLength and content type
    if (contentLength && isValidContentType) {
        // Check if there is enough to OTA Update
        bool canBegin = Update.begin(contentLength);

        // If yes, begin
        if (canBegin) {
            BLINKER_LOG_ALL("Begin OTA. This may take 2 - 5 mins to complete. Things might be quite for a while.. Patience!");
            // No activity would appear on the Serial monitor
            // So be patient. This may take 2 - 5mins to complete
            size_t written = Update.writeStream(client_s);

            if (written == contentLength) {
                BLINKER_LOG_ALL("Written : " + String(written) + " successfully");
            }
            else {
                BLINKER_LOG_ALL("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?" );
            }

            if (Update.end()) {
                BLINKER_LOG_ALL("OTA done!");
                if (Update.isFinished()) {
                    BLINKER_LOG_ALL("Update successfully completed. Rebooting.");
                    _status = BLINKER_UPGRADE_SUCCESS;
                    ESP.restart();
                } else {
                    BLINKER_LOG_ALL("Update not finished? Something went wrong!");
                    _status = BLINKER_UPGRADE_FAIL;
                }
            }
            else {
                BLINKER_LOG_ALL("Error Occurred. Error #: " + String(Update.getError()));
                _status = BLINKER_UPGRADE_FAIL;
            }
        }
        else {
            // not enough space to begin OTA
            // Understand the partitions and
            // space availability
            BLINKER_LOG_ALL("Not enough space to begin OTA");
            client_s.flush();
            _status = BLINKER_UPGRADE_FAIL;
        }
    }
    else {
        BLINKER_LOG_ALL("There was no content in the response");
        client_s.flush();
        _status = BLINKER_UPGRADE_FAIL;
    }
// #endif
}

bool BlinkerOTA::loadOTACheck() {
    char OTACheck;
    EEPROM.begin(BLINKER_EEP_SIZE);
    EEPROM.get(BLINKER_EEP_ADDR_OTA_CHECK, OTACheck);
    EEPROM.commit();
    EEPROM.end();

    if (OTACheck != BLINKER_OTA_START) {
        BLINKER_LOG_ALL(("OTA NOT START"));
        return false;
    }
    else {
        BLINKER_LOG_ALL(("OTA START"));
        return true;
    }
}

void BlinkerOTA::saveOTACheck() {
    EEPROM.begin(BLINKER_EEP_SIZE);
    EEPROM.put(BLINKER_EEP_ADDR_OTA_CHECK, BLINKER_OTA_START);
    EEPROM.commit();
    EEPROM.end();

    BLINKER_LOG_ALL(("OTA START-saveOTACheck()"));
}

void BlinkerOTA::clearOTACheck() {
    EEPROM.begin(BLINKER_EEP_SIZE);
    EEPROM.put(BLINKER_EEP_ADDR_OTA_CHECK, BLINKER_OTA_CLEAR);
    EEPROM.commit();
    EEPROM.end();

    BLINKER_LOG_ALL(("OTACLEAR()"));
    _status = BLINKER_UPGRADE_DISABLE;
}

bool BlinkerOTA::loadVersion() {
#if defined(BLINKER_PRO)
    uint32_t versionCheck;
    EEPROM.begin(BLINKER_EEP_SIZE);
    EEPROM.get(BLINKER_EEP_ADDR_OTA_INFO, versionCheck);//+BUNDLINGSIZE+isBundling
    EEPROM.commit();
    EEPROM.end();

    if (versionCheck != BLINKER_OTA_VERSION_CODE) {
        BLINKER_LOG_ALL(("OTA SUCCESS BLINKER_OTA_VERSION_CODE NOT UPGRADE"));
        _status = BLINKER_UPGRADE_SUCCESS;
        return false;
    }
    else {
        BLINKER_LOG_ALL(("OTA FAIL"));
        _status = BLINKER_UPGRADE_FAIL;
        return true;
    }
#else
    char versionCheck[11];
    
    EEPROM.begin(BLINKER_EEP_SIZE);
    EEPROM.get(BLINKER_EEP_ADDR_OTA_INFO, versionCheck);//+BUNDLINGSIZE+isBundling
    EEPROM.commit();
    EEPROM.end();

    if (String(versionCheck) != String(BLINKER_OTA_VERSION_CODE)) {
        BLINKER_LOG_ALL(("OTA SUCCESS BLINKER_OTA_VERSION_CODE NOT UPGRADE"));
        _status = BLINKER_UPGRADE_SUCCESS;
        return false;
    }
    else {
        BLINKER_LOG_ALL(("OTA FAIL"));
        _status = BLINKER_UPGRADE_FAIL;
        return true;
    }
#endif
}

void BlinkerOTA::saveVersion() {
    EEPROM.begin(BLINKER_EEP_SIZE);
    EEPROM.put(BLINKER_EEP_ADDR_OTA_INFO, BLINKER_OTA_VERSION_CODE);//+BUNDLINGSIZE+isBundling
    EEPROM.commit();
    EEPROM.end();

    BLINKER_LOG_ALL(("SAVE BLINKER_OTA_VERSION_CODE"));
}

// #endif

#endif