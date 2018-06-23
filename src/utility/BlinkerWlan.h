#ifndef BLINKER_WLAN_H
#define BLINKER_WLAN_H

#if defined(BLINKER_PRO)

#include <Blinker/BlinkerConfig.h>
#if defined(ESP8266)
    #include <ESP8266WiFi.h>
#elif defined(ESP32)
    #include <WiFi.h>
#endif
#include <EEPROM.h>

enum bwl_status_t{
    BWL_CONFIG_CKECK,
    BWL_CONFIG_FAIL,
    BWL_CONFIG_SUCCESS,
    BWL_CONNECTING,
    BWL_CONNECTED,
    BWL_DISCONNECTED,
    BWL_SMARTCONFIG_BEGIN,
    BWL_SMARTCONFIG_DONE,
    BWL_SMARTCONFIG_TIMEOUT,
    // BWL_STACONFIG_BEGIN,
    BWL_CONNECTED_CHECK,
    BWL_RESET
};

class BlinkerWlan {
    public :
        BlinkerWlan()
            : _status(BWL_CONFIG_CKECK)
        {}

        bool checkConfig();
        void loadConfig(char *_ssid, char *_pswd);
        void saveConfig(char *_ssid, char *_pswd);
        void deleteConfig();
        void smartconfigBegin(uint16_t _time = 15000);
        bool smartconfigDone();
        void connect();
        bool connected();
        void disconnect();
        void reset();
        bool run();
        bwl_status_t status() { return _status; }

    private :

    protected :
        char *SSID;
        char *PSWD;
        uint32_t connectTime;
        uint16_t timeout;
        bwl_status_t _status;
};

bool BlinkerWlan::checkConfig() {
    char ok[2 + 1];
    EEPROM.begin(BLINKER_EEP_SIZE);
    EEPROM.get(BLINKER_EEP_ADDR_WLAN_CHECK, ok);
    EEPROM.commit();
    EEPROM.end();

    if (String(ok) != String("OK")) {
        
        BLINKER_LOG1(("wlan config check,fail"));

        _status = BWL_CONFIG_FAIL;
        return false;
    }
    else {

        BLINKER_LOG1(("wlan config check,success"));

        _status = BWL_CONFIG_SUCCESS;
        return true;
    }
}

void BlinkerWlan::loadConfig(char *_ssid, char *_pswd) {
    char loadssid[BLINKER_SSID_SIZE];
    char loadpswd[BLINKER_PSWD_SIZE];

    EEPROM.begin(BLINKER_EEP_SIZE);
    EEPROM.get(BLINKER_EEP_ADDR_SSID, loadssid);
    EEPROM.get(BLINKER_EEP_ADDR_PSWD, loadpswd);
    // char ok[2 + 1];
    // EEPROM.get(EEP_ADDR_WIFI_CFG + BLINKER_SSID_SIZE + BLINKER_PSWD_SIZE, ok);
    EEPROM.commit();
    EEPROM.end();

    strcpy(_ssid, loadssid);
    strcpy(_pswd, loadpswd);

    BLINKER_LOG4(("SSID: "), _ssid, (" PASWD: "), _pswd);
}

void BlinkerWlan::saveConfig(char *_ssid, char *_pswd) {
    char loadssid[BLINKER_SSID_SIZE];
    char loadpswd[BLINKER_PSWD_SIZE];

    memcpy(loadssid, _ssid, BLINKER_SSID_SIZE);
    memcpy(loadpswd, _pswd, BLINKER_PSWD_SIZE);

    EEPROM.begin(BLINKER_EEP_SIZE);
    EEPROM.put(BLINKER_EEP_ADDR_SSID, loadssid);
    EEPROM.put(BLINKER_EEP_ADDR_PSWD, loadpswd);
    char ok[2 + 1] = "OK";
    EEPROM.put(BLINKER_EEP_ADDR_WLAN_CHECK, ok);
    EEPROM.commit();
    EEPROM.end();

    BLINKER_LOG1(("Save wlan config"));
}

void BlinkerWlan::deleteConfig() {
    char ok[3] = {0};
    EEPROM.begin(BLINKER_EEP_SIZE);
    // for (int i = BLINKER_EEP_ADDR_WLAN_CHECK; i < BLINKER_WLAN_CHECK_SIZE; i++)
    //     EEPROM.write(i, 0);
    EEPROM.put(BLINKER_EEP_ADDR_WLAN_CHECK, ok);
    EEPROM.commit();
    EEPROM.end();

    BLINKER_LOG1(("Erase wlan config"));
}

void BlinkerWlan::smartconfigBegin(uint16_t _time) {
    WiFi.mode(WIFI_STA);
    delay(100);
    WiFi.beginSmartConfig();
    connectTime = millis();
    timeout = _time;
    _status = BWL_SMARTCONFIG_BEGIN;

    BLINKER_LOG1(("Wait for Smartconfig"));
}

