# C3：最终单 ZIP 与基础平台复核（更新 2026-09-16）

> **2026-09-22 执行入口：[新进度与后续计划](DEVICE_V2_NEXT_EXECUTION_PLAN.md) §6。** C3 冻结范围已收口，9 月 18 日真实失效 BLE 候选/缓存优化也已完成；不包含新增用户定时/自动化，不按下文历史待办重开全矩阵。

唯一执行入口：[核心稳定版主线](DEVICE_V2_CORE_STABILITY_EXECUTION_PLAN.md)。本批是 C3，不恢复无缝通信、网关扩展或 Wio BLE 发布任务。C3 完成需要安装编译与相应真机证据，不能以其中一项代替另一项。

**最新状态见 §12.6：C3 冻结基础矩阵已关闭。七目标基础编译/六种实物模式，加上 HTTP
小补丁三 WiFi 目标补编译、Wio 续签停服故障修复后的标准固件闭环已通过。原失败样本
保留，不扩大为生产发布、任意故障或 Wio BLE 已验收；首次进入小包结果单列。**

## 1. 制品与修正边界

- 标准库仍是五个根文件、`src` 和四个公开示例；388 个文件，17 个实现编译单元。`docs/tests/tools/build`、账号、DeviceKey、WiFi 密码和设备备份不进入 ZIP。
- 使用当前本机 Gateway 的公开验签材料和 development profile：本地 Control、HTTP/MQTT 明文、允许测试用明文存储。不是可直接对外发布的生产信任配置，也不改变默认明文、显式可选 TLS 的合同。
- 最终 ZIP SHA256：`77cc302f0a8e3d155447f36a5bf9c925b571efb295dc5dd9c46ddb677ff93946`。两次独立 stage/打包字节相同。
- 初始编译 ZIP 为 `2f5f643b6b4c3de4968e69d2b12be755f2e0ef91716fc5304f53937f7c91739f`。编译开始后纠正 README，逐文件核对仅 `README.md` 改变；所有编译输入完全相同。最终包必须再通过真实 CLI 安装和逐文件核验，不能只改 SHA marker；不称 README 修订后又完成一次全量冷编译。
- README 删除“ESP32 WiFi+BLE 未接入 enableTime”的旧说明，并明确单 ZIP 的依赖安装、IDF NimBLE SDK 准入和 preview 边界。没有新增平台 API、产品包或凭据槽。
- 安装门禁原先默认所有例程不得链接授时，与已启用 `enableTime()` 的 CloudSmokeTest 冲突。最小回归先复现失败，再给矩阵 Target 明确 `time_enabled`；CloudSmoke 必须保留完整业务时钟，其余未启用例程仍禁止误链接。保留专用探针的显式覆盖参数，不据实际 ELF 反推预期、放宽检查。

包边界回归 **15/15**；架构、release profile 检查通过。没有改 MCU 或 App/Service 生产代码；没有更新 SDK、部署、迁移、暂存、提交或推送。

## 2. 安装编译矩阵

七目标使用原安装检查器在隔离 sketchbook 里安装同一个 ZIP，再编译包内原示例。首批三个 sketchbook 保留，恢复批次为每个未完成目标新建目录，避免覆盖中断证据。SDK/第三方依赖来自既有本机缓存，仍核对固定版本、来源路径、实际依赖和 map/ELF 闭包；不等价于重新下载的干净 SDK/CI。

首批完成 **4/7**；其余在 §6 所述本机磁盘风险下明确中止/未启动。恢复批次已完成 CloudSmoke、R4 BLE 和 Wio WiFi，当前安装编译 **7/7**，并再次逐项交叉验证安装字节、实际源码/依赖、map/ELF 闭包。不将原中断计为固件缺陷，也不把编译通过写成七项真机通过。ESP32 普通 WiFi 与 CloudSmoke 用 stock 3.3.6；BLE/WiFi+BLE 用已准入的 3.3.11 IDF NimBLE；R4 用 Renesas 1.5.3，Wio 用 Seeed SAMD 1.8.5 / rpcWiFi 1.1.0。没有为 BLE 安装另一份 Blinker 包。

| 目标 / 原公开示例 | program / 静态 RAM（B） | 本批结果 |
| --- | --- | --- |
| ESP32 WiFi / WiFiQuickStart | 994,203 / 50,912 | 编译、来源/依赖、链接闭包通过 |
| ESP32S3 BLE / BleQuickStart | 693,088 / 34,864 | 编译、来源/依赖、链接闭包通过 |
| ESP32S3 WiFi+BLE / WiFiBleQuickStart | 1,282,236 / 55,532 | 编译、来源/依赖、链接闭包及 §3 真机通过 |
| UNO R4 WiFi / WiFiQuickStart | 138,976 / 13,052 | 原 profile 编译闭包通过；新网络 profile 的当前 R4 真机及资源见 §9 |
| ESP32 / CloudSmokeTest | 1,011,591 / 51,016 | 恢复批编译、来源/依赖、实际授时链接闭包通过；不是 ESP32S3 真机制品 |
| UNO R4 BLE / BleQuickStart | 194,476 / 16,964 | 编译闭包及 §9 当前 R4 标准 BLE 三轮实物通过 |
| Wio Terminal WiFi / WiFiQuickStart | 143,176 / 68,072 | 恢复批编译闭包通过；ELF 口径见下，新网络 profile 的实物三轮及资源见 §10 |

静态 RAM 不含堆、任务栈/无线协议栈运行峰值；program 是 CLI 链接期统计，和含镜像头/对齐的 `.bin` 大小分别记录。R4 已完成目标实际引用 `renesas_uno/1.5.3`。NimBLE SDK 配置摘要 `534f3457aaf3ffbc82c45dc21d6b3ac57388c82d0d9a24bd2b8e6cadc3e48d9d`，host stack 5,120 B、IPC stack 2,048 B；本批没有测新任务栈水位，也没有放宽准入。不要把主机并行编译墙钟时长当 MCU 性能指标。

Wio 为上表 program 口径的显式例外：Seeed size recipe 只打印 `.text=142,440 B`，未打印 RAM。补用同编译器目录的 ELF size，`.data=736 B`、`.bss=67,336 B`，Flash 载荷为 `.text+.data=143,176 B`，静态 RAM 为 `.data+.bss=68,072 B`；不把 ELF 调试段算入 Flash，也不扣掉 SDK/RTOS 的静态预留。首次恢复汇总脚本因错误假设所有 CLI 都打印 RAM 而断言失败；修正的是此本机汇总口径，原安装编译门禁已经通过、未修改或放宽。

## 3. 真机边界与下一步

现场 COM3、COM11、COM12 均为 XIAO ESP32S3。COM11 是本批受控 WiFi+BLE 测试角色，另外两块保留见证，不因设备名含 R4 而计入 R4 硬件矩阵。

COM11 应用分区 `0x10000`、容量 3,342,336 B；原应用 1,369,792 B 已读回并与先前应用摘要一致。备份只用于回退应用，不回滚 NVS/授权。烧录前原固件基线：真实 App 自动 BLE 连接、Manifest、Property/Action/State、退出回收通过，原账户 MQTT 未替换。该基线不是新 ZIP 的真机通过。

COM11 现运行同 ZIP 原始 `WiFiBleQuickStart.ino`，没有修改 Sketch 或注入内部 composition。只写/校验应用区，固件 1,282,384 B，SHA256 `5fd8c2431a63e37d0d9ff0dfa46ec365d7c6a623295def024eb429e3f65de563`。原网络、逻辑 ID 和授权继续使用，无清 NVS、重新认领或 Key 刷新。

| 当前包的有限真机检查 | 结果 |
| --- | --- |
| 初次刷入后账户 MQTT | Manifest/Property/Action/State 通过，属性/按钮 ACK 184/188 ms |
| 自动 BLE 页面 | ready 3,507 ms；属性/按钮 326/302 ms，按钮次数精确增加 1 |
| 普通重启后 BLE | ready 3,533 ms；属性/按钮 288/274 ms；无重新绑定 |
| 原 MQTT 被动等待重启恢复 | 保持状态订阅，不发任何 Property/Action 诱导恢复；复位工具结束后约 2,143 ms，自动由旧 revision 5 / 按钮 3 收到新 revision 1 / 按钮 0 |
| 被动恢复后的再次控制 | MQTT 217/177 ms；BLE ready 3,305 ms，属性/按钮 300/300 ms |
| 退出与见证 | 每次返回首页，连接组/BLE link/测试钩子均 0，原账户 MQTT 未替换；COM3 和 COM12 原云控 Property/Action/State 通过，未改它们的固件或授权 |

保留两项最初未通过的原始观察，不能从报告删去：刚复位即发送的 Property 未成功，未发 Action；第二次以旧缓存按钮值作为执行前基准，断言 `2 != 3`，实际新 State 已由 revision 5 回到 3，Action 后 1→2 且有 ACK。测试基准改为先前真实 Property ACK 后观察到的 State，同时增加了上述**不发命令的被动恢复**，确认不用交互诱导也能自动恢复。没有修改生产就绪状态、加自动 Action 重试或延长产品超时。

StatePage 可能先来自 Broker 持久的最后已知快照；App 取得完整快照不意味着物理重启在同一瞬间已完成。断电/重连传播窗口内命令失败必须明确反馈，不自动重放业务 Action。本批证明有限普通恢复，不证明瞬时离线检测、P95 或无缝 SLA；缓存呈现/在线传播的产品语义仍须如实说明。

