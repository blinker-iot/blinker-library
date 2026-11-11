# C4：核心稳定版可复现构建与部署

入口更新：2026-09-22。上位入口：[新进度与执行计划](DEVICE_V2_NEXT_EXECUTION_PLAN.md)；C4 历史证据见 §10。新 S1/S2 只提取依赖、候选配置与构建闭包，**不等于恢复下方 C4-A 整组替换/升级**。下文“下一步”为历史计划，需服从新队列。

**最新优先级覆盖：按核心稳定执行表 §19，服务器后续稳定维护暂放，下面整组替换/新实例
准入等“下一包”不自动执行。R4 普通 WiFi/BLE 与 COM6 Wio 普通 WiFi 已按 §19/§20 补证；
Wio BLE 保留后置；C3 已按 C3 报告 §12.6 有限收口，首次进入/缓存小包也已完成。C4 未完成项
保留，不把本暂停写成部署已通过，也不自动恢复下面的整组维护任务。**

## 1. 当前状态与顺序

| 包 | 本批已完成 | 剩余退出条件 |
| --- | --- | --- |
| C4-A Service 配置一致性 | 配置/预检、分片事务、固定制品、整栈隔离、真实多库备份已过；原实例 PG＋3 SQLite 回写/中断续跑、Redis 路由失效通过；102/102＋12 项 Docker 门禁，见 §10 | **静态/动态整组替换及新实例准入尚未完成**；现有栈未备份/升级，配置/Secret 变更回退未实现。通过完整隔离门禁后再维护部署、真实双账号跨片/撤权 |
| C4-B App 构建 authority | 已按新入口生成并保留数据安装 APK；包内 326 个 JS/签名/实际 authority/原账号目录、账户 MQTT、两轮 S3 BLE 页面及冷启动通过，见 §6 | 该构建入口的有限验收已完成；不是生产 release、换网后设备 MQTT 或全平台/全权限回归 |
| C4-C SDK/单 ZIP | 官方 S3 3.3.11 完整溯源；同 ZIP CI 五＋二目标、摘要 preflight 和包边界 17/17 | 干净 runner 安装/编译/实际依赖/ELF 完整跑通，保存工具链、依赖和制品摘要；不能把本机旧 C3 结果写成新 CI |
| C4-D 候选发布 | 正式/preview 边界不变；C3 基础矩阵及 HTTP 临时错误恢复修复已有限收口，见 C3 §12.6，不增加用户构建工具或设备协议 | 最终三端发布制品、云端地址/公开信任材料/迁移、秘密隔离清单和干净 CI 仍需完成；本机定点补编译不等于生产发布 |

本批仅修既有配置缺口，没有改 MCU 生产实现、Service 路由业务/数据库或用户 API。配置变量属于部署层、App authority 属于构建层、SDK 检查属于 CI；不把这些塞入 Sketch 或协议帧。

## 2. Service：单一配置来源，不等于已部署

沿用 `DEVICE_V2_CROSS_SHARD_REQUESTS_ENABLED` / `DEVICE_V2_CROSS_SHARD_SHARES_ENABLED`，不新增开关。
根 `.env` 或启动进程环境决定 Compose 插值；显式映射优先于 Gateway env_file。Requests 同时进入 Gateway、所有静态 Broker、未来动态模板，shares 仅进入 Gateway。默认均 false，签发分享需要两者同时 true；开启不取代 ACL/账户认证。

回归先在原清单复现静态 Broker 未收到 requests，修正后默认及四种组合通过。Controller 原 create spec 回归验证 requests 正确且不泄入 Gateway 专属 shares，不修改 Controller 运行代码或新增配置解析器。

下一批部署顺序：

1. 只读盘点所有静态/动态 Broker、实际镜像摘要、当前开关及模板 fingerprint/reservation；保留原 DB/配置备份，范围限定本机测试栈。
2. **先补受控维护升级流程（§6.2），不能假设已有实现**，再处理已有动态容器及登记，保留 SQLite/PG/ledger。只改模板不会更新既有 Broker，不能直接覆盖入口或跳过冲突校验。
3. 所有参与请求的源/目标 Broker 对齐后再开放 Gateway 请求/分享签发。核验进程实际配置，再用真实双账号和真实设备验证跨片查询/Property/Action、权限撤销及断线恢复。一次命令一次执行，未知结果不自动重放 Action。
4. 任一分片未对齐，停止开放签发；精确回滚已修改配置/镜像而不清数据。回滚可能暂时中断已有跨片数据面，必须明确提示，不能以保留旧页面缓存冒充可用。

Service 的操作说明在 [deployment.md](../../aily-blinker-services/docs/deployment.md)“Device V2 跨分片开关（C4）”。本批 `compose config` 和 fake Docker 单测不创建、重启或升级任何容器，不算部署通过。

## 3. App：构建配置先于凭据作用域

浏览器 local 仍 HTTP `127.0.0.1:7000`；Android USB 构建叠加 `local-device`，使用 HTTPS `127.0.0.1:17000`。不修改基础 URL 常量、运行中 environment 或已生成 bundle，也不迁移另一 authority 的授权来绕过隔离。

执行 `npm run build:local-device` 后，使用原 Capacitor copy/Android debug 构建；本机 HTTPS proxy、ADB reverse、仓外 `BLINKER_DEBUG_NETWORK_CONFIG_DIR` 信任材料必须匹配。release 不信任 debug CA。生产配置、签名、LAN 默认 false 不变。`www` 中共享基础对象可以保留 7000 字面量，但实际被消费的 environment 必须是叠加后的 HTTPS 17000，不能简单按出现任意 7000 就判失败。

