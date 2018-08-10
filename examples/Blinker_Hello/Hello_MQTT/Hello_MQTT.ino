/*
 * 教程请见
 * https://doc.blinker.app/
 * 快速接入>esp8266MQTT接入
 * 快速接入>esp32MQTT接入
 */

#define BLINKER_PRINT Serial
#define BLINKER_MQTT

#include <Blinker.h>

char auth[] = "Your MQTT Secret Key";
char ssid[] = "Your WiFi network SSID or name";
char pswd[] = "Your WiFi network WPA password or WEP key";

// 新建组件对象
BlinkerButton Button1("btn-abc");
BlinkerNumber Number1("num-abc");

int counter = 0;

// 按下按键即会执行该函数
void button1_callback(const String & state) {
    BLINKER_LOG2("get button state: ", state);
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

void setup() {
    // 初始化串口
    Serial.begin(115200);
    // 初始化有LED的IO
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    // 初始化blinker
    Blinker.begin(auth, ssid, pswd);
    Button1.attach(button1_callback);
}

void loop() {
    Blinker.run();
    // 如果未绑定的组件被触发，则会执行其中内容
    if (Blinker.available()) {
        BLINKER_LOG2("Blinker.readString(): ", Blinker.readString());
        counter++;
        Number1.print(counter);
    }
}