当前状态与剩余顺序（以 §11 及核心稳定表 §21 更新）：

1. S3 同 ZIP WiFi+BLE、独立 BLE/WiFi、CloudSmoke 短采样均已补证，见 §7/§8；不能把某产品某平台通过等价为其他平台。保留 C1/C2 深故障证据，不重新展开矩阵；§7.3 整套环境/USB 恢复失败仍需有限对照，§8 单 Broker 暂停恢复不能替代。
2. 短采样已联合 MCU 诊断与原 App/Broker 证明开始、前后台/退出停发、自然到期和恢复。`telemetrySamples` 是成功发送数而不是 `onSample` 次数，通用回调行为由 host 测试补证。本次没有额外真机采集计数，不修改时钟/伪造 ACK，不外推所有平台或长时间 SLA。
3. R4 普通 WiFi/BLE（§9）与 COM6 Wio 普通 WiFi（§10）基础三轮已补证。Wio BLE 继续 preview；不把 ESP32 别名或编译结果充当实物证据，也不修改发布范围以掩盖缺项。
4. C4 服务器维护扩展按用户决定暂停；三端部署配置、共享跨片一致开关、生产信任材料、干净 SDK/CI 的未完成记录保留。当前本机测试通过不等于生产候选已发布。

## 4. 证据

本机 ignored `build/c3-matrix-20260914/` 保存 `prepared.json`/`final-prepared.json`、原始/最终 ZIP、各目标 compile log/map/ELF、安装与复核记录、精确 COM11 备份及 App/云控记录。源码树原有变更保留；本批不清除绑定、配网或 NVS。

首批收尾 `verify.py` 再查最终安装字节、ELF 摘要、来源与链接闭包，以及 8 项 BLE/云控记录和无命令被动重启证据，生成 `verified-summary.json`：已验 4 个编译目标，3 个明确待完成，`c3Complete=false`。三个独立 sketchbook 都已实际重装最终 ZIP，原始编译/中断记录没有覆盖。该文件是首批检查点，不是恢复批的完成状态；新证据见 §7。

## 5. C4 只读配置预审

**本节保留 2026-09-14 的运行状态/初审；2026-09-15 配置源码修正及 SDK 溯源见 [C4 执行表](DEVICE_V2_C4_REPRODUCIBLE_DEPLOYMENT_PLAN.md)。配置未部署，初审不能再作为当前源码缺项清单。**

在当前四个容器中读取各自配置模块，Gateway `appRelay.enabled/sharesEnabled` 均为 false，Broker 0/1/2 的 `appRelay.enabled` 均为 false。没有修改运行入口或部署。本机 Gateway 示例环境文件已有两个默认 false 的变量，但当前 Compose 清单未发现相应映射，不能仅在宿主机设值后就假定容器已收到。

后续 C4 必须从部署配置统一注入：Gateway 和所有参与源/目标转发的 Broker 使用相同的 `DEVICE_V2_CROSS_SHARD_REQUESTS_ENABLED`；Gateway 另有 `DEVICE_V2_CROSS_SHARD_SHARES_ENABLED` 控制分享签发。开关、匹配镜像和重启后的实际配置一起验收，最后再复验双账号跨片。该审计不等于本批开启了跨片分享，也不要求为了当前 WiFi/BLE 标准例程先展开跨片新功能。

恢复批次继续只读核对，C4 的具体缺项如下，不能以本机 C3 编译代替：

| 项目 | 当前证据 / C4 应执行内容 |
| --- | --- |
| 静态与动态分片配置 | `compose.dev.yaml` 未显式映射上述两开关；Controller 的 `CONTROLLER_SHARD_TEMPLATE_ENV_JSON` 同样没有 requests 开关。正式方案须覆盖 Gateway、全部静态 Broker **以及之后新建的动态 Broker**，避免仅修当前两个容器。保留默认 false；测试显式开启后核对实际进程配置与跨片请求/分享。不能只在宿主 `.env` 写值就声称所有分片生效 |
| ESP32 BLE 的 CI SDK 准入 | 初审 workflow 仅安装 stock `esp32@3.3.6` 后调用七目标，与实际 S3 BLE 的官方 `3.3.11` 基线不一致。**纠正：不能据此断言 stock 3.3.6 未启用 NimBLE 或必然编译失败**；本机其 S3 sdkconfig 实际启用 NimBLE、IPC 1024 B，3.3.11 为 IPC 2048 B。3.3.11 Core/S3 SDK 共 8,329 个文件已与官方归档逐文件吻合，无需私有 SDK。C4 现将同 ZIP 五目标放在原 stock/跨平台 SDK，两项 BLE 使用官方 3.3.11 并检查摘要；远端干净 CI 尚未运行，不外推其他芯片/SDK |
| 三端制品与地址/信任 | 记录库 ZIP/profile/SDK 摘要、实际 APK/authority、Gateway/Broker 镜像与源码修订。当前已安装 debug APK 的 HTTPS `17000` 与源码另一 local 配置不同；发布时从同一配置生成，不以 ADB reverse/临时入口补丁当部署合同。公开验签材料可入设备包，私钥/账号/DeviceKey/WiFi 密码不入包 |
| 发布退出条件 | 无当前 R4/Wio 硬件就保留实物待测；生产信任材料、匹配开关、干净安装编译和基础短真机均有证据后，才标候选可发布。未授权提交/推送或新建远端任务，本轮不实施这些操作 |

配置注入复用现有解析与部署入口，不新增第二套路由服务/协议。先补配置一致性测试，再修改清单和 CI，最后在备份/明确范围下部署验收；C3 的当前短真机仍优先。

## 6. 编译中断与安全恢复

本批开始时记录 C 盘约 15 GB 可用，随后跌至不足 100 MB；停编译前约 59 MB、停后约 58 MB，D 盘仍约 146 GB。只读检查发现 `C:/pagefile.sys` 表观长度 48 GiB，Arduino core 缓存约 278 MiB，另有约 5 GiB 的既有 C++ 索引缓存。**没有记录分页文件的本批前后长度，不能据此断言全部净增长来自分页文件或编译**；完整磁盘归因/系统配置不在本批擅自处理。

仅终止经命令行路径和父 PID 确认属于本批的两条 Arduino CLI 进程树：CloudSmoke 和 R4 BLE；后续 Wio 不再启动。没有删除文件、结束 IDE/Docker/其他工作进程、移动 SDK 或改系统分页设置。`resource-stop.json`、两个原始非零 driver 结果及已完成目标的原日志均保留。

最终 ZIP 的重新安装把该子进程的 `TEMP/TMP` 指向本批 D 盘目录，再核验已完成四目标；不伪造七目标通过。以下保留中断时的恢复计划，实际恢复结果见 §7：

1. 先有足够 C 盘余量；若需迁移系统/其他工具缓存，单独确认目标和回滚方法。
2. 使用本批最终 ZIP 和已核验的 SDK/依赖，保留原日志。后续编译也使用 D 盘临时目录、先单 job，避免再次叠加资源压力。
3. 原安装检查器支持 `--reuse-work-dir`，实际安装字节和 SHA 必须与最终 ZIP 匹配；分别只选未完成的 `esp32-cloud-smoke`、`uno-r4-ble`、`wio-terminal-wifi`。不要重新跑全部目标或把被中止构建中间产物算成功。
4. 然后继续 §3 的独立产品/短采样和换板真机。COM11 保持正式 WiFiBleQuickStart，可直接继续，不需清授权。

## 7. 磁盘处理后恢复：独立 BLE、编译与环境边界（2026-09-14）

### 7.1 安全恢复及软件门禁

本轮不再清 C 盘。开始时 C 盘约 42.85 GB 可用；这与此前迁移释放量不是同一统计，不推断全部空间变化来自缓存清理。新 `build/c3-resume-20260914/` 使用 D 盘 `TEMP/TMP` 和经 CLI 配置确认的 Arduino core cache；目标顺序执行、每次 `--jobs 1`。驱动仅监测并可停止自己创建的编译子进程树，不结束其他 IDE/服务、不修改系统分页文件或 SDK。

继续使用 §1 同一个最终 ZIP，不重做平台产品包。本轮又从当前源码/profile 独立打包，`recheck-artifacts` 的 ZIP 摘要仍相同，确认没有混入不同库版本。包边界回归 **15/15**、架构与 release profile 检查再次通过；先重建当前 host Debug，再运行 **62/62**，保存 `host-build.log`、`host-after-build.log/xml`；原云控工具确定性回归 **8/8**，保存 `cloud-witness.tap`。CloudSmoke 已取得真实 ELF，确认显式启用的授时实现完整链接；没有放宽其他未启用产品的裁剪检查。

### 7.2 同一测试板，真正换为独立 BLE 产品

核对 COM11 MAC `8c:bf:ea:1b:96:14` 后读回当前 WiFiBleQuickStart 应用，确认与 §3 摘要相同。仅将同 ZIP 的 `BleQuickStart` 应用写入 `0x10000`，大小 693,232 B、SHA256 `6bb02747bbff749d986c40cffff2a5cd0fd9aae0ec857c942e6701b90a163460`；不写引导/分区表/NVS，不 resetAccess、换 Key 或重新建设备。两个产品共用原实例/授权存储合同，但分别运行产品实现，不把混合通道测试当独立产品测试。

