# Device V2 平台无关制造凭据与 UNO R4 安装方案

> **已退役（2026-08-23）：**FactoryCredentialBundle 和 UNO R4 installer 已从库与门禁删除。本文只保留历史探索，不属于当前发布物。

> 状态：2026-08-16 已完成 `CloudCredential/BCS2` 软件实现与编译门禁；真机 USB、断电、首登和工位系统仍待验证。

## 1. 冻结决策

服务器只签发平台无关的制造输入，不生成 NVS、Preferences key 或 MCU 专属字段：

```json
{
  "schemaVersion": 1,
  "securityProfile": "education-plain",
  "cloudCredential": {
    "logicalDeviceId": "server-issued-id",
    "generation": 1,
    "credentialVersion": 1,
    "secret": "64-hex-characters"
  },
  "wifi": {
    "ssid": "factory-network",
    "authentication": "wpa2-personal",
    "credential": "controlled-input-only",
    "hidden": false,
    "generation": 1
  }
}
```

同一输入有两个工位执行器：

- ESP32：生成完整 NVS 分区镜像；
- UNO R4 WiFi：生成 BFB2，再由一次性 USB Serial 工厂草图写入 connectivity-module Preferences journal。

普通 Sketch、Arduino 示例、App 和设备日志都不包含 CloudCredential secret 或 WiFi credential。App 不参与制造密钥生成、领取或安装。

## 2. BFB2 二进制合同

BFB2 是固定 228 B、big-endian、zero-padding、CRC32 的平台无关安装记录。它只表示 Active
CloudCredential 和 confirmed WiFi profile，不表示存储布局，也不表示 Ownership Claim 已完成。

| Offset | Size | 内容 |
|---:|---:|---|
| 0 | 4 | `BFB2` |
| 4 | 1 | schema version `1` |
| 5 | 1 | reserved `0` |
| 6 | 2 | total size `228` |
| 8 | 4 | CloudCredential generation |
| 12 | 4 | credential version |
| 16 | 4 | WiFi generation |
| 20 | 1 | WiFi authentication |
| 21 | 1 | bit 0 = hidden；其余必须为 0 |
| 22..24 | 3 | logical ID、SSID、WiFi credential 长度 |
| 25 | 7 | reserved `0` |
| 32 | 32 | CloudCredential secret |
| 64 | 64 | logical device ID + zero padding |
| 128 | 32 | SSID + zero padding |
| 160 | 64 | WiFi credential + zero padding |
| 224 | 4 | 前 224 B CRC32 |

BFB2 不包含 `claimId`、`activationKey`、MAC、NVS namespace 或 Preferences key。`.bfb` 本身是敏感工件，SHA-256 只用于完整性和工位审计，不将明文变成安全存储。

BFB2 也不包含 OwnershipClaimGrant/grant fingerprint。设备虽已具备独立 112 B `BOS2` store，工位不得
自行合成该记录：它只能来自设备验证 server-signed exact grant 后的 Claim transaction。Ownership Claim v1
虽已冻结，是否定义新的 ownership-complete factory bundle 仍需单独冻结 factory authorization/export；当前 BFB2 只是
Cloud/WiFi pre-enrollment probe，运营后台不得据此标记用户 ownership Active。

## 3. UNO R4 安装事务

设备端不解析 JSON。工厂草图只接收一份完整 BFB2，执行严格长度、magic/version、reserved、padding、CRC、UTF-8、WiFi 规则和 credential 状态校验。

安装顺序固定为：

1. 预检现有 CloudCredential/WiFi；不存在或与 bundle 完全相同才继续；
2. 先安装 confirmed WiFi profile；
3. 最后安装 Active CloudCredential，作为首次制造事务的完成标记；
4. 重新读取验证；成功仅返回 BFB2 SHA-256，不返回任何 secret。

同一 BFB2 可在超时、写失败或掉电后安全重试。不同 BFB2 遇到任一已有记录时返回冲突，绝不覆盖。没有 erase、rotate、dump-secret 串口命令；量产后的 credential rotation/reset 必须走正式设备协议。

设备持久化不是 BFB2 原样保存：CloudCredential 编码为固定 116 B `BCS2`（suite、logical ID、generation、version、32 B secret、CRC32），不包含任何 claim/ticket/pending 字段。BFB2 v1 的 byte 5 保留为 0，首版隐含 HMAC-SHA256-32；新增 credential suite 必须升 BFB 版本，不能重解释既有记录。

## 4. 工位命令

生成平台无关工件：

```text
python tools/build_factory_credential_bundle.py \
  --input device.json \
  --output factory/device.bfb
```

工位先单独烧录 `tools/uno-r4-factory-installer/uno-r4-factory-installer.ino`，再安装：

```text
python -m pip install pyserial==3.5
python tools/install_uno_r4_factory_bundle.py \
  --bundle factory/device.bfb \
  --port COM7
```

工具等待 `BLINKER_FACTORY/1 READY 228`，发送 `INSTALL 228` 和 228 B payload，并要求设备返回完全相同的 SHA-256。安装完成后必须烧录对应批次的普通产品固件；普通固件不包含工厂串口入口。

## 5. 当前证据与未完成门禁

- Python 输入/BFB2 round-trip、C++ exact vector 和模拟串口 transcript 通过；
- Core installer 已覆盖首次安装、完全幂等、不同身份拒绝、WiFi selector 写中断后重试、CRC/截断拒绝；
- UNO R4 core 1.5.3 干净编译：66,700 B Flash / 7,924 B static SRAM；
- 真实 USB 自动复位/端口重枚举、Preferences 写中掉电、重复安装、普通固件覆盖、首次 WiFi/cloud authentication 和 24 h 仍需真机执行。

`education-plain` 在 ESP32 NVS 与 UNO R4 Preferences 中都不是机密存储。没有 Secure Boot、Flash/NVS encryption、受保护 secure element 的产品不得改名为 secure profile。

## 6. 服务器与 App 待实现

服务器制造记录至少需要：`logicalDeviceId`、`platformProfile`、`artifactKind`、`artifactSha256`、tool/firmware version、工位、install result、first-auth result、credential revoke state。服务器应先原子持久化唯一 Active credential，再允许一次性受控导出；失败不得把同一 secret 分配给另一物理资产。

App 不接触制造输入或 BFB2。App 只消费服务器确认后的设备所有权/在线状态；全擦除且没有 developer/factory recovery 身份时，显示“需要服务恢复”，不能在本地生成替代 authKey。
