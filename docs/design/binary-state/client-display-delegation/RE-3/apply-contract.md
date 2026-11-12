# RE-3：帧号安装点与实现交接

返回：[成果总览](README.md)。本页为 `DESIGN`／`REUSED`，没有实现或实机结论。遵守[帧号方案 B](../../client-display-delegation.md#3-帧号方案b已定)与[第 13 步](../../13-client-apply.md)，不新增第二套展示时钟。

## 1. 成功提交处写入的值

| 原生字段 | 同一目标 world 的来源 | 写入规则 |
| --- | --- | --- |
| Game `+0x25A70` | 帧头 game_frame 的32位值 F | 主线程安装事务完成的提交点写入，保持到下一次成功提交；无 SDK 帧 setter，不用完整 Init／RestoreState 达成 |
| Room `+0x11F0` | Host 原始 Room 基准 B | 按实际字段差异安装；保持原值，不能改成“本机 F−远端年龄” |
| Entity `+0x328` | 各实体 Host 原始基准 E | 同上，不能为了临时 draw 逐个重新锚定 |
| Game `+0x25A74` | 当前 `time_counter_base` C | `bit_cast<i32>(u32(F−C))`；原生可修改计时，不强行等于 F |
| Effect `+0x45C`（仅保留此需求时） | 当前到期帧 A | `bit_cast<i32>(u32(A−F))`；保持有符号结果和负值，不运行玩法更新 |
| 仍使用锚点的其他字段 | 各字段已核编码和活动条件 | 采用字段专属 writer；Familiar FireCooldown／Pickup DropDelay 当前为原值，不属于这一组 |
| Q8 Game／Room 内联显示标量 | 同目标 world 的 Host 当前值 | 不减一／趋近／扩波纹／累加横移；震屏 countdown 正数变0时清本地偏移等阈值动作在安装点补做 |
| Manager `+0x4A8B8` | 本地 Manager owner | 不在 world 帧安装中写 Host M，不从 D0 还原 |

`Game._frameCount` SDK 为 int，帧头为32位帧空间。换算采用 u32 模2^32运算，再按原字段位模式解释，避免 C++ 有符号溢出；既有 [anchors.h](F:/projects_isaac/issac_multiplay/mod/src/state/anchors.h) 已体现这种语义。B／E 是绝对基准，不是剩余量，不能经过 Countdown／ExpireFrame 编码。

## 2. 同一主线程安装点的顺序

1. **准备**：接受层给出不可变的目标 world、身份／代次、帧头 F 和完整必要输入。先完成资源／对象／关系准备，按候选 F 计算 TimeCounter／Effect 等最终值，不以当前旧 F 换算。
2. **安装**：在既有 NativeBuild／NativeApply owner 内安装 world、原始 B／E、资源与必要值，包括 Q8 当前值和对应阈值补做。实际安装未完成时不允许显示推进或 Render 观察半套世界。
3. **提交尾部**：与第13步 13.8 的 F写入→计数还原→注入顺序一起，组成同一个不可观察的提交尾部。写 F 之前已完成可失败的准备／换算并确认剩余写入可完成；全部完成后才发布新的 DisplayReady／应用事实。不能把“先写 F，后面慢慢补齐”称作成功安装。
4. **显示**：在完整目标可见后运行 RE-1／RE-2 具名显示职责，再进入原生 Game::Render 包装。Render 前确认读取的是这一已应用目标的 F／Room／对象，而不是到包帧、输入预测 tick 或网络线程中的候选。

安装写前失败时 F 不前进；部分写入后停止显示并交原 owner 恢复／清理，不能继续显示旧 world 或 ACK 新 world。若实现中提交尾部仍可能失败，必须移出这些可失败工作或提供一致的失败恢复，使 F 不作为已提交新时间泄漏；本次不声称有适用于所有原生资源／异常的通用原子回滚。

这里的顺序是**可观察提交合同**，不是允许后台直接写 Game，也不是承诺每项原生资源构造都可回滚。UI 查询／Lua／显示 helper 如会读取 F，同样不能在安装中途拿到新 F 与旧对象的组合。

### 空帧、停顿、重复帧与回退

- **空帧**：即使没有 Entity／Room 块变化，成功接受的新 world 帧头也推进 F；必须更新全部受 F 影响的已保留到期帧／基准换算值，不能只遍历本包 changed 字段。
- **停包**：不改 F／B／E；按 F 求值的年龄／shader 相位保持，Q8 当前值不自行变化，按本地调用推进的 UI／FX 可继续。禁止为追上墙钟自己加 F。
- **重复**：同一事务不重复提交／ACK／播放事件；新的有效 world 若 F 恰好相同，仍按接受层的序列与版本处理实际字段变化。F 不是事务 ID，也不是 C/S Authority tick。
- **跳帧**：安装目标 F 后直接求必要计数；不补跑 ΔF 次 Game／Room／Entity Update，不用旧 A／旧 roster 重演中间世界。
- **回退／重建**：J273 RestoreState、NextVictoryLap 等已证明 F 可恢复或清零。按实际 world 身份／版本接纳合法目标，不把 F 的全局单调性当接受条件；旧对象／事件资格仍由既有代次与生命周期撤销。

例如 Host 在 F=100 时 Effect Timeout=5，则 A=105；后续 F=101、A未变时还原4。若该 Effect 实际停走为5，Host 每窗重算得 A=106 并发出变化；Client 不能把“曾线性递减”当后续永久假设。TimeCounter 的正常 C=F−T 可保持不变，但公开改时会产生新 C。

## 3. Manager 的本地推进责任

当前 [gen_binding.py](F:/projects_isaac/issac_multiplay/mod/tools/gen_binding.py:891) 的 manager_update wrapper 在 FrameScope 后调用 ClientManagerBoundary，返回 true 就不进入 `super()`；[ClientManagerBoundary](F:/projects_isaac/issac_multiplay/mod/src/game/client/session/client.cpp:575) 在 Client 活动时处理 scene／网络／输入并最终返回 true。该函数没有递增原生 M。故当前 Manager 自然递增 `0x92EC09/0x92E441` 与原生 Game 调用点也不会执行。

**DESIGN**：

- 继续复用这个已知主线程边界与统一分派 owner。若仍不运行完整 Manager 原链，明确由同一 owner 在本地 Manager 周期中推进原生字段 M、调度显示；不为取得 M 而放行启动／菜单／Online／玩法。
- 按现行 4.1／6.2，M 偶数时运行30Hz本地显示批次，相机作为每个 Manager 帧的独立阶段，并遵守 Host 暂停／特定转场例外；不运行 Odd 玩法。原生关闭 interpolation 的奇数快返另记为引擎事实，不为复现它恢复完整 Manager 原链。不能把30Hz职责也无差别地每个 Manager 调用执行。
- 原生相机读本轮 M，Manager 随后在尾部递增，Render 读推进后的 M 并形成 `(M+1)%2`。后续实现应保留这一先后关系；不能在相机和 Render 之间临时换 Host M，或在两处各加一次。
- 本地一次推进按实际主线程周期／用途帧去重，与到包数量和 Host F 是否变化无关。原链透传时不再额外加一次；退休／恢复透传同轮不能混合两个 owner。原生条件阻止推进的情况见[写入表](native-clocks.md#6-manager-的本地帧)，不把 M 描述为任何条件下都必然加一。
- F 只在 world 成功提交时改变；M 只在其本地 owner 推进。这两个字段本身就承担两种时间，不另立展示时钟、Host Manager 镜像或墙钟累计器。

主方案 4.1／6.2 已改为 Manager 边界内两档调度；本次补齐的是 M 不会自然前进的静态结论、原生写入路径与相位顺序，不撤回这项现行设计。尚未实现，不能宣称本地60Hz已测得。RE-4 继续决定各计数的重复／跳帧策略；调度实现需与 RE-1 相机及 RE-2 FX 的实际依赖一起落源。

## 4. 继续拦截与写回的范围

完整 Game::Update、Room::Update、Room::Init、Entity／Grid Update 维持前两份 RE 的判定；NextVictoryLap、RestoreState、公共 Game 重置 helper 也不进入显示白名单。当前方案中普通 Game 帧递增已经被根隔离，没有必要再挂一条写回 Hook。

若后续某个**明确放行的具名显示路径**确实写了 F／B，必须按该路径处理：优先排除原生写入；按主方案采用写回时，也只能在同目标 owner、Update 返回后且所有显示消费者／Render 前恢复该目标的 F／B。仅在 Render 前恢复 F 不能补救此前显示查询已读错帧，更不能撤销完整玩法／生命周期动作；不能用无条件覆盖遮住未知重入或旧 Room 指针。

Render 包装不再借用或恢复 F；它检查已应用帧身份并按 RE-5 处理其他漂移。若未分类代码引起世界身份变化，停止相关显示，按真实生命周期处理，不把旧 F 强行写入新 Game／Room。

## 5. 旧代码的退役与替换点

| 现有路径 | 当前行为 | 后续按方案 B 替换 |
| --- | --- | --- |
| [client_presentation_native.cpp](F:/projects_isaac/issac_multiplay/mod/src/game/client/presentation/client_presentation_native.cpp:825) 的 FrameBorrow | 保存 Game F／Room B，draw 时装另一份 F，结束恢复 | 随手工绘制退役；用主线程安装点的持久 F／原始 B |
| [client_entity_draw_native.cpp](F:/projects_isaac/issac_multiplay/mod/src/game/client/entity/client_entity_draw_native.cpp:136) 及同文件238附近 | 绘制 scope 借用 Host M／F 并恢复；多处以派生 FrameCount 重新锚定 `+0x328` | 取消时间借用和重锚定；本地 M、已应用 F 和 Host 原始 E 由各自 owner 管理 |
| [client_room_native.cpp](F:/projects_isaac/issac_multiplay/mod/src/game/client/room/client_room_native.cpp:132) 的 ClientRoomFrameMaintain | 用当前本机 F−传入年龄重算 B，还写 initialized | 拆开真实 owner／初始化资格与原始 B 安装；不再每次展示重算 |
| [client_entity_common_native.cpp](F:/projects_isaac/issac_multiplay/mod/src/game/client/entity/client_entity_common_native.cpp:51) 的 Entity.FrameCount 特例 | 读取 F−E 作对照，安装时用 F−年龄反算 E | 正式二进制路径直接装 E；不能继续以旧公开年龄字段构造替代锚点 |
| [run_source.cpp](F:/projects_isaac/issac_multiplay/mod/src/world/binary/run_source.cpp:16) | 从同窗口 F 与实际 TimeCounter 产生 C | 采集语义继续可用；Client writer 采用该目标 F，还原位置移到同一安装提交点 |

本次未删除、改写或运行这些产品文件；旧记录中的构建／测试结果保持历史事实，不能当成本次方案 B 已落地。字段／资源裁剪、第13步调度与第14步退役仍按整体计划继续，第15步再统一覆盖正常／空帧／跳帧／暂停／失败／回退／换房表现。
