# Device V2 Arduino 发布配置合同

> 状态：2026-08-23，C4 第一片已实施。本文描述发布工程配置，不增加 Sketch 参数，也不改变 BBP/2、DeviceKey 或组件 API。

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
- profile 只含公开配置和公钥；签名私钥永不进入设备仓、profile 或 ZIP，应保留在服务器 KMS/密钥管理边界。
- profile 不含 DeviceKey、WiFi 密码或用户凭据。

## 3. 源码与产物边界

仓库 `OfficialConfig.h` 和两个 `*ServerKeys.h` 是可编译 development 模板。发布器先按当前标准库白名单复制，再只在 staged copy
中生成这三个头；工作树不被修改。`tools`、profile 和测试不会进入用户 ZIP。

本地 Broker/Gateway 联调同样使用 `build_arduino_release.py --profile <development-profile.json> --allow-development-profile`。
维护者在profile填写实际host/port、安全选项和匹配的公开签名材料；用户Key/SSID/密码只写私有Sketch，不写入库包。
2026-09-24已移除失配的`build_device_v2_interop_package.py`字符串替换/凭据注入入口，不保留第二套配置生成逻辑。
任务封闭试点可选`task_profile: "esp32-wifi-t2-lab-v1"`，production拒绝；交接方式见[任务试点说明](DEVICE_V2_TASK_PILOT_HANDOFF.md)。

## 4. 当前证据与剩余阻断

- 缺 profile 和未授权 development profile 均已做反向失败测试。
- development profile 两次构建得到字节一致的 256 文件 ZIP，SHA-256
  `1f2de604e6095667cd1c6e94a88969fd699460e641b13e01978ad66a432e5241`。
- profile 化 ZIP 的 UNO R4 WiFi clean compile、依赖/ELF/provenance 门禁通过：`134,392 B Flash / 12,956 B globals`。
- production parser 已覆盖 placeholder host、两组测试 key、TLS 无 CA 拒绝和 1～4 key ring 结构。

C4 尚不能标记全部完成：真实 production host/port、P-256/Ed25519 公钥 id、plain/TLS 选择与 CA 尚未由部署环境提供；取得这些
公开值后生成 production profile，再对该精确 ZIP重跑八目标门禁。不能用 development profile 或仓库模板替代该准入。

服务仓部署文档已表明设备 Gateway 域名为 `iot.yiyu.pro`，`api.yiyu.pro` 是 Aily 账户 API，二者不得混用。但当前服务代码
尚无 `BleEnrollmentGrant` 生产签发器和对应 P-256/Ed25519 key ring；因此不能只填入域名并继续沿用测试公钥。服务器完成
双算法签发/KMS 公钥导出后，才能冻结首个 production profile。
