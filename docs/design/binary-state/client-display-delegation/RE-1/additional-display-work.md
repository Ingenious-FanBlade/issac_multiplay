# RE-1：遗漏的显示工作与拆分边界

返回：[成果总览](README.md)。这些是 `Game::Update` 内候选函数以外的显示工作；本表定位后续需求，未实现值推进或调整布局源。

## 1. 已核内联工作

| 显示工作 | J273 位置／字段 | 静态行为 | Client 处理边界 |
| --- | --- | --- | --- |
| Bloom 计时 | `0x6F6D91—0x6F6D9C`；Game `+0x25AA0` | 正数递减；Render 还使用 `+0x25AA4/+0x25AA8` 的效果参数 | 从需求清单取必要当前表现状态；可拆标量计时，不调用产生该效果的玩法函数 |
| 震屏计时与结束复位 | `0x6F6DA2—0x6F6DCF`；`+0x25A80`、`+0x25A84/+0x25A88` | countdown>0 时减一，到零清偏移；实际随机偏移由相机更新产生 | countdown／触发来源须安装或用已定状态表达；两个相机入口产生本地外观偏移。不能只调用 Camera 而漏掉计时，否则会持续震动 |
| Hallucination 计时 | `0x6F6DD7—0x6F6DE2`；`+0x25AB0` | 正数递减；Render 还依赖总时间 `+0x25AB4` 和本地历史画面 | 不调用 `ShowHallucination` 补计时，该生产函数还会选择背景、随机并改实体；仅用表现状态和本地历史资源 |
| 六分量颜色趋近 | `0x6F6DE8—0x6F6E6C`；flag `+0x6691C`，current `+0x66920`，target `+0x66938`，step `+0x66950` | flag 非零时调 `0x6EB990`；逐分量按 step 向 target 趋近，六项相等时清 flag | 可以独立调用 helper 并复现完成位；current／target／step 是原生颜色状态，不需要重新运行玩法或共享 RNG |
| 闪电衰减 | `0x6F6E7B—0x6F6EA6`；`+0x6699C` | 正值大于 0.05 时乘 0.75，否则归零 | 可拆纯标量推进；触发／初值由 Host 决定 |
| 次级淡入／开场遮罩 | `0x6F75AF—0x6F76C8`；`+0x25B10`、speed `+0x25B30` | alpha 下降与 wall-clock／联机状态／开场完成混在一起；完成会 ShowName、更新 manager 相关状态 | 不复制整段等待或按本机墙钟裁决开场结束；采用 Host 已决定的表现状态，或只抽取明确的 alpha 运算 |
| 暗度平滑 | `0x6F78F4—0x6F794A`；`+0x25A8C` countdown、`+0x25A90` intensity | countdown>0 时 intensity 加约 1/21、上限 1，再减 countdown；否则减约 1/30、下限 0 | 可以拆出标量推进；不能将“显示暗度”与清房／诅咒规则重算混在一起；与已有同步 current 的关系由漂移校正处理 |
| 眩晕强度 | `0x6F7954—0x6F799A`；current `+0x669A0`、target `+0x669A4`、step `+0x669A8` | current 每次按 step 接近 target，差小于 step 则直接到 target | 可拆纯标量；Host 提供必要目标／表现状态，不运行触发眩晕的玩法 |
| Retrovision 倒计时 | `0x6F79A2—0x6F79AD`；`+0x25F58` | 正数递减；Room Render 以该值及 `+0x25F5C` 总时间算淡入／淡出 | 需要相应计时状态／到期帧，不能只同步永久 seed effect 布尔；计时锚点并入 RE-3／RE-4 |
| 周期 HUD 刷新请求 | `0x6F78C5/0x6F78CF`；`+0x22E94/+0x22E9C` | GameFrame 每 30 帧满足条件时设 type=1、countdown=2；type=2 不覆盖 | 请求是显示维护，可以在自有显示根保留；不连带复制前面的 GameFrame／TimeCounter 自增 |

这些值不能在安装 Host 当前值后无条件再减一次，就宣称仍等于 Host 当前帧。第 13 步必须明确哪些属于 Client 本地相位，哪些在 GameFrame／到期帧上求值，哪些仍为同步的表现状态；RE-4 负责停帧／跳帧风险。当前报告只核定运算与边界，不创建第二套展示时钟。

## 2. 淡入淡出必须拆分

原版 Wiki 的 FadeoutTarget 已列出选档、标题、新局和胜利圈等目标。J273 静态代码同样直接显示出副作用：

- `Game+0x25B28` 是 primary 当前 alpha；`+0x25B30` 是 speed；`+0x25B34` 是非零时向淡出推进并最终选择目标的控制值。
- `0x6F6EB0—0x6F6EE8`：target 为零时做减 alpha／下限零的纯显示运算。
- `0x6F6EF3—0x6F6F2A`：target 非零时加 alpha／上限一；后面立即进入 switch，**不能继续执行**。
- `0x6F6F3D—0x6F728D` 的完成分支修改 Manager 状态、调用 `0x931F40` 等退出链、进度／平台相关 helper，并可能进入下一局。`0x6F7151→0x6F3E10` 也是完成后的原生动作之一。
- `0x6F729E` 还改 Manager 状态；`0x6F72B5` 改 fade 控制值；它们不能成为 Client 的本地权威决定。

