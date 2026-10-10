# 本机任务：Arduino 封闭试点交接

更新：2026-09-24。本文件整理已实现入口及最小源码交接，不新增协议或扩大正式发布范围。实物与独立重建证据以[执行器设计](DEVICE_V2_TASK_EXECUTOR_DESIGN.md)§10.43～10.44、后续顺序以[主线](DEVICE_V2_NEXT_EXECUTION_PLAN.md)§6为准。

## 1. 适用范围

- 当前实测：XIAO ESP32S3、stock Arduino ESP32 3.3.11、普通手动 WiFi、原账号 owner、4 槽/单个布尔 `power` 目标。
- 维护者交付一个标准 Arduino 库 ZIP。任务编译默认关闭；普通用户不需要 Python、不接内部 composition、不填写存储/签名参数。
- 必须使用匹配本机服务地址/签名公钥、显式任务 profile 的 development 候选。仓库默认源码、普通 ZIP、安装成功都不等于任务已开放。
- 任务管理仍要求服务端的试点设备白名单和 owner 权限；设备固件开关不能绕过它们。共享用户的普通控制权限不等于持久任务管理权限。
- BLE、WiFi+BLE、网关、UNO R4/Wio 任务未准入；普通通信支持状态不受此表改变。8/10 槽只是资源测量，不能以此宣称实物可用。

## 2. 用户 Sketch

安装维护者提供的候选库，填入自己设备的 DeviceKey、SSID 和密码：

```cpp
#define BLINKER_ENABLE_TASKS 1
#include <Blinker.h>

BLINKER_PROPERTY(power, bool, blinker::readWrite());

void onPower(bool value) {
    // 在此设置实际输出；report 报告状态，不自动驱动 GPIO。
    power.report(value);
}

void setup() {
    power.onWrite(onPower);
    if (!Blinker.enableTasks(power)) return;
    if (!Blinker.begin("DEVICE_KEY", "WIFI_SSID", "WIFI_PASSWORD", power)) return;
}

void loop() {
    Blinker.run();
}
```

`enableTasks`在第一次`begin`前调用一次，选择的属性也必须传入`begin`并提供写回调；Property使用全局/长生命周期对象。它只配置固件调度白名单并复用授时，不创建任务、不初始化 owner 授权。普通可写属性不会自动全部变成任务目标，Action也不是此接口的目标。

## 3. App 操作与持久化

1. 设备正常联网后，在原任务页面明确启用本机任务；首次进入只读，不自动初始化。
2. 根据真实目标目录创建任务。创建倒计时不等于开始，须另行确认 Start；启用已有定义也不自动恢复倒计时。
3. 任务及执行进度由设备持久化，退出页面/停止 App 不取消已确认的任务。执行不依赖 App 保持设备页打开，不增加 MQTT 连接。
4. 重进页面读取认证结果。安装确认、执行完成、实际 GPIO/传感器输出是不同层次；示例的回调计数只能证明软件回调，不证明外部硬件动作。
5. 未知结果保留原请求；不能反复新建同一意图猜成功。正常 owner 删除设备走已有认证退休流程，不通过擦 Flash 代替远端删除。

倒计时断电/重启不自动续跑；日历任务需有效授时，失时不能承诺离线准时，也不补放失时期间错过的动作。首次初始化/同步结果未确认后又刷新 Key 的部分窗口仍明确拒绝恢复；不悄悄丢弃原意图、旧任务或降低鉴权。

## 4. 维护者责任

- 原单库打包 profile 设置 `task_profile: "esp32-wifi-t2-lab-v1"`，仅 development 可用；它固定4槽、60秒10写、30秒 Start 冷却、100ms提交预算。不要让初学者自行调整这些值。
- 配套原 schema043、任务签发/认证/原 MQTT carrier 及专用白名单；App 使用原任务页面。服务无须为此公开入口新建模型、连接或 API。
- 分发时固定库源码快照、依赖、SDK、profile 与 ZIP 摘要。不得把测试公钥、私有本机地址对应的候选改名当作生产版本。
- 一份 ZIP 可同时编译未启用任务的普通示例和 opt-in 示例；校验实际安装文件/编译来源/链接，禁止通过工作区 `--library` 冒充安装验证。
- 第四十五批已通过最小设备源码独立重建/安装编译；SDK/CLI外置，完整三端干净构建及生产材料仍缺。已分享 operator 真机拒绝缺受支持同片环境；写额度真机、最大目录、长期磨损及其他 MCU 不外推为通过。

