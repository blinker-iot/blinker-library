# Device V2 正式实施主线

> 更新：2026-08-28。旧 Widget/TLV、一次性 EnrollmentKey、`bdk_` 前缀和 CloudCredential 默认图不再构成
> 待办。Education WiFi 主线以无前缀 DeviceKey challenge-HMAC 为准。当前执行顺序以
> `DEVICE_V2_ARCHITECTURE_CLOSURE_AUDIT.md` 为准；本文件保留历史批次证据，不以较早的“已完成”覆盖最新复核。

> 最新 BLE 安全纠偏：早期批次中“Direct 必须依赖 encrypted GATT/SMP/bonding”的文字只保留为历史记录，
> 不再是现行合同。现行可移植基线是普通 GATT + Noise enrollment + Method 2 + DirectSecureRecord；XIAO、
> UNO R4、Wio 均已有应用安全真机证据，native link encryption 仅可作为平台增强能力。

## 当前执行点：C3 已完成，进入 C4 发布配置收口

C1 已把 ESP32、UNO R4 与 Wio Terminal 的纯 BLE Product 统一为
`BleEnrollmentGrant → DeviceAccessStore → Direct BBP/2`。设备只保存 physical instance、随机 accessEpoch 和
Admin/Shared controller；不再保存 account/owner/family/logical-device。旧 EnrollmentTicket、OwnershipClaim/Record、
CloudCredential enrollment、LocalSetup 8-operation saga、独立 ControllerCredentialStore、factory/pre-enrollment 工具、
平台旧 blob、Host/golden/SizeProbe/CI 引用均已物理删除。

新 application 只有 Hello 与 Enroll 两个请求，grant exact 绑定 setup locator/session、Noise transcript、随机 epoch、
controller digest、权限、时效和 server key；一次原子写入 176 B access root，响应为 HMAC mutation receipt。同一活动
session 只允许相同 bytes exact replay。BLE-only `clearAll()` 会清除整个 access root，从而允许物理 reset 后重新认领；
存在 DeviceKey 时仍保留 cloud 恢复入口。Wio storage schema 升为 2，测试阶段不迁移旧实验 image。

当前 `src` 的普通 `.cpp` 已从 69 个收口为 16 个：12 个跨平台职责单元、Arduino 通用单元和 ESP32/UNO R4/Wio
三个平台单元；69 个就地 `.inc` 只作为可审查实现片段，不会被 Arduino 单独编译。公开 API、wire 和 Product 行为未变。
Host ASan `25/25`、三平台 Core clean compile、architecture/package/Python syntax 已通过。同一 256 文件 ZIP 已在空
sketchbook 完成 ESP32 WiFi/BLE/WiFi+BLE/CloudSmoke、UNO R4 WiFi/BLE、Wio Terminal WiFi/BLE 八目标 clean compile；
依赖集合、最终 ELF 裁剪与安装源码 provenance 全部通过。下一批 C4 只收口正式 host/key/profile 注入和 placeholder
fail-closed，不改变 API、wire 或 16 单元边界。详细结构与资源见 `DEVICE_V2_IMPLEMENTATION_UNIT_LAYOUT.md`。

C4 第一片已实现 release profile 注入与反向门禁：正式 builder 缺 profile、误用 development、placeholder host、仓库测试
key、TLS 无 CA 均失败；CI 只能用显式 development 开关并上传带 development 名称的 artifact。profile 化 ZIP 已双构建
确定性并通过 UNO R4 WiFi 安装编译。真实 production host/key/profile 尚未提供，因此 C4 仍为当前执行点，详见
`DEVICE_V2_RELEASE_PROFILE.md`。

C1 最终软件门禁为 Windows ASan Host `25/25`、5,000 fuzz、五组跨语言 golden、architecture/package/Python syntax
通过；UNO R4 SizeProbe 为 `52,408 B Flash / 14,556 B globals`。ESP32 BLE Product clean compile 超过 10 分钟仍未
完成并主动终止，未继续运行后续 UNO R4/Wio Product；这不是 C1 逻辑失败，但已把 C2 从“优化项”提升为正式发布阻断。

## 历史执行点：U-ARCH 单库发布修订

2026-08-19 根据用户安装体验重新审查后，撤销“ESP32/UNO R4 × WiFi/BLE 分别发布 ZIP”的结论。Arduino
用户最终只安装一个 `BlinkerDeviceV2` 库；`BlinkerWiFi.h` 与 `BlinkerBLE.h` 只是同一库中的编译期 selector。
公开 Product、平台 Port 和 Arduino Port 已物理迁入 `src/BlinkerV2/arduino`，三类正式示例已收口到根
`examples/`，Advanced Core 示例移入 `tests/arduino`。根 `library.properties` 已改为双架构、统一依赖集合和
`dot_a_linkage=true`。ESP32 WiFiProv/WiFi+BLE 仍是 internal 候选，已退出公开 `src`，不会因 Arduino 递归
编译规则污染普通产品。

U-ARCH-1..5 已完成：同一个库目录 clean compile ESP32/UNO R4 的 WiFi/BLE 四组目标并通过反向 map/ELF 图检查；
唯一发布器和 CI 只生成一个公开 ZIP；旧 release manifest、两个 per-product/Core builder、12 组 profile、旧 verifier
及禁用 CI job 已物理删除。本地 interop builder 也已改为只覆盖统一 staged library 的控制面地址，不再接收平台参数。
U-ARCH-6 已完成：Arduino IDE 默认插入 `BlinkerWiFi.h`，BLE 用户显式使用 `BlinkerBLE.h`；正式门禁已在空
sketchbook 安装唯一 ZIP，显式准备 Arduino CLI 本地 ZIP 路径不会自动解析的精确依赖，并从安装目录通过四目标
clean compile、链接库集合和源码来源审计。完整修订见
`DEVICE_V2_STANDARD_ARDUINO_LAYOUT.md` 与 `DEVICE_V2_PACKAGING_CI_AND_PLATFORM_MATRIX.md`。

统一库首次实编还发现 Arduino archive 会扁平化对象 basename：Product/Port 同名 `Esp32WiFi.cpp.o` 导致
Product binding 被覆盖。四个 binding 已改为 selector 可达的 `*WiFiProduct.h`/`*BLEProduct.h`，单一发布器继续
检查普通源文件对象名冲突。随后复查发现旧门禁只检查 Blinker 类名，另一模式 `.cpp` 仍会触发 Arduino 外部库扫描；
现已把有外部依赖的 Port 实现改成 selector 引入的内部 inline `.ipp`，公共 Core 仍保持 `.cpp`/archive。

正式安装 ZIP 门禁中 ESP32 WiFi 为 `980,015/49,856 B`，不再编译 NimBLE；ESP32 BLE 为
`763,007/43,064 B`，不再编译 Arduino WiFi/PubSubClient，从默认 APP 分区约 96% 降至约 58%。UNO R4 WiFi
为 `125,432/12,696 B`，不再编译 ArduinoBLE/Crypto；UNO R4 BLE 为 `193,108/16,896 B`，不再编译
PubSubClient。R4 BLE 仍出现的 WiFiS3
来自 Renesas `Preferences.h -> Modem.h` 存储后端，不是 Blinker WiFi Product。CI 和 architecture gate 已冻结
selector/`.ipp` 结构及按目标的外部依赖反向检查，不能恢复多个用户 ZIP规避边界问题。

冻结精确依赖后的单一发布器已生成两个字节完全一致的 305 文件 ZIP（402,207 B，SHA-256
`7994e0cb2c1252d4b19ea8313d8bc4ab5d5ec435794ec08eb010edaec6de2db3`）。正式门禁在空 sketchbook 安装该 ZIP，
CLI 仅出现一个 `BlinkerDeviceV2 0.1.0`，根目录只有标准 metadata、`src` 与三类正式示例；门禁显式安装本地
`--zip-path` 不会自动解析的精确依赖后，从安装目录通过四模式 clean compile、实际链接库集合、唯一 map 和
`compile_commands.json` 来源审计。手工 ZIP 安装当前仍需先准备四个外部库；后续依赖收缩不能恢复 per-product
包。旧 per-product builder/profile/manifest 已完成物理退役。

本轮目录与发布修订后的 Host 门禁为 40/40，pre-enrollment Python/C++/UNO transcript exact 校验通过，
20,000 次 deterministic protocol fuzz 通过。下文 R-ARCH 等段落是形成当前结论的历史批次记录，不再覆盖上述
U-ARCH 事实源。

2026-08-19 R-ARCH-6 已完成。物理删除了只由历史测试维持、没有 Product 消费者的
`EnrollmentKey/WifiOnboarding/CloudSession` 旧登录图，共 14 个 Core 文件和 3 个专属 Host 测试；
`BlinkerV2Advanced.h`、fuzz、SizeProbe 与 CMake 同步移除旧接线。仍有真实实验消费者的
CloudCredential enrollment、LocalSetup、Ownership release 没有被误删，但它们不再是 Education WiFi 登录入口。

official builder 不再把全量 Core `.cpp` 复制到每个用户包，而是从唯一 selector、Product 和所选 Port 递归生成
Core 编译闭包；每个 Core `.cpp` 必须有同名 `.h`，包门禁同时拒绝 BLE 包携带 MQTT/WiFi 实现、WiFi 包携带
BLE/Noise 实现。公开包真实 clean compile 为 ESP32 WiFi `980,451/49,856 B`、ESP32 BLE
`764,423/43,064 B`、UNO R4 WiFi `125,488/12,696 B`、UNO R4 BLE `193,432/16,896 B`
（Flash/global RAM）。Core 工程 ZIP 仍为 263 个文件，解压后 Wio Terminal 为 `62,980 B` Flash。

同批补齐四个公开 Product 的 release example config，取消 `/release` ignore，修正 CI 的 `products/**` 触发、
UNO R4 YAML 缩进和 Python 全库语法门禁；删除根旧 ESP8266 issue template 与过期 keywords。最终软件证据为
MSVC ASan `40/40`、pre-enrollment exact、`20,000` 次 deterministic fuzz、schema 4 全配置/确定性归档、
architecture 与四个公开生成包真实链接。当前剩余阻断不是继续改目录，而是 App/Service 生产 BBP/2、真实
Gateway/Broker、ESP32/UNO R4 配网/掉电/重连/长稳以及 heap/stack/RF 水位。

2026-08-18 R-ARCH-5 已完成：release manifest 升为 schema 4。Core 工程包采用纯 include 白名单；除标准
metadata/README 外，源码与例程只含 `Blinker.h`、`BlinkerV2Advanced.h`、平台无关 Core 和
`AdvancedCoreModel`，不再把 `BlinkerWiFi.h`/
`BlinkerBLE.h` 伪装成无 composition 的可用入口。公开 Product 只能引用 `products/arduino` 与正式示例，
internal test profile 只能引用 internal probe；只有公开 Product 能生成 ZIP，internal 传 `--output-dir` 会
fail closed。official builder 改为正向复制 Core 子树、唯一入口与 manifest 精确源，不再复制整个 `src` 后删除。
当时 `build_device_v2_packages.py` 曾收口改名为 `build_device_v2_core_package.py`；该分包工具已在 U-ARCH-5
随 schema 4 manifest 一并退役，本段仅保留历史演进记录。

同批删除了仅由历史 Host 测试维持的 7 个 P-256 Developer Identity/FactoryIdentity 源文件及接线；当前产品
使用随机 `DeviceInstanceId`，未来制造证明必须以新的可选 attestation 合同重新设计，不能恢复旧图。保留了仍被
server key ring 使用的 P-256 canonical signature 测试。三类白名单、8 个 profile staging、4 个公开 profile
确定性归档与 4 个 internal profile 确定性测试树已通过。新的 Core 工程 ZIP 解压后在 Wio Terminal 实编为
`62,980 B` Flash；代表性公开 Product 实编为 ESP32 WiFi `995,647/49,856 B`、UNO R4 BLE
`193,400/16,896 B`，UNO R4 LocalBLE internal 测试树为 `161,232/14,840 B`（Flash/global RAM）。Host
MSVC ASan `43/43`、pre-enrollment exact 与 `20,000` 次 deterministic fuzz 通过。下一批执行 R-ARCH-6
最终路径、metadata、例程与跨平台资源回归。

2026-08-18 R-ARCH-3 已完成：`products/arduino/src/internal|platform` 及其空目录已物理删除，Product 真源
收口为 `products/arduino/common`、`products/arduino/esp32`、`products/arduino/uno_r4`。manifest、冻结哈希、
internal probe、架构门禁与官方包门禁同步切换；旧层级一旦重现会直接失败，三个 Product 源目录与 manifest
必须精确相等，不能残留无消费者文件。12 组包配置和确定性归档已在新布局通过；四个公开 QuickStart 真实
clean compile 为 ESP32 WiFi `995,647/49,856 B`、ESP32 BLE `772,139/43,064 B`、UNO R4 WiFi
`125,464/12,696 B`、UNO R4 BLE `193,416/16,896 B`（Flash/global RAM）。Host ASan `43/43`、
pre-enrollment exact 与 `20,000` 次 deterministic fuzz 通过。公开 selector、`begin()`、DeviceKey、BBP/2 与
App/Service 合同未变。下一批执行 R-ARCH-5：分别冻结 Core 工程包、用户 Product 包和 internal 测试包的
发布白名单。

