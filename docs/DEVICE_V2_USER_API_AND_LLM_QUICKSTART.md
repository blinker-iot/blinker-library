# Device V2 用户 API 与 LLM QuickStart 正式修订

> 状态：2026-08-18。R4a facade、callback、离线状态和普通批量入口已实现；ESP32/UNO R4 WiFi-only
> official composition 已正式切换到无前缀 DeviceKey challenge/session，并完成真实官方包编译。
> ESP32/UNO R4/Wio 普通 BLE 已绑定统一 facade/package；ESP32 WiFi+BLE 已增加正式源码 selector 与
> QuickStart，App/Service、Agent E2E 和真机仍未完成；UNO R4/Wio 明确不支持 WiFi+BLE。
> `Device/Product/Client/IProductLifecycle`
> 属于 Advanced/内部层，不能继续作为 QuickStart 扩散到各平台。
> 手动 WiFi connectivity 参数固定为 `Blinker.begin(deviceKey, ssid, password, interactions...)`；旧
> `developerEnrollmentToken/setupKey/localAccessKey` config 已从 public facade 删除，不应生成新 Sketch。
> 按 `DEVICE_V2_WIFI_BLE_ONBOARDING_DECISION.md`，
> ordinary Sketch 不传 productId，也不统一传 SetupCode。WiFiBLE/BLE 的 EnrollmentTicket 由 App 配网流程
> 管理；手动 WiFi-only 使用长期可查看/刷新的 DeviceKey。

## 1. 决策

此前暂缓继续增加 UNO R4、ESP32 等平台 composition 对象，先完成 R4a 用户 API 冻结。原因不是
R4 composition 不需要，而是修订前的普通示例暴露 `BLINKER_DEVICE`、`Device`、`Product`、
`BlinkerPlatform`、profile 和 `blinker::Result`。这些入口现已退出普通示例；R4b 可以在统一 facade
合同上继续，而不能重新复制平台专属业务 API。

保留的优点：

- `dev_edu`/Blynk 式的一次 `begin()`、持续 `run()` 和对象 callback；
- Device V2 的 Property/Action/Event、BBP/2、固定资源、非阻塞状态机和依赖解耦；
- 同一业务 Sketch 可跨 ESP32、UNO R4/Renesas 等平台复用。

不保留：Widget 设备类、Sketch 中永久 MQTT bearer authKey、JSON、动态 singleton、阻塞式联网、每 MCU
一套产品 API、手工 transport/buffer dependency graph。

## 2. 普通用户入口

普通用户每个 Sketch 只选择一个产品头文件：

~~~cpp
#include <BlinkerWiFi.h>  // WiFi/cloud
// 或 #include <BlinkerBLE.h>  // BLE-only
// ESP32 也可 #include <BlinkerWiFiBLE.h>  // App配网 + cloud + Direct BLE
~~~

头文件是编译期产品选择器，`Blinker.begin()` 中不再重复传模式。单一标准 Arduino ZIP 包含各平台实现，
selector 隔离外部依赖，最终由 section GC 和 ELF defined-symbol 门禁只保留可达的
adapter/native client/static workspace；不按模式或 MCU 拆 ZIP。
`Blinker.h` 仍是内部公共 facade 聚合头，但不再作为普通 Sketch 的产品选择入口。
ESP32 可用 `<BlinkerWiFiBLE.h>` 选择同一 ZIP 内的 WiFi+BLE 产品；它仍处于联调准入阶段，UNO R4/Wio
不提供该 selector。

唯一 WiFi-only 发布示例是：

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
    pinMode(LED_BUILTIN, OUTPUT);
    power.onWrite(setPower);

    if (!Blinker.begin(
            "REPLACE_WITH_DEVICE_KEY",
            "YOUR_WIFI_SSID",
            "YOUR_WIFI_PASSWORD",
            power)) {
        Serial.println(Blinker.lastErrorText());
    }
    power.report(false);
}

void loop() { Blinker.run(); }
~~~