bool BlinkerWlan::smartconfigDone() {
    if (WiFi.smartConfigDone())
    {
        WiFi.setAutoConnect(true);
        connectTime = millis();

        _status = BWL_SMARTCONFIG_DONE;

        BLINKER_LOG1(("SmartConfig Success"));

        return true;
    }
    else {
        return false;
    }
}

void BlinkerWlan::connect() {
    switch (_status) {
        case BWL_CONFIG_SUCCESS :
            WiFi.setAutoConnect(false);

            SSID = (char*)malloc(BLINKER_SSID_SIZE*sizeof(char));
            PSWD = (char*)malloc(BLINKER_PSWD_SIZE*sizeof(char));

            loadConfig(SSID, PSWD);
            WiFi.begin(SSID, PSWD);

            free(SSID);
            free(PSWD);

            _status = BWL_CONNECTING;
            break;
        case BWL_DISCONNECTED :
            disconnect();
            SSID = (char*)malloc(BLINKER_SSID_SIZE*sizeof(char));
            PSWD = (char*)malloc(BLINKER_PSWD_SIZE*sizeof(char));

            loadConfig(SSID, PSWD);
            WiFi.begin(SSID, PSWD);

            free(SSID);
            free(PSWD);
            connectTime = millis();
            BLINKER_LOG1(("connecting BWL_DISCONNECTED"));
            break;
    }
}

bool BlinkerWlan::connected() {
    switch (_status) {
        case BWL_SMARTCONFIG_DONE :
            if (WiFi.status() != WL_CONNECTED) {
                if (millis() - connectTime > 15000)
                {
                    BLINKER_LOG1(("\nsmartConfig time out"));
                    
                    WiFi.stopSmartConfig();
                    _status = BWL_SMARTCONFIG_TIMEOUT;
                }
                return false;
            }
            else {
                IPAddress deviceIP = WiFi.localIP();
                BLINKER_LOG1(("WiFi connected"));
                BLINKER_LOG1(("IP address: "));
                BLINKER_LOG1(deviceIP);
                BLINKER_LOG4(("SSID: "), WiFi.SSID(), (" PSWD: "), WiFi.psk());
                
                SSID = (char*)malloc(BLINKER_SSID_SIZE*sizeof(char));
                PSWD = (char*)malloc(BLINKER_PSWD_SIZE*sizeof(char));
                memcpy(SSID,"\0",BLINKER_SSID_SIZE);
                memcpy(SSID,WiFi.SSID().c_str(),BLINKER_SSID_SIZE);
                memcpy(PSWD,"\0",BLINKER_PSWD_SIZE);
                memcpy(PSWD,WiFi.psk().c_str(),BLINKER_PSWD_SIZE);
                saveConfig(SSID, PSWD);
                free(SSID);
                free(PSWD);

                WiFi.setAutoConnect(false);

                _status = BWL_CONNECTED_CHECK;
                return true;
            }
            break;
        case BWL_CONNECTING :
            if(WiFi.status() != WL_CONNECTED) {
                return false;
            }
            else {
                IPAddress deviceIP = WiFi.localIP();
                BLINKER_LOG1(("WiFi connected"));
                BLINKER_LOG1(("IP address: "));
                BLINKER_LOG1(deviceIP);
                BLINKER_LOG4(("SSID: "), WiFi.SSID(), (" PSWD: "), WiFi.psk());
                
                _status = BWL_CONNECTED_CHECK;
                return true;
            }
        case BWL_CONNECTED_CHECK :
            return (WiFi.status() == WL_CONNECTED);
        case BWL_RESET :
            return false;
        default :
            return false;
    }
    return false;
}

void BlinkerWlan::disconnect() {
    WiFi.disconnect(true);
    delay(100);
    _status = BWL_DISCONNECTED;
}

void BlinkerWlan::reset() {
    disconnect();
    _status = BWL_RESET;
}

bool BlinkerWlan::run() {
    switch (_status) {
        case BWL_CONFIG_CKECK :
            checkConfig();
            break;
        case BWL_CONFIG_FAIL :
            smartconfigBegin();
            break;
        case BWL_CONFIG_SUCCESS :
            connect();
            break;
        case BWL_CONNECTING :
            return connected();
            break;
        case BWL_DISCONNECTED :
            connect();
            break;
        case BWL_SMARTCONFIG_BEGIN :
            smartconfigDone();
            break;
        case BWL_SMARTCONFIG_DONE :
            return connected();
            break;
        case BWL_SMARTCONFIG_TIMEOUT :
            _status = BWL_CONFIG_FAIL;
            break;
        // case BWL_STACONFIG_BEGIN :
        //     connect();
        //     break;
        case BWL_CONNECTED_CHECK :
            return connected();
            break;
        case BWL_RESET:
            break;
        default :
            break;
    }
    return false;
}

#endif

#endif