2026-08-18 R-ARCH-2 与原计划 R-ARCH-4 合并完成：13 个伪独立 `adapters/` Arduino 小库已收口为
`ports/arduino`、`ports/esp32`、`ports/uno_r4`，39 个重复 metadata/README 和空目录已物理删除。保留 26 个
能力源文件和独立 translation unit；Crypto/Storage/BLE security 的三路 `__has_include` 迁移桥已删除，统一只
include Core 窄接口。release manifest 升为 schema 3：只把 Core 视为独立工程包，13 个 Port 通过精确 source
清单由 8 个 Product 选择；官方生成包仍按相对路径放入 `src/internal/repository/ports`。compile matrix 会为
窄 Port 探针临时生成测试库，普通用户不安装 Port。DeviceKey 登录正文冻结未变，BBP/2、组件、公开
`begin()` 及 App/Service 合同也未改变。

本批门禁：MSVC ASan Host `43/43`、pre-enrollment exact、`20,000` 次 deterministic fuzz；architecture、
Core/Port boundary、12 个配置的精确 package/双构建确定性全部通过。Arduino Port 探针覆盖 ESP32/UNO R4
WiFi、Storage、HTTP 与 UNO exact-connection BLE security；真实产品包实编为 ESP32 WiFi
`995,647/49,856 B`、ESP32 BLE `772,139/43,064 B`、UNO R4 BLE `193,408/16,896 B`。期间发现 Windows
大小写不敏感文件系统会让内部 `ArduinoBle.h` 截获外部 `ArduinoBLE.h`，已改名为 `ArduinoBleLink.*`，并在
Port boundary 禁止与外部入口头大小写碰撞。Port 现声明自己的 external dependency，Product 依赖必须严格等于
所选 Port 依赖并集。清理了 35 个无 tracked 文件的历史 build 目录和 Python cache；这些均为可重建产物。

2026-08-18 R-ARCH-1 已完成，修正了上一批仍存在的非标准扁平打包：official builder 现在把 Product/adapter
按仓库相对路径放入 `src/internal/repository`，生成配置固定在 `src/internal/generated`；公开包 `src/` 根恰好
只有 `BlinkerWiFi.h` 或 `BlinkerBLE.h`。构建器拒绝目标碰撞，package gate 对 12 个配置校验精确文件集合、
外部依赖隔离与确定性 ZIP。DeviceKey 登录冻结也改为忽略纯 `#include` 布局路径，避免文件整理被误判为协议
变化；实现正文仍被冻结，依赖变化由 architecture/package/Arduino compile 门禁负责。

源码分层后续顺序冻结在 `DEVICE_V2_STANDARD_ARDUINO_LAYOUT.md`：R-ARCH-3 已完成 common + platform
binding 收口，并继续按能力保留独立 translation unit，没有合成强制链接全部 SDK 的大 `Platform.cpp`。
R-ARCH-5 已建立 Core/Product/internal test 三类白名单；下一批只做发布树最终清理和回归，期间不改变
DeviceKey、BBP/2、组件或公开 `begin()`。

2026-08-18 收缩批已完成：公开 WiFi 固定为 `<BlinkerWiFi.h>` +
`Blinker.begin(DeviceKey, SSID, password, interactions...)`，公开 BLE 固定为 `<BlinkerBLE.h>` +
`Blinker.begin(interactions...)`；两者均删除冗余 mode tag。旧 `WiFi.onboard(...)`、
`WifiOnboardingProfile` 和默认 EnrollmentKey composition；ESP32/UNO R4 `CloudSmokeTest` 真实官方包
分别为 `1,011,211/49,888 B` 与 `127,320/12,724 B`（Flash/global RAM）。DeviceKey Product 和 BLE Product
完成合并，旧 `integrations/official-arduino`、`products/arduino` 和按产品复制源码的 package 层已物理删除；
唯一源码真源位于标准 `src/BlinkerV2/arduino/{internal,platform,ports}`，内部 probe 位于 `tests/internal`。
随后删除旧 ESP32/UNO R4 `WifiCloudStack/WifiBleComposition/*WifiPlatform`，只保留 ESP32 P6
`DeviceAccessStore + DeviceKey session + WiFiProv Security1 + NimBLE Direct` 业务图。SoftAP 是默认 bearer，
IDF-NimBLE 目标可编译期选择 BLE scheme。历史 P3 Plain 为 `1,762,543/62,980 B`；2026-08-23 正式安装 ZIP
默认 P6 Plain 为 `1,406,115/62,848 B`，`1,395,187/62,848 B` 仅为历史快照。UNO R4 历史 WiFi+BLE 候选已永久退出，
不再实现 setup driver；其产品只含独立普通 WiFi 和普通 BLE。双平台 BLE 已绑定统一 facade/package，实编为
ESP32 `772,139/43,064 B`、UNO R4 `193,416/16,896 B`。当前发布物是一个标准 Arduino ZIP，包含 WiFi、BLE、
ESP32 WiFi+BLE 和 CloudSmoke 四类正式示例；selector 隔离依赖发现，最终固件由 section GC、ELF
defined-symbol 与 dependency 门禁验证，不再把 archive inclusion table 误当成代码存活证据，也不生成每 MCU/
每 transport 的多个 ZIP。

本批进一步冻结 selector/tag-free API：ESP32 WiFi/BLE public package clean compile 为
`995,647/49,856 B`、`772,139/43,064 B`，UNO R4 WiFi/BLE 当前 R-ARCH-1 包为
`125,416/12,696 B`、`193,368/16,896 B`；Wio Terminal 的 fake lifecycle WiFi/BLE selector均为 `48,836 B`。
package gate 会物理删除未选 selector，`library.properties/includes` 只指向当前产品入口。
同批重测 TLS WiFi 包为 ESP32 `1,085,047/50,448 B`、UNO R4 `128,616/12,728 B`；此前资源表中的
`1,103,471/51,224 B` 与 `162,068/13,380 B` 属于旧 composition，已从当前矩阵纠正。
最终软件门禁为 MSVC ASan Host `43/43`、pre-enrollment exact、`20,000` 次确定性 fuzz、architecture
检查和全部 official profile 双构建确定性通过。本批临时 stage/build 目录已在记录资源后清除。
收尾依赖审计进一步删除三个 Crypto/Storage adapter 对完整 `BlinkerV2Advanced.h` 的无条件依赖；Product
显式 include 自己的 P-256/Ed25519 verifier。UNO exact-connection 安全源改为实现 Core 的平台中立
`IBleConnectionSecuritySource(StringView peerAddress)`，不再从 `renesas-uno-ble` 头跨目录继承通用 ArduinoBLE
adapter 合同。独立 UNO BLE security/portable crypto、UNO R4 BLE 与 ESP32 BLE 用户包均已 clean compile。

2026-08-18 完成 Controller Control v2 软件批：旧 v1 Grant 需要 298 B、依赖 logicalDeviceId 和设备可信
epoch，无法进入 256 B Small frame，也与 DeviceKey product 的真实状态不匹配。v2 改为 14 项 canonical
array，只绑定 physical instance、ownership generation、credential CAS、secret digest 与 fresh device nonce。
最大 Grant/Mutation/完整 BBP/2 frame 为 `193/231/241 B`。

BBP/2 已增加 Open/Challenge/Mutation/Receipt `0x30..0x33` 与 HELLO feature bit 9；Runtime 只允许协商成功、
encrypted Local transport 且已取得 `ManageControllers` 的 session。真实 endpoint 使用 caller-owned 256 B
workspace，控制窗口由单一 transport/session 独占，断连/stop 清零。三端 exact golden 已升级为 v2；Host
覆盖最大帧、未授权/未加密拒绝、Install/Rotate/Revoke、重放与 session 生命周期。

最终复核又关闭了两个不能带入产品图的边界：coordinator 现在先完成 deterministic Receipt proof/编码，
随后才允许 durable credential mutation，任何 crypto/缓冲失败都不会形成“已落盘但无回执”；Runtime 只有
本地接收上限至少 `241 B` 才发布能力，App HELLO 接收上限小于最大 Receipt frame `155 B` 时不协商该能力。
Contract/BBP 上界由编译期断言锁定。最终门禁为 Windows ASan Host `42/42`、pre-enrollment exact、
`200,000` 次 deterministic fuzz、architecture/16-package boundary、official aggregate 全 profile 双构建确定性；
Core clean compile 为 ESP32 `324,955/21,672 B`、UNO R4 `69,320/7,664 B`、Wio Terminal `48,236 B`。

后续审计否决了最小 OwnershipBinding：设备无需 owner/logical ID，只需 4 B accessEpoch。当前已完成：两槽
allocation-free table；140 B standalone BCT2；176 B P6 DeviceAccessStore；Security1 v2 Bootstrap 原子安装
DeviceKey + accessEpoch + Admin；Shared 权限钳制和最后 Admin 保护；P6 注入 P-256 Grant verifier 与 endpoint。

当前顺序固定为：

1. 已用真实设备组件 + fake I/O 串通 DeviceKey challenge/session、MQTT CONNECT/SUBSCRIBE 与 BBP/2 上下行，
   并补齐 Arduino HTTP request timeout；
2. 对当前 P3/P6 重新执行完整 Arduino link、Flash/global RAM，再以生产 Gateway/Broker + ESP32/UNO R4 完成
   功能 E2E 和 heap/stack/RF/循环门禁；旧 30.02%/25.58% 只作历史；
3. App/Service 实现 exact Bootstrap、每用户每设备 ACL、每设备 Shared KMS/轮换和 DirectRevocationPending；
4. Cloud Frame Authentication 移入后续安全批；未完成前 Plain profile 不生产，但不阻塞当前功能互操作。

普通 Sketch 已从 `<Blinker.h>` 切换到 mode selector；内部仍复用同一 facade。真机与生产三端仍是发布门禁。

STAB-1 当前证据：Host 严格构建/CTest `44/44`；ESP32 DeviceKey 云会话 clean link 为
`599,063 B Flash / 28,768 B globals`，UNO R4 DeviceKey 云会话为 `88,908 B / 11,232 B`，UNO R4 SizeProbe 为
`52,452 B / 16,632 B`；architecture 和 official aggregate package gate 通过。详见
`DEVICE_V2_CLOUD_LOGIN_STABILIZATION.md`。

此前 active `CompileDeviceKeySession` 仍实例化旧 `CloudCredential + HttpCloudSessionProvider`；STAB-1 已将其替换为
`ConfiguredDeviceKeySource + HttpDeviceKeySessionProvider + DeviceKeySessionArena`，上面的数字来自修正后的
DeviceKey 探针，旧 CloudCredential 数字不再作为当前主线证据。

### 设备侧上一片：R4b-6C internal BLE products

2026-08-17 完成 R4b-6C。ESP32 与 UNO R4 各自建立了 Platform BLE、LocalBLE 两个独立 official internal
profile；它们不是从 WiFi+BLE 对象图删掉 WiFi 得到的残缺变体。Platform BLE 只组合物理身份、Ownership
Claim、`LocalSetupApplication(ticketOperations=0)`、Ownership-domain Controller、Noise/Direct 与 server
签名验真；LocalBLE 只组合物理身份、Local-domain Controller、物理确认、Noise/Direct，不携带 Ticket、
Ownership、cloud host 或 server public key。

四种产品只复用一个 allocation-free `BleOnlyRadio`、`BleWorkspace` 和平台 radio glue。WiFi+BLE 也已改用同一
workspace/radio 实现，删除了重复的 Arduino clock、record/ATT buffers、direct config、ESP32 NimBLE wrapper
和 UNO R4 ArduinoBLE wrapper。架构门禁禁止 pure BLE composition 引用 Blinker WiFi/MQTT/HTTP/cloud 图；
聚合包门禁进一步验证 LocalBLE 配置不能携带 signing/cloud 字段，pure BLE 包不安装 PubSubClient。

四个 clean compile 的 Flash/global RAM 为：ESP32 Platform `760,639/41,584 B`、ESP32 Local
`731,287/40,792 B`、UNO R4 Platform `186,060/15,420 B`、UNO R4 Local `159,976/14,840 B`；对应完整
singleton 为 `6,960/6,176/5,632/5,072 B`。UNO R4 两图仍分别保留 17,348 B 和 17,928 B 静态余量。
共享 glue 后 WiFi+BLE clean regression 为 ESP32 `1,299,055/65,464 B`、UNO R4 `211,716/18,828 B`，
相对 R4b-6B 仅增加 `12/0 B` 与 `8/0 B`，没有第二套常驻 buffer。
最终门禁为 Host `38/38`、`200,000` 次 deterministic fuzz、architecture/15-package boundary、official
aggregate 双构建确定性，以及 ESP32/UNO R4 六个 internal composition clean compile 全部通过。
这些结果证明 package/link/static resource，不证明真实 central 的 exact encryption、配对恢复、动态
heap/stack 或长稳。因此下一步不得直接发布 facade：先完成四种 BLE 图的真机门禁，再冻结 R4b-7 的统一
`Blinker.begin(...)` 与 QuickStart；无硬件期间可以转入 R5 App/服务器 production codec/E2E harness。

### 上一片：R4b-6B BLE application/lifecycle

