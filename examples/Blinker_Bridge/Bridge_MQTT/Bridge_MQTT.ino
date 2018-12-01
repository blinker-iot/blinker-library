// /* *****************************************************************
//  *
//  * Download latest Blinker library here:
//  * https://github.com/blinker-iot/blinker-library/archive/master.zip
//  * 
//  * 
//  * Blinker is a platform with iOS and Android apps to 
//  * control embedded hardware like Arduino, Raspberrt Pi.
//  * You can easily build graphic interfaces for all your 
//  * projects by simply dragging and dropping widgets.
//  * 
//  * Docs: https://doc.blinker.app/
//  *       https://github.com/blinker-iot/blinker-doc/wiki
//  * 
//  * *****************************************************************
//  * 
//  * Blinker 库下载地址:
//  * https://github.com/blinker-iot/blinker-library/archive/master.zip
//  * 
//  * Blinker 是一个运行在 IOS 和 Android 上用于控制嵌入式硬件的应用程序。
//  * 你可以通过拖放控制组件，轻松地为你的项目建立图形化控制界面。
//  * 
//  * 文档: https://doc.blinker.app/
//  *       https://github.com/blinker-iot/blinker-doc/wiki
//  * 
//  * *****************************************************************/

#define BLINKER_PRINT Serial
#define BLINKER_MQTT

#include <Blinker.h>

char auth[] = "Your MQTT Secret Key";
char ssid[] = "Your WiFi network SSID or name";
char pswd[] = "Your WiFi network WPA password or WEP key";

#define BRIDGE_1 "Your MQTT Secret Key of bridge to device"

BlinkerBridge BridgeDevice1(BRIDGE_1);

void bridge1Read(const String & data)
{
    BLINKER_LOG("BridgeDevice1 readString: ", data);

    // must print Json data
    BridgeDevice1.print("{\"hello\":\"bridge\"}");
}

void setup() {
    Serial.begin(115200);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    Blinker.begin(auth, ssid, pswd);

    BridgeDevice1.attach(bridge1Read);
}

void loop()
{
    Blinker.run();
}
