#ifndef BLINKER_R4_HTTP_H
#define BLINKER_R4_HTTP_H

#if defined(ARDUINO_ARCH_RENESAS_UNO)

#include "WiFiS3.h"
#include "WiFiSSLClient.h"
#include "WiFiClient.h"

// HTTP状态码定义
#define HTTP_CODE_CONTINUE                  100
#define HTTP_CODE_SWITCHING_PROTOCOLS       101
#define HTTP_CODE_PROCESSING                102
#define HTTP_CODE_OK                        200
#define HTTP_CODE_CREATED                   201
#define HTTP_CODE_ACCEPTED                  202
#define HTTP_CODE_NON_AUTHORITATIVE_INFORMATION 203
#define HTTP_CODE_NO_CONTENT                204
#define HTTP_CODE_RESET_CONTENT             205
#define HTTP_CODE_PARTIAL_CONTENT           206
#define HTTP_CODE_MULTI_STATUS              207
#define HTTP_CODE_ALREADY_REPORTED          208
#define HTTP_CODE_IM_USED                   226
#define HTTP_CODE_MULTIPLE_CHOICES          300
#define HTTP_CODE_MOVED_PERMANENTLY         301
#define HTTP_CODE_FOUND                     302
#define HTTP_CODE_SEE_OTHER                 303
#define HTTP_CODE_NOT_MODIFIED              304
#define HTTP_CODE_USE_PROXY                 305
#define HTTP_CODE_TEMPORARY_REDIRECT        307
#define HTTP_CODE_PERMANENT_REDIRECT        308
#define HTTP_CODE_BAD_REQUEST               400
#define HTTP_CODE_UNAUTHORIZED              401
#define HTTP_CODE_PAYMENT_REQUIRED          402
#define HTTP_CODE_FORBIDDEN                 403
#define HTTP_CODE_NOT_FOUND                 404
#define HTTP_CODE_METHOD_NOT_ALLOWED        405
#define HTTP_CODE_NOT_ACCEPTABLE            406
#define HTTP_CODE_PROXY_AUTHENTICATION_REQUIRED 407
#define HTTP_CODE_REQUEST_TIMEOUT           408
#define HTTP_CODE_CONFLICT                  409
#define HTTP_CODE_GONE                      410
#define HTTP_CODE_LENGTH_REQUIRED           411
#define HTTP_CODE_PRECONDITION_FAILED       412
#define HTTP_CODE_PAYLOAD_TOO_LARGE         413
#define HTTP_CODE_URI_TOO_LONG              414
#define HTTP_CODE_UNSUPPORTED_MEDIA_TYPE    415
#define HTTP_CODE_RANGE_NOT_SATISFIABLE     416
#define HTTP_CODE_EXPECTATION_FAILED        417
#define HTTP_CODE_MISDIRECTED_REQUEST       421
#define HTTP_CODE_UNPROCESSABLE_ENTITY      422
#define HTTP_CODE_LOCKED                    423
#define HTTP_CODE_FAILED_DEPENDENCY         424
#define HTTP_CODE_UPGRADE_REQUIRED          426
#define HTTP_CODE_PRECONDITION_REQUIRED     428
#define HTTP_CODE_TOO_MANY_REQUESTS         429
#define HTTP_CODE_REQUEST_HEADER_FIELDS_TOO_LARGE 431
#define HTTP_CODE_INTERNAL_SERVER_ERROR     500
#define HTTP_CODE_NOT_IMPLEMENTED           501
#define HTTP_CODE_BAD_GATEWAY               502
#define HTTP_CODE_SERVICE_UNAVAILABLE       503
#define HTTP_CODE_GATEWAY_TIMEOUT           504
#define HTTP_CODE_HTTP_VERSION_NOT_SUPPORTED 505
#define HTTP_CODE_VARIANT_ALSO_NEGOTIATES   506
#define HTTP_CODE_INSUFFICIENT_STORAGE      507
#define HTTP_CODE_LOOP_DETECTED             508
#define HTTP_CODE_NOT_EXTENDED              510
#define HTTP_CODE_NETWORK_AUTHENTICATION_REQUIRED 511

class HTTPClient
{
public:
    HTTPClient();
    ~HTTPClient();

