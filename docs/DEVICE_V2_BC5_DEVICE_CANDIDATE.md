# BC-5 设备候选：冷预检与离线构建

本配方只生成一次显式 ESP32-S3 内测候选；不启动网关、不改账号/设备、不上传。`Esp32InitialCloudWifiOnlyHttpProbe` 的 `.invalid` 地址与夹具公钥仅作完整链接证明，禁止烧录。可运行入口单列为 [Esp32PreCloudWifiCandidate](../tests/internal/arduino/Esp32PreCloudWifiCandidate/Esp32PreCloudWifiCandidate.ino)，不进入普通示例或默认产品。

## 开始前

1. 现场必须有独立、已合法登记的 BLE-only 测试源。当前同账号能选中它，原 Admin/Presence/物理 instance 与板内一致，无 DeviceKey，原 TaskStore 实际可读且为 Unbound。USB 枚举、离线状态、没有任务显示、旧读取均不能证明这些条件；不清根或迁旧 scope 造样本。真正写前仍由原 TaskStore 重新检查 Unbound；Bound（包括零条）、Retired 或 Unknown 均拒绝。
2. 在已批准的测试环境只读确认实际控制面、认证后的 Broker、TLS 信任根、P256 算法 2／keyId 1 公钥以及 061/062；测试 Auth 域名不是设备控制面。两个输入仍采用现有 schema：严格发布形状的 Cloud WiFi transport profile（HTTPS/MQTTS、空业务 ring），以及对应测试 Gateway 的 development Cloud Task pilot signing profile（只含一个现行 P256 公钥）。`profiles/cloud-beta.json` 和 `esp32-softap-tasks-limited.json` 是既有公开参考，历史资料不能代替本轮匹配。只传公开 PEM、公钥和端点，不传账号、令牌、WiFi、DeviceKey 或私钥。
3. 冷预检不操作串口、ROM、Flash 或 NVS。2026-11-04 只枚举到 COM11（Espressif USB）；[最近有证据的收尾](DEVICE_V2_INDEPENDENT_ACCESS_REGISTRATION_EXECUTION_PLAN.md#146-匹配固件及两卡有限真机结果)保留 Cloud 固件、Key 和 Cloud 卡，删除 BLE 卡。现无独立合格源或 COM11 已转换的证据，不能使用此板开始 BC-5，也不提前刷写。

## 使用原渲染器（具备匹配输入后）

从审阅冻结后的隔离源码根目录运行，不把主工作树混合修改打入候选；为本批分配未使用的绝对临时目录。`$transport`、`$signing` 是前述已核对的 schema 1 公开输入文件，`$pubsub` 为现有 PubSubClient 2.8 目录；`$cli`、`$sdkConfig` 指向原固定工具及其配置，不升级依赖。第一条只复制原白名单到隔离目录，不重写本库、不生成公开 ZIP。第二条复用现有内部四槽 SoftAP/TLS 渲染和源码身份摘要；内部 profile 的 `softap` 选择不启动 stock manager。

```powershell
$boundary = '<本批新的隔离目录>'
$library = Join-Path $boundary 'library'
$build = Join-Path $boundary 'build'
python -B -c "import sys; from pathlib import Path; sys.path.insert(0,'tools'); import build_arduino_release as p; _,files=p.validate(); p.stage(Path(sys.argv[1]),files)" $library
python -B tools/esp32_provisioned_task_profile.py --library $library --boundary $boundary --transport-profile $transport --signing-profile $signing --bearer softap
& $cli --config-file $sdkConfig compile --fqbn 'esp32:esp32:XIAO_ESP32S3:PartitionScheme=default_8MB' --jobs 1 --library $library --library $pubsub --build-path $build tests/internal/arduino/Esp32PreCloudWifiCandidate
```

固定 Arduino CLI 1.5.1／ESP32 3.3.11；保留本次源码、两份公开输入和最终 app BIN 的 SHA-256、SDK、分区、必要 ELF 符号。渲染回执的 `deployable=false`、`runtimeSigningMatched=false` 和 `formalAdmission=false` 必须保留，不能把编译当作准入。若 CLI 最终选错库或 ELF 含 stock manager init/start/register，停止；原 Arduino STA 引入的未触发 manager deinit 符号不单独作为拒绝条件。

仅在 BC-5 入口、现场源、匹配 App/服务、实际分区与保留合同全部确认且另行批准写入后，才讨论设备操作。该 Sketch 上电不打开窗口；现场一次显式 USB 串口 `BC5-OPEN`（LF）才请求打开，输出只有结果码。它不打印 Key、WiFi、Admin 或临时 AP 材料；`witness` 是只读能力，不启用任务/普通 Direct 或通用 WiFiProv。官方 Apply 回执、实际 STA、当前新鲜 Cloud 仍分别取证；未知不重发。
