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
 * Make sure installed 1.0.2 or later ESP32/Arduino package,
 * if use ESP32 with Blinker.
 * https://github.com/espressif/arduino-esp32/releases
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
 * 如果使用 ESP32 接入 Blinker,
 * 请确保安装了 1.0.2 或更新的 ESP32/Arduino 支持包。
 * https://github.com/espressif/arduino-esp32/releases
 * 
 * 文档: https://doc.blinker.app/
 *       https://github.com/blinker-iot/blinker-doc/wiki
 * 
 * *****************************************************************/

/* 
 * BLINKER_PRO is use for professional device
 * 
 * Please make sure you have permission to modify professional device!
 * Please read usermanual first! Thanks!
 * https://doc.blinker.app/
 * 
 * Written by i3water for blinker.
 * Learn more:https://blinker.app/
 */

#define BLINKER_PRO_ESP
#define BLINKER_BUTTON
#if defined(ESP32)
    #define BLINKER_BUTTON_PIN 4
#else
    #define BLINKER_BUTTON_PIN D7
#endif

#include <Blinker.h>

char type[] = "Your Device Type";
char auth[] = "Your Device Secret Key";

#define powerKey "power"

bool sw_state = false;

float power_read()
{
    return sw_state ? random(0, 10)/2.0 : 0.0;
}

void switch_callback(const String & state)
{
    BLINKER_LOG("get switch state: ", state);

    if (state == BLINKER_CMD_ON) {
        sw_state = true;
        digitalWrite(LED_BUILTIN, HIGH);
        BUILTIN_SWITCH.print("on");
        Blinker.print(powerKey, power_read());
    }
    else {
        sw_state = false;
        digitalWrite(LED_BUILTIN, LOW);
        BUILTIN_SWITCH.print("off");
        Blinker.print(powerKey, power_read());
    }
}

/* 
 * Add your command parse code in this function
 * 
 * When get a command and device not parsed this command, device will call this function
 */
bool dataParse(const JsonObject & data)
{
    String getData;

    serializeJson(data, getData);
    
    BLINKER_LOG("Get user command: ", getData);

    // if you parsed this data, return TRUE.
    // return true;
    return false;
}

/* 
 * Add your heartbeat message detail in this function
 * 
 * When get heartbeat command {"get": "state"}, device will call this function
 * For example, you can print message back
 * 
 * Every 30s will get a heartbeat command from app 
 */
void heartbeat()
{
    BUILTIN_SWITCH.print(sw_state ? "on" : "off");    
    Blinker.print(powerKey, power_read());

    BLINKER_LOG("heartbeat!");
}

#if defined(BLINKER_BUTTON)
/* 
 * Blinker provide a button parse function for user if you defined BLINKER_BUTTON
 * 
 * Blinker button can detect singal click/ double click/ long press
 * 
 * Blinker.tick() will run by default, use interrupt will be better
 */
ICACHE_RAM_ATTR void buttonTick()
{
    Blinker.tick();
}

/* 
 * Add your code in this function
 * 
 * When button clicked, device will call this function
 */
void singleClick()
{
    BLINKER_LOG("Button clicked!");
}

/* 
 * Add your code in this function
 * 
 * When button double clicked, device will call this function
 */
void doubleClick()
{
    BLINKER_LOG("Button double clicked!");
}
#endif

void dataStorage()
{
    Blinker.dataStorage("power", power_read());
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
    BLINKER_DEBUG.debugAll();

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    Blinker.begin(auth, type);
    Blinker.attachData(dataRead);
    Blinker.attachParse(dataParse);
    Blinker.attachHeartbeat(heartbeat);

#if defined(BLINKER_BUTTON)
    Blinker.attachClick(singleClick);
    Blinker.attachDoubleClick(doubleClick);    
    // attachInterrupt(BLINKER_BUTTON_PIN, buttonTick, CHANGE);
#endif

    BUILTIN_SWITCH.attach(switch_callback);
    Blinker.attachDataStorage(dataStorage);
}

void loop()
{
    Blinker.run();
}