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

/*
 *
 *  This example not complete at all, TODO.
 * 
 * /

// #define BLINKER_PRINT Serial
// #define BLINKER_MQTT

// #include <Blinker.h>

// char auth[] = "Your MQTT Secret Key";
// char ssid[] = "Your WiFi network SSID or name";
// char pswd[] = "Your WiFi network WPA password or WEP key";

// #define BUTTON_1 "ButtonKey"

// void setup() {
//     Serial.begin(115200);

//     pinMode(LED_BUILTIN, OUTPUT);
//     digitalWrite(LED_BUILTIN, LOW);

//     Blinker.begin(auth, ssid, pswd);
//     Blinker.beginAuto();
//     Blinker.wInit(BUTTON_1, W_BUTTON);
// }

// void loop()
// {
//     Blinker.run();

//     if (Blinker.available()) {
//         BLINKER_LOG("Blinker.readString(): ", Blinker.readString());

//         uint32_t BlinkerTime = millis();

//         Blinker.beginFormat();
//         Blinker.vibrate();        
//         Blinker.print("millis", BlinkerTime);
//         Blinker.endFormat();
//     }

//     if (Blinker.button(BUTTON_1)) {
//         digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
//     }

//     Blinker.autoInput("key", "value");
//     Blinker.autoRun();

//     Blinker.delay(1000);
// }