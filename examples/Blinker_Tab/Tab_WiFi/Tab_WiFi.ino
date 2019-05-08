/* *****************************************************************
 *
 * Download latest Blinker library here:
 * https://github.com/blinker-iot/blinker-library/archive/master.zip
 * 
 * 
 * Blinker is a cross-hardware, cross-platform solution for the IoT. 
 * It provides APP, device and server support, 
 * and uses public cloud services for data transmission and storage.
 * It can be used in smart home, data monitoring and other fields 
 * to help users build Internet of Things projects better and faster.
 * 
 * Make sure installed 2.5.0 or later ESP8266/Arduino package,
 * if use ESP8266 with Blinker.
 * https://github.com/esp8266/Arduino/releases
 * 
 * Docs: https://doc.blinker.app/
 *       https://github.com/blinker-iot/blinker-doc/wiki
 * 
 * *****************************************************************
 * 
 * Blinker 库下载地址:
 * https://github.com/blinker-iot/blinker-library/archive/master.zip
 * 
 * Blinker 是一套跨硬件、跨平台的物联网解决方案，提供APP端、设备端、
 * 服务器端支持，使用公有云服务进行数据传输存储。可用于智能家居、
 * 数据监测等领域，可以帮助用户更好更快地搭建物联网项目。
 * 
 * 如果使用 ESP8266 接入 Blinker,
 * 请确保安装了 2.5.0 或更新的 ESP8266/Arduino 支持包。
 * https://github.com/esp8266/Arduino/releases
 * 
 * 文档: https://doc.blinker.app/
 *       https://github.com/blinker-iot/blinker-doc/wiki
 * 
 * *****************************************************************/

#define BLINKER_WIFI

#include <Blinker.h>

char auth[] = "Your Device Secret Key";
char ssid[] = "Your WiFi network SSID or name";
char pswd[] = "Your WiFi network WPA password or WEP key";

#define Tab_1 "TabKey"

BlinkerTab Tab1(Tab_1);

void tab1_callback(uint8_t tab_set)
{
    BLINKER_LOG("get tab set: ", tab_set);

    switch (tab_set)
    {
        case BLINKER_CMD_TAB_0 :
            BLINKER_LOG("tab 0 set");

            Tab1.tab_0();
            break;
        case BLINKER_CMD_TAB_1 :
            BLINKER_LOG("tab 1 set");

            Tab1.tab_1();
            break;
        case BLINKER_CMD_TAB_2 :
            BLINKER_LOG("tab 2 set");

            Tab1.tab_2();
            break;
        case BLINKER_CMD_TAB_3 :
            BLINKER_LOG("tab 3 set");

            Tab1.tab_3();
            break;
        case BLINKER_CMD_TAB_4 :
            BLINKER_LOG("tab 4 set");

            Tab1.tab_4();
            break;
        default:
            break;
    }

    Tab1.print();
}

void dataRead(const String & data)
{
    BLINKER_LOG("Blinker readString: ", data);

    Blinker.vibrate();
    
    uint32_t BlinkerTime = millis();
    Blinker.print(BlinkerTime);
    Blinker.print("millis", BlinkerTime);

    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

void setup()
{
    Serial.begin(115200);
    BLINKER_DEBUG.stream(Serial);
    
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    Blinker.begin(auth, ssid, pswd);
    Blinker.attachData(dataRead);
    Tab1.attach(tab1_callback);
}

void loop()
{
    Blinker.run();
}
