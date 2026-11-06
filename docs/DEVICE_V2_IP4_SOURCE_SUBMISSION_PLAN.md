# IP-4 匹配源码提交清单与 CI 验证计划

更新：2026-11-03。承接独立接入计划§17。§1～6为提交审选合同，**本批隔离投影、CI接线及有限验证取§7**。尚未暂存／提交／推送；源码类型核查不等于APK／固件重建或远端CI通过，不部署、不操作设备或账号。

## 1 来源、边界与提交单位

| 仓库 | 当前分支与已提交基线 | 本批角色 |
| --- | --- | --- |
| blinker-app | `i3w-dev-v2` / `8b19ac011b3acf0ec9070666d1b6ec255c30a54c` | 显式产品入口、共享根引用、注册终态／附近投影、公开有限回归 |
| capacitor-wifiprov | `main` / `1240a0771b746022c06825684fbe75f15c38d783` | 原 WiFiProv TS／Android owner、IPC、窄能力及配套回归 |
| blinker-library | `device_v2` / `0e28ca298c9b0282dea4b36e5c3bbc8f3efbfdab` | 根 owner／Presence 修复、既有四槽组合基线及发布检查 |

基线 SHA 不包含当前未提交源码。已验参考为 App `46ca8474…54bd6`、设备运行 BIN `e48cffa2…8f07`，来源及有限验收取主计划§14～16；不能把这两个制品摘要当成未来提交的 Git SHA，也不能据核心文件匹配宣称整个 dirty 树与 APK 同字节。

三仓没有一个共同提交号。“同提交 CI”指：库工作流检出确切 `libraryCommit`；App 集成检查锁定 `appCommit + wifiProvCommit`，并记录同批 `libraryCommit`。禁止只写 `main`、`latest` 或任意最新旁仓。待明确提交时建议顺序：插件完整候选 → 库完整候选 → App 候选及跨仓引用；各自保留可复核的父提交和文件／hunk 清单。失败不替换分支引用或自动重试。

共同约束：不改变原锁文件／依赖／SDK，不扩大准入，不迁 TaskStore、scope 或 UNKNOWN，不复制账号秘密／设备材料。下列路径相对对应仓库根；**列表不是 `git add .` 或目录全选指令**。

## 2 WiFiProv：一个完整插件候选

当前 6 项修改、33 项新增均属既有 owner／IPC／窄能力及回归闭包，建议同一插件提交，不拆成“只有定义可用、原生实现缺失”的中间交付。Android 26 项源码闭包已按§16匹配；其中 17 项待提交、9 项是原跟踪基线。

### 2.1 修改与新增生产源

```text
README.md
src/definitions.ts
src/web.ts
android/src/main/AndroidManifest.xml
android/src/main/java/com/wifiprov/capacitor/WiFiProvPlugin.java
android/src/main/java/com/wifiprov/capacitor/ProvisioningCallScope.java
android/src/main/java/com/wifiprov/capacitor/ProvisioningFailure.java
android/src/main/java/com/wifiprov/capacitor/ProvisioningInternetState.java
android/src/main/java/com/wifiprov/capacitor/ProvisioningLifecycleTrace.java
android/src/main/java/com/wifiprov/capacitor/ProvisioningSdkLocalOwner.java
android/src/main/java/com/wifiprov/capacitor/SoftApSystemConnectedWifi.java
android/src/main/java/com/wifiprov/capacitor/SoftApWorkerChannel.java
android/src/main/java/com/wifiprov/capacitor/SoftApWorkerClient.java
android/src/main/java/com/wifiprov/capacitor/SoftApWorkerProtocol.java
android/src/main/java/com/wifiprov/capacitor/SoftApWorkerService.java
ios/Sources/WiFiProvPlugin/WiFiProvPlugin.swift
```

Swift 修改只对 Android 专属能力返回不支持并显式拒绝，不是 iOS 新功能开放。下列跨平台静态测试直接读取 Swift，不能漏提它后仍声称该合同可复跑；iOS 编译与真机另列未验。

### 2.2 必要回归与公开 runner

```text
android/src/test/java/com/wifiprov/capacitor/ProvisioningCallScopeTest.java
android/src/test/java/com/wifiprov/capacitor/ProvisioningInternetStateTest.java
test/platform-mode-contract.test.mjs
test/bridge-host/README.md
test/bridge-host/WiFiProvPluginBridgeTest.java
test/bridge-host/android-fixtures.mjs
test/bridge-host/bridge-fixtures.mjs
test/bridge-host/run.mjs
test/owner-host/BlinkerFirstInstallOwnerTest.java
test/owner-host/BlinkerWifiReconfigureOwnerTest.java
test/owner-host/ProvisioningSdkLocalOwnerTest.java
test/owner-host/README.md
test/owner-host/SoftApSystemConnectedOwnerTest.java
test/owner-host/fixtures.mjs
test/owner-host/run.mjs
test/worker-host/ProvisioningLifecycleTraceTest.java
test/worker-host/README.md
test/worker-host/SoftApWorkerChannelTest.java
test/worker-host/SoftApWorkerClientTest.java
test/worker-host/SoftApWorkerProtocolTest.java
test/worker-host/WorkerHostTests.java
test/worker-host/android-client-fixtures.mjs
test/worker-host/run.mjs
```

三个 host 目录相互引用：owner 使用 bridge fixtures 和 worker 的 `WorkerHostTests`，worker Client fixture 也使用 bridge Android fixture。不能只提交各自 `run.mjs` 或把被引用的 fixtures 当产物删掉。

以下九项沿原提交保留，不制造新修改：`package.json`、`package-lock.json`、`tsconfig.json`、`rollup.config.mjs`、`LICENSE`、`android/build.gradle`、`android/proguard-rules.pro`、`src/index.ts`、`android/src/main/java/com/wifiprov/capacitor/BleDiscovery.java`。`index.ts` 只有换行字节差异，Git 无语义修改。原 `Package.swift`、podspec 和 `.gitignore` 仍保留，不能宣称 Android 导出工具覆盖 iOS 安装闭包。

## 3 设备库：8 项增量连同必要基线

### 3.1 已验 owner 增量 8 项

```text
src/BlinkerV2/interface/IDeviceAccessChangeObserver.h
src/BlinkerV2/interface/IDeviceAccessStore.h
src/BlinkerV2/identity/DeviceAccessStore.h
src/BlinkerV2/identity/DeviceAccessStore.inc
src/BlinkerV2/provisioning/ControllerControlCoordinator.inc
src/BlinkerV2/runtime/TaskRootOwnerGate.h
src/BlinkerV2/api/TaskProductLifecycle.h
src/BlinkerV2/arduino/platform/esp32/Esp32ProvisionedTaskExtension.h
```

