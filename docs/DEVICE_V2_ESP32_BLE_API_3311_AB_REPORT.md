# Arduino-ESP32 3.3.11 BLE API 同板 A/B 报告

日期：2026-09-03
目标：确认 ESP32-S3 的 `ipc0=76 B` 风险是否已由 Arduino-ESP32 3.3.11 修复，并区分 Core 自带 BLE API、
外部 NimBLE-Arduino、直接 ESP-IDF NimBLE C API 与 Blinker Port 的责任边界。

## 1. 固定环境

- 开发板：XIAO ESP32-S3，revision v0.2，8 MB PSRAM；
- 同一设备 MAC：`98:3d:ae:ee:7c:6c`；
- 串口：COM12；
- Arduino-ESP32：3.3.11；
- ESP-IDF：v5.5.5；
- `CONFIG_BT_NIMBLE_ENABLED=y`；
- `CONFIG_ESP_IPC_TASK_STACK_SIZE=2048`；
- 外部对照库：NimBLE-Arduino 2.5.0；
- 每轮只普通烧录，不擦除 NVS；所有结果均来自复位后的首次 BLE 初始化。

## 2. 结果

| 路径 | 首次初始化 | GATT/广播 | `ipc0` 启动→最低 | Flash | 静态 RAM |
|---|---:|---:|---:|---:|---:|
| NimBLE-Arduino 2.5.0 | 通过 | 通过 | 1280→1104 B | 525,576 B | 33,284 B |
| Arduino Core `BLEDevice` | 通过 | 通过 | 1280→1104 B | 560,252 B | 28,488 B |
| 裸 `nimble_port_init()`，无 Arduino 保留标记 | watchdog | 未进入 Host | 无有效终值 | 不可比较¹ | 不可比较¹ |
| 裸 C API + `esp32-hal-alloc-ble-mem.h` | 通过 | Host 启动 | 1284→1108 B | 不可比较¹ | 不可比较¹ |

¹ 直接 C API 的既有基线探针还包含可由串口启动的完整 WiFi 路径，不能与两个 BLE-only probe 比较固件大小。

动态水位补充：

- NimBLE-Arduino settled free/min heap：约 255,128/255,040 B；
- Core `BLEDevice` settled free/min heap：约 249,168/249,080 B；
- 两者 `btController` 最低剩余约 2,752～2,768 B；
- 两者均未复现 3.3.6 的 `ipc0=76 B`。

这些数字只用于最小 API A/B；不能外推为 Blinker WiFiProv+Direct 或 Edge Hub 的产品峰值。

补齐平台保留合同后，公开 `WiFiBleQuickStart` 已在相同 3.3.11 Core 完整编译通过：Flash 1,279,756 B，
静态 RAM 55,492 B。

## 3. 根因纠正

Arduino-ESP32 3.3.10/3.3.11 在 `initArduino()` 中检查 BLE 库使用标记。若应用没有声明 BLE 正在使用，Core 会
调用 `btMemRelease(BT_MODE_BLE)`，提前释放约 36 KB BLE 内存。

Core 自带 `BLEDevice` 和 NimBLE-Arduino 都会间接包含 `esp32-hal-alloc-ble-mem.h`。该头文件的启动期构造器把
`_bleLibraryInUse` 设为 true，使 BLE 内存在 `setup()` 前得到保留。Blinker 的直接 ESP-IDF C API Port 和旧
`Esp32SdkTaskStackBaseline` 没有接入这个 Arduino 合同，所以旧探针是在已释放内存上调用
`nimble_port_init()`；其 TLSF/heap/watchdog 栈不能证明 controller 有回归。

同一个直接 C API 探针加入官方保留标记后立即通过，闭合了上述因果链。3.3.10 的旧失败证据也必须作废；该版
仍只有 1024 B IPC stack，若要重新评价还需按正确合同重测。

## 4. 实施结论

1. Arduino-ESP32 3.3.11 恢复为 ESP32 BLE/WiFi+BLE/Edge Hub 的正式候选，但尚未因最小探针通过而成为
   production profile。
2. 不需要引入外部 NimBLE-Arduino，也不需要 fork NimBLE/Controller。外部库只作为独立 A/B 证据保留。
3. 当前共享 `Esp32NimBleRuntime` 已在 ESP32 Port 内条件包含 Arduino 官方 BLE 内存保留头；这属于平台启动
   合同，不得泄漏到 Product、BBP/2 或用户 Sketch。
4. Core 自带 `BLEDevice` 在 ESP32-S3/3.3.11 上已经是 NimBLE backend，技术上可以作为 Port 实现候选；但迁移
   不是本次故障的必要修复。必须先比较 peripheral+central 同时角色、WiFiProv 交接、原位 advertising 更新、
   fixed queue/callback 边界以及完整 Product Flash/RAM，不能只凭最小示例决定重写。
5. 内存保留合同和公开图编译门禁已完成；下一顺序是 3.3.11 真机 WiFiProv→Direct smoke → resource gate →
   10→30→100 循环/RF。SDK `BLEDevice` Port 迁移作为独立简化评估，不与修复混做。

## 5. 可复现探针

- `tests/internal/arduino/Esp32NimbleArduinoAdmissionProbe`
- `tests/internal/arduino/Esp32SdkBleAdmissionProbe`
- `tests/internal/arduino/Esp32SdkTaskStackBaseline`