2026-08-17 完成 R4b-6B。新增 `LocalControllerPairingApplication`，通过既有
`ILocalProvisioningSession -> BleLocalProvisioningEndpoint -> Noise` 接线，未复制 endpoint/GATT service。
application 固定 Hello/Challenge/Pairing/Error canonical envelope，只有 Hello 后的同一 Noise final hash 可进入
Ready；BLE request 无法调用 `beginConfirmedPairing`，session end 总是结束 pairing 并清零 locator/transcript。
exact golden 固定 application Hello `7/36 B`、Challenge `7/91 B`、Pairing `141/177 B` 和错误 `14 B`。

本片同时纠正 Provisioning capability 混淆：LocalBLE 使用独立 `LocalControllerPairingV1`，普通/PSK
capability 为 `0x0043/0x0061`，绝不携带平台 `EnrollmentV1`/`WifiConfigV1`。App 因而可以仅从扫描结果选择
Local codec 或平台 Ticket/Saga，不再靠产品名、MAC 或连接后试错。

重复的 BLE mode/locator/完成状态/断线恢复已从 `WifiBleLifecycle` 抽入 allocation-free
`BasicBleSetupLifecycle`。平台流程只读取 `BSG2 Complete`，Local 流程只读取 `Local/generation=0`
ControllerCredential；共享的是 radio state machine，不共享授权合同。新增正式内部 `BleOnlyLifecycle`，可按注入的
application/completion 承载平台 BLE-only 或 LocalBLE；旧 `DirectBleLifecycle` 保留为“制造阶段已预配 controller、
无需首次 setup”的窄高级路径，不冒充正式 BLE-only onboarding。

审查时关闭了 receipt/App secret 丢失后的锁死缺口：产品物理入口可调用 `openLocalPairingMode()` 重新广播，但它
只切 mode；Noise 建立后仍需独立物理确认绑定 exact transcript。已有 credential 在整个新 pairing session 结束前
保持 Direct 切换受阻，断开后再按 durable completion 回到 Direct。平台 Enrollment contract 明确拒绝该入口。

32 位 ABI/UNO R4 map 为 application `56 B`、coordinator `124 B`、BLE-only lifecycle `44 B`、WiFi+BLE
lifecycle `52 B`、两种 completion 各 `8 B`。Windows ASan 38/38、pre-enrollment exact、200,000 次
deterministic fuzz 已通过；Core clean compile 为 ESP32 `322,495/21,672 B`、UNO R4
`69,304/7,664 B`、Wio Terminal `48,236 B`。WiFi+BLE internal full graph 为 ESP32
`1,299,043/65,464 B`、UNO R4 `211,708/18,828 B`；相对 R4b-5D 分别增加 `1,672/16 B` 与
`376/16 B`，主要来自新增 capability/共享 completion 引用，未引入第二套 runtime 或 workspace。R4b-6C 只进入两平台 internal
BLE-only/LocalBLE package、链接和资源档，不公开 facade，也不等待真机才继续可执行的软件工作。

### 上一片：R4b-6A LocalBLE exact pairing

2026-08-17 完成 R4b-6A，并纠正“平台 BLE-only 与 LocalBLE 可以先共用一个 composition”的错误顺序。
平台 BLE-only 已有的正确底座是 `LocalSetupApplication + LocalSetupSaga(ticketOperations=0)`：Ticket 已绑定首个
controller ID、secret digest 与权限，Ownership commit 后由 `InstallInitialController` 在同一 Ticket 授权事务内
安装首个 Ownership-domain credential；不能再额外要求一个尚不存在 controller 才能认证的 local session，也
不应重复签发 ControllerGrant。后续新增/轮换 controller 才走 Ownership-domain Controller Control。

LocalBLE 则不能伪造上述服务器授权。本片新增独立 `LocalControllerPairingContract/Coordinator`：当前 Noise
final hash + 设备侧物理确认打开默认 60 s 窗口，设备 CSPRNG 生成 16 B nonce；App 在 Noise transport 中提交
16 B controller ID 和 32 B secret，权限 exact echo 设备策略，持久化为 `Local/generation=0/version=1`。设备用
streaming canonical CBOR 直接计算 HMAC receipt，不分配完整 transcript scratch。32 位 coordinator ABI gate
`<=128 B`；同窗口只允许 exact request replay，different request、context mismatch、超时、RNG failure 和 store
failure 均 fail closed。exact C++ golden、Host 状态测试和 deterministic fuzz 已接入。

本片没有公开 `Blinker.begin(BLE...)`，也没有把 BLE packet 变成可打开物理窗口的入口。当时规划的下一片
LocalBLE Noise application endpoint 与平台 BLE-only lifecycle 已由顶部 R4b-6B 完成；两者共享
BLE/Noise/Controller Method 2 底座，但不共享授权合同。权威合同见
`DEVICE_V2_LOCAL_CONTROLLER_PAIRING_CONTRACT.md`。

最终软件门禁为 Windows ASan 38/38、pre-enrollment exact、200,000 次 deterministic fuzz、架构检查、
15-package boundary 和 official 聚合包双构建确定性通过。新增 LocalBLE C++ golden 锁定 Challenge/Request/
Receipt 为 80/131/166 B、receipt transcript 160 B；UNO R4 SizeProbe ELF map 锁定 coordinator 为 124 B。
同一 Core clean compile 通过 ESP32 322,023 B Flash / 21,672 B global RAM、UNO R4 69,304/7,664 B、
Wio Terminal 48,236 B Flash。最后还补上 response-loss fault：Store 成功即锁定 exact request digest，receipt
buffer/发送失败后只能重试同一 bytes，不能在一个物理窗口安装第二个 identity。

### 上一片：R4b-5D 软件 fault gate

2026-08-17 完成无真机仍可严格验证的 R4b-5D。`BleRecordTransport` 现在把一次
`connected(sessionId)` 作为逻辑会话唯一准入：未准入包直接拒绝；同一活动 ID 被再次宣布时清除 RX/TX、关闭
上层 session 并隔离后续包；只有收到 disconnect 后才允许数值 ID 合法复用。已成立的
`encrypted/bonded/authenticated` 事实只允许升级，不允许在同一会话中回退；即使平台漏发 disconnect，`poll()`
也会以当前 session table 对账并关闭安全降级/消失的会话。它不能替代 adapter 的 exact-peer 识别，但可防止错误
peer 或快速重连继承旧 reassembly、队列和 Controller 授权状态。Broadcast 仍保持原子语义，不会静默跳过未准入 peer。

产品故障回收也已补齐：WiFi、Client/cloud、BLE profile 配置或 BLE stack 任一启动失败，
`BasicWifiBleLifecycle` 都会回收到 `Stopped`，同一对象可在修正原因后重试；Direct BLE endpoint 进入物理 Error
会成为 coordinator/product Fault，不再静默只丢失本地链路。Provisioning 的 session fault 仍按既定事务语义
`restartActive()`，不会被误判为不可恢复 Direct fault。上述路径均由 host fake link 自动化覆盖，没有新增用户 API、
动态分配或常驻字段。

最终门禁为 Windows ASan 36/36、pre-enrollment exact、200,000 次 deterministic fuzz、架构检查、
15-package boundary 和全部 official 聚合包双构建确定性通过。完整 internal graph clean/incremental 实编为：UNO R4
211,332/262,144 B Flash、18,812/32,768 B global RAM；ESP32 Minimal SPIFFS 1,297,371/1,966,080 B
Flash、65,448/327,680 B global RAM。本片分别增加 528 B、800 B Flash，static RAM 均增加 0 B。

R4b-5D 完成的是软件故障边界，不可能凭静态摘要证明动态 RAM 足够。UNO R4 仍只剩 13,956 B 给 stack/heap，
真实 WiFiS3 + ArduinoBLE + crypto 并发 watermark、pairing/encryption-loss、双无线共存、掉电和长稳继续阻断公开
WiFi+BLE facade，并归入真机/发布门禁。设备代码主线可进入 R4b-6；不能借此提前进入 R4b-7。

### 上一片：R4b-5C UNO R4 internal official composition

2026-08-17 完成 R4b-5C。审计 ArduinoBLE 2.0.2 后确认 `BLE.paired()` 是 any-peer 查询，不能证明
当前 central 已加密；旧 `ArduinoBleLink::setSessionSecurity()` 已删除。通用 adapter 现在按当前 central
跟踪连接，`requireEncryption=true` 却未注入 `IArduinoBleSecuritySource` 时 fail closed，加密丢失后撤销
session 并断开。架构门禁固定禁止重新引入 any-peer 查询或业务可写安全标记。

新增的 `renesas-uno-ble` 只实现一个平台事实源：把当前 central 地址映射为 ArduinoBLE ATT connection
handle，并读取该 handle 的 controller encryption-complete 位。该 adapter 与 internal official package
固定 ArduinoBLE 2.0.2；升级必须重新审计。ArduinoBLE 没有等价的 per-connection bond 事实，因此
`bonded=false`，应用身份仍由 Controller Method 2 授权，不把配对冒充所有权。

UNO R4 WiFi-only 与 WiFi+BLE 现共用 `RenesasUnoWifiPlatform` 和 `WifiCloudStack`；BLE 产品只增加现有
Preferences bank 的 Controller/BSG2 view、ArduinoBLE link 和 exact-handle security source，没有第二个
Client、MQTT runtime 或 Preferences handle。`uno-r4-wifi-ble-internal` 两次确定性聚合包通过，完整对象图
在 Renesas Core 1.5.3 + ArduinoBLE 2.0.2 实编为 210,804/262,144 B Flash、18,812/32,768 B global RAM，
静态 singleton 为 9,008 B。WiFi-only 回归为 159,156 B / 13,304 B，且不携带 ArduinoBLE。

R4b-5C 关闭的是软件安全事实和静态资源门禁，不是公开发布准入。其后规划的软件 fault gate 已由顶部
R4b-5D 完成；13,956 B 静态余量仍须真机 stack/heap watermark 验证。

### 上一片：R4b-5B ESP32 internal official composition

2026-08-17 完成 R4b-5B。WiFi-only 与 WiFi+BLE 已抽取并共用
`WifiCloudStack<Platform>`，不复制 platform、DeviceInstanceId、Claim/Cloud coordinator、WiFi lifecycle、
1,414 B control-plane arena、HTTP/PubSubClient/Managed MQTT，也不创建第二个 Client/runtime。ESP32
WiFi+BLE 只增加独立的 `bl_v2_ctl/table` Controller table、`bl_v2_setup/saga` BSG2、共享
Provisioning/Direct workspace、Noise/LocalSetup、Direct BBP/2、ModeCoordinator 和 Method 2 authorizer。
该对象图仅由 internal `esp32WifiBleLifecycle()` 编译探针引用；公开 `lifecycle(WiFiBleTag)` 仍未绑定。

BLE 栈选择已用完整 ELF 纠偏：Arduino-ESP32 Core Bluedroid 在默认分区为
1,795,395/1,310,720 B（136% Flash，70,344 B global RAM），不可作为 official 底座；NimBLE-Arduino
2.5.0 在默认分区为 1,296,523 B，虽能链接但只余 14,197 B，也不接受为发布资源档。正式 internal profile
固定 `Minimal SPIFFS (1.9MB APP with OTA)`，实编为 1,296,571/1,966,080 B（65% Flash）与
65,448/327,680 B global RAM（19%），余 669,509 B Flash/262,232 B RAM。WiFi-only 包不安装或链接 NimBLE，
继续使用默认分区。

旧 `Esp32BleLink` 的可伪造 security seam 同时被纠正：encrypted/bonded 只能来自 ESP32 BLE stack callback，
`setSessionSecurity()` 只能提升后续 application-authenticated bit；链路加密与通知订阅完成前不向 Runtime 发布
session。官方仍选择更小的 NimBLE adapter，Core Bluedroid adapter 只保留为非默认兼容/诊断路径。

R4b-5B 软件门禁已通过 internal 聚合包两次确定性构建、Minimal SPIFFS clean/incremental Arduino compile 和
WiFi-only 回归。其后规划的 UNO R4 当前连接加密事实源与 internal graph 已由顶部 R4b-5C 完成；公开 facade
仍未因此提前绑定。

### 上一片：R4b-5A 平台无关 WiFi+BLE 编排

2026-08-17 完成 R4b-5A。新增 allocation-free `BasicWifiBleLifecycle`/正式别名
`WifiBleLifecycle`：同一 `Client/Runtime/StateStore` 挂 Managed cloud 与 External Direct BLE，BLE
`start/poll/stop` 只归 `BleModeCoordinator`，不再允许 Client 与 product lifecycle 双重拥有同一 GATT endpoint。
启动/运行 gate 固定为 `BSG2 Complete && WiFi profile durable confirmed` 后才从 Provisioning 切 Direct；当前
LocalSetup session 未断开时不强切，未完成会话断开则用同一 locator 重启，Cloud 仅在 Saga Complete 且 WiFi
Online 时开放。BSG2 只在启动或新 setup request 后读取，不在每轮 loop 访问 flash。

