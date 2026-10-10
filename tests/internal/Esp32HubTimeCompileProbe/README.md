# Hub 自身可选授时：仅安装编译/链接探针

此例程不在发布 ZIP/examples 中，不是用户 API，也不是刷写/配网门禁。
使用真实 `Esp32EdgeHubProduct`（TLS、受保护 NVS），不覆写存储能力、认证或 SDK。
本探针验证完整 `Product::begin(time)` 注入；已有 `CompileEsp32EdgeHubInternal` 保留其手工 Client/南向各类型 ABI 测量职责，不用本探针的整体资源数字替换它。

在 ESP32 Arduino 3.3.11 IDF NimBLE 目标，以同一 release ZIP 安装到独立 sketchbook，标准 Arduino CLI 编译：

- 原例程：默认不创建业务时钟。
- 对照例程：只在此 Sketch 首行增加 `#define BLINKER_TEST_ENABLE_TIME 1`，创建一个 `TimeSyncService`，经原 `Product::begin` 注入自身 Client。
- 南向对照：在开启自身授时的基础上增加 `#define BLINKER_TEST_CHILD_TIME 1`；两个可选 `GatewayTimeResponder` 借用同一只读业务时钟，经唯一 executor 接入，不创建第二套时钟或队列。

release profile 必须有真实 PEM CA/TLS 配置，否则产品静态策略可能拒绝初始化，不能用被裁剪掉的代码作资源对照。
保留标准 Arduino 依赖发现，根入口 `BlinkerV2Advanced.h` 用于识别安装的库；不添加仓库 src include、不改安装后的 ZIP 字节。

检查编译来源、ELF/map：默认无 WallClock/TimeSyncService/Coordinator；自身授时版有同一业务时钟，但无 TimeSyncResponder/GatewayTimeResponder；南向版才允许保留两个 responder。
南向模块已在 B3-4c-3 实现并通过软件/安装编译门禁；真实 Method 2 权限和 Hello 协商由原 executor 处理。
此探针的编译成功仍不是子设备真机授时通过，须按 B3-4c-4 单独验证无线、运行资源与生产存储准入。
