# tx-04-11：GridEntity 及其子类的完整观测

日期：2026-10-30。状态：待实现、待整体 E2E。遵循[共同观测合同](README.md)，本轮只规划。

架构按[按域拆分详细设计](domain-observation-architecture.md)实施：tx-04-10 先把旧 Grid 迁入独立域，本 tx 在同一域内补全全部公开内容，不另建采集协调器、身份表、录制管线或 Hook 安装 owner。

## 1. 完整目标与来源

补全 GridEntity 公共、全部公开子类、描述／保存状态、Sprite 及关系的观测，并保持真实创建、替换、移除与房间归属。不实现 Grid 写回或 Replica 房间构建。

先读[原版 GridEntity](F:/projects_isaac/IsaacDocs/docs/GridEntity.md)、[原版类图](F:/projects_isaac/IsaacDocs/docs/snippets/GridEntityClassDiagram.md)、GridEntityDesc、GridEntityType／变体枚举及全部子类页；合入[REPENTOGON GridEntity](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/GridEntity.md)及其扩展子类。类型枚举、Lua 派生接口与原生 C++ 类不是天然一一对应，分别登记，不用一个类数量证明覆盖全部 Grid 类型。

## 2. 观测范围

| 对象 | 至少要覆盖的观测主题 |
| --- | --- |
| GridEntity 公共／Desc | 类型／变体、Index／位置、存在／替换、State／VarData、碰撞类、描述与保存状态、SpawnSeed／VariableSeed、公开 RNG 关联、显示／水面裁切等 |
| GridEntityDoor | 门槽、目标房间／连接、类型／状态、开关／锁定／破坏、进入资格及显示、动画与专用变量 |
| GridEntityPit | 坑形、填坑／桥接、相邻连接和专用保存／外观状态 |
| GridEntityPoop | 完整／受损／破坏、变体、专用变量与 Sprite |
| GridEntityPressurePlate | 按压／触发、专用计数／波次关联、GreedModeRNG 等关联状态 |
| GridEntityRock | 类型／特殊岩石、受损／破坏、专用计数／变量与外观 |
| GridEntitySpikes | 启闭、计时／阶段、动画与专用变量 |
| GridEntityTNT | 受损／引爆阶段与子类公开状态；ToTNT 的版本行为按 Wiki 解释 |
| GridEntityDecoration／Wall／Web | 装饰／墙体／蛛网的公开专用状态、存在变化与资源姿态 |
| GridEntityFire／Gravity／Statue | 对应燃烧／重力／雕像的实际公开状态、计时、关联和外观 |
| GridEntityLock／Stairs／TrapDoor／Teleporter | 锁定／开启／显隐／过渡入口、目标语义和各专用变量 |
| 其他枚举或实际对象 | 所有相关 Grid 类型及其适用转换接口、数据容器和明确未暴露项；不得默认为 Decoration 或直接跳过 |

以上类别均进入任务，不只保留旧 profile 中已有值。逐成员目录须展开继承／增量公开状态和查询参数域；表中的主题不替代完整字段表。门虽然可由 Room 访问，仍在此维护唯一 Grid 身份和专用字段，tx-04-12 只增加房间连接语义。

## 3. 身份、保存状态与读取方式

Grid 的“同一个格子”不等于“同一个对象”。同格删除后再生成必须保留旧退休与新身份；跨房重复 Index 必须带 room／dimension／floor 上下文。房间尺寸／可用格子和真实枚举决定范围，不将旧 448 格上限或八门槽记录直接当成所有数据已完整。

公开 `GetSaveState()` 与 `Desc` 的同义来源按 Wiki 记录，不能重复计为独立观测；保存描述也不自动包含 Sprite、临时计时或全部派生运行状态。必须分别解释当前对象状态、房间保存状态和装饰集合的对应。

优先使用公开字段／Getter／ToX 转换、Grid 枚举及生命周期来源。ToX 返回 nil 时记录类别／不适用情况，不通过调用 Init／Update／Destroy／Spawn／SetState 来获得状态。Grid RNG 只读当前信息并关联 tx-04-14，不推进 RNG 来验证。

沿用 tx-04-08 的实际值原则：保存夹具请求的 type／variant／seed／varData 与实际创建结果；差异不能通过修改观测原值或强制回写消除。真实行为和 Wiki 解释有差异时记录具体版本与场景，仅针对疑问查源码或补必要研究。

## 4. 一次整体 E2E 与交付

一个入口编排各类、不同房间形状、受损／破坏／触发／开启、坑桥接、门锁及连接、同格移除再生成、离开回访等场景。比较实际后态与真实生命周期，不要求所有请求参数原样成为生成后态。

交付所有 Grid 类／类型／公开成员目录、描述／运行字段对应、身份与替换记录、资源姿态、实际观察和未完成清单。观察不到的类型或阶段不能算通过；来源不足明确补充路线，不以公共 State 字段替代子类完成声明。全目标完成后仅一次整体 E2E。

## 5. 与后续 tx 的分工

本 tx 负责活动 Grid 及其保存描述、门本体状态；tx-04-12 管房间布局、环境和连接图；tx-04-13 管非活动房间保存集合与回访历史；tx-04-14 管随机流统一目录和 Mod 专用扩展。跨域使用同一 Grid 身份，避免多份状态互相覆盖。

## 6. 公共接口与域内扩展

grid 域拥有模型、字段目录、枚举、派生读取、生命周期、资源姿态及 payload 编码／读取规则。通过 DescribeDomain／OpenScope／Enumerate／CaptureState／OnEvent／ValidateScope／FinishScope 等[公共接口](domain-observation-architecture.md#5-公共调用接口)接入，同一次普通窗口共用协调器的 context、RNG 见证、身份／引用和提交；不把全部子类分支加回 collector 或总 writer。

Grid 槽位、门槽与对象主体分开。同格重建保留旧退休和新主体；当前 Desc／SaveState 属于本域，离房保存集合由 room_save 建独立 SaveEntry，并以有依据的关系关联。room／level 通过引用表达门槽 membership 和结构目标，不重复门本体字段。

保留旧 ready／index、实际原生到达、槽位前后复核和请求／实际值分离；读取失败、未知类型或超预算使相应域／组合明确不完整。新 schema 扩展不重写旧字段编号和 min-read-v1 判据，兼容输出来自同一次采集。以上结构、跨域引用、预算和停用行为并入本 tx 的统一 E2E。
