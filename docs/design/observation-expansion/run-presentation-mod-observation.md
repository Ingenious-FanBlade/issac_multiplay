# tx-04-14：整局规则、随机选择、HUD／独立表现与 Mod 状态的完整观测

日期：2026-10-30。状态：待实现、待整体 E2E。遵循[共同观测合同](README.md)，本轮只规划。

架构按[按域拆分详细设计](domain-observation-architecture.md)实施，本 tx 接入 run、random、presentation、mod 四域。正式状态、真实事件和 render 来源使用共同记录合同及各自 scope；不把所有数据强塞入每 tick 的 Entity/Grid 快照。

## 1. 完整目标与 Wiki 入口

补全前三个世界层级之外的整局规则与进度、随机与选择系统、HUD 与独立表现、Mod 自定义状态，形成跨域观测总目录。不实现世界下行或 Replica，不因纯展示客户端未来可能不需要某数据而省去本 tx 的观测。

优先资料：

- 原版 Game、Isaac、Level、Seeds、RNG、GlobalFunctions、ItemPool、EntityPlayer／TemporaryEffects、HUD、Sprite、SFXManager、MusicManager、ModReference 及相关枚举／回调。
- REPENTOGON [Game](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Game.md)、[RNG](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RNG.md)、[ItemPool](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ItemPool.md)、Seeds、Level、HUD／PlayerHUD／PlayerHUDHeart、History／HistoryHUD、Minimap、Sprite／资源扩展、MusicManager、[EntitySaveStateManager](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntitySaveStateManager.md)及其关联容器。

逐成员目录覆盖相关公开状态，不按本计划表格中的示例停止扩展。

## 2. 整局规则、进度、概率与时钟

| 主题 | 观测内容与区别 |
| --- | --- |
| 整局规则与进度 | 难度、Challenge、模式／贪婪／Victory Lap、状态 flags、暂停／加载／结束、公开挑战参数、特殊路线／任务条件、伤害／拾取／访问／交易等公开计数和历史 |
| 特殊房间概率 | 恶魔／天使／星象房的公开原值、修正量、条件／历史、最终可获得的显示结果，以及与 tx-04-12 实际生成结果的关系；不能把修正量当最终概率 |
| Seeds | 开局／关卡／房间／装饰／奖励等具名种子、Seed Effects、当前描述与变化；不把种子相同当成当前世界或 RNG 进度相同 |
| 时间 | Authority tick、Game:GetFrameCount、Game.TimeCounter、Isaac.GetFrameCount、房间／实体年龄和 Sprite 帧的原值、单位、暂停／加载行为及采样阶段 |

原版 `Level:GetAngelRoomChance()` 是概率修正值，记录字段名和来源必须表达这一点。最终显示值与计算条件均观察时分别保存，避免客户端需求反过来覆盖实际原值。

Game 帧数、应用帧数、TimeCounter 与项目 tick 不合并成一个计数器。Wiki 的 30／60 帧说明只是相应 API 的语义参考，不用于改变已有 Authority 调度或伪造跨回调同时性。

## 3. 随机与选择系统

分别建立**随机流、当前状态、真实调用、选择结果、选择池状态**，而不是只有一个 global seed：

- 收录 Entity Drop RNG、Player 的道具／卡／药／饰品 RNG、Grid／PressurePlate RNG、Level 生成和恶魔／天使 RNG、公开 Mod RNG，以及实际可发现的其他流。
- 优先使用当前只读状态与 `GetSeed()`、REPENTOGON `GetShiftIdx()` 等公开信息，标明流的 owner、用途、身份、获取方式与有效期；查询是否创建／懒初始化对象不明确时查具体文档／绑定，不能无条件实例化所有 RNG。
- `Random()`／`RandomVector()`／RNG `Next`、`RandomInt`、`RandomFloat` 等按**真实调用**记录已存在的输入／结果与可获得的前后状态；不为观测多调用一次，也不把 Phantom 结果冒充真实已消费结果。Mod 的其他随机源通过适配声明登记。
- 公开 Lua 路径之外的原生调用不能因 Lua wrapper 已覆盖就宣称全部覆盖。先列具体调用来源缺口，复用已知点位或只补必要入口，不对每条随机流强制做一套逆向。
- ItemPool 覆盖公开池列表、当前／初始权重、移除与黑名单、饰品／卡药相关状态、药丸颜色—效果映射／鉴定、真实选择／重掷结果和适用条件。REPENTOGON `GetCollectiblesFromPool()` 等可读入口优先。

`GetCollectible()`、`GetCollectibleFromList()` 等“Get”方法可能抽取、使用默认 Random 或加入黑名单；不放入无副作用轮询器。记录真实调用时保持原调用一次、不改参数／返回值／消费顺序。源不可见的随机流或事件如实列出缺口，不能把掉落结果存在当成 RNG 内部状态已完整。

## 4. HUD 与独立表现

观测世界／Player 状态到 HUD 的关联，以及 HUD 自己的公开运行状态：心量布局、资源、主动／口袋槽、充能、属性／概率／计时、Boss 血条、道具历史、提示／文本、Minimap 表现、Sprite／Overlay／图层／资源姿态，以及独立效果对象。