| 正式 App 的有限检查 | BLE ready / Property / Action（ms） | 状态结果 |
| --- | --- | --- |
| 独立 BLE 初次刷入 | 3,393 / 249 / 251 | Manifest/State，按钮 0→1 |
| 独立 BLE 普通 MCU 重启 | 3,193 / 250 / 246 | 原绑定继续，按钮 0→1 |
| 独立 BLE App 杀进程/冷启动 | 3,295 / 279 / 268 | 保留 App 数据，按钮 1→2 |
| 恢复原 WiFi+BLE 应用 | 3,389 / 290 / 304 | 原绑定继续，按钮 0→1；随后 MQTT 154/168 ms |

每轮使用原页面/Property/Action，状态精确确认一次执行；每轮页面内账户 MQTT 会话未替换，退出后连接组/BLE link/测试钩子均 0。跨 App 进程当然重新建立账户 MQTT，不声称旧进程连接常驻。恢复应用经 esptool 读回校验，与备份及 §3 SHA256 一致。COM11 最终仍是原 WiFiBleQuickStart 和原逻辑 ID/Key v1/Admin epoch 1，不留独立 BLE 测试固件。

这是三个有限恢复样本和一次回退验证，不是长稳/DoS/全部 SDK 准入，也未覆盖 BLE 长时断电或所有手机。无需为本包运行 100 次轮换。

### 7.3 本地服务重启与 TLS 见证板：通过和未解决分开

Docker Desktop 原本未运行；启动既有服务后恢复了因 ADB daemon 重启丢失的反向端口。以已安装 App 的实际 authority `https://127.0.0.1:17000`、账户 WSS `18885`、通知 `17443` 为准，而不是源码中的另一份 local URL。没有安装新 APK、清手机资料、迁移数据库或修改运行配置；初次 API 不通/账户 MQTT retrying 时失败的准备步骤不是 BLE 固件失败。

两块 **ESP32S3** TLS 见证 COM3/COM12 在最初整个环境恢复后，原云控均未取得 fresh Manifest/State，未发 Action。串口观察时出现 `USB_UART_CHIP_RESET` 启动记录，随后新 session 与控制恢复。因此不能将它们算作无干预恢复：串口打开/驱动触发复位、缓冲启动输出或 USB 输出阻塞等原因尚未区分，不能直接归咎 TLS/NimBLE，也不能因随后成功删掉失败证据。

之后仅执行一次既有 TLS proxy 容器重启，不操作 MCU/串口、配置不变：App 账户 MQTT 自动重新 ready，COM3 Property/Action **96/137 ms**、COM12 **133/84 ms**，各有一次实际按钮计数增加。该有限代理重启通过，**不等价于已解决最初整套环境/USB 的无人值守恢复观察**。后续先在已确认的最终独立 WiFi 制品上做一次有界对照，记录设备启动/会话，不扩大时限或自动重发结果未知的 Action。

### 7.4 证据与剩余顺序

新恢复目录保存逐目标退出结果/原始日志、分区备份及写入校验、host 构建/测试、连接诊断和 `tls-restart.json`。复用旧只读/页面工具的新增 `standalone-*`、`resume-witness`、`after-serial-observation`、`tls-proxy-restart` 记录以唯一文件名保存在 `build/c3-matrix-20260914/`；没有覆盖原中断/失败结果。最后 App 为原 owner 首页、MQTT ready、groups/BLE links 0、LAN 无占用、测试钩子为空。

新 `verify.py` 最终通过，生成恢复目录的 `verified-summary.json`：7 项安装/实际依赖/ELF 与当前源码重新打包摘要全部匹配，62 项 host、8 项云控工具、4 条 BLE 页面链、恢复应用摘要、TLS 初次失败及代理重启成功对照、首页资源回收均有断言核验，`c3Complete=false`。收尾再读 `app-status-final-confirm-*`，仍为相同 owner 首页/MQTT ready/资源零占用。编译驱动已正常退出，C 盘约 39.4 GiB 可用，无遗留编译进程。

本节当时剩余的 ESP32 独立 WiFi/短采样、R4 WiFi/BLE 和 Wio WiFi 已由 §8～§10 补证；**当前只按 §11 的关闭清单执行**，不再按历史缺板状态重跑。七目标安装编译已关闭，§7.3 恢复观察仍待对照；C4 维护已另行暂停。既有见证板不因旧记录被清授权。本恢复批没有设备/App/Service 生产源码修改、暂存、提交或推送。

## 8. 独立 WiFi / CloudSmoke 实物与候选通道边界（2026-09-14）

本节延续 C3，不启动无缝切换专项。用户暂不能换 R4/Wio，保持其实际平台待测；只操作 COM11 应用分区，COM3/COM12 为保留原固件的 S3 见证。继续使用 §7 相同 ZIP，两个 Sketch 副本只替换正式示例的 DeviceKey/SSID/password 三项用户输入，来自原 owner 正常查看 Key；无新建、刷新 Key、清授权或擦 NVS。私有输入保存在本机受限且 ignored 的 `build/c3-wifi-20260914/`，不得进入源码/文档/ZIP。

### 8.1 真机先发现核心缺陷，不能只凭 ACK 宣布通过

COM11 的原 WiFi+BLE 应用重新读回，SHA256 仍为 `5fd8c2431a63e37d0d9ff0dfa46ec365d7c6a623295def024eb429e3f65de563`。独立 WiFiQuickStart 按真实 `XIAO_ESP32S3:PartitionScheme=default_8MB`、stock Arduino ESP32 3.3.6 单 job 编译：program **969,147 B**、静态 RAM **49,892 B**，依赖/来源/map/ELF 和未启用 Time 的闭包通过。应用 **969,296 B**，SHA256 `26a3a016a73c8077640749d97f2e033f500dcfd9ca119b8792fa56e38449dd5f`，只写并校验 `0x10000`，原逻辑 ID/Key/网络继续使用。

旧 APK 下实际页面初次 Cloud ready **5,697 ms**，Property/Action ACK **158/148 ms**；但 600 ms 后页面选择 BLE、原生 BLE link 为 0，页面失去 ready/fresh State，退出才重新显示已成功执行的 `power=true / buttonPresses=1`。这是**App 候选通道覆盖正在使用的通道**，不是 MQTT 未执行或物理设备重复执行；没有重试 Action。原失败证据保留，不计作完整页面门禁通过。

定位两处职责越界：`DeviceUiPort.refreshBlePresence()` 把 nearby 当成可用 BLE；普通 Direct 准备在 `ensureReady()` 前选择 BLE。页面有限重试还先关闭原 scope、以 `syncing` 一律关闭控制，使已可用 Cloud 在候选扫描期间被主动中断。

修正仅在 App 原选择器/页面生命周期：nearby 只作发现提示，Direct 完成 Ready 且原 scope 未取消才选用；页面复用原 connection group，保留旧 ready scope 的 StateInterest，候选成功后再退休旧 scope，失败保留原可用路径。隐藏/后台/销毁同时关闭旧 scope 和正在准备的候选，迟到成功不能复活。控制仍检查当前通道 Ready、fresh State 和活跃 scope，不因旧缓存开启。无新 MQTT、协议、重试器、连接容量或 Action 重放，没有布局/编辑器修改。

新增回归先出现 **9 项预期失败**（候选三种结果、nearby、页面五种退休结果），再修正通过；最终 **9 文件 151/151**，含原 LAN、gateway-child、离线 Direct、租约/生命周期/页面与 Session 回归。App/spec TypeScript 和四个修改 TS 文件 ESLint 通过。旧测试工具还补充“ACK 后页面仍 ready 且 State 精确匹配”断言，不能只记录耗时就称通过。

### 8.2 新 APK / 独立 WiFi / 正式 CloudSmoke 实物结果

新 APK `d4bdd26001b4add2e5b56a401ced04c7fb828c7a56fb1b0ae1b3d2d18561814d` 已保留数据安装，并由 Android 实际 base.apk SHA256 复核。LAN 仍默认关闭；沿用原 debug 本地 HTTPS authority/CA，既有构建制品地址调整不计作 C4 生产配置收口。

独立 WiFi 复测：页面 Cloud ready **5,788 ms**，Property/Action **146/224 ms**，State 精确符合请求；随后 12 个 500 ms 间隔观测均保持 Cloud ready，无 BLE link。普通 MCU 重启后新 revision 1 / 按钮 0 就绪 **5,619 ms**，再次属性/按钮 **137/243 ms**，退出 groups/BLE links/hooks 为 0。**初次进入仍受历史 Direct 凭据触发的原 5 秒探测影响**；本轮关闭的是“已经可用的页面被候选探测破坏”，不是首次 Ready/无缝延迟 SLA。

CloudSmoke 按相同 S3 FQBN/SDK/ZIP 编译、实际来源/map/ELF/授时闭包通过：program **972,899 B**、静态 RAM **49,980 B**；应用 **973,040 B**，SHA256 `68ceaaf3b952e4dc1bdc0e42d398624d843a491d2045b2a0603e103a7f07424a`。只替换三项用户输入，保留正式 `onSample()` / `enableTime()` 示例，无周期诊断输出、内部 composition 或额外采样计数。