三个字符串只由 WiFi `begin()` 重载接受；顺序固定为 DeviceKey、SSID、password，开放网络传空 password。
它们在 `begin()` 内同步校验/复制，实际联网、DeviceKey challenge/session、MQTT 和 BBP/2 由 `run()`
非阻塞推进。设备每次重启都以同一 DeviceKey 获取短期 SessionCredential，不保存长期 MQTT bearer。
WiFiBLE 示例不在 Sketch 中出现 SSID/password/永久 authKey/SetupCode；App 在 SoftAP WiFiProv Security1
session 中写入 WiFi 和一次性 DeviceKey/controller bootstrap。设备正常重启不依赖 App 短期 intent。
`begin()` 只完成模型和 runtime 的同步初始化，不阻塞等待 WiFi、BLE、HTTP 或 MQTT；实际连接由
`run()` 状态机推进。

## 3. 为什么 interaction 显式传给 begin

不使用隐藏的全局自动注册。`Blinker.begin(..., power, ...)` 显式列出 interaction，可以：

- 由声明顺序确定稳定 Manifest ordinal/Field ID；
- 避免静态初始化顺序问题；
- 不增加第二份全局指针注册表；
- 让编译器在漏注册、重复注册和资源超限时给出直接错误；
- 让用户和 LLM 一眼看到该设备公开的完整能力。

## 4. callback 与状态合同

普通 callback 使用 `void`，错误处理由库的状态和诊断 API 承担：

- `Property<T>::onWrite(void (*)(T requested))`；
- `Action<T>::onInvoke(void (*)(T input))`；
- callback 必须在硬件接受请求后显式 `report(actual)`；
- 不把 callback 返回值自动解释为状态，以免异步动作、拒绝和真实硬件值产生歧义；
- 需要返回结构化错误的高级用户使用 `onWriteChecked` / `onInvokeChecked`。

`report(actual)` 的成功定义为“本地权威状态已提交”，不是“所有远端已经收到”。断网时最新
Property 保留在 StateStore，重连后通过 State snapshot 同步；Event 是瞬时事实，默认离线返回
NotConnected，若产品需要可靠离线事件，必须显式选择有界 outbox profile。

单字段继续使用 `power.report(true)`。多个相关 Property 使用 R4a 类型安全批量入口：

~~~cpp
Blinker.report(
    power, true,
    temperature, 24.5f);
~~~

该调用提交一个 PATCH Report 和一个 `stateRevision`，内部复用现有 `StateUpdate`。连续两个
`property.report()` 则明确表示两个独立状态变化；库不按超时隐式合并，也不维护动态 JSON buffer。

## 5. 已修正的生命周期问题

旧实现中 `Client::report()` 要求完整 `Client::begin()` 已完成，而 WiFi+BLE provisioning 阶段会故意
延后 data-plane Client 启动。这与 QuickStart 在 `Blinker.begin()` 后立即初始化 Property 的预期冲突。

R4a-2 已完成以下拆分：

1. `Client::prepare()`：绑定、seal 和验证 Manifest，初始化 StateStore；允许本地 Property commit；
2. `Client::begin()`：在 CloudCredential 可用后启动 transport/data plane；
3. 离线 Property 只写现有 StateStore，不另建 per-field pending buffer；
4. 联网后以当前 revision 的 snapshot 收敛，不承诺回放每一次中间状态。

host 19/19、ASan 和 200,000 次 deterministic fuzz 已通过。facade 后续必须直接复用该合同，不能再
建立另一份离线缓存。

## 6. 分层与依赖所有权

~~~text
Sketch / Agent generated code
  -> BlinkerWiFi.h | BlinkerBLE.h selector
  -> Property / Action / Event
  -> Device / Product / Client                 (Advanced/internal)
  -> BBP/2 runtime + StateStore + TransportHub (Core)
  -> narrow adapters
       -> PubSubClient / native WiFi / BLE / storage / crypto
~~~

