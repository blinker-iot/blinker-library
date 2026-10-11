# Blinker 核心版：Git 拉取、编译与独立短验收

更新：2026-10-11。三端通过各自当前分支交付必要源码、回归和公开说明，不分发或保留本轮本地交接包、APK、ZIP及源码快照。接收同事从 Git 构建自己的候选，记录实际提交与配置；发送方已有编译、真机结果不能代替本人签收。当前是内部候选，**独立接收待完成，不是生产发布**。

验收复用现有执行链：真实 App 获取设备能力与新鲜状态，完成一次属性写入、一次按钮动作，再检查离页重入及普通设备重启。任务、自动化、LAN/P1及H1只按已准入范围选择，不因取得源码默认开放全部功能。

## 1. 拉取同一组源码

在同一父目录拉取三仓与五个插件。App 的五项 `file:../...` 依赖需要这个布局；两个 LocalAccess 回归还读取相邻设备仓的公开 golden fixture。不要只复制 App 的 `src`、旧 `node_modules` 或生成的 Capacitor 路径。

```powershell
git clone --branch device_v2 https://github.com/blinker-iot/blinker-library.git
git clone --branch i3w-dev-v2 https://github.com/blinker-iot/blinker-app.git
git clone --branch dev_v2 https://github.com/ailyProject/aily-blinker-services.git
git clone https://github.com/coloz/capacitor-android-shortcuts.git
git clone https://github.com/i3water/capacitor-mdns.git
git clone https://github.com/i3water/capacitor-ntfy.git
git clone https://github.com/coloz/capacitor-wechat.git
git clone https://github.com/i3water/capacitor-wifiprov.git
```

五插件已有必要原生实现，本批不另扩展插件功能。新克隆目录固定如下已核对的提交，避免插件 `main` 后续移动影响配对：

```powershell
git -C capacitor-android-shortcuts switch --detach 2c187e447542843dba1264bd95c297686ade6c49
git -C capacitor-mdns switch --detach 00f994f2896327781c64f93851932f1acbddcb8e
git -C capacitor-ntfy switch --detach 83b57ee2efdd829db025d37fb898d349e6cd6a35
git -C capacitor-wechat switch --detach ba0bfc5677b2a0dfe11d231c43b52e07af0bf487
git -C capacitor-wifiprov switch --detach a21e87642a853242677f7dccd8847e9785837ab0
```

这些命令只用于新克隆目录，不覆盖接收者已有修改。三端在开始验收时分别记录 `git rev-parse HEAD` 与 `git status --short`，整轮固定提交，不中途随分支升级。本批实际提交由发送方推送结果给出；同名库版本 `0.1.0`、schema或镜像tag不能代替提交和实际制品摘要。

## 2. 设备库：从 Git 编译与配置

Git 默认 `OfficialConfig.h` 是占位控制地址，源码可编译但**不自带可上线服务**。服务地址和验签公钥由维护 profile 注入，终端用户只填自己当前 DeviceKey、SSID、密码及组件，不修改内部 composition。

维护者在设备仓根先检查单库边界：

```powershell
python -B tools/build_arduino_release.py --check-only
python -B tests/architecture/check_device_v2.py
python -B tests/release/check_release_profile.py
python -B -m unittest discover -s tests/release -p 'test_arduino*.py'
```

`tests/release/development_profile.json` 是公开**编译 fixture**，地址 `device-v2-ci.invalid`、测试公钥不能当实际服务或生产信任根。编译维护者可以用它生成标准库目录：

```powershell
python -B tools/build_arduino_release.py --stage-dir build/git-acceptance/BlinkerDeviceV2 --profile tests/release/development_profile.json --allow-development-profile
```

真机联网前，把该 JSON 复制到仓外并由环境维护者填写实际控制 host/port、明确 plain/TLS 选择及匹配服务的公开验签 ring；**不得填签名私钥、WiFi密码或 DeviceKey**。用新的空 stage 目录和该 profile 再运行同一发布器。它只在 stage 生成三个配置头，不覆写仓库默认源码，不改 CI fixture。设备端普通 session HTTP/MQTT允许明确明文，challenge-HMAC身份校验仍保留；App HTTPS/WSS与 Direct 鉴权不因此取消。

Arduino IDE 将生成的单个 `BlinkerDeviceV2` 库目录放入自己的 sketchbook `libraries`；根部必须有 `library.properties`、`src`、`examples`，不得双层嵌套。也可由接收者给发布器增加 `--output-dir <新的输出目录>` 生成**一个** Arduino ZIP 后安装，不按 MCU/模式拆包。这些是接收者自己构建的输出，不是本机交接制品。遇旧同名库先保留并隔离，核对编译实际选中的路径。

