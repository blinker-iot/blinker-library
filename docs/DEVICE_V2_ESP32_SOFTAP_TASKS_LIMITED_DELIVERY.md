# ESP32 SoftAP 四槽组合：有限交付与逐设备开通

本文件承接[测试对照计划§30](DEVICE_V2_TEST_REPORT_RECONCILIATION_AND_CORE_PLAN.md#30-qa-3新批冷预检两类日历真实执行及受限支持收口2026-11-01)及§31。
功能正向证据已覆盖既有固定组合；本批仅补普通 Arduino 的明确安装入口，不把内部 BIN 当作公开发行包。

## 1. 两种安装合同

| 项目 | 普通公开 cloud-beta | 显式四槽有限交付 |
| --- | --- | --- |
| 入口 | 原 `build_arduino_release.py --profile profiles/cloud-beta.json` | `build_esp32_softap_tasks.py` |
| 图 | ESP32 手动 WiFi／TLS；0 槽 | XIAO ESP32S3／Core 3.3.11／SoftAP WiFiProv 后 Cloud HTTPS／MQTTS；4 槽 |
| 示例 | WiFiQuickStart、CloudSmokeTest | Esp32SoftApTasksQuickStart：power bool＋uptimeSeconds realtime 1000ms，任务目标仅 power |
| 授权 | 原 DeviceKey 会话 | 原 DeviceKey＋已安装 Admin 双根；P256 keyId1 |
| 准入 | 不开放任务 | 仍须维护者显式开同一个设备 ID，安装不等于开通 |

不需要 R4 的维护 Core、SDK 改动或额外 Arduino 板型。库级配置必须先于 API／配网默认选择加载，
所有实际配置消费者共享同一套库级配置；配置中性实现单元不必强制额外include。
不能靠 Sketch 定义三个宏就宣称四槽已接入。
源库 `TaskConfig.h` 默认仍 0 槽，原普通发布器、D4 手动 WiFi guard 及公开七目标不扩大。

## 2. 维护者打包与隔离验证

公开输入为 `profiles/cloud-beta.json` 的既有传输／CA和
`profiles/esp32-softap-tasks-limited.json` 的实际公钥投影：
P256／线协议 2／keyId1／SPKI SHA256
`02879e20a6ad6e021ed7e6c857f0a351db14b1568c467db519aac09920b6d061`。
此为§30已存事实，**不是本批新鲜运行签发核验**；私钥只留服务端，不进入库、ZIP或示例。

在库仓库根目录，选择尚不存在或为空的独立输出：

```powershell
python tools/build_esp32_softap_tasks.py --stage-dir build/limited-tasks/stage --output-dir build/limited-tasks/artifacts
python tests/arduino/verify_installed_release.py --zip-path build/limited-tasks/artifacts/BlinkerDeviceV2-0.1.0.zip --softap-task-manifest build/limited-tasks/artifacts/limited-profile.json --work-dir build/limited-tasks/installed --data-dir D:/arduino-esp32-3.3.11/data --dependency-dir PATH_TO_PINNED_LIBRARIES --arduino-cli PATH_TO_ARDUINO_CLI --target esp32-softap-tasks --jobs 1
```

第二步使用原 Core 3.3.11、原 `esp32:esp32:XIAO_ESP32S3:PartitionScheme=default_8MB`，
仅这个新目标，不自动跑七目标。依赖复用原 PubSubClient 2.8；不下载或升级 SDK。
打包复用原文件白名单、全 TU profile renderer、库内容身份及确定性归档；受限 board guard 对三份生成头同时生效。
原例程只发布 PoP 占位，打包拒绝已填 PoP 或额外私人头。

输出仍是一份 Blinker 库，附 `SHA256SUMS` 和公开 `limited-profile.json`，标识 ZIP、profile与库内容身份。
该 receipt 的 `deployable`／`runtimeSigningMatched`／`formalAdmission`／`installationCompileVerified`／`hardwareVerified`
均为 false：**它只证明静态构造，安装编译与运行结果分别记证据，不改旧 link-only 标签。**
隔离验证器在复制出的 Sketch 用明确非秘密的 `compile-only-not-a-device-pop` 替换占位，
安装目录仍逐字匹配 ZIP；**这个编译 fixture 和生成 BIN 只能作链接验证，不能刷作设备候选**。
不得把打包 receipt 传给 Service 的 runtime-candidate checker冒充实际运行证据。

## 3. 普通 Arduino 用户的最小操作

1. 只安装维护者提供、校验值匹配的一份受限 ZIP。移开旧 Blinker 库副本，不能普通／受限两份同名库并存；不要手工编辑 SDK。
2. 选择 XIAO ESP32S3、原 Core 3.3.11和已验 `default_8MB` 分区。其它 ESP32／Core 不由本包准入。
3. 打开 `Esp32SoftApTasksQuickStart`，先 Save As 到个人 Sketch，再在该副本的 `UserProvisioning.h` 填自己的非空 1–64 UTF-8 字节 PoP。
   占位、空值和过长值编译拒绝；PoP不由 DeviceKey／WiFi 密码派生，不共享或提交个人头。
4. 用原 App 的 SoftAP 流程配网、建立身份及 Admin。Sketch 不写 DeviceKey／SSID／WiFi 密码，不清根、不自动初始化 task scope。
5. 原 `Blinker.enableTasks(power)` 只注册目标并启用业务授时；随后原 `begin(power, uptimeSeconds)`／`run()`。
   定时查询／保存／启停／Start仍走原 App、鉴权、CAS与只读协调链。

已绑定设备不能当作空根重装；原 NVS、授权、TaskStore、scope和UNKNOWN保持。
更换 Manifest、根或身份必须走现有显式合同，不搬旧 scope、不回滚旧 NVS、不重放写入。
本包仅支持短回调 `power.report`；日期／星期需设备有效UTC及固定偏移，不承诺 DST、无限离线或 GPIO 物理精度。
纯 BLE／LAN 任务管理、BLE 配网＋任务组合、数值任务、任意 Sketch／平台不在范围内；Wio和Error8另列。

## 4. 逐设备有限开通，不改变默认

§31交付准备批不部署、不打开名单；后续[§32](DEVICE_V2_TEST_REPORT_RECONCILIATION_AND_CORE_PLAN.md#32-qa-3com11单id有限开通与新鲜目录read只读收口2026-11-01)
已仅用COM11既有运行候选完成一次明确授权的单ID开通／新鲜目录与Read，结束精确恢复配置、关闭三名单。
该结果不替新Arduino包装补造刷机证据、不自动保持开放。以后明确安排开通时，沿原 Service 接线完成以下步骤即可，无需新 API：

1. 只读核实际设备版本、同 owner／instance、DeviceKey＋Admin双根、现存 scope和任务；保存完整日志与UNKNOWN，不能清空当初始化。
2. 新鲜核实际 Gateway 的算法2、keyId1、所选公钥 SPKI、provisioned 三接线及匹配运行固件。
   公开静态 profile／ZIP checksum不替代这一步；新增打包例程也不能伪称已刷机验收。
3. 只把**同一逻辑设备 ID**加入 Gateway `DEVICE_V2_TASK_MANAGEMENT_DEVICES`、
   `DEVICE_V2_TASK_PROVISIONED_DEVICE_IDS`及 Broker `DEVICE_V2_TASK_ALLOWED_DEVICE_IDS`。
   原 `deploy/beta/compose.tasks.yaml`＋`compose.tasks-qa3.yaml`共享 `BETA_TASK_DEVICE_ID`；Ed25519名单空。
4. 在当前完整规范化 Compose上，复用 `deploy/beta/check-tasks-qa3-config.cjs --evidence ABSOLUTE_PUBLIC_RUNTIME_EVIDENCE.json --require-automation`
   做离线一致性检查（stdin为规范化 JSON），保留原地图／自动化／定位、镜像及只读私钥挂载。
   checker不认证证据来源、时间、板上 BIN或TaskStore；不能拿本包 receipt、夹具或旧 link-only记录改标签通关。
5. 实际开通后仅健康、context及一次新鲜 catalog／Read确认，不重跑已过倒计时、日期／星期、Start、配网或资源矩阵。
   缺 Admin、错公钥、身份／scope不匹配时继续拒绝，不降级 Key-only，不自动Initialize或重发。

撤名单只关闭云管理入口，**不会停止已启用的本地任务**；要停用需原正常API确认或有证据的退休流程。
撤回服务配置不回滚 TaskStore／revision／授权，完整请求日志和UNKNOWN不清理。

## 5. 本批验证与下一道门

静态回归、隔离安装／原 SDK 单目标完整链接的本批实测结果统一登记在主计划§31；失败亦保留。
安装编译通过只完成“普通 Arduino 可构建”门槛，不是新发行公告或扩大正式准入。
下一批如需交付某台设备，应按§4对那个ID显式核验；当前COM11既有功能结论沿§30复用，不重新制造样本。
本批不提交推送；不将源码打包等同远端CI或正式发行签收。
