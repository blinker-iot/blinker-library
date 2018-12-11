/* *****************************************************************
 *
 * Download latest Blinker library here:
 * https://github.com/blinker-iot/blinker-library/archive/master.zip
 * 
 * 
 * Blinker is a platform with iOS and Android apps to 
 * control embedded hardware like Arduino, Raspberrt Pi.
 * You can easily build graphic interfaces for all your 
 * projects by simply dragging and dropping widgets.
 * 
 * Docs: https://doc.blinker.app/
 *       https://github.com/blinker-iot/blinker-doc/wiki
 * 
 * *****************************************************************
 * 
 * Blinker 库下载地址:
 * https://github.com/blinker-iot/blinker-library/archive/master.zip
 * 
 * Blinker 是一个运行在 IOS 和 Android 上用于控制嵌入式硬件的应用程序。
 * 你可以通过拖放控制组件，轻松地为你的项目建立图形化控制界面。
 * 
 * 文档: https://doc.blinker.app/
 *       https://github.com/blinker-iot/blinker-doc/wiki
 * 
 * *****************************************************************/

#define BLINKER_PRINT Serial
#define BLINKER_MQTT
#define BLINKER_DUEROS_SENSOR

#include <Blinker.h>

char auth[] = "Your MQTT Secret Key";
char ssid[] = "Your WiFi network SSID or name";
char pswd[] = "Your WiFi network WPA password or WEP key";

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
        default :
            BlinkerDuerOS.temp(20);
            BlinkerDuerOS.humi(20);
            BlinkerDuerOS.pm25(20);
            BlinkerDuerOS.pm10(20);
            BlinkerDuerOS.co2(20);
            BlinkerDuerOS.aqi(20);
            BlinkerDuerOS.print();
            break;
    }
}

void dataRead(const String & data)
{
    BLINKER_LOG("Blinker readString: ", data);

    Blinker.vibrate();
    
    uint32_t BlinkerTime = millis();
    Blinker.print(BlinkerTime);
    Blinker.print("millis", BlinkerTime);
}

void setup()
{
    Serial.begin(115200);

    #if defined(BLINKER_PRINT)
        BLINKER_DEBUG.stream(BLINKER_PRINT);
    #endif

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    Blinker.begin(auth, ssid, pswd);
    Blinker.attachData(dataRead);

    BlinkerDuerOS.attachQuery(duerQuery);
}

void loop()
{
    Blinker.run();
}