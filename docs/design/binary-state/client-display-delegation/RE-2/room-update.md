# RE-2：Room::Update 子调用分类

返回：[成果总览](README.md)。根 VA `0x7F7F60`，RVA `0x3F7F60`。下文 `J273_STATIC` 为指令／反编译事实；调用安排另标为设计。只研究静态链，没有执行任何原生函数。

## 1. 根函数的职责顺序

| 位置／原生调用点 | 工作 | Client 判定与替代 |
| --- | --- | --- |
| `0x7F7F60` 开头至 `0x7F9086` | 首帧／特殊房处理、保存／物品／随机来源、生成及对象查询；其中 `0x7F834E→0x424340` 已直接 Spawn | 拦截；生命周期和 Host 实体后态由 world 安装，不运行首帧特例 |
| `0x7F9087—0x7F9092` | 水色 lerp countdown `Room+0x7298` 正数减一 | 可拆纯显示值；不能为此进入前后玩法段 |
| `0x7F9098—0x7F911A` | 消费 `+0x7234/+0x7238` 待处理 Grid 集合，调用 Grid 虚析构并清槽／集合 | 拦截；真实 Grid 退休归安装事务 |
| `0x7F9148` 周围 | 对实际 `width*height` 非空 Grid 调 vtable `+8` Update | 拦截；Door／Pit／Rock 等更新不能按“静态地形”放行 |
| `0x7F91AA` 后 | path DWORD 表 `+0x76C` 和 NPC 占格 WORD 表 `+0xE6C` 的周期维护 | Host AI／路径状态；不为本地显示重建模拟 |
| `0x7F933C→0x7F3860` | Room `+0x11F4` 控制的房内变更 helper；可生成 Entity、替换 Grid、进入 Grid Init | 拦截；不是显示刷新入口，不将未有 SDK 名的 helper 误称纯清房函数 |
| `0x7F9358—0x7F93A1` | Boss deathspawn 计数兜底及警告，回写 `+0x7224` | Host；不能当相邻的普通动画计时 |
| `0x7F93A7→0x413F00` | EntityList 玩法更新，含实体 Update／碰撞／列表维护 | 拦截；显示排序按 RE-1 单独维护 |
| `0x7F93B4→0x96BF40` | FXLayers 显示更新，参数 false | 可独立纳入显示根；见第 2 节 |
| `0x7F93CB/0x7F93D3`，`0x7F9438/0x7F9444` | Backdrop floor ANM2 主／overlay 动画；特定 backdrop 26 保持 Idle | 可拆显示子段；不是对任意世界 Entity 的动画更新授权 |
| `0x7F9453→0x82EE90` | 地狱横移背景动画、位移，随后移动 Entity／Player 碰撞 | 混合；只抽显示前段，见第 3 节 |
| `0x7F9475→0x7E1DE0` | 首帧检查房间敌人／效果并修改音乐 layer 等状态 | 不纳入显示根；音乐请求／集合归 RE-8 |
| `0x7F963E→TemporaryEffects`、`0x7F9645→0x7F7610` | Room 效果计时与死亡名单；后者可给 Player 属性、生成 Pickup／Effect，并立即 Update | Host 规则结果；不为更新死亡名单图标运行这条链 |
| `0x7F96A7→TriggerOutput(9)`，`0x7F97C1→TriggerClear` | 清房条件、事件组、门与奖励；TriggerClear 再 `0x7FC087→SpawnClearAward 0x7F0DE0` | 全部拦截；发送实际 Clear／门／奖励／表现事件，不能让 Client 再裁决 |
| `0x7F982B→0x7F7AD0` | 检查压力板等 Grid，修改描述标志，并 Morph／Update NPC | 拦截，不能当纯门图标维护 |
| `0x7F9832→0x7E64C0` | Player／门／位置条件与房间请求，最终可调用 `0x6F9710` 转场 | 拦截；Host 决定的转场由 RE-9 回放 |
| `0x7F98A7` 后的环境分支 | 随机生成 Effect41／87 等并调用实体 Update；还随机触发闪电／音效 | 不在 Client 另造实际 Entity；复用 Host 实体和 RE-8 的表现状态／音效 |
| `0x7F9BB2→0x8199D0` | 按雨区域采样，生成雨 Effect135 | 拦截；真实雨 Entity 仍由 Host 安装 |
| `0x7F9BB9→0x819C40` | 环境粒子 Effect138 的随机生成和即时 Update | 拦截；外观 Entity 也有身份／列表 owner，不能与 Host 实体重复 |
| `0x7F9BC0→0x81A0B0` | 水流：写实体速度，并生成 Effect99／调用 Update | 拦截；水流视觉／shader 使用已安装状态，不能重跑物理 |
| `0x7F9DBD—0x7FA08E` 的特殊房分支 | 伤害／实体生成等特殊房效果；含 Player 虚调用 | 拦截，不能把“房间背景特殊”当纯显示依据 |
| `0x7FA091—0x7FA0E5` | 两份 RoomShockwave 的 count／radius 更新 | 可抽两份独立纯值运算，见第 4 节 |
| `0x7FA0EB—0x7FA59B` | 血墙持续与随机生成，实体 Update | 拦截；EmitBloodFromWalls 的已生成结果由实体域承接 |
| `0x7FA5B2—0x7FA868` | 水量上升、描述标志、GridPit 状态／方法、熔岩衰减及颜色更新 | 整段混合，不能直接放行；必要值与颜色分开处理 |
| `0x7FA8AE→MamaMegaExplosion`，`0x7FA8C6→0x7F7420` | 爆炸／Greed 波次与生成／事件等 | Host；不是屏幕效果时钟 |
| `0x7FA8CD→0x7F5950` | 原生环境音目标／平滑／启动停止／声像生产 | 不为显示运行整条消费者；由 RE-8 音频状态／事件承接 |
| `0x7FA8F3→ShopRestockPartial` | 补货队列计时到期、商品与价格生产 | Host；Client 只显示实际 Pickup／Price |
| `0x7FA998—0x7FAA0F` 等尾部 | Troll Bomb 生成、批量 NPC／Pickup 动作、复制／重排／再生成等玩法 | 拦截；未分类 helper 默认不放行，不把尾部当显示收尾 |

