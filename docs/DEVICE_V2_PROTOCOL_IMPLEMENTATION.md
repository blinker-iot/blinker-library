# Device V2 协议实现说明

> 当前实现：BBP/2 fixed frame + restricted CBOR，设备 reference；生产 App/服务器尚未接入。

## 分层

`protocol/cbor` 只负责有界 definite-length CBOR；`protocol/bbp2` 负责 frame/message；`model` 负责
Property/Action/Event Manifest 与状态；`runtime` 负责 session gate、路由、ACK/Error；transport 只发送
完整 frame，BLE 在 frame 外分片。

Core 无 Arduino、JSON、MQTT、BLE 或 MCU SDK 依赖。MQTT、BLE、HTTP、storage、crypto 通过 interface
和独立 adapter 注入。

## 已实现

- 10 B BBP/2 header、known flag 校验和完整长度校验；
- restricted CBOR：最短整数/长度、UTF-8、有限浮点、深度/容器/text/bytes 上限；
- HELLO、Manifest 分页、ManifestAccept/ID gate；
- Property state Report/Write、Action COMMAND、EVENT；
- StateRequest/StatePage 与 stateRevision；
- ACK/Error、可靠 outbox、replay window；
- 4 B BLE fragment/reassembly；
- keyed body 与已授权 session-local Field ID body；
- streaming Manifest canonical SHA-256。

## Manifest 与页面边界

~~~text
Device Manifest: Property / Action / Event / type / access / constraints
App PageSchema: widget / layout / style / bindings
~~~

设备不会发送 WidgetCatalog，也不会按 key 后缀推断 UI。App/服务器可依据 Manifest 和产品模板生成
建议，但新增 widget 不应修改设备 protocol/model。

## 状态和路由

Property Report 先提交 bounded state store，再推进 revision 并 fan-out。Property Write 和 Action 由
COMMAND visitor 先做 kind/type/access/constraint 校验。Event 单独广播，不写 state store。

Advanced PATCH Apply 先按 expected revision 和全部字段做 preflight，再调用应用 transaction callback，
成功后原子 commit；不能部分修改。State snapshot 以完整 Manifest ordinal 分页，Action/Event 为空洞。

每条 transport session 独立保存 HELLO、远端 limits、authorization、sequence、replay 和 idReady。
TransportHub 可以让 MQTT 与 BLE 同时在线，但不合并这些 session-local 安全状态。

## 当前保留/未完成

- CRC32C flag 未启用；
- HistoryRequest/HistoryBatch 未冻结；
- production App/server codec/session 不在本仓实现；
- outbound reliable Event 的高级策略按产品需求评审；
- 真机网络、无线并发和长稳不是 host 实现完成即可替代。

精确 wire 见 `BBP2_NORMATIVE_WIRE_SPEC.md`；安全与 ID gate 见
`DEVICE_V2_MANIFEST_ID_AUTH_CONTRACT.md`。
