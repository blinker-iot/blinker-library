# ESP32 云端受限候选安装说明

本文只适用于发布工程用 `profiles/cloud-beta.json` 生成的 `BlinkerDeviceV2` 单一库 ZIP。
它是当前内测候选，不是六种模式的完整云端发行，也不是 Arduino Library Manager 已上线的版本。

## 1 支持范围与安装材料

| 项目 | 当前固定值 |
| --- | --- |
| 库版本／固件版本 | `0.1.0`／`device-v2-0.1.0` |
| 产品配置 | `esp32-cloud-wifi-v1` |
| 代表板型 | ESP32 Dev Module，FQBN `esp32:esp32:esp32` |
| 原 SDK | Espressif 官方 Arduino-ESP32 **3.3.11** |
| 连接模式 | 手动填写 WiFi，HTTPS／MQTTS 云接入 |
| 控制面／MQTT | `iot.yiyu.pro:443`；MQTT 地址由认证后的 CloudSession 返回，当前为 `mqtt.yiyu.pro:8883` |
| 普通持久任务容量 | **0 槽**；不包含 App 管理的四槽持久任务 |
| 正常例程 | `WiFiQuickStart`、`CloudSmokeTest` |

本候选不开放 BLE、WiFi+BLE 配网、LAN、网关或 R4／Wio 云端模式；不要通过修改宏或绕过编译 guard 启用。
库中保留的其他平台实现及例程不代表本 profile 已开放这些能力。Sketch RAM 内 Timer 与持久任务也不是同一种功能。

应取得同一次候选构建提供的：

- `BlinkerDeviceV2-0.1.0.zip`；
- 配套 `SHA256SUMS`。

GitHub 的“Download ZIP”、裸克隆仓库或复制 `src/` **不能替代这个 profile 化 ZIP**：
仓库默认配置头是 development 模板，发布器才会在副本中注入公开服务配置、TLS CA、空业务签名 ring 及内容身份头。
不要手工修改 SDK、库内部配置头或把私有联调 stage 当安装包。

## 2 先核对版本和校验值

在放置 ZIP 和 `SHA256SUMS` 的目录校验。PowerShell 示例：

```powershell
Get-Content -LiteralPath .\SHA256SUMS
Get-FileHash -LiteralPath .\BlinkerDeviceV2-0.1.0.zip -Algorithm SHA256
```

将 `Get-FileHash` 的 SHA-256 与 `SHA256SUMS` 中同名文件的 64 位摘要逐字比较，大小写不影响比较；
不一致时停止安装，核对文件是否来自同一批次，不自动重新下载或忽略错误。
Linux／支持 `sha256sum` 的环境可直接执行：

```sh
sha256sum -c SHA256SUMS
```