**结论**：整条 fade FSM 不进白名单，也不把某个原函数中段地址强转成可调用入口。只从已同步的当前颜色／alpha／必要状态绘制，或在自有函数中复现已核的纯 alpha 推进并在到达目标时钳住。退出／保存／换局／开场完成只能由 Host 已决定的生命周期驱动。若 current alpha 已按状态安装，不能再叠加一次推进导致漂移。

## 3. 相机的真实写入与随机

两个相机入口读取已安装 Player 的位置、方向／攻击相关显示输入、focus entity、当前 Room 几何与门位置，以及本机视口；它们更新本地 ring、平滑与 Room scroll，不执行实体模拟。

- `UpdateUltrasmooth` 的焦点读取包含 `Entity_Player::GetFocusEntity 0x771D30` 与 EntityList 类型查询；依赖实际 Player／攻击实体关系已经安装，不能用空关系或旧房间缓存代替。
- Ultrasmooth `0x91E5B1/0x91E5BA`、Drag `0x91E907/0x91E913` 写 Room `+0x1204/+0x1208`。
- Ultrasmooth `0x91E456/0x91E463`、Drag `0x91E822/0x91E82F` 写 Game `+0x25A84/+0x25A88` 屏幕震动偏移。
- 震屏条件下 Ultrasmooth `0x91E274/0x91E2E3`、Drag `0x91E676/0x91E6DA` 调 `0x6EB4B0`，SDK 将其声明为 `Isaac::genrand_int32`／全局 `Random()`。不能声称两个入口没有随机。
- 相机使用的是 Client 自己的 RNG 状态和本地 ring，按 Q4 接受外观差异。不要从 Host 下发 RNG 游标，也不要把这份状态用于权威实体更新。若旧 Room scroll／screen shake 布局仍在同步，后续要改为本地显示或明确漂移写回，不能由两个 owner 互相覆盖。

RE-1 按方案第 6.1 节只核这两个更新入口及其直接显示依赖，没有恢复旧“全相机／全虚函数闭包必须穷尽”的研究要求。公开 Camera:Update 的存在也不等于本轮已经接通相机调用。

## 4. 明确没有顺带放行的部分

| 路径 | 保留拦截的原因／后续归属 |
| --- | --- |
| `Game::Update` 前段 Console／模式 helper、PauseScreen／GameOver／Leaderboard 等 | 含输入、模式／菜单与生命周期，不因具有 UI 外观就认作纯显示 |
| `0x6F77BB→0x41CB40`（Game+0x1D294 演出 owner） | 不在已核候选；保留默认拒绝与必要同步的演出状态，不猜为纯显示 |
| `Game+0x669EC` 相关全屏覆盖与 `+0x67FD0` 等倒计时 | 普通更新会在结束时调用转场入口 `0x6F7829→0x6F9B30`、`0x6F7884→0x6F9710`；不能只看 Render 读它们便放行生产链 |
| `0x6F78E4→PlayerManager::Update` | SDK 已识别为 PlayerManager 更新；不为 HUD roster／属性刷新重新运行 |
| `0x6F78EF→0x4210F0`、末尾 debug 容器清理等 | 未进入显示候选，不整段借用 Game 尾部来获取少量标量运算 |
| `ItemOverlay::Update`、Boss／TV 演出、StageTransition 更新 | 继续按方案使用状态／事件；Boss 与转场内的混合分支交 RE-9，不能以本次入口核定替代副作用分类 |
| Room 背景／水面／光照推进 | RE-2 范围；本任务未列为“没有遗漏”，只是已把 Game 外壳的遗漏单独定位 |
| Game／Room／Entity 帧号本地递增 | 不属于显示推进；由安装事务方案 B 对齐，RE-3 定位与处理 |

## 5. 后续应覆盖的失败方式

本次没有代码或测试；以下交第 13 步实现和第 15 步统一验收，不在 RE-1 中试跑：

1. world 半套状态、旧 Room／Camera、失效 PlayerHUD 关联被显示入口读取。
2. 外层再调 PlayerHUD、排序、地图刷新，或照抄全部提前返回分支，使同批次推进多次／音效重复。
3. 只接 GameUpdate hook 而 Manager 更新仍被整体跳过，显示根根本没有被调度。
4. 两个 Camera 更新连续调用，或将 Host GameFrame 奇偶当 Client Manager 半帧参数，造成平滑／override 错误。
5. 漏震屏 countdown 或内联显示标量，出现永久震动、闪电／暗度／fade 停在旧值。
6. 淡出完成触发菜单、保存、新局；演出结束触发转场；生命周期副作用绕过 Host。
7. 用 UpdateVisibility 重算 Client 门／探索，覆盖权威 DisplayFlags；只调 Minimap Update 而缓存仍旧。
8. 在网络线程／无 context／无图形资源时刷新地图，或刷新目标关系错误导致后端提交／清屏异常。
9. 远端 PlayerHUD／Minimap 查询落到本机物理设备；规则 getter 读本机 PGD 或错误概率来源。
10. 空 renderEL、scratch 不足、渐进排序未完成或双 owner 移除／排序造成悬空引用。
11. 颜色、darkness、timer 等既同步又本地推进，增量未变时逐帧漂移；到包时重复推进。
12. 断网／暂停／合帧时，按相等帧或每次调用计数的显示消费者漏触发或重复触发；转场在等待目标时继续换房。

资源异常、图形 context 丢失、特殊角色／大房相机、April's Fool、暂停与停包的实际外观都尚无本次运行结论。静态完成只表示 RE-1 的定位与分类产出已经形成。