    // 初始化方法
    bool begin(String url);
    bool begin(String url, const char* caCert);
    bool begin(WiFiClient &client, String url);
    bool begin(WiFiSSLClient &client, String url);  // 修改这里
    bool begin(String host, uint16_t port, String uri = "/", bool https = false);
    bool begin(WiFiClient &client, String host, uint16_t port, String uri = "/");
    bool begin(WiFiSSLClient &client, String host, uint16_t port, String uri = "/");  // 修改这里

    // 结束连接
    void end();
    void close();

    // 设置方法
    void setTimeout(uint16_t timeout);
    void setConnectTimeout(int32_t connectTimeout);
    void setReuse(bool reuse);
    void setUserAgent(const String& userAgent);
    void setAuthorization(const char* user, const char* password);
    void setAuthorization(const char* auth);

    // 头部管理
    void addHeader(const String& name, const String& value);
    void collectHeaders(const char* headerKeys[], const size_t headerKeysCount);
    String header(const String& name);
    String header(size_t i);
    String headerName(size_t i);
    int headers();
    bool hasHeader(const String& name);

    // HTTP请求方法
    int GET();
    int POST(uint8_t* payload, size_t size);
    int POST(const String& payload);
    int PUT(uint8_t* payload, size_t size);
    int PUT(const String& payload);
    int PATCH(uint8_t* payload, size_t size);
    int PATCH(const String& payload);
    int sendRequest(const char* type, String payload);
    int sendRequest(const char* type, uint8_t* payload, size_t size);
    int sendRequest(const char* type, Stream* stream, size_t size);

    // 响应处理
    String getString();
    String getBody();
    size_t getSize();
    const uint8_t* getStreamPtr();
    WiFiClient* getStreamPtr2();
    WiFiClient& getStream();
    int writeToStream(Stream* stream);

    // 错误处理
    String errorToString(int error);

    // 状态检查
    bool connected();

protected:
    struct RequestArgument {
        String key;
        String value;
    };

    WiFiClient* _client;
    WiFiSSLClient* _clientSecure;  // 修改这里
    
    /// 请求主机
    String _host;
    /// 请求端口
    uint16_t _port;
    /// 请求URI
    String _uri;
    /// 请求协议
    String _protocol;
    /// 请求头部
    RequestArgument* _currentHeaders;
    size_t _headerKeysCount;
    String* _currentHeaderKeys;
    size_t _currentHeadersCount;
    
    /// 认证信息
    String _base64Authorization;
    /// User-Agent
    String _userAgent;
    
    /// 连接超时
    int32_t _connectTimeout;
    /// 读取超时
    uint16_t _timeout;
    
    /// 重用连接
    bool _reuse;
    /// 使用HTTPS
    bool _useHTTPS;
    
    /// 响应代码
    int _returnCode;
    /// 响应大小
    int _size;
    /// 响应头部
    String _responseHeaders;
    /// 响应内容
    String _payload;
    
    bool _canReuse;
    bool _tcpKeepAlive;
    
    // 内部方法
    bool connect();
    bool sendHeader(const char* type);
    int handleHeaderResponse();
    int returnError(int error);
    String base64String(String user, String password);
    bool addHeader(const String& name, const String& value, bool first, bool replace = true);

private:
    // 解析URL
    bool beginInternal(String url, const char* expectedProtocol);
    void clear();
};

// 构造函数
HTTPClient::HTTPClient() 
{
    _client = nullptr;
    _clientSecure = nullptr;
    _currentHeaders = nullptr;
    _currentHeaderKeys = nullptr;
    _headerKeysCount = 0;
    _currentHeadersCount = 0;
    _port = 0;
    _timeout = 5000;
    _connectTimeout = 5000;
    _reuse = true;
    _useHTTPS = false;
    _returnCode = 0;
    _size = -1;
    _canReuse = false;
    _tcpKeepAlive = false;
    _userAgent = "HTTPClient/1.0";
}

// 析构函数
HTTPClient::~HTTPClient() 
{
    if (_client) {
        _client->stop();
    }
    if (_clientSecure) {
        _clientSecure->stop();
    }
    clear();
}

// 清理资源
void HTTPClient::clear() 
{
    if (_currentHeaders) {
        delete[] _currentHeaders;
        _currentHeaders = nullptr;
    }
    if (_currentHeaderKeys) {
        delete[] _currentHeaderKeys;
        _currentHeaderKeys = nullptr;
    }
    _headerKeysCount = 0;
    _currentHeadersCount = 0;
}

