#define BLINKER_WIFI_Multi
#define BLINKER_WIFI
#include <Blinker.h>

char auth[] = "Your Device Secret Key";

void setup()
{
  Serial.begin(115200);
  BLINKER_DEBUG.stream(Serial);
  
  Blinker.addAP("ssid_from_AP_1", "your_password_for_AP_1");
  Blinker.addAP("ssid_from_AP_2", "your_password_for_AP_2");
  Blinker.addAP("ssid_from_AP_3", "your_password_for_AP_3");
  
  Blinker.begin(auth);
}

void loop()
{
  Blinker.run();
}