首批新增配置两项回归，与页面/选择器相关回归合计 111/111；App/spec TypeScript、定点 lint、Angular builder 成功。首批未 native 打包/安装的状态已由 §6 更新；新增真机证据来自新 APK，不借旧 APK 的测试为新制品背书。

## 4. SDK：纠正私有 SDK 误解

S3 BLE/WiFi+BLE 使用的是 [官方 Arduino-ESP32 3.3.11 / ESP-IDF 5.5.5](https://github.com/espressif/arduino-esp32/releases/tag/3.3.11)。本机 cached 归档摘要与 [官方包索引](https://espressif.github.io/arduino-esp32/package_esp32_index.json) 一致，安装 Core 2,052 文件与 S3 SDK 6,277 文件逐一比对无差异/额外文件（仅排除安装器 `installed.json` 元数据）。不是自定义编译或 fork，也不需要 NimBLE-Arduino。

| 本机 S3 SDK 配置 | 官方 stock 3.3.6 | 官方 stock 3.3.11 |
| --- | --- | --- |
| IDF NimBLE | enabled | enabled |
| IPC 栈 | 1024 B | 2048 B |
| NimBLE Host 栈 | 5120 B | 5120 B |

这只证明输入来源/配置不同，不证明某个低水位的根因已由 SDK 自动修复，也不外推其他芯片或旧 SDK 必然失败。C3 物理证据对应 S3 3.3.11，当前 CI 因而使用此基线。

可追溯摘要（SHA-256）：

- 官方 Core ZIP：`a18203f2429f5551ec929b80e8129e439df7a3a009b536b305f00b4a1f1bbb62`。
- 官方 S3 SDK ZIP：`fec76794708694120c6ec803cdbfc7dcd36286ea22b7d38097b7909bbd19f7b5`。
- 关键 `platform.txt`、`sdkconfig` 和 `libbt.a` 摘要固定在 `tests/arduino/verify_esp32_ble_sdk.py`；CI 使用显式的隔离 `ARDUINO_DIRECTORIES_DATA`，检查器只读、不修改用户 SDK。官方 CLI 安装阶段校验归档摘要。

同一个 ZIP，先 stock 3.3.6 WiFi/CloudSmoke＋R4 WiFi/BLE＋Wio WiFi（五目标），后官方 3.3.11 BLE/WiFi+BLE（两目标）；实际依赖/来源/map/ELF 校验未放宽，Wio BLE preview 门禁仍单列。Python 和摘要检查是维护/CI 工具，Arduino 用户只安装官方开发板包和一个 Blinker ZIP，不增加私有 SDK、平台专用 ZIP 或 Sketch 配置参数。

远端 CI 本批未触发；干净 Linux 安装仍为待验收。不能仅因源码 workflow 修正或本机 SDK pin 通过就标七目标远端成功。

本批按公开 `tests/release/development_profile.json` 两次独立打包，均为 388 文件，ZIP SHA-256
`2a7c6438f028037149ad6eed3f1c2853d26615d6ea5db01e03ade5c6736c2cae`。这是 **CI fixture 包，不是待烧录现场包或生产包**。
与 C3 本机真机 ZIP 比较，仅 README 及两个 profile 生成头 `OfficialConfig.h` / `PortableServerKeys.h`
不同，其余源码/示例逐字节一致。首轮“仅 README 不同”的测试断言确实失败，原因是 C3 使用本机
公开验签材料/地址，而本轮使用公开 CI fixture；未改生产代码或配置使断言表面通过。
`package.json` 保留差异、用途及旧断言原因。下一批现场构建须独立核验当前 authority/公钥，
不能把不同 profile 的编译/真机证据合并为同制品通过。

## 5. 网络与本轮证据边界

用户网络改为 `mostfun2026`，只读发现电脑 WiFi 地址 `192.168.1.139`，与此前测试 profile 不同。手机/三板各自网络、设备到 Gateway、手机到设备必须在下一批主动故障测试前重新核验；不沿用旧 IP/SSID，不把换网本身判为软件恢复失败。本轮没有改设备 WiFi、刷固件、resetAccess 或清绑定，未触发整套服务/USB 故障复测。

本机 ignored `build/c4-config-20260915/` 保存 SDK 溯源 `sdk-audit.json`、Angular `app.xml`、配置/包边界等六组原始日志及 `checks.json`、两次独立 CI fixture 打包和逐文件对照 `package.json`。涉及账户/私钥/网络密码不进入公开产物。源码已有其他变更均保留，未暂存/提交/推送。

R4/Wio 实物明确暂缓。BLE 首次探测耗时作为体验边界记录，默认先完成本页配置/部署和已暴露的恢复问题，不展开新抢占/并行握手方案，也不清历史凭据掩盖延迟。

## 6. C4-B 新 APK 闭环与 C4-A 升级前置纠正（2026-09-15）

### 6.1 可复现 App 入口已真正装机

使用与 `npm run build:local-device` 相同的 Angular 配置命令，随后原 Capacitor copy、Android
`assembleDebug processReleaseMainManifest`，没有修改生成 JS。APK 84,741,335 B，SHA-256
`d1ceb65ca1c75fbdd90053d5c3b48ec6767dcead39021befcfe249b2d3ec621c`。
326 个打包 JS 与原 Angular 输出逐字节校验，Capacitor HTTPS/cleartext=false、桥日志关闭、
debug CA 摘要/签名均通过，release 合并清单不包含本机网络信任；本轮未构建 release APK。

首次 ADB 备份在手机断开时失败，只得到 327,680 B 残片，已改为明确 incomplete 文件名保留，
不是有效回滚包。用户允许调试后恢复 ADB，读取手机旧 APK 摘要与既有 C3 完整备份吻合后，
保存完整备份，再 `install -r`；重新读取手机实际 base.apk 摘要，与新 APK 完全一致。
未卸载/清 App 数据，原 owner/绑定继续。Gradle 首轮实际 52 秒成功后守护进程持有管道，
只在确认本批 daemon 已 idle 后关闭该进程，未结束其他构建/IDE/服务；未把等待误判为编译失败。

| 新 APK 有限真机 | BLE ready | Property / Action ACK | 结果 |
| --- | --- | --- | --- |
| 新包首次进入原 COM11 混合设备页 | 3,231 ms | 269 / 299 ms | Manifest/State；power false→true，按钮 0→1 |
| 真实 App force-stop/冷启动后再进入 | 3,528 ms | 266 / 267 ms | 原授权继续；power true→false，按钮 1→2 |

两轮原账号目录 API 成功、实际 authority 为 HTTPS `127.0.0.1:17000`、账户 MQTT ready。
每轮页面内账户 Session 未替换；跨进程当然建立新账户连接，不声称冷启动仍复用旧进程 MQTT。
每轮退出 groups/BLE links/hooks 为 0、LAN 无占用，默认 LAN 未开启。COM11 未刷固件/重置；
COM3/COM12 未操作。控制有真实 ACK 和精确 State，不是工具伪造回执。
这是 **App→BLE→S3** 与 App 管理面/账户 MQTT 的有限验收，不是设备 MQTT 或跨片分享通过；
未改变设备网络参数，换网后的三个设备云端接入仍需单独重新核验。

### 6.2 部署审查必须纠正的前置缺口

只读实际盘点 Gateway、Controller、Broker 0/1/2：当前 requests/shares 均关闭；Broker 2 带
reservation，匹配运行中 Controller 的旧模板指纹。仅在内存加入 requests=true 或按仓库
Compose 生成候选模板，都不再匹配。当前 Controller 没有 upgrade/replace 动作，原
create/start/stop/restart/remove **不是**原地保 reservation 升级流程。

此前“按原受控流程重建”的文案已纠正为**先实现维护工具/验收**。新增只读
`scripts/controller-template-preflight.js` 复用 `loadConfig` 和 `BrokerController` 的原校验，
不创建第二套 fingerprint 算法、不输出 secret/保留关系、不启动/停止/删除容器。命令见
Service deployment.md。实际运行退出 2，Broker 2 明确 `SHARD_TEMPLATE_MISMATCH`；各容器
ID/镜像/Running 与盘点前一致，未以入口补丁、标签重写、删租户或清授权完成测试。

Controller 27/27＋预检 5/5（根 `npm run test:controller` 共 32）通过：新增默认 false、开启
true、换镜像、generation 和动态范围变化的拒绝；原模板恢复后同 reservation replay；
无 Docker 写入/删卷、无敏感元数据输出。root 变化即使 key ID/generation 未变也拒绝，不能
仅凭相同标签认为派生凭据一致；root 轮换不属于本包。Compose 四组合继续通过。

运行层还需纳入下一包快照：只读 `docker diff` 显示 Gateway 的 index/local-access-repository
和静态 Broker 的 index 存在文件层标记。这不等于其当前内容必然不同于镜像（历史复制后
可能已恢复原字节），必须核验实际 SHA/构建来源，不以镜像标签或容器 Running 代替源码一致性。

**下一包固定为 C4-A 单机维护升级，不做零停机平台：**

1. 先定义可审计的计划/检查点，冻结同一 project、精确当前 container ID、旧/新 template、
   原 reservation、凭据 generation、数据卷和回滚镜像。credential root/generation 轮换不混入本包。
2. 复用原 Controller spec 构造/派生凭据和 Docker adapter；不要让公共 HTTP 请求任意指定
   镜像/环境/挂载，也不扩大 App API。保持同 reservation、数据卷及租户归属，不走新认领。
3. 在维护窗口停止新分配，确认迁移/cleanup 空闲；生成并校验 PG/所有 SQLite/配置备份。
   工具确认真实容器状态后才逐步替换；失败/中断可恢复原镜像/模板/保留关系，不能只靠脚本
   内存里的“已完成”。先 fake Docker/隔离 Docker 的失败与回滚门禁，**再操作当前测试栈**。
4. 最后才按全源/目标 Broker→Gateway 签发的顺序启用开关，核验 inventory、原 reservation
   replay、数据/凭据/健康，再执行双账号真实跨片与撤权；任一步未过，不标 C4-A 完成。

### 6.3 证据与未完成范围

ignored `build/c4-native-20260915/` 保存原始构建日志、完整旧/新 APK、打包/签名/安装校验、
实际进程/镜像盘点、预检与 32 项回归、`verified-summary.json`。BLE 原工具两条新样本
`app-ble-cfour-new-build-*` / `app-ble-cfour-cold-*` 保存在原 C3 工具目录，并由本批汇总
重新断言 ACK/单次状态变化/原 Session/退出回收；没有覆盖旧失败记录。

下一步不重复构建同一 APK 或再测两轮 BLE 来替代动态分片维护缺口。C3 整套环境恢复、
R4/Wio 实物、干净 SDK CI、生产信任 profile 均未关闭。本批没有服务部署/DB 迁移、设备
刷写/清授权、提交或推送；没有把 USB 中断当 BLE 缺陷。

## 7. C4-A 维护内核及隔离 Docker 门禁（2026-09-15）

本批完成 §6.2 的事务内核/持久检查点/隔离失败门禁，**尚未部署现有测试栈，也未关闭 C4-A**。
设备/App 不增加源码或协议；没有清理绑定、刷板、开启分享或重复执行 BLE 样本。

### 7.1 最小实现和明确边界

Service `scripts/lib/broker-maintenance.js` 复用 `BrokerController.buildCreateSpec`、原配置/
指纹/metadata 校验以及 Docker adapter；独立操作器模块，不扩 Controller HTTP。新增 Docker
inspect image/volume、rename 原语。冻结精确旧 ID、同 reservation、原数据卷、固定候选镜像
ID，拒绝 root/key ID/generation/项目/分片池/网络/卷变化；候选标签增加本次维护 ID 以识别
响应丢失后的真实候选，不能把同名外来容器当作本事务创建。

`maintenance-journal.js` 保存无原始 env/token 的计划、摘要链/序号/fsync 日志和独占锁。
配置再次加载后与原计划校验；每次写操作前记录 intent，返回后记录 done。真实子进程退出
验证已落盘意图可读取、残留锁阻止第二操作者；人工核实进程退出后才恢复。截断/摘要损坏
直接拒绝，不自动删尾部或靠超时抢锁。尚未做主机/VM 断电或磁盘损坏门禁。

维护三步含义不能混淆：

| 阶段 | 已保证 | 尚不能宣称 |
| --- | --- | --- |
| apply → ready | 停旧保留回滚容器；同卷、同 reservation/派生凭据候选健康 | 正常 inventory/可以解除维护；旧容器 labels 仍会触发重复/指纹拒绝 |
| rollback | 停/删除精确候选但不删卷；验证并恢复数据库；原 ID/名称/运行状态恢复 | 只恢复旧镜像即可撤销数据库变化 |
| commit 最终确认 | 重新验健康，删除停止旧容器；同 reservation replay/唯一库存 | 最终确认后仍可对已删除旧容器直接 rollback；需要另一个镜像/备份恢复计划 |

故障不自动吞掉，也不在不确定状态下自动重新开放入口。候选已经运行而健康失败，也会恢复
数据库而不只是换回旧可执行文件。已成功回滚再调用不重复写数据；开始最终确认后只允许
沿最终确认方向续跑。跨分片整组最终确认及失败补偿仍是下一批整栈操作器的职责。

**纠正维护窗口遗漏：** `offlineFence` 仅检查精确 Controller/Gateway 实例均已停止且自动
重启关闭，它是分配隔离，不是全流量隔离。现有栈操作器还须持有整项目排他锁，暂停 MQTT/
HTTP 入口、相关 Broker/自动化写入及迁移/cleanup。PG、每个 SQLite、原配置与有效回滚制品
均须验证后才能替换；注入的 backup.verify/restore 必须接真实备份。原有 SQLite 备份/校验
脚本继续复用；隔离 fixture 不替代生产数据恢复。重新放开流量后不能再用旧快照覆盖新写入。

### 7.2 测试结果和真实失败修正

- `npm run test:controller` **54/54**：原 32 项＋维护 22 项。覆盖 stop/rename/create/start/
  remove 成功后响应丢失、健康失败/超时、commit/rollback 续跑、备份损坏/恢复失败、落盘失败、
  占用锁、日志损坏、原容器/外来候选/卷漂移、root 同 generation 变化和保留关系 replay。
- Compose 默认与四组合回归继续通过，维护模块未进入用户/设备 API。
- `npm run test:controller-maintenance-docker` 只创建随机命名 project、内部网络、无公开端口、
  独立测试 SQLite/卷。真实 Docker 通过两条路径：不健康候选写入后回滚原 ID/原数据；创建
  已成功但返回丢失后重新打开日志续跑、健康、最终确认、唯一 inventory 与同 reservation replay。
- 测试前后 **21 个既有容器**的 ID、image、Running、StartedAt 相同。fixture 容器/卷/网络/
  临时镜像标签已按精确所有权清理；验证过的 SQLite 备份及失败/成功证据保留。本项不运行实际
  Broker MQTT/ACL，不能算真实双账号跨片或授权撤销通过。

首轮并非全部通过：fake 用例曾被原 Controller 空分片池校验提前拒绝，改为有效但漂移的池
以测试维护边界；隔离 Docker 的镜像 FROM 裸 ID、SQLite WAL 只读侧文件检查及挂载集合
顺序校验也暴露了失败。已分别修为完整固定镜像引用、独立可校验快照/只读查询、按 Destination
规范化挂载集合，未删除失败证据或放宽卷/容器身份检查。

### 7.3 现有源码基线发现与下一步

只读现有容器，使用原镜像的无网络临时容器计算相同命名文件的 SHA：

| 检查 | 结果 |
| --- | --- |
| Gateway dist/index.js | 与原镜像一致，之前 docker diff 的 C 标记本身不证明当前字节不同 |
| Gateway dist/device-v2/local-access-repository.js | **与原镜像不同**；与本地 dist 及当前 src 按原 tsconfig 的 TypeScript 输出逐字节一致 |
| Broker 0/1/2 dist/index.js | 各自与原镜像一致 |
| Controller controller.js / docker-client.js | 与运行镜像一致；本批新增原语还未部署 |

这只证明所列文件，不是整个源码/依赖树溯源通过。旧 Gateway 镜像没有当前 local-access 修正，
不能直接当作当前有效行为的回滚镜像。**下一批顺序固定：**

1. 将已有 local-access 修正纳入可复现 Gateway 制品，核对当前有效源码与回滚清单；不继续
   向运行容器复制生成 JS。补齐 Broker/Controller 制品与静态/动态模板配对。
2. 实现现有单机栈操作器：真实配置重载/项目锁、入口及全写入者隔离、迁移空闲、PG/所有
   SQLite/配置/镜像备份及校验。先演练静态与动态整组失败恢复，再操作当前测试栈。
3. 在维护窗口通过全部 Broker→新 Controller inventory/replay→Gateway 请求/分享开关确认
   后恢复入口，执行真实双账号跨片查询/Property/Action、撤权及恢复，才关闭 C4-A。
4. 再补 C3 换网基线和整套服务/USB 恢复；R4/Wio 继续等待实物，干净 CI/生产 profile 未关闭。

成功证据位于 Service ignored `build/c4-maint-1789456495527-75d634/`：result.json、各事务计划/
日志、SQLite 备份及 runtime-audit.json。此前同前缀目录保留失败和有限证据，不当成功样本。
本轮未部署既有服务、未修改现有 DB、未暂存/提交/推送；主线仍是基础稳定与可复现部署，不是
无缝通信、History/Bulk 或新增网关形态。

## 8. C4-A 制品与整栈维护隔离（2026-09-15）

本批推进 §7.3 的制品固化和整栈关闭/原状态恢复，**不等于现有栈升级完成**。
设备/App 源码、协议和授权未改变；主线继续是核心稳定可用。

### 8.1 制品与回滚代码基线已固化

新增 Service `scripts/prepare-maintenance-images.js`，先复制原 Dockerfile、锁文件、workspace
配置和 src 到新建上下文并记录 SHA（本次 240 个输入），再构建 Gateway/Broker/Controller。
拒绝符号链接/私钥等异常编译输入，不携带本机 dist、环境文件、DB 或外层测试脚本；src 内
原有编译输入（含原 tsconfig 纳入的测试文件）保持不变。不会因构建
期间工作树继续变化而混入不同批次源文件。三种镜像真实构建通过，源码/镜像清单保留。

Gateway 旧运行层差异仍只有已审查的 local-access-repository.js；从新源码镜像提取的编译
文件与实际运行摘要一致。在原镜像上构建 recovery，Gateway dist 和共享 packages **179 个
代码文件**逐一与当前有效代码树匹配；不执行 docker commit、不复制实际容器环境或数据、不
向运行中 dist 写文件。这个 recovery 固化原有效代码，候选新镜像仍需业务/数据库兼容门禁。

补齐 Compose `CONTROLLER_IMAGE`：可以显式使用新 Controller 制品，默认镜像名保持原值。
它与动态 Broker 的 `CONTROLLER_SHARD_TEMPLATE_IMAGE` 分开，三端镜像不能混用。默认/
自定义镜像和四种 requests/shares 组合测试通过，现有容器开关没有变动。

镜像与只读清单位于 Service ignored `build/c4-images-1789457540541-75c4c7/`：
`images.json`、固定 context、`live-fence-readonly-plan-v2.json`、构建日志。候选/回滚及原栈镜像
归档另做 **linux/amd64** 专项导出：292,025,856 B，11 个镜像描述符、81 个 blob/引用均验证，
SHA-256 `e956939ba8492761774f0fa30cfd33d3ec850e97a125d60e9909a63b22e79bc8`。
以 `archive-verified.json` 为入口。尚未做干净引擎导入/启动，也未备份实际 DB/部署秘密。

真实失败记录保留：本地 recovery 的 repo@digest 首次触发远端解析失败，改为唯一基线
标签＋ID/输出内容校验；未选平台的首版归档存在未导出的 attestation 描述符，不当作完整
引用图校验通过。改用 CLI 明确支持的 linux/amd64 导出后完整图校验通过；不声称其他平台可恢复。

### 8.2 整栈维护隔离已实现并通过生命周期门禁

`stack-maintenance-fence.js` 独立于 Broker 替换，复用 journal/摘要与 Docker adapter，
不扩公共 API。每个运行目录的事务锁之外，使用共用私有锁目录按 project 加锁，跨目录第二
操作者会被拒绝；不按超时抢锁，不覆盖历史计划。摘要实现从分片模块收回通用 journal，
避免整栈隔离反向依赖 Broker 实现。

顺序固定为：入口→Gateway/Controller→Broker 停止；PG/Redis 不被操作。入口和控制进程
禁用自动重启；Broker 保留原 no/unless-stopped policy，以手动 stop 关闭，避免改变 Controller
创建合同。实际每步读回实例/配置/状态，PG/Redis 的运行状态/StartedAt 变化也拒绝。复原
顺序 Broker→Controller→Gateway→入口，必须先验证恢复证明并检查就绪；原来停止的容器不
被顺手启动，最后读回所有原运行状态和 policy 才记录完成。

整栈计划 version 2 明确记录数据存储实例 StartedAt。早期只读 version 1 缺少该字段，当前
执行器直接拒绝，不猜测历史基线；重新只读生成 v2，并核对两次的 ID/镜像/配置/Running/
policy 一致，旧计划继续作为历史证据保留。最终汇总见同制品目录 `verified-final.json`。

这只是**受控单机维护隔离**，不是宿主机故障/外部管理员操作下的全局分布式锁。停止写入者
后还必须检查迁移/cleanup 空闲和取得 PG/SQLite/配置备份。数据恢复接口不在本模块实现，
调用者不能注入始终 true 的回调当作真实恢复证明。

现场只读盘点修正了一项假设：TLS 代理属于主 Compose project，明文代理独立运行。入口
按明确 ID 指定，允许这两种归属并去重，禁止将数据库/Controller/Broker 误列为入口；未声明的
未知服务仍拒绝。现有计划覆盖 **9 个容器**，未调用其 close/restore 或更改 restart policy。

**重要边界：** `restoreOriginal` 仅接受原 ID/配置。执行过服务或分片替换后，它会拒绝，
不能用于新版本开放入口。后续协调器必须依据替换回执显式准入新 ID、镜像、配置、数据卷和
原 reservation，再验证库存/replay/权限后恢复入口，不能简单放宽未知容器检查。

### 8.3 门禁结果、清理与下一包

1. Controller/预检/维护/源码上下文共 **72/72**，Compose 配置矩阵通过；原 public action
   白名单不变。新增 policy 更新仅接受有界 RestartPolicy 字段，不能传任意 Docker update。
2. 真实隔离 Docker 覆盖 Compose/独立混合入口、stop 已成功但响应丢失后续跑、未就绪 Broker
   保持入口关闭、原状态/policy 恢复；同项目第二运行目录锁与配置/实例漂移另有确定性回归。
   fixture 的 PG/Redis 只是生命周期占位，不当作实际数据库恢复通过。
3. 原 Broker 隔离门禁在通用摘要整理后再次通过：失败候选回滚原容器/SQLite、响应丢失后
   续跑最终确认及同 reservation replay。现场既有容器未被本轮测试变更，fixture 仅清理自己的
   容器，日志/计划/备份保留；原有失败和前序成功样本不覆盖。

最终整栈生命周期证据：Service `build/c4-fence-1789458748566-8b6963/result.json`，21 个既有
容器 ID/镜像/Running/StartedAt 未变；Broker 回滚/续跑证据位于
`build/c4-maint-1789458199119-da7ea3/`。两类测试都只操作各自独立 fixture。

**下一包按此顺序收口，不再增加旁支功能：**

1. 接入实际单机栈的配置/备份端口：项目锁＋入口及写入者关闭→确认迁移/cleanup 空闲→PG、
   每个 SQLite、原配置/秘密的受保护备份与完整性校验；SQLite 复用原 backup/verify 脚本。
2. 整合静态服务与动态分片的整组替换、失败恢复、新实例准入及最终开放入口。现有原 ID
   复原路径不能偷换为新 ID 任意放行；先完成隔离的真实数据库与整组失败恢复。
3. 最后才关闭现有测试栈执行 Broker→Controller inventory/replay→Gateway 开关升级，恢复
   入口后完成真实双账号跨片/撤权，再补 C3 网络及整套服务/USB 恢复。

现有 requests/shares 仍关闭；C4-A 尚未完成部署。R4/Wio、干净 SDK CI、生产 profile 等
原缺项保留，不进行新 BLE/LAN/无缝通信扩展。本批未刷板、清授权、暂存、提交或推送。

## 9. C4-A 真实数据库备份与恢复门禁（2026-09-15）

### 9.1 完成的实施范围

复用整栈 fence/journal 和原 backup-sqlite / verify-sqlite-backup，新增独立维护数据端口，
不修改 Controller HTTP、设备协议或 App API。备份要求项目锁和原实例关闭状态仍有效，
输出只写新建空的受限目录；失败不覆盖之前输出，可在保持隔离后新建目录重试。

1. **秘密隔离：** Windows 使用当前用户＋SYSTEM 的受保护 ACL，读回验证；Unix 要求当前
   uid、0700。完整 runtime Config.Env 和显式配置文件保存在该目录；journal 只记摘要。
   密码通过 stdin 送给临时 PG 客户端，不写命令行/普通日志。当前配置捕获仅限显式给定文件
   与 runtime inspect，不能据此声称所有 JWT/卷内证书/部署 Secret 已完整打包。
2. **PG：** 仅支持当前 PostgreSQL 16 Gateway schema。拒绝其他客户端、未完成迁移/cleanup、
   非 normal 路由、进行中 Broker 操作/分配 lease、未支持的其他 schema/分区/外部表/大对象。
   持有迁移 advisory lock 和只读 REPEATABLE READ/exported snapshot，pg_dump custom/schema
   使用同一快照；SQLite 备份后再次比较 PG 行摘要/序列，变化则整组不签发完成回执。
3. **SQLite：** 使用每个原 Broker 自身镜像内的 better-sqlite3，不依赖宿主 Node 的 native ABI。
   源卷只读、先检查冻结/未清理导出、外键；原 SQLite 工具完成备份/新恢复副本及 integrity。
   再逐表比较内容摘要，不只检查行数。源 DB/WAL 前后摘要一致；原卷不执行 checkpoint/改写。
4. **可恢复性：** 在两个全新的无网络入口 PG tmpfs 实例中，分别恢复 custom 全库和独立
   schema-only 导出；比较全部行内容、序列、38 个 migration 记录与完整 schema（含函数、
   触发器、约束、索引、view、ACL）。不对既有 PG 使用 restore --clean。
5. **回执：** 文件 SHA/大小、各分片 ID/卷/镜像和项目 planDigest 绑定；备份清单本身标记
   restoreVerified=false，只有独立 restore-verified.json 证明恢复验证，不混淆“已导出”和
   “已验证恢复”。在隔离 fixture 中再次读回原 PG/SQLite 后才让 fence 恢复原运行状态。

备份是受控单机维护窗口模型，不抵御外部管理员并发改库/改卷或主机故障；不是 HA/PITR。
原 Redis 实例不操作，本轮没有证明 Redis 状态可随 PG/SQLite 回滚。

### 9.2 实测中纠正的假设

- Windows PowerShell 模块路径与 owner 写入权限不能假设可用；改为只设置/验证 DACL，
  不改目录所有者，失败不降级成普通目录。失败目录当时未写入秘密。
- 只读打开 WAL 数据库仍可能需要构建 -shm。改为在已关闭写入者的前提下，把源 DB/WAL
  复制到 helper 内临时目录再调用原 SQLite API；前后检验源摘要。不使用会忽略 WAL 的
  immutable=1，也不给原卷写权限。新增崩溃遗留 WAL 中第二条设备记录的真实保留门禁。
- PG 官方初始化阶段的 Unix socket 可先于最终 TCP server 就绪，改为检查 127.0.0.1 TCP。
- 旧迁移 DROP COLUMN 留下的物理列序号在逻辑恢复时会重排；删除重复的局部 schema 摘要。
  PostgreSQL 对等价 CHECK 的 array cast/AND 表达式也会规范化。由独立 schema 导出经过
  同版本数据库解析后，与恢复全库的完整 schema 比较；只去掉随机 psql restrict 包装令牌，
  不正则忽略约束或弱化为“表名相同”。此前不匹配/失败均保留，不冒充成功样本。

### 9.3 证据与下一步

Service ignored `build/c4-data-1789460841565-b57931/result.json`：8 项门禁通过，PG 46 张表/
38 个实际迁移、3 个实际 Broker schema SQLite；新库数据恢复与原 fence 重开通过。
使用 §8 已固定 Gateway/Broker 镜像 ID；测试的 Gateway/Controller/Broker 进程是生命周期
占位进程，不是实际 MQTT/Gateway 业务服务。Redis 也是生命周期占位，本轮不验证其内容。
22 个预先存在容器的 ID/镜像/Running/StartedAt 不变，fixture 容器与 3 个数据卷已按精确
ID/专有标签清理；私有备份/源证明/失败目录均保留。软件 86/86，Compose 矩阵通过。

本批完成的是“关闭写入者→真实多库备份→空库恢复证明→验证原数据未变→原入口复原”。
**不是**“升级后向既有卷/PG 回写旧数据”，不是动态 reservation 真 Broker replay，
不是镜像干净引擎导入，也不是现有栈实际升级或三端跨片通过。

下一包不再扩框架，整合原分片维护事务、静态服务替换与新实例准入/失败整组恢复：

1. 冻结静态/动态候选实际 spec、原配置/JWT/Secret 覆盖清单和原 reservation/凭据关系；
   依据明确替换回执准入新 ID，不放宽原 fence 的未知实例拒绝。
2. 在隔离真库做“候选写入后失败→关闭全部新写入者→PG＋各 SQLite＋配置共同恢复”，
   明确 Redis 投影/授权缓存失效策略，不能仅恢复某个 SQLite 再放行全部业务。
3. 新 Broker 的直接 MQTT/HTTP published ports 在准入前也不能泄漏流量入口；停 HTTP
   proxy 并不等于阻断所有 Broker 端口。完成 inventory/reservation replay/健康正文后才重开。
4. 通过后才备份并升级现有测试栈、真实双账号跨片/撤权，再补 C3；R4/Wio 仍暂缓。

不额外增加终端用户的 Python/SDK 操作；维护脚本及私有产物均不进入 Arduino 发布包。

## 10. C4-A 原实例多库回滚、中断续跑与候选端口准入（2026-09-15）

### 10.1 最小职责和执行顺序

保留原整栈 fence、分片替换事务和数据库备份工具；新增 `stack-data-restore` 数据端口，
不合并成 Controller 新 HTTP API。当前只支持**原容器身份和原配置仍有效**的停写恢复。
静态/动态候选替换以及新 ID 准入仍是下一包，不能放宽 fence 的未知 ID/配置漂移拒绝。

执行顺序固定为：记录全栈原运行状态 → 关闭入口及全部写入者 → 捕获备份和恢复证明 →
生成每个分片维护计划。这样分片计划中的 `original.running=false`，原事务回滚及重复
调用会保持 Broker 停止；仅全栈验证完成后由 fence 按最初运行状态重开。不新增第二套
延迟启动参数，也不在某个 SQLite 恢复后提前放行那个 Broker。

新增共用候选 spec 检查，动态替换的计划和重放均拒绝提前 published ports（包括 127.0.0.1）、
PublishAllPorts、privileged、host/default/bridge/container/service 网络捷径；ExposedPorts
声明可以保留。**无宿主发布端口不代表完整隔离：**同命名 Docker 网络的其他进程仍可能
访问候选。静态候选及实际网络/直接入口必须另行完成整组准入，不能宣称已解决。

### 10.2 数据恢复、缓存和失败边界

1. 必须有 journal 绑定的 backup-set 与独立 restore-verified 回执，完整 Broker ID 集合、
   planDigest、文件 SHA 及解析过的完整 PG schema 证明均匹配。每个写入步骤重新检查
   fence 和备份；已开始重开入口的计划不得再回写旧数据。
2. PG16 在单事务内检查其他客户端/迁移锁/支持的 schema，再恢复该数据库的 public schema。
   不执行 CREATE DATABASE，不接受额外用户 schema。回写后比较全部行/序列和完整 schema，
   不是只看表名或条数。维护窗口不是外部管理员并发写保护、HA 或 PITR。
3. SQLite 在原镜像内执行，目标只为原 Broker 精确命名数据卷中的 DB 文件。覆盖前把失败的
   DB/WAL/SHM 留在同卷按 operationId 命名的隔离目录，绑定文件摘要；重试复用并检查该目录。
   已校验备份先复制至同卷再替换，随后 integrity/内容摘要及全组读回确认；不会删整个卷。
4. 每次尝试先记录 `data-rollback-started`；PG/任一 SQLite 完成回执丢失时不重开写入者。
   全库读回与配置/缓存检查通过后才写 `data-rollback-complete`。以**最新一次**开始/完成
   的顺序判断，旧完成标记不能遮盖新的恢复失败。当前注入的是响应/回执边界异常，不是
   Docker 主机断电测试；不承诺宿主故障下的跨存储原子性。
5. 按原 Gateway 实际 REDIS_DB/路由 prefix，只清理路由投影及路由版本；保留 rate budgets、
   权益事件及其他 namespace。不使用 FLUSHDB/FLUSHALL。Gateway 或任何 Broker 开启 MQTT
   Session persistence，或 Redis 端点/范围不支持时，在数据写入前拒绝本路径。
   这不是所有 Redis 业务数据/授权缓存的完整恢复证明。
6. 本批配置策略是**显式源文件必须保持原摘要**，变化则拒绝；没有自动覆盖配置、JWT 或
   其他 Secret。完整秘密覆盖清单、配置变更恢复和实际候选运行后的缓存兼容仍需验证。

### 10.3 本批证据、发现和剩余工作

- Controller/预检/维护相关软件 **102/102**，Compose 矩阵通过。新增回归含持久 MQTT 拒绝、
  宿主端口拒绝、停写后计划回滚保持停止和最新恢复检查点判断。
- 最终独立 Docker 证据：Service ignored
  `build/c4-data-1789462584217-10d332/result.json`，**12 项门禁**，46 PG 表/38 个实际迁移/
  3 个实际 Broker SQLite schema/真实 Redis。模拟候选新增表和修改记录，分别在实际 PG
  回写后、首个 SQLite 回写后注入完成回执异常；全组保持停止，续跑成功且原来停止的
  Broker 仍停止。两个路由 key 被清理，限流及其他 key 值保留。
- Gateway/Controller/Broker 的运行进程仍是生命周期占位。候选数据写入由 fixture helper
  执行，不是实际候选业务流量、动态 reservation 真 Broker replay 或四端 MQTT 联调。
- 最终样本的 **21 个**既有容器 ID/镜像/Running/StartedAt 全部不变，fixture 容器和 3 个
  数据卷精确清理，私有备份保留 D 盘；失败 DB 隔离副本位于测试卷，随测试卷清理，不冒称
  这些临时副本仍存在。实际操作器必须保留原数据卷/隔离副本直到明确处理。
- 首次 PG 回写在完整 schema 对照中确实失败：新建 public schema 缺少 PG 默认注释。
  修正初始化语义，未忽略 schema 差异；失败目录 `build/c4-data-1789461901204-acfa7f/`
  保留。中间 `build/c4-data-1789462213080-135a28/` 虽有 restored journal，但没有最终
  result，故不计完整通过；测试现已保存前态清单，并在清理/最终核对失败时仍输出失败结果。

下一包按依赖收口，不再新开维护框架分支：

1. 冻结静态/动态候选的精确 spec、实际配置/JWT/Secret 覆盖及保留关系；依据真实替换回执
   准入新 ID/镜像/卷/原 reservation/派生凭据，直接端口与同 Docker 网络均不可提前漏入。
2. 复用分片事务与本批数据端口，在实际候选服务启动后执行整组失败恢复、原身份/配置恢复
   和新版本 inventory/replay/健康正文验收；不能拿生命周期占位门禁替代。
3. 完成镜像干净引擎导入验证后，才备份并升级当前本机测试栈，执行双账号跨片/撤权；
   随后 C3 整套环境恢复。R4/Wio 实物、干净 SDK CI 和生产 profile 原缺项仍保留。

现有栈未实际备份/部署；C4-A 不关闭。本批未改 App/MCU 源码、未刷板/清授权/暂存/提交/推送。