官方 integration package 用静态对象持有 native client、adapter、buffer 和 lifecycle，不使用 heap，
也不要求普通用户传入 TCP/MQTT 对象。默认 MQTT adapter 仍是 PubSubClient。高级产品可通过
Advanced API 注入兼容的 MQTT/TCP/BLE/storage/crypto 实现，但不会形成第二套业务 API。

## 7. 跨平台规则

- WiFi 业务代码在 ESP32、UNO R4/Renesas 等平台相同，只由 integration package 替换 adapter；
- BLE 业务代码同样跨平台；模式只更换 `<BlinkerWiFi.h>/<BlinkerBLE.h>`，产品包只链接其 lifecycle；
- UNO R4 只支持独立 WiFi 或独立 BLE，不提供 WiFiBLE；
- 不新增 `Esp32Product`、`R4Product` 或 MCU 专属 interaction；
- UNO R4 已通过完整 WiFi-only onboarding 静态编译，但仍是 PlainFlash 教育 profile，不伪装成 secure BLE provisioning；
- Wio Terminal 已复用同一公开 WiFi/BLE API；WiFi 已完成真机 DeviceKey、MQTT/BBP2 与 Gateway 恢复门禁。
  BLE 已通过首次安全认领，但 `rpcBLE 2.1.3` 不能可靠重新广播，因此只标记 experimental，示例/API 一致不等于
  平台已获 production 准入。

## 8. LLM 友好门禁

文档和示例只保留一套普通语法，并提供机器可读的 API 摘要与禁止项：

- 生成 `Property` 传感器、可写控制、`Action`、`Event`；只有 ESP32 WiFiBLE 产品测试才生成双连接示例；
- 禁止生成旧 Widget、JSON、Sketch authKey、`addTransport()`、手工 buffer 或 `blinker::Result` callback；
- interaction 对象名避开 `alarm`、`time`、`index` 等常见 C/SDK 全局符号；需要保持外部 key 时使用
  `*_KEY` 宏，例如对象 `alarmEvent` 对应 stable key `"alarm"`；
- WiFi 与 BLE 示例只允许入口头和必要连接参数不同；不为 UNO R4 生成 WiFi+BLE；
- 所有示例进入真实 Arduino compile CI；
- Agent 评测至少覆盖继电器、传感器、一次动作、告警和双连接，记录首轮编译成功率与误用 Advanced API 次数。

## 9. 清理范围

本批清理结果与后续规则：

- README 和普通 examples 删除 `BLINKER_DEVICE`、手工 `Device/Product` 与 Result callback；
- 原 `CorePortable` 已改名为 `AdvancedCoreModel`，不再叫 QuickStart；
- 删除文档中的伪 API `BlinkerPlatform platform(...)`；
- 若代码搜索证明没有调用方，删除只服务旧手工构造路径的 compatibility overload；
- 继续禁止 dynamic singleton、旧 Widget/TLV/JSON 和复制第三方库回流。

## 10. 执行顺序

1. R4a-1（已完成）：无 radio 依赖的薄 `Blinker` facade 与 fake lifecycle host 测试；
2. R4a-2（已完成）：拆分 model prepare/data-plane start，实现离线 Property commit；
3. R4a-3（已完成）：普通 void callback、Advanced checked callback 与显式原子批量入口；
4. R4a-4（进行中）：ESP32/UNO R4/Wio 的 WiFi/BLE selector、tag-free facade 与文档已完成；ESP32
   WiFi+BLE selector/QuickStart 已接线，继续完成 clean ZIP、Agent 与真机门禁；
5. R4b-1～R4b-3（已完成软件门禁）：身份/持久域、exact control-plane、ESP32/UNO R4 WiFi-only official composition；
6. R4b-4/R4b-6：EnrollmentTicket、BLE-only composition 和双平台普通 BLE facade 已完成设备软件接线；
7. R4b-5：WiFi+BLE composition 仅保留 ESP32 P6；
8. R4b-7：按产品收口 package/示例，再进入 App/服务器 E2E 与真机门禁。