| 实际操作 | MCU / App 结果 |
| --- | --- |
| 未开页面 | samples=0、dropped=0、leaseExpired=0，业务 UTC 已同步 |
| 原页面打开 | 新 Manifest 自动获取，旧布局只作迁移预览、未保存编辑器布局；1000 ms Telemetry 启动，samples 增至 15；Property ACK 227 ms |
| 真实 Android HOME | appActive=false、groups=0、租约不可见且 inactive；间隔约 7.1 秒，samples **17→17**、dropped **0→0** |
| Activity 回前台 | 原页面自行恢复，samples 增至 39；该前后台窗口原账户 MQTT Session 未更换 |
| 退出首页 | 间隔约 7.1 秒，samples **41→41**、dropped=0；异步清理后租约/连接组归零，不把刚导航的一瞬间当作 ACK 已完成 |
| 停止续租、自然到期 | 仅暂停本机 `aily-iot-dev-broker-shard-1-1` **38,454 ms**，finally 恢复且独立 45 秒 watchdog 兜底；暂停内 leaseExpired **0→1**，samples 停在 **71**，第二次仍 71、dropped=0；串口持续打开、无 MCU reset/改时钟 |
| Broker 恢复 | 原页面自行恢复 ready/Telemetry，samples 增至 **86**，Property ACK **132 ms**；没有重进页面或工具重开流。实际发生正常 MQTT 重连，Session 已更换，不称全程会话不变 |
| 再次退出 | 间隔约 7.1 秒，samples **104→104**、dropped=0、leaseExpired=1；全部诊断结束后才关闭串口 |

samples 为 `telemetrySamples` 成功发送数，dropped 为 `telemetryDropped`，不是 `onSample` 次数。第一次到期诊断在 Broker 仍暂停期间；第二次在恢复后约 0.3 秒，MCU 当时短暂为 reconnect 状态。随后首次被动观察已经 ready，未连续捕捉切换瞬间，因此不能给出精确恢复时延/P95。本批证明到期与停止发送，不称真机直接测得停止调用采集回调；通用回调行为仍由 host 回归补证。初版本机串口工具与 Python `serial` 包重名，在尚未打开串口前修正名称；正式采样全程使用同一个持续串口句柄。

### 8.3 收尾、证据与剩余主线

COM11 已恢复并校验 §8.1 原 WiFi+BLE 应用，NVS/逻辑 ID/Key/绑定未变。新 APK 下自动 BLE ready **3,807 ms**，属性/按钮 **257/300 ms**、按钮 0→1；随后 MQTT **180/120 ms**、按钮 1→2。COM3/COM12 两块 TLS 见证在 Broker 恢复后分别 **97/124 ms**、**150/64 ms** 通过 Property/Action/State，没有操作其串口、重置或固件。Broker 已 running/not paused；原 owner 首页 Cloud ready，groups/BLE links/hooks 均 0，LAN 未启用。

证据目录 `build/c3-wifi-20260914/` 保存编译/来源/应用与 APK 摘要、`selection-red.xml` / `selection-final.xml`、`page-*`、`diagnostics-*`、`pause-*`；见证和恢复 BLE 记录由原工具写入 `build/c3-matrix-20260914/*wifi-smoke*`。`verify.cjs` 交叉核验生成 `verified-summary.json`，覆盖 151 项回归、WiFi 精确 State/持续 ready、真实暂停内到期顺序、停止/恢复计数、手机实际 APK、原应用恢复、三板控制及退出。编译临时/core cache 在 D 盘、单 job 顺序执行，C 盘仍约 39 GB 可用。

本批只修改 App 原选择器/页面及回归和方案记录；设备/Service 生产源码未改，无暂存/提交/推送。**覆盖 §3/§7 的旧待办：S3 独立 WiFi/短采样已关闭，C3 整体仍未完成。** R4 WiFi/BLE、Wio WiFi 实物待换板；§7.3 整套环境/USB 恢复失败仍须单独有限对照，单 Broker 暂停通过不能消去旧失败。C4 静态/动态部署开关、生产地址/信任材料及干净 SDK/CI 仍待收口，不进入 100 轮或新协议扩展。

## 9. COM9 UNO R4：恢复基础真机与本机换网（2026-09-15）

按核心稳定表 §19，服务器维护扩展暂停，Wio 实物继续暂缓，BLE 首次进入延迟后置。
本节覆盖旧“先完成 C4 才换板”的顺序。当前物理 COM9 确认为 UNO R4 WiFi，USB serial
`F412FA6F2F88`，Renesas 1.5.3；不是目录中名称含 R4 的 ESP32 见证设备。COM3/COM11 未烧录、
未清授权；手机保留原 owner 和 App 数据。WiFi/BLE 分别运行，不增加 R4 混合产品。

### 9.1 保全与标准 BLE 检查边界

先用板载 CMSIS-DAP 读取 RA4M1 完整 262,144 B 程序区，备份 SHA256
`e4d05b53ac82ffbd72cf85e099eeac0d0f3800f317cc5869b71a949a832e1440`；文件含原应用，保存在
本轮受限私有目录，不上传。桥接 ESP32 的固件及 NVS/Preferences 未擦除或升级；RA 程序区
备份不等价于桥接 NVS 备份。早期 BOSSA read 返回失败且仅产生 0 B，不算有效备份。

先利用网络准备窗口烧录同 §7 原 ZIP 的标准 `BleQuickStart`。首次标准 upload 在连接阶段
`No device found / Device unsupported` 失败，没有进入擦除/写入；用户拔插后再次标准 CLI
upload 成功。应用 194,484 B、SHA256
`98a6ecad1ae154edc845e463e1c7a8ebaae2e8f08b0bc0f4a005e30b0e712d0a`。
校验工具最初错误使用地址 0，实际 ELF `.text` 起址为 **0x4000**；改正地址后校验通过并
恢复运行。这是测试工具错误，不算固件损坏。调试校验启用 work-area-backup，并在失败路径
也 reset run，避免校验结束留下 MCU halt。已删除本批不再使用、可能误用整片备份的旧 BOSSA
辅助脚本；保留失败日志。完整备份不可直接当作从 0x4000 开始的 application.bin 烧录。

手机实际扫描到 R4 `Blinker` Direct 广播（wire 3、mode 2、capabilities 81）。现场地址
`F4:12:FA:6F:2F:89` 仅定位本次板卡，不用于业务认证；另一条广播属于真实独立 COM11。
本轮账号持有的凭据未匹配 R4 的既有 Direct 授权，尚无 Provisioning 广播，故**只有发现证据，
没有本轮 BLE 认领/控制通过结论**。采集扫描使用始终返回 false 的 matcher，结束的
`BLE_DIRECT_SCAN_TIMEOUT` 是采集窗口结束，不等于设备没广播。用户随后明确允许仅重置 R4 BLE
授权并用当前账号重新认领，不删除 App 历史设备。通过公开 `Blinker.resetAccess()` 的临时
维护 Sketch 执行，禁止直接修改凭据字节、删除整个 Preferences namespace 或清其他板。

### 9.2 mostfun2026 的必要测试网络调整

用户决定不切回旧网络，允许只调整本机测试网络。电脑当前 `192.168.1.139`，旧 Gateway
公告与明文入口绑定仍是 `192.168.2.76`。保存精确容器 inspect、网络/挂载及非目标基线后，
只重建 Gateway 和受限 plain HTTP proxy：Gateway 两项 public host 与两个宿主绑定改成新 IP，
`17002` 为设备控制入口、`18884` 为设备明文 MQTT。保留所有原 Docker 网络和卷；未迁移
数据库、替换 Broker/Auth 或开启 C4 candidate/业务开关。

Gateway 使用已有 recovery image 保留原运行实例热修后的有效代码；替换前后逐项核对
`dist/packages` **179 个文件相同**。apply 时另 **19 个非目标容器**身份/镜像/运行基线未变。
健康、代码、挂载与原 App 用户 MQTT 恢复后，仅移除两个已停止的前身容器，**未删除任何卷**；
原 inspect、镜像及数据卷保留，可按记录重建，但不承诺原容器 ID 可恢复。

电脑 TCP 17002/18884 可达；手机原生 TCP/HTTP 得到无效路径的 400，证明链路可达，不把 400
当成设备登录成功。既有 App loopback HTTPS/ADB reverse 未改；设备 TLS IP/SAN 未在本批验证。
当前是有记录的本机 runtime 网络调整，不声称已完成可复现发布；后续 Compose 不应直接套回旧 IP。
原 C4 只读容器 ID 基线因这次两容器替换已过期，恢复 C4 时须重新采集，不能沿用其“完全未变”断言。

### 9.3 R4 WiFi 制品与实物结果

仍为同设备实现/同 SDK，profile 仅更改 control.host。重新生成单 ZIP 的 SHA256
`90a314b9af861313823137ebde0526be6ebca49444091a8fa249816d893acfc6`；与 §7 的 388 文件比对，
只有 `README.md`、`OfficialConfig.h` 不同，不能把新摘要伪称原 ZIP。标准 `WiFiQuickStart`
私有副本只替换原 owner 正常查看的 DeviceKey、SSID/password，未旋转 Key 或新建设备。

编译/安装字节、依赖、来源与 ELF 裁剪通过：program **138,984 B**，静态 RAM **13,052 B**；
实际依赖仅 BlinkerDeviceV2 / Preferences / PubSubClient / WiFiS3，没有链接 BLE 或未启用的
业务授时。应用 138,992 B，SHA256
`f01dc8fe3740735ff59f9c5019d50b2693e21808c311b8c0105244c1d348719d`。

切换 WiFi 时标准 upload 再次在握手阶段 `No device found on COM9`，没有新写入；用户只拔插
COM9 后再次标准 upload 成功。按 ELF 的 0x4000 校验成功并恢复运行，没有改桥接固件。
这两次握手失败仍保留为本板烧录恢复观察，尚未归因于库、SDK 或桥接固件，不以随后成功抹掉。