### 3.2 组合、选择器及生命周期基线 14 项

```text
src/BlinkerV2/api/BlinkerFacade.h
src/BlinkerV2/api/Product.h
src/BlinkerV2/api/TaskProductOwner.h
src/BlinkerV2/arduino/config/TaskConfig.h
src/BlinkerV2/arduino/config/Esp32WiFiProvOptions.h
src/BlinkerV2/arduino/internal/WiFiProduct.h
src/BlinkerV2/arduino/internal/WiFiTaskExtension.h
src/BlinkerV2/arduino/platform/esp32/Esp32EdgeHubProduct.h
src/BlinkerV2/arduino/platform/esp32/Esp32WiFiBLEProduct.h
src/BlinkerV2/arduino/ports/esp32/Esp32WiFiProv.h
src/BlinkerV2/interface/IProductLifecycle.h
src/BlinkerV2/runtime/DeferredProvisioningShutdown.h
src/BlinkerV2/runtime/InitialProvisioningWindowIntent.h
src/BlinkerWiFiBLE.h
```

### 3.3 持久确认与续期基线 16 项

```text
src/BlinkerV2/control/HttpDeviceKeySessionProvider.h
src/BlinkerV2/control/HttpDeviceKeySessionProvider.inc
src/BlinkerV2/interface/ISessionProvider.h
src/BlinkerV2/protocol/bbp2/Messages.h
src/BlinkerV2/provisioning/TaskAuthorityContract.h
src/BlinkerV2/provisioning/TaskAuthorityContract.inc
src/BlinkerV2/provisioning/TaskCommandAdapter.h
src/BlinkerV2/provisioning/TaskCommandAdapter.inc
src/BlinkerV2/provisioning/TaskManagementContract.h
src/BlinkerV2/provisioning/TaskManagementContract.inc
src/BlinkerV2/provisioning/TaskManagementSession.h
src/BlinkerV2/runtime/DeviceRuntime.inc
src/BlinkerV2/runtime/LocalAccessCloudTransport.h
src/BlinkerV2/runtime/TaskCloudTransport.h
src/BlinkerV2/transport/ManagedMqttTransport.h
src/BlinkerV2/transport/ManagedMqttTransport.inc
```

以上共 38 项当前 HEAD 外的生产文件，是审选范围而非仅本轮新修改。31 项与 e48c stage 的 LF 内容一致；差异另包括已审 DA-4 双 selector／启动开窗意图，以及公开源去除候选诊断。不能整棵复制私有 stage：尤其 `TaskCloudTransport.h`、`ManagedMqttTransport.inc` 应保留当前无 `Qa3TailTrace` 的公开实现，不带入该私有诊断 consumer。

### 3.4 回归、冻结及原交付工具闭包

```text
tests/host/CMakeLists.txt
tests/host/test_cooperative_presence_main.cpp
tests/host/test_task_product_main.cpp
tests/host/test_device_access_store_main.cpp
tests/host/test_esp32_provisioning_start_intent.cpp
tests/host/test_http_device_key_session_main.cpp
tests/host/test_task_management_session_main.cpp
tests/architecture/check_device_v2.py
tests/architecture/test_product_selectors.py
tests/architecture/test_release_freeze_review.py
tests/freeze/device_key_cloud_login.sha256
tests/freeze/task_service_confirmation.json
tests/release/test_arduino_package.py
tests/release/test_release_support_freeze.py
tests/release/test_esp32_provisioned_task_profile.py
tests/release/test_esp32_softap_tasks_delivery.py
tests/release/test_esp32_softap_tasks_example.py
tests/release/test_esp32_wifiprov_options.py
tests/release/test_wifiprov_standard_example.py
README.md
examples/WiFiBleQuickStart/WiFiBleQuickStart.ino
examples/Esp32SoftApTasksQuickStart/Esp32SoftApTasksQuickStart.ino
examples/Esp32SoftApTasksQuickStart/README.md
examples/Esp32SoftApTasksQuickStart/UserProvisioning.h
profiles/README.md
profiles/esp32-softap-tasks-limited.json
tools/build_arduino_release.py
tools/build_esp32_softap_tasks.py
tools/esp32_provisioned_task_profile.py
tests/arduino/verify_installed_release.py
.github/workflows/device-v2-ci.yml
```

既有跟踪基线仍要可获取：`library.properties`／`library.json`、`profiles/cloud-beta.json`、`tools/arduino_release_profile.py`、原 host stubs／golden／公共实现 TU 及 `.github/workflows/device-v2-cloud-beta.yml`。它们不因清单而修改或重复提交。

必须按 hunk 审选：

- `tests/host/CMakeLists.txt` 仅选本批 owner／启动意图及必要基线接线；若保留全部 diff，另需 `tests/host/test_utc_snapshot_serial_main.cpp` 与 `tests/internal/support/UtcSnapshotSerial.h`，不能只提引用不提源。这两项 UTC 观测不自动纳入本批。
- login freeze 相对 HEAD 含 v50～v53 必要历史，不能只取 v53 两行又漏掉对应生产依赖；§15 的两项追溯保持，不改成接受任意 dirty 源。
- 发布脚本若保留当前 limited profile 接线，上述示例三文件、profile、两工具及对应测试必须一起；不能只提 builder 让新导入在干净检出中缺失。

### 3.5 必要文档与索引

```text
docs/DEVICE_V2_ACCESS_REGISTRATION_CONTRACT_V1.md
docs/DEVICE_V2_INDEPENDENT_ACCESS_REGISTRATION_EXECUTION_PLAN.md
docs/DEVICE_V2_IP4_SOURCE_SUBMISSION_PLAN.md
docs/DEVICE_V2_ACCESS_IDENTITY_AND_NEARBY_EXECUTION_PLAN.md
docs/DEVICE_V2_TEST_REPORT_RECONCILIATION_AND_CORE_PLAN.md
docs/DEVICE_V2_ESP32_SOFTAP_TASKS_LIMITED_DELIVERY.md
```

主线 `docs/DEVICE_V2_NEXT_EXECUTION_PLAN.md` 如需随交付更新，只审选当前索引／边界链接。其它修复、入网、旧发行文档保留原历史；不为本清单打包整套进度，也不夹入活动记录等独立方案的修改。

## 4 App 提交审选

### 4.1 独立注册／共享根／附近的新增源与回归