// 开始连接 - URL版本
bool HTTPClient::begin(String url) 
{
    return beginInternal(url, nullptr);
}

// 开始连接 - 客户端+URL版本
bool HTTPClient::begin(WiFiClient &client, String url) 
{
    _client = &client;
    _useHTTPS = false;
    return beginInternal(url, "http");
}

bool HTTPClient::begin(WiFiSSLClient &client, String url) 
{
    _clientSecure = &client;
    _useHTTPS = true;
    return beginInternal(url, "https");
}

// 开始连接 - 分离参数版本
bool HTTPClient::begin(String host, uint16_t port, String uri, bool https) 
{
    _host = host;
    _port = port;
    _uri = uri;
    _useHTTPS = https;
    
    if (_useHTTPS && !_clientSecure) {
        _clientSecure = new WiFiSSLClient();
    } else if (!_useHTTPS && !_client) {
        _client = new WiFiClient();
    }
    
    return true;
}

bool HTTPClient::begin(WiFiClient &client, String host, uint16_t port, String uri) 
{
    _client = &client;
    _host = host;
    _port = port;
    _uri = uri;
    _useHTTPS = false;
    return true;
}

bool HTTPClient::begin(WiFiSSLClient &client, String host, uint16_t port, String uri) 
{
    _clientSecure = &client;
    _host = host;
    _port = port;
    _uri = uri;
    _useHTTPS = true;
    return true;
}

// 内部URL解析方法
bool HTTPClient::beginInternal(String url, const char* expectedProtocol) 
{
    clear();
    
    // 解析协议
    if (url.startsWith("http://")) {
        _useHTTPS = false;
        _protocol = "http";
        url = url.substring(7);
        if (_port == 0) _port = 80;
    } else if (url.startsWith("https://")) {
        _useHTTPS = true;
        _protocol = "https";
        url = url.substring(8);
        if (_port == 0) _port = 443;
    } else {
        return false;
    }
    
    // 检查协议匹配
    if (expectedProtocol && _protocol != expectedProtocol) {
        return false;
    }
    
    // 查找路径
    int uriIndex = url.indexOf('/');
    if (uriIndex == -1) {
        _host = url;
        _uri = "/";
    } else {
        _host = url.substring(0, uriIndex);
        _uri = url.substring(uriIndex);
    }
    
    // 查找端口
    int portIndex = _host.indexOf(':');
    if (portIndex != -1) {
        _port = _host.substring(portIndex + 1).toInt();
        _host = _host.substring(0, portIndex);
    }
    
    // 创建客户端
    if (_useHTTPS && !_clientSecure) {
        _clientSecure = new WiFiSSLClient();
    } else if (!_useHTTPS && !_client) {
        _client = new WiFiClient();
    }
    
    return true;
}

// 结束连接
void HTTPClient::end() 
{
    if (_client && _client->connected()) {
        _client->stop();
    }
    if (_clientSecure && _clientSecure->connected()) {
        _clientSecure->stop();
    }
}

void HTTPClient::close() 
{
    end();
}

// 设置超时
void HTTPClient::setTimeout(uint16_t timeout) 
{
    _timeout = timeout;
}

void HTTPClient::setConnectTimeout(int32_t connectTimeout) 
{
    _connectTimeout = connectTimeout;
}

// 设置连接重用
void HTTPClient::setReuse(bool reuse) 
{
    _reuse = reuse;
}

// 设置User-Agent
void HTTPClient::setUserAgent(const String& userAgent) 
{
    _userAgent = userAgent;
}

// 设置认证
void HTTPClient::setAuthorization(const char* user, const char* password) 
{
    if (user && password) {
        _base64Authorization = base64String(String(user), String(password));
    }
}

void HTTPClient::setAuthorization(const char* auth) 
{
    if (auth) {
        _base64Authorization = String(auth);
    }
}

// 添加头部
void HTTPClient::addHeader(const String& name, const String& value) 
{
    addHeader(name, value, false, true);
}