| 原 App 页面实际检查 | ready / Property / Action（ms） | 精确状态 |
| --- | --- | --- |
| 标准 R4 WiFi 初次刷入 | 1,073 / 637 / 767 | 原 Key v1 上线；Manifest 三字段，power false→true，按钮 0→1 |
| 普通 RA MCU reset | 2,884 / 322 / 289 | 原 ID/Key，无再次认领；新 revision 1 / 按钮 0，控制后按钮精确 1 |
| Android App 杀进程/冷启动后进入 | 581 / 423 / 385 | 保留账号资料，Manifest/State 继续；power true→false，按钮 1→2 |

初次控制后 12 个 500 ms 间隔观察均为 cloud ready，State 保持一致；三轮退出后 groups/BLE
links/hooks 均 0、LAN 无占用，每个页面窗口内原账户 MQTT Session 未替换。App 跨进程自然新建
账户会话，不声称跨杀进程保留连接；581 ms 是进入设备页时长，不包含 Android 整个启动过程。
完成控制后又独立读回 WiFi 应用，SHA256 与构建精确相同。Reset 只通过板载调试器发普通
MCU reset，不等价于整板断电/桥接 NVS 持久化或整套服务无人值守重启。该批未启用 Time/Telemetry，
不外推 S3 CloudSmoke 的授时/停采结论至 R4。已复用原页面检查函数，仅注入待测逻辑 ID，
没有内部 composition、伪造 ACK、自动重发 Action 或新增业务协议。

### 9.4 BLE 重认领与回执边界

用户已明确授权仅重置 COM9 BLE Access 并正常重认领。维护程序只使用 `BlinkerBLE.h`、三个
原字段和 `Blinker.resetAccess()`；只有显式串口 `R` 指令才执行一次，开机不自动重置，防止
测试中拔插导致刚认领的授权再次丢失。平台实现确认为仅清 DeviceAccess blob；设备实例、
WiFi slots 与手动 WiFi Sketch Key 是独立来源。没有清整个 namespace、伪造 grant 或更改服务器
授权规则；App 原认领页仍负责发现/Noise/服务器签发/设备安装及进入控制页。

该临时程序已编译并通过安装字节、依赖/来源/ELF 裁剪：program **194,748 B**、静态 RAM
**16,968 B**，应用 SHA256 `c194aa09c9ca1c4ca78693815e29afe763b859f3b2a6ee229c69e1927ce43091`。
比原标准 BLE 多 272 B program / 4 B 静态 RAM，仅用于维护，不是最终示例验收。实际使用
ArduinoBLE / Crypto / Preferences / WiFiS3 / BlinkerDeviceV2；WiFiS3 是该 SDK 的桥接依赖，
链接闭包仍排除 WiFi/MQTT 产品图，不能据依赖名误判成 R4 WiFi+BLE 产品。

从已通过的 WiFi 切换至维护程序时，标准 uploader 也在握手阶段报 `No device found`，没有
进入擦除/写入。用户拔插后上传成功、完整 application 读回 SHA256 精确匹配。因此握手问题
不能只归因于“BLE 正在运行”；是否与桥接下载状态/调试 reset 相关尚未隔离。

串口只发送一次 `R`；20 秒未收到 OK/FAILED 文字，保留 `confirmed=false / failed=false`
原记录，不重发。随后 R4 实际广播从 Direct 变为 Provisioning（wire 1 / mode 1 / capabilities 7），
App 原 `/guide/ble` 流程完成安全握手、签发/安装与 Direct 能力读取，逻辑 ID
`ble_nBVYKR02cHTPzjkjudJhhQ`，三字段正确，并通过原 `finish()` 自动进入 fresh BLE 控制页。
首页收尾 groups/BLE links 0，页面窗口原账户 MQTT Session 未替换。本批**未收到串口回执**，
重置行为由随后 Provisioning 和真实重新认领共同补证，不把串口超时删去或改写为成功。
没有删除 App 原历史设备，也没有向数据库写入测试授权。

恢复原标准 BleQuickStart 的首次 upload 又在握手阶段失败、无新写入；用户只拔插 COM9 后
再次上传成功。维护程序开机不自动 reset，所以刚建立的授权没有再次被清理。随后完整读回
应用 SHA256 与 §9.1 原标准二进制精确相同，不留临时维护逻辑；未重复 resetAccess/认领，
未重做 WiFi 三轮或冷编译，未扩 C4/桥接固件升级。

### 9.5 标准 BLE 实物收口与剩余项

| 原 App 自动 BLE 页面 | ready / Property / Action（ms） | 精确状态 |
| --- | --- | --- |
| 恢复标准 BLE 固件 | 4,074 / 356 / 331 | 新授权继续，power false→true，按钮 0→1 |
| 普通 RA MCU reset | 4,141 / 323 / 326 | 不重新认领，按钮重启后 0→1 |
| App 杀进程/冷启动后进入 | 5,914 / 317 / 327 | 本机凭据持久化后可恢复，power true→false，按钮 1→2 |

三轮均确认 fresh BLE State 和属性/Action 精确结果；每轮退出 groups/BLE links/hooks 为 0，
LAN 未占用，页面窗口内账户 MQTT Session 保留。App 跨进程重新连接 MQTT，不算旧连接常驻；
上述 ready 从进入设备页计时，不是整个 App 启动时长/P95，BLE 首次进入优化仍后置。

认领后的首页扫描也作了有限核对：仅新 R4 BLE 记录和真实 COM11 分别 nearby；旧 R4 WiFi
记录及历史 BLE 记录没有被同一广播同时标为 nearby。它是本现场两块广播设备的映射证据，
不是所有历史记录/多手机发现的全量回归。

**COM9 最终运行原标准 BleQuickStart 和新 BLE 授权；不再运行维护程序，不需再次拔插或清授权。**
App 为原 owner 首页、账户 MQTT ready、无本地连接占用；COM3/COM11 固件与绑定未改，不把
它们在换网后的云端离线计作已经复测通过。停止的旧网络容器及其回滚材料边界见 §9.2。

R4 本批普通 WiFi/BLE 接入、能力/控制/状态、MCU/App 有限恢复已补齐；Wio WiFi 实物仍暂缓，
C3 整体仍未完成。R4 整板长断电、Time/Telemetry 跨平台实测及 §7.3 整套环境恢复不由本次
RA reset/冷启动替代；按声明范围另行选择必要补证，不能自动扩成 100 次/无缝专项。
多次标准 upload 需拔插的现象仍为独立未定位风险，后续若处理，应先做上传/调试 reset/桥接
状态的有界对照，不凭本轮现象修改 BLE 协议或直接升级桥接固件。C4 服务器维护仍暂停。

证据目录 `build/c3-r4-20260915/`（ignored）保存原备份、精确网络快照、制品/构建/失败与成功
upload、广播及页面记录。`verify-results.cjs wifi` 已逐项交叉核验三轮 WiFi、精确按钮变化、
退出资源零占用及应用读回。最终 `verify-results.cjs all` 又交叉验证新认领、标准 BLE 读回及
三轮 BLE 控制/退出：`wifiPassed=true / r4BlePassed=true / c3Complete=false`；串口未知结果与
后续无线补证分开保存。编译/上传/调试进程已正常退出，D 盘构建与私有证据保留。
私有输入不进入源码/ZIP；本批未新增生产实现，不暂存/提交/推送。

## 10. COM6 Wio Terminal：普通 WiFi 实物补证（2026-09-16）

用户将 R4 换为 Wio，按核心稳定表 §20 推进，不恢复 Wio BLE/RTL 固件专项或 C4 维护扩展。
现场 Wio USB serial `03C8D76F53544C3257202020FF051C07`，runtime COM6 / `2886:802D`，
同 serial bootloader COM7 / `2886:002D`。COM3/COM11 不操作；手机原 owner/安装数据保留。

### 10.1 保全、网络与制品

Stock Seeed bossac 已读出 SAMD51 内部程序区 524,288 B，SHA256
`e0adbbf2ba99f906cb847a9f40e70920349e3febde9421f91337f7505d6ade3c`，保存于 D 盘受限目录。
这是包含 bootloader 的内部程序备份，不是 RTL 固件/外置 QSPI 全盘备份；不能把整片文件
直接从 0x4000 写回。后续只上传普通 application，既有 QSPI 最后两扇区的 V2 记录不主动清除，
不升级无线固件。端口转换必须同时匹配 VID/PID/serial，不能按新出现的串口任取板卡。

Docker 启动后，电脑和手机仍在 mostfun2026，电脑地址由 `.139` 变为 `.133`。仅重建既有
Gateway/明文 HTTP 入口的网络配置，原有效业务代码 179 文件保持相同、19 个非目标容器
身份/镜像/运行基线不变；保留数据库和所有挂载。宿主 `17002/18884` TCP 及原 App 账户 MQTT
已可达，不把该结果当成 MCU 登录通过。原 C4 容器 ID 基线需重新采集，不沿用旧快照升级。
手机到设备 HTTP 入口也已获得无效路径的 400，证明网络路径可达；健康检查和原账户 MQTT
恢复后，仅移除两个已停止的网络前身容器，未删卷。原 inspect/镜像/配置仍保留，但原容器 ID
不可恢复；没有借网络变更部署 C4 candidate 或迁移数据库。