精确候选摘要以实际同批 `SHA256SUMS` 为准；[发布方案 §11.5](DEVICE_V2_LIBRARY_RELEASE_AND_AILY_BUILDER_PLAN.md#115-精确候选与校验值)只记录R5历史包。
之后的定位源码与说明改变下一候选字节，不能沿用R4／R5旧摘要；版本号相同不代表 ZIP 字节相同。
校验文件用于核对下载完整性，不替代可信下载来源或发布签名。

## 3 安装 SDK 与当前目标依赖

在 Arduino IDE 的开发板管理器选择 Espressif 官方 `esp32` **3.3.11**，开发板选择 **ESP32 Dev Module**。
此处沿用云端候选已验 SDK，不使用普通 Plain WiFi 的 3.3.6 结果代替，也不要求升级到其他版本。

通过库管理器安装 **PubSubClient 2.8**。本 ESP32 云端 WiFi 目标的实际依赖为：

| 来源 | 库 |
| --- | --- |
| 手工安装并固定版本 | PubSubClient 2.8 |
| 官方 ESP32 SDK 自带 | WiFi、Network、NetworkClientSecure |
| 本候选 ZIP | BlinkerDeviceV2 0.1.0 |

不要另外安装同名 WiFi／NetworkClientSecure 第三方库来覆盖 SDK；当前目标也不需要 NimBLE-Arduino、ArduinoBLE、Crypto 或 Seeed RPC 库。

统一库的 `library.properties` 必须使用 Arduino 支持的扁平依赖字段，因而仍声明所有平台共用的六项依赖：
PubSubClient 2.8、ArduinoBLE 2.0.2、Crypto 0.4.0、Seeed Arduino rpcWiFi 1.1.0、rpcBLE 1.0.0、rpcUnified 2.1.4。
Library Manager 可解析整套声明，不按当前 Sketch 自动缩成一项；这不代表这些平台已被本 profile 开放。
手工添加 ZIP 不应被视为依赖已自动装好：已验的 Arduino CLI `--zip-path` 不会解析并安装 `depends`，
因此本候选安装说明显式要求先安装当前目标需要的 PubSubClient。

## 4 避免旧库重名冲突

安装前检查当前 sketchbook 的 `libraries/` 中是否已有旧 Blinker、V1、development 副本，或另一份含 `Blinker.h` 的手工安装库。
同一个版本名可能仍是不同 profile，不能仅凭 IDE 显示 `0.1.0` 判定已匹配。

若发现重复库，将不用于本项目的整个库目录移动到 sketchbook `libraries/` **以外**保留；
不要只在 `libraries/` 里面改成 `Blinker-old`、`backup` 等名称，Arduino 仍可能继续扫描。
不要批量删除库、用户 Sketch、私有凭据或设备授权，也不需要为安装此包清除 Flash、解绑设备或刷新 DeviceKey。

在 IDE 开启编译详细输出，确认实际使用的是本次安装的 `BlinkerDeviceV2` 路径。
若出现 “Multiple libraries were found for Blinker.h”，先核对 “Used”／“Not used” 路径，再处理冲突；
不要通过向 Sketch 添加库内部 include、拼接旧 Core／Port 包或全局增加头文件路径掩盖冲突。

## 5 安装单一库并编译例程

Arduino IDE 选择“项目／Sketch → 导入库／Include Library → 添加 .ZIP 库／Add .ZIP Library”，安装已校验的 ZIP。
只安装这一个库包，不另装 Blinker Core、Port 或按平台拆分的包。

CLI 的对应操作可使用已有 Arduino CLI；示例沿用本批已验的 1.5.1：

```sh
arduino-cli lib install "PubSubClient@2.8"
arduino-cli lib install --zip-path BlinkerDeviceV2-0.1.0.zip
```

CLI 本地 ZIP 安装需要显式允许本地不受信任包安装；若当前配置拒绝，可仅对本次进程设置
`ARDUINO_LIBRARY_ENABLE_UNSAFE_INSTALL=true`，完成后结束该进程。先校验来源与 SHA，不因此关闭其他校验。
本说明不要求重装已匹配的 SDK 或依赖。

从“文件 → 示例 → BlinkerDeviceV2”打开 `WiFiQuickStart`，只替换三个用户值：

- 对应当前内测环境设备的 DeviceKey；
- 2.4 GHz WiFi SSID；
- WiFi 密码。

不要将用户值填入 profile、库源码或 Git。设备 Gateway 与统一账户 Auth 是不同入口，
不需要把 `Blinker.begin()` 改为 Auth 域名，也不需要用户复制服务器 CA 或业务公钥。

选择正确板型和串口后编译、上传。原例程应持续调用 `Blinker.run()`；
避免加入长时间阻塞等待、无限打印或用 `delay()` 替代主循环推进。
控制确认应以真实 ACK 和后续新鲜 State 为准，单次发布成功不等于设备已经执行。

`CloudSmokeTest` 复用同一安装和连接方式，包含授时、控制及租约驱动的实时采样；
实时数据仅在授权租约有效且到期时采样。串口 `d` 可单次读取诊断和业务 UTC；保持串口读取，
不要把诊断改成无界周期打印。编译或安装成功不等于完成新一轮硬件／云端验收。

## 6 Aily Builder 与 CI 边界

Aily Builder 对本精确公开 profile 的两个目标已按 R4 冻结源码完成编译与完整链接。
这不是“远端 main 已经包含修正”或“任意发行版 builder 均已兼容”的声明；
具体 builder 输入和产物证据见[发布方案 §10](DEVICE_V2_LIBRARY_RELEASE_AND_AILY_BUILDER_PLAN.md#10-r4-当前公开候选安装与编译)。
不要为兼容问题手工向 Sketch 补实现片段或安装另一份 Blinker 库。

匹配扫描／缓存修复已交付至 `ailyProject/aily-builder` 的 `i3w-builder`，固定提交
[`e18855124a03e40ec10f47912e62db113dbf2716`](https://github.com/ailyProject/aily-builder/commit/e18855124a03e40ec10f47912e62db113dbf2716)。
需要从源码使用此版本时按该提交检出，再沿用其锁文件 `npm ci`／`npm run build` 和现有命令入口；
不把未更新的 npm 版本号、其他分支或提交前的 dist 当作该修复版本。不升级包版本或依赖，也未发布 npm 包。

匹配CI默认仅手动构建、校验候选及两个已声明目标；可选`verify_location_input`只增加同一ZIP的独立定位输入消费者。
该选项复用相同SDK和已装PubSubClient，在新的sketchbook中验证`--location-input`；不是再打一份库、开放设备位置或运行GPS硬件。
若要宣称本次CI验证了定位输入，必须勾选此项并确认对应step实际成功，不能以默认两目标成功替代。
CI不部署服务器、不刷板，也不自动发布正式Release。
新增 workflow 配置不代表 GitHub 已运行通过；实际远端结果仍须在触发后独立记录。
不在 CI 中填写账号、DeviceKey、WiFi 密钥或私有备份。

当前默认分支 `master` 尚无该新工作流，不能仅推开发分支后就宣称可手动 dispatch。
现有 `device_v2` push 质量 workflow 在 host／package 门禁通过后，以本地 `uses` 调用同提交的候选 workflow，
显式传 `verify_location_input: true`，不改 `master`、不继承 secrets、不重复维护打包步骤。
手动入口仍保留默认关闭的选项；部署无关，必须核实际 run／提交与定位步骤成功后再取其 artifact。

从 GitHub Actions 下载的 artifact 还有一层外部压缩容器。先展开它，取得里面的
`BlinkerDeviceV2-0.1.0.zip` 和 `SHA256SUMS`，再校验、安装这个库 ZIP；不要把外层 artifact 当作 Arduino 库。
每次 CI 按对应提交生成自己的校验文件，不假定它与尚未提交的本机候选字节相同。

## 7 可选设备定位接口

当前源码提供`BLINKER_LOCATION(position)`和`Blinker.location(position, latitude, longitude[, sampledAtMillis])`，
只接收用户驱动／解析器给出的WGS84观测；不安装GPS依赖，不自行扫描串口或产生默认点。
首次接入须把`position`列入`Blinker.begin`；完整最小用法随库README提供，不要求安装者能打开仓库文档。
进阶精度／UTC、失锁、年龄及同boot时基见[统一输入API](DEVICE_V2_LOCATION_INPUT_API.md)。
Cloud位置仍需既有精确Manifest与owner准入，不能把API声明、CI选项或编译通过当成设备已获授权。
本机当前源码的隔离安装／原SDK完整链接证据取定位方案§19；它不是旧R5 ZIP或本次远端CI的证明。