本批当时纠正安全标记：普通 `BleFrameTransport` 只做 frame/分片，不再声明
`TransportFeatureAuthenticated`。该阶段曾要求 Direct BLE 同时具有平台报告的 encrypted GATT 与
`IAuthorizationProvider` 的 Controller session 授权；这一链路层前置条件已由 2026-08-28 的应用安全基线取代。
ESP32 NimBLE 有真实 encrypted/bonded 观察来源；通用 ArduinoBLE 没有 portable 当前连接安全接口，故本片当时阻断 UNO R4 official WiFi+BLE 并禁止伪造
`encrypted=true`；该缺口现已由顶部 R4b-5C 的 Renesas exact-handle adapter 关闭。Renesas Preferences bank 在旧编号之后新增 `sg0/sg1` BSG2 双槽域，248 B 不提高既有
268 B 最大 scratch；core release 同步取消对 LocalProvisioning/Noise endpoint 的过期排除。

Host 新增三组 lifecycle 场景并锁定 Direct external transport 单次 poll，BLE mode 测试锁定 transport 不得
冒充 authenticated，Renesas storage 覆盖完整 248 B BSG2。最终软件门禁为 Windows ASan 36/36、
200,000 次 deterministic fuzz、架构检查、三端全量 golden、15-package boundary、六平台 Core ELF、
UNO R4 storage/SizeProbe clean compile，以及 ESP32/UNO R4 official Plain QuickStart 全源 clean compile；
四种 Plain/TLS 聚合包结构与确定性均通过。权威实施记录见
`DEVICE_V2_WIFI_BLE_COMPOSITION_IMPLEMENTATION.md`。本片之后规划的 ESP32 与 UNO R4 internal official
对象图现已分别由顶部 R4b-5B/R4b-5C 完成；公开 facade 仍留在 R4b-7。

### 上一片：R4b-4d durable setup saga

2026-08-17 完成 R4b-4d。`LocalSetupOperationContract` 冻结首次 Controller 安装与
ConfigureNetwork canonical CBOR；LocalSetup operation 从 5 个扩展为 8 个，Cloud 路径固定为 App exact
relay Request/Grant/Receipt/CommitAck，不要求设备先联网，也不把 Ticket、ManualDeviceKey 或 raw cloud secret
塞入日常协议。

新增平台无关、allocation-free `LocalSetupSaga`、240 B runtime record、4 B store 与 248 B `BSG2` durable
record。单一状态机串联 Ownership、首个 Controller、WiFi candidate 与 CloudCredential；
`ClaimCommitVerified`/`CloudCommitVerified` 两个 checkpoint 保证“外层已验 ACK、内层 journal 清理、外层推进”
任一处掉电都能恢复。Saga 不持有 BLE/WiFi SDK、HTTP、MQTT、JSON 或第二套网络/Cloud secret。

本批审查中立即纠正两项会放大的错误：网络 body 已提交后的重放不再无条件再次 `commit()`；相同 body 只校验
当前 profile，不增加 generation。其次，`CandidateStored` 不等于联网成功：未确认 candidate 在 Cloud 开始前
可以替换；身份/Cloud saga Complete 后首次 AP 连接失败，也可用同一 durable ticketId 的新
session-bound Ticket 重开安全 setup 并替换 candidate；`confirmActive()` 后入口关闭。这样错误 SSID 不会迫使
用户整机重置，也不会重做 Ownership/Controller/Cloud enrollment。

软件门禁：Windows ASan 36/36、200,000 次 deterministic fuzz、架构检查和 C++/TypeScript/Node 全量 golden
通过；BLE-only 与 WiFi+BLE Saga fault tests 覆盖 outer save、inner clear、exact replay 和网络恢复。
UNO R4 Core 1.5.3 Small SizeProbe 为 52,324 B Flash / 14,668 B global RAM，ELF 精确
`LocalSetupApplication/SagaRecord/SagaStore/Saga` 为 216/240/4/40 B；相对上片 +284 B global RAM 是探针
常驻三个新增 `sizeof` 数组，不是产品自动常驻。ESP32 Core 3.3.1、UNO R4 Core 1.5.3、Wio Terminal Core
均已生成 ELF。权威合同见 `DEVICE_V2_LOCAL_SETUP_APPLICATION_CONTRACT.md`。

R4b-4d 之后的第一片平台无关 WiFi+BLE 编排、BSG2 Renesas storage seam 与 BLE transport 安全标记纠正，
已由顶部 R4b-5A 完成。剩余是 ESP32 official 对象图、UNO R4 加密事实来源与目标板门禁；不提前扩散 facade，
不新增 MCU 专属业务 API；HC-05/Bluetooth Classic SPP 暂不进入主线。

### 上一片：R4b-4c LocalSetup application

2026-08-16 完成 R4b-4c。新增平台无关、allocation-free `LocalSetupApplication`：冻结
Hello/完整 16 B setupSessionId、最终 Noise transcript callback、EnrollmentTicket relay/accept、五类授权
operation、严格 request sequence/exact replay 和错误分类。Ticket 前 mutation 不可达；BLE-only 只接受
operation profile `0`，WiFi+BLE 只接受完整 profile `3`，不允许部分 capability。产品 mutation 只经
`ILocalSetupOperationHandler`，应用对象不直接持有 Ownership/WiFi/Controller store，也不保留完整 Ticket。

本批纠正了一个会封死掉电恢复的设计：application 不再仅看到 Ownership Active 就拒绝新 session，而由
handler 的只读 `prepareSetupSession(locator,generation)` 区分 fresh、可恢复 saga 与真正 Active。否则设备在
“Ownership 已落盘、首个 Controller 尚未安装”时掉电，会永远无法重新进入 setup。该片要求的单一 durable
saga 已由顶部 R4b-4d 完成。

LocalSetup request/response/body/workspace 上限为 529/273/512/512 B；operation workspace 由 256 B scratch +
256 B response-body storage 复用。UNO R4 32 位 `LocalSetupApplication` 精确 216 B。最终软件门禁为 Release
与 Windows ASan 35/35、200,000 次 deterministic fuzz、三端 LocalSetup exact golden；UNO R4 Core 1.5.3
Small SizeProbe clean compile 为 52,284 B Flash / 14,384 B global RAM。权威合同见
`DEVICE_V2_LOCAL_SETUP_APPLICATION_CONTRACT.md`。

该片规划的 ConfigureNetwork、Cloud path 与 crash-consistent setup saga 已由顶部 R4b-4d 收口；official
BLE-only/WiFi+BLE composition 仍未因此自动完成。

### 上一片：R4b-4b shared Noise secure session

2026-08-16 完成 R4b-4b。普通 DIY 的 provisional secure wire 固定为
`Noise_NN_25519_AESGCM_SHA256`，无需 SetupCode；Enhanced/Manufactured 可显式选择 NNpsk0。两档共用
allocation-free `NoiseNnSession`，旧 `NoiseNnPsk0Session` 文件和固定入口已删除。标准 NN 首包按规范实现为
`e[32] || clear payload`，第二包才带 AEAD tag；不再错误复用固定 48 B 握手开销。suite 由 composition/channel
config 在广播前确定，Mode capability bit1/bit5 互斥，连接内禁止协商、探测和降级。普通
`BleNoiseModePreparation`不需要 PSK provider；增强档才短暂读取32 B高熵PSK，未使用的provider open/close
lifecycle 已删除。

本批同时纠正跨层标识歧义：13 B Service Data 尾部是 8 B `setupSessionLocator`，不是 EnrollmentTicket
冻结的16 B `setupSessionId`。Core类型和接口已按 locator 重命名；下一片 application coordinator 必须持有完整
16 B ID并在受保护payload中交给App，禁止补零、复制或hash locator冒充完整授权ID。权威wire见
`DEVICE_V2_PORTABLE_NOISE_WIRE_CONTRACT.md`。

本片软件门禁为 Windows ASan 33/33、20,000 次 deterministic fuzz、三端全量 golden、14-package
boundary 与 official ESP32/UNO R4 Plain/TLS 聚合包。UNO R4 Core 1.5.3 Small SizeProbe 实编为
52,268 B Flash / 14,168 B global RAM；ELF 精确为 `NoiseNnSession` 272 B、
`BleNoiseProvisioningChannel` 520 B、`BleNoiseModePreparation` 16 B。probe 为锁定 ABI 故意把 session
及内含 session 的 channel 同时实体化，不能把该总 RAM 当作产品运行峰值；真实峰值仍待真机测量。
同批 SDK 编译通过 ESP32 Core 3.3.1 mbedTLS crypto + ordinary NN（357,479/20,816 B）、Wio Terminal
portable crypto（44,316/3,612 B）和 UNO R4 SoftwareATSE P-256 verifier（57,196/7,564 B）。

该片之后规划的 application envelope 与 durable saga 已由顶部 R4b-4c/R4b-4d 完成；剩余缺口是 official
composition，不再把“envelope/saga 尚未实现”保留为当前状态。

2026-08-16 完成 R4b-4a 的平台无关 EnrollmentTicket v1。新的 16-pair canonical CBOR body 最大 268 B，
签名 transcript 最大 216 B，绑定 DeviceInstanceId、当前 setupSessionId、完整 setupTranscriptHash、
ownership generation、首个 Controller ID/secret digest/权限和 BLE-only/WiFi+BLE 明确 operation mask；TTL
硬上限 15 分钟。Ticket 不包含 JWT、owner/product 文本、WiFi、raw controller secret、CloudCredential 或
SessionCredential，也不持久化为重启登录凭据。无可信 RTC 的 MCU 不猜 epoch，但仍 exact-match 当前
session/transcript，Gateway 兑换必须强制 TTL。codec/verifier 无 heap、JSON、平台分支；verifier 使用既有
server key ring，216 B caller workspace 用后清零。Host 新增独立状态/边界测试、最大 268/216 B 锁定、
第 8 路 deterministic fuzz seed，以及 C++/App-compatible TypeScript/Node 的 3/2/3 项 exact golden。

该 R4b-4a 时点尚未完成普通 Noise wire；这一缺口现已由顶部 R4b-4b 修订收口。EnrollmentTicket 本身的
268/216 B wire、验证和持久化边界未改变。

R4b-4a 当时的软件门禁：Windows ASan 33/33、20,000 次 deterministic fuzz、三端全量 golden、14-package
boundary 与 ESP32/UNO R4 Plain/TLS official package 确定性检查通过。UNO R4 Core 1.5.3 Small SizeProbe
实编为 52,268 B Flash / 13,632 B global RAM；相对上一片 52,260/13,472 B 的 +160 B 是探针把三个
`sizeof` 数组同时常驻后的测量增量，不是 runtime 自动常驻。ELF 精确对象为 EnrollmentTicket 104 B、
VerificationContext 72 B、Verifier 4 B；workspace 仍由 caller 分阶段复用，不在 verifier 对象内。
同一 Small SizeProbe 在 ESP32 Core 3.3.1 clean compile 为 302,363 B Flash / 27,648 B global RAM；这些
均为 source/ABI 门禁，不是 BLE product、radio heap 或并发峰值。

2026-08-16 完成 R4b-3 WiFi-only 官方组合的软件接线，不再把它描述为 pre-enrolled-only 探针。
ESP32 与 UNO R4 共用同一个 `WifiComposition<Platform>`、`<Blinker.h>` 和单一 `Blinker.begin()`：
DIY/教育入口为 `blinker::WiFi.onboard(ssid, password, enrollmentKey)`，预注册入口仍为
`blinker::WiFi`。组合实例化稳定随机 `DeviceInstanceId`、Ownership Claim、Cloud Credential
Enrollment、共享 control-plane arena、Cloud Session 与 PubSubClient MQTT；ESP32 注入 mbedTLS
P-256/X25519/GCM，UNO R4 注入 portable Ed25519/X25519/GCM。服务器签名和 Cloud Enrollment
X25519 公钥环由发布配置生成并以 `const` descriptor 借用，不把私钥或长期 bearer 放入 Sketch。
WiFi/EnrollmentKey 只在 `begin()` 同步复制到固定缓冲；WiFi journal 提交后立即清空 SSID/密码，
onboarding Active 后解除 HTTP header 并清空 EnrollmentKey。

该 R4b-3 时点的软件门禁为 Windows ASan 31/31、20,000 次 deterministic fuzz、14-package boundary、ESP32/UNO R4
Plain/TLS 确定性聚合包。官方 `WiFiOnboardingQuickStart` 实编结果：ESP32 Core 3.3.1 为
1,017,199 B Flash / 50,632 B global RAM；UNO R4 Core 1.5.3 为 158,892 B / 13,348 B，余
19,420 B SRAM。TLS 组合也已实编：ESP32 为 1,103,471/51,224 B，UNO R4 为
162,068/13,380 B，均未静默降级。当前没有真机，所以上述只能证明完整对象图、依赖、链接和静态资源；真实 AP、四阶段
HTTP、broker、known-answer、掉电、stack/heap watermark 与长稳仍是发布阻断。无硬件主线下一步进入
R4b-4 EnrollmentTicket/BLE-only 控制面接线；不得用当前 WiFi-only composition 改名冒充 WiFi+BLE。

