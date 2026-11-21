# Entity 公共字段同步必要性复核

日期：2026-11-09（Asia/Singapore）。布局版本 **30**。按用户要求复核第 3 步候选，修订唯一布局源 `mod/binding/state/entity.json`、生成表及第 5／10／13／14／15 步合同。只有静态资料分析和布局生成／纯编译，没有运行测试、游戏、注入或部署；功能验收仍只在第 15 步进行。

> 后续版本31纠正：[valid／exists／dead再复核](entity-lifecycle-synchronization-review.md)撤回下文valid的公共传输要求；其原生字节归本地owner，Player独立激活资格保留为P7结构职责。exists／dead保留结论不变。本文33范围和223B是版本30历史范围，当前公共最小状态410B。

## 结论与适用范围

Client 只构造、安装和展示 Host 的结果，不运行 Entity Update、原生 Interpolate（虚槽 4）、碰撞／位置模拟或状态到期处理。按这个已有边界，**33 个公共范围不需要传输：32 个归 Host 权威，`removed_by_factory` 归本地生命周期**。没有删除原生字节归属、降低对象大小、填补 UNKNOWN 或提升 SDK 声明的证据等级。

“用途已知”“公开 getter 存在”“旧目录曾发送”“不能从当前 Position 推导”都不是同步理由。本次同时保留发现真实展示消费者的字段。结论针对 J273 原版及已适配官方 REPENTOGON 路径；任意第三方 Lua 绘制回调读取私有计数不在当前支持声明内。

## 不再传输的范围

| 范围 | 字段／数量 | 归属与理由 |
|---|---|---|
| `+2E0/+2E4` | 毒／火伤害相位，2 个 i32 | 权威。Update 在对应 flag 分支加一、按 mod 20 决定伤害；Client 不重演伤害节奏，接收最终 HP、颜色及效果成员 |
| `+210—+23F` | 火伤害冷却、受伤、冻结、毒、减速、魅惑、混乱、点金、恐惧、灼烧、Boss 状态冷却、缩小，12 个 i32 | 权威。持续时间、清除状态与玩法更新在 Host；展示使用最终 flags、颜色、Sprite 和子类表现 |
| `+2E8`、`+2F0—+307` | 流血、磁化、诱饵、击退、虚弱、冰、硫磺标记，7 个 i32 | 权威。同上；不为公开 countdown getter 恢复传输 |
| `+31C` | PauseTimer，1 个 i32 | 权威。只约束 Host 位置／暂停路径，不是全局展示暂停状态；暂停时仍同步最终位置和姿态 |
| `+314—+31B` | 暂停保存位置，Vector | 权威。位置路径在 PauseTimer 仍正时将其写回 Position；Client 不执行这条回写，不需要复现保存历史 |
| `+344—+34B` | 原生半帧插值历史位置，Vector | 权威。虚槽 4 保存并推进 Host 半帧位置。Client 不调用该槽；若以后采用网络展示插值，历史由已应用的 world／body 快照在本地建立，不能以 Host 的半帧历史代替 |
| `+308—+30F` | 击退方向，Vector | 权威。Host 击退／位置更新输入；Client 接收最终位置、速度和姿态 |
| `+278/+27C`、`+388` | 毒伤害、火伤害、接触伤害，3 个 f32 | 权威。伤害裁决输入；最终 HP、死亡展示和效果成员照常同步 |
| `+2D4/+36C` | Mass、Friction，2 个 f32 | 权威。物理／半帧运动输入，Client 不执行对应模拟；不因删除传输而声称 SDK 的 Mass 偏移已完成新的运行时验证 |
| `+190/+3B0` | CollidesWithGrid、IsInvincible，2 个 bool | 权威。当前碰撞结果和伤害可受性裁决开关；Player 另有真正控制闪烁的独立伤害冷却，不与公共计时混淆 |
| `+177` | removedByFactory，bool | 本地。反映当前进程工厂删除及 REP EntityManager 记账；由 Client 构造／退休 owner 维护，不写入 Host 的工厂标志 |

同步撤销以上范围的 `wire`、专用采集资格和线上 validator，并移出 E1／E2／E3；28 项公开成员回映射改为 `authority`，保留原生字段定位。E3 仍有两项缩小状态，不清空、不改后续块号。布局最小负载每个 Entity 减少 **223B**（22 个 8B 计时锚点、3 个 8B Vector、5 个 f32、3 个 bool）；这是布局算术，不是性能实测。

## 继续保留的字段

| 字段 | 正面展示依据与安装要求 |
|---|---|
| `valid +170` | Player RenderHead `783680` 在 `783707` 读取当前身体的 valid，参与角色 29／38 的替身复合绘制；不能概括为“仅分配标志”并删去。第 5／13 步须结合身体成员／备份角色和本地 owner 安装其展示语义；不得热写 false 让本地池误以为可复用，也不得写 true 冒充已构造。布局源已为该字段指定`entity_valid_presentation_owner`，禁止按无处理器的direct字段合并写回；必要时在完整绘制借用期覆盖后恢复，不能提前声称该writer已实现 |
| `exists +172`、`dead +173`、`visible +171` | 公共 Render `6AC4ED`，Tear／Bomb／Pickup／Text 等先检查 exists；成员仍在表并不代表可画。Player Render `785F3B` 以 dead 控制展示，Room Light `7FF848` 也读取 dead。它们不是可用成员表统一替代的本地池状态；不得通过 Kill／Remove 重演死亡 |
| GridCollisionClass `+184` | `IsFlying 6A7C90` 及 WaterClipInfo `6B2F70` 的飞行分支需要它；REP nullable override 另行保留 |
| EntityCollisionClass `+188` | 公共 PostRender `6ACCA7/6ACCCD`、Laser PostRender `956FC9/956FFC` 按此值选择调试碰撞几何颜色。当前支持原生 PostRender，不能以常见画面未启用调试为由删掉 |
| Size `+370`、SizeMulti `+374` | 公共／Laser PostRender `6ACC90/6ACCF8/956FB4` 绘制碰撞几何；Room Light `7FFD4C` 读 Size；Familiar helper `6473C5` 读 SizeMulti |
| SpeedMultiplier `+39C` | Effect Render `600E60` 的 variant172／subtype1 分支在 `6039A0` 读取该值，参与方向旋转；不是可随 Friction 一起删去的纯物理量 |
| Position／PositionOffset／Velocity／TargetPosition、最终 flags／颜色／Sprite／HP、FrameCount 基准 | NPC 特殊绘制会用 Velocity（如 `6CF1E6/6CF200`）；Effect variant172 等会读 TargetPosition（`603945`）；既有 WaterClip／资源／帧周期依赖保留。公共计时器被删不意味着清除相应视觉状态 |

