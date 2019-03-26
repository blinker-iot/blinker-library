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

#define BLINKER_PRO
// #define BLINKER_BUTTON
// #define BLINKER_BUTTON_PIN D7

#include <Blinker.h>

/* 
 * Add your command parse code in this function
 * 
 * When get a command and device not parsed this command, device will call this function
 */
bool dataParse(const JsonObject & data)
{
    String getData;

    data.printTo(getData);
    
    BLINKER_LOG("Get user command: ", getData);
    return true;
}

/* 
 * Add your heartbeat message detail in this function
 * 
 * When get heartbeat command {"get": "state"}, device will call this function
 * For example, you can print message back
 * 
 * Every 30s will get a heartbeat command from app 
 */
String direc_set(uint8_t dir)
{
    switch (dir)
    {
        case 0:
            return "东";
        case 1:
            return "西";
        case 2:
            return "南";
        case 3:
            return "北";
        case 4:
            return "东南";
        case 5:
            return "东北";
        case 6:    
            return "西南";
        case 7:
            return "西北";
        default:
            return "东";
    }
}

void heartbeat()
{
    // Blinker.print("hello", "blinker");
    float so2 = random(0, 30) / 2.0;
    float no2 = random(0, 120) / 2.0;
    float co = random(0, 10) / 2.0;
    float o3 = random(0, 180) / 2.0;
    float pm10 = random(0, 200) / 2.0;
    float pm25 = random(0, 160) / 2.0;

    Blinker.print("aqi", so2/60.0 + no2/40.0 + co/4.0 + o3/160.0 + pm10/70.0 + pm25/35.0);
    Blinker.print("temp", random(15, 30));
    Blinker.print("humi", random(50, 80));
    Blinker.print("pressure", random(800, 1000));
    Blinker.print("speed", random(0, 20));
    Blinker.print("direction", direc_set(random(0, 7)));
    Blinker.print("lux", random(0, 60));
    Blinker.print("so2", so2);
    Blinker.print("no2", no2);
    Blinker.print("co", co);
    Blinker.print("o3", o3);
    Blinker.print("pm10", pm10);
    Blinker.print("pm2_5", pm25);

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
void buttonTick()
{
    Blinker.tick();
}

/* 
 * Add your code in this function
 * 
 * When button clicked, device will call this function
 */
void singalClick()
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
    BLINKER_DEBUG.stream(Serial);
    BLINKER_DEBUG.debugAll();

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    Blinker.begin(BLINKER_AIR_STATION);
    Blinker.attachData(dataRead);
    Blinker.attachParse(dataParse);
    Blinker.attachHeartbeat(heartbeat);

#if defined(BLINKER_BUTTON)
    Blinker.attachClick(singalClick);
    Blinker.attachDoubleClick(doubleClick);    
    attachInterrupt(BLINKER_BUTTON_PIN, buttonTick, CHANGE);
#endif
}

void loop()
{
    Blinker.run();
}