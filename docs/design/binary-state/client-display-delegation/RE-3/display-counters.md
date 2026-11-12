# RE-3：显示依赖的逐帧计数目录

返回：[成果总览](README.md)。`F` 为最近已成功应用 world 的 GameFrame，`M` 为 Client 本地 ManagerFrame。以下以本地 Wiki／SDK、RE-1／RE-2 与本次 J273 指令为据，区分**读取时钟求值**和**每次调用写状态**；不把所有 timer 统一外推。

## 1. 原生时间消费

| 值／范围 | 原生位置与行为 | Client 归属／安装 |
| --- | --- | --- |
| Game 年龄／Room 年龄／Entity 年龄 | Game `0x402A60`；Room `0x809590`；Entity `0x6A5290`；后两项由 F 减各自基准得出 | F 只取帧头，Room `+0x11F0`／Entity `+0x328` 只装原始 Host 锚点；没有本地年龄累加器 |
| TimeCounter，Game `+0x25A74` | 普通递增 `0x6F7894`；Minimap::Render `0x967763` 读时间；挑战22在 `0x967720` 读 `max(28800−TimeCounter,0)` | 仍是独立 i32；当前 `run.json::time_counter_base` 为 u32 基准，按 F 还原，保留负值／回绕；不额外运行计时事件规则 |
| 本地 ManagerFrame | `+0x4A8B8`；相机 caller `0x92E926` 后取奇偶；Room::Render `0x807787/0x807CC2` 读 M，同时 `0x80779B/0x807CD6` 读 F 组合相位 | 保留本地 M；原生 Render 直接消费两种时间，不再借用 Host M 或 D0 |
| 平均 Player 位置缓存 | `0x995B30` 在 bool 参数为真时，`0x995B49—0x995B55` 以 M 比较全局缓存戳 `0xBD01E8`；`0x995D2A` 写回当前 M | 这是本地缓存戳，不是应安装的 Host 计数。M 冻结可让该分支重复取旧坐标；world 安装先于本轮显示查询，不在同一个 M 内反复更换世界并消费旧缓存 |
| ANM2 动画游标 | SDK ANM2::Update `0x404550`，有资源才调主／overlay 的 `0x404480`；后者 `0x4044AE` 以 ANM2 `+0x104 playbackSpeed` 加至 AnimationState `+0x10 animFrame`，再处理帧／循环／事件 | **按调用推进，不读取 F/M 来决定本次是否已经推进**。仅本地显示拥有的动画可按既定显示批次调用；Host 安装姿态的实体 Sprite 不因此全体自动 Update |
| Game 驱动的 shader 相位 | Room::Render 上述 F/M 组合；Game::Render `0x6F8E05` 读取 F；FX Render `0x96CA3B/0x96CA41` 用 F 与 Room 基准 | 同步必要输入后由原生绘制计算，不发送旧最终时间 packet，也不临时换入另一份 F |

原版 [Sprite.md](F:/projects_isaac/IsaacDocs/docs/Sprite.md:207) 明确要求按更新频率调用 Update；在 Render 回调中推进应控制到偶数帧。SDK [ANM2.zhl](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/ANM2.zhl:4)／[AnimationState.zhl](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/AnimationState.zhl:61) 给出 Update、playbackSpeed 与 animFrame 布局。不能把“F 没变”当作 ANM2 自动停止的证据。

## 2. 显示依赖的状态／倒计时与 Q8