这张表按功能覆盖任务要求，不宣称每个通用容器／算术 helper 都已命名或每个实体变体都已穷尽。根保持拦截，因此未列入白名单的子路径不会意外执行。

## 2. FXLayers 的显示更新

`0x96BF40 __thiscall(FXLayers*, bool)` 由普通 Room 在 `0x7F93B4` 传 false，Level 换房收尾 `0x7395CE` 传 true。bool 影响平均 Player 位置直接取值／平滑，不能把换房路径的 true 当每帧默认。

本次静态核对的职责：

- `_loaded +8` 为假时直接返回。
- `+0x18/+0x1C/+0x20` 的 FX vector，元素 `0x4C`；有效 pose 数和 `0x14` stride pose。按已有速度环绕位置，或按平均 Player 位置／视口做 parallax。Image 虚调用用于尺寸等显示依赖。
- `+0x34` poop countdown 正数递减，并更新内嵌 FXPoop 的 pose。
- 有 vein 根时，更新十根 vein 的显示尺度／透明度和两份 Sprite；`0x96F090` 维护本地 vein child 图，可分配／释放 `0x34` 节点。它不是实际 Entity 工厂。
- Vein 显示读取当前场景 type407／variant0 的生命比例等输入；这些消费依赖应从已安装的 NPC 状态取得，不能只同步 Player 坐标就声称所有 FX 已具备输入。
- 尾部 `0x995B30` 按当前 roster 求平均位置；含本地 ManagerFrame 缓存和原版 Online 条件读取。C/S 保持原版 Online 会话关闭，不创建 NetInputDevice 或借用原版同步机制。

