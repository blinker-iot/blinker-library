#ifndef BLINKER_HTTP_CLIENT_FALLBACK_H
#define BLINKER_HTTP_CLIENT_FALLBACK_H

#include "BlinkerPlatformDetector.h"

#if defined(BLINKER_NATIVE_WIFI) && !defined(BLINKER_PLATFORM_ESP32)

#include <Arduino.h>
#include <Client.h>

class HTTPClient
{
    public:
        HTTPClient() {}

        bool begin(const String& url)
        {
            _url = url;
            return parseUrl();
        }

        bool begin(Client& client, const String& url)
        {
            _client = &client;
            return begin(url);
        }

        void addHeader(const String& name, const String& value)
        {
            _headers += name;
            _headers += F(": ");
            _headers += value;
            _headers += F("\r\n");
        }

        int GET()
        {
            return request(F("GET"), String());
        }

        int POST(const String& body)
        {
            return request(F("POST"), body);
        }

        String getString()
        {
            return _payload;
        }

        String errorToString(int code)
        {
            if (code == HTTPC_ERROR_NOT_CONNECTED) return F("connection failed");
            if (code == HTTPC_ERROR_CONNECTION_LOST) return F("connection lost");
            return String(code);
        }

        void end()
        {
            if (_activeClient) _activeClient->stop();
            _activeClient = NULL;
            _client = NULL;
        }

    private:
        static const int HTTPC_ERROR_NOT_CONNECTED = -1;
        static const int HTTPC_ERROR_CONNECTION_LOST = -2;

        bool parseUrl()
        {
            _secure = _url.startsWith(F("https://"));
            bool plain = _url.startsWith(F("http://"));

            if (!_secure && !plain) return false;

            int schemeLen = _secure ? 8 : 7;
            int pathStart = _url.indexOf('/', schemeLen);
            String authority = pathStart == -1 ? _url.substring(schemeLen) : _url.substring(schemeLen, pathStart);
            _path = pathStart == -1 ? F("/") : _url.substring(pathStart);

            int portStart = authority.indexOf(':');
            if (portStart >= 0)
            {
                _host = authority.substring(0, portStart);
                _port = authority.substring(portStart + 1).toInt();
            }
            else
            {
                _host = authority;
                _port = _secure ? 443 : 80;
            }

            return _host.length() > 0;
        }

        int request(const String& method, const String& body)
        {
            _payload = String();

            WiFiClient plainClient;
            BlinkerWiFiSecureClient secureClient;

            if (_client)
            {
                _activeClient = _client;
            }
            else if (_secure)
            {
                blinkerWiFiSecureSetInsecure(secureClient);
                _activeClient = &secureClient;
            }
            else
            {
                _activeClient = &plainClient;
            }

            if (!_activeClient->connect(_host.c_str(), _port))
            {
                _activeClient = NULL;
                return HTTPC_ERROR_NOT_CONNECTED;
            }

            _activeClient->print(method);
            _activeClient->print(F(" "));
            _activeClient->print(_path);
            _activeClient->print(F(" HTTP/1.1\r\nHost: "));
            _activeClient->print(_host);
            _activeClient->print(F("\r\nConnection: close\r\n"));

            if (body.length())
            {
                if (_headers.length() == 0)
                {
                    _activeClient->print(F("Content-Type: application/json;charset=utf-8\r\n"));
                }
                _activeClient->print(F("Content-Length: "));
                _activeClient->print(body.length());
                _activeClient->print(F("\r\n"));
            }

            _activeClient->print(_headers);
            _activeClient->print(F("\r\n"));

            if (body.length()) _activeClient->print(body);

            unsigned long timeout = millis();
            while (!_activeClient->available())
            {
                if (!_activeClient->connected())
                {
                    _activeClient = NULL;
                    return HTTPC_ERROR_CONNECTION_LOST;
                }
                if (millis() - timeout > 10000UL)
                {
                    _activeClient->stop();
                    _activeClient = NULL;
                    return HTTPC_ERROR_CONNECTION_LOST;
                }
                delay(10);
            }

            String statusLine = _activeClient->readStringUntil('\n');
            statusLine.trim();
            int firstSpace = statusLine.indexOf(' ');
            int secondSpace = firstSpace == -1 ? -1 : statusLine.indexOf(' ', firstSpace + 1);
            if (secondSpace == -1) secondSpace = statusLine.length();
            int statusCode = firstSpace == -1 ? HTTPC_ERROR_CONNECTION_LOST : statusLine.substring(firstSpace + 1, secondSpace).toInt();

            bool inBody = false;
            while (_activeClient->connected() || _activeClient->available())
            {
                String line = _activeClient->readStringUntil('\n');
                if (!inBody)
                {
                    if (line == F("\r") || line.length() == 0)
                    {
                        inBody = true;
                    }
                    continue;
                }
                _payload += line;
            }

            _activeClient->stop();
            _activeClient = NULL;
            return statusCode;
        }

        String _url;
        String _host;
        String _path;
        String _headers;
        String _payload;
        uint16_t _port = 80;
        bool _secure = false;
        Client* _client = NULL;
        Client* _activeClient = NULL;
};

#endif

#endif