bool HTTPClient::addHeader(const String& name, const String& value, bool first, bool replace) 
{
    // 查找是否已存在
    for (size_t i = 0; i < _currentHeadersCount; i++) {
        if (_currentHeaders[i].key.equalsIgnoreCase(name)) {
            if (replace) {
                _currentHeaders[i].value = value;
                return true;
            }
            return false;
        }
    }
    
    // 添加新头部
    RequestArgument* newHeaders = new RequestArgument[_currentHeadersCount + 1];
    for (size_t i = 0; i < _currentHeadersCount; i++) {
        newHeaders[i] = _currentHeaders[i];
    }
    
    if (first) {
        // 插入到开头
        for (size_t i = _currentHeadersCount; i > 0; i--) {
            newHeaders[i] = newHeaders[i - 1];
        }
        newHeaders[0].key = name;
        newHeaders[0].value = value;
    } else {
        // 添加到末尾
        newHeaders[_currentHeadersCount].key = name;
        newHeaders[_currentHeadersCount].value = value;
    }
    
    if (_currentHeaders) {
        delete[] _currentHeaders;
    }
    _currentHeaders = newHeaders;
    _currentHeadersCount++;
    
    return true;
}

// HTTP GET请求
int HTTPClient::GET() 
{
    return sendRequest("GET", "");
}

// HTTP POST请求
int HTTPClient::POST(const String& payload) 
{
    return sendRequest("POST", payload);
}

int HTTPClient::POST(uint8_t* payload, size_t size) 
{
    return sendRequest("POST", payload, size);
}

// HTTP PUT请求
int HTTPClient::PUT(const String& payload) 
{
    return sendRequest("PUT", payload);
}

int HTTPClient::PUT(uint8_t* payload, size_t size) 
{
    return sendRequest("PUT", payload, size);
}

// HTTP PATCH请求
int HTTPClient::PATCH(const String& payload) 
{
    return sendRequest("PATCH", payload);
}

int HTTPClient::PATCH(uint8_t* payload, size_t size) 
{
    return sendRequest("PATCH", payload, size);
}

// 发送请求 - 字符串版本
int HTTPClient::sendRequest(const char* type, String payload) 
{
    return sendRequest(type, (uint8_t*)payload.c_str(), payload.length());
}

// 发送请求 - 字节数组版本
int HTTPClient::sendRequest(const char* type, uint8_t* payload, size_t size) 
{
    // 建立连接
    if (!connect()) {
        return returnError(-1);
    }
    
    // 发送头部
    if (!sendHeader(type)) {
        return returnError(-2);
    }
    
    // 发送Content-Length
    WiFiClient* stream = _useHTTPS ? (WiFiClient*)_clientSecure : _client;
    if (payload && size > 0) {
        stream->print("Content-Length: ");
        stream->println(size);
    }
    
    stream->println();
    
    // 发送数据
    if (payload && size > 0) {
        stream->write(payload, size);
    }
    
    // 处理响应
    return handleHeaderResponse();
}

// 建立连接
bool HTTPClient::connect() 
{
    if (_useHTTPS) {
        if (!_clientSecure) return false;
        if (_clientSecure->connected()) {
            if (_reuse) return true;
            _clientSecure->stop();
        }
        return _clientSecure->connect(_host.c_str(), _port);
    } else {
        if (!_client) return false;
        if (_client->connected()) {
            if (_reuse) return true;
            _client->stop();
        }
        return _client->connect(_host.c_str(), _port);
    }
}

// 发送头部
bool HTTPClient::sendHeader(const char* type) 
{
    WiFiClient* stream = _useHTTPS ? (WiFiClient*)_clientSecure : _client;
    
    // 请求行
    stream->print(type);
    stream->print(" ");
    stream->print(_uri);
    stream->println(" HTTP/1.1");
    
    // Host头部
    stream->print("Host: ");
    stream->println(_host);
    
    // User-Agent头部
    stream->print("User-Agent: ");
    stream->println(_userAgent);
    
    // 认证头部
    if (_base64Authorization.length() > 0) {
        stream->print("Authorization: Basic ");
        stream->println(_base64Authorization);
    }
    
    // 连接头部
    if (_reuse) {
        stream->println("Connection: keep-alive");
    } else {
        stream->println("Connection: close");
    }
    
    // 自定义头部
    for (size_t i = 0; i < _currentHeadersCount; i++) {
        stream->print(_currentHeaders[i].key);
        stream->print(": ");
        stream->println(_currentHeaders[i].value);
    }
    
    return true;
}

