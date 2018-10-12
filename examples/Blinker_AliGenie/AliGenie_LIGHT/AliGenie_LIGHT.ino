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
#define BLINKER_ALIGENIE_LIGHT

#include <Blinker.h>

char auth[] = "Your MQTT Secret Key";
char ssid[] = "Your WiFi network SSID or name";
char pswd[] = "Your WiFi network WPA password or WEP key";

void aligeniePowerSate(const String & state)
{
    BLINKER_LOG2("need set power state: ", state);
}

void aligenieColor(const String & color)
{
    BLINKER_LOG2("need set color: ", color);
}

void aligenieBright(const String & bright)
{
    BLINKER_LOG2("need set brightness: ", bright);
}

void aligenieRelativeBright(int32_t bright)
{
    BLINKER_LOG2("need set relative brightness: ", bright);
}

void aligenieColoTemp(int32_t colorTemp)
{
    BLINKER_LOG2("need set colorTemperature: ", colorTemp);
}

void aligenieRelativeColoTemp(int32_t colorTemp)
{
    BLINKER_LOG2("need set relative colorTemperature: ", colorTemp);
}

void aligenieQuery(int32_t queryCode)
{
    BLINKER_LOG2("AliGenie Query codes: ", queryCode);
}

void setup()
{
    Serial.begin(115200);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    Blinker.begin(auth, ssid, pswd);

    Blinker.attachSetPowerState(aligeniePowerSate);
    
    BlinkerAliGenie.attachSetColor(aligenieColor);
    BlinkerAliGenie.attachSetBrightness(aligenieBright);
    BlinkerAliGenie.attachRelativeBrightness(aligenieRelativeBright);
    BlinkerAliGenie.attachSetColorTemperature(aligenieColoTemp);
    BlinkerAliGenie.attachRelativeColorTemperature(aligenieRelativeColoTemp);
    BlinkerAliGenie.attachQuery(aligenieQuery);
}

void loop()
{
    Blinker.run();

    if (Blinker.available()) {
        BLINKER_LOG2("Blinker.readString(): ", Blinker.readString());

        uint32_t BlinkerTime = millis();

        Blinker.vibrate();        
        Blinker.print("millis", BlinkerTime);
    }
}