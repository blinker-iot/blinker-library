# UNO R4：Arduino IDE维护profile安装与选择

## 1. 最小交付决定（§11.54）

沿用同一Blinker库和`R4TasksQuickStart`，在Sketchbook中侧装一个独立维护板型，
不编辑Arduino15的SDK、不新增R4专用库ZIP、不让用户在Sketch手工拼SDK／heap配置。
这是Windows本地维护profile交付，不是Boards Manager公开发行或任意Sketch资源认证。

| IDE选择 | 行为 |
| --- | --- |
| 官方`Arduino UNO R4 WiFi` | 原SDK与同一库；持久任务默认0槽，普通通信／Sketch Timer不变 |
| `Blinker UNO R4 WiFi - Tasks4 Plain (Core 1.5.3)` | 只对已验固定图使用原四槽Plain／Ed25519及保护配方；服务仍需精确设备ID准入 |

维护板FQBN为`blinker:renesas_uno:r4tasks`，路径为
`<Sketchbook>/hardware/blinker/renesas_uno`。保留`renesas_uno`架构，不影响同一库的兼容选择。
它与官方板可能具有相同USB标识，用户须明确选板，不能依靠串口自动识别替代profile选择。
该方式基于[Arduino官方平台规范](https://docs.arduino.cc/arduino-cli/platform-specification/)的本地平台及`platform.local.txt`机制。

## 2. 配方与边界

- 自动复制原Core1.5.3并应用既有六文件Modem／Serial overlay；显式引用原GCC7-2017q4及bossac1.9.1-arduino5，不下载或升级。
- 全部C／C++／assembly消费者从板型配方取得同一固定配置；沿用原NO_USB、MQTT361、RPC1000及四槽写策略，不需额外CLI参数。
- 原heap adapter只放入维护Core的一份TU，由原`--whole-archive`配方带入；原四份Core合同头另加一份纯heap算术头，不复制整个库到Core，不增加owner。
- 维护profile不携带第二份Blinker，不覆盖用户库。安装前核对同一库完整src／公开配置／验签材料及固定示例、PubSubClient2.8和Crypto0.4.0；不匹配时停止。
- 公开服务配置来自与单库相同的release profile，而不是强塞带C++验签类型的头到C／assembly／Core。Key／SSID／密码不属于profile安装输入。
- 正常发布时生成纯预处理库身份头；维护Config按唯一已核库的绝对路径include并比较八个32位字，库源码／公开profile更新后拒绝旧板型。新版任务合同也拒绝缺绑定的旧维护板；C／C++／assembly无运行数据，不增加IDE编译时Python或全树hash步骤。
- 库仍默认0槽，只有选择维护板才由独立recipe开启固定4槽；换回官方板不需要改库或删宏。任意额外回调、BLE／LAN／TLS、故障自动恢复不随之开放。

这只是§11.52受限图的安装接线变化，不重开资源审计；Wio与Error8保持原边界。

## 3. 维护者安装一次

先通过既有单库工具构建匹配公开profile的**同一个**ZIP，并按正常方式安装到IDE的Sketchbook，
以及安装声明的原版本依赖。不要把源码根目录的未配置占位库误称为已配置服务版本；安装器遇到不匹配会拒绝，不会帮用户改写库。

```powershell
python tools/build_arduino_release.py --profile <public-profile.json> --allow-development-profile --stage-dir <new-library-stage> --output-dir <output>
# 按IDE“添加ZIP库”安装上述单库；不要额外安装第二份Blinker。
python tools/install_r4_arduino_profile.py --sketchbook <IDE-Sketchbook> --sdk <Core-1.5.3目录> --compiler <原GCC-7-2017q4的bin目录> --bossac <bossac-1.9.1-arduino5目录> --profile <同一public-profile.json>
```

安装／核验工具使用已有Python3.10+，不自动下载工具链或依赖。以上参数由维护者提供／核对；
后续IDE构建Sketch不需要Python、路径参数或SDK手工修改。
每台电脑本地安装一次，不直接跨电脑复制生成目录；原编译器／上传器路径由该机安装器核对后写入。
安装器只处理固定维护板目录。原SDK／库／依赖均只读；已有匹配安装只核验，不反复写入。
未知、被修改、含额外文件／链接或配置不符的目标不覆盖；失败暂存保留以供核对，不盲目删除。
若需要更新维护profile，应先用匹配版本工具核验并撤销原安装，再安装新版本，不提供强制覆盖开关。

重复同组参数加`--verify-only`可核验；加`--uninstall`只在完整核验后把自有目录移至
`<Sketchbook>/.blinker-profile-backups/<unique>/renesas_uno`，使IDE不再发现该板，保留可恢复副本，不递归删除用户内容。

### 更新顺序与错配恢复（§11.56）

1. 更新Git工具／单库ZIP／公开profile前，保留原匹配版本工具源码、公开profile及原依赖／SDK输入。先用原同组参数执行`--verify-only`，再`--uninstall`；看到旧维护板型已移出硬件目录后再更新。不用当前新工具代替原匹配工具撤销旧profile。
2. 安装新版本的同一个库及其匹配公开profile，用新版本工具重新侧装、核验，再重开IDE明确选择维护板。用户的个人Sketch与凭据不由安装器改写，仍需符合固定图合同。
3. 若已经先更新：停止构建，保留旧profile及报错。恢复原匹配工具、库、公开profile、依赖与所引用的SDK输入，再按步骤1撤销，然后按步骤2更新。不删manifest、不手改identity、不force覆盖／递归删目录；无法恢复匹配输入时先保留现场，不自动采用未知profile。

编译错误`R4 maintenance profile/library mismatch`或缺少`LibraryBuildIdentity.h`表示发布身份不匹配；新版库配旧板会报缺少匹配的library/profile binding。它们不是设备离线、DeviceKey失效或任务UNKNOWN，不应为此清授权或重放任务。

身份只绑定正常发布的`library.properties`、`src`及渲染后的公开配置，排除身份头自身；路径、个人凭据及examples不参与摘要。手工改源码但不重新发布身份、换实际依赖／工具二进制均不由这项编译保护自动验证，仍需显式核验。备份只是保留安装文件，不自动回滚设备任务、账号或运行状态；不要把兼容性摘要当防篡改签名或任意Sketch准入。

## 4. 用户使用

1. 安装后重新打开IDE，从“工具→开发板”明确选择维护板；若需普通WiFi程序，仍选择官方板。
2. 从同一Blinker库打开`R4TasksQuickStart`，另存到自己的Sketch文件夹，填写其`UserCredentials.h`的三个字符串。
3. 使用原`enableTasks(power, duty)`、`begin(...)`和`run()`，保持固定三属性／短回调；按普通“验证／上传”操作，无额外adapter文件或编译参数。
4. 通过匹配Service确认真实容量4及授权。`enableTasks`不初始化owner、不建任务、不Start；设备ID准入和原App管理链仍独立进行。

未选择维护板时，该任务参考示例明确编译拒绝；普通WiFi示例不受影响。
本机开发公开公钥／`.invalid`测试入口不能当作线上生产配置，也不能用占位编译固件要求设备上线。

## 5. 历史安装验证（§11.54）

先做安装fixture：独立目标、无覆盖、配置匹配、幂等、篡改／额外文件／链接拒绝、可恢复撤销及带空格路径。
再在独立Sketchbook正常安装单库，自动安装维护profile，核真实板发现和工具属性解析；只用板型FQBN编译未改的现有示例，
不额外传`--build-property`、不放Sketch heap TU。核实际全消费者配置、唯一owner／wrapper、原_sbrk链和静态边界。
官方板普通0槽做有限差异，不重跑七目标矩阵、BLE／断电或任务Start。原SDK和库逐文件保持，IDE GUI点击和硬件运行不据CLI构建冒称通过。

§11.54当时的Windows有限验证已完成（表中442文件和产物数值仅对应当批版本，不是§11.57新护栏产物）：

| 范围 | 结果／限制 |
| --- | --- |
| 安装fixture | 11/11，无skip；含真实Windows junction、未知／改动目标拒绝、幂等及可恢复撤销 |
| 共用构建帮助函数回归 | 原生成器10/10；不重复旧资源／硬件矩阵 |
| 实际正常单库安装 | 一个442文件ZIP经标准CLI正常安装；同一库与公开配置匹配，无平台内第二份库 |
| 带空格Sketchbook选板 | 独立FQBN可发现，Core1.5.3、原GCC及bossac绝对路径实际展开正确 |
| 官方板对照 | 仍选择原SDK，C／C++／assembly／link额外参数为空；安装库头文件实际编译确认容量0。不是普通固件全矩阵重跑 |
| 维护板完整构建 | 仅传板型、输出路径及普通构建参数，无`--build-property`；Sketch仅原`.ino`＋凭据头，完整链接通过 |
| 静态接线 | 132TU统一recipe配置，唯一owner／heap wrapper及实际原_sbrk链；程序180912 B、静态RAM17248 B、heap跨度9088 B／栈预留6144 B，与前图一致 |
| 实际安全撤销 | 唯一测试profile完整核验后移至同Sketchbook备份；1063文件逐字节保全，真实板发现不再显示该FQBN，无递归删除 |

§11.54的实际构建使用公开CI配置及占位凭据，不用静态结果自动给设备准入；对应新运行短差异见下节，IDE GUI点击仍未测试。
安装及撤销仅发生在私有隔离Sketchbook，未改本机日常IDE的Sketchbook；无关修改终核取任务方案§11.54。
安装、编译、运行和正式服务准入四者不混同；如后续使用此新接线烧板，仅补一次必要启动／查询短差异，不重开资源或旧长循环。

## 6. 维护板型运行证据（§11.55／§11.57）

匹配当前本机服务的同一库／原参考示例，经正常维护板recipe完整编译及postverify，已在COM9通过启动、真实App空表Catalog及一次原power控制。实际容量4、revision0、无任务定义；唯一控制有匹配requestId／sequence的真实ACK及新鲜State，不保存或Start任务，不据此扩大固定图支持边界。

使用独立临时云身份和空scope，不复用旧scope；正常在线退休及移除核实后，原十二键／授权精确双读恢复，再恢复此前批准的已验普通WiFi BIN（不是新鲜主程序备份）。原SDK、日常IDE目录、普通默认0槽不变；Wio暂停、Error8独立待定位，详细恢复和日志边界见[任务方案§11.55](DEVICE_V2_TASK_AND_SKETCH_TIMER_EXECUTION_PLAN.md#1155-维护板型启动查询及原控制短差异)。本机成功不等于公开Boards Manager分发、任意Sketch或线上服务准入。

新增发布身份护栏后的实际产物已在[任务方案§11.57](DEVICE_V2_TASK_AND_SKETCH_TIMER_EXECUTION_PLAN.md#1157-构建缓存清理及新护栏候选的有限运行差异)独立补齐：同一正常单库、原SDK／维护板型完整编译及postverify，132TU实际身份依赖通过；COM9启动、真实App四槽空表和原power ACK／新鲜State通过，原固件／十二键及临时配置完成恢复。它不借用§11.55旧BIN冒充新产物证据；IDE GUI／公开板管理器分发仍未验，不据此扩大任意Sketch支持范围。

现行收口及剩余范围见[§11.58](DEVICE_V2_TASK_AND_SKETCH_TIMER_EXECUTION_PLAN.md#1158-有限支持收口后的剩余范围与文档对齐)。本安装／运行闭环已关闭；上面的命令是维护入口，不是每批都要重复执行的清单。