2026-08-16 完成 R4b-3 第四片的真实密码 provider 与发布边界。Core 将只验 digest 的
`IP256DigestVerifier` 从含私钥生成/签名的 `IP256CryptoProvider` 拆出，避免 UNO R4 的 server-only
后端伪造无关能力；同时新增 `IEd25519DigestVerifier`。`BlinkerArduinoCryptoAdapter` 以固定
`Crypto@0.4.0` 在同一 portable provider 中实现 X25519、AES-256-GCM 与 Ed25519 验签；
`BlinkerEsp32CryptoAdapter` 使用 core 内置 mbedTLS 实现 X25519、AES-GCM 和 low-S P-256 验签；
`BlinkerRenesasUnoCryptoAdapter` 只包装 core 自带 `SoftwareATSE` 的 P-256 验签。未发布的旧
`esp32-noise` 名称和空目录已删除，不再让 adapter 名称绑定单一上层用途。

固定 X25519、AES-GCM、Ed25519、P-256 valid/mutated/canonical vectors 已进入 Arduino probe；Host
server key-ring/算法/unknown key/high-S/Ed25519 S 边界为 31/31，20,000 次 fuzz、14-package boundary、
ESP32/UNO R4 official package 确定性检查均通过。实际总量：UNO portable 65,060/6,932 B、UNO
`SoftwareATSE` verifier 57,220/7,564 B、Wio portable 44,316 B Flash/3,612 B `.data+.bss`；ESP32
native 为 357,163/20,816 B。以上仅证明源码、SDK、链接和静态资源；真机 known-answer、stack/heap、
无线并发和安全审计仍是 official 接线前门禁。服务器应按 product profile 固定 Ed25519 或 P-256 key
ring，App 只 relay exact bytes，不能让未认证请求选择算法。

2026-08-16 完成 R4b-3 第三片的公共 composition substrate，但没有把不完整路径伪装成 official
产品。新增固定 1,414 B `WifiControlPlaneArena`：Onboarding 使用 256/640/256 B，Cloud Session 使用
同一物理 256/640/518 B，两个阶段不重叠，因此相对“现有 Cloud 1,414 B + 新增 Onboarding 734 B”
不增加第二套静态大 buffer，节省 734 B。新增 40 B `WifiOnboardingCloudLifecycle`，顺序固定为
WiFi online → durable onboarding → Cloud Session/MQTT；注册完成前无条件关闭 cloud network gate，断网
不重做已完成认领，SameRequest/Backoff/RestartTransaction 自动重试，ReplaceEnrollmentKey 与
ReclaimOwnership 停在可诊断 Fault。状态直接取自 `WifiOnboardingFlow`，没有复制四个易分叉的布尔状态。

UNO R4 Preferences bank 在保留原 0..5 BlobId 和 key 的前提下，尾部新增 `di/oc/ce/or` 四个双槽域，
分别承载 DeviceInstanceId、Ownership Claim、Cloud Enrollment 和 Ownership Release；旧 Cloud/WiFi/
Ownership/Controller 数据编号不迁移、不误读。Host `/W4 /WX`、ASan、正常/断网/2 秒 Backoff/永久 Key
错误和 arena 同址测试已通过，UNO R4 Core 1.5.3 storage probe 实编为 68,636 B Flash、7,732 B global
RAM；40 B lifecycle ABI gate 与新增持久域均由目标编译验证。
UNO R4 Small SizeProbe 把 arena/lifecycle 作为独立测量数组常驻后为 52,260/13,472 B；ELF symbol 精确为
arena `0x586` = 1,414 B、lifecycle `0x28` = 40 B、flow `0x3c` = 60 B。该 probe 会同时常驻所有被测对象，
不能误当成最终 product composition 的全局 RAM；产品相对现有 1,414 B Cloud arena 不再增加 734 B
Onboarding 大 buffer。

该时点的审计确认 official composition 不能绕开真实验签和 envelope crypto；此缺口已由本文件顶部
第四片的软件 provider/SDK 编译门禁补齐。协议不新增 envelope algorithm，仍固定
X25519/HKDF-SHA256/AES-256-GCM；official composition 仍等待真机运行门禁，服务器/App 生产代码也尚未接入。

> 以下 2026-08-16 R4b-3 段落是 `EnrollmentKey/bek_` 实验实现的历史证据，用于判断可复用的
> Claim/Cloud/arena/journal；其 Key parser、header、错误语义和产品 API 均已被本文件顶部 R4b-2F 取代。

2026-08-16 完成 R4b-3 第二片：新增平台无关 `WifiOnboardingFlow`，用同一组 caller-owned
228/250/256 B request/response/workspace 串联 durable Claim、server-signed Ownership Grant、Receipt、
server-signed CommitAck、sealed CloudCredential request、signed Grant、HMAC Receipt/CommitAck。flow 本身
32 位不超过 60 B，不新增 heap、JSON、MCU 分支、第三份 journal 或 MQTT API；Active 重启不再发送 Key。

审计中纠正了第一版“Ownership Receipt 收空 204 即清 Claim journal”的错误：Plain HTTP 下伪造 204 会造成
设备已清 journal、Gateway 仍 Pending 的不可恢复分叉。现新增最大 250 B `OwnershipClaimCommitAck`，签名
绑定 exact receipt digest；设备验签后才 exact-clear `BOC2`。四个 `/api/v2/device-onboarding/*` endpoint、
成功 200、15 B strict Ownership/EnrollmentKey error body 与 retry/recovery class 已冻结；Cloud 阶段继续复用
现有 3xxx error contract。设备 Host 已覆盖四阶段、三处断电恢复、Active restart、Backoff、transaction restart、
transport error 和错误 Ack 不清 journal。服务器/App 生产代码和真机仍未接入。

2026-08-16 完成 R4b-3 第一片并修复一个不能继续放大的断电缺口：Ownership Claim 的 requestId/nonce
不再只放 RAM，而是先写独立 48 B `BOC2` journal；Grant 前重启、Ownership 已落盘但 Receipt 未确认、
Receipt 响应丢失均重放同一 exact request。journal 只在服务器确认 Receipt 后 exact-clear；已 Active 时禁止
取消。32 位对象为 record 36 B、store 8 B、coordinator 20 B，coordinator 比旧 RAM-window 设计更小。

同片冻结 WiFi-only `EnrollmentKey` 最小外层：`bek_` + 22..86 个 unpadded base64url 字符，通过
`Blinker-Enrollment-Key` HTTP header 携带；Ownership/Cloud canonical CBOR body 均不加入或复制 Key。
`EnrollmentKeyHttpExchange` 只包装注入的 `IHttpClient`，32 位 36 B，不拥有 WiFi/TLS/Arduino Client 或
协议 buffer。Ticket 没有被塞入同一万能 token：它必须在 R4b-4 绑定 BLE setup transcript 后单独冻结。
Ownership Release signed ACK 后的 cleanup 也已加入旧 Claim journal，避免旧 generation 阻塞再次认领。
Host C++11 `/W4 /WX`、ASan 和 20,000 次 deterministic fuzz 为 30/30；UNO R4 Small SizeProbe 为
52,244/12,020 B，`WifiOnboardingFlow` 32 位 ABI 实测 60 B；ESP32 Core 为 319,511/21,672 B，
Wio Terminal Core 为 48,236 B Flash，official ESP32/UNO R4 Plain/TLS 聚合包可复现。
收口审查同时删除了已无 decoder 的旧 local fuzz seed，并修正原有 7 个 seed 使用 `% 6` 导致后四项错路由、
Cloud seed 从未命中 Cloud decoder 的测试缺口；新 onboarding error decoder 已纳入第 7 路并重跑通过。
权威边界见 `DEVICE_V2_WIFI_ENROLLMENT_KEY_HTTP_CONTRACT.md`。该段是第二片时点记录；第三片已完成公共
arena/lifecycle 与 UNO 持久域；其后第四片已补齐软件 crypto provider，但 official 产品仍等待真机与
服务器/App E2E，当前不宣称生产可用。

2026-08-16 完成 R4b-2 第五片：冻结独立 Ownership Reset/Transfer v1。事务为 Request → server-signed
Grant → Receipt → server-signed CommitAck；Reset 可保留/清除 WiFi，Transfer 必须清除 WiFi，generation
只能精确加一。设备先持久化 Grant，再写 next-generation Unclaimed tombstone；Receipt 前保留 Cloud、
Controller 和 Network 通信能力；CommitAck 持久化后才幂等清理，并以 release journal 最后删除收口。

本片同时完成 R4b-0 的物理清理：旧 `OperationalCredential`、Enrollment/1、ClaimProof/LocalProvisioning、
`ProvisioningCoordinator`、`HttpEnrollmentDriver`、`ResetCoordinator` 和 `ProductOrchestrator` 源码、测试、
golden 与构建目标全部删除。P-256 developer identity 的公开 ID 派生被拆为独立 identity utility，通用
Noise/BLE 通道保留，不再依赖旧 enrollment wire。权威合同为
`DEVICE_V2_OWNERSHIP_RELEASE_CONTRACT.md`。

软件证据：MSVC C++11 全量 ASan/fuzz 28/28；Ownership Release exact golden 为 C++ 11/App 6/Node 11；
全量跨端 golden、official ESP32/UNO R4 Plain/TLS 聚合包均通过。UNO R4 Small SizeProbe 为
52,228/11,912 B，新增 release 对象的 32 位 ABI 为 156/8/16/28/8 B；Wio Terminal Core 为
48,236 B Flash。尚无真机，不能据此宣称 BLE 配网、射频并发、掉电或长期运行已准入。

2026-08-16 完成 R4b-2 第一片：新增平台无关 `ControllerHmacSha256Authorizer` method 2，冻结
42 B Init、53 B Challenge、33 B Proof 及角色分离的 exact HMAC transcript。Provider 每步从
`IControllerCredentialSource` 重新加载 selector/version/permissions，不在 session 复制 secret；68 B
session slot 只保留 selector 与 16 B 双 nonce。challenge 与 proof 之间发生 rotate/revoke/permission change
会立即拒绝。旧全局 authKey/SetupPSK method 1 源码和发布排除特例已删除。

`IAuthorizationProvider` 现返回 `AuthorizationDecision`，Runtime 保存 per-session permission mask：Manifest/
State 和设备上报下发要求 `Observe`，Command/Apply/Event 要求 `Control`；已认证 cloud transport 保持原有
语义。旧裸 callback 双入口已删除。C++ production、App-compatible TypeScript 和 Server Node 已共享
`controller_auth_golden_vectors.properties`。

2026-08-16 完成 R4b-2 第二片：新增 exact canonical CBOR `ControllerGrant`、
`ControllerMutationReceipt`、`ControllerCommitRequest`，以及平台无关 verifier/coordinator。Grant 只签
`SHA-256(controllerSecret)`，绑定当前 DeviceInstanceId、logicalDeviceId、ownership generation、version
CAS、permissions、短 TTL 和设备持有的 16 B control-window nonce；raw secret 不进入服务器。设备先 durable
Install/Rotate/Revoke 再返回 receipt；Install/Rotate receipt 由新 secret HMAC，App 还需完成 Method 2 后
幂等 Commit。Revoke 明确无目标 secret proof。服务器 Commit 只收敛 registry/审计，不反向控制设备激活。

本片同时修正了一个会削弱无 RTC 平台新鲜度的接口：`apply()` 不再接受调用方提供的 nonce；coordinator
必须先 `beginControlWindow()` 保存设备本次生成值，不同 nonce 在窗口结束前返回冲突。输入、共享 256 B
workspace 与输出重叠也会拒绝；Grant transcript 严格上界 242 B，Grant/Receipt/Commit body 上界
298/145/126 B 由最大值测试锁定。新增 12 项 Controller control 三端 exact golden 和 Host 状态/重试/错误
门禁。该段是第二片时点记录；Ownership Claim、Cloud Credential Enrollment 与 Ownership Release 已在
后续第三至第五片完成；当前仍不声称 LocalBLE 或真实 BLE composition 已完成。

2026-08-16 完成 R4b-2 第四片：新增独立 `CloudCredentialEnrollmentContract`，长期 32 B HMAC secret
不再像旧 Activation 那样进入 raw request，而由设备生成后使用 pinned server X25519 public key、
HKDF-SHA256 与 AES-256-GCM sealed envelope 注册。服务器签 Grant；设备在 durable 安装 `CloudCredential`
后发送 HMAC Receipt；服务器以 HMAC CommitAck 收敛，Plain HTTP 上也不能伪造 ACK 让设备清理 journal。

掉电事务使用独立 136 B `BCE2`，不把 requestId/ephemeral private/grant digest 混入 116 B Active `BCS2`。
32 位 `CloudEnrollmentRecord/Store/Sealer/Coordinator` 精确为 128/8/8/32 B，coordinator 不缓存 secret 或
workspace。`INoiseCryptoProvider` 只继承新的通用 `IX25519AesGcmCryptoProvider`，Noise 自身改用共用 RFC
5869 HKDF，既有 golden 无字节变化。Host 30/30；新增 Cloud 三端 exact 为 C++ 15/App 5/Node 19，Node
使用真实 X25519/AES-GCM。Arduino 实编为 ESP32 320,967/21,672 B、UNO R4 69,296/7,664 B、Wio
48,236 B，R4 Small SizeProbe 52,148/11,696 B，ESP32 Noise adapter 356,651/20,800 B。该时点 UNO R4/Wio
真实 crypto adapter 尚未完成；本文件顶部第四片已补齐软件 provider，profile HTTP/BLE driver 与真机仍未完成。权威合同见
`DEVICE_V2_CLOUD_CREDENTIAL_ENROLLMENT_CONTRACT.md`；后续第五片已完成独立 Ownership Reset/Transfer
并整组删除旧 OperationalCredential/ProvisioningCoordinator/HttpEnrollmentDriver。