持续音乐／循环音效／持续提示等作为有寿命的状态；一次性音效、闪光、震屏和提示触发保留真实事件与时序。音频控制状态、绘制参数与实际声卡输出／最终 GPU 像素是不同观测层，不把记录函数参数称为完整录屏或音频采样。

公开 HUD／Sprite Getter 优先；不额外调用 Update／Render／Play／Load 来“读取”当前表现。render 时刻的状态与 Authority 后态分别标记；瞬时事件发生但前后样本均不存在时仍可被事件记录表达，不伪造常驻对象。复用 tx-04-10 的 Sprite／资源模型，避免两个目录重复统计同一 Entity-owned Sprite。

## 5. Mod 自定义状态

本 tx 必须交付明确的 Mod 观测适配机制和真实适配样本，而非只写“未来支持”。范围包括受支持装载 Mod 的 `Entity:GetData()`、自有 run／floor／room／player 状态、保存状态扩展、Mod RNG、独立绘制／提示以及生命周期。

适配声明需给出 Mod 标识／版本、命名空间／schema、只读采集函数、归属／寿命、字段与引用、资源和事件、适用时机、未暴露内容。普通 Lua table 按有界对象图复制，处理循环／共享引用、稀疏键、函数／userdata／线程、失效引用和非有限值；不能无限遍历 `_G` 或凭字符串化 userdata 获得完整状态。深度／字节预算耗尽明确截断，不静默删键。

对于私有闭包、局部变量、自定义 C++ 对象或任意即时绘制，公开 Wiki 不会自动给出 Mod 自己的内部状态。应由 Mod 提供只读适配／公开接口；确有需要再针对具体缺口扩展。对已声明受支持的 Mod，缺失项属于未完成；不把“任意未知 Mod 全部内部状态已自动捕获”作为虚假承诺。

EntitySaveStateManager 优先使用已有内容查询；Wiki 中 `GetEntityData()` 可创建表，而 `TryGetEntityData()` 只在已有时返回。观察者不能通过创建表、SaveData／写存档或主动触发保存来补全信息。磁盘 Mod 存档与运行内存状态分别表达；未授权持久数据不改写。

这扩展的是**观测能力**，不自动宣布任意第三方 gameplay Mod 可在 C/S 或未来 Replica 上运行。自有观测 Lua、纯 UI Lua、E2E 场景夹具与第三方 gameplay Mod 分开登记，避免沿用旧“界面 mod 不注册 gameplay 回调”规则而把正式只读采集一概排除，也避免将只读观测放宽成玩法修改。

## 6. 一次整体 E2E 与交付

一个统一入口覆盖整局规则／概率条件变化、暂停／加载／计时差异、具名随机流当前状态和真实消费、池权重／移除／黑名单／重掷、药丸鉴定、HUD／独立 Sprite／提示／音乐与一次性事件，以及 Mod 表／保存扩展／RNG／资源和退休。

夹具制造正常玩法变化，观察者不制造额外随机、抽取、表创建、绘制或持久副作用。保留实际调用／结果及有来源依据的前后记录，不额外消费随机来构造所谓对照。输出跨域总目录、随机流及池模型、时钟表、表现状态／事件、Mod 适配 schema 与原始样本、未暴露和未观察内容。

六个 tx 的最终汇总须逐域区分已实现／已观察／未完成，并指出哪些数据足以供后续同步设计使用、哪些仍需补充；这只是观测结果汇总，不在本 tx 启动状态同步实现。全目标完成后仅一次整体 E2E。

## 7. 四个域的接口与时序交付

| 域 | 与已有模型的衔接 | 本 tx 新增的时序与接口内容 |
| --- | --- | --- |
| run | 引用 level／player、实际生成结果和随机流；保留每种计时原值 | 整局生命周期、规则／进度状态及变化版本，不合并 Game 帧／应用帧／Authority tick |
| random | 复用 Entity/Grid 旧 RNG 见证与 owner 引用；统一此前未完整目标 | owner＋用途＋参数域＋寿命的流身份、池状态、实际调用前后与结果；不以 seed 作为流 ID |
| presentation | 引用 Entity-owned Sprite／通用资源姿态、Player 状态与世界探索 | Render scope、独立 HUD／Sprite、持续音画对象和一次性事件；tick 可空，关联不冒充同时采样 |
| mod | 通过具名 namespace/schema 扩展，不覆盖基础世界字段 | 只读适配注册、生命周期／阶段声明、有界图复制、引用／共享节点与截断；Mod RNG／HUD 关联已有域 |

各域通过[公共调用接口](domain-observation-architecture.md#5-公共调用接口)声明状态／事件能力、适用阶段、预算和失效条件，自行提供 payload 编码／reader。实际调用通知仍经过统一 Hook／Lua 来源 owner，原调用 exactly-once；事件与后态各自发布，后态提交失败也不得删除真实事件或挂到旧 completed tick。

完成跨域总目录、实际启用范围、版本复用与缺口汇总；对未知 schema、未适配 Mod、缺少原生随机消费来源分别报告，不能忽略后称全域完整。Recorder 的写盘／队列失败只关闭对应录制消费端，生产状态采集不依赖调试录制。以上事件时序、scope、引用、预算／图截断与停止行为并入本 tx 统一 E2E。
