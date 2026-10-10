# dev_edu 公开示例 API 使用清单

> 历史快照：由已删除的清单工具从 checkpoint `86052cf` 之前的
> `dev_edu examples/**/*.ino` 生成。旧例程已从标准 V2 库根移除；本表只用于迁移设计，
> 不把所有历史头文件符号自动升级为兼容承诺。

- Arduino Sketch：30
- Widget/Assistant 全局对象声明：20
- `Blinker.*` 调用：156

## 编译模式宏

| 宏 | 出现 Sketch 数 |
|---|---|
| `BLINKER_WIFI` | 28 |
| `BLINKER_WIDGET` | 19 |
| `BLINKER_BLE` | 1 |
| `BLINKER_BUTTON_PIN` | 1 |
| `BLINKER_WITHOUT_SSL` | 1 |
| `BLINKER_ALIGENIE_LIGHT` | 1 |
| `BLINKER_ALIGENIE_MULTI_OUTLET` | 1 |
| `BLINKER_ALIGENIE_OUTLET` | 1 |
| `BLINKER_ALIGENIE_SENSOR` | 1 |
| `BLINKER_DUEROS_LIGHT` | 1 |
| `BLINKER_DUEROS_MULTI_OUTLET` | 1 |
| `BLINKER_DUEROS_OUTLET` | 1 |
| `BLINKER_DUEROS_SENSOR` | 1 |
| `BLINKER_MIOT_LIGHT` | 1 |
| `BLINKER_MIOT_MULTI_OUTLET` | 1 |
| `BLINKER_MIOT_OUTLET` | 1 |
| `BLINKER_MIOT_SENSOR` | 1 |

## `Blinker` 全局门面调用

| 调用 | 次数 |
|---|---|
| `Blinker.begin()` | 29 |
| `Blinker.attachData()` | 29 |
| `Blinker.run()` | 29 |
| `Blinker.vibrate()` | 23 |
| `Blinker.print()` | 23 |
| `Blinker.delay()` | 4 |
| `Blinker.reset()` | 1 |
| `Blinker.notify()` | 1 |
| `Blinker.push()` | 1 |
| `Blinker.sms()` | 1 |
| `Blinker.second()` | 1 |
| `Blinker.minute()` | 1 |
| `Blinker.hour()` | 1 |
| `Blinker.wday()` | 1 |
| `Blinker.month()` | 1 |
| `Blinker.mday()` | 1 |
| `Blinker.year()` | 1 |
| `Blinker.yday()` | 1 |
| `Blinker.time()` | 1 |
| `Blinker.setTimezone()` | 1 |
| `Blinker.wechat()` | 1 |
| `Blinker.weather()` | 1 |
| `Blinker.weatherForecast()` | 1 |
| `Blinker.attachWeather()` | 1 |
| `Blinker.attachWeatherForecast()` | 1 |

## Widget/Assistant 对象声明

| 类型 | 声明数 |
|---|---|
| `BlinkerButton` | 6 |
| `BlinkerNumber` | 6 |
| `BlinkerRGB` | 4 |
| `BlinkerImage` | 1 |
| `BlinkerSlider` | 1 |
| `BlinkerText` | 1 |
| `BlinkerTab` | 1 |

## 高频链式方法

| 方法 | 次数 |
|---|---|
| `.print()` | 127 |
| `.powerState()` | 40 |
| `.stream()` | 29 |
| `.attachData()` | 29 |
| `.color()` | 27 |
| `.vibrate()` | 23 |
| `.icon()` | 16 |
| `.text()` | 16 |
| `.attach()` | 13 |
| `.brightness()` | 12 |
| `.mode()` | 11 |
| `.colorTemp()` | 11 |
| `.attachQuery()` | 11 |
| `.attachPowerState()` | 9 |
| `.debugAll()` | 8 |
| `.value()` | 6 |
| `.setBrightness()` | 6 |
| `.setPixelColor()` | 6 |
| `.show()` | 6 |
| `.temp()` | 6 |
| `.humi()` | 6 |
| `.pm25()` | 6 |
| `.time()` | 6 |
| `.unit()` | 5 |
| `.co2()` | 4 |
| `.delay()` | 4 |
| `.println()` | 4 |
| `.toInt()` | 3 |
| `.attachColor()` | 3 |
| `.attachMode()` | 3 |
| `.attachBrightness()` | 3 |
| `.attachColorTemperature()` | 3 |
| `.attachCancelMode()` | 2 |
| `.attachRelativeBrightness()` | 2 |
| `.attachRelativeColorTemperature()` | 2 |
| `.report()` | 2 |
| `.aqi()` | 2 |
| `.pm10()` | 2 |
| `.state()` | 2 |
| `.reset()` | 1 |
| `.attachLongPressStop()` | 1 |
| `.tick()` | 1 |
| `.notify()` | 1 |
| `.push()` | 1 |
| `.sms()` | 1 |
| `.tab()` | 1 |
| `.second()` | 1 |
| `.minute()` | 1 |
| `.hour()` | 1 |
| `.wday()` | 1 |
| `.month()` | 1 |
| `.mday()` | 1 |
| `.year()` | 1 |
| `.yday()` | 1 |
| `.setTimezone()` | 1 |
| `.wechat()` | 1 |
| `.weather()` | 1 |
| `.weatherForecast()` | 1 |
| `.attachWeather()` | 1 |
| `.attachWeatherForecast()` | 1 |

## 可归属到声明对象的直接调用

| 对象类型 | 调用 | 次数 |
|---|---|---|
| `BlinkerButton` | `.icon()` | 10 |
| `BlinkerButton` | `.attach()` | 5 |
| `BlinkerNumber` | `.icon()` | 5 |
| `BlinkerRGB` | `.attach()` | 4 |
| `BlinkerButton` | `.color()` | 2 |
| `BlinkerButton` | `.print()` | 2 |
| `BlinkerButton` | `.text()` | 2 |
| `BlinkerImage` | `.attach()` | 1 |
| `BlinkerImage` | `.print()` | 1 |
| `BlinkerNumber` | `.print()` | 1 |
| `BlinkerRGB` | `.brightness()` | 1 |
| `BlinkerSlider` | `.attach()` | 1 |
| `BlinkerSlider` | `.color()` | 1 |
| `BlinkerTab` | `.attach()` | 1 |
| `BlinkerTab` | `.print()` | 1 |
| `BlinkerTab` | `.tab()` | 1 |
| `BlinkerText` | `.text()` | 1 |

## 使用边界

- 链式调用统计包含 `Serial.*`、底层对象和 Widget；它用于识别语法习惯，
  不能单独证明某个方法属于 Blinker Widget。
- 直接调用只统计 `variable.method()`；跨行 fluent continuation 会进入上一张
  总表，但不会被错误归属到某个 Widget 类型。
- 新 Device V2 API 的兼容范围必须再经过语义审查，不能由出现次数自动决定。
