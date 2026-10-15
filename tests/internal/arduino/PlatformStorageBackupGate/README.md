# Platform external storage maintenance gate

此内部夹具仅用于测试板 USB 备份与精确恢复，不是用户例程或产品 API。普通主 MCU 固件备份不包含 UNO R4 WiFi 桥接模块的 Preferences，也不包含 Wio Terminal 的外部 QSPI。夹具不启动 Blinker 产品、WiFi、BLE、任务或迁移流程，不升级连接模块固件。

必须先保存原主 MCU 固件，确认物理板身份、目标与端口，释放原产品连接，再加载对应维护固件。所有快照包含原始网络信息和凭据，只能保存到私有位置；禁止终端、请求日志、调试输出、Git 或公共验收包。CRC 用于完整性及当前状态检查，不是身份认证。主机仍须核对原私有备份的 SHA 和物理板身份。

## USB 二进制合同

115200 baud，无启动文本、调试信息、JSON 或自动操作。所有长度和帧 CRC 字段为小端；payload 内的原生产记录保持原字节序。CRC 为标准 reflected CRC32，初值及最终异或为 `0xffffffff`，多项式 `0xedb88320`。

每次只允许一个请求。禁止流水线请求或尾随额外字节；主机收到完整响应前不得发送另一指令。

| 请求 | 响应 | 含义 |
| --- | --- | --- |
| ASCII `d`，无换行 | `BVS1` + u16 length + u32 CRC + payload | 两次完整读取一致，并通过布局和合法实例 anchor 检查后，才导出快照 |
| ASCII `r` + `BVR1` + u16 length + u32 expectedCurrentCRC + u32 desiredCRC + payload | `BVA1` + u32 finalCRC | 完整接收、当前两读一致、两个 CRC、目标布局和同实例校验全部通过后，才恢复并读回核对 |
| 任何失败 | `BVE1` + u8 code | 不输出原始内容；固定错误码见下表 |

`r` 成功还需主机另做两次 `d`，核对每次 CRC、长度和 payload 全字节等于原私有备份，然后才允许恢复原主 MCU 固件。仅 CRC 或 `BVA1` 相同不能替代全字节验证。

接收 header 与 payload 共用 15 秒期限；发送帧共用 15 秒期限。主机须为 R4 多次 RPC 和 Wio 扇区擦除另留处理时间，例如 180 秒响应超时。外层期限不能保证 SDK 硬件故障时的严格总时限：R4 SDK UART TX 与生产 SAMD51 原生 cache 停止路径保留其原忙等待行为。不得为避免超时而跳过预检或读回。

部分 USB 响应或断线只视为失败，不在其后追加成功帧，不从任意 payload 内扫描 magic 重新同步。应关闭该流、重新核对板身份，再进行维护态只读对账。

## 目标范围

### UNO R4 WiFi

仅使用桥接模块 `bl_v2` namespace 的以下 10 个 owned BLOB keys，顺序固定：

```text
ws0 ws1 w00 w01 w10 w11 di0 di1 ac0 ac1
```

payload 为 `R4K1` 加 10 项，每项是 u8 present、u16 length、原始 `BRJ2` envelope。缺失项必须是 present=0、length=0。最大 1306 B。保留实际 key 缺失状态、tombstone、旧逻辑 ID、generation、CRC 和全部 raw payload，不做逻辑重编码。

读取先用公开 `modem.write` 确认 type 查询 RPC 成功，只有明确返回 `PT_INVALID` 才解释为缺失。RPC 失败、非 BLOB、尺寸或日志不合法、相同 generation 冲突均停止。不能用 `Preferences::isKey()` 的 false 代替缺失证据。`di0/di1` 最新有效项必须含合法 `BDN2` anchor；全空、无 namespace、无 anchor 或坏日志不会被创建或修复。

恢复前必须打开原有只读 namespace 并两次稳定读取。恢复仅对上述精确 keys 做必要的 `putBytes/remove`，每项写前对照原当前值、写后读回。不同 namespace 和未知 keys 不枚举、不修改。此快照不是完整桥接 NVS 备份，不调用 namespace clear。

`Preferences::begin()` 会使用既有 modem 初始化，可能重置连接模块的活动连接，但不刷连接模块固件、不清持久数据。维护期间不得另有 BLE/WiFi 客户端或产品并发操作。

### Wio Terminal

payload 恰为 8192 B：固定 4 MiB QSPI 的 `0x3fe000` 与 `0x3ff000` 两个 4096 B 扇区，包含全部 padding。先核对实际 JEDEC 容量与尾部地址。每银行只接受全 FF 空白或合法 `BWB2` image；至少一银行含合法 active `BDN2` anchor。未知、损坏、相同 generation 的不一致 image 或无 anchor 停止，不格式化空区，不认领外国扇区。

兼容原有 572 B、508 B、488 B CRC image。旧 image 比较采用与生产一致的逻辑 padding 归零语义，但导出和恢复保持 raw 8192 B，不修改旧格式 padding 或重新编码 generation。

