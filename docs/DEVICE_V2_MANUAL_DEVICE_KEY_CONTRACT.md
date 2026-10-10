# Device V2 ManualDeviceKey 历史方案（已取代）

> 状态：2026-08-17 已归档，不得作为设备/App/服务器实施依据。
> 权威合同为 `DEVICE_V2_DEVICE_KEY_CONTRACT.md`；ESP32 配网优先级见
> `DEVICE_V2_ESP32_WIFIPROV_PRIORITY_PLAN.md`。

## 取代原因

本文件原方案把 `ManualDeviceKey` 仅作为首次 Claim/全擦恢复授权，设备上线后改用另一把长期
`CloudCredential`，并规定刷新 Key 不影响 Active 设备。该语义与产品最终要求冲突：

> 用户刷新 Key 后，旧 Key 以及旧 Key 曾换取的 MQTT/WebSocket/Session 登录信息都必须立即失效，
> 已建立的旧连接也必须被服务器主动断开。

因此原 `ManualDeviceKey/bdk_ + CloudCredential + RecoveryGrant` 默认模型已经撤回。以下新语义生效：

- 用户概念统一为 `DeviceKey`；
- 32 B CSPRNG secret，外部固定 43 字符无填充 Base64URL；
- 不带 `bdk_`、`bek_`、`brk_` 或其他前缀；
- Education/DIY 默认每次启动使用 DeviceKey challenge-HMAC 换短期 SessionCredential；
- 刷新使 credential version 单调递增，旧 Key、旧 session 和已有连接立即失效，不设 grace；
- App 可长期 reveal/copy 当前 Key；全擦后重新粘贴/配网当前 Key 回到原 logical device；
- `EnrollmentTicket` 只用于短期 App 配网，`ControllerCredential` 只用于 Direct BLE；
- 原 CloudCredential 代码暂留 Enhanced/历史评估，不进入默认 QuickStart/release composition。

历史文档、源码或测试中出现 `ManualDeviceKey` 时只能解释为“待迁移的 DeviceKey 前身”，不能继续实现其
“刷新不掉线”或带前缀格式。具体三端字段、存储、错误、刷新事务和实施批次全部以新权威合同为准。