本表是 RE-1／RE-2 的计数汇总，细节分别见[Game 内联显示工作](../RE-1/additional-display-work.md)、[Room 更新拆分](../RE-2/room-update.md)。**现行 Q8** 见[主方案 4.6](../../client-display-delegation.md#46-内联显示标量q8)：Game／Room 内联标量的原生运算仅用于定位与理解，Client 安装 Host 当前值，不执行这些运算。HUD／地图／FX／特定背景 Sprite 的本地推进另列；未实现新的计时 owner。

| 范围 | 原生写入方式／位置 | 显示委托时需要明确的责任 |
| --- | --- | --- |
| HUD refresh | `0x97AD02` 在 consumed flag `HUD+0x54EC` 为真时减 `+0x54F0`，随后清 flag，到期清 type `+0x54E8` | 条件推进，不是无条件每帧减；脏请求与实际消费配套，本地缓存不装 Host 历史 |
| HUD 两个显示计时 | `0x97AD2C/0x97AD3D` 将 `+0x54B4/+0x54B8` 的**非负值**减一，0 可变 −1 | 不把它们套为到零夹限的普通正倒计时；保留哨兵语义 |
| HUD 消息／fortune、StatHUD 差值提示 | HUD 调 `0x8317A0` 推进消息状态／TextIn/Stay/Out 与 Sprite；PostUpdate 的 StatHUD `0x83C860` 维护显示缓存和提示计时 | 横幅事件先初始化本地 owner，再按显示批次推进；音效去重交 RE-8，不逐帧复制 Host 的动画缓存 |
| PlayerHUD 三色闪烁／输入渐变 | `0x834776/0x834782/0x83478E` 递减 `+0xC/+0xD/+0xE`，尾部维护 `+0x6BC` 的 Drop 显示渐变；另 `0x832D38` 读 F 的45帧相位 | 本地计数与 canonical 显示输入各有来源；F 的周期读取与本地每次递减不是同一种计时 |
| Minimap 按键与震动 | `0x965FEC` 增 `+8 holdTime`；`0x96600C—0x966010` 将正的 `+0x61C` 减一并更新 offset | 本地 UI 输入／动画；不由 Host F 的 Δ值补跑多次按键消费 |
| FXLayers | `0x96C219/0x96C231` 对正 `+0x34 poopFxCountdown` 减一；pose／vein 图按调用变化 | 本地外观 owner；采用资源、实际 Player／NPC 后态，不能与同步相同结果争写 |
| Backdrop floor／地狱背景 Sprite | Room `0x7F93CB/0x7F93D3` 推主／overlay AnimationState；地狱背景 `0x82EEB5/0x82EEBD` 同类调用 | 按显示批次推进；地狱 helper 后半含移动实体／碰撞，继续按 RE-2 拆分 |
| 水色 lerp | Room `+0x7298` 正值在 `0x7F9092` 减一，duration `+0x7294` 和四个 KColor 端点参与 Render 求值 | Q8：安装同目标的端点／duration／countdown，不再本地减一 |
| 两份 RoomShockwave | `0x7FA091—0x7FA0E5`：count<max 时 count++、radius+=speed；两份独立状态 | Q8：安装两份权威当前值，不本地 count++ 或扩半径，不重跑伤害波／实体生产 |
| Bloom／震屏／hallucination／Retrovision | Game `+0x25AA0/+0x25A80/+0x25AB0/+0x25F58`，原生每次普通 Update 有条件递减；震屏到期还清偏移 | Q8：安装必要当前值，不本地递减；震屏倒计时由正数变0时，在安装点清本地相机偏移 `+0x25A84/+0x25A88` |
| 颜色／闪电／暗度／眩晕／fade | RE-1 的 `0x6EB990` 六色趋近，`+0x6699C` 闪电衰减、`+0x25A8C/+0x25A90` 暗度、`+0x669A0/+0x669A4/+0x669A8` 眩晕，以及两套 fade alpha | Q8：安装需求保留的当前值；Client 不调颜色趋近／SetColorModifier／`0x818740`，不拆 alpha 段，fade 生命周期仍由 Host 决定 |

地狱横移的 Sprite 动画仍在本地推进，但视图 `Room+0x776C` 内的 `+0x11C/+0x124` 位移按 Q8 安装当前值，不能连同 Sprite 一起累加。waterAmount／熔岩强度同样为 Q8 当前值。需求清单决定保留哪些输入，不重新开启这些值的本地生产。

**DESIGN**：本地推进依赖主方案既定的 Manager 边界两档调度，不以网络到包次数或 Render 次数直接相乘：30Hz 偶数批次与相机阶段分开。停包时 F 保持，本地消息／地图／FX 等可以继续推进；Q8 当前值保持到后续有效安装，阈值动作在安装点补做。RE-4 处理去重／跳帧，不新增独立展示时钟。

## 3. 已有子类计时合同不能混用

| 字段 | 当前事实 | 按方案 B 处理 |
| --- | --- | --- |
| Effect `+0x45C Timeout` | 当前 [entity-effect.json](F:/projects_isaac/issac_multiplay/mod/binding/state/entity-effect.json:952) 为 `expire_frame`，Host `ExpireFrame`／Client `RemainingFrames`；公共尾有正值递减、101 双减、141 刷新等非统一分支 | 若需求仍保留，每个成功 world 按最新 A−F 重写；即使实体块不变而帧头变也要更新。Host 每窗重算 A；Client 不运行 Effect::Update／移除来补时间 |
| Familiar `+0xD4C FireCooldown` | [2026-11-11 用户决定](../../10-entity-player-capture.md:59)：可一帧两减／重置，当前 [entity-classes.json](F:/projects_isaac/issac_multiplay/mod/binding/state/entity-classes.json:774) 的 wire 为 copy/i32 | 保持原值到下一份采样；不再调用旧 DecodeEntityCounter 外推 |
| Pickup `+0x554 DropDelay` | 同文用户决定：正值且 collisionClass=0 时减一，否则可直接清零；当前 [entity-classes.json](F:/projects_isaac/issac_multiplay/mod/binding/state/entity-classes.json:4185) 为 copy/i32 | 同上；不恢复旧条件倒计时 handler，也不在 Client 跑 Pickup::Update |

旧 `13-client-apply.md` 的部分子类段、schema 的个别回映射 reason 仍保留历史 mode/value 说明。本次以实际字段 semantics／wire 及明确用户决定区分；不因通用例子或仍存在的 [entity_counters.h](F:/projects_isaac/issac_multiplay/mod/src/state/entity_counters.h) 函数，就认定当前字段仍采用那种编码。本次未重新生成 schema、修改字段源或产品代码。

Effect 的 `RemainingFrames` 是模2^32相减再按 i32 解释，**不夹限负值**。它与只允许正值降到零的 `DecodeEntityCountdown` 不是同一合同。若 Host 某变体停走，Host 的 A 会随 F 改变并重新发出；不可以把上次 A 当永恒线性关系。

## 4. 官方 REPENTOGON 的额外消费者

- [Anm2Extras.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/Anm2Extras.cpp:315)：Dogma／Gold shader 在 affectedByPause 时取 Game F，否则取 M/2，再按256周期取相位。明确需要两种时间，不能把整个 Manager 借成 Host 值。
- [InterpolV2.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/InterpolV2.cpp:54)：选项启用且满足自身条件时，以本地 M 奇偶选择0或0.5的插值偏移，再乘 playbackSpeed。该代码有选项条件，本次不宣称当前运行配置已经开启。
- [ExtraRenderSteps.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ExtraRenderSteps.cpp:63)：本地错误显示保存 M 作为激活锚点，后续按 M−锚点做脉冲／淡化。这类资源 UI 不应绑定 Host world 时间。

这些是官方源码的静态语义补充，不能把原版 IDB 的绝对地址直接当作 REPENTOGON C++ wrapper 地址。Lua 的 Q6、音效 Q7 和本地选项继续按主方案执行。

## 5. 交给 RE-4 的已定位线索

本次只定位计数来源，不提前完成 RE-4 的触发／丢帧策略：

- Game `0x6F78BA` 新 F 的30帧周期刷新；Client 不执行原增量段后如何提出刷新。
- Game::Render `0x6F7CA0—0x6F7CA9` 比较 Room 年龄是否恰为30；跳过30或重复绘制30的影响。
- Camera `0x91E249/0x91E64B` 对 F 做 `%3` 后选择震屏分支；停在相同 F 时反复调用会继续消费本地随机。
- PlayerHUD `0x832D38` 的 `%45` 相位，Minimap 时间文字 `0x9677C4` 的 F 周期；它们与按调用递减的本地闪烁需要区分。
- 平均位置缓存 `0x995B49` 的 M 相等快返，以及 F/M 组合 shader、官方可选插值，在一个本地周期内装多份 world 时的顺序。

转场自己的阶段／计数与等待目标行为归 RE-9；未放行的实体／Grid／游戏规则计时不因本表而转为 Client 本地模拟。