`0x452020` 是数值 `fmod`，不是 RNG。Vein child 选择在 `0x96F090` 内有独立 RNG 状态推进，允许按 Q4 归本地外观；不能从“未直接 call 全局 Random”推出整链没有随机。未发现该显示链进入 Entity／Grid Spawn、实体 Update 或持久写入。

**采用条件**：主线程、已正常构造／加载的 FXLayers／图像／ANM2／有效容器和 vein owner，当前 Player／Room／必要 NPC 输入已提交；没有换房／退休或并发访问。它写 pose、倒计时、平均位置和 vein 图，不是只读函数。被改为本地产生的范围应从同步结果退役；仍保留为同步状态的写入纳入 13.4 漂移校正。

**设计**：放在安装后的显示推进中，保持原生“实体后态已经就绪，再推进 FX”的依赖；不用实体 Update 创造这个后态。与 Backdrop 动画、颜色／水色值推进同批去重，不在每次 Render 或每个到包回调重复执行。资源准备和 `_loaded` 不能靠随意置 true 伪造。

## 3. 两个背景路径必须区分

### 3.1 现有 BackdropEffects hook 继续拦截

当前 `mod/binding/hooks.json` 的 `client_backdrop_effects` 是 `Backdrop::ClientBackdropEffects`，RVA **`0x515890`**，七参数，原生 VA **`0x915890`**。本次读到多个 `0x424340` Spawn 和实体虚调用 `+0xC`，例如 `0x915D39` Spawn 后 `0x915D70` Update。它生成并配置真实世界对象，不是“调用一次即可推进背景”的显示时钟。

**决定**：此入口保持 ExpectedSkip／拒绝其原函数，Host 生成的装饰实体由 world 安装；需要在白名单中新增的是 `0x96BF40` 的具名 FX 显示职责，不能把旧 hook 简单改成 CallOriginal。

### 3.2 地狱横移 `0x82EE90`

该视图位于 Room `+0x776C`：`+0` Room 回指，`+4` Sprite，`+0x118` 活动，`+0x11C` 绘制位移，`+0x120` 速度，`+0x124` 累计位移。

活动时的显示前段：

1. Sprite 已加载时 `0x82EEB5/0x82EEBD` 分别推进主／overlay AnimationState。
2. `0x82EECC—0x82EEDE` 做 `累计 += 速度`，写 `+0x11C` 与 `+0x124`。

但后面 `0x82EEFA—0x82EFAE` 遍历实际 Entity：Player `0x82EF97→0x6A6A40` 碰撞，其他类型直接修改位置，type951 的特定分支还改额外坐标。**不能调用整个 helper。**

**设计**：仅复现上述 Sprite／位移子段，或由 Host 安装必要当前位移，择一确定字段 owner；不要跳进中段地址当独立函数，不移动任何已安装实体。实际特殊背景 Sprite 的资源／姿态输入仍要完整。

## 4. 水面、波纹与颜色

### 4.1 可以抽出的纯值

| 子段 | 精确运算 | 前置／限制 |
| --- | --- | --- |
| 水色计时 `0x7F9087—0x7F9092` | `if (Room[+0x7298] > 0) --countdown` | 端点、duration／countdown 由同一显示状态 owner 管理；不是每次安装后再减一次 |
| Wave A `0x7FA091—0x7FA0B6` | 若 `+0x70D8 count < +0x70DC max`：count++，`+0x70CC radius += +0x70D4 speed` | A 位于 `+0x70C4`；保持中心、强度等输入，无真实 Entity／物理推进 |
| Wave B `0x7FA0BE—0x7FA0E5` | 若 `+0x70F4 count < +0x70F8 max`：count++，`+0x70E8 radius += +0x70F0 speed` | B 位于 `+0x70E0`，不与 A 合并成一个计时器 |
| Backdrop floor 动画 | 已加载时分别 Update `Room+0x1C34`／`+0x1C54` AnimationState；特定 backdrop26 按原判断转 Idle | 它们属于 `Room+0x1BC8` 内的 floorANM2（ANM2 起点 `+0x1C04`）；不误用 ANM2 起点作为 AnimationState this |

