# BLE→独立Cloud注册 v2：根、发行及未决生命周期合同

更新：2026-11-04。内部发行／近期认证读取／安装确认／独立激活及两卡删除闭合，061与062已在隔离实库验证；App／无线交接为内部候选，BC-5 冷预检及现有阻点见[执行计划 §18](DEVICE_V2_BLE_TO_CLOUD_REGISTRATION_PLAN.md#18-bc-5-冷预检构建准入与内部同-intent-入口2026-11-04)。不开放 HTTP、不部署业务服务、不提交推送。

## 1 不改变的产品与信任边界

- 同一物理实例的BLE Direct和Cloud各有卡、逻辑ID及额度。WiFiProv BLE／SoftAP只是Cloud配网通道；不以MAC或一次扫描合并身份，不把Key补回源BLE ID。
- 原`access-registration-v1`、060约束、正数Cloud版本、确认摘要及Cloud→BLE行为不变。新方向不传0／null伪装旧`cloudCredentialVersion`，也不恢复旧enable-cloud入口。
- 服务端核当前owner／tenant、可信BLE源产品、原实例关联和Admin上下文。源逻辑卡没有Cloud Key不证明MCU物理无Key或TaskStore为空；设备实际写入仍须BC-2真实认证及原Store／owner／CAS／Unbound晚检。
- 新合同不使服务端获得Admin secret，也不把客户端布尔值当作硬件证明。后续安装确认须明确“App验证原认证通道回执”的信任模型；若要独立服务端验硬件证明，另定证明能力，不能借一个digest宣称已实现。

## 2 严格方向化根

内部prepare只接受下列精确形状，未知字段拒绝：

```json
{
  "contract": "access-registration-cloud-v2",
  "requestId": "<nonzero-16B-canonical-base64url>",
  "sourceLogicalDeviceId": "<owned-BLE-ID>",
  "targetProduct": "cloud",
  "name": "新Cloud设备",
  "expectedRoot": {
    "kind": "ble-no-cloud-key-v2",
    "deviceInstanceId": "<16B>",
    "accessEpoch": 1,
    "controllerId": "<16B>",
    "credentialVersion": 1,
    "adminFingerprint": "<32B>",
    "cloudKey": { "state": "absent" }
  }
}
```

epoch及Admin版本为正uint32；所有字节串须精确长度、非零、canonical base64url。`cloudKey`是独立判别形状，不是可空的旧字段。源须可信`ble_direct`且当前无Cloud Key／会话历史，当前Admin完整选择器匹配、原实例关联成立；既有共享根或未决根恢复／已签grant不能绕过。所有权隔离不接受body指定owner或tenant。

服务端分配独立Cloud目标ID，不接受客户端指定目标、不以源ID或MAC推导目标。一个源已有live准备／发行时拒绝另起；相同owner＋requestId先核精确请求摘要，再决定是否新占额。字段变化409，重放不延长TTL、不生成新ID或nonce。已退役／旧UNKNOWN不自动迁入新方向。

## 3 intent、目标与Key的不可变绑定

intent为requestId的同一16 B opaque值。准备行冻结owner、tenant、源ID、目标ID、完整expectedRoot、name、clientNonce及请求摘要；clientNonce由服务端随机生成，供后续显式Method2用途，不由App覆盖。

发行只新增目标`credentialVersion=1`、locator16及issuanceDigest32。摘要独立域绑定owner／tenant、intent、源／目标ID、根、clientNonce和目标Key版本／locator；name由不可变准备摘要及账本另行绑定。设备BC-2的另一个HMAC证明同时绑定此intent及候选Key原字节，两者用途不同。

两服务端摘要均为下列数组的标准`JSON.stringify`、无BOM UTF-8、SHA-256原始32 B；公开投影以canonical base64url编码。不复用v1域或旧rootTuple：

```text
rootTuple = [kind, deviceInstanceId, accessEpoch, controllerId,
             credentialVersion, adminFingerprint, "absent"]
prepare = ["blinker.access-registration-cloud.prepare.v2", owner, tenant,
           "access-registration-cloud-v2", requestId, sourceId, "cloud", name, rootTuple]
issuance = ["blinker.access-registration-cloud.issuance.v2", owner, tenant,
            intentId, sourceId, targetId, rootTuple, clientNonce, 1, locator]
```

复用原非零32 B Key生成、locator和真实vault AES-256-GCM。staging密文AAD使用原`purpose=device-key`、**目标**ID／版本／locator，并以issuanceDigest作为额外binding。不在BLE源ID下封装，不默认创建另一KEK，不记录原始Key或Admin secret。发行里程碑与密文、目标、摘要在同一事务提交，才可向调用方返回元数据；原Key在finally清零。

原`v2_device_keys`读取器没有额外binding，因此staging密文不能原封复制进正式Key表。BC-3b激活须在同一事务验证原发行行、按staging AAD解密，再按原正式目标AAD重新封装并清零；不能为方便而放宽vault解密。

## 4 额度、到期与删除

| 状态 | 时间／取消／源删除 | 额度和材料 |
| --- | --- | --- |
| prepared，未发行 | 120秒后可关闭；明确取消或源删除可关闭 | 关闭后释放独立目标预留；没有Key材料 |
| key_issued | TTL只结束新写窗口，不释放；取消拒绝释放 | 保留原目标、密文、nonce、摘要和预留 |
| install_unknown | 不重新发行／安装；仅RESULT_UNKNOWN可按§7提交真实确认 | 同样保留，不以超时、404或源删除推断失败 |
| committed | 目标正常独立设备生命周期，不依赖源仍存在 | 原目标卡计量，预留转换不重复扣额 |
| 目标正常删除 | 沿原owner删除、Broker撤权及墓碑 | 不删另一卡或物理根；不从旧intent复活目标 |

统一额度仍是非deleted逻辑卡＋未转换的有效目标预留，按目标ID `UNION` 去重。新ledger的prepared仅在未过期时计量；key_issued／install_unknown不看TTL，源或账号删除也不偷偷释放它们。超额只阻新分配，不重写旧授权或UNKNOWN。

源删除前仅prepared可cancel；已发行则标未决并保留发行行及原BLE材料引用。服务端删除不是设备物理撤权确认。尚未物化的目标不是可伪造的active卡，不能用普通DELETE404释放其未决预留；后续须真实协调／安全撤权合同才能收尾。账号删除阻止后续发行／激活，保留已发行审计和密文，不复活旧账号或意图。

## 5 BC-3a内部接线与锁序

新增061 staging ledger，不改已发布060，不在无Key准备阶段伪造060所需正数Cloud根，也不提前插入active目标／正式Key／任务scope。ledger不是第二套设备执行链或vault；它记录尚未转换的同一次原注册操作。

内部仅提供prepare、元数据read、密文issue、markUnknown及cancel。metadata永不包含原Key、密文、secret或认证proof；issue精确重放只返回同一发行元数据，不重新随机Key。没有明文Key HTTP或普通登录绕近期认证的入口。

复用原SERIALIZABLE、user→tenant锁序，再锁原intent／可信source及Admin；根消费者在同user／tenant范围协调，不在事务内做Broker、设备或网络调用。原有限SQL序列化重试不等于业务请求或设备配置重发。Key generation只可能在数据库事务回滚后重算；已提交发行行始终优先识别，不能换已存在材料。

生命周期薄口共同约束原quota、根／Presence变更、BLE恢复、源删除和账号删除。即使源尚无060共享root/member也须检查新ledger，不能因root=null漏掉已发行引用。原BLE writer及Presence材料保留，不根据新目标或幸存卡自动转移writer。

### 5.1 BC-3b内部秘密借用的最小合同

内部`withIssuedKey({token,region,intentId}, consume, {signal}?)`只接受原真实身份解析器给出的owner／tenant，同一token及区域再过原近期认证；不接受body owner／tenant、自称verified、普通登录或旧测试放宽开关。metadata仓库仍可仅注入原encrypt接口；未配置秘密读取依赖时关闭，不自动创建KEK或借其它连接。

原recentAuth新增一个调用期的`withVerified`闭包：保留Redis实际到期时间、当前注销代际和本地时钟单调校验；DB等待后，在解密及实际借用前同步晚检，不在事务内再次访问Redis。结束即失效，不保存或转交长期许可；不会从核验完成时重新算5分钟。跨进程注销仍受原Redis CHECK的既有线性化边界限制，不宣称Auth与PostgreSQL具有跨存储原子撤权。

私有事务口沿user→tenant→intent→source／Admin重新核授权、根和原窗口，只允许`key_issued`且没有关闭原因。`prepared`／UNKNOWN／终态或已过原120秒窗口均不能取得秘密；已发行过期仍保留额度和原材料。源删除／根改变按原coordinate持久转UNKNOWN后再拒绝，不因异常回滚掉保守协调。秘密消费者使用SERIALIZABLE但`maxRetries=0`，不能在COMMIT失败后自动再次执行callback。

借用原发行Key前，重新核完整issuanceDigest、staging额外AAD及原locator。明文仅作为32B Buffer同步借给受信任内部consumer；不返回字符串、base64 Key或密文。consumer须同步返回undefined，不允许异步网络／数据库操作；所有路径finally清零所借Buffer。该合同不是任意代码沙箱，不保证consumer自行复制的秘密或JS／GC内部副本已清零。

返回只有非秘密metadata；callback完成不是设备已安装或持久ACK。通过借用边界后，取消／认证到期不倒写已发生结果；consumer异常及事务提交后结果未知仍按原事实交给调用方协调，不自动重试、不换Key。后续无线写前仍须BC-2及BC-4全部门禁，不能把这个内部方法当作网络安装许可。

## 6 调用方及无线交接仍后置

1. 内部读取合同见§5.1；后续接HTTP／App前仍须和安装确认、激活及删除闭合，不提供可单独绕过的秘密端点。refresh、`/me`或普通已登录不替代近期认证。
2. 内部安装确认／方向化摘要、原根CAS及原子提交见§7；实际App回执消费者仍待BC-4，不用Broker成功或普通布尔值冒充设备持久确认。
3. 内部060根物化、原BLE materialOwner、writer及独立删除已闭合；实际调用方须复用这些结果，不复制Presence或自行转移writer。
4. BC-4再接App显式用途、PreCloudDirect与原WiFiProv交接，取消后退休原授权，不携带旧会话能力跨radio owner。真实安装ACK、官方配网回执、Cloud新鲜上线分别判定。

061数据库门已由独立测试库补齐，具体5项和隔离边界见本批结果；内部读取的离线carrier不代签该新增消费者的真实数据库并发、实际Auth或无线效果。后续数据库测试只用显式隔离URL，不连接业务库；私有golden授权HOLD不变。

## 7 BC-3b安装确认、幂等及独立激活合同

### 7.1 信任模型与精确输入

本批仅新增内部commit，不挂HTTP、不部署。owner／tenant仍由受信任的服务身份层给出，不接受未知调用方以body指定所有权。下面的receipt表示**App已经核验同一Method2认证通道的精确成功ACK**，不是服务端已独立验过硬件签名；通用成功布尔值、Broker收讫、WiFiProv结果或Cloud上线均不能替代它。BC-4尚须实现实际认证、回执产生和App验证消费者，本批合成向量不代签该链路。

```json
{
  "contract": "access-registration-cloud-v2",
  "commitId": "<nonzero-16B-base64url>",
  "clientNonce": "<original-issued-clientNonce-16B>",
  "receipt": {
    "kind": "app-verified-initial-cloud-key-v1",
    "authMethod": 2,
    "suite": 1,
    "sessionBinding": "<committed-Method2-session-16B>",
    "deviceNonce": "<same-Method2-device-nonce-16B>",
    "result": "installed",
    "credentialVersion": 1,
    "locator": "<original-issued-target-locator-16B>",
    "issuanceDigest": "<original-issued-digest-32B>",
    "ackDigest": "<SHA256-of-exact-authenticated-success-ACK-bytes-32B>"
  },
  "confirmationDigest": "<confirmation-digest-32B>"
}
```

所有层级仅接受列出的精确键，字节串沿原canonical／非零检查；`unknown`、失败、仅在线或其它认证method／suite均拒绝。clientNonce同时是原发行nonce与本次Method2的client nonce，不能另起不关联的nonce；sessionBinding与deviceNonce须由同一已提交会话的App适配器提供。locator／issuanceDigest必须等原ledger。摘要仅用于关联和幂等，不把公开digest称为所有权证明。

### 7.2 摘要与UNKNOWN协调

沿原UTF-8／`JSON.stringify`数组／SHA256规则，两个新域严格分离：

```text
receiptTuple = [kind, authMethod, suite, sessionBinding, deviceNonce,
                result, credentialVersion, locator, issuanceDigest, ackDigest]
confirmation = ["blinker.access-registration-cloud.confirmation.v2",
                owner, tenant, intentId, sourceId, targetId, rootTuple,
                clientNonce, 1, locator, issuanceDigest, receiptTuple]
commit = ["blinker.access-registration-cloud.commit.v2", owner, tenant,
          intentId, "access-registration-cloud-v2", commitId,
          clientNonce, confirmationDigest]
```

每次commit包括精确重放，都先以不可变ledger和receipt重新计算confirmation，不能只比一个调用者自称的digest。已committed同commitId及同摘要返回原目标状态；改变任一会话／ACK／Key／nonce／commit字段均409或格式400，零新激活。精确重放优先于当前源状态和原TTL，所以删源后不会否定曾提交的事实；目标已删则只返回墓碑，不复活、不另建Key。

新确认只接受`key_issued`且无关闭原因，或`install_unknown`且原因精确`RESULT_UNKNOWN`。仍须重新核活owner／tenant、源BLE／Admin／实例关联及全部CAS；源删除、账号删除、根变化及其它UNKNOWN原因均不接受确认。原120秒只结束新的Key借用／安装窗口，不能据此否定已经完成的安装，因此最终确认可晚到，不重新发Key或重放设备写。UNKNOWN的旧原因不能被先洗成RESULT_UNKNOWN再确认；成功提交保留原RESULT_UNKNOWN作审计。

### 7.3 一个事务的正式目标与材料归属

062增量扩展061的committed状态、六个确认字段和不变guard；060／061历史文件保持原文。060根的Admin版本约束仅由062扩到正uint32，旧v1解析仍要求版本1，不把实际版本冒填1。

一个原SERIALIZABLE事务取得user→tenant→intent及真实源／Admin保护后，创建独立Cloud目标、以原staging AAD解密并按正式目标AAD重封Key、复制非秘密Direct根元数据、物化060根及两产品成员，最后持久commit摘要。任意失败全部回滚；秘密在同步重封后清零，不把明文跨DB等待，不复制staging密文到正式Key表。原BLE卡仍无Cloud Key／会话历史，TaskStore和任务scope不迁移，原Presence不复制。该事务只有数据库动作，有限SQL序列化重试不代表设备重写；COMMIT后丢响应只读原intent协调，不再发行／安装。

已committed退出pending围栏和预留计量，转由active独立Cloud目标计量，目标ID沿原UNION去重。v2committed ledger固定`materialOwner=原BLE source`，不按instance或幸存卡选writer：BLE活卡才可沿原Presence写链，Cloud只读；删BLE后Cloud仍可读已安装材料但无writer；删Cloud后BLE仍可读写原材料。任何不完整／多重／不一致的方向关联均拒绝，缺表不是旧方向许可。

两卡正常删除只退役本卡、撤其ACL并沿原Broker／分享／任务生命周期，不删除另一卡或把物理根恢复到安装前。最后一张卡删除后保留必要密文和不可变确认审计，但没有活引用或读写权限；物理擦除与GC另议。本批不靠回滚NVS、重放UNKNOWN或伪装远端撤权完成来清账。

reverse删除准入仍须在原锁下晚检完整root／bindingRevision及当前source Admin，不凭历史committed的字段形状或已知hash代替CAS。已激活后的新BLE根恢复仍受060活成员围栏，退出pending不等于可改共享Admin；已完成精确重放和原活BLE的Presence轮换不因此被永久禁止。

## 8 BC-4专用安装报文与实际回执

本节为内部候选，不新增公开能力位，不开放普通 Direct、Cloud/Broker、旧 `Install` 或 `Bootstrap` 入口。接收者必须由显式 PreCloudDirect owner 独占，复用原真实 Method 2 提交、PlatformAccess Admin/All 权限、同物理连接及 BC-2 一次性 HMAC。当前 owner 不允许普通控制、Presence 写入或 Task/Cloud 并行启动。不能用 `RxContext` 标志、普通成功布尔值或合成 `AuthResult` 冒充该授权。

| 报文 | Kind／flags | Canonical CBOR body | Body／完整帧 |
| --- | --- | --- | --- |
| 安装请求 | `0x38`／`0` | `[1, intent:bstr16, rawKey:bstr32, proof:bstr32]` | 87／97 B |
| 已安装回执 | `0x39`／`IsResponse` | `[1, intent:bstr16, locator:bstr16]` | 36／46 B |

两报文严格 BBP/2 v2、10 B header、非零 sequence；回执沿请求 sequence。CBOR 仅最短编码、确定长度、精确字段数、非零字节串，不接受尾随字节。安装请求不使用 `AckRequired` 或可靠队列重发。proof 沿原 BC-2 186 B transcript，绑定实际 Method 2 clientNonce（等于服务发行的 nonce）、deviceNonce、sessionBinding、源根、intent 和候选 Key。DeviceKey、proof 及解密暂存不进入日志或回执。

设备写入前确认同一会话的回执容量，预编码 intent 与从候选 Key 实际派生的 locator；安装仍由原 owner、真实 Unbound、原 Admin CAS 和持久写后读回决定。只有原安装返回成功，才向原加密会话发送该回执。排入发送队列不等于 App 已收到。写后丢连接／发送失败不得覆写设备已有的持久结果，也不得回退明文、用通用 ACK 判成功或自动重发。

App 只接受从同一 Direct AEAD 会话成功解封、严格核 flags／header／sequence／intent／locator 的回执。`ackDigest` 是收到的**精确 CBOR body 36 B** 的 SHA256，不是重编码、密文或完整帧。设备不接收或声称核过 `issuanceDigest`：App 从本次冻结的发行记录取其原值，核对应的 `targetKey`、owner/tenant 和源/目标，再生成§7 `receipt` 与 `confirmationDigest`。安装请求一旦尝试发送，未拿到精确回执就保留 UNKNOWN；取消或超时不是未写入证据。

后续交接的必要条件分开验证：App 完成原 intent 的确认（或只读协调取得同一已提交目标），然后退休原会话；设备必须验证原生 BLE 停止及反初始化成功后才允许 WiFiProv 获得 owner。App GATT `close()`、广告消失、普通 `Stopped` 枚举、等待固定时间都不能代替设备原生停止证明。设备持久 ACK、WiFiProv 回执及新鲜 Cloud 可用状态分别判定。

合成互操作向量（不是私有 golden 或硬件证据）：intent=`01×16`、instance=`02×16`、controller=`03×16`、epoch=7、Admin version=3、clientNonce=`05×16`、Key=`06×32`、sessionBinding=`08×16`、deviceNonce=`09×16`、Admin secret=`0b×32`、Method=2、suite=1、domain=2、permissions=15。

```text
proof      ac3b3692ad77d0e4695d85eeb780a70bbdfdf729a665d698b7972dd3400fbbb7
locator    063f21c532be08683280eabbf47efd90
ACK body   8301500101010101010101010101010101010150063f21c532be08683280eabbf47efd90
ackDigest  3c793f77c866db148aebd3ea6312934f9950ed67fe223e3ca8546a98db73b63c
```

owner=`synthetic-owner`、tenant=`synthetic-tenant`、source=`synthetic-ble`、target=`synthetic-cloud`、fingerprint=`04×32`（只用于摘要向量，不模拟真实 Admin 验证）时：`issuanceDigest=psj8flDI6tNxgYi2eiPZSlxLKTD2GXZG7sJQxpYzG4s`，`confirmationDigest=zE-YlbU7osjCYxhx0t6OI304mS7sTlkB6UKj6Nim1qU`（Base64URL）。

### 8.1 同意图显式交接（仅内部候选）

`InitialCloudKeyHandoff` (`0x3A`) 是 10 B 基础头、flags 0、非零且不同于安装请求的 sequence，body 是 36 B canonical CBOR `[1, intent:bstr16, locator:bstr16]`（完整 46 B）。两个字节串均非全零。没有协议响应、自动重试或 WiFi 凭据字段。

App 用不可由外部更改的精确安装回执向同 owner／tenant 的权威确认端口提交原 intent；只接受对应 active Cloud 目标、原 BLE 材料所有者、原根、发行 nonce 和 Key 元数据的 committed 投影。普通布尔值、云在线、任意目标读取或超期结果不得授予发送权。核验后仅在原有效 Method 2／AEAD 链路发送一次，再退休 GATT；取消或原窗口结束即撤销等待，迟到响应不得写。可能已提交而发送／关闭不明时保留 UNKNOWN，不重放。当前端口不挂 HTTP 或页面。

设备只有在安装成功且精确加密 ACK 已入队时，才允许原 transport／session／binding、未过期原 Method 2 和当前 Admin 提交完全一致的 intent／locator。它验证的是原认证 App 的交接声明，不是设备验证服务端签名。进入过持久化但失败或 ACK 未入队，原候选 owner 退休；更换会话不会恢复写权限。尚未授权时，断开、重启或仅持有 Key 都不能获得交接。授权后，本地元组可被原主管消费一次，显式取消清除；该元组本身不是允许启用 WiFiProv 的凭证。

后续实际接线还必须分别证明原 GATT 释放、设备原生 Host 释放、新 owner 用途及凭据新鲜性，并在 WiFi 接纳前复核原时限、持久 instance／Admin 根和 Key locator；任一未知停在只读协调。不得以 `CRED_RECV` 作为写前检查：原 ESP32 SDK 先应用 STA 凭据、再发该事件。未具备写前窄门禁时，不开放任何新 WiFi 下发路径。

### 8.2 候选写前接纳：保留 WiFiProv wire，不启动 stock manager

候选只接原 SDK 的 `proto-ver`、Security1 `prov-session` 和官方 `prov-config` codec。第一份有效 Set 暂存 SSID／口令并固定原生 Security1 session；此后拒绝重新握手和再 Set。仅该 session 的第一次 Apply 可把暂存值排入一个固定大小的 mailbox。精确请求限当前 Android SDK 的 Set／Apply／GetStatus 子集，拒绝嵌入 NUL、非规范长度、BSSID／channel、未知字段和空响应假成功。不挂 `prov-ctrl`、任意 custom、原 Bootstrap／Install，也不调用 stock manager 的 STA Apply。

原协作 owner 消费已接受的 `0x3A`，先显式退休 Direct 并核原生 Host 释放，再从持久读取物理 instance、原管理员 ID／epoch／version 与已安装 Key locator；第一笔实际 WiFi journal 写前重复检查这些事实以及原 setup／Method 2 时限。验证和写均在同一协作 owner 上，HTTP 回调不碰 NVS，失败、未知、超期或取消不重开。写入复用原 `WifiCredentialStore`；成功只表示候选 journal 可恢复，不表示已关联、收到 WiFiProv 最终回执或云在线。

Security1 是原 PoP 握手加 CTR，不作为本链的逐帧认证或公网安全边界。实际调用方仍须提供同 intent 派生、用途分离的独立 SoftAP 口令与非空 PoP，并排除 STA／LAN 暴露；HTTP 退完并确认后才进入原 STA 路径，丢回执沿 UNKNOWN／新鲜 Cloud 只读协调。底层 HTTP 在应用检查前的分配和停止预算仍属于原 SDK，不能用 103 B 的应用报文上限冒充 HTTP 整体边界（原 SDK 只收至多 63 B 口令，不支持此路径的 64 B raw PSK）。当前不挂产品入口。

### 8.3 同意图 SoftAP 临时凭据与退出顺序

HMAC-SHA256 使用原 32 B PlatformAccess Admin secret。固定输入为 ASCII `blinker.initial-cloud-wifi-handoff.v1` 加尾随 NUL、一个用途字节，然后依次拼接实际 `instance16`、原已提交 Method 2 `sessionBinding16`、`intent16`、已安装 `locator16`，均为原始字节。用途 1 为 `BLK-` + 摘要前 8 B 小写 hex；用途 2 为 WPA2 口令、用途 3 为 Security1 PoP，均取各自摘要前 16 B 小写 hex。两种口令独立、不改 `0x3A`、不出现在持久记录／日志／回执，不以 SSID 作为授权。

合成互操作向量：Admin=`0b×32`、instance=`02×16`、binding=`08×16`、intent=`01×16`、locator=`063f21c532be08683280eabbf47efd90`。SSID=`BLK-74bc382b85aaf840`，WPA2=`23ee394de6bad1f536e3d13dafdeba77`，PoP=`59723ae943c16b76dafa58bf0316799f`。

App 在原认证会话内派生并清原 secret；只有权威 commit、原 `.3A` 一次发送与本地 GATT 退休成功后，才向显式调用方借出临时凭据一次，原账号取消／时限或回调结束均清理。设备只从原认证 handoff 捕获 binding，重新取当前 Admin 派生，receiver 启动后清该 binding；原生 NimBLE 释放、冷 WiFi owner、无 STA／LAN 是独立条件。热点在启用前设置 RAM、WPA2 和单连接，只有 AP 与 DHCP ready 后才开放窄 listener；实际写前仍通过 §8.2 的同根 guard。`Apply` 只认有明确类型的 SDK 排队回执，未见回执不重发。HTTP 和热点均确证释放前不得进入原 STA；journal 成功、实际 STA 已连接、新鲜 Cloud 可用仍是三个不同事实。

### 8.4 显式调用方、Local 退出与 Cloud 确认

Android 候选只在显式 `blinker-initial-cloud` 用途开启。临时 SSID、WPA2、PoP 和原窗口不能通过普通 connect 参数替换；每次尝试有一个独立 `candidateId` 关联 worker 和退出，不以该 ID 自身授予权限。App 传原绝对到期时间和剩余预算，native 在桥接入口取更短预算转换为单调时钟的整次截止，不用 connect 或 IPC 重开窗口。只允许 Security1 与精确 AP `Network` 上的固定 WiFiConfig 操作；过期、换 owner、默认路由回落、旧 GetInfo／Bootstrap／普通配网命令都不能获得写权限。候选不更改现有入口的默认行为。

`not-sent`、`set-unconfirmed` 和 `apply-not-sent` 均表示本调用未发 Apply；`apply-unknown` 只保留可能已发的事实；`apply-queued` 只来自同 owner 的明确 Apply 回执。仅同 `candidateId` 的 Binder 死亡证据，或该候选确证从未启动，才可释放 App 的 Cloud fence。旧 worker 退出、缺失当前 worker、`ready`、固定等待、页面返回或账号取消本身都不是退出证据。

只读确认只在可能已发 Apply 且已证实退出后读取本次已激活 Cloud 卡，核当前账号、目标 ID／Key locator／版本、真实 instance 和在本次尝试之后签发的新鲜会话。未匹配、取消或读取失败不改变原 Apply 结果；Cloud `online` 也不回填官方回执。此段没有重发端口，不开放新的服务端 HTTP 权限或替代原近期认证。
