# blinker-library
Blinker library for embedded hardware. Works with Arduino, ESP8266, ESP32.  

# What's Blinker
[Blinker](https://blinker.app/) is a platform with iOS and Android apps to control embedded hardware like Arduino. You can easily build graphic interfaces for all your projects by simply dragging and dropping widgets.  
  
[Blinker](https://blinker.app/) 是一个运行在 IOS 和 Android 上用于控制嵌入式硬件的应用程序。你可以通过拖放控制组件，轻松地为你的项目建立图形化控制界面。  

# Reference/参考
* [EN-英文](https://github.com/blinker-iot/blinker-doc/wiki/Blinker-Arduino-library-reference)  
* [CN-中文](https://github.com/blinker-iot/blinker-doc/wiki/Blinker-Arduino-%E5%BA%93%E4%BD%BF%E7%94%A8%E6%89%8B%E5%86%8C)  
* [CN-中文/官网](https://doc.blinker.app/?file=003-%E7%A1%AC%E4%BB%B6%E5%BC%80%E5%8F%91/02-Arduino%E6%94%AF%E6%8C%81)  
<!-- 
---  

# 目前支持的硬件
* Arduino boards
    - Arduino Uno, Duemilanove
    - Arduino Nano, Mini, Pro Mini, Pro Micro, Due, Mega
* 使用 [esp8266/arduino](https://github.com/esp8266/arduino) 的ESP8266  
* 使用 [espressif/arduino-esp32](https://github.com/espressif/arduino-esp32) 的ESP32  
  
# 连接类型
* Bluetooth Smart (BLE 4.0)  
* WiFi  
* MQTT  
  
# 准备工作
开始使用前你需要做好如下准备:
* [Arduino IDE](https://www.arduino.cc/en/Main/Software) 1.6.12及更新版本
* 使用 Arduino IDE 的开发板管理器安装 [esp8266/arduino](https://github.com/esp8266/arduino)
* 按照 [安装说明](https://github.com/espressif/arduino-esp32#installation-instructions) 安装 [espressif/arduino-esp32](https://github.com/espressif/arduino-esp32)
  
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
>**Blinker.begin()** 将使用默认设置配置 Serial(默认使用软串口)   
>  
>Blinker.begin();// 默认设置: 数字IO 2(RX) 3(TX), 波特率 9600 bps  
>Blinker.begin(4, 5);// 设置数字IO 4(RX) 5(TX), 默认波特率 9600 bps  
>Blinker.begin(4, 5, 115200);// 设置数字IO 4(RX) 5(TX) 及波特率 115200 bps  
>  
>若配置时Blinker.begin(0, 1);  
>0 1对应硬串口的RX TX, 库会默认使用硬串口与BLE模块进行通信  
>Blinker.begin(15, 14);//Arduino MEGA中如15, 14对应硬串口Serial3  
>  
>注意使用软串口时:  
>使用Arduino MEGA时以下IO可以设置为RX: 10, 11, 12, 13, 50, 51, 52, 53, 62, 63, 64, 65, 66, 67, 68, 69  
>使用Arduino Leonardo时以下IO可以设置为RX: 8, 9, 10, 11, 14, 15, 16  
  
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

```
*发送数据最大为 128 字节  
*MQTT方式接入时, print需间隔1s以上  
例:  
Blinker.print("hello");  
Blinker.delay(1000);  
Blinker.print("world);  
```  

### Blinker.beginFormat() && Blinker.endFormat()
当使用 **beginFormat** 时, **print** 发送出的数据都将以 Json 格式存入发送数据中。 这个发送数据将在使用 **endFormat** 时发送出去。
```
Blinker.beginFormat();  
Blinker.print("Hello","Blinker");
Blinker.print("start","end");
Blinker.print("number",123);
Blinker.endFormat();
```
>使用 endFormat 后, 发送的 Json 数据: {"Hello":"Blinker","start":"end","number":123}  


### Blinker.notify()
使用 **notify** 时, 发送数据以感叹号开始, 将会发送消息通知到app, 否则将会发送Json数据到app  

发送通知
```
Blinker.notify("!notify");
```
发送Json数据, 如 {"notice":"notify"}
```
Blinker.notify("notify");
```

## App Widgets
### Blinker.wInit()
组件初始化, 建议在使用前初始化 **Button** 、**Slider** 、 **Toggle** 及 **RGB**
```
Blinker.wInit("ButtonName", W_BUTTON);  
Blinker.wInit("SliderName", W_SLIDER);  
Blinker.wInit("ToggleName", W_TOGGLE);  
Blinker.wInit("RGBName", W_RGB);//键词, 类型  
```
>类型:  
>W_BUTTON 按键  
>W_SLIDER 滑动条  
>W_TOGGLE 开关  
>W_RGB RGB调色板  
>以上四种组件数量限制为 16个/种

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
### BlinkerButton  
按键组件在App中可以设置 按键/开关/自定义 三种模式,  
**按键** 模式下支持 点按/长按/释放(tap/pre/pup) 三个动作  
**开关** 模式下支持 打开/关闭(on/off) 两个动作  
**自定义** 模式下支持 自定义指令 发送  

初始化, 创建对象
```
#define BUTTON_1 "ButtonKey"

BlinkerButton Button1(BUTTON_1);
```
用于处理 **button** 收到数据的回调函数
```
void button1_callback(const String & state)
{
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    BLINKER_LOG2("get button state: ", state);
}
```
> 在回调函数中, **state** 的值为:  
> **按键** : 点按(tap)/长按(pre)/释放(pup)  
> **开关** : 打开(on)/关闭(off)
> **自定义** : 你设置的值  

在 **setup()** 中注册回调函数
```
Button1.attach(button1_callback);
```
> 也可以在初始化时注册回调函数:
> BlinkerButton Button1(BUTTON_1, button1_callback);  

#### Button1.icon();
Button1.color("#FFFFFF");
Button1.text("Your button name or describe");
Button1.print("on");
### BlinkerJoystick
### BlinkerRGB
### BlinkerSlider
### BlinkerNumber
### BlinkerText
### BUILTIN_SWITCH
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
### Blinker.gps()
刷新手机 **GPS** 功能
```
Blinker.freshAhrs();
```
读取 **GPS** 数据
```
String result_LONG = Blinker.gps(LONG);  
String result_LAT = Blinker.gps(LAT);
```
> LONG 经度  
> LAT 维度  

<!-- ### Blinker.rgb()
读取 **RGB** 数据
```
uint8_t result_R = Blinker.rgb("RGBKEY", R);
uint8_t result_G = Blinker.rgb("RGBKEY", G);
uint8_t result_B = Blinker.rgb("RGBKEY", B);
```
### Blinker.vibrate()
发送手机振动指令, 震动时间, 单位ms 毫秒, 数值范围0-1000, 默认为500
```
Blinker.vibrate();
Blinker.vibrate(255);  
```
## SWITCH
APP中默认 **SWITCH** 组件
### Blinker.switchAvailable()
是否收到APP发来 **SWITCH** 控制指令
```
bool result = Blinker.switchAvailable();
```
### Blinker.switchGet()
读取 **SWITCH** 状态, 打开(ON)时返回true, 关闭(OFF)时返回false
```
bool state = Blinker.switchGet();
```
### Blinker.switchUpdate()
发送 **SWITCH** 最新状态, 用户务必在执行完控制动作后返回
```
Blinker.switchUpdate();
```
### Blinker.switchOn()
设置 **SWITCH** 状态为打开, 会触发 **Blinker.switchAvailable()**
```
Blinker.switchOn();
```
### Blinker.switchOff()
设置 **SWITCH** 状态为关闭, 会触发 **Blinker.switchAvailable()**
```
Blinker.switchOff();
```
## NTP time  
> NTP 目前仅试用于WiFi/MQTT接入  
### Blinker.setTimezone()  
设置时区, 如: 北京时间为+8:00  
```
Blinker.setTimezone(8.0);
```
### Blinker.time()
获取当前ntp时间, 单位为秒(s)
```
uint32 times = Blinker.time();
```
### Blinker.second()
获取当前时间秒数, 单位为秒(s), 获取成功时值: 0-59, 获取失败时值: -1
```
int8_t sec = Blinker.second();
```
### Blinker.minute()
获取当前时间分钟数, 单位为分(m), 获取成功时值: 0-59, 获取失败时值: -1
```
int8_t min = Blinker.minute();
```
### Blinker.hour()
获取当前时间小时数, 单位为小时(h), 获取成功时值: 0-23, 获取失败时值: -1
```
int8_t hour = Blinker.hour();
```
### Blinker.wday()
获取当前时间为当周的日期, 单位为天(d), 获取成功时值: 0-6(依次为周日/一/二/三/四/五/六), 获取失败时值: -1
```
int8_t wday = Blinker.wday();
```
### Blinker.mday()
获取当前时间为当月第几天, 单位为天(d), 获取成功时值: 1-31, 获取失败时值: -1
```
int8_t mday = Blinker.mday();
```
### Blinker.yday()
获取当前时间为当年第几天, 单位为天(d), 获取成功时值: 1-366, 获取失败时值: -1
```
int16_t yday = Blinker.yday();
```
### Blinker.month()
获取当前时间为当年第几月, 单位为月(mon), 获取成功时值: 1-12, 获取失败时值: -1
```
int8_t month = Blinker.month();
```
### Blinker.year()
获取当前时间对应年, 单位为年(y), 获取成功时值: 201x, 获取失败时值: -1
```
int16_t year = Blinker.year();
```
## 设备延时
### Blinker.delay()
延时函数, 在延时过程中仍保持设备间连接及数据接收处理
```
Blinker.delay(500);
```
>*为了连接设备成功, 需要延时时务必使用该函数;  
>使用此函数可以在延时期间连接设备及接收数据并处理数据, 延时完成后才能执行后面的程序;  

## Bridge
**Bridge** 功能用于 **MQTT** 设备与设备间的通信(无需使用app进行控制).
### Blinker.bridge()
填入需要 **Bridge** 桥接通信设备的 **authKey** 建立桥接功能, 桥接成功将返回 true, 桥接失败返回 false.
```
char bridgeKey[] = "Your MQTT Secret Key of bridge to device";

bool state = Blinker.bridge(bridgeKey);
```
> 建立桥接通信的设备务必属于同一个用户的设备, 一个Diy设备最多可以与4个Diy设备建立桥接通信  

### Blinker.bridgeAvailable()
检测是否有接收到桥接设备发来的数据
```
char bridgeKey[] = "Your MQTT Secret Key of bridge to device";

bool result = Blinker.bridgeAvailable(bridgeKey);
```
### Blinker.bridgeRead()
读取接收到的数据
```
char bridgeKey[] = "Your MQTT Secret Key of bridge to device";

String data = Blinker.bridgeRead(bridgeKey);
```
`*读取数据最大为 256 字节`
### Blinker.bridgePrint()
发送数据
```
Blinker.print(data);
```
发送一个Json数据, 如 {text1:data}
```
char bridgeKey[] = "Your MQTT Secret Key of bridge to device";

Blinker.bridgePrint(bridgeKey, text1, data);
```  
发送一个带单位的Json数据, eg: {"temp":"30.2 °C"}
```
char bridgeKey[] = "Your MQTT Secret Key of bridge to device";

Blinker.bridgePrint(bridgeKey, "temp", 30.2, "°C");
```
>发送的Json数据可以在 Blinker APP 的 TEXT 组件中显示  

```
*发送数据最大为 128 字节  
*MQTT方式接入时, bridgePrint需间隔1min以上  
例:  
Blinker.bridgePrint(bridgeKey, "hello");  
Blinker.delay(60000);  
Blinker.bridgePrint(bridgeKey, "world);  
```  

### Blinker.bridgeBeginFormat()&&Blinker.bridgeEndFormat()
当使用 **beginFormat** 时, **bridgePrint** 发送出的数据都将以 Json 格式存入发送数据中。 这个发送数据将在使用 **endFormat** 时发送出去。
```
Blinker.bridgeBeginFormat();  
Blinker.bridgePrint(bridgeKey, "Hello","Blinker");
Blinker.bridgePrint(bridgeKey,"start","end");
Blinker.bridgePrint(bridgeKey, "number",123);
Blinker.bridgeEndFormat();
```
>使用 endFormat 后, 发送的 Json 数据: {"Hello":"Blinker","start":"end","number":123}  
>*MQTT方式接入时, 除间隔1min外建议使用 beginFormat/endFormat 进行数据发送  

## Cloud
### Blinker.cloudUpdate()
上传配置信息到云端
```
Blinker.cloudUpdate("Hello blinker!");
```
> 上传信息数据最大为 256 字节  

### Blinker.cloudGet()
拉取云端的配置信息
```
String cloud_data = Blinker.cloudGet();

BLINKER_LOG2("Blinker.cloudGet(): ", cloud_data);
```
## SMS短信
设备通过 **MQTT** 接入时可以使用 **Blinker.sms()** 默认向该设备所属用户注册对应的手机发送一条短信.
```
Blinker.sms("Hello blinker! Button pressed!");
```
>注: 每个用户短信使用限制为 10条/天/人, 20字/条  
>目前diy用户只能向设备所属用户注册对应的手机发送短信  
  
后期将增加功能，付费用户可以在app端设置10个短信接收手机号, 对其中一个手机号发送一条信息
```
char phone[] = "18712345678";
Blinker.sms("Hello blinker! Button pressed!", phone);
```

>**注意**  
>- 禁止发送互联网金融相关的所有内容，包括验证码、系统通知和营销推广短信  
>- 系统通知类短信不支持营销内容  
>- 禁止发送涉及：色情、赌博、毒品、党政、维权、众筹、慈善募捐、宗教、迷信、股票、移民、面试招聘、博彩、贷款、催款、信用卡提额、投资理财、中奖、抽奖、一元夺宝、一元秒杀、A货、整形、烟酒、交友、暴力、恐吓、皮草、返利、代开发票、代理注册、代办证件、加群、加QQ或者加微信、贩卖个人信息、运营商策反、流量营销等信息的短信  
>- 营销推广短信除上述禁止内容外，另不支持：保险、房地产、教育、培训、游戏、美容、医疗、会所、酒吧、足浴、助考、商标注册、装修、建材、家私、会展、车展、房展等信息的短信  
>- 如出现违法违规或者损害到相关他人权益的,平台将保留最终追究的权利！请各会员严格遵守规范要求，加强自身业务安全，健康发送短信  

## 天气查询
设备通过 **MQTT** 接入时可以使用 **Blinker.weather()** 查询天气情况.
```
String weather_default = Blinker.weather();//默认查询设备ip所属地区的当前时刻的天气情况

String weather_chengdu = Blinker.weather("chengdu");//查询成都市当前时刻的天气情况

String weather_beijing = Blinker.weather("beijing");//查询北京市当前时刻的天气情况
```

```
String location = "chengdu";//传入参数为对应城市拼音/英文
String weather = Blinker.weather(location);
```
**返回信息中字段及信息说明**  

| 参数 | 描述 | 示例 |
| - | - | - |
| fl | 体感温度，默认单位：摄氏度 | 23 |
| tmp | 温度，默认单位：摄氏度 | 21 |
| cond_code | 实况天气状况代码 | 100 |
| cond_txt | 实况天气状况描述 | 晴 |
| wind_deg | 风向360角度 | 305 |
| wind_dir | 风向 | 西北 |
| wind_sc | 风力 | 3 |
| wind_spd | 风速，公里/小时 | 15 |
| hum | 相对湿度 | 40 |
| pcpn | 降水量 | 0 |
| vis | 实况天气状况代码 | 100 |
| cloud | 云量 | 23 |

**天气代码对照表**  

| 代码 | 中文 | 英文 | 
| - | - | - | 
| 100 | 晴 | Sunny/Clear | 
| 101 | 多云 | Cloudy | 
| 102 | 少云 | Few Clouds |
| 103 | 晴间多云 | Partly Cloudy |
| 104 | 阴 | Overcast |
| 200 | 有风 | Windy |
| 201 | 平静 | Calm |
| 202 | 微风 | Light Breeze 
| 203 | 和风 | Moderate/Gentle Breeze |
| 204 | 清风 | Fresh Breeze |
| 205 | 强风/劲风 | Strong Breeze |
| 206 | 疾风 | High Wind, Near Gale |
| 207 | 大风 | Gale |
| 208 | 烈风 | Strong Gale |
| 209 | 风暴 | Storm |
| 210 | 狂爆风 | Violent Storm |
| 211 | 飓风 | Hurricane |
| 212 | 龙卷风 | Tornado |
| 213 | 热带风暴 | Tropical Storm |
| 300 | 阵雨 | Shower Rain |
| 301 | 强阵雨 | Heavy Shower Rain |
| 302 | 雷阵雨 | Thundershower |
| 303 | 强雷阵雨 | Heavy Thunderstorm |
| 304 | 雷阵雨伴有冰雹 | Thundershower with hail |
| 305 | 小雨 | Light Rain |
| 306 | 中雨 | Moderate Rain |
| 307 | 大雨 | Heavy Rain |
| 308 | 极端降雨 | Extreme Rain |
| 309 | 毛毛雨/细雨 | Drizzle Rain |
| 310 | 暴雨 | Storm |
| 311 | 大暴雨 | Heavy Storm |
| 312 | 特大暴雨 | Severe Storm |
| 313 | 冻雨 | Freezing Rain |
| 314 | 小到中雨 | Light to moderate rain |
| 315 | 中到大雨 | Moderate to heavy rain |
| 316 | 大到暴雨 | Heavy rain to storm |
| 317 | 暴雨到大暴雨 | Storm to heavy storm |
| 318 | 大暴雨到特大暴雨 | Heavy to severe storm |
| 399 | 雨 | Rain |
| 400 | 小雪 | Light Snow |
| 401 | 中雪 | Moderate Snow |
| 402 | 大雪 | Heavy Snow |
| 403 | 暴雪 | Snowstorm |
| 404 | 雨夹雪 | Sleet |
| 405 | 雨雪天气 | Rain And Snow |
| 406 | 阵雨夹雪 | Shower Snow |
| 407 | 阵雪 | Snow Flurry |
| 408 | 小到中雪 | Light to moderate snow |
| 409 | 中到大雪 | Moderate to heavy snow |
| 410 | 大到暴雪 | Heavy snow to snowstorm |
| 499 | 雪 | Snow |
| 500 | 薄雾 | Mist |
| 501 | 雾 | Foggy |
| 502 | 霾 | Haze |
| 503 | 扬沙 | Sand |
| 504 | 浮尘 | Dust |
| 507 | 沙尘暴 | Duststorm |
| 508 | 强沙尘暴 | Sandstorm |
| 509 | 浓雾 | Dense fog |
| 510 | 强浓雾 | Strong fog |
| 511 | 中度霾 | Moderate haze |
| 512 | 重度霾 | Heavy haze |
| 513 | 严重霾 | Severe haze |
| 514 | 大雾 | Heavy fog |
| 515 | 特强浓雾 | Extra heavy fog |
| 900 | 热 | Hot |
| 901 | 冷 | Cold |
| 999 | 未知 | Unknown |

## AQI查询
设备通过 **MQTT** 接入时可以使用 **Blinker.aqi()** 查询空气质量情况.
```
String aqi_default = Blinker.aqi();//默认查询设备ip所属地区的当前时刻的空气质量情况

String aqi_chengdu = Blinker.aqi("chengdu");//查询成都市当前时刻的空气质量情况

String aqi_beijing = Blinker.aqi("beijing");//查询北京市当前时刻的空气质量情况
```

```
String location = "chengdu";//传入参数为对应城市拼音/英文
String aqi = Blinker.aqi(location);
```
**返回信息中字段及信息说明**  

| 参数 | 描述 | 示例 | 
| - | - | - | 
|pub_time | 数据发布时间,格式yyyy-MM-dd HH:mm | 2017-03-20 12:30 |
| aqi | 空气质量指数，[AQI和PM25的关系](https://zh.wikipedia.org/wiki/%E7%A9%BA%E6%B0%94%E8%B4%A8%E9%87%8F%E6%8C%87%E6%95%B0) | 74 | 
| main | 主要污染物 | pm25 | 
| qlty | 空气质量，取值范围:优，良，轻度污染，中度污染，重度污染，严重污染，[查看计算方式](https://zh.wikipedia.org/wiki/%E7%A9%BA%E6%B0%94%E8%B4%A8%E9%87%8F%E6%8C%87%E6%95%B0) | 良 |
| pm10 | pm10 | 78 |
| pm25 | pm25 | 66 |
| no2 | 二氧化氮 | 40 |
| so2 | 二氧化硫 | 30 |
| co | 一氧化碳 | 15 |
| o3 | 臭氧 | 20 |  

## Debug
将这行代码添加到你的工程文件第一行, 以启用串口调试输出功能:
```
#define BLINKER_PRINTER Serial
```
在 `void setup()` 中初始化串口Serial :
```
Serial.begin(115200);
```
你可以用额外的硬件串口 (HardWareSerial) 或者软串口 (SoftWareSerial) 来调试输出 (你需要额外的适配器将该串口连接到你的电脑上).  
  
如果你想调试输出更多细节信息 :
```
#define BLINKER_PRINTER Serial
#define BLINKER_DEBUG_ALL  //add this behind
```
## LOG
开启调试输出 (Debug) 后可以使用 **BLINKER_LOG()** 打印输出调试信息:
```
BLINKER_LOG1("detail message 1");  
BLINKER_LOG2("detail message 1", " 2");  
BLINKER_LOG3("detail message 1", " 2", " 3");  
BLINKER_LOG4("detail message 1", " 2", " 3", " 4");  
BLINKER_LOG5("detail message 1", " 2", " 3", " 4", " 5");  
BLINKER_LOG6("detail message 1", " 2", " 3", " 4", " 5", " 6");  
```

# 感谢
[WebSockets](https://github.com/Links2004/arduinoWebSockets) - Blinker 用这个库建立了一个 websocket 服务器  
[Adafruit_MQTT_Library](https://github.com/adafruit/Adafruit_MQTT_Library) - Blinker 用这个库建立了一个 MQTT 客户端  
[ArduinoJson](https://github.com/bblanchon/ArduinoJson) - Blinker 用这个库解析 Json   -->