```text
src/app/core/device-v2/ble-direct/access-registration-api.ts
src/app/core/device-v2/ble-direct/access-registration-contract.ts
src/app/core/device-v2/ble-direct/access-registration-coordinator.ts
src/app/core/device-v2/ble-direct/access-registration-checkpoints.ts
src/app/core/device-v2/ble-direct/credential-root-references.ts
src/app/core/device-v2/ble-direct/credential-root-lifecycle.ts
src/app/core/device-v2/ble-direct/registration-presence-api.ts
src/app/core/device-v2/ble-direct/registration-presence-contract.ts
src/app/core/device-v2/ble-direct/registration-presence-match.ts
src/app/core/device-v2/ble-direct/access-registration-auth.spec.ts
src/app/core/device-v2/ble-direct/access-registration-ports.spec.ts
src/app/core/device-v2/ble-direct/credential-root-lifecycle.spec.ts
src/app/core/device-v2/ble-direct/provisioning-references.spec.ts
src/app/core/device-v2/ble-direct/registration-presence-api.spec.ts
src/app/core/device-v2/ble-direct/registration-presence-store.spec.ts
src/app/core/device-v2/ble-direct/root-reference.spec.ts
src/app/core/device-v2/ble-direct/test-fixtures/access-registration-confirmation-v1.json
src/app/core/services/device-v2-ble-access-registration.spec.ts
src/app/core/services/device-v2-ble-presence-lifecycle.spec.ts
src/app/core/services/device-v2-ble-registration-presence.spec.ts
src/app/core/services/device-v2-management.product.spec.ts
src/app/core/device-v2/device-ui.connectivity-credentials.spec.ts
src/app/sections/guide/ble-device/ble-access-registration-guide.ts
src/app/sections/guide/ble-device/ble-device-conflict.render.spec.ts
src/app/sections/guide/ble-device/ble-device-registration.render.spec.ts
tools/test-access-registration.mjs
tools/test-access-registration.setup.ts
tools/test-access-registration.tsconfig.json
tools/test-access-registration.vitest.mts
```

### 4.2 必要既有消费者的修改

```text
src/app/core/device-v2/ble-direct/client.ts
src/app/core/device-v2/ble-direct/credential-store.ts
src/app/core/device-v2/ble-direct/credential-store.spec.ts
src/app/core/services/device-v2-ble.service.ts
src/app/core/services/device-v2-management.service.ts
src/app/core/services/device-v2-management.service.spec.ts
src/app/core/model/response.model.ts
src/app/core/device-v2/device-ui.port.ts
src/app/core/device-v2/provisioning/wifiprov-retirement.ts
src/app/core/device-v2/provisioning/wifiprov-retirement.spec.ts
src/app/sections/device/device-settings/device-settings.ts
src/app/sections/device/device-settings/device-settings.spec.ts
src/app/sections/guide/ble-device/ble-device.page.ts
src/app/sections/guide/ble-device/ble-device.page.html
src/app/sections/guide/ble-device/ble-device.page.spec.ts
src/app/sections/guide/connection-method.page.scss
src/app/home/components/deviceblock/deviceblock.ts
src/app/home/components/deviceblock/deviceblock.html
src/app/home/components/deviceblock/deviceblock.scss
src/app/home/components/deviceblock/deviceblock.spec.ts
src/app/home/components/deviceblock/deviceblock.dom.spec.ts
src/environments/environment.cloud-beta.ts
src/environments/environment.spec.ts
tools/source-handoff.mjs
tools/source-handoff.test.mjs
docs/APP_DEVICE_V2_SOURCE_HANDOFF.md
```

Settings 原删除／Key 轮换、WiFiProv retirement、`DeviceUiPort.watchCredentialChanges` 的授权失效刷新不可遗漏，否则新注册入口正确而旧入口仍可能沿用已删除授权。首页列表保留实际通道与附近分离，不以附近命中当 Cloud 在线。上述大型 Service、Settings 和 spec 仍须按下文 hunk 边界审选，不是全部 diff 自动许可。

### 4.3 原配网／账号恢复前置基线

当前 `esp32-provision.page.ts` 已在原 resolve 中显式传 `'cloud'`，必须随新 Management 参数合同审选。其完整页面又引用此前 AR／ON 模块；不能只摘本页或拿 IP-4 的25项代替这些前置。审选时曾提出先基线再增量；§7核实语义交织后固定为同一接入原子候选，清单仍分层供审阅，不强造可运行的中间历史。隔离输入已完成产品TS核查，不代表完整制品已重建。

```text
src/app/tools/esp32-provision/esp32-provision.page.ts
src/app/tools/esp32-provision/esp32-provision.page.html
src/app/tools/esp32-provision/esp32-provision.page.scss
src/app/core/device-v2/provisioning/capacitor-esp32-wifiprov.transport.ts
src/app/core/device-v2/provisioning/wifiprov-admin.ts
src/app/core/device-v2/provisioning/wifiprov-allocation.ts
src/app/core/device-v2/provisioning/wifiprov-allocation-store.ts
src/app/core/device-v2/provisioning/wifiprov-account-ready.ts
src/app/core/device-v2/provisioning/wifiprov-cloud-boundary.ts
src/app/core/device-v2/provisioning/wifiprov-cloud-reconciliation.ts
src/app/core/device-v2/provisioning/wifiprov-completion.ts
src/app/core/device-v2/provisioning/wifiprov-config-info-read.ts
src/app/core/device-v2/provisioning/wifiprov-first-install.ts
src/app/core/device-v2/provisioning/wifiprov-management-port.ts
src/app/core/device-v2/provisioning/wifiprov-network-state.ts
src/app/core/device-v2/provisioning/wifiprov-owned-options.ts
src/app/core/device-v2/provisioning/wifiprov-pop.ts
src/app/core/device-v2/provisioning/wifiprov-softap-acquisition.ts
src/app/core/device-v2/provisioning/wifiprov-softap-stages.ts
src/app/core/device-v2/provisioning/wifiprov-system-connected.ts
src/app/core/device-v2/provisioning/wifiprov-unknown-checkpoint.ts
src/app/core/device-v2/provisioning/wifiprov-wifi-failure.ts
src/app/core/device-v2/provisioning/wifiprov-wifi-reconfigure.ts
src/app/core/device-v2/provisioning/wifiprov-write-preflight.ts
src/app/core/device-v2/account-client.ts
src/app/core/device-v2/account-client.spec.ts
src/app/core/services/device-v2.service.ts
src/app/core/services/device-v2.service.spec.ts
src/app/core/services/app-visibility.service.ts
src/app/core/services/app-visibility.service.spec.ts
```

配套已存在的基线回归同时审选，不能将新增有限 95 项写成覆盖全部配网修订：

