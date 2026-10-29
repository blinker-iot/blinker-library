# 公开发布配置

当前只维护一个可用于云端内测的公开 profile：[`cloud-beta.json`](cloud-beta.json)。
它是同一库的受限配置，不是全部平台／连接模式已经开放的声明。首发支持冻结与证据见
[发布方案 R3](../docs/DEVICE_V2_LIBRARY_RELEASE_AND_AILY_BUILDER_PLAN.md#9-r3-首发边界与公开配置冻结)。
用户安装步骤、当前目标依赖和旧库冲突处理见[安装说明](../docs/DEVICE_V2_ESP32_CLOUD_BETA_INSTALLATION.md)。

| 配置项 | 冻结值／边界 |
| --- | --- |
| 库／固件版本 | `0.1.0`／`device-v2-0.1.0` |
| 产品配置 | `esp32-cloud-wifi-v1`，ESP32 手动 WiFi，普通持久任务 0 槽 |
| 安装编译目标 | `esp32-wifi`、`esp32-cloud-smoke`；沿用公开云端 D1b 的官方 Arduino-ESP32 3.3.11，不升级 SDK |
| 控制面 | `https://iot.yiyu.pro:443`；不要改为 Aily Auth 域名 |
| MQTT | TLS；地址仍取自经过认证的 CloudSession，当前为 `mqtt.yiyu.pro:8883` |
| TLS 信任 | Sectigo E46 根 CA；两个 PEM 使用同一 DER 摘要，见下文 |
| 业务签名 ring | P-256／Ed25519 均为空，按现有 guard 禁入 BLE、WiFi+BLE、配网、网关、LAN 和任务 |
| 本地存储 | `allow_plain_storage: true`；不等于链路明文或设备 Flash 加密保证 |

根 CA DER SHA-256：`c90f26f0fb1b4018b22227519b5ca2b53e2ca5b3be5cf18efe1bef47380c5383`。
`channel: production` 表示使用严格发布配置校验，不代表该内测环境已经完成正式发行签收。

SDK 按已验环境分别固定：普通 Plain WiFi 的代表基线是 3.3.6，公开云端 TLS 两例程的既有基线是 3.3.11。
R4 对照原 `build.options.json` 和冻结输入纠正了 R3 将前者套用于后者的说明；不因此重编已通过目标，
也不将某一 profile 的编译结果外推到另一个 SDK。

## 使用现有发布入口

使用全新／空的输出目录，源码默认头不被改写：

```sh
python -B tools/build_arduino_release.py --profile profiles/cloud-beta.json --stage-dir build/cloud-beta/stage --output-dir build/cloud-beta/artifacts
```

用户只安装这个单一 ZIP，填写对应环境的 DeviceKey、SSID 和密码。不向 profile 添加用户数据、私钥或网络密钥。
发布器继续只派生 `OfficialConfig.h`、`Esp32ServerKeys.h`、`PortableServerKeys.h` 和内容身份头
`LibraryBuildIdentity.h`，不建立第二套生成器。当前 Aily 代表目标的 development 包不是本 profile 的最终安装证明；
R4 已完成公开 profile 的隔离安装、CLI 原产物复用及 Aily 两目标完整链接。
R5 的安装文档更新会改变 ZIP 的 README 和摘要，不改变编译输入；精确候选版本／校验值见
[发布方案 §11.5](../docs/DEVICE_V2_LIBRARY_RELEASE_AND_AILY_BUILDER_PLAN.md#115-精确候选与校验值)。

## 候选 CI 与正式发行边界

`.github/workflows/device-v2-cloud-beta.yml` 仅允许手动触发，复用本发布器和
`verify_installed_release.py`，安装原 SDK 3.3.11，显式验证两个云端 WiFi 目标后，
才保留一个 ZIP 与它自己的 `SHA256SUMS`。它不部署、不刷板、不发布 GitHub Release，也不需要账号或设备凭据。
每个 CI 候选以提交 SHA 命名；未提交的本机工作树、不同检出字节或其他构建不能共用旧摘要。

原 `device-v2-ci.yml` 是 development／多平台质量门禁，不作为这个受限云端候选的发行证明。
新工作流只覆盖 Arduino CLI；Aily 继续引用 R4 冻结源码的本机证据，不从未包含这些修正的远端分支或任意最新版下载 builder。
配置写入不等于 CI 已执行通过；本批尚未提交推送或触发远端 CI。

## 多平台／BLE 配置的后续条件

普通单库仍保留六种模式、七个编译目标，不将此受限 profile 冒充多平台包。完整普通 profile
复用 schema 1，省略受限 `product_profile`，由所选环境提供实际控制面／传输选择和两组业务公钥：

- ESP32 BLE／WiFi+BLE：`EcdsaP256Sha256Raw`（线协议值 2），65 B uncompressed SEC1。
- R4 BLE：`Ed25519`（线协议值 1），32 B 公钥。Wio BLE 仍是预览，不因有此公钥自动准入。
- 每组首项必须是服务端当前可签发的 keyId；认领使用 `serverSigningKeys[0].keyId`，不能按 id 排序替代发布顺序。
  轮换验证 ring 可保留旧 key，但需同时满足每个消费者的签发合同。
- CA、MQTT JWT／Auth 密钥和业务 Grant 公钥用途不同，禁止互相代替。

2026-10-27 云端只读核对仅发现 P-256 keyId 1 的业务签发材料，现用于逐设备准入的任务链路；
未配置 Ed25519 业务签发，公网未开放 BLE／Direct 认领入口。因此本批不新建“完整生产 profile”，
不把任务公钥注入零槽 `cloud-beta.json`，也不通过测试 key 或假 key 补齐空缺。完整公开公钥和核对摘要保存在
[发布配置合同](../docs/DEVICE_V2_RELEASE_PROFILE.md#5-r3-公开环境核对2026-10-27)。
