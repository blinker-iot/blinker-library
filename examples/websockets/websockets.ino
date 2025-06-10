/*
 * WebSocketServer.ino
 *
 *  Created on: 22.05.2015
 *  Modified for Arduino UNO R4 WiFi
 */

#include <Arduino.h>
#include <WiFiS3.h>
#include <WebSocketsServer.h>

WebSocketsServer webSocket = WebSocketsServer(81);

#define USE_SERIAL Serial

// WiFi 凭据
const char* ssid = "i3water";        // 替换为您的WiFi名称
const char* password = "09876543"; // 替换为您的WiFi密码

void hexdump(const void *mem, uint32_t len, uint8_t cols = 16) {
    const uint8_t* src = (const uint8_t*) mem;
    USE_SERIAL.print("\n[HEXDUMP] Address: 0x");
    USE_SERIAL.print((ptrdiff_t)src, HEX);
    USE_SERIAL.print(" len: 0x");
    USE_SERIAL.print(len, HEX);
    USE_SERIAL.print(" (");
    USE_SERIAL.print(len);
    USE_SERIAL.println(")");
    
    for(uint32_t i = 0; i < len; i++) {
        if(i % cols == 0) {
            USE_SERIAL.print("\n[0x");
            USE_SERIAL.print((ptrdiff_t)src, HEX);
            USE_SERIAL.print("] 0x");
            USE_SERIAL.print(i, HEX);
            USE_SERIAL.print(": ");
        }
        if(*src < 16) USE_SERIAL.print("0");
        USE_SERIAL.print(*src, HEX);
        USE_SERIAL.print(" ");
        src++;
    }
    USE_SERIAL.println();
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            // USE_SERIAL.print("[");
            // USE_SERIAL.print(num);
            // USE_SERIAL.println("] Disconnected!");
            break;
            
        case WStype_CONNECTED:
            {
                // IPAddress ip = webSocket.remoteIP(num);
                // USE_SERIAL.print("[");
                // USE_SERIAL.print(num);
                // USE_SERIAL.print("] Connected from ");
                // USE_SERIAL.print(ip[0]);
                // USE_SERIAL.print(".");
                // USE_SERIAL.print(ip[1]);
                // USE_SERIAL.print(".");
                // USE_SERIAL.print(ip[2]);
                // USE_SERIAL.print(".");
                // USE_SERIAL.print(ip[3]);
                // USE_SERIAL.print(" url: ");
                // USE_SERIAL.println((char*)payload);

                // 向客户端发送连接确认消息
                webSocket.sendTXT(num, "Connected to Arduino UNO R4 WiFi");
            }
            break;
            
        case WStype_TEXT:
            {
                USE_SERIAL.print("[");
                USE_SERIAL.print(num);
                USE_SERIAL.print("] get Text: ");
                USE_SERIAL.println((char*)payload);

                // 回声服务器 - 将收到的消息发送回客户端
                String message = "Echo: " + String((char*)payload);
                webSocket.sendTXT(num, message.c_str());

                // 如果需要广播到所有客户端，取消注释下面的行
                // webSocket.broadcastTXT("message here");
            }
            break;
            
        case WStype_BIN:
            USE_SERIAL.print("[");
            USE_SERIAL.print(num);
            USE_SERIAL.print("] get binary length: ");
            USE_SERIAL.println(length);
            hexdump(payload, length);

            // 发送二进制数据回客户端
            // webSocket.sendBIN(num, payload, length);
            break;
            
        case WStype_ERROR:			
        case WStype_FRAGMENT_TEXT_START:
        case WStype_FRAGMENT_BIN_START:
        case WStype_FRAGMENT:
        case WStype_FRAGMENT_FIN:
            break;
    }
}

void setup() {
    USE_SERIAL.begin(115200);
    
    // 等待串口连接
    while (!USE_SERIAL) {
        delay(10);
    }
    
    USE_SERIAL.println();
    USE_SERIAL.println("=== Arduino UNO R4 WiFi WebSocket Server ===");
    USE_SERIAL.println();

    // 启动倒计时
    for(uint8_t t = 4; t > 0; t--) {
        USE_SERIAL.print("[SETUP] BOOT WAIT ");
        USE_SERIAL.print(t);
        USE_SERIAL.println("...");
        USE_SERIAL.flush();
        delay(1000);
    }

    // 连接到WiFi
    USE_SERIAL.print("Connecting to WiFi: ");
    USE_SERIAL.println(ssid);
    
    WiFi.begin(ssid, password);
    
    // 等待WiFi连接
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(1000);
        USE_SERIAL.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        USE_SERIAL.println();
        USE_SERIAL.println("WiFi connected successfully!");
        delay(5000);
        USE_SERIAL.print("IP address: ");
        USE_SERIAL.println(WiFi.localIP());
        USE_SERIAL.println("WebSocket server started on port 81");
        USE_SERIAL.println();
        USE_SERIAL.print("Connect to: ws://");
        USE_SERIAL.print(WiFi.localIP());
        USE_SERIAL.println(":81");
    } else {
        USE_SERIAL.println();
        USE_SERIAL.println("WiFi connection failed!");
        USE_SERIAL.println("Please check your credentials and try again.");
        return;
    }

    // 启动WebSocket服务器
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
    
    USE_SERIAL.println("WebSocket server is ready!");
}

void loop() {
    // 检查WiFi连接状态
    if (WiFi.status() != WL_CONNECTED) {
        USE_SERIAL.println("WiFi connection lost! Reconnecting...");
        WiFi.begin(ssid, password);
        
        // 等待重新连接
        while (WiFi.status() != WL_CONNECTED) {
            delay(1000);
            USE_SERIAL.print(".");
        }
        
        USE_SERIAL.println();
        USE_SERIAL.println("WiFi reconnected!");
        USE_SERIAL.print("IP address: ");
        USE_SERIAL.println(WiFi.localIP());
    }
    
    // 处理WebSocket事件
    webSocket.loop();
}