原 owner 正常查看现有 `device_4720de42-308b-4a63-acfe-ebb41016c027` 的 Key v1，
未旋转/重新创建；标准 WiFiQuickStart 私有副本仅替换 DeviceKey/SSID/password 三处。
同 C3 源码按本机 control.host 生成 ZIP，SHA256
`39b19ad7f561e352b6d69a65522740d05e0f5cf399dede3e69176d46e63fb2de`；388 文件中只
`README.md` 和 `OfficialConfig.h` 与 §7 不同，不将新 profile 的 ZIP 冒称原摘要。
SDK 仍 Seeed SAMD 1.8.5 / rpcWiFi 1.1.0，单 job、构建和临时目录均在 D 盘。

### 10.2 安装编译、上传与独立读回

同安装库字节、实际编译来源、依赖和 ELF 裁剪均通过。`.text=142,304 B`、`.data=736 B`、
`.bss=67,336 B`，program **143,040 B**，静态 RAM **68,072 B**。沿用 §2 的 ELF 口径，
不把 CLI 仅打印的 `.text` 当作全部 Flash、不把静态 RAM 当作运行峰值。与原占位 Sketch
的大小差异来自本机 profile/用户输入，不能声称本轮优化减少了代码。

依赖为 BlinkerDeviceV2、PubSubClient 2.8、rpcWiFi 1.1.0、rpcUnified 2.1.4、FreeRTOS
10.4.3、FS 2.1.3、mbedtls 3.0.2 及 SDK 依赖；没有启用 Time、LAN 或 BLE 产品。
本次冷编译约 19 分钟，按单 job 完成；不是 MCU 执行时长或通信延迟。

标准 Arduino CLI upload 一次成功，未出现 R4 的握手失败。随后通过 stock bossac 从
ELF 的 **0x4000** 独立读回 **143,040 B**，SHA256 与实际构建一致：
`0aab5504055b959f420f40430252e23f66f8d4f3c354bab1176335faa21a0ca2`。
只有应用上传与读回/普通 reset；没有清 QSPI、resetAccess、升级 RTL 或恢复旧备份。
正常应用是否写过其存储事务不由该读回证明，不能把“不主动擦除”写成 QSPI 字节完全未变。

### 10.3 原 App 三轮控制与环境中断

| 普通 WiFi 页面实际检查 | ready / Property / Action（ms） | 精确状态 |
| --- | --- | --- |
| ADB 恢复后标准示例进入 | 1,356 / 214 / 240 | 原 ID / Key v1；power false→true，按钮 0→1，revision 1→3 |
| 普通 SAMD MCU reset 后进入 | 1,961 / 141 / 149 | 无再次认领；按钮重启后 0→1，新 revision 1→3 |
| 保留数据的 App 冷启动后进入 | 627 / 256 / 189 | MCU 不重置；power true→false，按钮 1→2，revision 3→5 |

复用原 App 页面/账户 MQTT 和原组件 API；每轮只发一次 Property 与一次 Action，无隐式重发。
初轮 12 个 500 ms 间隔观测均保持 cloud ready、正确 State、无 BLE link。三轮退出后均为
首页，groups/BLE links/hooks 为 0，LAN 无占用。ready 是进入设备页时长，**不是整个 App
启动、MCU 重新上网时长或 P95**；App 冷启动样本先等待原账户通道可用，再开始页面计时。

保留以下中断与测量边界，不把后来通过覆盖成从未失败：

- 初次 open 在 CDP 断开前已进入设备页，但工具未取得完成回执；ADB 恢复后先检查真实页面
  和未执行的按钮 0，再退出清理，不盲目重复 open/命令。USB 断开后本机 9222 被 Karma
  浏览器占用，工具识别错误上下文后停止；改用手机专用 9332 和明确 `https://localhost/`
  匹配，没有关闭其他浏览器或修改业务 App。
- 再次 USB 断开使 ADB reverse 丢失。初轮有效控制是在账户 MQTT 恢复期间进入，
  `sameAccountSession=false` 如实保留；证明恢复后控制/状态正确，**不证明初轮会话全程不变**。
  普通 MCU reset 和最终 App 冷启动各自页面窗口的账户会话均保持；跨 App 杀进程当然新建连接。
- 第一次 App 冷启动遇到缺失的反向端口，原 account/connection 报网络错误，25 秒有界 open
  失败，未发 Property/Action。恢复三条原 reverse 后重新做保留数据的真正冷启动
  （PID 9548→10572），最终一轮通过。测试工具增加账户就绪前置，并等待 Angular 挂载，
  没有增加生产超时/重试或伪造 ready。先前失败样本和 PID 17595→9548 也保留。

这些是本机 USB/调试通道/服务可达性中断，不足以归因于设备 MQTT 缺陷；最终精确控制 ACK
与 State 是真实物理 Wio 交互，不只依赖 Broker 缓存页面或日志里是否打印 connect。

### 10.4 收尾及剩余项

COM6 最终保持本批标准 WiFiQuickStart、mostfun2026、原逻辑设备/Key v1；未刷新 Key、
清授权、新建设备或改其他两块板。App 为原 owner 首页、账户 MQTT ready、资源/钩子归零。
证据保留于 `build/c3-wio-20260916/`：受限程序备份/私有 Sketch、安装/ELF 验证、
upload/readback、页面/MCU reset/冷启动与中断记录。`verify-results.cjs` 已交叉核验，
结果 **wioWifiPassed=true / c3Complete=false**，初轮会话变化、失败的冷启动前置均显式保留。

设备/App/Service 生产源码本批未改，未暂存/提交/推送。只更新主线记录及 ignored 联调工具，
不因“清理”删除仍在使用的平台实现。原网络前身容器清理边界见 §10.1，D 盘备份不删除。

**下一步只补 §7.3 整套环境恢复的有限对照与声明范围核对**，先确认当前 profile/现场设备，
不在两块旧网络见证板离线时直接作故障结论。本轮普通 MCU reset 不等于整板长断电、
外置 QSPI 所有记录持久化或整套服务器恢复；没有测 Wio Time/Telemetry、运行峰值 RAM 或
长期循环。Wio BLE/RTL 长稳、BLE 首次延迟、C4 服务器后续维护继续后置，不转向无缝扩展。

## 11. C3 关闭前审计与有界剩余项（2026-09-16）

### 11.1 不再扩大的关闭清单

| 项目 | 当前判定 | 关闭所需工作 |
| --- | --- | --- |
| 单 ZIP 安装/来源/依赖/ELF | 七目标已通过 | 不因文档变化重跑七目标冷编译；实现/编译输入改变才定点重跑 |
| 基础平台实物 | S3 WiFi/BLE/WiFi+BLE、R4 WiFi/BLE、Wio WiFi 已覆盖；各自恢复边界见 §3/§7～§10 | 不再重复注册、清授权或重新做全部平台；C1/C2 深故障沿用原冻结证据 |
| 当前实现、profile 和实际 App | 本批核对完成，见 §11.2 | 归档各自制品与网络差异，不伪称七目标、所有实物均使用字节相同的网络配置 |
| 已知环境恢复异常 | **未关闭：§7.3**；代理单独重启、单 Broker 恢复已过，但不能替代 | 修正当前测试前置后做一次有界对照，给出自然恢复证据及历史失败的可解释结论；若仍失败先修相应模块 |
| 声明边界 | 本节冻结为基本接入、控制/状态、有限恢复和退出回收 | 如实保留未测项，不靠删掉可选 TLS 或降低安全校验来关闭 |

这里“六种模式已覆盖”不等于每种模式具备同等深度的全部 API/故障测试。普通 MCU reset
不等于整板长断电；跨平台 Time/Telemetry、运行峰值 RAM、任意 SDK、长稳、全部权限与
来源组合未验收。Wio BLE/网关/LAN 扩展、100 次循环与无缝 SLA 不进入本包。R4 上传握手
偶发失败需拔插的工具链/桥接限制保持公开，不据此凭空修改 BLE wire。

**C3 关闭不等于候选可对外发布。** C4 的干净 runner、正式 profile/信任材料、匹配三端发布
与跨片配置仍未完成；服务器维护暂停不应阻止本机 C3 对现有故障做有限复测。当前没有
要求先完成整组部署/迁移/回滚操作器，也没有授权据本报告自动推送或发布。

### 11.2 当前源码、安装包与现场的交叉核验

本批使用原 release generator 和原 `c3-matrix-20260914/profile.json`，在全新 stage
从当前源码生成 ZIP，核对文件集合及全部 388 个文件摘要。与 §1 原 `77cc302f…` 包相比，
**只有 README.md 的官方 SDK 说明变化，没有编译输入变化**。新生成包 SHA256 为
`24f116e9dc1d5ebd34be32fc7a5fab14075e2efdc8d60e776226b4053a54e1ab`，不是原包的摘要。
这项工作是重打包/逐文件审计，未冒称进行了新的七目标冷编译或实际 IDE 安装。

R4 WiFi 实物 ZIP `90a314b9af861313823137ebde0526be6ebca49444091a8fa249816d893acfc6`、
Wio WiFi 实物 ZIP `39b19ad7f561e352b6d69a65522740d05e0f5cf399dede3e69176d46e63fb2de`
也校验归档摘要和安装库文件集合；相对原 C3 只差 README 与地址 profile 生成的
`OfficialConfig.h`，实现源码没有额外分叉。不能直接将仓库中的测试公钥头与生成头逐字节
相比来判断实现漂移；原生成器会输出规范化信任材料，本批比较的是同 profile 生成结果。

