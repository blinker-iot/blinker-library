# blinker-library
Blinker library for embedded hardware. Works with Arduino, ESP8266, ESP32.  

# What's Blinker
[Blinker](https://blinker-iot.com/) is a platform with iOS and Android apps to control embedded hardware like Arduino. You can easily build graphic interfaces for all your projects by simply dragging and dropping widgets.  
  
[Blinker](https://blinker-iot.com/) 是一个运行在 IOS 和 Android 上用于控制嵌入式硬件的应用程序。你可以通过拖放控制组件，轻松地为你的项目建立图形化控制界面。  

# Reference/参考
* [EN-英文](https://github.com/i3water/blinker-library#currently-supported-hardware)  
* [CN-中文](https://github.com/i3water/blinker-library#%E7%9B%AE%E5%89%8D%E6%94%AF%E6%8C%81%E7%9A%84%E7%A1%AC%E4%BB%B6)  

  
# Currently supported hardware
* Arduino boards
    - Arduino Uno, Duemilanove
    - Arduino Nano, Mini, Pro Mini, Pro Micro, Due, Mega
* ESP8266 based boards with [esp8266/arduino](https://github.com/esp8266/arduino)  
* ESP32 based board with [espressif/arduino-esp32](https://github.com/espressif/arduino-esp32)  
  
# Connection types
* WiFi
* Bluetooth Smart (BLE 4.0)
  
# Prerequisites
You should have the following ready before beginning with any board:
* [Arduino IDE](https://www.arduino.cc/en/Main/Software) 1.6.12 or later
* Install the [esp8266/arduino](https://github.com/esp8266/arduino) core via the Arduino IDE Boards Manager
* Install the [espressif/arduino-esp32](https://github.com/espressif/arduino-esp32) cora following [Installation Instructions](https://github.com/espressif/arduino-esp32#installation-instructions) 
<!-- * Install the [WebSockets](https://github.com/Links2004/arduinoWebSockets) library via the Arduino Library Manager   -->
  
# Blinker Api
## Configuration
### Blinker.begin()
Call **Blinker.begin()** to configure Blinker:
```
Blinker.begin(...);
```
Choose different parameters based on the type of connection you use  
  
BLE:
```
#define BLINKER_BLE  
#include <Blinker.h>  
  
void setup() {  
    Blinker.begin();  
}
```  

>SerialBLE Modules:  
>**Blinker.begin()** will config SoftWareSerial with default settings.  
>  
>Blinker.begin();// default settings digital pins 2(RX) 3(TX) and baudrate 9600 bps  
>Blinker.begin(4, 5);// config digital pins 4(RX) 5(TX) with default baudrate 9600 bps  
>Blinker.begin(4, 5, 115200);// config digital pins 4(RX) 5(TX) and baudrate 115200 bps  
  
WiFi:
```
#define BLINKER_WIFI  
#include <Blinker.h>  
  
void setup() {  
    Blinker.begin(ssid, pswd);  
}
```  
  
MQTT:
```
#define BLINKER_MQTT  
#include <Blinker.h>  
  
void setup() {  
    Blinker.begin(auth, ssid, pswd);  
}
```
> MQTT support hardware: WiFiduino, WiFiduino32, ESP8266, ESP32  

**begin()** is basically doing these steps:  
1.Configure hardware  
2.Wait for connection app  

## Connection management
### Blinker.connect()
This function will try onnecting to app.  
Return true when connected, return false if timeout reached.  
Default timeout is 10 seconds.
```
bool result = Blinker.connect();  
  

uint32_t timeout = 30000;//ms  
bool result = Blinker.connect(timeout);
```
### Blinker.disconnect()
Disconnect **Blinker** connection
```
Blinker.disconnect();
```
### Blinker.connected()
Get the status of **Blinker** connection
```
bool result = Blinker.connected();
```
### Blinker.run()
This function should be called frequently to process incoming commands and perform of Blinker connection. It is usually called in void loop() {}
```
void loop() {
    Blinker.run();
}
```
## Data management
### Blinker.available()
Return true when data already arrived and stored in the receive buffer
```
bool result = Blinker.available();
```
### Blinker.readString()
This function to reads characters from Blinker into a string.
```
String data = Blinker.readString();
```
`*max read data bytes is 256bytes`
### Blinker.print()
Prints data to Blinker app
```
Blinker.print(data);
```
Prints a Json data to Blinkrt app, eg: {"temp":30.2}
```
Blinker.print("temp", 30.2);
```  
Prints a Json data with unit to Blinkrt app, eg: {"temp":"30.2 °C"}
```
Blinker.print("temp", 30.2, "°C");
```
>Json data can display in the Blinker TEXT widget  

`*max send data bytes is 128bytes`

## App Widgets
### Blinker.wInit()
Init widget, **Button** and **Slider** widget recommended to initialize before use.
```
Blinker.wInit("ButtonName", W_BUTTON);  
Blinker.wInit("SliderName", W_SLIDER);  
Blinker.wInit("ToggleName", W_TOGGLE);//keyName, type  
```
### Blinker.button() 
Device receives an update of **Button** state from app, return true when **Pressed**, return false when **Released**.
```
bool result = Blinker.button("Button1");
```  
### Blinker.slider()
Return the latest update of **Slider** value from app
```
uint8_t result = Blinker.slider("Slider1");
```
### Blinker.toggle() 
Device receives an update of **Toggle** state from app, return true when **ON**, return false when **OFF**.
```
bool result = Blinker.toggle("ToggleKey1");
```
### Blinker.joystick()
Return the latest update of **Joystick** value from app
```
uint8_t result_X = Blinker.joystick(J_Xaxis);
uint8_t result_Y = Blinker.joystick(J_Yaxis);
```
### Blinker.ahrs()
Send **AHRS** attach commond to Blinker
```
Blinker.attachAhrs();
```
Return the latest update of **AHRS** value from app
```
int16_t result_Yaw = Blinker.ahrs(Yaw);
int16_t result_Roll = Blinker.ahrs(Roll);
int16_t result_Pitch = Blinker.ahrs(Pitch);
```
Send **AHRS** detach commond to Blinker
```
Blinker.detachAhrs();
```
### Blinker.vibrate()
Send vibrate commond to Blinker, default vibration time is 500 milliseconds
```
Blinker.vibrate();
Blinker.vibrate(255);  
```
## Delay
### Blinker.delay()
This function can process incoming commands and perform of Blinker connection when delay
```
Blinker.delay(500);
```  
  
# Thanks
[WebSockets](https://github.com/Links2004/arduinoWebSockets) - for Blinker to build up a websocket server  
[Adafruit_MQTT_Library](https://github.com/adafruit/Adafruit_MQTT_Library) - for Blinker to build up a mqtt client  
  

---
# 目前支持的硬件
* Arduino boards
    - Arduino Uno, Duemilanove
    - Arduino Nano, Mini, Pro Mini, Pro Micro, Due, Mega
* 使用 [esp8266/arduino](https://github.com/esp8266/arduino) 的ESP8266  
* 使用 [espressif/arduino-esp32](https://github.com/espressif/arduino-esp32) 的ESP32  
  
# 连接类型
* WiFi
* Bluetooth Smart (BLE 4.0)
  
# 准备工作
开始使用前你需要做好如下准备:
* [Arduino IDE](https://www.arduino.cc/en/Main/Software) 1.6.12及更新版本
* 使用 Arduino IDE 的开发板管理器安装 [esp8266/arduino](https://github.com/esp8266/arduino)
* 按照 [安装说明](https://github.com/espressif/arduino-esp32#installation-instructions) 安装 [espressif/arduino-esp32](https://github.com/espressif/arduino-esp32)
<!-- * 使用 Arduino IDE 的库管理器安装 [WebSockets](https://github.com/Links2004/arduinoWebSockets)     -->
  
# Blinker接口函数
## 设备配置
### Blinker.begin()
使用 **Blinker.begin()** 来配置 Blinker:
```
Blinker.begin(...);
```
根据你使用的连接方式选择不同的参数用于配置Blinker  
  

BLE:
```
#define BLINKER_BLE  
#include <Blinker.h>  
  
void setup() {  
    Blinker.begin();  
}
```  
  
>串口蓝牙模块:  
>**Blinker.begin()** 将使用默认设置配置 SoftWareSerial   
>  
>Blinker.begin();// 默认设置: 数字IO 2(RX) 3(TX), 波特率 9600 bps  
>Blinker.begin(4, 5);// 设置数字IO 4(RX) 5(TX), 默认波特率 9600 bps  
>Blinker.begin(4, 5, 115200);// 设置数字IO 4(RX) 5(TX) 及波特率 115200 bps  
  
WiFi:
```
#define BLINKER_WIFI  
#include <Blinker.h>  
  
void setup() {  
    Blinker.begin(ssid, pswd);  
}
```  
  
MQTT:
```
#define BLINKER_MQTT  
#include <Blinker.h>  
  
void setup() {  
    Blinker.begin(auth, ssid, pswd);  
}
```
> MQTT 支持的硬件: WiFiduino, WiFiduino32, ESP8266, ESP32  

**begin()** 主要完成以下配置:  
1.初始化硬件设置;  
2.连接网络并广播设备信息等待app连接;
## 连接管理
### Blinker.connect()
建立 **Blinker** 设备间连接并返回连接状态, 默认超时时间为10秒
```
bool result = Blinker.connect();  
  

uint32_t timeout = 30000;//ms  
bool result = Blinker.connect(timeout);
```
### Blinker.disconnect()
断开 **Blinker** 设备间连接
```
Blinker.disconnect();
```
### Blinker.connected()
返回 **Blinker** 设备间连接状态
```
bool result = Blinker.connected();
```
### Blinker.run()
此函数需要频繁调用以保持设备间连接及处理收到的数据, 建议放在 **loop()** 函数中
```
void loop() {
    Blinker.run();
}
```
## 数据管理
### Blinker.available()
检测是否有接收到数据
```
bool result = Blinker.available();
```
### Blinker.readString()
读取接收到的数据
```
String data = Blinker.readString();
```
`*读取数据最大为 256 字节`
### Blinker.print()
发送数据
```
Blinker.print(data);
```
发送一个Json数据, 如 {text1:data}
```
Blinker.print(text1, data);
```  
发送一个带单位的Json数据, eg: {"temp":"30.2 °C"}
```
Blinker.print("temp", 30.2, "°C");
```
>发送的Json数据可以在 Blinker APP 的 TEXT 组件中显示  

`*发送数据最大为 128 字节`

## App Widgets
### Blinker.wInit()
组件初始化, 建议在使用前初始化 **Button** 和 **Slider**
```
Blinker.wInit("ButtonName", W_BUTTON);  
Blinker.wInit("SliderName", W_SLIDER);//键词, 类型  
```
### Blinker.button() 
读取开关/按键数据, 按下(Pressed)时返回true, 松开(Released)时返回false
```
bool result = Blinker.button("Button1");
```
### Blinker.slider()
读取滑动条数据
```
uint8_t result = Blinker.slider("Slider1");
```
### Blinker.toggle() 
读取拨动开关数据, 打开(ON)时返回true, 关闭(OFF)时返回false
```
bool result = Blinker.toggle("Toggle1");
```
### Blinker.joystick()
读取摇杆数据
```
uint8_t result_X = Blinker.joystick(J_Xaxis);
uint8_t result_Y = Blinker.joystick(J_Yaxis);
```
### Blinker.ahrs()
开启手机 **AHRS** 功能
```
Blinker.attachAhrs();
```
读取 **AHRS** 数据
```
int16_t result_Yaw = Blinker.ahrs(Yaw);
int16_t result_Roll = Blinker.ahrs(Roll);
int16_t result_Pitch = Blinker.ahrs(Pitch);
```
关闭手机 **AHRS** 功能
```
Blinker.dettachAhrs();
```
### Blinker.vibrate()
发送手机振动指令, 震动时间, 单位ms 毫秒, 数值范围0-1000, 默认为500
```
Blinker.vibrate();
Blinker.vibrate(255);  
```
## 设备延时
### Blinker.delay()
延时函数, 在延时过程中仍保持设备间连接及数据接收处理
```
Blinker.delay(500);
```
>*为了连接设备成功, 需要延时时务必使用该函数;  
>使用此函数可以在延时期间连接设备及接收数据并处理数据, 延时完成后才能执行后面的程序;  

# 感谢
[WebSockets](https://github.com/Links2004/arduinoWebSockets) - Blinker 用这个库建立了一个 websocket 服务器  
[Adafruit_MQTT_Library](https://github.com/adafruit/Adafruit_MQTT_Library) - Blinker 用这个库建立了一个 MQTT 客户端

