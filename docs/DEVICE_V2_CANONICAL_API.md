# Device V2 canonical API：Property / Action / Event

> 状态：语义合同基线2026-08-18，普通入口校正于2026-10-11；安装使用取[当前QuickStart](DEVICE_V2_USER_API_AND_LLM_QUICKSTART.md)，实际候选/有限验收取[交付计划§10](DEVICE_V2_USABILITY_FOLLOWUP_EXECUTION_PLAN.md#10-核心稳定与最小内部交付)。本文取代旧 Button/Switch/Slider/Text、TypedWidget 与
> WidgetCatalog 设备 API。旧方案从未发布，不提供兼容层。
> WiFi-only connectivity 表面已完成双平台软件接线；身份与注册以
> `DEVICE_V2_DEVICE_KEY_CONTRACT.md` 和 `DEVICE_V2_ESP32_WIFIPROV_PRIORITY_PLAN.md` 为准。
> 普通 Sketch 不传 productId；手动 WiFi 使用无前缀 DeviceKey，ESP32 App 配网优先 WiFiProv Security1。

## 1. 冻结结论

设备固件只描述物理设备能够做什么，不描述 App 应如何显示：

| 设备语义 | 用途 | BBP/2 路径 | 是否进入状态快照 |
|---|---|---|---:|
| `Property<T>` | 可读事实，可选接受期望值 | PATCH Report / COMMAND | 是 |
| `Action<T>` | 一次性调用，不代表持续状态 | COMMAND | 否 |
| `Event<T>` | 设备发生的瞬时事实 | EVENT | 否 |

按钮、开关、滑块、文本、图表、语音卡片都属于 App `PageSchema`。一个 bool Property 可以同时
绑定开关、按钮、自动化和语音；设备不需要知道这些消费者。设备 Manifest 不再携带
WidgetCatalog、组件类型、布局、颜色、标题、JS 或推荐卡片。

## 2. 普通用户目标 API

2026-10-13 可选 Sketch 定时：`#include <BlinkerTimer.h>`、`blinker::Timer<4> timers`，用 `setInterval(ms, callback)`／`setTimeout(ms, callback)` 注册，在原 `loop()` 的 `Blinker.run()` 后调用 `timers.run()`。固定容量、无堆分配，周期错过时跳过、不追赶；取消用 `cancel(handle)`，查询当前有效用 `active(handle)`。这是独立的开机内单调时间工具，不创建 App 任务、不写 Flash、不替代 realtime 租约。明确合同和验收状态见[现有任务与 Sketch timer](DEVICE_V2_TASK_AND_SKETCH_TIMER_EXECUTION_PLAN.md)。

可选授时诊断：`blinker::WallClockSnapshot clock; bool valid = Blinker.timeStatus(clock);` 读取原业务时钟，false 时区分 `Unsynchronized`／`Stale`，true 时可观察来源、样本年龄和不确定度。未启用／未绑定时初始化输出；不新增时钟、协议请求或 facade 字段，不以该接口推断任务授权或来源可信。

可选本机任务（2026-09-24，封闭普通ESP32 WiFi试点）：维护者匹配候选下，在`#include <Blinker.h>`前定义`BLINKER_ENABLE_TASKS 1`，在原`begin(...)`前调用并检查`Blinker.enableTasks(power, ...)`。只选择允许调度的现有标量Property，自动复用业务授时；属性仍须注册写回调并传入`begin()`，存活覆盖整个会话。没有新的begin参数、workspace或用户侧Python步骤；维护者提供容量/policy/签发配置。普通可写不等于允许调度，初始化owner authority仍需App明确操作。默认源码不准入、缺profile编译失败；其他模式不开放。精确合同以[执行器§8.17](DEVICE_V2_TASK_EXECUTOR_DESIGN.md#817-第四十三批最小任务启用入口与维护者profile)为准，不把软件/编译通过冒充该新入口的真机验收。

可选业务 UTC（B3-4c，2026-09-13）：普通 WiFi / 纯 BLE / ESP32 WiFiBLE 在原 `begin(...)` 之前调用 `Blinker.enableTime()`；
使用 `uint64_t utc; if (Blinker.time(utc)) { ... }` 读取 UTC 秒。默认关闭，失败/未同步/过期返回 false 并清空输出。
不新增 begin 参数、组件字段、NTP/连接，也不修改授权/证书时间；明文 MQTT 样本不具备抗篡改保证。
纯 BLE 复用已认证 Control 手机的原 Direct 会话；Observer 不授时，不增加连接或第二次 Hello。
WiFi 云授时与 ESP32 BLE Control 的首次授时/断连保持/普通重启已有真机证据；Observer 与 R4/Wio 当前硬件仍待验。
LAN 共用 responder 已接线，普通 ESP32 + 真实 App 的明文自然刷新/Observer 负例短闭环已过；内部 LAN 尚未产品化、默认开关不变。
WiFiBLE/内部 Hub 已接自身可选授时；Hub 已通过可选模块接南向协商/回应的软件链路，不新增公开 Hub facade；真实子设备授时仍待短真机。
ITimeSync共享扩展接口仍属内部；普通Sketch仅在需要诊断时选择本节新增timeStatus快照，不接触权限/ITimeSync。ESP32 3.3.11 混合/Hub 自身默认/开启单 ZIP 对照已过；
增量分别为程序/RAM +3,076/+120 B 与 +2,992/+112 B，均未刷板，不能替代这两种 composition 的真实授时验证。
Hub 自身 enableTime 不等于所有孩子支持授时；内部两个独立 16 B module 借用同一业务服务，仅实际安装才协商，普通 Sketch 无需配置这些内部接口。
资源、协商、单槽 scope 接替限制与剩余入口统一见[业务授时 §15](DEVICE_V2_TIME_SYNC_IMPLEMENTATION_PLAN.md#15-b3-4c-3唯一南向-executor-的可选授时2026-09-13)。

> Property/Action/Event、model prepare/data-plane start、离线 Property、全局 facade、void/checked
> callback 和普通批量包装已经实现。当前 `Device/Product` 写法只作为 Advanced/内部验证，不再作为
> QuickStart。WiFi-only 已正式绑定 DeviceKey challenge/session，并完成 ESP32/UNO R4 官方包实编；
> ESP32/UNO R4 普通 BLE 已绑定同一 tag-free `Blinker.begin(...)` facade 并完成 official package 实编；
> App/Service 的 Cloud Telemetry 与 Wio 真机链已完成；Cloud/Direct 双 route Core 隔离也已通过 Host 门禁。
> ESP32 WiFiBLE采用WiFiProv Security1 bootstrap＋WiFi Cloud＋SDK NimBLE Direct；当前默认BLE配网，SoftAP由维护者显式选择，公开selector/QuickStart/`begin()`不变。
> 已有有限App/Service与实物结果不等于生产发布或全平台准入。UNO R4/Wio不提供WiFiBLE。普通代码
> 不得调用 `official_detail`。

~~~cpp
#include <Blinker.h>

BLINKER_PROPERTY(power, bool, blinker::readWrite());
BLINKER_PROPERTY(temperature, float, blinker::readOnly());
BLINKER_ACTION(reboot);
BLINKER_EVENT_KEY(alarmEvent, "alarm", uint32_t);

void writePower(bool requested)
{
    digitalWrite(LED_BUILTIN, requested ? HIGH : LOW);
    power.report(digitalRead(LED_BUILTIN) == HIGH); // 只上报硬件确认的事实
}

void doReboot()
{
    // 设置待重启标志；不要在 callback 中阻塞。
}

void setup()
{
    power.onWrite(&writePower);
    reboot.onInvoke(&doReboot);
    Blinker.begin(
        "REPLACE_WITH_DEVICE_KEY",
        "YOUR_WIFI_SSID",
        "YOUR_WIFI_PASSWORD",
        power,
        temperature,
        reboot,
        alarmEvent);
}

void loop()
{
    Blinker.run();
}
~~~

入口头文件在Arduino编译依赖扫描前选择产品：WiFi默认使用`<Blinker.h>`（`<BlinkerWiFi.h>`等价），BLE使用`<BlinkerBLE.h>`，
ESP32 WiFi+BLE 使用 `<BlinkerWiFiBLE.h>`；`begin()` 均不重复填写 mode tag。一个发布 ZIP 包含这些实现，
最终固件只保留被选 composition；门禁以第三方依赖闭包和 ELF defined symbols 验证，而不假设各 Core 都按
archive 成员抽取。UNO R4/Wio 只有相互独立的 WiFi/BLE。普通 Sketch 不填写
productId/Legacy authKey，不构造 platform、transport、buffer 或 MQTT client。手动 WiFi-only 参数顺序固定为
`DeviceKey, SSID, password`；开放网络传空 password。BLE/WiFiBLE 的 App bootstrap intent 不作为普通 Sketch
参数或重启凭据。`begin()` 不阻塞等待网络；实际连接由 `run()` 推进。

各声明的 `*_KEY` 变体仅用于 C++ 名称不能作为稳定 wire key 的情况。
默认应让对象名与 Manifest key 相同，以减少字符串重复并提高 Agent 生成准确率。

高级约束仍属于 Property/Action 本身，而不是 UI：

~~~cpp
BLINKER_PROPERTY(
    temperature,
    float,
    blinker::rangedProperty(
        blinker::PropertyMode::ReadOnly, -40.0, 125.0, 0.1, "°C"));

BLINKER_PROPERTY(
    title,
    blinker::StringView,
    blinker::textProperty(blinker::PropertyMode::ReadWrite, 64));
~~~

## 3. 状态与命令语义

- `report(actual)`：更新设备权威状态、推进 `stateRevision`、发送 PATCH Report。
- `onWrite(requested)`：接收一个写入意图。应用必须先校验并驱动硬件，再显式 `report(actual)`。
- 普通 callback 返回 `void`，必须显式 `report(actual)`；库不得把 callback 返回值隐式变成状态。
- 需要结构化拒绝/错误的高级代码使用 `onWriteChecked/onInvokeChecked` 返回 `Result`；失败时设备状态
  不得被自动修改或伪造为成功。
- `Action::onInvoke()` 不产生持久状态；如动作改变设备事实，应由相关 Property 另行 `report()`。
- `Event::emit()` 不改变状态快照和 `stateRevision`。
- 单个事实使用 `property.report(actual)`；多个相关事实应使用 R4a 普通批量入口
  `Blinker.report(property1, value1, property2, value2, ...)`，形成一个 PATCH Report 和一个
  `stateRevision`。该入口是对现有 Advanced `StateUpdate` 的类型安全薄包装，不使用 heap，也不做
  超时隐式拼包。

远端单字段写入统一走 COMMAND。全局 PATCH Apply 仅用于带 expected revision 的原子期望状态事务；
两者不能用同一个 callback 隐式混合。

## 4. 生命周期与资源

- Spec 是 immutable descriptor，正常由宏生成并放在 static/Flash 生命周期中。
- 运行时对象只保存 Spec、Client 和 callback 指针；不保存 UI DOM 或第二份组件状态。
- `DeviceSchema` 按声明顺序提供稳定 Manifest/Field ID；固件升级若改变顺序或约束，必须提高 Manifest revision。
- `Device::begin()` 自动绑定 Schema 中所有 interaction；同一对象不能绑定到两个 Device。
- callback 可在 `begin()` 前注册。Property `report()` 在模型 prepare 后即可提交本地
  StateStore；Event `emit()` 默认仍要求在线 transport，除非显式启用有界 outbox。
- Small/Standard/Large 是全构建资源配置；普通 Sketch 不手写 buffer 容量。

## 5. 统一平台与传输

Product API 在 ESP32、UNO R4/Renesas、Wio Terminal 等平台保持一致。平台包只实现能力接口：

- WiFi station、TLS HTTP、MQTT client；
- BLE link/GATT、secure record；
- clock、random、credential storage；
- 必要的 crypto provider。

同一个 `Device` 可以同时添加 cloud MQTT 与 local BLE transport。二者共享 Manifest、状态仓和
业务 callback，但各自拥有 HELLO、鉴权、sequence、ID gate、重组和重放窗口。官方 composition
只负责组装，不允许发明 ESP32 专属业务 API。

公开 `Product` 只依赖 `IProductLifecycle`。旧 WiFi+BLE `ProductOrchestrator` 曾验证该 seam 的
延迟绑定、transport 注册和失败回滚，但因依赖已撤回的 Enrollment/1 已退出 public/release。
新 coordinator 必须继续实现同一 seam；平台 package 只负责静态持有依赖并构造对应 lifecycle，
不能绕开 `Product` 暴露新的业务方法。

Core 只依赖 `IMqttClient`、`IHttpClient`、`IBleLink` 等窄接口。默认平台包可以使用
PubSubClient；用户也可注入兼容 adapter。外部库不复制进 Core，不以 ArduinoMqttClient 作为强制依赖。

## 6. PageSchema 边界

App/服务器独立保存：

~~~text
PageSchema
  widget instance: type/layout/style
  binding:
    value  -> Property key
    write  -> writable Property key
    invoke -> Action key
    event  -> Event key
~~~

拖拽编辑器、模板或 Agent 都生成同一种 PageSchema。前端 JS renderer 根据 widget registry 渲染；
JS 代码本身不由设备下发，也不得直接执行设备提供的脚本。服务器可依据类型、access、constraints
生成默认绑定建议，但这属于 server/App 算法，不是 BBP/2 设备资源。

### 6.1 App 如何识别并更新组件

设备不再发送“更新 switch/button/text 组件”，而是发送稳定语义状态。例如：

~~~text
PATCH Report
  revision: 42
  values:
    power: true
    temperature: 24.5
~~~

App 在 Manifest 验证完成后，用 PageSchema 建立反向索引：

~~~text
power       -> ui-power.value
            -> ui-status.value
            -> automation/voice consumers
temperature -> ui-temperature.value
            -> ui-summary.subtitle
~~~

因此：

- 一个 Property 可同时更新一个或多个组件；
- 一个组件也可把不同 slot 绑定到多个 Property；
- 多字段 PATCH 必须整体校验、原子写入 App state store，再安排一次渲染，不能逐字段产生中间画面；
- PageSchema 永久保存 stable key；协商后的 Field ID 先经 session map 还原为 key，不能直接进入 UI；
- 首次连接/重连先完整 staging 同一 revision 的 State snapshot，提交后才能消费增量 PATCH。

用户操作反向进行：组件 `write` slot 找到 writable Property key，App 发送 COMMAND。界面可显示
pending，但不能把请求值直接当成设备事实；设备驱动硬件后 `report(actual)`，App 再以 Report/State
更新权威值。Action 的 `invoke` 和 Event 的 `event` slot 分别进入调用结果与事件流，不进入 Property
状态仓。

### 6.2 内容与样式如何变化

设备仍可改变组件显示的内容，但传递的是语义 Property，而不是组件 JSON。例如设备发布 `mode`、
`faultCode`、`temperature`；PageSchema/WidgetRegistry 决定文字格式、本地化、颜色和 style token。
一个受信的 PageSchema rule 可以把 `faultCode != 0` 映射为 `danger` 样式，但设备不得下发任意 JS、
CSS 或组件私有命令。确实由设备产生的短文本可建模为有最大长度的 Text Property；状态码/枚举优先
于设备侧本地化字符串。

### 6.3 设备上线但页面不存在

这是合法且必须支持的状态，尤其适用于 BLE 配网完成后才创建 logical device 的流程。设备仍只上传
Capability Manifest；App/服务器按以下顺序处理：

1. 用 kind/type/access/constraints 生成确定性通用 PageSchema，保证 Agent 离线时仍能控制；
2. Agent 可结合设备名称、Manifest、ProductProfile 和用户描述生成同结构草稿；
3. 用户确认后独立保存，并记录 `sourceManifestFingerprint`，不修改设备 Manifest；
4. fingerprint 变化只把页面标记为 stale，保留兼容 binding 和用户布局。

Manifest 的 key/单位/约束足以生成可用但朴素的页面；显示名、分组、图标、布局和主题属于
PageSchema/ProductProfile。前端应使用受控 WidgetRegistry 渲染声明式 schema，不执行 Agent 或设备
提供的任意 JS。

手动拖拽、模板和 Agent 不是三套页面格式。它们都只能调用同一个 PageSchema reducer，使用受控
WidgetRegistry 中的组件并产生声明式变更；Agent 输出先校验、预览，再由用户确认。PageSchema 应保存
`sourceManifestFingerprint` 和一份不含状态值的 `sourceCapabilitySummary`，用于固件更新后的差异诊断。

App 每次取得当前 Manifest 后都比较 fingerprint：

1. 相同则直接使用当前页面；
2. 不同则把页面标记为 `stale-compatible` 或 `stale-breaking`，并按新 fingerprint 只提示一次“控制页面需要更新”；
3. 新增 endpoint 放入“可添加组件”面板；删除 endpoint 时保留组件位置但禁用 binding；type/access/constraints
   改变时标记 `needsReview`；不受影响的 binding 继续工作；
4. 用户选择更新后创建新的 PageSchema revision，才把 `sourceManifestFingerprint` 切到当前值。选择“稍后”
   不得清空页面，也不得重复弹窗；可用 `lastPromptedManifestFingerprint` 记录提示状态；
5. 无页面时让用户任选空白拖拽、确定性通用页或 Agent 草稿，三条路径最终仍保存同一 PageSchema。

页面更新提示不能阻止设备上线或完整 State snapshot；但最新状态的 `manifestFingerprint` 必须与当前
Manifest 相同才可进入 UI state store。旧 fingerprint 的状态不得套到新 binding 上。

### 6.4 单个与多个状态上报

~~~cpp
power.report(true); // 一个 Property，一个 revision

// 两个 Property 原子提交为一个 revision。
Blinker.report(
    power, true,
    temperature, 24.5f);
~~~

连续两次 `property.report()` 是两个独立事实和两个 revision；只有显式 `Blinker.report(...)` 才保证
原子批量。`dev_edu` 按超时把多次 print 隐式合并到动态 JSON buffer 的行为不继承，因为它的发送
边界取决于时间、需要字符串查找/重建 JSON，也无法清晰表达原子语义。

## 7. 明确废弃

以下未发布实验从正式主线删除：`Components.*`、`TypedWidget.*`、`WidgetCatalog.*`、
`BLINKER_BUTTON/SWITCH/NUMBER/SLIDER/TEXT`、`EndpointKind::Button/...`、HELLO Catalog key 10/11、
feature bit 8、MessageKind `0x07/0x08`。线值 `0x07/0x08` 和 feature bit 8 保留，BBP/2 v2 不复用。

删除原因不是“暂时没实现”，而是它重复建模 UI、扩大 Flash/SRAM/Manifest，并迫使每种新控件修改
设备库。未来新增 App widget 不应要求升级设备固件。