手机通过 `pm path` 定位实际 APK，再读取文件 SHA256，仍为 C4-B 已验收包
`d1ceb65ca1c75fbdd90053d5c3b48ec6767dcead39021befcfe249b2d3ec621c`。
只读原 App 目录/账户：原 owner、首页、账户 MQTT ready、连接组/BLE link/测试钩子为 0、
LAN 无占用；Wio cloud reachable，原 TLS 两个逻辑见证及旧网络混合见证不在线。本批没有
发送 Property/Action、重新认领、打开 MCU 串口或复位设备，因此不是新的物理控制通过。

当前 mostfun2026 下电脑 `192.168.1.133`；TLS proxy 实际挂载证书只包含 localhost、
127.0.0.1、旧 `192.168.2.76`，有效期至 2026-09-19 07:10:10 UTC，不匹配现址。
这是**当前复测前置失败**，不能拿它证明 9 月 14 日 §7.3 的历史根因，也不能关闭验证绕过。
Wio 当前走明文的成功不证明可选 TLS 见证已恢复。

包边界 **17/17**、架构检查、release profile 检查通过。忽略目录
`build/c3-closeout-20260916/` 保留每次独立生成的 stage/ZIP 与 `preflight-*.json`；最终
`preflight-1789534082763.json` 对应上述结果。初始工具对原始公钥头/README 字节相同的错误
假设已修正，原失败输出/记录保留；没有改生产源码使测试表面通过。**c3Complete=false**。

### 11.3 下一步只做 §7.3 的有界恢复对照

1. 核对实际连着的 S3、逻辑 ID、应用/授权备份及 current profile；缺席的原 COM12 不按旧
   端口推定在线。选一块已确认可恢复的 S3 建立标准 WiFi 的可选 TLS 基线，不动 Wio 和
   其他板；若需换地址/证书/固件，逐项留摘要与差异，不清 NVS、不刷新 Key、不关闭证书验证。
2. 先证明真实登录、MQTT、fresh Manifest/State、一次 Property/Action 正常，并保留按钮计数
   与会话/启动证据。TCP 可达或首页缓存在线不能替代；基线不成立不启动停服测试。
3. 明确本次故障集合、原容器/配置及恢复动作，测试前依据现有登录/MQTT 重试策略固定有界
   观察窗口。整组测试服务恢复与单 TLS proxy/单 Broker 恢复分别命名，不借窄样本冒称全部
   Docker/USB 故障已测；不为复测做镜像升级、迁移或扩展 C4 工具。
4. 故障—恢复期间不开 MCU 串口、不 reset、不刷固件、不发 Property/Action 诱导上线。
   先取得设备自然恢复及 fresh 会话/响应证据，再各发一次属性/按钮，验证精确 ACK/State、
   计数连续和未重启。App 的 ADB reverse 丢失须单独记录，不算固件失败；未知 Action 不重放。
5. 退出释放资源，恢复原测试入口；记录成功/失败及历史 §7.3 的归因边界。不能排除串口
   打开/USB reset 干扰时明确保留不确定性，不声称“已证明只是工具问题”。若失败即定点定位
   登录、MQTT 或 SDK/测试固件的职责，不用扩大超时、100 次循环或第二套恢复编排掩盖。

通过并完成相应结论后，更新本节清单和主线 `C3` 状态；此前保持未关闭。正式 production
profile 和干净 CI 独立进入 C4，不与本机恢复测试相互循环依赖。

### 11.4 首次进入优化：紧接 C3 的下一小包

源码 `device-ui.port.ts` 的 Cloud-capable 普通分支在存在历史 Direct 凭据时，先等待
`ble.ensureReady(..., 5000)`，失败后才调用 `connectCloud()`；旧凭据对应的固件即使已
换成 WiFi-only，也可能走这条等待。因此历史页面约 5.6～5.8 秒与纯 BLE 本身约 3～6 秒
必须分开分析；后者包含扫描/GATT/鉴权/能力状态准备，不能单靠削短超时解决。

下一包只改原连接组的准备顺序：可用 Cloud 不被失败 BLE 候选串行阻塞，继续复用**唯一
账户 MQTT**；BLE 真正 ready 后才允许按既有规则选路，没有第二个 MQTT、重复页面组或
命令广播。无 nearby 缓存不等于没有 BLE，不能据此禁掉离线直连；纯 BLE、WiFiProv handoff
与网关子设备的原合同分别保留，不一刀切并行化所有模式。

先补候选成功/失败/迟到、Cloud 不可用、页面退出/后台/账号切换、无重复 Action 与资源回收
回归，再用匹配 APK 做短真机。分别记录凭据加载、发现、GATT/MTU、安全会话、Hello/
Manifest/State 耗时；缓存只能加速候选发现，不能代替鉴权或 fresh ready。暂不新增广播协议、
改变安全层、加重试队列或扩网关/无缝 SLA。若 C3 暂被测试环境阻断，可独立做测量准备，
但不得借优化通过把未解释的恢复异常改成已关闭。

本批仅完成审计、前置检查及文档收口；未修改设备/App/Service 生产源码、重启服务、烧录、
清授权、暂存、提交或推送。没有查实可删除的生产死代码，故不为追求删除量拆掉现有模块。

## 12. S3 恢复对照与 Wio 续签故障收口（2026-09-16）

§12.1～12.4 按发生顺序保留中间状态及失败；最终当前结论以 §12.6 为准。

### 12.1 S3 标准 TLS WiFi 的有界恢复

COM3 实物 MAC `8c:bf:ea:8e:f1:2c` 与原 owner 的逻辑设备相符。保留原 DeviceKey、NVS/
分区表；备份旧应用与首 64 KiB 后，仅写入标准 WiFiQuickStart 应用。测试 profile 改为
现址 `192.168.1.133`，TLS 叶证书补齐 SAN；本机 CA 以原密钥/subject/SPKI 续期，旧材料
保留。没有关闭证书验证；stock SDK 3.3.6 未启用完整证书日期检查，不能声称等同完整 PKI
日期验收。应用 1,061,680 B，program 1,061,539 B、静态 RAM 50,476 B，独立校验通过。

新 ZIP `f34ad407955bb31ffbe3e45d623ad52dc1adb2218e1fd1844c33ab0422d7680a`；应用
`199aa9ba090a8605a52f1971e06b53727e21468c9900fa963c0fb7afa9cbf6b1`。本轮不是七目标
重新冷编译；只有 S3 TLS profile/Sketch 用户输入变化，设备库实现未变。

基线原 APK `d1ceb65c…`：page ready 548 ms，Property/Action 95/129 ms，按钮 0→1。
按预定 120 秒窗口停止九个既有 IoT 容器并恢复：实际停止 38.414 秒，启动后 28.006 秒
自然取得 fresh State/ready，按钮仍为 1。未打开 MCU 串口/复位/发送业务命令诱导恢复；
随后 Property/Action 179/202 ms、按钮 1→2、revision 3→5。原账户 MQTT 会话自然重建，
退出 groups/BLE links/hooks=0。watchdog 未干预；全部 16 个容器恢复，ID/镜像不变，
其他七个容器启动时间未变。没有升级镜像、迁移 Auth、清授权或旋转 Key。

原 §7.3 环境/USB 异常未在这次标准 TLS 固件的有限对照中复现，仍不归因为 NimBLE、TLS
或 USB 的某个已证实根因。整机/USB 控制器重启、长断电、正式部署仍不包含在本次证据中。
原始证据：`build/c3-closeout-20260916/recovery/fault-1789536443110.json` 及同目录摘要。

### 12.2 新 Wio 恢复异常：当前唯一 C3 阻塞项

随后在已正常工作的账户/服务下补查 Wio，25 秒页面窗口未取得 fresh State，cloud lastSeen
停留于 `1789536444108`（停止服务时），按钮/属性均未发送。COM6 始终在线，尚不能区分
设备 TCP/SDK 阻塞、登录重试终止或网络问题；不能用缓存三字段 Manifest 算作在线。
已关闭失败页面、释放资源，没有清 QSPI/授权、升级 RTL 或首先复位来隐藏现场。

早先 S3-only 汇总过早标记 `c3Complete=true`，已纠正为 **false**，S3 原始通过证据不变。
`c3-wio-20260916/page-open-selection-apk-1789536992957.json` 和对应 close 保留失败现场。
下一步仅加入按需公开诊断到私有测试 Sketch（无自动重置/授权变更），定位后恢复标准示例
并重复有限停服恢复；不以故障后的人工重启正常关闭本项。

静态审查发现 rpcWiFi 1.1.0 的两参数 TCP connect 默认无限等待，而 HTTP 适配器已调用
250 ms 三参数入口；MQTT 仍经默认入口。它是待验证风险，不是当前失败的已证实根因。
不能直接传入数秒 timeout：该 SDK 的 timeval 仅填写 tv_usec，需注意秒边界兼容性。

### 12.3 首次进入小包：代码通过，实物覆盖边界明确

App 仅在原普通持久页面连接组并行准备 Cloud/Direct 候选；Cloud 继续共用唯一账户 MQTT，
只有已鉴权 BLE ready 才切路。一次性读取、纯 BLE、网关子设备与配网 handoff 原逻辑不变。
覆盖候选/凭据迟到、失败先后、已有 BLE、退出/后台/账户取消；Vitest **456/456**、类型检查
和两文件 lint 通过。首次扩大执行误纳 Jasmine-only 文件造成 13 个 suite 加载失败，随后按
各框架入口重新执行 Vitest；不能把这次工具选取错误写成生产故障或隐藏原输出。

