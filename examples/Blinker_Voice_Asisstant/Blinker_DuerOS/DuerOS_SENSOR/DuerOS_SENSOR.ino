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
 * Make sure installed 2.7.4 or later ESP8266/Arduino package,
 * if use ESP8266 with Blinker.
 * https://github.com/esp8266/Arduino/releases
 * 
 * Make sure installed 1.0.4 or later ESP32/Arduino package,
 * if use ESP32 with Blinker.
 * https://github.com/espressif/arduino-esp32/releases
 * 
 * Docs: https://diandeng.tech/doc
 *       
 * 
 * *****************************************************************
 * 
 * Blinker 库下载地址:
 * https://github.com/blinker-iot/blinker-library/archive/master.zip
 * 
 * Blinker 是一套跨硬件、跨平台的物联网解决方案，提供APP端、设备端�?
 * 服务器端支持，使用公有云服务进行数据传输存储。可用于智能家居�?
 * 数据监测等领域，可以帮助用户更好更快地搭建物联网项目�?
 * 
 * 如果使用 ESP8266 接入 Blinker,
 * 请确保安装了 2.7.4 或更新的 ESP8266/Arduino 支持包�?
 * https://github.com/esp8266/Arduino/releases
 * 
 * 如果使用 ESP32 接入 Blinker,
 * 请确保安装了 1.0.4 或更新的 ESP32/Arduino 支持包�?
 * https://github.com/espressif/arduino-esp32/releases
 * 
 * 文档: https://diandeng.tech/doc
 *       
 * 
 * *****************************************************************/

#define BLINKER_WIFI
#define BLINKER_DUEROS_SENSOR

#include <Blinker.h>

char auth[] = "Your Device Secret Key";
char ssid[] = "Your WiFi network SSID or name";
char pswd[] = "Your WiFi network WPA password or WEP key";


BLINKERDUEROS BlinkerDuerOS;

void duerQuery(int32_t queryCode)
{
    BLINKER_LOG("DuerOS Query codes: ", queryCode);

    switch (queryCode)
    {
        case BLINKER_CMD_QUERY_AQI_NUMBER :
            BLINKER_LOG("DuerOS Query AQI");
            BlinkerDuerOS.aqi(20);
            BlinkerDuerOS.print();
            break;
        case BLINKER_CMD_QUERY_CO2_NUMBER :
            BLINKER_LOG("DuerOS Query CO2");
            BlinkerDuerOS.co2(20);
            BlinkerDuerOS.print();
            break;
        case BLINKER_CMD_QUERY_PM10_NUMBER :
            BLINKER_LOG("DuerOS Query PM10");
            BlinkerDuerOS.pm10(20);
            BlinkerDuerOS.print();
            break;
        case BLINKER_CMD_QUERY_PM25_NUMBER :
            BLINKER_LOG("DuerOS Query PM25");
            BlinkerDuerOS.pm25(20);
            BlinkerDuerOS.print();
            break;
        case BLINKER_CMD_QUERY_HUMI_NUMBER :
            BLINKER_LOG("DuerOS Query HUMI");
            BlinkerDuerOS.humi(20);
            BlinkerDuerOS.print();
            break;
        case BLINKER_CMD_QUERY_TEMP_NUMBER :
            BLINKER_LOG("DuerOS Query TEMP");
            BlinkerDuerOS.temp(20);
            BlinkerDuerOS.print();
            break;
        case BLINKER_CMD_QUERY_TIME_NUMBER :
            BLINKER_LOG("DuerOS Query time");
            BlinkerDuerOS.time(millis());
            BlinkerDuerOS.print();
            break;
        default :
            BlinkerDuerOS.temp(20);
            BlinkerDuerOS.humi(20);
            BlinkerDuerOS.pm25(20);
            BlinkerDuerOS.pm10(20);
            BlinkerDuerOS.co2(20);
            BlinkerDuerOS.aqi(20);
            BlinkerDuerOS.time(millis());
            BlinkerDuerOS.print();
            break;
    }
}

void dataRead(const String & data)
{
    BLINKER_LOG("Blinker readString: ", data);

    Blinker.vibrate();
    
    uint32_t BlinkerTime = millis();
    
    Blinker.print("millis", BlinkerTime);
}

void setup()
{
    Serial.begin(115200);
    BLINKER_DEBUG.stream(Serial);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    Blinker.begin(auth, ssid, pswd);
    Blinker.attachData(dataRead);

    BlinkerDuerOS.attachQuery(duerQuery);
}

void loop()
{
    Blinker.run();
    BlinkerDuerOS.run();
}
