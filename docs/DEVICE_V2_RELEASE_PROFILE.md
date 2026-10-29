# Device V2 Arduino 发布配置合同

> 合同基线：2026-08-23；当前公开环境核对更新于2026-10-27。首发边界以[发布方案 R3](DEVICE_V2_LIBRARY_RELEASE_AND_AILY_BUILDER_PLAN.md#9-r3-首发边界与公开配置冻结)为准，公开配置入口见[profiles](../profiles/README.md)。不增加Sketch参数或改变BBP/2、DeviceKey、组件API，不重开整组C4维护。

## 1. 结论

用户安装的仍是一个 `BlinkerDeviceV2` ZIP。控制面 host、HTTP/MQTT 明文或 TLS、CA、固件版本以及服务器验签公钥由
发布工程在生成 ZIP 时一次注入；用户代码仍只填写 DeviceKey、SSID、password 和组件。

正式归档必须执行：

```powershell
python tools/build_arduino_release.py `
  --stage-dir D:\temp\blinker-release `
  --output-dir D:\temp\blinker-artifacts `
  --profile D:\secure-build-input\production-profile.json
```

缺少 `--profile`、profile 为 `development`、host 为 `.example/.invalid`、使用仓库测试公钥，或选择 TLS 却未提供对应 PEM CA
时，发布器直接失败。`--allow-development-profile` 供 CI 或明确的本机封闭试点使用；不得将 development 候选当作正式生产发布。

这里的 fail-closed 不是强制 TLS。资源受限设备的 production profile 可以选择 Plain HTTP/MQTT；TLS 可分别开启，开启后不允许
无 CA 或静默降级。`allow_plain_storage` 同样由 profile 明确选择。

## 2. Profile schema 1

```json
{
  "schema": 1,
  "channel": "production",
  "firmware_version": "device-v2-0.1.0",
  "control": {
    "host": "iot.example.org",
    "port": 80,
    "cloud_session_tls": false,
    "mqtt_tls": false,
    "cloud_session_ca_pem": "",
    "mqtt_ca_pem": ""
  },
  "allow_plain_storage": true,
  "server_keys": {
    "p256": [{"id": 2, "public_key": "04...65-byte-SEC1-hex..."}],
    "ed25519": [{"id": 2, "public_key": "...32-byte-hex..."}]
  }
}
```

- `p256` 用于 ESP32 mbedTLS 平台，公钥为 65 B uncompressed SEC1；`ed25519` 用于 portable Crypto 平台，为 32 B。
- 每种算法允许 1～4 个唯一 `uint32` key id，便于先加入新 key、服务器切换签发、再删除旧 key。
- 以上是完整普通 profile；显式受限 `esp32-cloud-wifi-v1` 的双空 ring 是例外，不能去掉产品标记后冒充完整 profile。认领使用各组首项 keyId，发布顺序必须对应当前可签发 key，不能按数字重新排序。
- profile 只含公开配置和公钥；签名私钥永不进入设备仓、profile 或 ZIP，应保留在服务器 KMS/密钥管理边界。
- profile 不含 DeviceKey、WiFi 密码或用户凭据。

## 3. 源码与产物边界

仓库 `OfficialConfig.h` 和两个 `*ServerKeys.h` 是可编译 development 模板。发布器先按当前标准库白名单复制，再只在 staged copy
中生成这三个头及内容身份头 `LibraryBuildIdentity.h`；工作树不被修改。`tools`、profile 和测试不会进入用户 ZIP。

本地 Broker/Gateway 联调同样使用 `build_arduino_release.py --profile <development-profile.json> --allow-development-profile`。
维护者在profile填写实际host/port、安全选项和匹配的公开签名材料；用户Key/SSID/密码只写私有Sketch，不写入库包。
2026-09-24已移除失配的`build_device_v2_interop_package.py`字符串替换/凭据注入入口，不保留第二套配置生成逻辑。
任务封闭试点可选`task_profile: "esp32-wifi-t2-lab-v1"`，production拒绝；交接方式见[任务试点说明](DEVICE_V2_TASK_PILOT_HANDOFF.md)。

## 4. 当前证据与剩余阻断

以下ZIP摘要、旧416文件白名单和编译占用是原批次的历史证据，不是当前源码的安装证明。含N3/整数step的内部ZIP及隔离七目标结果已记录于[执行计划§10.6](DEVICE_V2_USABILITY_FOLLOWUP_EXECUTION_PLAN.md#106-n3与整数step单库候选冻结及安装编译2026-10-11)，后续README/BLE实时例程交付修订取§10.7。最新 Aily 代表目标和443文件 development 包见[发布方案§7～8](DEVICE_V2_LIBRARY_RELEASE_AND_AILY_BUILDER_PLAN.md#8-r2b-通用入口与片段兼容收口)。精确摘要与每个目标的来源分别记录，不以旧包证明新例程，也不为文档收口更换SDK/依赖或生产信任材料。

- 缺 profile 和未授权 development profile 均已做反向失败测试。
- development profile 两次构建得到字节一致的 256 文件 ZIP，SHA-256
  `1f2de604e6095667cd1c6e94a88969fd699460e641b13e01978ad66a432e5241`。
- profile 化 ZIP 的 UNO R4 WiFi clean compile、依赖/ELF/provenance 门禁通过：`134,392 B Flash / 12,956 B globals`。
- production parser 已覆盖 placeholder host、两组测试 key、TLS 无 CA 拒绝和 1～4 key ring 结构。

完整多平台公开发行尚不能标记完成：当前云端 host／TLS CA 已核对，且已有 ESP32 WiFi 受限 profile；但仍缺可用的 Ed25519 业务签发及 BLE 公网入口，不能据此生成多平台完整 profile。取得对应环境的公开材料与开放范围后，再对精确ZIP执行所声明的安装门禁。当前`verify_installed_release.py`是七个普通目标，任务另显式opt-in；不是八个默认目标。不能用development profile或仓库模板替代生产准入，也不能继承旧ZIP的字节证明。

设备 Gateway 域名 `iot.yiyu.pro` 与当前统一测试 Auth `api-test.yysc.tech` 不得混用。当前服务代码
已实现`ble-enrollment-signer.js`，`index.js`也可从配置文件装配P-256/Ed25519签发ring；“源码没有签发器”是早期缺口，不再作为当前事实。存在签发代码不等于所选生产环境已配置KMS、密钥/公开ring或完成验收；冻结production profile前须核对实际部署及公开材料，不能只填域名并继续沿用测试公钥。内部development候选按明确本机profile验收，无须以生产KMS完成作为前置。

## 5 R3 公开环境核对（2026-10-27）

本轮仅执行既有云端的只读观测与两个 TLS 握手；未部署、签发业务请求、读取账号／设备凭据或改变逐设备名单。
观测时间为 `2026-10-27T12:45:01Z`，Gateway 健康，镜像为
`sha256:947b0746fe2b00c6917d578ecb698092c813fa377c26c58714de08f3fa9361b4`。
原始脱敏结果在忽略目录 `build/library-release-r3-20261027/runtime-public.json`。

| 核对项 | 实际结果 |
| --- | --- |
| API／MQTT TLS | `iot.yiyu.pro:443`、`mqtt.yiyu.pro:8883` 均用公开 profile 的 E46 CA 通过主机名及证书链验证；仅 TLS 握手，不登录或发布 MQTT |
| 普通 WiFi 鉴权 | 复用现有 DeviceKey challenge-HMAC／CloudSession；CA 是传输信任，不是 Grant 公钥 |
| 已配置业务签发 | `EcdsaP256Sha256Raw`，协议值 2，keyId 1；从运行 Gateway 所用私钥在容器内派生公开值，未导出私钥 |
| 公钥格式／摘要 | 65 B uncompressed SEC1；SPKI DER SHA-256 `57f69d14769fd9a099c1d4cc11fac64f8389ee975200f9fa65f53e18b58a6025`，与 D4 记录一致 |
| Ed25519 业务签发 | 未配置；不能借用 MQTT JWT／Auth 的 Ed25519 材料 |
| 任务准入 | 仅 1 个既有设备，Ed25519 子集 0；名单未读取到公开文档，也未调整 |
| 公网 BLE／Direct／配网／LAN | API allowlist 未开放对应入口；LocalAccess 关闭，Plain 云接入关闭。签发器可构造不等于产品入口已开放 |

当前 P-256 公开值（仅记录核对结果，不作为零槽 profile 的新增能力）：

```text
algorithm = EcdsaP256Sha256Raw (2)
keyId = 1
publicKeySec1Hex = 04c264243dbe1f8e767727df5302f9432d979d56455e83490f074ac18cb6f052a3cd72f0b239c75057704c8f8bbbde93b118e1f6883d703bbcf134b85f0330443d
```

现有 Gateway 代码从同一业务 ring 装配 enrollment／Controller／Task signer，默认优先 P-256；
这不是每种业务已经开放的证明。当前公网用途是受逐设备准入控制的 ESP32 任务链路。
因此 `cloud-beta.json` 保留双空 ring 与零槽，不创建缺 Ed25519 的“完整普通 profile”，也不改变 D4 scope。
后续完整候选需要实际开放范围、两种算法的当前可签发 keyId／公开值与 ring 首项共同匹配，再单独执行匹配包验收。
