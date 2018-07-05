/*
 * BLINKER_PRO is use for professional device
 * 
 * Please make sure you have permission to modify professional device!
 * Please read usermanual first! Thanks!
 * https://doc.blinker.app/?file=home-%E9%A6%96%E9%A1%B5
 */

#define BLINKER_PRINT Serial
#define BLINKER_PRO
#define BLINKER_BUTTON
#define BLINKER_BUTTON_PIN D7

#include <Blinker.h>

#define BUTTON_1 "ButtonKey"

bool dataParse(const JsonObject & data) {
    String getData;

    data.printTo(getData);
    
    BLINKER_LOG2("Get user command: ", getData);
    return true;
}

void singalClick() {
    BLINKER_LOG1("Button clicked!");
}

void doubleClick() {
    BLINKER_LOG1("Button double clicked!");
}

void setup() {
    Serial.begin(115200);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    Blinker.attachParse(dataParse);
    Blinker.attachClick(singalClick);
    Blinker.attachDoubleClick(doubleClick);

    Blinker.begin(BLINKER_AIR_DETECTOR);
    Blinker.wInit(BUTTON_1, W_BUTTON);
}

void loop()
{
    Blinker.run();

    if (Blinker.available()) {
        BLINKER_LOG2("Blinker.readString(): ", Blinker.readString());

        uint32_t BlinkerTime = millis();

        Blinker.beginFormat();
        Blinker.vibrate();        
        Blinker.print("millis", BlinkerTime);
        Blinker.endFormat();
    }

    if (Blinker.button(BUTTON_1)) {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }
}