```text
src/app/core/device-v2/provisioning/wifiprov-account-ready.spec.ts
src/app/core/device-v2/provisioning/wifiprov-admin-existing.spec.ts
src/app/core/device-v2/provisioning/wifiprov-allocation.spec.ts
src/app/core/device-v2/provisioning/wifiprov-cloud-boundary.spec.ts
src/app/core/device-v2/provisioning/wifiprov-cloud-reconciliation.spec.ts
src/app/core/device-v2/provisioning/wifiprov-completion.spec.ts
src/app/core/device-v2/provisioning/wifiprov-config-info-read.spec.ts
src/app/core/device-v2/provisioning/wifiprov-first-install.spec.ts
src/app/core/device-v2/provisioning/wifiprov-management-port.spec.ts
src/app/core/device-v2/provisioning/wifiprov-network-state.spec.ts
src/app/core/device-v2/provisioning/wifiprov-pop.spec.ts
src/app/core/device-v2/provisioning/wifiprov-softap-acquisition.spec.ts
src/app/core/device-v2/provisioning/wifiprov-softap-handoff.spec.ts
src/app/core/device-v2/provisioning/wifiprov-softap-stages.spec.ts
src/app/core/device-v2/provisioning/wifiprov-unknown-checkpoint-store.spec.ts
src/app/core/device-v2/provisioning/wifiprov-wifi-failure.spec.ts
src/app/core/device-v2/provisioning/wifiprov-wifi-reconfigure.spec.ts
src/app/core/device-v2/provisioning/wifiprov-write-preflight.spec.ts
src/app/tools/esp32-provision/esp32-provision.acquisition.spec.ts
src/app/tools/esp32-provision/esp32-provision.ble-pop.spec.ts
src/app/tools/esp32-provision/esp32-provision.completion.render.spec.ts
src/app/tools/esp32-provision/esp32-provision.completion.spec.ts
src/app/tools/esp32-provision/esp32-provision.discovery.spec.ts
src/app/tools/esp32-provision/esp32-provision.first-install.spec.ts
src/app/tools/esp32-provision/esp32-provision.lifecycle.spec.ts
src/app/tools/esp32-provision/esp32-provision.network.spec.ts
src/app/tools/esp32-provision/esp32-provision.page.spec.ts
src/app/tools/esp32-provision/esp32-provision.permissions.spec.ts
src/app/tools/esp32-provision/esp32-provision.reconfigure.spec.ts
src/app/tools/esp32-provision/esp32-provision.recovery.spec.ts
src/app/tools/esp32-provision/esp32-provision.scan-handoff.spec.ts
src/app/tools/esp32-provision/esp32-provision.softap.spec.ts
src/app/tools/esp32-provision/esp32-provision.stages.spec.ts
src/app/tools/esp32-provision/esp32-provision.system-connected.spec.ts
src/app/tools/esp32-provision/esp32-provision.wifi-reconfigure.spec.ts
src/app/sections/guide/ui6-provision.render.spec.ts
src/app/sections/guide/ui6-reconfigure.render.spec.ts
```

原生发现与受限 SoftAP 网络策略的前置，不是本轮扩全局明文权限：

```text
android/app/src/main/java/iot/diandeng/tech/ble/WiFiProvDiscovery.java
android/app/src/test/java/iot/diandeng/tech/ble/WiFiProvDiscoveryTest.java
android/app/src/beta/AndroidManifest.xml
android/app/src/beta/res/xml/wifiprov_network_security_config.xml
tools/cloud-beta.test.mjs
```

Discovery 只在同 owner 显式 stop 后吞预期取消；网络例外仅精确 SoftAP 地址，不放全局 cleartext、不改变 HTTPS WebView origin。CI 同步与配置测试必须保留该窄边界。

### 4.4 按 hunk／文案键审选，不整文件夹带

```text
src/app/configs/api.config.ts
public/i18n/ar/ar.json
public/i18n/de/de.json
public/i18n/en/en.json
public/i18n/es/es.json
public/i18n/fr/fr.json
public/i18n/ja/ja.json
public/i18n/ko/ko.json
public/i18n/pt/pt.json
public/i18n/ru/ru.json
public/i18n/zh_cn/zh_cn.json
public/i18n/zh_hk/zh_hk.json
```

- `api.config.ts` 选择注册用途的 `ACCESS_REGISTRATION_*` 路由／鉴权分类及所选配网基线必需项，不夹入独立定位 `LOCATION_LATEST/LEASE/NETWORK`。
- 11 份 locale 选择真实注册／冲突／恢复模板引用键，并随所选配网基线补其必要键；不整体加入 AI、地图、组件编辑器等累计文案。
- 原配网页、Management Service、Settings、大型 spec，以及 `device-v2.service.ts` 的其它功能依赖必须分层。只交本文件列出的接入基线时，不可留下对未选定位等新模块的导入；否则仍是缺依赖候选，须继续拆 hunk，不能为使类型通过把无关功能全带上。
- `angular.json` 目前只是 testing 的 `externalDependencies` 修订，不是 cloud-beta 生产接线；单独审定，不默认纳入。`android/capacitor.settings.gradle` 为 Cap sync 生成的本机 pnpm 路径，明确排除。

### 4.5 已提交基线、其它旁仓及服务端前置

App 的原 `package.json`、`pnpm-lock.yaml`、workspace、`vitest.config.ts`、`tsconfig.spec.json`、`src/test-setup.ts`、`account-scope.ts`、Direct crypto／wire／session／secure-channel／transport／presence／presence-key-api、`esp32-wifiprov.ts`、原拦截器及 Data／User Service 都保留原基线，不制造修改。`GATEWAY_NO_AUTH_REPLAY` 已在该基线，不能删掉再靠新模块补救。

App 的另外四个 `file:../` 插件当前 clean，CI 仍需按原锁和以下 SHA 同级检出；它们不是本轮提交对象：

| 仓库 | 基线 SHA |
| --- | --- |
| capacitor-android-shortcuts | `2c187e447542843dba1264bd95c297686ade6c49` |
| capacitor-mdns | `8525b992e8a53c4f60b97b076da48606a373135e` |
| capacitor-ntfy | `ed658dd2b6e89fa6fd89a8c85df7b5680851170e` |
| capacitor-wechat | `ba0bfc5677b2a0dfe11d231c43b52e07af0bf487` |

Gateway 仍须匹配原 registration／presence API、059／060 及 Nginx 窄路由；历史部署来源取主计划§13.3，不在本轮重新核线上状态、部署或提交服务仓。三仓源码 CI 通过不自动证明运行服务匹配，也不把 Service 业务库搬到 CI。

## 5 同提交 CI：计划而非本批执行结果

### 5.1 现有能力与真实阻点

设备库已有 `device-v2-ci.yml`：architecture／有限 Python、host-quality／sanitizers、package-boundary 及原 SDK 矩阵。`cloud-beta-candidate` 依赖 host-quality 和 package-boundary，同库 SHA 调用 `device-v2-cloud-beta.yml`，只验 ESP32 手动 WiFi 的零槽 `esp32-wifi`、`esp32-cloud-smoke` 及可选 location input。**不因此覆盖四槽配网图或 BLE→Cloud。**

