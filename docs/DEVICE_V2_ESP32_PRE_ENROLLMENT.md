# ESP32 WiFi 制造期 pre-enrollment

> **已退役（2026-08-23）：**相关生成器、fixture 与 NVS CI 依赖已删除。当前 Education WiFi 只使用可查看/轮换的 DeviceKey；制造方案仅作为未来文档议题。

> 状态：2026-08-16 已迁移到 `CloudCredential/BCS2` 并通过 C++/Python exact 门禁；真机烧录和首登仍待验证。

ESP32 不再拥有单独的服务器输入 schema。它与 UNO R4 共用
`DEVICE_V2_FACTORY_CREDENTIAL_BUNDLE_AND_UNO_R4_INSTALLER.md` 定义的
`securityProfile: education-plain` 制造输入；ESP32 工位执行器只负责把该逻辑记录转换成目标分区的 NVS image。

## NVS 布局

| namespace/key | 大小 | 语义 |
|---|---:|---|
| `bl_v2_cloud/credential` | 116 B | Active CloudCredential (`BCS2`) |
| `bl_v2_net/wifi_sel` | 16 B | active slot 0 + WiFi generation |
| `bl_v2_net/wifi_0` | 116 B | confirmed WiFi profile |

`wifi_1` 留给设备上线后的 crash-recoverable rotation。Cloud record 根本不定义 enrollment-only `claimId/activationKey`，首次认领事务不能借用该长期记录保存 pending 状态。

## 构建

```text
python -m venv .factory-nvs
.factory-nvs/Scripts/python -m pip install esp-idf-nvs-partition-gen==0.3.0
python tools/build_esp32_pre_enrollment.py \
  --input device.json \
  --output factory/device.bin \
  --nvs-python .factory-nvs/Scripts/python
```

工具严格验证平台无关输入和三份持久化记录，再调用固定版本的 Espressif NVS generator。它不生成 secret、不调用 `esptool`、不写设备。输出 receipt 包含 `platformProfile: esp32-wifi`、`storageProfile: education-plain-nvs`、分区信息、entry size 和镜像 SHA-256，不包含 WiFi/CloudCredential secret。

默认 offset `0x9000`、size `0x5000` 只适用于已核对的对应 partition table。整分区镜像只能用于 fresh-device 制造，禁止覆盖已出厂设备的整个 NVS；现场更新必须走正式 credential/WiFi transaction。

`education-plain-nvs` 可被物理读取，不是 secure profile。真实发布仍需验证 partition offset、烧录/断电/重复烧录、首次 WiFi/MQTT、全擦除和 credential revocation。