纯值逻辑需要写入自有具名函数，不能把根函数的一小段起始地址强转后调用。步骤 13／RE-3／RE-4 决定本地批次、Host 锚点、停顿与跳帧；本任务不另设时钟。

### 4.2 不能整段运行的水流／水量

`0x81A0B0` 读取 `Room+0x7460/+0x7464`，在 `0x81A184/0x81A18C` 给实际 Entity `+0x360/+0x364` 加水流速度；后面还生成 Effect99 并立即 Update。即使只想要水纹／粒子，也不能放行这个 helper。

`Room+0x7240` 水量上升的分支会改 descriptor flags，并在阈值处调用 GridPit 方法；`+0x7740` 熔岩强度乘约 0.92、跨阈值时调用 `0x70EFF0`，随后 `0x818740` 更新颜色。它们不是独立纯浮点函数。水量／熔岩的权威当前值优先直接安装，颜色与绘图独立消费；若要本地表现过渡，只抽已核的值运算，不重跑 Grid／规则动作。

### 4.3 水色与灯光由原生 Render 消费

官方 `LuaRoom.cpp` 明确注释公开水色 getter 不是最终水色；setter 写目标端点及 duration 标记。J273 `0x816BB0` 以默认 FX 色、seed79 条件、四个水色端点、`+0x7294 duration`／`+0x7298 countdown` 求值，并按 waterAmount 处理 alpha／乘色。不能把旧方案的最终 shader packet 塞回端点再求值，否则会二次乘量。

新方案应提供这些原生消费者实际需要的窄状态，让 Client Render 计算最终 packet。水面反向图形条件仍是实际 BackdropConfig 的 `+0x139` byte；本地经典／反射画质选择不从 Host 后端复制。

`Room::PreRender` 已有 `0x803E62→RenderShadows`、`0x804200→RenderEntityLight`、`0x80422E→RenderGridLight`，尾部 `0x8044E1→坑纹理重建 0x81BB30`、`0x8044E8→水面预绘制 0x816250`。因此这些由 Game::Render 树承担，显示推进根**不额外再调用整条 PreRender 或水面预绘制**。LightGradient／SpotLight 资源先准备；当前必要 Player／Grid／curse／lightning／Room 等输入已安装后，由原生树消费。完整 Render 尾部闭包不在 RE-2 重做，按 RE-5／E28 处理漂移。

### 4.4 房间颜色 API 的职责

SDK `Room::ComputeColorModifier`（`0x818540`）从 `Room+0x1B10` 的 FX roomColor、brightness／contrast 读取，再结合房型、描述标志、pit 数／lava 等求六 float 结果，不执行实体模拟。官方 `Room:UpdateColorModifier(process,lerp,rate)` 在 process=true 时调用它，再调 `Game::SetColorModifier`；process=false 时显式从 KColor＋两个 float 组成六分量，不能把 FXParams 原内存当连续 ColorModState。

原生 `0x818740` 是类似的提交包装：Compute → SetColorModifier(rate=0.015)，并按 Game 初始化／恢复状态决定 lerp。SetColorModifier 写 `Game+0x66938 target`、`+0x66950 step`、`+0x6691C`，必要时也写 current；RE-1 的 `0x6EB990` 才负责后续趋近。

**设计**：显示状态／房间效果变脏时用 SDK API 提交颜色，Render 前按 RE-1 推进；不要无条件每帧重设渐变，也不要与仍在同步的 Game 颜色写入竞争。Host 已裁决的 FX／pit／lava 输入缺失时不能用本地玩法补算。
