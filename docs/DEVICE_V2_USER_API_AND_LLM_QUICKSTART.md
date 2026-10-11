# Device V2 用户 API 与 LLM QuickStart 正式修订

> 更新：2026-10-11。本文是当前用户/LLM入口，不再派发旧R4a/R4b批次。WiFi默认入口为`Blinker.h`，BLE及ESP32 WiFi+BLE使用明确selector；UNO R4/Wio不提供WiFi+BLE，Wio BLE仍为preview。统一facade、DeviceKey、callback、离线Property和批量入口保留；实际版本与验收范围取[最小交付计划§10](DEVICE_V2_USABILITY_FOLLOWUP_EXECUTION_PLAN.md#10-核心稳定与最小内部交付)。实时/H1有限结果取[实时与历史方案](DEVICE_V2_LIVE_AND_HISTORY_EXECUTION_PLAN.md)，不等于生产发布或接收同事签收。

## 0. 最短安装与接入

维护者提供匹配服务地址和公开验签材料的**一个**`BlinkerDeviceV2-0.1.0.zip`及SHA-256。用户不按MCU/模式另装多个ZIP，不运行Python/Node维护脚本，也不填写composition、workspace或MQTT对象。

1. 安装所选开发板Core及`library.properties`声明的固定依赖；版本与实际编译证据取交付计划，不把一块S3结果当所有ESP32实物通过。
2. Arduino IDE选择“项目→加载库→添加.ZIP库”；手动复制时，一个库根直接含`library.properties`、`src`、`examples`，不要重复嵌套或同时保留旧同名库。CLI本地ZIP安装不会自动补齐依赖。
3. 从“文件→示例→BlinkerDeviceV2”选择下表例程；用户只改接入参数及业务组件。库ZIP不包含完整SDK或本机服务。
4. 上线前请维护者核对profile、App HTTPS origin/信任及服务器下发地址是否在当前网络可达。源码development模板不是任意网络可直接上线的正式配置；不要先清Flash或授权解决地址错误。

| 场景 | 正式例程 / 头文件 | begin与首次接入 |
| --- | --- | --- |
| 手动WiFi | `WiFiQuickStart` / `<Blinker.h>`，`<BlinkerWiFi.h>`等价 | App创建普通设备、复制当前DeviceKey；`begin(key, ssid, password, interactions...)` |
| 纯BLE | `BleQuickStart` / `<BlinkerBLE.h>` | `begin(interactions...)`；App正常发现/认领，不以MAC决定身份或传WiFi Key |
| ESP32 WiFi+BLE | `WiFiBleQuickStart` / `<BlinkerWiFiBLE.h>` | `begin(interactions...)`；默认BLE WiFiProv Security1配网，之后Cloud＋Direct BLE，同一逻辑设备/名字 |
| 诊断/实时/授时 | `CloudSmokeTest` / `<Blinker.h>` | 仍是普通手动WiFi，增加租约采样、可选业务UTC及串口按需诊断 |

四例程只用公开API。控制例程在内存更新并回报`power`、按钮回报`buttonPresses`，不声称已驱动GPIO；实际硬件须在callback中执行后报告事实。BLE实时例程使用`onSample()`，不在loop不停上报。附近≠已鉴权连接，账户MQTT ready≠设备在线，Manifest能力≠App页面布局。

手动WiFi的DeviceKey是长期可重新查看/刷新的无前缀Key，短session到期、普通重启或丢失Sketch不要求重新建页面；完全擦除后从原设备页再复制当前Key。刷新Key不换逻辑ID，但旧Key取得的云凭据也应失效，旧代码需更新Key。Cloud明确plain可用、TLS可选；这不取消身份校验或BLE Direct安全record。

## 1. 决策

普通示例只暴露入口头、interaction、`begin()`、`run()`及必要业务callback。`Device/Product/Client/IProductLifecycle`属于Advanced/内部层，不再为不同MCU复制业务API。

保留的优点：

- `dev_edu`/Blynk 式的一次 `begin()`、持续 `run()` 和对象 callback；
- Device V2 的 Property/Action/Event、BBP/2、固定资源、非阻塞状态机和依赖解耦；
- 同一业务 Sketch 可跨 ESP32、UNO R4/Renesas 等平台复用。

不保留：Widget 设备类、Sketch 中永久 MQTT bearer authKey、JSON、动态 singleton、阻塞式联网、每 MCU
一套产品 API、手工 transport/buffer dependency graph。

## 2. 普通用户入口

普通用户每个 Sketch 只选择一个产品头文件：

~~~cpp
#include <Blinker.h>  // 默认WiFi/cloud；BlinkerWiFi.h等价
// 或 #include <BlinkerBLE.h>  // BLE-only
// ESP32 也可 #include <BlinkerWiFiBLE.h>  // App配网 + cloud + Direct BLE
~~~

头文件是编译期产品选择器，`Blinker.begin()` 中不再重复传模式。单一标准 Arduino ZIP 包含各平台实现，
selector 隔离外部依赖，最终由 section GC 和 ELF defined-symbol 门禁只保留可达的
adapter/native client/static workspace；不按模式或 MCU 拆 ZIP。
`Blinker.h`是普通WiFi默认入口，不要求额外填写模式。ESP32可用`<BlinkerWiFiBLE.h>`选择同一ZIP的混合产品；其有限实物结果与普通发布配置分开，UNO R4/Wio不提供该产品。

WiFi-only基本写法如下；完整普通控制使用`WiFiQuickStart`，诊断使用`CloudSmokeTest`：

~~~cpp
#include <Blinker.h>

BLINKER_PROPERTY(power, bool, blinker::readWrite());
void setPower(bool requested)
{
    digitalWrite(LED_BUILTIN, requested ? HIGH : LOW);
    power.report(digitalRead(LED_BUILTIN) == HIGH);
}

void setup()
{
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    power.onWrite(setPower);

    if (!Blinker.begin(
            "REPLACE_WITH_DEVICE_KEY",
            "YOUR_WIFI_SSID",
            "YOUR_WIFI_PASSWORD",
            power)) {
        Serial.println(Blinker.lastErrorText());
        return;
    }
    power.report(false);
}

void loop() { Blinker.run(); }
~~~

三个字符串只由 WiFi `begin()` 重载接受；顺序固定为 DeviceKey、SSID、password，开放网络传空 password。
它们在`begin()`内同步校验/复制；实际联网、DeviceKey challenge/session、MQTT与BBP/2由`run()`状态机推进，底层SDK/socket仍可能有界等待，不承诺每次调用零阻塞。重启沿用同一DeviceKey获取短期SessionCredential，不保存永久MQTT bearer。
WiFiBLE示例不在Sketch中出现SSID/password/永久authKey/SetupCode；App默认在BLE WiFiProv Security1会话中写入网络及接入bootstrap。SoftAP仅由维护者显式选择，正常重启不依赖App的短期intent；没有已发布的手动混合三字符串重载。
`begin()` 只完成模型和 runtime 的同步初始化，不阻塞等待 WiFi、BLE、HTTP 或 MQTT；实际连接由
`run()` 状态机推进。

## 3. 为什么 interaction 显式传给 begin

不使用隐藏的全局自动注册。`Blinker.begin(..., power, ...)` 显式列出 interaction，可以：

- 由声明顺序确定稳定 Manifest ordinal/Field ID；
- 避免静态初始化顺序问题；
- 不增加第二份全局指针注册表；
- 让声明、绑定及固定容量准入显式发现遗漏、重复或资源超限，不承诺所有问题都在编译期发现；
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

内部模型与通信启动分开，用户不需要分别调用它们：

1. `Client::prepare()`：绑定、seal 和验证 Manifest，初始化 StateStore；允许本地 Property commit；
2. `Client::begin()`：在 CloudCredential 可用后启动 transport/data plane；
3. 离线 Property 只写现有 StateStore，不另建 per-field pending buffer；
4. 联网后以当前 revision 的 snapshot 收敛，不承诺回放每一次中间状态。

facade复用该合同，不再建立另一份离线缓存；历史测试数量不作为当前候选的新验收。

## 6. 分层与依赖所有权

~~~text
Sketch / Agent generated code
  -> Blinker.h | BlinkerBLE.h | BlinkerWiFiBLE.h selector
  -> Property / Action / Event
  -> Device / Product / Client                 (Advanced/internal)
  -> BBP/2 runtime + StateStore + TransportHub (Core)
  -> narrow adapters
       -> PubSubClient / native WiFi / BLE / storage / crypto
~~~

标准Arduino库内部平台composition用静态对象持有native client、adapter、buffer和lifecycle，
也不要求普通用户传入 TCP/MQTT 对象。默认 MQTT adapter 仍是 PubSubClient。高级产品可通过
Advanced API 注入兼容的 MQTT/TCP/BLE/storage/crypto 实现，但不会形成第二套业务 API。

## 7. 跨平台规则

- WiFi业务代码在ESP32、UNO R4/Renesas等平台相同，由内部platform/ports替换实现；
- BLE 业务代码同样跨平台；模式只更换 `<BlinkerWiFi.h>/<BlinkerBLE.h>`，产品包只链接其 lifecycle；
- UNO R4 只支持独立 WiFi 或独立 BLE，不提供 WiFiBLE；
- 不新增 `Esp32Product`、`R4Product` 或 MCU 专属 interaction；
- UNO R4 WiFi/BLE已有各自有限实物结果，存储和链路安全能力独立说明，不伪装成ESP32配网；
- Wio WiFi已有有限实物结果，BLE仍为显式preview，长稳/占用DoS未关闭；API一致不等于全平台production准入。

## 8. LLM 友好门禁

文档和示例只保留一套普通语法，并提供机器可读的 API 摘要与禁止项：

- 生成Property传感器、可写控制、Action、Event；混合例程只选ESP32 WiFiBLE，不虚构其他板的混合入口；
- 禁止生成旧 Widget、JSON、Sketch authKey、`addTransport()`、手工 buffer 或 `blinker::Result` callback；
- interaction 对象名避开 `alarm`、`time`、`index` 等常见 C/SDK 全局符号；需要保持外部 key 时使用
  `*_KEY` 宏，例如对象 `alarmEvent` 对应 stable key `"alarm"`；
- WiFi 与 BLE 示例只允许入口头和必要连接参数不同；不为 UNO R4 生成 WiFi+BLE；
- 从正式例程生成并按所选板型编译；不把发送方旧编译证据当接收方已验收。

## 9. 清理范围

普通入口不保留旧`BLINKER_DEVICE`、Widget/TLV/JSON、伪`BlinkerPlatform`或手工composition示例。只有源码搜索证实无消费者时才删除旧实现；当前安全、任务、Direct及platform模块不因“文件多”而合并或删除。本文移除旧阶段队列，不在用户安装说明中继续扩散内部进度。

## 10. 最短排障与独立接收

排障按阶段：板型/例程/profile → 实际WiFi/IP及服务可达 → 当前Key → session/MQTT → Manifest/fresh State → ACK/真实结果。`begin()`成功只表示同步初始化。必要时`Serial.begin()`后用`Blinker.debug(Serial)`或向CloudSmokeTest串口发送一次`d`；保持串口读取，不反复同步输出或打印Key/密码/bearer。

普通重启、清网络、清本地访问、云端owner移除及取消分享不是同一操作。仅支持的模式可显式使用`resetNetwork()`/`resetAccess()`并检查返回；不假定所有板有统一重置按钮，不以清网络代替解绑，不先清授权解决连接问题。

实时回调只受自己的租约驱动，基础State/自动化/任务独立继续。业务UTC可选，未同步不得伪造时间；任务仍为维护者profile＋服务三侧准入的ESP32普通WiFi owner四槽试点，普通包不含task_profile。自动化保持已准入同owner/tenant/shard两WiFi设备、一个bool/uint32动作，不扩大引擎。H1只保存已采纳State变化，重连baseline不是操作，onSample不是历史；分享读覆盖设备保留窗口，撤权后新查询拒绝并清旧数据，不承诺即时push清屏。详细合同取各自现行方案。

接收同事在包外填写下表；这是模板，不是发送方代签：

| 接收项 | 最小记录 |
| --- | --- |
| 材料 | 接收人/日期、ZIP实际SHA-256、匹配App/Gateway/参与Broker及公开profile标识；同名版本不代替来源 |
| 安装 | 实际板型/Core/固定依赖、一个库的路径、所选正式例程、实际编译结果；缺环境明确未测 |
| 接入控制 | 可达服务/有效信任，正常创建Key或BLE认领，fresh Manifest/State、一次Property/按钮ACK及结果，普通重启/重入恢复；未知Action不盲重放 |
| 选定功能 | 只验所选实时/H1/任务/自动化限定范围，不自动启用LAN、网关、离线存储或Bulk |
| 收尾 | 保留账号/请求/授权；通过/失败/未测、恢复范围、接收人确认，回传脱敏结论而非凭据/Flash/业务库 |

App重建取[源码交接](../../blinker-app/docs/APP_DEVICE_V2_SOURCE_HANDOFF.md)，服务取[现行索引](../../aily-blinker-services/docs/README.md)。不要整体分发现场build。独立接收未完成就保持最后一步待办；生产配置/签名、iOS及Wio BLE正式长稳另行准入，不重开旧R4a/R4b或扩功能代替签收。