恢复原备份的 active 银行在先，另一银行在后。仅当扇区内容不同才擦除、按页写回整个 4096 B，并全扇区读回核对；最后再次稳定读取完整 8192 B。地址和擦除范围固定，不提供任意地址、chip erase、format 或 RTL8720DN 固件指令。

`WioRawQspi.h` 只保留兼容文件名，不再复制原生 driver、寄存器、cache 或擦写循环。它通过便携的 `WioOwnedQspi.h` 复用生产 `WioTerminalQspiOwner` 与两个固定 region lease；原生实现和 Adafruit MIT 授权仅在生产 `WioTerminalQspi.inc`。使用公共 `BlinkerV2Advanced.h` 让 Arduino 正确发现库及其实现单元，不构造产品。

每笔 snapshot、比较或恢复共享一个 10 秒调用准入／迟到检查预算，两区域切换和逐页写只取得剩余预算，不重新计时。它不是 SDK 硬截止：原生 cache／AHB 和等待保留。正常事务结束释放自己的 lease，随后真实身份银行／任务视图才能接入；native false、迟到或提交后的读回不一致会锁定 owner 和维护 helper，后续不再读取、补写、cleanup、reset 或通过 end/begin 解锁。`yield()`期间的重入也不能释放另一 lease。

### Wio 可选固定 16 KiB 模式

默认 `BLINKER_INTERNAL_WIO_STORAGE_GATE_MODE=0` 保持上述 8 KiB `d/r` 合同及身份恢复顺序，不增加任务 workspace 或任务擦除权限。新增模式仅供内部准入维护，不成为用户 API：

| mode | 导出范围 | 恢复权限 | 必需配置 |
| --- | --- | --- | --- |
| 0（默认） | 身份 `[0x3fe000,0x400000)`，8192 B | 原身份两扇区 | 无新增配置 |
| 1 | 任务＋身份 `[0x3fc000,0x400000)`，16384 B | 无；`r`拒绝 | `BLINKER_INTERNAL_WIO_STORAGE_SOURCE_ID`，64 hex 字符串 |
| 2 | 同 mode 1 | 仅固定任务两扇区，身份绝不写 | source ID；`BLINKER_INTERNAL_WIO_TASK_REGION_RESERVED=1`；`BLINKER_INTERNAL_WIO_STORAGE_BASELINE_SHA256_HEX`，原精确16 KiB备份SHA256 |

模式1/2的前8192 B作为 raw导出，不解码任务、不归一化，外国／损坏／空白字节均可被保护；后8192 B仍通过原 BWB2／BDN2 anchor 严格校验。原区域出现布局失败时保留失败，不认领、格式化或创建新身份。读成功、全FF、region lease以及宏设为1都不能证明任务区物理独占；操作前还需明确原固件没有 Flash FS／SFUD旁路及整区独占依据。

模式2在现有双读稳定当前值、请求CRC及同实例预检之外，还要求 desired 全16384 B的SHA256精确匹配编译时原备份，且 desired/current 身份8192 B逐字节完全相同。只允许两个任务扇区地址，写前全扇区匹配 current，必要时逐256 B页恢复，并全扇区读回；不提供任意地址、整片擦除或身份区修复。若身份区与原备份不同，模式2拒绝，不顺带恢复身份。外层恢复锁及写意图文件仍必须保留。

任务 `clear()` 产生墓碑，不是恢复原全FF。完成试写后须按原raw备份精确恢复任务区，再独立两次 `d`，每次全16384 B等于原备份；不能以墓碑、逻辑任务为空或原8 KiB维护成功代替全区域恢复。实际试写前必须已准备这一受限恢复路径及授权。主 MCU Flash备份仍不能代替外部QSPI备份。

模式1/2额外提供无秘密元数据请求 ASCII `p`（无换行）。响应为 `BVP1`＋u16 LE payload length（固定63）＋u32 LE CRC32(payload)＋63 B payload：

| payload offset | 字段 |
| --- | --- |
| 0 | u8 mode |
| 1 | u32 LE布局预期容量4194304，不是本次实测容量 |
| 5 | u32 LE任务起始0x3fc000 |
| 9 | u32 LE身份起始0x3fe000 |
| 13 | u16 LE每区域8192 B |
| 15 | 16 B SAMD51 UID，每32位word按BE，hex顺序与Seeed 1.8.5 USB serial一致 |
| 31 | 32 B source ID |

`p`只读取主MCU UID，不打开QSPI。完整source ID应来自已冻结的源码输入摘要，并与匹配BIN编译／上传读回证据一起核对；它不是安全启动证明。native begin在`d/r`时验证JEDEC容量字节及4 MiB边界，目前不导出完整manufacturer/type，不能报告已测得完整三字节JEDEC。

主机每次按固定帧位置解析，不扫描magic、寻找首个设备、自动换端口或自动重发。必须先比对当前物理USB UID、mode、source ID和布局，再接受16384 B响应；CRC只检查传输完整性。raw串流和备份只写私有文件，不打印payload；原文件独立读回SHA并拒绝覆盖。缺板或缺独占依据时只完成软件门禁，不试写、不把模型代替整板断电。