本批退出后不保留可用测试设备：按正常 owner 删除并验证任务退休，恢复 COM3 原固件/授权与本机白名单，撤去临时测试代理。删除响应的 `brokerCleanupPending`、`localAccess` 结果各自记录，不能以任务退休成功替代所有清理证明。

## 5. 维护者源码交接与重建

普通用户不执行本节。维护者使用`tools/build_arduino_handoff.py`导出最小源码目录：原库白名单、原发布器/profile模块、安装门禁、公开任务编译例程、交接自测及PubSubClient 2.8源码/许可证。另含公开profile、逐文件SHA-256和依赖要求，不复制`.git`、现场`build`、用户凭据或全部测试/SDK。

```powershell
python tools/build_arduino_handoff.py export `
  --profile <已实测development-profile.json> `
  --reference-zip <已实测BlinkerDeviceV2-0.1.0.zip> `
  --dependency-dir <PubSubClient-2.8源码目录> `
  --out <不存在的维护者交接目录>
```

导出前会用当前原始源码/profile重建并与参考ZIP逐字节比较，发生源码漂移就拒绝，不能只把旧ZIP摘要贴到新源码上。接收方在新目录执行交接物里的脚本：

```powershell
python <交接目录>/source/tools/build_arduino_handoff.py verify --bundle <交接目录>
python <交接目录>/source/tools/build_arduino_handoff.py rebuild `
  --bundle <交接目录> --work-dir <交接目录外的新重建目录>
python <交接目录>/source/tests/arduino/verify_installed_release.py `
  --arduino-cli <arduino-cli可执行文件> `
  --zip-path <新重建目录>/artifacts/BlinkerDeviceV2-0.1.0.zip `
  --work-dir <新的隔离安装目录> --data-dir <已安装ESP32-3.3.11的Arduino数据目录> `
  --dependency-dir <交接目录>/dependencies --target esp32-wifi-tasks --jobs 4
```

任务目标必须显式选择；不进入普通七目标默认矩阵，不新增正式例程/第二个用户库。门禁验证安装字节、编译来源、所需任务/授时符号及BLE/LAN/网关等不应带入的图。默认示例可另选`--target esp32-wifi`验证任务关闭。

维护者要求Python 3.11+、Arduino CLI 1.5.1、stock ESP32 3.3.11及其编译器/工具；它们**没有包含在交接目录内**，离线编译须事先安装完整缓存。PubSubClient源码随清单提供，不依赖开发者原来的Arduino库目录。重建要求新输出目录、拒绝覆盖；缺失/改动/额外文件均拒绝校验，不自动清理用户目录。

SHA-256清单是完整性检查，不是发布者数字签名；仅运行可信来源的维护者脚本。源码交接物与标准用户ZIP职责不同，不应导入Arduino IDE。它不包含完整CI/主机测试、App APK或服务镜像，不能据此声称三端干净checkout、SDK全文件锁定或生产可复现构建已完成。

### 5.1 S2工具复用补充（2026-10-08）

原工具已可交接普通WiFi的development候选，不再强制task_profile；它仍拒绝production，旧task-pilot清单兼容。本文任务用法/准入不变，普通profile不会隐式开启任务。第六十三批普通WiFi同ZIP重建及完整支持边界见[候选清单](DEVICE_V2_CANDIDATE_SOURCE_AND_SUPPORT.md)；第45批冻结快照不修改。

## 6. 第四十五批交接结果

冻结快照：`D:/codes/local-maintenance/task-handoff-20260924`，433项文件清单；独立重建产物在`D:/codes/local-maintenance/task-rebuilt-20260924/artifacts/`。ZIP与上一批真实App/COM3验收候选完全一致；完整隔离安装/任务编译通过，Flash984,948 B、静态RAM53,936 B。交接9项自测在两目录通过、原打包回归19项通过；摘要集中在执行器§10.44。快照内说明为导出时版本，不为追加结果而修改已校验快照。

这允许维护者在匹配本机服务/白名单下交接owner封闭试点，不等于安装后自动开放所有设备。正式三端发布/SDK完整闭包/共享operator等限制仍见§1和§4；下一步进入A1最小设计，不重复本轮硬件测试。
