# Device V2 WiFi平台适配器与能力边界

更新日期：2026-08-19

状态：ESP32与UNO R4的 station 软件实现和编译门禁已建立；真实AP、掉电、异常协处理器、双无线峰值与24 h
测试尚未完成。ESP32 WiFiProv Security1 仍是 internal 优先候选；UNO R4 保持普通 WiFi 与普通 BLE 两条独立
Product；Wio 已有独立普通 WiFi/BLE Product，WiFi 真机主链已通过。详见 `DEVICE_V2_ESP32_WIFIPROV_PRIORITY_PLAN.md`。

## 1. 冻结的分层

Core只保留`IWifiStation`与`WifiConnectionLifecycle`。前者隔离平台SDK，后者统一负责candidate/confirmed journal、连接超时、确认、回滚及重连退避。平台差异分别位于：

- `src/BlinkerV2/arduino/ports/esp32/Esp32WiFi.*`：Arduino ESP32 `WiFi`；
- `src/BlinkerV2/arduino/ports/uno_r4/UnoR4WiFi.*`：UNO R4 WiFi 的 `WiFiS3`。

两者都属于同一个 Arduino 库中的内部 Port，不是独立安装包。新增 MCU 时在统一库中增加新的
`IWifiStation` Port 与平台 Product binding；不得把平台头文件、全局 WiFi 对象或平台重试策略反向引入 Core。
两种适配器对象的 32 位 ABI 均为 8 B，只保存状态和错误，不复制长期凭据。

## 2. 凭据生命周期

`WifiNetworkConfig`提供caller-owned只读view；`IWifiStation`合同要求`start()`返回后不得保留任何view。适配器在`start()`中把SSID和credential复制到33 B与65 B的临时NUL结尾栈buffer，调用平台SDK后立即清零。ESP32 SDK在`WiFi.begin()`返回前已复制配置；WiFiS3在返回前已同步生成并发送`BEGINSTA`命令。因此适配器不保存第二份密码，也不把密码写入日志或Arduino `String`成员。

长期保存只由 `WifiCredentialJournal` 及其平台 storage sink 负责。station adapter 不能持久化 WiFi、CloudCredential 或 MQTT session secret。

## 3. cooperative并不等于绝对无阻塞

`IWifiStation::start()`不得执行association、DHCP或reachability等待循环；`poll()`每次只做一次平台状态事务。SDK本身的有界同步工作仍需如实计入硬件门禁：

- ESP32的`WiFi.begin()`在netif尚未启动时可能等待约1 s，之后association和DHCP异步进行；停止使用`disconnectAsync(false, false)`；
- WiFiS3 1.5.3默认在`begin()`中等待最多10 s。适配器每次先调用`WiFi.setTimeout(0)`，忽略此模式下无意义的即时返回值，再由`poll()`观察连接；
- UNO R4的每个WiFiS3命令仍是同步协处理器事务。协处理器无响应时，单次事务仍可能触发其内部超时。该SDK限制只能通过真机watchdog/loop-latency门禁量化，不能在接口注释中宣称“完全异步”。

## 4. 安全能力矩阵

| 平台 | Open | WPA2-Personal | WPA3-Personal | WPA2/WPA3 transition | 64-hex raw PSK |
|---|---:|---:|---:|---:|---:|
| ESP32 Arduino 3.3.1 | 支持 | 支持 | 支持，设置WPA3最低门槛 | 支持，设置WPA2最低门槛 | 由ESP32 SDK处理 |
| UNO R4 / WiFiS3 1.5.3 | 支持 | 支持 | 明确拒绝 | 明确拒绝 | 明确拒绝 |

UNO R4高层API不能指定WPA3或transition认证模式，且公开合同只支持最长63字符passphrase。适配器返回`UnsupportedFeature`，App应显示“当前硬件不支持该网络安全模式”，不得误报密码错误，也不得静默降级。

ESP32的`Wpa2Wpa3Personal`使用WPA2最低安全门槛，使WPA2与transition AP均可接入；`Wpa3Personal`使用WPA3最低门槛，不降级到WPA2。

## 5. 当前软件证据

本批真实Arduino clean compile结果：

| 目标 | Flash | 静态RAM | 说明 |
|---|---:|---:|---|
| ESP32 WiFi adapter standalone | 904,951 B | 43,352 B | 包含ESP32 WiFi平台栈链接基线 |
| UNO R4 WiFi adapter standalone | 56,524 B | 7,528 B | 包含WiFiS3链接基线 |
| ESP32 Small + compact NimBLE + 1.5 MiB dual OTA product probe | 1,288,083 B | 60,720 B | app slot 1,572,864 B，余量284,781 B（18.11%） |
| UNO R4 product probe | 148,400 B | 14,208 B | 仍有18,560 B供heap/stack；不是运行期峰值 |

Host测试覆盖ESP32/UNO R4的启动、状态、停止、安全门槛及不支持认证模式；实现和ASan共同检查短期buffer没有越界或被对象保留，显式`secureZero`由代码审查与既有Core测试覆盖。MSVC `/W4 /WX`、ASan 16/16及20,000次deterministic fuzz通过。standalone目标已在最终认证能力修正后再次clean compile；完整product数字仍需在正式orchestrator完成后重测，不能把probe数字当发布预算。

Wio Terminal 已接入 `rpcWiFi/rpcBLE` 平台 adapter、存储与独立普通 WiFi/BLE Product。WiFi 已完成真实 SDK
编译以及 DeviceKey、HTTP session、MQTT/BBP2、Gateway 重启恢复真机门禁；BLE 仅完成软件/编译门禁，仍不得
宣称 pairing、断电恢复或长稳已验收。

## 6. 清理结果与下一主线

旧独立 adapter 包和空 provisioning 目录已删除。公开平台/Port 统一位于
`src/BlinkerV2/arduino`；WiFiProv 候选只位于 `tests/internal/arduino`。唯一发布边界由标准库根和
`tools/build_arduino_release.py` 校验，不再维护 per-product manifest。

下一主线不是扩展旧 CloudCredential orchestrator，而是：

`DeviceKey exact contract -> challenge/session/rotate -> ESP32 WiFiProv Security1 probe/adapter -> App/Service E2E`

station adapter 与 WiFi credential journal 保持复用。WiFiProv 只负责 ESP32 的本地 WiFi candidate 和单一
Blinker provisioning bundle；日常登录统一由 DeviceKey challenge 完成。旧 JSON `authKey` 只留 legacy，
不能消费 V2 DeviceKey/session，也不能作为失败 fallback。