## Wio主机协调与单笔探针

`tests/hardware/wio_storage_gate.py`只操作已核对并完成烧录的匹配夹具，不上传、reset或升级SDK。显式指定端口、实际USB UID与各夹具的冻结source ID；全零source不能操作硬件。`p`的预期容量不能替代`d`时native检查，编译tag也不能替代匹配BIN和上传证据。

先备份原主固件／授权并确认外部Flash使用情况，再用mode1采集原16 KiB。主机只将raw保存在库忽略的`build`私有目录，原文件`xb`创建、独立读回；不打印raw或凭据。例如下列占位参数必须全部替换并人工核对，不能直接照抄运行：

```text
python tests/hardware/wio_storage_gate.py capture --port <PORT> --uid <USB_UID> --source-id <CAPTURE_SOURCE_SHA> --private-dir build/private/wio-<UID>
```

使用原raw SHA编译匹配storage-only probe和mode2恢复夹具。probe不启动网络或TaskEngine、不自动创建身份，所有操作均须显式发送：`OPEN_CAPTURE`、`REGION_ATTEST`、`READ`、`REPLACE1`、`READ`、`IDENTITY_END`、`READ`、`CLEAR_FIXTURE`、`READ`。其中region需外部独占确认，replace／clear各限一次。真实四条停用644 B fixture只在USB维护使用，不能直接交给产品启动／Initialize。

主机每个UID只保留一个session claim。capture两读失败且尚未创建original／state／`.next`时，可在同一目录、同source、确认真实整板复电后重新执行仅读capture；不会删除claim或换目录。存在部分备份、state或`.next`时保留并停止，不能覆盖文件“恢复成功”。只有完整私有备份和state落盘后才seal claim、允许probe。

每条probe及raw恢复请求在发送前fsync意图；丢回执、尾随数据或部分请求保留unknown，不重发。复电后只读`reconcile`需要匹配mode1（或显式mode2），仅执行`p,d,d`并私有保存新鲜raw；它不能重新初始化probe或解锁已消耗的恢复意图。原raw精确相同才关闭session；不同且身份变动则停止，不修身份。不同但身份相同、从未发送raw恢复时，才可在独占确认下显式执行`restore-tasks`的一次受限恢复。恢复回执丢失后，即使复电也不能再发`r`，只读对账仍不相同则人工协调。

这些动作不产生自动硬件准入；实际存储耗时、真实断电及产品任务／ACK／State仍按主线独立验收。离线回归用`python -m unittest discover -s tests/hardware -p test_wio_storage_gate.py`，不导入pyserial、不打开USB。完整物理操作顺序取任务与Sketch执行计划§11.13。

现storage-only probe是单启动fixture：写入后冷启动时原完整baseline／全空预检不再满足，不能重发OPEN_CAPTURE／REGION_ATTEST证明TaskStore恢复。mode1／2冷后raw双读只是原字节协调，不等于真实TaskStore fresh load；不得替换原baseline绕过。只读冷恢复候选及产品恢复仍是主线待办。

## 失败处理

任何 `r` 失败都锁定本次启动后的后续 `r`。R4保留原维护合同；Wio若只是请求预检失败可在健康owner下做受限只读检查，但native false／迟到或不一致读回之后，`d`也拒绝且无新增QSPI调用，必须保留现场并等待真实整板复电后独立协调。部分写入失败不自动回滚、清空、再次写入或启动普通产品。全局恢复不是断电原子事务；原私有备份必须保留。损坏银行或日志会阻断导出与恢复，需要人工处理，不能把失败读为零值继续测试。软件reset不能作为外部Flash未知结果的恢复证据。

| code | 含义 |
| --- | --- |
| 1 | 未知命令 |
| 2 | 请求 header 或 payload 接收超时 |
| 3 | magic、长度或尾随字节不合法 |
| 4 | desired payload CRC 不匹配 |
| 5 | 当前存储读取或布局校验失败 |
| 6 | 第二次完整读取与第一次不同或失败 |
| 7 | 当前稳定快照 CRC 与 expectedCurrentCRC 不同 |
| 8 | desired/current 布局或同实例 anchor 不匹配 |
| 9 | 精确写入或逐项读回失败 |
| 10 | 最终快照与 desired 长度、CRC 或全字节不一致 |
| 11 | 本次启动已锁定恢复 |

夹具静态快照 RAM 为 R4 2612 B、Wio默认16384 B、Wio模式1/2为32768 B；R4 第二次快照另用有限1306 B栈，最大key scratch为280 B，Wio第二次读取／扇区对照按64 B栈缓冲比较。模式2SHA上下文也须计入维护栈，不把静态余量称为实际峰值。这些仅属于维护固件，不进入普通产品RAM。目标守卫只允许UNO R4 WiFi或Wio Terminal，不改变生产布局、SDK、声明profile或用户接口。
