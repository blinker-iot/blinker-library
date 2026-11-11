# Device V2 统一 Product API 与平台组合

> **2026-08-18 API/打包修订：**普通入口已改为 `<BlinkerWiFi.h>` 或 `<BlinkerBLE.h>`，
> `Blinker.begin()` 不再传 `blinker::WiFi/BLE/WiFiBLE` tag。公开发布物按 mode 生成完整标准 Arduino 库；
> WiFi+BLE 目前只保留 ESP32 internal 产品。下文仍出现 tag、单一大包或 CloudCredential 默认图的段落只作
> 历史设计证据；当前权威实现见 `DEVICE_V2_CANONICAL_API.md` 与
> `DEVICE_V2_PACKAGING_CI_AND_PLATFORM_MATRIX.md`。

> 正式决策：平台差异只能改变注入的能力和 profile，不能改变业务 API。
> 身份/注册语义已完成纠偏、exact 参数尚未冻结；以
> `DEVICE_V2_WIFI_BLE_ONBOARDING_DECISION.md` 为准。普通 API 不包含 productId，也不要求 BLE/WiFiBLE
> 统一传 SetupCode。

## 公共层

~~~text
Property / Action / Event + Device
                 |
               Product  begin/run/status
                 |
          IProductLifecycle composition
          /          |             \
       WiFi         BLE          WiFi+BLE
                 platform adapters
~~~

`Device` 是唯一 endpoint/state/BBP2 core 所有者；`Product` 是生命周期入口；composition 组装 WiFi、
BLE、HTTP enrollment、cloud session、MQTT、storage、random 和 crypto。一个具体 lifecycle 是 radio/
transport 启停的唯一所有者。

Core 不 include ESP32、WiFiS3、ArduinoBLE、NimBLE 或 PubSubClient。integration、native client、buffer
和 adapter 由应用/产品静态持有，不在 `begin()` 动态拼装，不建立全局 singleton。

这里的“Core 不 include 平台 SDK”是内部模块边界，不等于普通用户必须手工安装并引用多个互不相关的
Arduino 库。Arduino 只会链接当前库源码以及由 include 可达的依赖；一个既未被 include、也不在同一
发布单元中的独立 composition 不会因为安装在本机就自动提供 `integration::lifecycle(...)`。因此最终
官方发布冻结为两层：

- 普通 `Blinker` 分发包：每个平台/mode 确定性组装一个完整 standard Arduino archive；用户只包含
  `<BlinkerWiFi.h>` 或 `<BlinkerBLE.h>`，不传 mode tag、TCP/MQTT/native 对象；
- Advanced 工程包：继续保留纯 Core 与独立 adapter ZIP，供新 MCU 接入、替换 MQTT engine、CI 和资源
  探针使用；它们不是普通用户的多包安装流程。

official composition 可以包含薄平台 glue 并 include 对应 SDK/第三方头，但 `BlinkerV2` Core 目录仍保持
平台无关、无 heap/JSON/SDK 依赖。外部库源码不复制进仓库。当前根包仍是工程 Core，`CompileFacade`
通过 fake lifecycle 验证 API；在 official composition 进入同一普通发布单元前，不得宣称根包单独即可
运行 WiFi/BLE QuickStart。

## 目标用法

普通用户通过入口头在编译依赖扫描前选择 composition，不构造平台对象：

~~~cpp
#include <BlinkerWiFi.h>

BLINKER_PROPERTY(power, bool, blinker::readWrite());

void setPower(bool requested)
{
    digitalWrite(LED_BUILTIN, requested ? HIGH : LOW);
    power.report(digitalRead(LED_BUILTIN) == HIGH);
}

void setup()
{
    power.onWrite(setPower);
    Blinker.begin(
        "REPLACE_WITH_DEVICE_KEY",
        "YOUR_WIFI_SSID",
        "YOUR_WIFI_PASSWORD",
        power);
    power.report(false);
}

void loop() { Blinker.run(); }
~~~

WiFi、BLE 分别使用 `BlinkerWiFi.h`/`BlinkerBLE.h` selector；同一业务对象与 `begin()/run()` 在 ESP32、
UNO R4/Renesas 等平台保持相同。selector 与 mode-specific archive 共同确保静态裁剪。
App 配网的 EnrollmentTicket 由 lifecycle 接收，不进入 ordinary `begin()`；headless WiFi-only 可在 WiFi
profile 中显式提供 SSID/password/ManualDeviceKey。Active 后所有 profile 都跳过 bootstrap，按能力
使用长期 CloudCredential 或 ControllerCredential。
`Device/Product/IProductLifecycle` 及 native dependency injection 留在
Advanced/内部层。ESP32 不得拥有另一个 `Esp32Product` 业务模型。

2026-08-16 边界修订：旧 WiFi+BLE `ProductOrchestrator` 曾直接实现 `IProductLifecycle` 并验证
`Device` prepare、延迟 attach、启动和失败回滚，但它依赖已撤回的 Enrollment/1，现已退出
public/release。保留的是 `IProductLifecycle` seam 与上述行为测试思路；新 coordinator 不复用旧
credential/onboarding 状态。

WiFi-only 已收敛为唯一的 `WifiOnboardingCloudLifecycle`：它同时覆盖 pre-enrolled Active fast-path 与
ManualDeviceKey 首次认领/全擦恢复，按 WiFi → onboarding → Cloud/MQTT 排序并复用同一 control-plane arena。
早期只支持 pre-enrolled 的 `WifiCloudLifecycle` 已删除，不再向 Advanced 用户暴露第二套路径。
ESP32/UNO R4 official composition 已提供 native storage/crypto/Client/MQTT 对象；仍缺真机和生产三端 E2E。

## Product mode

- WiFi：WiFi lifecycle + TLS/HTTP/cloud session + MQTT。
- BLE：BLE mode + secure local session；若不能直接访问 server，App 做 authenticated claim relay。
- WiFiBle：以上二者组合，同一 Device/Manifest/state，独立 transport session。

产品可以不支持某 profile；能力不足必须在构造/begin 返回明确错误，不用 MCU 宏静默降级。

## 外部库

默认 MQTT 适配 PubSubClient，因为 API 小、生态成熟且当前平台验证路径更完整。`IMqttClient` 让产品
可以注入其他 engine；这不等于 Core 同时维护两套行为。BLE adapter 同理。第三方源码不复制进 core
Arduino package。

## 当前执行顺序与退出条件

1. 先完成 R4a：薄 `Blinker` facade、model prepare/data-plane start 分离、离线 Property commit、
   void callback 与 Advanced checked callback；
2. 三 profile 用相同 interaction 示例，并通过 Arduino compile 与 Agent 生成门禁；
3. 再完成 R4b：独立可裁剪的 WiFi/BLE lifecycle 和各平台静态依赖容器，并将 official composition
   确定性组装进普通 Arduino 发布单元；
4. Product status 区分 localReady/cloudReady/provisioning/activation；
5. 真机验证 mode ownership、MQTT/BLE 并发、断线和资源峰值；
6. App/服务器 BBP2 E2E 后才发布联网 QuickStart。

旧 `ProductOrchestrator` 仅是已归档的 seam/回滚实验，不再是内部底座。`BlinkerPlatform platform(...)`
不再是候选普通 API；不得把旧编排器简单改名为 ESP32 专属 facade。详细 API 合同见
`DEVICE_V2_USER_API_AND_LLM_QUICKSTART.md`。