| 公开例程 | 接入与范围 |
| --- | --- |
| `WiFiQuickStart` / `Blinker.h` | App 创建普通设备并复制其长期 DeviceKey，填入自己的 Key/网络。普通 WiFi 是三端短链首选 |
| `BleQuickStart` / `BlinkerBLE.h` | App 正常发现、认领、安全连接；地址/MAC不是逻辑身份，附近不等于认证连接 |
| `WiFiBleQuickStart` / `BlinkerWiFiBLE.h` | ESP32 的 BLE WiFiProv 配网及同一身份的 WiFi+BLE；不套用于 R4/Wio |
| `CloudSmokeTest` | 普通 WiFi 诊断/采样；串口按需一次 `d`，不连续刷输出、不打印密钥 |

已记录的编译基线：普通 ESP32 WiFi/Cloud 为 Core 3.3.6；XIAO S3 BLE/WiFi+BLE 为3.3.11；UNO R4为Renesas1.5.3；Wio WiFi为Seeed SAMD1.8.5。外部依赖为PubSubClient2.8、ArduinoBLE2.0.2、Crypto0.4.0；Wio另需Seeed FS2.1.3、mbedTLS3.0.2、rpcUnified2.1.4、rpcWiFi1.1.0。按所选模式安装；ESP32用内部ESP-IDF NimBLE，不增加第三方NimBLE库。

