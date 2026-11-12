# RE-2：Room 更新与初始化的拦截／拆分清单

日期：2026-11-12。状态：本任务静态研究完成；未修改产品代码、安装 Hook、修改目标内存、构建、启动游戏或运行测试。归入原二进制重构第 13 步，资源需求交第 7／8 步，不另开 tx。

任务来源：[修改方案第 6 节](../../client-display-delegation.md#6-新增逆向任务j273-静态反编译)。目标仍为官方 REPENTOGON 1.1.3／v1.9.7.12.J273。先查本地原版／REPENTOGON Wiki、SDK 声明和官方实现，再以当前 J273 IDB 的指令、交叉引用和反编译核对；没有使用 J460 偏移。

> **2026-11-12 审阅后标注**：用户随后决定 Q8，`Room::Update` 内联显示标量同步 Host 当前值，Client 不本地推进。结论 8、9 和分类总表中“水色倒计时和波纹值推进”的本地推进，以及 [Update 报告](room-update.md) 4.1、4.4 节在 Client 提交房间颜色的设计，由[修改方案 4.6](../../client-display-delegation.md#46-内联显示标量q8)取代；地址、运算和副作用事实保留。FXLayers 与 ANM2 动画仍在本地推进。

## 成果入口

- [入口、ABI 与资料依据](entrypoints-and-sources.md)：原生地址、SDK 差异、定位模式与来源。
- [Room::Update 分类](room-update.md)：EntityList、Grid、清房／奖励、背景、FX、水流／水色／波纹、光照与音频。
- [Room::Init 拆分](room-init.md)：原生初始化阶段、生成与副作用、显示资源和冷构建顺序、现有代码交接。

## 结论

1. `Room::Update`（VA `0x7F7F60`）继续整体拦截。即使跳过 EntityList，它仍会更新／销毁 Grid、处理清房与奖励、修改 Player／NPC／Pickup、执行碰撞和切房，不能把原函数剩余部分当显示更新。
2. `Room::Init`（VA `0x7E8AD0`）也是混合根。**只拦截 Spawn 不能使它变成安全显示初始化**：前段就清 TemporaryEffects、销毁旧 Grid／列表、重置全局状态；中间改描述／进度／规则，末尾还有资源、Camera、Lua GC 和特殊房 helper。Client 冷构建应复用已核显示子段，由已有 NativeBuild／NativeApply owner 承担结构事务。
3. SDK 把 Init 第一参数声明为 `int param_1`，但 J273 实际按 `RoomConfig_Room*` 读取其 `+0x14` 名称及 `+0x46/+0x47` 尺寸。现有 hook 的参数名 `roomType` 不是其语义。ABI 仍是 ECX=Room、两个栈 word、`retn 8`；本次未修改 SDK 或 Hook。
4. `client_backdrop_effects` 对应 **`0x915890` 装饰实体生成 helper**，会 Spawn 并进入实体方法，继续拦截；它不是 FXLayers 动画更新。实际 `FXLayers` 更新是 **`0x96BF40`**，可在资源／容器／Player 等依赖有效时独立纳入显示推进，常规参数为 false。
5. 背景资源链 `Backdrop::Init(true) → 0x917910 → pre_render_walls 0x911310` 夹带 `0x915890` 和另外两处直接 Spawn。构图与实体生成需要拆分；现有“两处直接生成分支”防护不应因去掉手工绘制而被误删。图形缓存本地产生，真实装饰 Entity 仍由 Host world 安装。
6. 水流 `0x81A0B0` 会写 Entity velocity，雨 `0x8199D0`、环境粒子 `0x819C40` 会生成并更新实际 Effect，均不作为本地显示更新。水量／熔岩衰减段还调用 GridPit 方法，不能整段放行。
7. 地狱横移背景 `0x82EE90` 会移动 Entity 并对 Player 调碰撞。只取其 Sprite 动画和背景位移运算；后面的实体循环继续拦截。
8. 可拆出的遗漏显示工作包括：Backdrop floor Sprite 两路动画、两份 RoomShockwave 的计数／半径、水色 lerp countdown、必要颜色计算。光照 Sprite、背景 surface、Camera 和特殊背景资源归初始化；原生阴影／光照／水面绘制随 Game::Render 树运行，不在显示根重复执行完整 PreRender。
9. `Room::ComputeColorModifier`／`Game::SetColorModifier` 可表达房间颜色更新；后者会写 Game 颜色目标／步长，与 RE-1 的颜色趋近是前后两项职责。已有同步颜色与本地生产要归同一 owner，不能每帧反复重设渐变。

## 分类总表

| 判定 | 范围 | 后续职责 |
| --- | --- | --- |
| 保持根拦截 | 完整 Room::Update／Room::Init，EntityList／Grid 更新、清房／奖励／补货／选路／房内效果生产 | Host 唯一裁决；Client 安装实际结果 |
| 可独立安排 | `FXLayers` 更新 `0x96BF40`；已构造 Sprite 的显示动画；水色倒计时和波纹值推进 | 第 13 步显示根；安装后、Render 前，去重与帧语义接 RE-3／RE-4 |
| 冷构建时复用 | LightGradient／SpotLight 加载、`Backdrop::Init(false)`、Camera 构造、特定地狱背景资源初始化；必要颜色 API | NativeBuild／NativeApply；有效主线程／图形 context／资源／对象关系 |
| 混合，拆分 | Backdrop 构图链、地狱横移、完整水量／熔岩更新、Room 的背景／FX 选择初始化 | 保留显示子段；不执行 Entity／Grid／规则消费者 |
| Render 自己完成 | 阴影、光照集合、坑纹理、水面预绘制、water shader、实体 light／water clip | 原生 Game::Render／Room::PreRender／Render；RE-5 记录漂移，避免重复调用 |
| 不从 Room 更新补算 | 环境音频、雨与水流粒子、装饰 Entity、门／奖励／进度、Host 选中的主题和资源 | 状态／事件／资源引用；RE-7／RE-8／RE-9 按原范围承接 |

## 完成口径与未决项

RE-2 要求的八类职责均已有定位、分类和消费替代：EntityList、Grid、清房与奖励、相机、背景、水面、光照、生成。白名单有明确前置条件；未命名的规则 helper 和未分类分支继续默认拒绝，不把它们包装成纯显示，也不要求把所有未知字节或所有 Render 子调用穷尽后才交付。

本次未实现新的分派／资源构建，未声明当前 Client 已支持这些显示更新。FX 资源筛选中哪些纯外观随机可本地产生、哪些属于 Host 已选结果，需要随需求清单落源；完整 RoomInit、Backdrop 混合链和各类异常也没有通用回滚结论。帧计数及停顿／跳帧归 RE-3／RE-4，Render 写集归 RE-5，音频归 RE-8，转场换房点归 RE-9，实际表现与性能只在第 15 步统一验收。

本次收尾仅静态文档审阅：核对任务范围、来源、地址／ABI、原生顺序和现有 owner 分工。没有运行时观察或 E2E PASS。