新签名 APK `6b45f1875d6d3d3587b5fe27f2469b5aaef0f302d0bb6ff51792d7c833caa4b8`
包含逐字节校验的 326 个 JS 资产，已正常更新安装、保留原账号数据。短真机：COM3 Cloud
ready 620 ms、Property/Action 143/94 ms；COM11 自动 BLE ready 3,649 ms、267/276 ms，
精确 State 和退出回收通过。未证明纯 BLE 更快，也尚未完成 BLE 不可用但 Cloud 在线的
真实前后对照。证据 `build/c3-closeout-20260916/selection/`。当前先修复 Wio 的 C3 尾项。

### 12.4 定相位复现与最小修正

只增加按需 `printDiagnostics()` 的诊断 Sketch（库实现不变）经编译/来源/独立读回通过，
145,240 B 应用、静态 RAM 68,072 B；COM6 原授权/QSPI/RTL 未变。非续签窗口短停服自然
恢复 29.457 秒，主循环持续响应、按钮 1 保留；恢复后属性/按钮 144/313 ms，按钮 1→2。
一次重复调用已用过的测试钩子被前置拒绝，未发送命令；关闭后重新开页才进行上述控制，
不是重试未知 Action。串口初始 DTR=false 无输出，115200/DTR=true 后正常，uptime 连续；
本轮有诊断串口观测，不混写为完全无串口的标准示例门禁。

原失败前最后 session 在 `1789535995054` 签发，停服开始约在签发后 448 秒；实际配置
refreshAfter=480 秒、TTL=600 秒，停服覆盖续签点。按同相位安排第二次有限对照，
`fault-1789539536519.json` 的 120 秒窗口再次失败：Wio uptime 持续增加、network=1，
但 productError=5（UnsupportedFeature）、degraded/cloud=0，服务器无新成功续签。
这次复现排除了“MCU 主循环无限卡住”作为该故障的解释；没有根据猜测换 SDK/网络库。

根因链：HTTP 入口暂不可用 → 本机代理产生 chunked 空 502/504 → 小型 HTTP 适配器
不支持 chunked，返回 UnsupportedFeature → 登录提供者将协议错误保持为 terminal，
因此普通重连不会恢复。独立代理测试与适配器测试先复现失败，再作两个小改动：

- 代理自己的空错误响应显式 `Content-Length: 0`，不再让 Node 自动分块；正常转发不变。
- 设备只对 408/425/429/5xx 的 chunked 响应关闭 socket 并返回 WouldBlock，复用现有
  有界退避。200/重定向/鉴权拒绝等仍拒绝不支持的编码；不解析或接受 chunk bytes，
  不新增缓冲区、HTTP chunk decoder、协议/鉴权开关或重试队列。

设备 host ASan 回归 **62/62**；代理 **10/10**（新增四个 HTTP/HTTPS 502/504 真实 socket
响应头测试）通过。host 首次启动缺 ASan runtime PATH，补齐工具链路径后重新执行；
测试 FakeClient 新 socket 的残留输入问题也已纠正，随后旧实现确实在重试分类断言失败。
没有把工具/假对象失败当作设备故障。

修复位于通用 Arduino HTTP 适配层，不局限 Wio；正式库共享实现只有这一处头文件变化。
因此 §11“所有编译输入不变”的结论只适用于旧审计时点。正在生成更新 ZIP 并补受影响
WiFi 目标的编译/来源检查，Wio 恢复标准示例后必须再过同相位停服/恢复，才允许关闭 C3。
此时仍 **c3Complete=false**；不以诊断程序非续签窗口的一次成功关闭问题。

### 12.5 修正后标准 Wio 已通过同相位门禁

新 Wio ZIP `eba23d666f3ab40ed9920c61fe20fef8894cafa240adfe8c55e830a4022272ab` 与旧包
逐文件比较，仅 `ArduinoClientHttp.h` 改变；原标准 Sketch 只含既有用户参数，没有诊断/
自动重置逻辑。应用读回 SHA256 `57db2967c1555ec452c8b85695db7c5cd9eeb7cb17b5458922f7fcebfc092602`，
program/应用 143,072 B（+32 B）、静态 RAM 68,072 B（不变）。原 Key/QSPI/RTL 保留。

基线 ready 823 ms、Property/Action 206/206 ms、按钮 0→1；重新开页等待真实 session
480 秒续签边界。在签发后 448 秒启动同一九容器故障，停服窗口确实覆盖续签点。
`wio-fixed/fault-1789540830789.json`：启动服务后 **32.794 秒** App 恢复 fresh ready，
设备新 session 于启动后 8.908 秒签发，按钮保持 1；随后 Property/Action **218/115 ms**，
按钮 1→2、revision 3→5。观察期间无串口/MCU reset/业务命令，watchdog 未干预。
原容器 ID/镜像保留，其他七容器未变，退出 groups/BLE links/hooks=0、LAN 无占用。

代理旧/新实现还作了隔离 localhost 原始响应比较：同一个空 502，旧实现明确生成 chunked，
新实现 Content-Length=0、无 Transfer-Encoding，见 `wio-fixed/outage-wire-repro.json`。
这不是抓取 DeviceKey/账户请求，未向运行服务注入伪造凭据。

修复后的 R4 WiFi 安装/来源/依赖/ELF 补编译通过：program 139,016 B（+40 B）、静态 RAM
13,052 B（不变），未烧录缺席的 R4。Wio 与 R4 新包都只有同一个 HTTP 头文件差异；
ESP32 补编译正在完成最后核验。新包边界 17/17、架构/profile 检查也已通过。
**C3 功能尾项已通过，当前只待 ESP32 补编译汇总；未伪称重新完成七目标冷编译或生产发布。**

### 12.6 最终冻结结论

ESP32S3 TLS WiFi 的维护补编译也通过安装字节、来源、依赖和 ELF 检查：program
1,061,599 B（+60 B）、静态 RAM 50,476 B（不变）。R4 +40 B、Wio +32 B；三个 profile
各自与其原包比较，都仅改变同一个 `ArduinoClientHttp.h`，没有增加运行时缓冲区。
这次是七目标基础矩阵之后的定点维护补编译，不冒称新七目标完整冷编译或 R4 再次烧录。

| 本批维护包 | ZIP SHA256 | 结果 |
| --- | --- | --- |
| Wio 当前地址 / Plain WiFi | `eba23d666f3ab40ed9920c61fe20fef8894cafa240adfe8c55e830a4022272ab` | 编译/来源/ELF＋标准固件独立读回与续签故障真机通过 |
| R4 当前测试 profile / Plain WiFi | `1379f27e7ec972d9975dd13884b2d33777568cacc3588c247280492386e7c126` | 编译/来源/ELF 通过，本批未烧录 |
| S3 当前地址 / TLS WiFi | `e0bfcd0a01e89047ef78cbc6ff0812339c08767684d16cd1cbfe038613ab45bf` | 编译/来源/ELF 通过；COM3 本批先前的标准 TLS 恢复实物证据仍对应修正前镜像 |

上述是测试 profile 的可追溯制品，不是要求用户分 MCU 安装多个库。公开结构仍是同一套
Arduino 库/四个标准例程；正式统一地址与信任材料属于 C4，不能直接发布带本机地址的 ZIP。

`verify-final.cjs` 交叉检查了原 S3 对照、Wio 新旧定相位结果、新 session、计数/ACK、退出
资源、三目标单文件差异及手机实际 APK 摘要，生成 `final-summary.json`：
**c3Complete=true，冻结的基本接入/控制/有限恢复矩阵已收口。** 最后 Wio 实际停服
37.341 秒、App 自然 fresh ready 32.794 秒；这属于故障恢复，不是页面首次连接耗时。
host 62/62、App 456/456、代理 10/10、包边界 17/17、架构/profile 检查通过。

最终现场：COM6 为修正后的标准 WiFi 与原 Key，没有诊断/维护程序；COM3 为本轮标准 TLS
WiFi，COM11 原 WiFi+BLE 固件/授权未改；手机为 `6b45f187…` 匹配 APK、首页、唯一账户
MQTT ready、本地 groups/BLE links/hooks=0。代理已在原容器恢复中加载空响应 framing 修正，
未改 Service 业务路由/数据库、迁移/镜像或设备授权。没有暂存/提交/推送。

仍明确后置：干净 CI/正式部署、全部可选 API/跨平台 Time/Telemetry、Wio BLE、长断电/
宿主 USB 故障、100 次循环、峰值 RAM、无缝 SLA。原 SDK 默认无 TCP connect 上限风险
独立保留，不能借本次不同根因的通过删除；本批没有引入猜测性 SDK wrapper。
当前转入主线 §23 的首次进入实物对照，纯 BLE 3～6 秒握手尚未优化。

后续状态（2026-09-18）：上述首次进入对照已按主线 §24 完成，并修复等待 Presence 期间的
页面取消竞态；匹配 APK 的 Cloud/退出/原固件 BLE 恢复通过。见
[首次进入记录](DEVICE_V2_FIRST_CONNECT_OPTIMIZATION_REPORT.md)。这是 C3 后的定点维护包，
不是重新执行全部 C3 矩阵；C3 既有冻结证据及排除项不变。