N3和整数step修复已在源码中；最新BLE实时例程有S3/R4隔离编译证据，其他五目标保留旧编译和输入等价依据，不宣称本批重跑七目标。R4新版实时例程实物未测，Wio BLE仍preview。具体矩阵取[§10.6～§10.7](DEVICE_V2_USABILITY_FOLLOWUP_EXECUTION_PLAN.md#106-n3与整数step单库候选冻结及安装编译2026-10-11)。源码安装、链接通过、实际联网和独立签收分别记录。

## 3. App：从 Git 构建

工具链基线：Node22.22.3、npm10.9.8、pnpm11.19.0、JDK21、Android SDK36。保留App的 `pnpm-lock.yaml`、workspace和五插件各自npm锁。Gradle/Maven及SDK是外部工具，不声称完整离线包。

先在**五个插件目录分别**执行以下命令，任一步失败先处理原错误，不继续用旧dist：

```powershell
npm ci --ignore-scripts
npm run build
```

再在 `blinker-app` 根执行：

```powershell
pnpm install --frozen-lockfile --ignore-scripts
npm run device-template:build
node node_modules/typescript/bin/tsc --noEmit -p tsconfig.app.json
node node_modules/@angular/cli/bin/ng.js build --configuration local,local-device
node node_modules/@capacitor/cli/bin/capacitor sync android
```

配置好JDK/SDK后在 `android` 目录执行 `./gradlew.bat :app:assembleDebug`。不在App运行 `npm install/ci` 或提交sync生成的pnpm路径。构建后记录Git提交、明确配置、APK摘要及签名；新电脑的debug签名通常不同，不能承诺更新已有手机，也不能靠卸载清数据解决签名冲突。

普通 `local,local-device` 保持HTTPS WebView origin，LAN/P1默认关闭。本机API配置为 `https://127.0.0.1:17000`，用户MQTT下发须匹配可信WSS（原测试18885端口）。同事网络及证书不自动继承本机配置。既有 `BLINKER_DEBUG_NETWORK_CONFIG_DIR` 仅用于显式、已审阅的debug公开CA资源，详见[App说明](../../blinker-app/docs/APP_DEVICE_V2_SOURCE_HANDOFF.md)；CA私钥和更新签名库不进Git，不放开全域HTTP/证书校验。以前实测APK另有LAN-only overlay，不称普通Git重建得到相同字节或默认开关。

## 4. Service：从 Git 构建与运行前置

在 `aily-blinker-services` 根使用匹配Node版本安装，不能复制其他机器/Node ABI的SQLite二进制：

```powershell
npm ci
npm run build
npm run typecheck
npm test --workspace @aily-iot/contracts
npm run test:display-metadata --workspace aily-iot-gateway
npm run test:state-history --workspace aily-iot-gateway
npm run test:state-history --workspace aily-iot-broker
```

Node20是现有Docker基线，本机源码验证用22.22.3；自行构建镜像须按实际Node/安装输入记录。公开 `.env.example`、Compose与Controller/共享协议构建闭包在Git中；真实`.env`、业务库、Auth/Kong、运行密钥、TLS代理仍由环境维护者配置，**编译成功不是完整账号环境已部署**。

优先复用已有匹配本机服务，不为同事验收再次升级整组环境。需独立部署时先备份，Gateway应用现有迁移至051，Broker H1表由自身初始化；沿[服务部署说明](../../aily-blinker-services/docs/deployment.md)核对实际端点、内部用途密钥与固件公钥。任务/自动化及LAN仍按封闭资格启用，不能删准入/鉴权绕过缺配置。H1带历史租户迁移当前明确拒绝，不宣称已有历史搬迁能力。

以前本机仅参与Broker shard1部署并验过H1，不外推其他Shard已部署；源码从同一Git提交构建并运行是接收者的新结果，不套用旧镜像ID。账号正常登录取得，DeviceKey从自己设备页面取得，私有配置不通过Git或签收表传递。

## 5. 实际安装前与停止条件

电脑、手机、设备须网络互通；设备使用支持的2.4 GHz WiFi。维护者确认profile与实际控制地址、MQTT下发、公钥/TLS信任一致；相同SSID不证明互通。缺有效账号、服务信任、匹配grant或资格时，联网项填“未测”，不伪造Token/State、直写业务库或清授权代替。

使用已有手机前保留账号、请求意图/日志、缓存与授权，核对AppID、更新签名、origin和数据合同，满足条件才 `adb -s <手机序列> install -r <自己构建的APK>`。本机USB HTTPS/WSS需要映射时，只在确认服务可达且受信后创建：

```powershell
adb -s <手机序列> reverse tcp:17000 tcp:17000
adb -s <手机序列> reverse tcp:18885 tcp:18885
```

刷写前确认具体板、端口、原程序和恢复范围；需恢复原板时先备份并核验。没有授权不擦Flash、清访问凭据或刷桥接固件。新空板与需恢复原板分别记录。

## 6. 控制、恢复与附加检查

设备组件长期存活，`begin()`显式传组件，保留`onWrite/onInvoke/onSample`及 `loop()` 的 `Blinker.run()`；不在loop无节制上报。`begin()`成功只表示同步初始化，必须继续等实际认证通信。普通例程的power/计数是内存状态，不是物理继电器，也不保证断电持久。

| 检查 | 操作与通过条件 |
| --- | --- |
| 首次读取 | 正常App打开设备页，逻辑ID正确，读取本次fresh Manifest/State，不只显示缓存；账户MQTT ready不等于设备在线 |
| 属性写入 | power改成相反值再改回；原Command与真实ACK/完成结果关联，设备回调报告State与App显示一致，ACK不能代替State |
| 按钮动作 | 读buttonPresses，只点击一次button，取得真实结果和计数+1；未知结果先查原请求/状态，不盲换ID重放Action |
| 退出重入 | 再次查询fresh State，不重复按钮；保留原请求结果和日志 |
| 普通重启 | 保持原账号、ID、Key版本、网络和授权，重启后正常接入并查fresh能力/状态，再做一次power来回控制 |
| 恢复 | 使用原板时按备份范围恢复原固件/授权并校验，再确认原程序正常读取/上线；只写应用不等于所有Flash/桥接NVS恢复 |

Cloud用既有Command ledger核对；BLE用认证Direct响应/State，不误称Broker已持久记录。普通重启将power/计数重新初始化为默认值是示例正常行为；检查身份、授权、再次接入，不要求内存业务值跨断电保留。只读数值不强行写入，整数step试点仅在已选可写Manifest上补测。

附加范围由接收者预先选定：

- 实时：正确来源、实际间隔、新鲜度与有界曲线；离页/后台不续租，有限尾包后停止，返回恢复。普通源码不默认开LAN。
- H1：仅Broker采纳的标量State历史，真实变化、不变值去重、重连baseline、范围分页和名称；时间是服务采纳时间，不保证onSample/Event或未上Cloud的Direct状态归档。
- 分享：正常邀请/领取和ACL检查；H1撤权通过条件是下一次真实查询拒绝并清空页面，不声称立即push清屏或分享控制已测。
- 任务/自动化：普通profile不默认带task_profile；自动化仍限定已准入同owner/tenant/shard的两WiFi设备及一个bool/uint32固定动作。不扩槽、多动作、浮点、离线存储或Bulk。

结束只撤除本次自己新建且不再需要的ADB映射，不全局清端口。不回滚业务库、删除审计或用不兼容旧APK隐藏结果。失败与unknown如实保留，私有原始日志留在接收者本机，只回传脱敏信息。

## 7. 独立签收记录模板

复制本节到接收者自己的记录，不在公共Git填写密码、Token、原始Key、Flash、业务库或私有日志。没有本人实录时继续“待独立接收”；未测平台和功能不勾成通过。

| 项目 | 接收者填写 |
| --- | --- |
| 接收人、日期 | 待填写 |
| 三端Git提交、五插件提交 | 待填写 |
| 实际工具链、板型/FQBN、模式与依赖 | 待填写 |
| profile摘要/公开端点、App配置与签名、服务schema | 待填写；不写秘密 |
| 本人源码构建与安装结果 | 通过/失败/未测及脱敏依据 |
| fresh Manifest/State | 通过/失败/未测 |
| power真实ACK/State | 通过/失败/未测及请求定位 |
| 一次button/counter、unknown处理 | 通过/失败/未测 |
| 离页重入、普通重启及原程序恢复 | 通过/失败/未测与实际恢复范围 |
| 实时/H1/分享/任务/自动化选测 | 范围、通过/失败/未测；不自动全选 |
| 遗留问题和本人结论 | 待填写；发送方不代签 |
