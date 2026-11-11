# Device V2 Manifest、Field ID 与本地鉴权契约

> 状态：freeze candidate 0.6，2026-08-15。精确 wire schema 以
> `BBP2_NORMATIVE_WIRE_SPEC.md` 为准。

## Manifest

Manifest 是有序 `FieldSpec[]`：每项是 Property、Action 或 Event，包含完整 stable key、type、
access、session Field ID 和可选 constraints。它不包含 widget、页面、布局、样式或 binding。

~~~text
{0: manifestRevision, 1: fields[]}
field = {0:key, 1:kind, 2:type, 3:access, 4:id, 5..10:constraints}
fingerprint = SHA-256(canonical logical Manifest bytes)
~~~

kind：Property=0、Action=1、Event=2。Property 只允许 Read/Write/Notify；Action 只能 Command；
Event 只能 Event。Action/Event 无状态，但继续占 ordinal/ID，保证后续字段 ID 不因状态过滤而改变。

encoder 必须流式输出并更新 SHA-256，不要求完整 Manifest scratch。Field 顺序、key、kind/type/access、
constraints 或 revision 变化都会改变 fingerprint。constraints 数字固定为 float64 canonical profile。

## 分页与缓存

Manifest 只在完整 field 边界分页。所有页必须有相同 revision、fingerprint、total，cursor 连续推进；
接收端完整 staging、重建 canonical bytes 和验证 SHA-256 后才能原子替换缓存。

StatePage cursor 使用完整 Manifest ordinal。某页可以跨过 Action/Event，因此 values map 数量可小于
`nextCursor-cursor`。所有 StatePage 必须同一 stateRevision，完整后才发布到 UI/cache；冲突从 cursor 0
重启，不能拼接两个 revision。

## Keyed 与 Field ID

Keyed text key 是基础模式，离线 BLE 和首次连接无需 schema 即可解析。Field ID 是 uint16、从 1 开始，
只在当前 session 完成以下 gate 后允许：

1. HELLO 协商 Manifest + EndpointIds；
2. 接收端完整验证当前 revision/fingerprint；
3. 接收端发送精确匹配的 ManifestAccept；
4. 设备为该 session 设置 `idReady`。

Manifest 变化、session 重连或重新鉴权会清除 `idReady`。ID 不得持久化到 PageSchema、数据库 binding
或跨 session cache；持久引用永远使用完整 stable key。

## 本地鉴权

BLE link encryption/bonding 是 radio/link 层；BBP/2 authorization 是 application/session 层。允许本地
COMMAND、PATCH Apply 或敏感 State 前必须建立加密 secure-record session，并在 BBP/2 session 内完成
配置的 auth method。断开、超时、mode 切换和 ownership reset 清除 auth、sequence、replay、ID gate。

Direct BLE 与 cloud MQTT 即使属于同一设备，也不得共享 session-local auth、sequence、Field ID ready
或重放窗口。服务器 cloud session 的授权不能自动授权附近的 BLE central。

App/服务器生产接入见 `APP_SERVER_BBP2_REFACTOR_INPUT.md`。