第二片软件门禁为 MSVC C++11 26/26、三端全量 golden、架构检查、pre-enrollment、12-package official
aggregate；Aily Builder 全源码复编通过 ESP32 318,999/21,672 B、UNO R4 69,304/7,664 B、Wio Terminal
48,236 B Flash；Aily 本次实际解析 ESP32 Arduino core 3.3.0，因此该数字不冒充 3.3.1 资源结果。新增
control codec 是平台无关 C++，但这些仍只是编译/软件证据，不代替真机 BLE、掉电和
栈峰值测量。

第一片软件门禁为 MSVC C++11 24/24、架构检查、pre-enrollment、12-package official aggregate、
C++/App TypeScript/Server Node 全量 cross-language golden，以及 ESP32、UNO R4、Wio Terminal、ESP8266、
RP2040、nRF52840 六平台 `CompileV2`。尚无真机，因此这些结果只证明源码/ABI/协议一致性，不升级 BLE
射频、配网、掉电、并发与长期稳定性准入状态。

普通/Advanced `productId`、HELLO productId 与 ClaimStart productId 已撤回。旧 P-256 Developer identity、
Enrollment/1、永久 setup PSK 和旧 ProductOrchestrator 已从源码、测试和正式发布归档删除；独立 P-256
Developer identity 保留，但已解除对 Enrollment/1 的依赖。
当前已取消“一个 DeveloperSetupCode”假设，改为 WiFi-only ManualDeviceKey、App 配网 EnrollmentTicket、
共同 Ownership Claim、独立 Cloud/Session credential、独立 Controller credential，以及 WiFi-only、
WiFi+BLE、BLE-only、LocalBLE 的失败恢复；权威输入是
`DEVICE_V2_WIFI_BLE_ONBOARDING_DECISION.md`。但不需要把所有设备侧工作都阻塞在三端 wire 评审上：先完成
不依赖 wire 的 `DeviceInstanceId` 和持久域拆分，再冻结 exact contract；产品入口仍必须等真实 coordinator
可用后再实现。

当前首批代码已新增平台无关 `DeviceInstanceId`：16 B 非零 CSPRNG 值、固定 28 B crash-atomic CRC record
和 `dev_` + lowercase hex 文本表示。存在有效记录时不调用随机源；记录损坏时不自动重生，从而避免同一
物理设备静默变成新资产。该 ID 是公开标识，允许 PlainFlash，并必须跨 Network/Application/Ownership
reset 保留。MSVC 全量在加入 Controller control 第二片后为 26/26；移除旧 ESP32 developer-identity 发布包后，正式 package boundary
为 12 包；删除旧 Config 后同一 WiFi-tag facade
在 ESP32、UNO R4、Wio 分别为 319,107/22,320 B、69,880/8,292 B、48,820 B Flash（前两项为
Flash/global RAM，Wio 未报告 RAM）。本批 pre-enrolled official WiFi 当前重编为：ESP32
984,119/49,952 B、UNO R4 122,456/12,704 B；Wio 当时 Core 为 48,204 B Flash。前两项包含独立
Ownership store，但按 WiFi-only capability 不实例化 Controller store，仍是 pre-enrolled Cloud/WiFi
探针而非完整 Claim/Controller coordinator。

设备 Active 后的持久登录根命名 CloudCredential，每次开机换取短期 SessionCredential；
ManualDeviceKey 只在手动 Initial/Recovery bootstrap 使用，Ticket 只在 App 配网 Pending 使用；两者均不参与
日常登录。App 与服务器仓已分别新增
`docs/DEVICE_V2_ONBOARDING_IMPLEMENTATION_PLAN.md`，避免生产团队继续按 legacy authKey 或旧
Enrollment/1 开发。

本批已把这一边界落实到代码：`CloudCredential` 在当前 32 位 ABI 为 108 B RAM 对象（≤112 B 门禁），`CloudCredentialStore` 使用
116 B `BCS2` CRC record；它只保存 suite、logicalDeviceId、generation、credentialVersion 和 32 B
authentication secret。`HttpCloudSessionProvider` 只读 `ICloudCredentialSource`，工厂 BFB2、ESP32 NVS、
UNO R4 Preferences 与 Python exact vectors 均消费同一对象。旧 148 B `BOP2/OperationalCredentialStore`
不再进入 `BlinkerV2Advanced.h`、official composition 或发布 ZIP；BFB2 仍保持 228 B wire 不变，避免把
设备持久化布局误当成工位传输合同。

随后完成独立 `OwnershipRecord/OwnershipRecordStore/IOwnershipSource`：当前 32 位 ABI 为 104/8 B，固定
112 B `BOS2` 只保存已验证 Active 结果的 generation、logicalDeviceId 与 exact signed grant SHA-256。
它不提前冻结 owner/tenant/签名算法/CBOR。授权 reset/transfer 通过 generation CAS 写入
`Unclaimed + nextGeneration` tombstone，禁止普通 clear 丢失防回滚 floor。2026-08-16 Claim v1 冻结后已
收紧为 fresh 只能 generation 1、tombstone 只能 exact generation、Active owner 之间不能直接覆盖，禁止
跳过 floor。ESP32 使用 `bl_v2_owner/binding`，UNO R4 Preferences bank 使用
独立 `ow0/ow1`，均不与 Cloud/WiFi 混存。

## 已退出批次

### R0：架构和资源底座

- Core 与平台 SDK、MQTT、HTTP、BLE、storage、crypto 通过窄接口解耦；
- 外部库不再复制进核心 Arduino 包；adapter 独立打包；
- Small/Standard/Large 固定资源 profile、compact runtime slot 与共享 state arena；
- Manifest streaming hash、Manifest 分页、State snapshot 分页；
- BLE 4 B session-local 分片头。

### R1：协议与运行时

- BBP/2 fixed frame + restricted CBOR；keyed baseline + ManifestAccept 后的 session-local ID；
- HELLO/auth/reliable delivery/revision/ACK/Error；
- cloud MQTT 与 local BLE 多 transport hub；
- identity、credential transaction、reset、WiFi journal、portable Noise 和 cloud session seams。

### R2：业务模型正式纠偏

- 放弃未发布 TLV branch 兼容和设备 WidgetCatalog；
- FieldSpec 收敛为 Property/Action/Event；
- 普通 API 使用 `BLINKER_PROPERTY/ACTION/EVENT` + 全局 `Blinker`；`BLINKER_DEVICE` 仅留 Advanced；
- 写意图、Action、Event、权威状态和原子 PATCH Apply 分离；
- README、关键词、wire spec、golden、host tests 与核心示例迁移。

## R3：多平台编译与 Core/adapter 结构清理（当时软件门禁已退出）

退出条件：

- ESP32、UNO R4、Wio Terminal 对当前源码编译通过；
- host 全目标与三端 golden 通过；
- `src/tests/examples` 不再引用旧 WidgetCatalog/TypedWidget/组件宏；
- release manifest 不包含已删除实现；
- `AdvancedCoreModel` 只保留底层 model compile probe；普通 API 由 `CompileFacade` 门禁；
- 32 位 interaction/Device static_assert 通过，并记录 UNO R4 Core 全局 RAM；精确对象串口 SizeProbe
  留到有真机批次。

当时结果：17/17 host tests、三端 BBP2 29/6 golden、package boundary 均通过；原 CorePortable 在
ESP32、UNO R4 WiFi、Wio Terminal 以相同 API 编译。32 位 interaction static_assert 通过；无真机，
因此运行峰值和无线功能不属于本批结论。

这里的“已退出”只针对 dev_edu/Widget/bundled dependency 清理和当时的 Core 编译基线，不表示后续加入的
Enrollment/1、P256 developer identity、PSK BLE 实验已经成为正式发布合同。把 R3 简写成“发布树无旧
引用”曾掩盖 `src` 中残留的旧 V2 onboarding；该问题现已在 R4b-0 物理删除阶段收口。

## 当前批次 R4：业务 API 已收敛，接入模型重新冻结（进行中）

R4a 与 R4b 有先后关系，但 R4b 内部还必须按依赖拆开：

- **R4a 用户/Agent API**：普通 Sketch 只出现 `<BlinkerWiFi.h>` 或 `<BlinkerBLE.h>`、Property/Action/Event、全局
  `Blinker.begin()/run()` 和 void callback；不得出现 `Device/Product/Client`、`BlinkerPlatform`、
  transport/buffer 或 `blinker::Result`。
- **R4b-0 旧 onboarding 隔离**：停止新依赖旧 Enrollment/1、永久 setup/local key、P256-derived
  developer ID 和旧编排器；
- **R4b-1 身份/持久域**：DeviceInstanceId、Cloud、Ownership、Controller 与 Network journal 已完成软件基线；
- **R4b-2 exact control-plane**：Claim/Cloud/Controller/Release 已冻结并实现设备软件基线；R4b-2F 重构
  ManualDeviceKey parser/proof/version/reveal/rotate，并新增 RecoveryGrant codec/journal/golden，当前待实施；
- **R4b-3～R4b-6 coordinator/composition**：WiFi-only、BLE setup substrate、durable saga、WiFi+BLE internal
  graph/fault gate、LocalBLE exact pairing/application/shared lifecycle，以及 R4b-6C 双平台四个 Platform
  BLE/LocalBLE internal package/resource 均已完成软件门禁；
- **R4b-7 facade/package**：在真实路径可用后冻结统一 begin 和 QuickStart。ESP32、UNO R4 等只替换
  adapter，不改变业务代码。

目标普通入口为 mode selector 和一个 tag-free `begin()`；selector 在 Arduino 扫描依赖前确定发布产品，
分别链接可裁剪 composition，不能退化为把三套依赖全部装入一个运行时 enum。默认 MQTT adapter
继续使用 PubSubClient，Advanced API 允许注入其他 adapter，但不制造第二套业务 API。

历史 `ProductOrchestrator` 曾验证 lifecycle、失败回滚、停止与重启，但其 onboarding 状态机依赖已废弃的
Enrollment/1/setup PSK，现已退出公开与发布 API，不能再称为正式底座。保留的是 `IProductLifecycle`、
`DirectBleLifecycle` 等窄接口；当时的 `WifiCloudLifecycle` 后来已被统一 staged lifecycle 替代并删除。

R4a-1～R4a-3 已落地：`Client::prepare()` 与 data-plane `begin()` 已分离；
单字段 report 和 Advanced `StateUpdate` 可在 prepared/offline 状态提交 StateStore、推进 revision，
联网后用 snapshot 收敛；Event 离线返回 `NotConnected`。host 19/19、ASan 与 200,000 次 deterministic
fuzz 已通过；普通入口已统一为编译期入口头 + 全局 `Blinker`，void/checked callback 与
`Blinker.report(...)` 原子批量包装也已实现。fake-lifecycle fixture 只验证公开 WiFi/BLE 两种 tag-free
语法。2026-08-22 ESP32 WiFi+BLE 已以 `<BlinkerWiFiBLE.h>` 进入正式设备 facade 和同一个 ZIP；该静态
API 完成不等于 App/Service/真机生产准入完成。

本批 facade Aily Builder 结果：ESP32 318,835 B Flash / 22,336 B global RAM；UNO R4 WiFi
70,032 B / 8,308 B；Wio Terminal 49,012 B Flash（该输出未给 global RAM）。fixture 不含真实 radio，
只验证普通语法、静态 Session、callback 和批量状态路径。构建时必须隔离同名旧 Blinker 库，防止
`<BlinkerWiFi.h>/<BlinkerBLE.h>` 被错误解析到其他同名 package。

WiFi/BLE selector 矩阵使用同一 fixture，通过 `BLINKER_FACADE_PROFILE` 选择入口头；未选择的 Session
不会实例化。历史 WiFiBLE fake-tag 数字只证明当时门面无额外状态，已从当前矩阵移除，不能作为真实 radio
composition 的资源证据。

本批还复核了现有 `CompileV2` Core/API profile：加入 BFB2 后 ESP32 clean Arduino CLI 为
322,371 B / 21,672 B；UNO R4 WiFi、Wio Terminal 最近结果为 73,788 B / 7,644 B、52,780 B Flash
（Wio 的 global RAM 未由该次输出提供）。这些结果验证底座没有被 prepare/offline 修改破坏，不能替代完整
WiFi/BLE product size 或真机门禁。

R4a 退出条件：20～30 行新手示例可表达控制/传感器/Action/Event；三 profile 只允许 `begin()` 的连接
意图不同；示例编译和 Agent 生成门禁通过；普通 API 不泄漏内部类型。业务 facade 主体已可供 R4b
内部使用，但 connectivity 参数须到 R4b-7 才冻结。不得把当前完整
WiFi+BLE 编排器简单改名成 ESP32 facade，也不得用它冒充 BLE-only/cloud-only profile。

详细合同见 `DEVICE_V2_USER_API_AND_LLM_QUICKSTART.md`。