// 处理响应头部
int HTTPClient::handleHeaderResponse() 
{
    WiFiClient* stream = _useHTTPS ? (WiFiClient*)_clientSecure : _client;
    
    _payload = "";
    _responseHeaders = "";
    
    unsigned long timeout = millis();
    while (!stream->available() && millis() - timeout < _timeout) {
        delay(1);
    }
    
    if (!stream->available()) {
        return returnError(-3);
    }
    
    // 读取状态行
    String statusLine = stream->readStringUntil('\n');
    statusLine.trim();
    
    // 解析状态码
    int firstSpace = statusLine.indexOf(' ');
    if (firstSpace != -1) {
        int secondSpace = statusLine.indexOf(' ', firstSpace + 1);
        if (secondSpace != -1) {
            _returnCode = statusLine.substring(firstSpace + 1, secondSpace).toInt();
        }
    }
    
    // 读取响应头部
    String line;
    int contentLength = -1;
    while (stream->available()) {
        line = stream->readStringUntil('\n');
        line.trim();
        
        if (line.length() == 0) {
            break; // 空行表示头部结束
        }
        
        _responseHeaders += line + "\n";
        
        // 检查Content-Length
        if (line.startsWith("Content-Length:")) {
            contentLength = line.substring(15).toInt();
        }
    }
    
    // 读取响应体
    if (contentLength > 0) {
        char* buffer = new char[contentLength + 1];
        int bytesRead = stream->readBytes(buffer, contentLength);
        buffer[bytesRead] = '\0';
        _payload = String(buffer);
        delete[] buffer;
        _size = bytesRead;
    } else {
        // 读取直到连接关闭
        while (stream->available()) {
            _payload += (char)stream->read();
        }
        _size = _payload.length();
    }
    
    return _returnCode;
}

// 获取响应字符串
String HTTPClient::getString() 
{
    return _payload;
}

String HTTPClient::getBody() 
{
    return _payload;
}

// 获取响应大小
size_t HTTPClient::getSize() 
{
    return _size;
}

// 检查连接状态
bool HTTPClient::connected() 
{
    if (_useHTTPS && _clientSecure) {
        return _clientSecure->connected();
    } else if (_client) {
        return _client->connected();
    }
    return false;
}

// Base64编码
String HTTPClient::base64String(String user, String password) 
{
    String auth = user + ":" + password;
    // 这里需要实现Base64编码，简化版本
    return auth; // 实际应该返回Base64编码的字符串
}

// 返回错误
int HTTPClient::returnError(int error) 
{
    _returnCode = error;
    return error;
}

// 错误转字符串
String HTTPClient::errorToString(int error) 
{
    switch (error) {
        case -1: return "Connection failed";
        case -2: return "Send header failed";
        case -3: return "Response timeout";
        default: return "Unknown error";
    }
}

// 头部相关方法的简化实现
void HTTPClient::collectHeaders(const char* headerKeys[], const size_t headerKeysCount) 
{
    _currentHeaderKeys = new String[headerKeysCount];
    for (size_t i = 0; i < headerKeysCount; i++) {
        _currentHeaderKeys[i] = headerKeys[i];
    }
    _headerKeysCount = headerKeysCount;
}

String HTTPClient::header(const String& name) 
{
    // 从_responseHeaders中查找指定头部
    int start = _responseHeaders.indexOf(name + ":");
    if (start != -1) {
        start += name.length() + 1;
        int end = _responseHeaders.indexOf('\n', start);
        if (end != -1) {
            String value = _responseHeaders.substring(start, end);
            value.trim();
            return value;
        }
    }
    return "";
}

String HTTPClient::header(size_t i) 
{
    if (i < _headerKeysCount) {
        return header(_currentHeaderKeys[i]);
    }
    return "";
}

String HTTPClient::headerName(size_t i) 
{
    if (i < _headerKeysCount) {
        return _currentHeaderKeys[i];
    }
    return "";
}

int HTTPClient::headers() 
{
    return _headerKeysCount;
}

bool HTTPClient::hasHeader(const String& name) 
{
    return _responseHeaders.indexOf(name + ":") != -1;
}

#endif // ARDUINO_ARCH_RENESAS_UNO

#endif // BLINKER_R4_HTTP_H