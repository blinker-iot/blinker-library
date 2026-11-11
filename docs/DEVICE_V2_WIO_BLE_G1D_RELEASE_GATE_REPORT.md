# Device V2 Wio BLE G1D 发布门禁记录

> 日期：2026-08-29。目标是关闭 Wio Terminal BLE 的未认证连接占用、认证期限、反复断连和
> 资源复用风险。本文只记录真实完成的门禁；发布前 24 h 已认证 DirectSecure 长稳尚未完成，因此 Wio BLE
> 继续保持 `BLINKER_ENABLE_WIO_BLE_PREVIEW=1`，不进入正式发布矩阵。

## 1. 本批结论

- “连接但不启用 CCCD notification”会被 Wio 的 15 s readiness deadline 主动物理断开；
- “启用 notification 但不完成 BBP/2 认证”会被 Frame/Noise 的 15 s authentication deadline 主动物理断开；
- 32 次连续 `connect -> subscribe -> read RSSI -> disconnect` 全部成功，断开后广播和连接资源可继续复用；
- 测试前后 FreeRTOS heap 水位完全一致，未发现 session、packet queue 或 RTL connection 资源累积泄漏；
- 真机没有暴露需要修改核心状态机的缺陷。本批没有为了测试结果增加新 flag、计数器或平台分支；
- 单连接占用 DoS 与反复重连的短门禁已经关闭。G1D 只剩 24 h 已认证 DirectSecure soak，不得用空闲广播、
  未认证超时循环或 Host mock 替代。
- 30 min 已认证 engineering soak 已完成：真实 owner App 始终运行 Method 2/DirectSecureRecord，三次 Property、
  一次 Action/ACK、一次 App 后台/前台和结束资源水位均通过。它关闭日常开发门禁，但不能替代候选发布的
  24 h soak。

## 2. 固定测试环境

| 项目 | 值 |
|---|---|
| Device | Wio Terminal / SAMD51 |
| BLE NCP | RTL8720DN，Seeed 官方固件 `2.1.3` |
| Arduino core | Seeeduino SAMD `1.8.5` |
| Sketch | `tests/arduino/WioBleReleaseGate` |
| Android central | PGEM10，Capacitor Bluetooth LE 原生插件 |
| BLE locator | `2C:F7:F1:22:8B:7C`，仅为本次测试地址，不是设备身份 |
| GATT packet | 20 B；MTU 23 |
| Build | 186,672 / 507,904 B Flash |
| Runtime baseline | FreeRTOS free 17,120 B；minimum-ever 17,072 B；newlib headroom 109,816 B |

固件由 clean matrix 目标 `wio-terminal-preview-release-gate` 编译并经 BOSSA write/verify 成功后运行，
`Blinker.status()` 持续为 `state=4 / local=1 / error=0`。

## 3. 真机结果

### 3.1 连接但不订阅

使用 Android 原生 `disconnected|<deviceId>` 事件计时，避免把一次 `readRssi()` 最长 5 s 的调用超时误算到
设备释放时间。默认自动化门禁三次从 Android `connect()` 完成到断开事件分别为：

```text
14,461 ms
14,487 ms
14,454 ms
```

它们与设备侧 `connectedAtMillis_` 的 15 s deadline 一致；Android `connect()` promise 比设备回调稍晚完成，
因此从 promise 完成计时会略小于 15 s。

### 3.2 已订阅但未认证

默认自动化门禁三次从 TX notification 建立到原生断开事件分别为：

```text
15,111 ms
15,096 ms
15,124 ms
```

说明 GATT readiness 与 BBP/2 authentication 是两个独立期限：启用通知不能无限占用唯一 central，且没有
通过 Noise/Method 2 的连接拿不到 Manifest、State 或 Command 权限。

### 3.3 反复断连和资源水位

- 32/32 次完整 GATT 建连、订阅、RSSI 操作和主动断开成功；单次约 581～1,069 ms；
- 循环前：`free=17120 / minimum=17072 / headroom=109816`；
- 循环及六次 deadline 释放后：`free=17120 / minimum=17072 / headroom=109816`；
- 设备最终仍为 `state=4 / local=1 / error=0`。

