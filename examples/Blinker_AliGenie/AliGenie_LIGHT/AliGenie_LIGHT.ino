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
#define BLINKER_ALIGENIE_LIGHT

#include <Blinker.h>

char auth[] = "Your Device Secret Key";
char ssid[] = "Your WiFi network SSID or name";
char pswd[] = "Your WiFi network WPA password or WEP key";

// Download Adafruit_NeoPixel library here:
// https://github.com/adafruit/Adafruit_NeoPixel
#include <Adafruit_NeoPixel.h>

#define PIN            13
#define NUMPIXELS      24
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

#define RGB_1 "RGBKey"

BlinkerRGB WS2812(RGB_1);

uint8_t colorR, colorG, colorB, colorW;
bool wsState;
String wsMode = BLINKER_CMD_COMMON;

void pixelShow()
{
    pixels.setBrightness(colorW);

    for(int i = 0; i < NUMPIXELS; i++){
        pixels.setPixelColor(i, colorR, colorG, colorB);
    }
    pixels.show();
}

void ws2812_callback(uint8_t r_value, uint8_t g_value, uint8_t b_value, uint8_t bright_value)
{
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    BLINKER_LOG("R value: ", r_value);
    BLINKER_LOG("G value: ", g_value);
    BLINKER_LOG("B value: ", b_value);
    BLINKER_LOG("Rrightness value: ", bright_value);

    colorR = r_value;
    colorG = g_value;
    colorB = b_value;
    colorW = bright_value;

    pixelShow();
}

String getColor()
{
    uint32_t color = colorR << 16 | colorG << 8 | colorB;

    switch (color)
    {
        case 0xFF0000 :
            return "Red";
        case 0xFFFF00 :
            return "Yellow";
        case 0x0000FF :
            return "Blue";
        case 0x00FF00 :
            return "Green";
        case 0xFFFFFF :
            return "White";
        case 0x000000 :
            return "Black";
        case 0x00FFFF :
            return "Cyan";
        case 0x800080 :
            return "Purple";
        case 0xFFA500 :
            return "Orange";
        default :
            return "White";
    }
}

void aligeniePowerState(const String & state)
{
    BLINKER_LOG("need set power state: ", state);

    if (state == BLINKER_CMD_ON) {
        digitalWrite(LED_BUILTIN, HIGH);

        BlinkerAliGenie.powerState("on");
        BlinkerAliGenie.print();

        wsState = true;

        if (colorW == 0) colorW = 255;
    }
    else if (state == BLINKER_CMD_OFF) {
        digitalWrite(LED_BUILTIN, LOW);

        BlinkerAliGenie.powerState("off");
        BlinkerAliGenie.print();

        wsState = false;

        colorW == 0;
    }

    pixelShow();
}

void aligenieColor(const String & color)
{
    BLINKER_LOG("need set color: ", color);

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

    pixelShow();

    BlinkerAliGenie.color(color);
    BlinkerAliGenie.print();
}

void aligenieMode(const String & mode)
{
    BLINKER_LOG("need set mode: ", mode);

    if (mode == BLINKER_CMD_ALIGENIE_READING) {
        // Your mode function
    }
    else if (mode == BLINKER_CMD_ALIGENIE_MOVIE) {
        // Your mode function
    }
    else if (mode == BLINKER_CMD_ALIGENIE_SLEEP) {
        // Your mode function
    }
    else if (mode == BLINKER_CMD_ALIGENIE_HOLIDAY) {
        // Your mode function
    }
    else if (mode == BLINKER_CMD_ALIGENIE_MUSIC) {
        // Your mode function
    }
    else if (mode == BLINKER_CMD_ALIGENIE_COMMON) {
        // Your mode function
    }

    wsMode = mode;

    BlinkerAliGenie.mode(mode);
    BlinkerAliGenie.print();
}

void aligeniecMode(const String & cmode)
{
    BLINKER_LOG("need cancel mode: ", cmode);

    if (cmode == BLINKER_CMD_ALIGENIE_READING) {
        // Your mode function
    }
    else if (cmode == BLINKER_CMD_ALIGENIE_MOVIE) {
        // Your mode function
    }
    else if (cmode == BLINKER_CMD_ALIGENIE_SLEEP) {
        // Your mode function
    }
    else if (cmode == BLINKER_CMD_ALIGENIE_HOLIDAY) {
        // Your mode function
    }
    else if (cmode == BLINKER_CMD_ALIGENIE_MUSIC) {
        // Your mode function
    }
    else if (cmode == BLINKER_CMD_ALIGENIE_COMMON) {
        // Your mode function
    }

    wsMode = BLINKER_CMD_COMMON; // new mode

    BlinkerAliGenie.mode(wsMode); // must response
    BlinkerAliGenie.print();
}