R4b 早期曾完成首个 ESP32 WiFi pre-enrolled 聚合包，并新增平台无关 `WifiCloudLifecycle`，只编排已有
`WifiConnectionLifecycle + ManagedMqttTransport + Client`，不持有 BLE、enrollment、native SDK 或
额外协议 buffer；32 位 ABI 门禁为 24 B。host 已覆盖启动、WiFi 上线、cloud ready、掉线降级和停止。
新增源码随整库在 ESP32、UNO R4、Wio 递归编译通过，global RAM 分别仍为 22,336 B、8,308 B、
未由 Wio Aily 输出；fake facade Flash 分别为 319,083 B、70,040 B、49,012 B。它只是 R4b WiFi-only
公共生命周期底座；其上已新增 release-only `Esp32WifiComposition`，静态组装 NVS、WiFi、HTTP、
Cloud Session、PubSubClient 与 MQTT，普通 QuickStart 只 include `Blinker.h`。聚合构建器只复制所需
Core/adapter/glue，冻结全包 Small profile，生成确定性 ZIP，且不携带 docs/tests/旧实现或第三方源码。
该段为历史演进记录：当前完整 `WifiOnboardingCloudLifecycle` 已覆盖 pre-enrolled Active fast-path，旧
`WifiCloudLifecycle` 与专属 probe 已删除；最新 composition/资源结论以本文顶部为准。

用户修订后日常 cloud-session/MQTT 不再全局强制 TLS：官方包的 `cloudSessionTransport`、`mqttTransport` 默认 Plain，
可分别选择 TLS 并提供 CA；禁止运行时静默降级。Core 接受两种枚举并拒绝 session response 与固件
MQTT profile 不一致。以下为迁移到独立 CloudCredential 前一阶段的历史大小：Active transaction
字段清零修订后，ESP32 3.3.1 QuickStart 的 Plain/TLS
编译结果分别为 982,707/49,904 B 与 1,072,123/50,496 B（Flash/static RAM）；
19 项 ASan host 测试与 5,000 次 fuzz 通过。当前仍是 `education-plain-nvs` pre-enrolled profile。
制造期工具现已实现：只接收服务器已发行 Active CloudCredential 和 WiFi，生成
`bl_v2_cloud/credential + bl_v2_net/wifi_sel + wifi_0` 的 fresh-device NVS image/脱敏 receipt；Python
与 C++ `BCS2` store exact、反向解码、负例和 BFB2 exact 已通过。116 B CloudCredential record 不再含
enrollment-only claimId/activationKey。生产服务器端点和真机 E2E 仍不存在，不能宣称量产安全/发布完成。
该制造期 pre-enrollment 路径仍要求受控 secret 交付；DIY 首次 Cloud 注册已另行冻结为 X25519/AES-GCM
sealed envelope，不再以 TLS 作为长期 secret 的唯一保密层。详见
`DEVICE_V2_OFFICIAL_WIFI_PACKAGE.md` 与 `DEVICE_V2_ESP32_PRE_ENROLLMENT.md`。

UNO R4 official WiFi composition 现已完成软件基线：ESP32/UNO R4 共用
`WifiComposition<Platform>`，平台文件只保留 native Client、station、CA 和 storage wiring。UNO R4
不占用 RA4M1 的 8 KiB Data Flash/EEPROM，而在 WiFiS3 connectivity module Preferences 上使用双 key
generation/CRC/tombstone journal，始终报告 `PlainFlash`。以下同样是迁移前的历史大小：Plain/TLS QuickStart 分别为
122,232/12,656 B 与 125,424/12,688 B（Flash/static RAM）；ESP32 Plain/TLS 共享模板回归为
982,727/49,904 B 与 1,072,139/50,496 B。当前迁移后 MSVC 22/22 host 与
12-package boundary 通过。平台无关 `education-plain` 制造输入、228 B BFB2、
UNO R4 USB Serial 工厂草图/host 工具已实现；草图为 66,700/7,924 B（Flash/static RAM），同一 bundle
  幂等重试且不同 identity 不覆盖。尚缺真机 USB/掉电/联网/长稳。此前写成“下一软件批次进入 BLE-only/
  WiFi+BLE composition”会跳过 Ownership/Controller 与 exact control-plane，现已纠正：OwnershipRecord
  与 ControllerCredential 已完成；下一批进入 R4b-2 exact control-plane，之后才允许实现新 coordinator。详见
`DEVICE_V2_FACTORY_CREDENTIAL_BUNDLE_AND_UNO_R4_INSTALLER.md`。

R4b 组装前又关闭了一项依赖放大问题：`device-cloud-session/1` 原本要求每次日常登录注入
`IFactoryIdentity` 并重复发送 `physicalDeviceId`，但 Active CloudCredential 已由服务器绑定
到 physical asset。正式修订为未发布的 `/2`（9 对 request、8 项 HMAC transcript），provider 只依赖
只读 `ICloudCredentialSource`/random/HTTP/clock。MAC identity 实验未保留；factory identity 继续只服务安全认领与
ownership reset。C++、TypeScript、Node 的 cloud-session golden 已同步覆盖 Plain/TLS response；旧
Enrollment/1 golden 仅是历史软件探针，必须由新的 Ownership/Cloud/Controller exact vectors 替换。

2026-08-16 清理门禁：旧 `WifiBleProductProbe`、`Esp32SecureProductProbe`、
`CompileEsp32IdentityAdapter` 与整个 `adapters/esp32-identity` 已从工作树、compile matrix、workflow 和
release manifest 删除。随后 Ownership Release exact golden 覆盖等价失败/恢复行为，旧
Enrollment/1 源码和历史测试已整组物理删除；P-256 developer identity 作为独立可选能力保留。

同轮修正了仍测量旧 onboarding 对象的 UNO R4 `SizeProbe`：现在只覆盖 active public/release 边界，
Small 本批加入 48 B `ControllerControlCoordinator` 门禁后实编为 52,124/11,464 B；Standard/Large
上批 Controller credential 门禁结果为 52,148/13,772 B、52,148/22,964 B，本批未重复
（Flash/static RAM）。32 位 ELF 实测 Property/Action/Event 为
16/16/8 B，CloudCredential/Store 为 108/8 B；此前文档中的 12 B interaction gate 和 112 B
CloudCredential“固定大小”均不准确，已分别改为真实值与跨 ABI 上限说明。

随后全局审计又发现 active `CompileDeviceKeySession` 和 `CompileEsp32StorageAdapter` 仍引用旧
OperationalCredential 接口；现已改为只读 `ICloudCredentialSource` 与 `CloudCredentialStore`，
active Arduino fixtures 对旧 onboarding 类型为零引用。ESP32 storage 在加入独立 ownership blob 后为
313,183/21,256 B；
ESP32/UNO R4 cloud-session 为 586,672/28,832 B、85,736/11,304 B（Flash/static RAM）。

本轮 Ownership host test 覆盖 112 B exact vector、CRC 损坏、Active 幂等、禁止直接覆盖、reset CAS、
Unclaimed tombstone、旧 generation 回滚拒绝、写失败和 full-erase 边界。平台 storage compile 为 UNO R4
59,796/7,636 B、ESP32 313,183/21,256 B（Flash/static RAM）。BFB2 暂不扩展 Ownership：它仍是 Cloud/WiFi
pre-enrollment probe，不是完整 ownership onboarding；必须等 R4b-2 exact receipt contract 冻结后再决定
新的工厂 bundle，而不是把本地 `BOS2` 当成服务器 wire。

随后完成独立 `ControllerCredential/ControllerCredentialStore/IControllerCredentialSource`：设备不保存
App 角色字符串，而保存 `Observe/Control/ProvisionNetwork/ManageControllers` 权限位；16 B controllerId
与 32 B HMAC secret 组成 64 B RAM 记录。当前固定 140 B `BCT2` 是按 controllerId 排序的两槽
canonical table，Store 本身为 8 B，不常驻两份记录；它支持 verified install、credentialVersion CAS
rotation、单 controller revoke、Ownership generation 隔离和 reset 全清。Local 域固定 generation 0，平台
域必须绑定非零 Ownership generation。ESP32 使用独立 `bl_v2_ctrl/table`，UNO R4 在既有 blob 编号后追加
`ct0/ct1`，不改变 Cloud/WiFi/Ownership 兼容槽。旧 `PskHmacSha256Authorizer` 的全局 authKey/SetupPSK
派生不接入新表；R4b-2 method 2 三端向量落地后，其源码和发布排除特例均已删除，不再保留未发布兼容层。
当前 storage fixture 为 ESP32 历史 314,699/21,360 B、UNO R4 本批 68,636/7,732 B；
纯 WiFi official composition 不实例化无用的 Controller store，其当前 Plain 为 984,119/49,952 B、
122,456/12,704 B（Flash/static RAM）。Controller 表不常驻 RAM；一次写入含 encode + read-back 校验的
最坏有界栈工作区约 1 KiB，仍须真机 stack watermark。Controller 日常 auth transcript 已由
`DEVICE_V2_CONTROLLER_AUTH_METHOD.md` 冻结；安装/轮换/撤销 exact control-plane 已由
`DEVICE_V2_CONTROLLER_CONTROL_CONTRACT.md` 冻结，尚待接入 BBP/2/Noise 与真实 composition。

## R5：App/服务器生产对接

R4b-2 第三片当时完成独立 Ownership Claim v1：request/grant/receipt/commit 最大 63/243/165/183 B，签名 transcript 最大 199 B，共用 256 B workspace；fresh 仅 generation 1，tombstone 只接受 exact generation。该时点 coordinator 为 56 B RAM request window，Host 只证明“调用方仍持有 exact Grant”时能重建 receipt，尚未形成 profile 可执行的掉电恢复。该缺口已由本文件顶部 R4b-3 第一片的 48 B `BOC2`/20 B coordinator 修正。第三片历史编译结果为 ESP32/UNO R4/Wio 319,459/21,672 B、69,296/7,664 B、48,236 B Flash，UNO R4 Small 52,124/11,520 B。

R4b-2 第四片已完成独立 Cloud Credential Enrollment v1：最大 228/248/190/190 B 的 sealed
request/signed Grant/HMAC Receipt/HMAC CommitAck，136 B `BCE2` 掉电日志，32 B coordinator 和 256 B
caller workspace。长期 secret 使用 pinned server X25519 + HKDF-SHA256 + AES-256-GCM，即使 Plain HTTP
也不裸传。当前 Host 30/30；ESP32/UNO R4/Wio 为 320,967/21,672 B、69,296/7,664 B、48,236 B，
R4 Small SizeProbe 52,148/11,696 B。该段是第四片时点记录；后续第五片已完成 Ownership Release 并删除
旧 Enrollment/1 整组实现。

- **R5-1 codec**：R4b-2F 完成后，Server 单一 protocol package + App 独立 codec，覆盖全部 shared exact/invalid/fuzz；
- **R5-2 control plane**：Gateway V2 schema/repository 与 Enrollment/Recovery/Ownership/Cloud/Controller/Release；
- **R5-3 cloud session**：SessionCredential、topic ACL、TTL/revoke、Gateway/Broker identity；
- **R5-4 data plane（进行中）**：Broker 已完成 HELLO/Manifest、State snapshot/latest-state 与 Report Patch，
  Gateway 已提供 owner Manifest/State 读取；normalized router、控制/event/history 与 App stream 尚未完成；
- **R5-5 App/page**：四种 onboarding reducer、secure BLE、source-lane state、PageSchema/WidgetRegistry；
- **R5-6 E2E**：fake device/App/服务故障注入、shard migration、灰度和真机交接。

Legacy JSON 仅在独立 adapter/namespace 兼容，不进入新设备固件、V2 session 或 credential 表；adapter 可以
向共同业务层输出带 wire-family 标记的 normalized event，但不得伪造 V2 Manifest/revision/readiness。Direct
BLE 和 cloud 不得共享 auth、sequence、Field ID、revision 或 source freshness。

共同任务见 `APP_SERVER_BBP2_REFACTOR_INPUT.md`；生产仓内执行方案分别见
`D:\codes\blinker-app\docs\DEVICE_V2_ONBOARDING_IMPLEMENTATION_PLAN.md` 与
`D:\codes\diandeng-service\docs\DEVICE_V2_ONBOARDING_IMPLEMENTATION_PLAN.md`。

## R6：真机与发布

- ESP32 WiFi+BLE provisioning/control/cloud 实机、峰值 SRAM、断线、24 h；
- UNO R4/Wio 的实际产品 profile 按硬件能力验收，不能把 compile pass 等同于功能齐全；
- enrollment、ownership reset、credential rotation 和 broker session termination E2E；
- 用户/Agent QuickStart、错误诊断、Arduino library package/CI；
- 达标前保持 `BlinkerDeviceV2` 名称，不替换线上 `Blinker`。

## 禁止回归

- 不恢复设备 Widget 类、WidgetCatalog 或运行时 schema 学习；
- 不把 PageSchema/JS/layout/style 放入 Manifest；
- 不复制 ArduinoJson、MQTT、WebSocket、BLE 库进 Core；
- 不建立每 MCU 一套 Product API；
- 不以 HTTP 取到凭据代表 MQTT online；
- 不把 ManualDeviceKey/Ticket 当作 deviceId、CloudCredential、ControllerCredential 或重启登录 key；
- 不重新建立所有 profile 共用的万能 SetupCode；
- 不以 reference golden 代表生产 App/服务器已经接入。