可复现脚本为 `tests/hardware/wio_ble_release_gate.mjs`。它直接复用 App 已安装的 Capacitor BLE 插件，
不修改 App 生产代码，也不进入 Arduino ZIP。默认执行三次 readiness timeout、三次 authentication timeout 和
32 次完整重连。

### 3.4 软件回归

- `tests/architecture/check_device_v2.py`：通过；
- Windows ASan：30/30 通过；
- deterministic fuzz：200,000 cases 通过；
- Host 未认证连接槽位复用：既有 128-cycle 门禁继续通过；
- Wio release gate clean compile：通过。

## 4. 尚未完成的分层长稳门禁

24 h 是候选发布证据，不再作为每批开发或 G2-0A 合同工作的同步前置。相关改动先执行 30～60 min
authenticated engineering soak；固件、App、RTL/SDK 组合冻结后，再执行一次完整 24 h release soak。

正式 soak 必须使用真实 owner App 完成 Method 2，并在 DirectSecureRecord 会话中运行，而不是只保持 BLE
连接。最低验收条件：

1. 连续 24 h 无 MCU/RTL 重启、GATT 卡死、认证降级或明文业务 frame；
2. 每 10 min 至少完成一次 Property write/State response，每 30 min 完成一次 Action/ACK；
3. 中途至少执行 100 次 App 主动断连/重连，重连必须产生新 session、重新 Method 2，旧 secure record 不可重放；
4. 10 min warm-up 后记录 heap/minimum/headroom 基线，结束时不能出现持续下降或不可恢复的固定块泄漏；
5. App background/foreground、App force-stop/relaunch、USB 断开手机调试但保留 BLE、Wio 一次真实断电恢复均通过；
6. 结束后再次运行三种组件命令并读取最终 Manifest/State，不能只以“仍在广播”判定通过。

2026-08-29 已从 Android debug App 安全存储中的既有 refresh token，经本机 Gateway 成功刷新测试会话，
并通过 `/auth/me` 与 `/api/v2/devices` 鉴权验证；未绕过验证码，也未把 token 写入仓库或输出日志。账号阻塞已解除，
但当时尚未运行 authenticated soak，因此不能据此提升发布状态。

### 4.1 30 min authenticated engineering soak（2026-08-30）

- 最终无临时 trace 固件为 188,280 B；同一固件真实重刷期间 App 在详情页观察到 `connecting`，约 3.3 s 后
  自动恢复 `蓝牙已连接`，并重新得到 `Manifest 1 / Layout 1 / State 1`。没有返回设备列表、重复 enrollment
  或使用 MAC 作为身份；
- 01:43:04～02:13:53 连续执行 30 次一分钟采样，全部保持 authenticated DirectSecure 会话和有效
  Manifest/Layout/State；State revision 随合法命令推进，不要求固定为 1；
- 第 10/20/30 min 分别完成 `power=true/false/true` 的 Property/State/ACK；第 30 min 完成一次
  `buttonPresses=1` Action/ACK；
- 第 15 min 将 App 切至后台 5 s 后恢复前台，原详情页恢复且仍可控制；
- warm-up 基线与结束水位完全一致：FreeRTOS `free=17120 / minimum=17072`，newlib headroom `109224 B`；
- App 有界前台重连使用 1,000/2,500/5,000 ms 三次退避预算；页面离开、App 非前台、会话恢复或组件销毁
  都会取消后续 timer。该策略复用既有 Presence 扫描、Method 2 与 session，不增加设备广播、注册或协议状态。

## 5. 后续顺序

1. 30 min authenticated engineering soak 已完成；日常硬件收口复用该门禁，不重复扩散新状态机；
2. 候选发布前完成 24 h authenticated release soak，并把原始时间线、heap 水位和失败恢复记录追加到本文；
3. G2-0A exact contracts 立即开始，不等待 24 h；G2 硬件发布声明仍受本门禁约束；
4. 不提前实现 Mesh、roaming、多 child scheduler，也不再回到 RTL bonding fork 扩散问题。