void aligenieBright(const String & bright)
{
    BLINKER_LOG("need set brightness: ", bright);

    if (bright == BLINKER_CMD_MAX) {
        colorW = 255;
    }
    else if (bright == BLINKER_CMD_MIN) {
        colorW = 0;
    }
    else {
        colorW = bright.toInt();
    }

    BLINKER_LOG("now set brightness: ", colorW);

    pixelShow();

    BlinkerAliGenie.brightness(colorW);
    BlinkerAliGenie.print();
}

void aligenieRelativeBright(int32_t bright)
{
    BLINKER_LOG("need set relative brightness: ", bright);

    if (colorW + bright < 255 && colorW + bright >= 0) {
        colorW += bright;
    }

    BLINKER_LOG("now set brightness: ", colorW);

    pixelShow();

    BlinkerAliGenie.brightness(bright);
    BlinkerAliGenie.print();
}

void aligenieColoTemp(int32_t colorTemp)
{
    BLINKER_LOG("need set colorTemperature: ", colorTemp);

    BlinkerAliGenie.colorTemp(colorTemp);
    BlinkerAliGenie.print();
}

void aligenieRelativeColoTemp(int32_t colorTemp)
{
    BLINKER_LOG("need set relative colorTemperature: ", colorTemp);

    BlinkerAliGenie.colorTemp(colorTemp);
    BlinkerAliGenie.print();
}

void aligenieQuery(int32_t queryCode)
{
    BLINKER_LOG("AliGenie Query codes: ", queryCode);

    switch (queryCode)
    {
        case BLINKER_CMD_QUERY_ALL_NUMBER :
            BLINKER_LOG("AliGenie Query All");
            BlinkerAliGenie.powerState(wsState ? "on" : "off");
            BlinkerAliGenie.color(getColor());
            BlinkerAliGenie.mode(wsMode);
            BlinkerAliGenie.colorTemp(50);
            BlinkerAliGenie.brightness(colorW);
            BlinkerAliGenie.print();
            break;
        case BLINKER_CMD_QUERY_POWERSTATE_NUMBER :
            BLINKER_LOG("AliGenie Query Power State");
            BlinkerAliGenie.powerState(wsState ? "on" : "off");
            BlinkerAliGenie.print();
            break;
        case BLINKER_CMD_QUERY_COLOR_NUMBER :
            BLINKER_LOG("AliGenie Query Color");
            BlinkerAliGenie.color(getColor());
            BlinkerAliGenie.print();
            break;
        case BLINKER_CMD_QUERY_MODE_NUMBER :
            BLINKER_LOG("AliGenie Query Mode");
            BlinkerAliGenie.mode(wsMode);
            BlinkerAliGenie.print();
            break;
        case BLINKER_CMD_QUERY_COLORTEMP_NUMBER :
            BLINKER_LOG("AliGenie Query ColorTemperature");
            BlinkerAliGenie.colorTemp(50);
            BlinkerAliGenie.print();
            break;
        case BLINKER_CMD_QUERY_BRIGHTNESS_NUMBER :
            BLINKER_LOG("AliGenie Query Brightness");
            BlinkerAliGenie.brightness(colorW);
            BlinkerAliGenie.print();
            break;
        default :
            BlinkerAliGenie.powerState(wsState ? "on" : "off");
            BlinkerAliGenie.color(getColor());
            BlinkerAliGenie.mode(wsMode);
            BlinkerAliGenie.colorTemp(50);
            BlinkerAliGenie.brightness(colorW);
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
    BlinkerAliGenie.attachColor(aligenieColor);
    BlinkerAliGenie.attachMode(aligenieMode);
    BlinkerAliGenie.attachCancelMode(aligeniecMode);
    BlinkerAliGenie.attachBrightness(aligenieBright);
    BlinkerAliGenie.attachRelativeBrightness(aligenieRelativeBright);
    BlinkerAliGenie.attachColorTemperature(aligenieColoTemp);
    BlinkerAliGenie.attachRelativeColorTemperature(aligenieRelativeColoTemp);
    BlinkerAliGenie.attachQuery(aligenieQuery);

    pinMode(14, OUTPUT);
    digitalWrite(14, HIGH);
    pinMode(15, OUTPUT);
    digitalWrite(15, HIGH);

    colorR = 255;
    colorG = 255;
    colorB = 255;
    colorW = 0;
    wsState = true;

    pixels.begin();
    pixels.setBrightness(colorW);
    WS2812.attach(ws2812_callback);
    pixelShow();
}

void loop()
{
    Blinker.run();

    for(int i = 0; i < NUMPIXELS; i++){
        pixels.setPixelColor(i, colorR, colorG, colorB);
    }
    pixels.show();
}