ColorParams 的 Duration／Lifespan、Effect Timeout、Familiar FireCooldown、Player 独立伤害闪烁计时等属于各自的展示合同，不在本次 20 个 Entity 公共倒计时排除范围内。`+175` 和其余 UNKNOWN 仍保持未知；两项缩小状态本轮不连带裁剪。

## 静态依据及证据边界

目标 IDB 为 `isaac-ng-v1.9.7.12.exe.unpacked.exe`，ImageBase `0x400000`；下列地址均为 VA，没有修改或保存 IDB。

- 复读公共 Render／Shadow／PostRender `6AC4E0/6AD450/6AC660`，以及 Player `784A60/7862A0`、NPC `6CEC90/6D0650`、Tear `675AC0`、Projectile `659460`、Pickup `6E35D0`、Familiar `61E360/61E320`、Effect `600E60`、Knife `93ECD0`、Bomb `6A0F90`、Laser `9514B0/955290/956F80`、Slot `6620E0`、Text `5CC580`、Dummy `6A3630` 和 Room Light `7FC110` 的调用／字段候选。
- 对这些展示根做有界直接调用追踪（最多六层，452 个函数；Lua 回调桥及图像／标准库按既有边界处理），与全 `.text` 的候选位移交叉检查；对正面命中核对实际对象。**位移相同不是 Entity 消费者证明**：Room Light `7FCF5D` 的 `+300` 来自另一对象，不是 Entity IceCountdown；`903A50` 的 `+2E0/+2E4` 是统计／解锁对象，也不是毒／火相位。这些不是运行时观察，更不是任意地址别名的全程序形式化证明。
- 毒／火相位原始读写为 `6AAE03/6AAE17`、`6AAECD/6AAEDD`；状态计时递减和设置证据沿用[第 3 步证据](entity-common-evidence.md)。NPC 状态外观读最终 Sprite／flags，不为清除状态再执行 Update；Player 实际闪烁读取 `+13B8`，沿用[Player 闪烁证据](../tx-04-20/player-i39-source.md)。
- 暂停位置回写 `6C089C—6C08AE`；原生半帧槽 `6AC2E8/6AC2F1` 写历史、随后按 Friction／Velocity／SpeedMultiplier 推进 Position，并调用位置／碰撞 helper。它是 Host 模拟，不应为展示插值在 Client 放行。旧隔离入口和 `13.4` 的“Client 不运行原生 Update”前提继续适用。
- 官方 REP `LuaInterfaces/Entities/LuaEntity.cpp` 的公共计数 API 是直接 getter／setter 及状态复制；`Patches/EntityManager.cpp:158—169` 的 Remove wrapper 消费 `_removedByFactory` 决定本地删除通知。`Patches/ASMPatches/ASMGridEntityCollision.cpp` 写 `_collidesWithGrid` 是碰撞路径，不提供新增展示消费者。

## 后续接线和统一验收

- 第 10 步不读取／换算这些上线已排除范围；Host 生命周期观测若仍需 `+177`，照规范第 11 节提供真实读取资格，不将其恢复为状态字段。
- 第 13 步不安装、还原计时或漂移比较这些范围；本地工厂标志与原生容器／关系正常由 owner 管理。保留 valid 的展示输入不能绕过本地生命周期。网络位置插值若实现，以本地快照为历史并保留暂停／换代重置语义。
- 第 14 步连同旧公开计时／伤害／物理字段的采集、写入及相等比较一起退役；本轮不提前切换仍在运行的旧 JSON 路径。
- 第 15 步沿用 E09／E25 检查最终位置、死亡、状态颜色／图标、暂停、替身身体、碰撞调试几何和光照；不要求上述 Host 玩法原值相等，也不为不上线字段造解码拒绝用例。E28 检查本地生命周期与绘制借用恢复；E34 只比较实际保留的展示计时。不得因本次移除公共计时器而跳过子类／ColorParams／Player 真正的展示计时。

原第 3 步“保存历史／计时原值以保真”的实现记录保持历史语境，由本次同步职责修订取代其传输要求。没有新增测试或验收编号。

本轮普通Release／Win32产品构建（`ISAACCS_BUILD_E2E_TOOLS=OFF`）已重新生成全部布局表；Entity最小状态由634B降为411B，未知仍为335B。受限进程环境的首次构建停在CMake子进程阶段并被终止，改用正常Windows构建环境后，`isaac_cs_state`与完整`zhlIsaacCS`编译／链接成功；没有运行测试或部署。
