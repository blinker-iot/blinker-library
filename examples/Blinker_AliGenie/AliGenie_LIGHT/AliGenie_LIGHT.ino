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

// Download Adafruit_NeoPixel library here:
// https://github.com/adafruit/Adafruit_NeoPixel
#include <Adafruit_NeoPixel.h>

#define PIN            5
#define NUMPIXELS      9
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

#define RGB_1 "RGBKey"

BlinkerRGB WS2812(RGB_1);

uint8_t colorR, colorG, colorB, colorW;
bool wsState;

void ws2812_callback(uint8_t r_value, uint8_t g_value, uint8_t b_value, uint8_t bright_value)
{
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    BLINKER_LOG2("R value: ", r_value);
    BLINKER_LOG2("G value: ", g_value);
    BLINKER_LOG2("B value: ", b_value);
    BLINKER_LOG2("Rrightness value: ", bright_value);

    colorR = r_value;
    colorG = g_value;
    colorB = b_value;
    colorW = bright_value;

    for(int i = 0; i < NUMPIXELS; i++){
        pixels.setPixelColor(i, colorR, colorG, colorB, colorW);
    }
    pixels.show();
}

void aligeniePowerSate(const String & state)
{
    BLINKER_LOG2("need set power state: ", state);

    if (state == BLINKER_CMD_ON) {
        digitalWrite(LED_BUILTIN, HIGH);

        BlinkerAliGenie.powerState("on");
        BlinkerAliGenie.print();

        wsState = true;

        colorW = 255;

        for(int i = 0; i < NUMPIXELS; i++){
            pixels.setPixelColor(i, colorR, colorG, colorB, colorW);
        }
        pixels.show();
    }
    else if (state == BLINKER_CMD_OFF) {
        digitalWrite(LED_BUILTIN, LOW);

        BlinkerAliGenie.powerState("off");
        BlinkerAliGenie.print();

        wsState = false;

        for(int i = 0; i < NUMPIXELS; i++){
            pixels.setPixelColor(i, colorR, colorG, colorB, 0);
        }
        pixels.show();
    }
}

void aligenieColor(const String & color)
{
    BLINKER_LOG2("need set color: ", color);

    if (color == "Red") {
        colorR = 255;
        colorG = 0;
        colorB = 0;
    }
    else if (color == "Yellow") {
        colorR = 255;
        colorG = 255;
        colorB = 0;
    }
    else if (color == "Blue") {
        colorR = 0;
        colorG = 0;
        colorB = 255;
    }
    else if (color == "Green") {
        colorR = 0;
        colorG = 255;
        colorB = 0;
    }
    else if (color == "White") {
        colorR = 255;
        colorG = 255;
        colorB = 255;
    }
    else if (color == "Black") {
        colorR = 0;
        colorG = 0;
        colorB = 0;
    }
    else if (color == "Cyan") {
        colorR = 0;
        colorG = 255;
        colorB = 255;
    }
    else if (color == "Purple") {
        colorR = 128;
        colorG = 0;
        colorB = 128;
    }
    else if (color == "Orange") {
        colorR = 255;
        colorG = 165;
        colorB = 0;
    }

    if (wsState == false) {
        wsState = true;
        colorW = 255;
    }

    if (colorW == 0) {
        colorW = 255;
    }

    for(int i = 0; i < NUMPIXELS; i++){
        pixels.setPixelColor(i, colorR, colorG, colorB, colorW);
    }
    pixels.show();

    BlinkerAliGenie.color(color);
    BlinkerAliGenie.print();
}

void aligenieBright(int32_t bright)
{
    BLINKER_LOG2("need set brightness: ", bright);

    BlinkerAliGenie.brightness(bright);
    BlinkerAliGenie.print();
}

void aligenieRelativeBright(int32_t bright)
{
    BLINKER_LOG2("need set relative brightness: ", bright);

    BlinkerAliGenie.brightness(bright);
    BlinkerAliGenie.print();
}

void aligenieColoTemp(int32_t colorTemp)
{
    BLINKER_LOG2("need set colorTemperature: ", colorTemp);

    BlinkerAliGenie.colorTemp(colorTemp);
    BlinkerAliGenie.print();
}

void aligenieRelativeColoTemp(int32_t colorTemp)
{
    BLINKER_LOG2("need set relative colorTemperature: ", colorTemp);

    BlinkerAliGenie.colorTemp(colorTemp);
    BlinkerAliGenie.print();
}

void aligenieQuery(int32_t queryCode)
{
    BLINKER_LOG2("AliGenie Query codes: ", queryCode);

    switch (queryCode)
    {
        case BLINKER_CMD_QUERY_ALL_NUMBER :
            BLINKER_LOG1("AliGenie Query All");
            BlinkerAliGenie.powerState("on");
            BlinkerAliGenie.color("red");
            BlinkerAliGenie.colorTemp(50);
            BlinkerAliGenie.brightness(50);
            BlinkerAliGenie.print();
            break;
        case BLINKER_CMD_POWERSTATE_NUMBER :
            BLINKER_LOG1("AliGenie Query Power State");
            BlinkerAliGenie.powerState("on");
            BlinkerAliGenie.print();
            break;
        case BLINKER_CMD_COLOR_NUMBER :
            BLINKER_LOG1("AliGenie Query Color");
            BlinkerAliGenie.color("red");
            BlinkerAliGenie.print();
            break;
        case BLINKER_CMD_COLORTEMP_NUMBER :
            BLINKER_LOG1("AliGenie Query ColorTemperature");
            BlinkerAliGenie.colorTemp(50);
            BlinkerAliGenie.print();
            break;
        case BLINKER_CMD_BRIGHTNESS_NUMBER :
            BLINKER_LOG1("AliGenie Query Brightness");
            BlinkerAliGenie.brightness(50);
            BlinkerAliGenie.print();
            break;
        default :
            break;
    }
}

void setup()
{
    Serial.begin(115200);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    Blinker.begin(auth, ssid, pswd);

    Blinker.attachSetPowerState(aligeniePowerSate);
    
    BlinkerAliGenie.attachColor(aligenieColor);
    BlinkerAliGenie.attachBrightness(aligenieBright);
    BlinkerAliGenie.attachRelativeBrightness(aligenieRelativeBright);
    BlinkerAliGenie.attachColorTemperature(aligenieColoTemp);
    BlinkerAliGenie.attachRelativeColorTemperature(aligenieRelativeColoTemp);
    BlinkerAliGenie.attachQuery(aligenieQuery);

    WS2812.attach(ws2812_callback);

    colorR = 0;
    colorG = 0;
    colorB = 0;
    colorW = 0;
    wsState = true;
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