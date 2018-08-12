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
#define BLINKER_BLE

#include <Blinker.h>

#include <modules/NeoPixel/Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN            8
#define NUMPIXELS      9
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

#define RGB_1 "RGBKey"

BlinkerRGB WS2812(RGB_1);

void ws2812_callback(uint8_t r_value, uint8_t g_value, uint8_t b_value, uint8_t bright_value)
{
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    BLINKER_LOG2("R value: ", r_value);
    BLINKER_LOG2("G value: ", g_value);
    BLINKER_LOG2("B value: ", b_value);
    BLINKER_LOG2("Rrightness value: ", bright_value);

    uint8_t colorR = map(r_value, 0, 255, 0, bright_value);
    uint8_t colorG = map(g_value, 0, 255, 0, bright_value);
    uint8_t colorB = map(b_value, 0, 255, 0, bright_value);

    for(int i = 0; i < NUMPIXELS; i++){
        pixels.setPixelColor(i, pixels.Color(colorR,colorG,colorB));
        pixels.show();
    }
}

void setup()
{
    Serial.begin(115200);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    Blinker.begin();

    WS2812.attach(ws2812_callback);
}

void loop()
{
    Blinker.run();

    if (Blinker.available()) {
        BLINKER_LOG2("Blinker.readString(): ", Blinker.readString());

        Blinker.vibrate();
        
        uint32_t BlinkerTime = millis();
        Blinker.print(BlinkerTime);
        Blinker.print("millis", BlinkerTime);

        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }
}