## DKEY-2C：manual WiFi internal composition 与最小平台层（2026-08-17）

本批把 DeviceKey Core 从独立 provider 接到完整设备图，但未提前修改公开 facade 或生产 route：新增
`ConfiguredDeviceKeySource`、精确 `DeviceKeySessionArena` 与平台无关 `WifiCloudLifecycle`；ESP32/UNO R4
旧 `WifiPlatform` 改为继承最小 network/storage platform，新 DKEY 包因此不再携带 Ownership、CloudCredential
协调器所需的 crypto adapter。旧兼容图继续使用原派生平台，避免迁移中间态失去可编译消费者。

UNO R4 32 位 ELF 将 configured source/arena/lifecycle 锁定为 `40/1260/20 B`，Provider 保持 280 B。
Small SizeProbe 为 `52,428/16,616 B`，相对上一批多出的 1,320 B 完全等于三个新 ABI 测量数组。
完整 Plain internal 图为 ESP32 `986,287/48,360 B`、UNO R4 `118,164/11,200 B`；同轮旧兼容图为
`1,023,823/50,576 B`、`159,140/13,304 B`。UNO R4 新/旧 composition singleton 为 3,532/3,964 B。

最终门禁为普通 Host 41/41、Windows ASan 41/41、ASan 200,000 deterministic fuzz、architecture、official
aggregate boundary，以及 ESP32/UNO R4 新旧四个 official clean compile。下一设备批进入 ESP32 WiFiProv
Security1 资源/adapter 与持久 `DeviceKeyStore` 安装路径；Service/App 可以延后，但仍阻断公开 facade 切换和
真实登录/轮换 E2E。

## DKEY-2D：DeviceKey 配网安装与 ESP32 WiFiProv Security1（2026-08-18）

本批没有把短期 EnrollmentTicket 变成设备重启登录 Key，也没有继续扩展旧 8-operation setup saga。新增
transport-neutral `DeviceKeyProvisioningEndpoint`：`GetInfo` 返回 public instance id/Key 状态，`Install`
严格接受一个 43 字符 DeviceKey。同一 setup session 只允许一次不同 Key，响应丢失时相同 Key 可幂等重试；
当前 32 位 ABI 上限 16 B；对象不保存 Ticket、第二份 Key 或动态 buffer。

ESP32 新增独立 `esp32-wifiprov` adapter，直接使用底层 `network_prov_mgr` 的 BLE Security1、自定义
`blinker-config` endpoint 和 stop/deinit lifecycle。WiFi candidate 进入现有 Blinker 双槽 journal；同时修复
`Esp32WifiStation`，在 `WiFi.begin()` 前强制 `WiFi.persistent(false)`，使 SDK WiFi NVS 不再成为第二个
持久密码源。adapter 作为可选 package 登记，但尚未加入公开 facade 或默认 official composition。

真实 ESP32 3.3.1 P2+DeviceKeyStore+WifiCredentialStore clean build 为 `1,689,811 B Flash / 58,268 B global
RAM`。默认 `1,310,720 B` app slot 超限；`min_spiffs` 双 OTA slot 为 `1,966,080 B`，剩余 276,269 B
（约 14.1%）。因此 WiFiProv 可以继续作为 ESP32 候选，但不能无条件进入默认包；该结论随后由下方 WP-P3
完整 DKEY cloud/PubSubClient/BBP2 composition 继续验证。真机 heap/stack、资源回收、
配网失败/掉电以及 App/Service pending N+1 激活仍未验证。

设备软件门禁已复验：普通 Host 42/42、Windows ASan 42/42、architecture check、official aggregate package
boundary 与 UNO R4 clean SizeProbe 全部通过。UNO R4 当时为 `52,436 B Flash / 16,628 B global RAM`；endpoint
旧 12 B 测量发生在 P6 access pointer 接线前，当前只冻结 `<=16 B` 门槛。该数字与 ESP32 WiFiProv 平台栈
资源必须分开解释。

## WP-P3：WiFiProv 到 DeviceKey cloud/BBP2 完整设备图（2026-08-18）

现有 manual `DeviceKeyWifiStack` 原先内置 Platform 和 `ConfiguredDeviceKeySource`，无法消费 WiFiProv 写入的
持久 `DeviceKeyStore`。本批改为注入 Platform 与 `IDeviceKeySource`：手动 ESP32/UNO R4 和自动 ESP32
composition 共用唯一的 HTTP challenge/session、PubSubClient、`MqttFrameTransport` 与 BBP/2 runtime，没有复制
第二套 cloud stack。未使用的 `deviceKey()` accessor 同批删除。

新增 internal ESP32 composition 在缺少 Key/WiFi 或启动时显式 force 时进入 Security1；完成并 deinit
provisioning manager 后才启动 cloud lifecycle。DeviceKey NVS 损坏会 fail closed，不会被当作首次设备覆盖；
WiFi candidate 仍由现有双槽 journal 确认。P3 package 不包含旧 crypto adapter，ELF 符号也不包含
CloudCredential、EnrollmentKey、Ownership 或旧 Crypto provider。

最终 clean build：P3 `1,758,803 B Flash / 62,964 B global RAM`，`min_spiffs` 的 1,966,080 B 双 OTA slot
剩余 207,277 B（约 10.54%）；手动回归为 ESP32 `986,483/48,368 B`、UNO R4 `118,156/11,208 B`。
P3 当时只通过软件/静态资源门禁，尚无真机 heap/stack/100 次循环证据；因此后续 P4 Direct BLE 与 P5 TLS
被要求独立 spike，不在该紧余量图上盲目叠加。

## WP-P4：单 Bluedroid Direct BLE 组合与准入结论（2026-08-18）

P4 审计确认 Arduino-ESP32 3.3.1 默认 WiFiProv BLE 和内置 `BLEDevice` 都使用 Bluedroid；现有 official
Direct BLE 却使用外部 NimBLE-Arduino。顺序运行仍会把两套 Host 链进固件，因此新 DeviceKey 主线禁止复用旧
NimBLE WiFi+BLE composition。P4 改用内置 `esp32-ble` adapter，WiFiProv 配网时保留 BLE controller memory，
manager 完整 deinit 后再启动 Direct BBP/2。

本批把 P3 状态机抽成共享模板，P3 WiFi-only 与 P4 Direct BLE 只通过一个小扩展区分，不复制 DeviceKey、HTTP、
MQTT 或 provisioning 生命周期。Direct 扩展复用同一个 `Client/StateStore`，只增加 BCT2 controller store、
4 B fragment transport 和 Method 2 authorizer。`Esp32BleLink` 现在只销毁自己初始化的 singleton，并用
`BLEDevice::deinit(false)` 保持同次上电可重启；架构门禁禁止该 adapter 再出现 `deinit(true)`。

重构时的 P3 为 `1,758,819/62,964 B`；P4 初次 clean build 为 `1,821,783/65,776 B`，增量
`62,964 B Flash / 2,812 B global RAM`。1,966,080 B app slot 只余 144,297 B（约 7.34%），未通过 10%
发布门槛。ELF 审计为 NimBLE/Esp32NimBle/ble_hs/nimble_port 全部 0，存在 Bluedroid、WiFiProv、DeviceKey
session 和 Controller Method 2；旧 CloudCredential/Ownership secure graph 未进入。

当时 P4 只作为 P6 比较基线且不切公开 facade；P6/P5 完成后，其无消费者的 product wrapper/profile/probe 已在
同日删除，失败结论只保留在文档。此处记录的 Controller Grant BBP/2 缺口已在 2026-08-18 的 v2 Core 批关闭；
当前这两项已由 accessEpoch + atomic Bootstrap 关闭；2026-08-22 P6 静态资源重测也已关闭，剩余阻断为 App/Service production、
Plain MQTT frame authentication 与真机 E2E。

本批最终门禁：MSVC AddressSanitizer host 42/42、200,000 次 deterministic fuzz、pre-enrollment exact、
architecture、16-package boundary、包含 P4 profile 的 official aggregate deterministic verifier、P3/P4 ESP32
编译与 P4 ELF Host 审计全部通过。`asan-ubsan` 组合不在 MSVC 上伪执行；UBSan 继续由 Linux CI 的 Clang job 承担。

## WP-P6：SoftAP Security1 + NimBLE Direct 静态准入（2026-08-18）

P4 空间失败后没有通过改分区或取消 OTA 规避，而是先修正结构边界：`Esp32WifiProvAdapter` 只保留 manager、
Security1、custom endpoint 和 credential transaction；共享 composition 新增 `ProvisioningPolicy`，scheme
的选择不再由 Direct BLE extension 的 `retainsBleMemory()` 间接决定；该多余接口已删除。最初把 BLE/SoftAP
factory 拆成公共 `.cpp` 即使最终 section 可被删除，仍会触发 Arduino 全库依赖扫描；2026-08-23 已进一步收口为
`Esp32WiFiProv.ipp` 编译期互斥选择器，使未选 scheme 连依赖发现阶段也不可见。

P6 使用 SoftAP Security1 承载首次 WiFi/DeviceKey 安装，manager deinit 后启动 NimBLE Direct。AP
`serviceKey`（WPA 密码）与 Security1 PoP 分开传入；二者都不是 DeviceKey。最终精简后的 P6 Plain 为
`1,375,887 B Flash / 60,856 B global RAM`，在 `1,966,080 B` 双 OTA slot 中剩余 `590,193 B`
（30.02%），通过 10% 静态门槛。P3 Plain 同轮最终为 `1,758,907/62,964 B`。P6 唯一消费者需要的
NVS/Controller/BLE lifecycle 已收回自己的 translation unit；没有继续保留一个伪通用、实际只有单消费者的平台模板。

P6 ELF 存在 `network_prov_scheme_softap`、NimBLE/ble_hs、DeviceKey session、PubSubClient、BBP/2 与
Controller Method 2；`network_prov_scheme_ble`、protocomm BLE、`BLEDevice::`、Bluedroid、旧
CloudCredential/Ownership/Noise product graph 均为 0。架构脚本新增 translation-unit isolation 门禁，official
aggregate verifier、P3/P6 clean compile 与 P6 ELF 审计通过。

当时结论是 P6 成为 ESP32 WiFi+BLE 默认领先候选。2026-08-22 已完成 Controller v2 binding/bootstrap、标准
`src` composition、`<BlinkerWiFiBLE.h>`、正式单 ZIP 安装编译和当前 28.48% 静态余量门禁；当前不再保留
P3/BLE WiFiProv **业务图**，但 IDF-NimBLE 目标可通过 `.ipp` 选择器启用隔离的 BLE scheme factory。尚缺 App 双 bearer UX、
增强档 AP key/PoP、真机 heap/stack/RF 共存、100 次循环和 24 h。
P4 仅保留历史数据。

同日清理批确认 `BasicWifiBleLifecycle/WifiBleLifecycle` 已无正式 Product 消费者；正式 ESP32 图由
`Esp32WifiBleProduct` 直接编排。因此旧 runtime 头、Advanced include、52 B SizeProbe、架构正向检查和五组
自证 host 测试已物理删除，并增加 retired-path 反向门禁。保留的 BLE-only/WiFi-cloud 测试与 41/41 ASan、
200,000 fuzz 均通过。

## WP-P5：P3/P6 TLS 静态资源冻结与 P4 清理（2026-08-18）

P5 没有增加另一套 product composition。Plain/TLS 继续使用同一 P3 或 P6 graph，仅由生成的 release config
选择 `WiFiClient`/`WiFiClientSecure`、端口和 CA。最终 Arduino-ESP32 3.3.1、`min_spiffs` clean-source 结果为：

| 图 | Flash / static RAM | 1,966,080 B app slot 余量 | 结论 |
|---|---:|---:|---|
| P3 Plain | 1,758,907 / 62,964 B | 207,173 B（10.54%） | 勉强通过 |
| P3 TLS | 1,845,907 / 63,556 B | 120,173 B（6.11%） | 拒绝作为默认 |
| P6 Plain | 1,375,887 / 60,856 B | 590,193 B（30.02%） | 通过 |
| P6 TLS | 1,463,067 / 61,456 B | 503,013 B（25.58%） | 通过 |

P3 TLS ELF 同时存在 mbedTLS 与 BLE WiFiProv、没有 NimBLE；P6 TLS ELF 同时存在 mbedTLS、SoftAP 与 NimBLE，
没有 BLE provisioning/Bluedroid。P6 Plain/TLS 均通过专用 ELF 审计。TLS 是目标资源档的显式编译期选择，失败时
不得静默降级到 Plain；同时不能因高资源板可承受 TLS，就把低资源设备的 Plain profile 从合同中删除。

P4 product wrapper、platform、official profile 和 example probe 在确认没有发布消费者后物理删除；其 7.33% 失败
数据仍保留用于说明决策。后续最终 profile 审计确认独立 `esp32-ble` adapter 同样没有产品消费者，已从源码、
manifest 与 CI 删除；ESP32 普通 BLE 只保留 NimBLE 产品路径。
下一设备软件批已完成 Controller Grant/Install/Rotate/Revoke 的 BBP/2 v2 Core；当前转向 P6 最小 ownership
binding 与首次 controller bootstrap。App/Service 端点与真机 E2E 仍分别是后续生产阻断。
