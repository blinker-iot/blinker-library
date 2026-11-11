# Device V2 官方 WiFi 包：ESP32 与 UNO R4

> 状态：2026-08-18。普通 WiFi 已切到唯一 DeviceKey challenge/session 图；旧
> `WifiOnboardingProfile/WifiComposition/WiFi.onboard(...)` 已退出发布边界。本文描述当前设备侧可编译事实，
> 不把尚未完成的生产 Gateway/Broker、Key rotate 或真机测试写成已完成。

## 1. 唯一用户入口

ESP32 与 UNO R4 使用同一 Sketch：

```cpp
#include <BlinkerWiFi.h>

BLINKER_PROPERTY(power, bool, blinker::readWrite());

void setPower(bool requested) {
    power.report(requested);
}

void setup() {
    power.onWrite(setPower);
    Blinker.begin(
        "REPLACE_WITH_DEVICE_KEY",
        "YOUR_WIFI_SSID",
        "YOUR_WIFI_PASSWORD",
        power);
}

void loop() {
    Blinker.run();
}
```

DeviceKey 是 32 B 随机值的 43 字符无 padding base64url 编码，不带 `bdk_`、`bek_` 等前缀。
参数顺序固定为 DeviceKey、SSID、password；开放网络传空 password。普通 Sketch 不 include adapter、
WiFi、HTTP、MQTT、crypto 或 storage 头文件，也不调用 `official_detail`。

## 2. 当前实现链路

```text
Blinker.begin(DeviceKey, SSID, password, interactions...)
  -> WifiDeviceKeyProfile
  -> integration::lifecycle(profile)
  -> DeviceKeyWifiProduct<Platform>
       -> WifiCredentialStore + WifiConnectionLifecycle
       -> ConfiguredDeviceKeySource
       -> HttpDeviceKeySessionProvider
       -> short-lived SessionCredential
       -> PubSubClientAdapter + ManagedMqttTransport
       -> MqttFrameTransport + BBP/2 Client
       -> platform capability
            ESP32: WiFiClient[/Secure] + NVS
            UNO R4: WiFiClient/SSLClient + Preferences journal
```

设备每次启动都用 DeviceKey challenge-HMAC 申请短期 MQTT credential。DeviceKey 不作为 deviceId；
MQTT username/password 不持久化。Key 刷新后的旧 Key、旧 session 和现有连接必须由生产服务器主动吊销。

## 3. 包与依赖

| 字段 | ESP32 | UNO R4 |
|---|---|---|
| `platformProfile` | `esp32-wifi` | `uno-r4-wifi` |
| architecture | `esp32` | `renesas_uno` |
| storage | NVS | WiFiS3 Preferences journal |
| mandatory external dependency | PubSubClient | PubSubClient |
| default cloud root | DeviceKey | DeviceKey |
| server signature/cloud encryption key ring | 不需要 | 不需要 |

Plain/TLS 是构建期选择。TLS 包必须提供 CA；Plain 不得被描述为机密链路，也不得运行时静默升级或降级。
Cloud Frame Authentication 属于后续增强批，不阻塞当前功能联调，但在不可信网络上仍是生产安全门槛。

## 4. 当前验证证据

- ESP32、UNO R4、Wio Terminal 新 WiFi facade clean compile 通过；Wio 当前只验证公共 API，不代表已有
  official WiFi product adapter。
- ESP32/UNO R4 Plain/TLS 官方包形状、依赖边界和确定性 ZIP 通过。
- 真实 `CloudSmokeTest` 官方包 clean compile：
  - ESP32：`1,011,211 B Flash / 49,888 B global RAM`；
  - UNO R4：`127,320 B Flash / 12,724 B global RAM`。
- `CloudSmokeTest` 使用公开 API，包含可写 `power` Property、5 秒 uptime 状态上报和下行回报；不再使用
  `ClientStorage/EndpointDescriptor/IProductLifecycle/official_detail`。

## 5. 尚未完成

- 真实 APP 创建/显示/刷新 DeviceKey；
- 生产 HTTP challenge/session、Broker ACL、旧 session 主动断线；
- 真机 WiFi、断网重连、heap/stack watermark、掉电和 24 h；
- WiFi+BLE 改接同一 DeviceKey product；
- 发布生成链路继续确保每个模式输出标准 Arduino `src`，并收口仓库内 products/adapters 的维护结构。

以上未完成项不能通过恢复旧 EnrollmentKey/CloudCredential 默认图规避。
