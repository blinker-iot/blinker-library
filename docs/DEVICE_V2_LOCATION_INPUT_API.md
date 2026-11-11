# 设备定位观测输入

定位硬件、串口协议及解析由用户实现。Blinker只接受一份WGS84观测并复用现有私有Cloud租约上传；没有GPS库依赖。
本接口属于当前源码的可选功能，不代表已冻结的R5旧ZIP已有该API。首版上传范围仍是已准入的ESP32 Cloud路径。

## 最小接入

```cpp
#include <Blinker.h>

BLINKER_PROPERTY(power, bool, blinker::readWrite());
BLINKER_LOCATION(position);

void setup() {
    Blinker.begin("YOUR_DEVICE_KEY", "YOUR_WIFI_SSID", "YOUR_WIFI_PASSWORD",
                  power, position);
}

// 由你的GPS/其他设备解析器在得到新观测时调用；此处不实现驱动。
bool acceptPosition(double latitude, double longitude, uint32_t sampledAtMillis) {
    return Blinker.location(position, latitude, longitude, sampledAtMillis);
}

void loop() {
    // 在此推进你的非阻塞解析器，并按新样本调用acceptPosition。
    Blinker.run();
}
```

纬度在前、经度在后，单位是度；不要与地图里longitude／latitude字段顺序混淆。
整数E7转换前先校验有限值与范围，按最近E7取整。便捷输入不猜精度或UTC，两者保持未知。
`Blinker.begin`必须包含`position`并成功绑定，单个Client目前只支持一个私有定位字段。
自定义稳定键可用`BLINKER_LOCATION_KEY(position, "myPosition")`，不会注册全局隐藏字段。

## 四种输入与两个本地状态操作

| 调用 | 含义 |
| --- | --- |
| `Blinker.location(position, latitude, longitude)` | 这次调用就是新观测的采样时刻 |
| `Blinker.location(position, latitude, longitude, sampledAtMillis)` | 明确提供样本原始本地tick，保留解析／排队延迟 |
| `Blinker.location(position, fix)` | 新观测，使用`blinker::LocationFix`完整元数据 |
| `Blinker.location(position, fix, sampledAtMillis)` | 完整元数据和真实样本年龄 |
| `Blinker.locationLost(position)` | 停止把上个fix用于新采样；保留最后一个本地点与顺序 |
| `Blinker.clearLocation(position)` | 清本地观测与顺序；用于显式清理或无法测量的休眠／时基重置后 |

bool为true只表示本地校验／快照操作成功，不是上传、ACK、云端保存或地图刷新确认。
失败原因可读`Blinker.lastError()`／`lastErrorText()`；非法值不会覆盖原有效点。
无效／重复／乱序点不是新的fix；驱动确定失锁应显式调用`locationLost`，不能反复提交无效值并期待它清空旧点。

`sampledAtMillis`必须与本设备`millis()`／产品绑定时钟同boot、同时间基准；不是GPS UTC、外部模块tick或跨重启的持久时间。
无tick重载仅供刚取得的新观测，不要在每次`loop()`用旧坐标重新盖当前时间。默认最大样本年龄30秒，采样间隔下限1秒；
实际是否上传及频率仍受授权租约控制，输入函数本身不发包。更新／读取间隔均须小于2^31毫秒，无法测量的睡眠或时基重置后先清本地快照。

## 可选精度与UTC

```cpp
bool acceptDetailedPosition(int32_t latitudeE7, int32_t longitudeE7,
                           uint32_t accuracyCm, uint64_t gnssUtcMillis,
                           uint32_t sampledAtMillis) {
    blinker::LocationFix fix;
    fix.valid = true;
    fix.latitudeE7 = latitudeE7;
    fix.longitudeE7 = longitudeE7;
    fix.hasHorizontalAccuracy = true;
    fix.horizontalAccuracyCm = accuracyCm;
    fix.timeQuality = blinker::LocationTimeQuality::GnssUtc;
    fix.capturedAtUtcMillis = gnssUtcMillis;
    return Blinker.location(position, fix, sampledAtMillis);
}
```

只有来源确实给出精度／UTC时才填对应标记；未知保持默认false／Unknown和0，不能假填精度0或把调用时间当UTC。
GNSS UTC只是观测元数据，不会自动设置业务时间、系统时间或凭据时间。
同一快照中的纬经度／精度／时间作为一个现有codec对象采样，不拆成独立State属性。

## 支持与责任边界

- 用户负责模块／来源真实性及坐标转换，传入WGS84；不把GCJ02、BD09、IP城市中心或手动安装位置冒充设备GNSS观测。
- 保留现`device-gnss-v1`协议及来源语义，不证明卫星精度，也不因有数据接口自动开放其他定位来源。
- Location／Spec对象应与Sketch／Device同生命周期；输入和`Blinker.run()`串行执行，不在IRQ／其他任务并发调用。
- `locationLost`和`clearLocation`不删除云端最后位置；旧投影按原新鲜度规则变为上次位置，不发送假0,0或null点。
- 字段键、顺序、Manifest改变仍需匹配服务端精确定位绑定及owner权限；声明API并非自动授权。BLE／LAN／其他平台位置上传不在本阶段开放。
- 当前源码已通过[§19](DEVICE_V2_DEVICE_LOCATION_IMPLEMENTATION_PLAN.md#19-loc-5b-隔离安装与原sdk单目标完整链接2026-10-29有限收口)的内部ZIP隔离安装及原ESP32 SDK3.3.11完整链接，未对外发布、部署或操作设备。驱动／精度由用户集成验证。
- 可编译参考见`tests/arduino/CompileLocationInput`；其中六个volatile函数指针只用于维护者链接验收，用户接入不需要这些锚点，也不会由探针产生观测。
