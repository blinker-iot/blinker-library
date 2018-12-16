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
#define BLINKER_WIFI
#define BLINKER_DUEROS_LIGHT

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

void duerPowerState(const String & state)
{
    BLINKER_LOG("need set power state: ", state);

    if (state == BLINKER_CMD_ON) {
        digitalWrite(LED_BUILTIN, HIGH);

        BlinkerDuerOS.powerState("on");
        BlinkerDuerOS.print();

        wsState = true;

        if (colorW == 0) colorW = 255;
    }
    else if (state == BLINKER_CMD_OFF) {
        digitalWrite(LED_BUILTIN, LOW);

        BlinkerDuerOS.powerState("off");
        BlinkerDuerOS.print();

        wsState = false;
    }

    pixelShow();
}

void duerColor(const String & color)
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

    BlinkerDuerOS.color(color);
    BlinkerDuerOS.print();
}

void duerMode(const String & mode)
{
    BLINKER_LOG("need set mode: ", mode);

    if (cmode == BLINKER_CMD_DUEROS_READING) {
        // Your mode function
    }
    else if (cmode == BLINKER_CMD_DUEROS_SLEEP) {
        // Your mode function
    }
    else if (cmode == BLINKER_CMD_DUEROS_ALARM) {
        // Your mode function
    }
    else if (cmode == BLINKER_CMD_DUEROS_NIGHT_LIGHT) {
        // Your mode function
    }
    else if (cmode == BLINKER_CMD_DUEROS_ROMANTIC) {
        // Your mode function
    }
    else if (cmode == BLINKER_CMD_DUEROS_READING) {
        // Your mode function
    }

    wsMode = mode;

    BlinkerDuerOS.mode(mode);
    BlinkerDuerOS.print();
}

void duercMode(const String & cmode)
{
    BLINKER_LOG("need cancel mode: ", cmode);

    if (cmode == BLINKER_CMD_DUEROS_READING) {
        // Your mode function
    }
    else if (cmode == BLINKER_CMD_DUEROS_SLEEP) {
        // Your mode function
    }
    else if (cmode == BLINKER_CMD_DUEROS_ALARM) {
        // Your mode function
    }
    else if (cmode == BLINKER_CMD_DUEROS_NIGHT_LIGHT) {
        // Your mode function
    }
    else if (cmode == BLINKER_CMD_DUEROS_ROMANTIC) {
        // Your mode function
    }
    else if (cmode == BLINKER_CMD_DUEROS_READING) {
        // Your mode function
    }

    wsMode = BLINKER_CMD_COMMON; // new mode

    BlinkerDuerOS.mode(wsMode); // must response
    BlinkerDuerOS.print();
}

void duerBright(const String & bright)
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

    BlinkerDuerOS.brightness(colorW);
    BlinkerDuerOS.print();
}

void duerRelativeBright(int32_t bright)
{
    BLINKER_LOG("need set relative brightness: ", bright);

    if (colorW + bright < 255 && colorW + bright >= 0) {
        colorW += bright;
    }

    BLINKER_LOG("now set brightness: ", colorW);

    pixelShow();

    BlinkerDuerOS.brightness(bright);
    BlinkerDuerOS.print();
}

void duerQuery(int32_t queryCode)
{
    BLINKER_LOG("DuerOS Query codes: ", queryCode);

    switch (queryCode)
    {
        case BLINKER_CMD_QUERY_TIME_NUMBER :
            BLINKER_LOG("DuerOS Query time");
            BlinkerDuerOS.time(millis());
            BlinkerDuerOS.print();
            break;
        default :
            BlinkerDuerOS.time(millis());
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
    
    BlinkerDuerOS.attachPowerState(duerPowerState);
    BlinkerDuerOS.attachColor(duerColor);
    BlinkerDuerOS.attachMode(duerMode);
    BlinkerDuerOS.attachCancelMode(duercMode);
    BlinkerDuerOS.attachBrightness(duerBright);
    BlinkerDuerOS.attachRelativeBrightness(duerRelativeBright);
    BlinkerDuerOS.attachQuery(duerQuery);

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