§17审选时App和WiFiProv均无远端工作流；本批已新增App匹配CI定义并在其中编译／验证插件，见§7。尚未触发远端，不能把本机95／14项或插件host结果写作远端通过。原App `source-handoff` 的 `rebuild` 仍只接受其声明的普通／本机配置，不代签46ca cloud-beta APK。

设备库跨语言 golden 另有既有私有服务仓读取权限 HOLD，取[发行计划§14.9](DEVICE_V2_LIBRARY_RELEASE_AND_AILY_BUILDER_PLAN.md#149-现行结果与发行决定源码已交付私有-oracle-授权待补hold)。未明确只读 CI 授权前，不复制私有协议到公开仓、不使用服务器 SSH 密钥或个人管理员 token、不跳过 golden 后称发行通过。

### 5.2 明确提交后的最小验证范围

1. **来源门**：在干净 CI 检出记录三仓完整 SHA、所有必要文件及锁文件摘要；App 按原同级 `file:../capacitor-wifiprov` 布局检出精确插件提交。沿 App／template 原 pnpm frozen lock 与插件原 npm lock，忽略安装钩子后显式构建；原 Node／pnpm／npm 版本取仓库及现有工具合同，不安装“最新版”。若旁仓私有也须明确只读授权，不继承本机或部署凭据。不得用当前 HEAD 冒充未提交候选，preflight 仅验存在，不能验版本匹配。
2. **插件门**：原 `package-lock.json` 的 `npm ci` 与 `npm run build`，不回写锁；`node --test test/platform-mode-contract.test.mjs` 验 TS／Web／Swift 拒绝合同。Android 经匹配 App Cap sync 后确认实际 Gradle project 名，编译该插件并跑 `testDebugUnitTest`。沿原 JDK 21、Android 36、AGP 8.13.0 和 Espressif `lib-2.4.4`，不换版本。
3. **App 有限门**：原 pnpm 锁安装后，运行 `node tools/test-access-registration.mjs --preflight`、`node tools/test-access-registration.mjs`、`node --test tools/source-handoff.test.mjs` 和 `node --test tools/cloud-beta.test.mjs`。有限入口是 4 文件，不改为会引入旧无关 spec 类型错误的全 App test；不绕过原 CAS／权限测试语义。§4.3 基线回归先完整保留；若对应基线形成新组合、runner 尚无公开选择入口，须先补有限 CI 选择，不以这 4 文件代签全部基线。当前未宣称这项接线已实现。
4. **App 制品门**：原 `pnpm run build:cloud-beta`、原 `tools/sync-cloud-beta.mjs` 与 Gradle `:app:assembleBeta`（beta 是 buildType，不是 BetaDebug flavor）；核 HTTPS、包名、原生注册表、功能开关及更新签名合同。CI 无匹配签名时只称源码／debug 签名变体构建，不称可覆盖当前手机安装的匹配 APK；不上传签名私钥。CI 不登录或接触账号／设备。
5. **库有限门**：同 `libraryCommit` 执行原 architecture／freeze、CMake host（含 cooperative Presence、Store、TaskProduct、启动意图及持久确认／续期相邻合同）和 package-boundary；已有工作流覆盖的门不新增第二套 runner。具体远端结果须取该提交的 run，而不是复用本机已验计数。
6. **四槽组合门**：若本次交付声明包括普通Arduino四槽组合，额外在同SHA复用 `tools/build_esp32_softap_tasks.py` 及 `tests/arduino/verify_installed_release.py --target esp32-softap-tasks --softap-task-manifest ...`，原SDK3.3.11、XIAO ESP32S3、公开示例／profile，只做安装／完整链接。默认cloud-beta没有此门；本批已按§7增加独立受限调用，尚待远端结果。不能烧编译夹具、加入真实Key／PoP或扩大默认准入，也不签内部BLE四槽运行图。
7. **发行门**：仅在明确授权的私有只读 checkout 补齐后执行真实服务端 golden，再核同库 SHA 的声明目标 CI。HOLD 或 skip 必须保留，缺任一依赖不得改成“全部通过”。

插件 host runner 若需迁入 CI，复用 `test/{bridge,owner,worker}-host` 原脚本及 README，提供明确的原 JAR／Capacitor source 输入和固定 JSON 摘要；runner 不自行下载或自动重试。只跑一个默认 class 不代表全部 host 合同；这项 CI 依赖接线尚未实现，不用被动 fixture 代签原生 ABI、Binder 或无线行为。

## 6 提交门、排除项与后续顺序

本轮已按§7生成一次隔离源码投影，不暂存或提交；它不是Git补丁组合或完整制品。这是新输入验证，不代表需再次采集配网、自动化、Start或断电样本。

明确排除：各仓 `build/`、`.build/`、stage、SDK/cache、`node_modules/`、`dist/`、生成的 Capacitor settings／assets、APK／BIN／ZIP／JAR、账号与完整请求日志副本、真实 PoP／Key／网络密码／NVS／签名私钥、设备库 `docs/partition.csv` 及无关 AI／地图／编辑器修改。原日志和 UNKNOWN 留原私有位置，不删除、不作为提交附件。

混合投影与CI定义已按§7实施，不重复派发。后续依次：明确Git授权后形成精确提交组合 → 确认必要只读权限及工作流可用 → 仅运行该组合的声明门并记录结果。当前不触发远端、不写secrets、不修改账号或准入；本文件不是发行公告。

## 7 混合源码隔离与匹配 CI 实施（2026-11-03）

### 7.1 实际分层及有限工具

实际diff核对：DeviceV2 Service全属Cloud／Local隔离；Management为新鲜owner／禁重放、产品resolve及Cloud只读确认；Settings为共享根生命周期和独立删除。它们与原配网页、AR／ON薄模块形成一个接入原子候选，不抽第二个HTTP服务、不为拆历史恢复旧enable-cloud跨产品复用逻辑。真正混合的是API中的定位改动和locale中的QA分享计数。

App新增纯`selectAccessApi(base,current)`只取五个注册常量、鉴权调用及精确helper；`selectAccessLocale`只取26＋1个注册键，保留base其余字节／PROFILE，支持缺父对象语言。未知／缺失／歧义形状拒绝，不猜hunk，实际API／11locale不修改。

机器输入`tools/access-source-files.json`包含72前置、54接入、12投影及8维护输入，共146个不重复路径，冻结base `8b19ac0…54c`。新增提交项：

```text
tools/access-source-selection.mjs
tools/access-source-selection.test.mjs
tools/access-source-files.json
tools/prepare-access-sources.mjs
tools/prepare-access-sources.test.mjs
tools/access-ci.mjs
tools/access-ci.test.mjs
.github/workflows/device-v2-access.yml
```

`node tools/prepare-access-sources.mjs --out build/NEW_DIRECTORY`复用原导出边界，从Git读基线blob并叠明确源／投影，不改源码或索引。仅允许经`git check-ignore`确认的本App新build子目录；拒绝覆盖、HEAD漂移、缺失输入、链接、TAB／控制字符及越界路径，保留二进制blob。不是通用patch引擎，base变化必须重新审阅，不能自动接受最新HEAD。原导出白名单同步包含这些维护入口和workflow，避免再次漏交。

### 7.2 本批实际验证

新增集合唯一用例：纯投影10、组装／路径7、App CI计划／冻结base接线7、导出新1、库受限CI新6，共31项通过，重叠重跑不累计。保留整文件选择混入定位／分享计数的先红；现有目录反例曾命中不同拒绝分支，修正边界后核对；独立审阅另补TAB截断和输出独占保护。未重跑原95项或旧真机矩阵。

只生成一次App忽略目录`build/ip4-source-selection-20261103/candidate`，1979份输入包含原Git基线，不是1979项新修改。无emit产品TS核查先因原已安装WiFiProv旧声明缺能力停止；按原同级file依赖改为匹配插件源码解析后通过，无其它缺失前置。不回写`node_modules`、声明、生产tsconfig或锁，不用断言通关。该结果不签Angular模板、原生ABI、APK或SDK完整链接。

首次组装后仅维护工具／CI回归及路径保护调整，所核产品源未变；stage保留当时摘要，不声称后续维护工具与stage全字节相同，也不把它作为可交付制品。

### 7.3 CI定义与剩余门

App新workflow仅手动触发：App锁`github.sha`，WiFiProv／Library须完整40位SHA，四旁仓固定§4.5参考SHA。先核来源、HEAD、clean和原锁；私仓无读取权即失败，不借部署凭据。App检出保留审阅base历史，投影测试比较固定base而非未来新HEAD。原版本显式build插件后App frozen install、平台拒绝合同、维护测试及原四文件入口；Android显式true才cloud-beta sync、插件javac／JUnit和`:app:assembleBeta`，false不签原生。无APK上传、发布或账号／设备操作；有限入口不代签全部AR／ON或全App测试。

库新增`.github/workflows/device-v2-softap-tasks.yml`仅供原CI调用，caller同SHA且保留`needs:[host-quality,package-boundary]`。原CLI1.5.1／SDK3.3.11、单worker，复用固定builder／安装validator，仅四槽SoftAP目标；只上传ZIP、校验和公开profile回执。新增`tests/architecture/test_softap_tasks_ci.py`随交付，不开放独立manual发行、不加`always()`／秘密。普通cloud-beta仍零槽，准入不扩大。

本批**未触发远端CI、未构建Android或执行SDK完整链接**；私有golden只读授权HOLD不变。当前只完成本机维护合同及隔离产品TS门；提交组合形成后才执行对应远端声明门，不用旧HEAD或本机计数伪装发行。授权、TaskStore、scope、UNKNOWN、完整日志和无关dirty保留，未暂存／提交／推送。

## 8 后续BC-1独立增量（2026-11-03）

[反向注册计划§6](DEVICE_V2_BLE_TO_CLOUD_REGISTRATION_PLAN.md#6-bc-1实施与有限结果)新增只读任务绑定观测，和§7的既有App投影／e48c运行候选分开记录；不回写旧候选摘要或声称已部署。后续必要源码审选另包含：

```text
src/BlinkerV2/runtime/TaskSnapshotStore.h
src/BlinkerV2/runtime/TaskSnapshotStore.inc
tests/host/test_task_binding_observation_main.cpp
docs/DEVICE_V2_BLE_TO_CLOUD_REGISTRATION_PLAN.md
```

以及原`tests/host/CMakeLists.txt`中新目标和add_test两处hunk；保留其它既有dirty接线。现有host-quality会随同提交编译／运行该目标，无需新增重复SDK队列。同步主计划§19、v1合同及旧DA页首当前索引，不重复提交无变化文件。一个host目标含9组场景已过，不代表反向已开放或全部质量／发行CI已过；授权、TaskStore、UNKNOWN、私有golden HOLD及暂不提交推送边界不变。

## 9 后续BC-2a独立增量（2026-11-03）

[反向注册计划§7](DEVICE_V2_BLE_TO_CLOUD_REGISTRATION_PLAN.md#7-bc-2a内部保根首次key事务与旧旁路封堵)在BC-1之后补内部保根首次Key事务及旧UNKNOWN旁路封堵；这是新增源码层，不属于已验e48c固件或§7隔离App投影，不覆盖其输入摘要。后续审选以下10个生产路径中的本批必要hunk，保留原Presence／任务／持久确认等前置，不整体覆盖混合dirty文件：

```text
src/BlinkerV2/interface/IDeviceAccessStore.h
src/BlinkerV2/interface/IDeviceAccessChangeObserver.h
src/BlinkerV2/identity/DeviceAccessStore.h
src/BlinkerV2/identity/DeviceAccessStore.inc
src/BlinkerV2/runtime/TaskRootOwnerGate.h
src/BlinkerV2/arduino/platform/TaskModule.h
src/BlinkerV2/arduino/platform/esp32/Esp32TaskModule.h
src/BlinkerV2/arduino/internal/WiFiTaskExtension.h
src/BlinkerV2/arduino/platform/esp32/Esp32ProvisionedTaskExtension.h
src/BlinkerV2/provisioning/DeviceKeyProvisioningEndpoint.inc
```

配套新三目标及相邻测试最小变更：

```text
tests/host/test_task_initial_cloud_key_main.cpp
tests/host/test_device_key_initial_cloud_gate_main.cpp
tests/host/test_cooperative_initial_cloud_key_product_main.cpp
tests/host/CMakeLists.txt
tests/host/test_device_access_store_main.cpp
tests/host/test_task_product_main.cpp
```

旧Store仅修受影响4函数和窄执行入口，原TaskProduct main仅显式return0供新fixture复用，不把旧文件全部累计diff当本批增量。同步反向计划§7、主计划§20、DeviceKey合同§4.2及本清单。新三目标25组、必要相邻回归及架构检查已过；没有原SDK编译、真实认证或远端CI结论。

本批没有App／插件／服务端源码变更，不改锁、磁盘或wire版本。网络M2／intent授权尚未接，BC-3的Key发行／UNKNOWN生命周期也未实现；此清单不构成开放注册或发行授权。所有新host源必须随同提交进入原CMake／host-quality，不能只交生产头文件；原私有golden HOLD、完整日志保留及暂不提交推送边界不变。

## 10 后续BC-2b独立增量（2026-11-04）

设备内部授权合同取[反向注册计划§8](DEVICE_V2_BLE_TO_CLOUD_REGISTRATION_PLAN.md#8-bc-2b已提交method2与单次注册意图的最小合同)。本批接真实Method2 producer与实际写前guard，仍依赖§8～9的原TaskStore、owner和首次Key事务；不覆盖已验App／固件输入摘要。审选以下必要路径中的本批hunk：

```text
src/BlinkerV2/interface/IInitialCloudKeyInstaller.h
src/BlinkerV2/interface/IDeviceAccessStore.h
src/BlinkerV2/identity/DeviceAccessStore.h
src/BlinkerV2/identity/DeviceAccessStore.inc
src/BlinkerV2/arduino/platform/TaskModule.h
src/BlinkerV2/arduino/platform/esp32/Esp32TaskModule.h
src/BlinkerV2/arduino/internal/WiFiTaskExtension.h
src/BlinkerV2/arduino/platform/esp32/Esp32ProvisionedTaskExtension.h
src/BlinkerV2/security/DirectSecureSession.h
src/BlinkerV2/transport/BleFrameTransport.h
src/BlinkerV2/transport/BleFrameTransport.inc
src/BlinkerV2/security/InitialCloudKeyRegistration.h
src/BlinkerV2/security/InitialCloudKeyRegistration.inc
src/BlinkerV2/implementation/BlinkerSecurity.cpp
tests/host/test_initial_cloud_key_registration_main.cpp
tests/host/test_initial_cloud_key_commit_guard_main.cpp
tests/host/CMakeLists.txt
tests/host/test_cooperative_initial_cloud_key_product_main.cpp
```

新增实现沿原Security责任单元包含，不增加第二套编译单元或加密依赖；旧BC-2a product test仅增加显式返回和可选main名，供新目标复用原fixture，不重跑旧suite。原CMake／host-quality须带两个新目标，不只提交授权头文件。同步反向计划§8、本清单及主计划§21，实际本机结果以反向计划§8.4为准，不提前当远端CI或原SDK链接结论。

本批不修改App／WiFiProv／Gateway、默认产品图、磁盘格式、公开wire、锁或SDK；BC-3的独立目标／Key发行／UNKNOWN生命周期和BC-4的PreCloudDirect阶段仍待实现。私有服务golden授权HOLD继续保留，本批自有合成HMAC向量不替代它；暂不提交推送。

## 11 后续BC-3a服务端与文档增量（2026-11-04）

本批开始审选原`aily-blinker-services/dev_v2`，HEAD仍为`44ebe5e997eff861c98bf7debc1bf7870621938e`，未暂存／提交／推送。只补内部staging及保留围栏；原App／WiFiProv和设备库生产源未变，不把新Gateway源码当已部署服务或旧固件输入。合同取[BLE→Cloud v2](DEVICE_V2_BLE_TO_CLOUD_REGISTRATION_CONTRACT_V2.md)，结果取[反向计划§9](DEVICE_V2_BLE_TO_CLOUD_REGISTRATION_PLAN.md#9-bc-3a独立cloud发行账本及保留围栏2026-11-04)。

服务仓新生产源：

```text
services/gateway/src/device-v2/access-registration-cloud-contract.js
services/gateway/src/device-v2/access-registration-cloud-repository.js
services/gateway/src/device-v2/access-registration-cloud-key.js
services/gateway/src/device-v2/access-registration-cloud-lifecycle.js
services/gateway/src/device-v2/device-key-material.js
services/gateway/src/migrations/061-device-v2-cloud-registration-staging.js
```

原路径仅选必要hunk，保留其它累计dirty：

```text
services/gateway/src/device-v2/repository.js
services/gateway/src/device-v2/registration-quota.js
services/gateway/src/device-v2/access-registration-lifecycle.js
services/gateway/src/device-v2/ble-enrollment.js
services/gateway/src/database.js
services/gateway/src/database-migrations.js
services/gateway/package.json
package.json
```

其中repo只提共用Key生成；quota加原UNION目标；生命周期／BLE恢复／账号删除各接原事务薄口；registry只追加061。与原IP-2 server baseline、059／060、原vault／注册／quota及删除消费者一起审选，不能仅挑新六文件就宣称可重建。060和v1合同不改，不把旧enable-cloud打开，不提前建active Cloud卡或搬任务scope。

新有限测试和fixture必须同交：

```text
services/gateway/test-device-v2-cloud-registration-contract.js
services/gateway/test-device-v2-cloud-registration-repository.js
services/gateway/test-device-v2-cloud-registration-key.js
services/gateway/test-device-v2-cloud-registration-migration.js
services/gateway/test-device-v2-access-registration-cloud-lifecycle.js
services/gateway/test-device-v2-access-registration-cloud-enrollment-guard.js
services/gateway/test-helpers/cloud-registration-fixture.js
```

另带可选真实数据库runner`services/gateway/test-device-v2-cloud-registration-postgres.js`，仅接受显式隔离`TEST_DATABASE_URL`、本批未执行。维护脚本分别是`test:cloud-registration-staging`（六新文件，40项）和独立PG入口；原workspace Device V2 runner调用前者，不自行启动Docker、下载依赖或连接云数据库。

受影响旧测试仅选空新ledger carrier／两个quota反例及registry历史期望hunk：registration-quota、access-registration-lifecycle、ble-enrollment、direct-access、presence-keys、access-registration-presence、registration-product和access-registration-postgres。PG history保留59／60前缀及原before MAX／count60，再追加61，不修改生产migrator以绕历史断言。新DDL纯断言及假的pg client不代签真实迁移；缺数据库验证保持待验。

设备库文档随同包含新v2合同、反向计划§9、主计划§22及本清单。排除原始Key、KEK、`.env`、网络密码、私有日志、备份及构建产物；新向量只用合成fixture。原锁／SDK、无关本地修改及私有golden授权HOLD不变。未来先补数据库及BC-3b读取／确认／激活，再接BC-4；本清单不是开放或发行公告。

## 12 BC-3b数据库前置准备增量（2026-11-04）

本批服务仓仅`services/gateway/test-device-v2-cloud-registration-postgres.js`新增第5事务子项，并将read／issue的executor变为可选、默认observer保持。沿原生产SERIALIZABLE事务测试回滚和COMMIT后调用方丢响应，不添加生产Key读取／激活、环境变量、依赖或部署配置。同步反向计划§10、主计划§23及本清单。

只有Node语法及空白检查，五项PG均未运行；独立数据库选择待用户确认。不将该源码准备计作实库通过，不绕过数据库／私有golden门或运行业务库。未暂存、提交或推送，无关本地修改保留。

## 13 BC-3b实库与内部读取增量（2026-11-04）

§12的环境待确认已由用户授权后的独立数据库解除：061五个实库子项全部通过，库保留供后续复用。详情取[反向计划§11](DEVICE_V2_BLE_TO_CLOUD_REGISTRATION_PLAN.md#11-bc-3b061实库与内部key读取2026-11-04)，不能由此称无线或完整反向注册已开放。

在§11／12必要源码之上，本批追加：

```text
services/gateway/src/device-v2/access-registration-cloud-key-service.js
services/gateway/test-device-key-recent-auth-lease.js
services/gateway/test-device-v2-cloud-registration-key-service.js
services/gateway/test-device-v2-cloud-registration-key-reader-repository.js
scripts/run-gateway-isolated-postgres.py
scripts/test-run-gateway-isolated-postgres.py
docs/SERVICE_DEVICE_V2_ISOLATED_POSTGRES_TESTING.md
```

原文件只取本批必要hunk：`device-key-recent-auth.js`内部lease、`access-registration-cloud-repository.js`可选keyAccess／私有锁口、`test-helpers/cloud-registration-fixture.js`实际事务选项记录、Gateway的`test:cloud-registration-key-read`及workspace调用、`.gitignore`的Python维护缓存排除。未修改原Lua、登录／注销、锁文件、SDK、061 DDL或060；新reader不在HTTP／index挂载，不混入下一批激活或Presence变更。

源仓HEAD仍为原`44ebe5e997eff861c98bf7debc1bf7870621938e`，索引未改；原未提交的AI／部署／任务等无关修改保持。新Node31叶及必要相邻25叶、Python7项和PG5子项分开记录，不累计pattern跳过或旧host／无线。维护runner的有界输出／单进程修正仅有限离线验证，不为保存日志重跑PG。

设备库只同步本计划、反向计划§11、主计划§24和v2合同；不新增设备库生产修改。服务器测试密码、SSH凭据、私有`runner.json`／env、完整私有日志、setup脚本、备份及构建缓存全部排除。暂不暂存／提交／推送，不触发旧HEAD远端CI，不绕过私有golden HOLD。后续确认／激活／材料归属／删除闭合后再决定显式调用方交付。

## 14 BC-3b确认、激活及独立删除增量（2026-11-04）

承接§13，在原服务工作树内部完成确认／原子激活／材料归属／删除，结果取[反向计划§12](DEVICE_V2_BLE_TO_CLOUD_REGISTRATION_PLAN.md#12-bc-3b确认与激活收口2026-11-04)。新增生产与测试文件：

```text
services/gateway/src/device-v2/access-registration-cloud-activation.js
services/gateway/src/migrations/062-device-v2-cloud-registration-activation.js
services/gateway/test-device-v2-cloud-registration-commit-contract.js
services/gateway/test-device-v2-cloud-registration-activation-migration.js
services/gateway/test-device-v2-cloud-registration-activation-repository.js
services/gateway/test-device-v2-cloud-registration-presence-lifecycle.js
services/gateway/test-device-v2-cloud-registration-shared-root-enrollment.js
services/gateway/test-device-v2-cloud-registration-activation-postgres.js
```

必要旧文件按hunk审选，不能把累计dirty全算本批：

- cloud-contract：严格receipt／commit解析及两个独立摘要域；cloud-repository：原事务内部commit、committed投影／墓碑及精确重放；cloud fixture：正式writer及回滚载体，不模拟真实SQL锁证明。
- access-registration-presence-binding／lifecycle：固定原BLE writer、原AAD别名读取、删除前完整CAS及末删审计保留；removal仅在原清理条件保必要材料，仍撤本卡ACL；ble-enrollment只给新的已有根恢复接原共享根围栏，终态重放保留。
- 原BLE／Presence／lifecycle fixture只补实际新查询的明确载体；原061历史测试及旧access-registration PG只追加62且保原前缀，不重跑旧实库。database-migrations只注册062；060／061本体不修改。
- Gateway新增`test:cloud-registration-activation`含五个新纯目标，独立`test:cloud-registration-activation-postgres`只接受明确隔离URL；workspace原Device V2 runner接前者，不自动连接数据库或部署。锁文件、SDK及旧维护入口保持。

新增纯Node唯一36叶、必要相邻35叶与新PG7子项分开记录，不叠加定向改后重跑或父项。新PG复用§13的SSH／专用库运行当前本地源码一次，after-hook正常清理，不复制源码到服务器、不构建或替换业务镜像。

设备库只更新主计划§25、反向计划§12、v2合同§7及本清单，无生产源／App／WiFiProv改动。保留无关本地修改、授权、TaskStore、scope、UNKNOWN及完整日志，排除私有凭据／配置、备份和构建产物。尚未暂存、提交、推送或触发远端CI；HTTP和BC-4实际调用方未开放，私有golden HOLD不变。

## 15 BC-4a 专用 wire、App 回执及 NimBLE 退出证据（2026-11-04）

合同／验证以[反向计划§13](DEVICE_V2_BLE_TO_CLOUD_REGISTRATION_PLAN.md#13-bc-4a专用安装回执与原生停止证据2026-11-04)为准，不改变旧 IP-4 已验候选的源码清单。

- 库：`protocol/bbp2/InitialCloudKeyInstall.h`、`security/InitialCloudKeyRegistrationEndpoint.h`、`Frame.h` 新 kind；`BleFrameTransport` 的绑定加密发送／擦除、原 `BleRecordTransport` 的窄队列预检，连同 `test_initial_cloud_key_wire_main.cpp`、复用的 Method 2 fixture 和 CMake 定向入口。普通 Runtime 没有新 handler 或公开能力位。
- ESP32 native：`Esp32NimBle.h/.ipp`、`test_esp32_nimble_main.cpp` 和实际修改的 `stubs/esp32_nimble`；只认 ACL／Host 停止／任务暂停和反初始化的证据，保留失败 claim。原 WiFiBLE product、DeviceKey/login freeze 不加入本批增量。
- App：`ble-direct/{client.ts,crypto.ts,secure-channel.ts,cloud-registration-wire.ts,cloud-registration-install.spec.ts}`、`protocol/device-v2/types.ts`；在原 `tools/access-source-files.json` 只补该 client 的 5 个依赖／测试输入，避免源码选取遗漏。保留原 runner 的四套测试边界，不冒称新组合已在既有远端CI构建。
- Gateway：只新增 `test-device-v2-cloud-registration-commit-contract.js` 的 BC-4 固定回执向量叶，不动生产代码、migration 或 HTTP。同步 BBP/2 候选登记与 v2 合同§8。

原锁、SDK、权限与完整日志保持；不提交、推送、部署或烧录，不重复实库和无线样本。排除私有配置、测试账户数据、原始现场凭据和 build 目录；合成向量不是私有 golden，发行 HOLD 不变。
