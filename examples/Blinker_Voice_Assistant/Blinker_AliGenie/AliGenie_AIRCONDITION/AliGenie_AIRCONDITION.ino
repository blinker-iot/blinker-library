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
 * Make sure installed 1.0.5 or later ESP32/Arduino package,
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
 * Blinker 是一套跨硬件、跨平台的物联网解决方案，提供APP端、设备端、
 * 服务器端支持，使用公有云服务进行数据传输存储。可用于智能家居、
 * 数据监测等领域，可以帮助用户更好更快地搭建物联网项目。
 * 
 * 如果使用 ESP8266 接入 Blinker,
 * 请确保安装了 2.7.4 或更新的 ESP8266/Arduino 支持包。
 * https://github.com/esp8266/Arduino/releases
 * 
 * 如果使用 ESP32 接入 Blinker,
 * 请确保安装了 1.0.5 或更新的 ESP32/Arduino 支持包。
 * https://github.com/espressif/arduino-esp32/releases
 * 
 * 文档: https://diandeng.tech/doc
 *       
 * 
 * *****************************************************************/

#define BLINKER_WIFI
#define BLINKER_ALIGENIE_AIRCONDITION

#include <Blinker.h>

char auth[] = "Your Device Secret Key";
char ssid[] = "Your WiFi network SSID or name";
char pswd[] = "Your WiFi network WPA password or WEP key";

bool oState = false;
bool hsState = false;
bool vsState = false;
String setLevel = "auto";
uint8_t setTemp = 26;

void aligenieTemp(uint8_t temp)
{
    BLINKER_LOG("need set temp: ", temp);

    setTemp = temp;

    BlinkerAliGenie.temp(temp);
    BlinkerAliGenie.print();
}

void aligenieRelativeTemp(uint8_t relTemp)
{
    BLINKER_LOG("need set temp: ", relTemp);

    setTemp = setTemp + relTemp;

    BlinkerAliGenie.temp(setTemp);
    BlinkerAliGenie.print();
}

void aligenieLevel(const String & level)
{
    BLINKER_LOG("need set level: ", level);
    // auto	自动风
    // low	低风
    // medium	中风
    // high	高风

    setLevel = level;

    BlinkerAliGenie.level(level);
    BlinkerAliGenie.print();
}

void aligenieHSwingState(const String & state)
{
    BLINKER_LOG("need set HSwing state: ", state);
    // horizontal-swing

    if (state == BLINKER_CMD_ON) {
        digitalWrite(LED_BUILTIN, HIGH);

        BlinkerAliGenie.hswing("on");
        BlinkerAliGenie.print();

        hsState = true;
    }
    else if (state == BLINKER_CMD_OFF) {
        digitalWrite(LED_BUILTIN, LOW);

        BlinkerAliGenie.hswing("off");
        BlinkerAliGenie.print();

        hsState = false;
    }
}

void aligenieVSwingState(const String & state)
{
    BLINKER_LOG("need set VSwing state: ", state);
    // vertical-swing

    if (state == BLINKER_CMD_ON) {
        digitalWrite(LED_BUILTIN, HIGH);

        BlinkerAliGenie.vswing("on");
        BlinkerAliGenie.print();

        vsState = true;
    }
    else if (state == BLINKER_CMD_OFF) {
        digitalWrite(LED_BUILTIN, LOW);

        BlinkerAliGenie.vswing("off");
        BlinkerAliGenie.print();

        vsState = false;
    }
}

void aligenieMode(const String & mode)
{
    BLINKER_LOG("need set mode: ", mode);

    BlinkerAliGenie.mode(mode);
    BlinkerAliGenie.print();
}

void aligeniePowerState(const String & state)
{
    BLINKER_LOG("need set power state: ", state);

    if (state == BLINKER_CMD_ON) {
        digitalWrite(LED_BUILTIN, HIGH);

        BlinkerAliGenie.powerState("on");
        BlinkerAliGenie.print();

        oState = true;
    }
    else if (state == BLINKER_CMD_OFF) {
        digitalWrite(LED_BUILTIN, LOW);

        BlinkerAliGenie.powerState("off");
        BlinkerAliGenie.print();

        oState = false;
    }
}

void aligenieQuery(int32_t queryCode)
{
    BLINKER_LOG("AliGenie Query codes: ", queryCode);

    switch (queryCode)
    {
        case BLINKER_CMD_QUERY_ALL_NUMBER :
            BLINKER_LOG("AliGenie Query All");
            BlinkerAliGenie.powerState(oState ? "on" : "off");
            BlinkerAliGenie.hswing(hsState ? "on" : "off");
            BlinkerAliGenie.vswing(vsState ? "on" : "off");
            BlinkerAliGenie.level(setLevel);
            BlinkerAliGenie.mode("quietWind");
            BlinkerAliGenie.temp(setTemp);
            BlinkerAliGenie.print();
            break;
        case BLINKER_CMD_QUERY_POWERSTATE_NUMBER :
            BLINKER_LOG("AliGenie Query Power State");
            BlinkerAliGenie.powerState(oState ? "on" : "off");
            BlinkerAliGenie.print();
            break;
        default :
            BlinkerAliGenie.powerState(oState ? "on" : "off");
            BlinkerAliGenie.hswing(hsState ? "on" : "off");
            BlinkerAliGenie.vswing(vsState ? "on" : "off");
            BlinkerAliGenie.level(setLevel);
            BlinkerAliGenie.mode("quietWind");
            BlinkerAliGenie.temp(setTemp);
            BlinkerAliGenie.print();
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
    
    BlinkerAliGenie.attachPowerState(aligeniePowerState);
    BlinkerAliGenie.attachHSwing(aligenieHSwingState);
    BlinkerAliGenie.attachVSwing(aligenieVSwingState);
    BlinkerAliGenie.attachLevel(aligenieLevel);
    BlinkerAliGenie.attachMode(aligenieMode);
    BlinkerAliGenie.attachTemp(aligenieTemp);
    BlinkerAliGenie.attachRelativeTemp(aligenieRelativeTemp);
    BlinkerAliGenie.attachQuery(aligenieQuery);
}

void loop()
{
    Blinker.run();
}