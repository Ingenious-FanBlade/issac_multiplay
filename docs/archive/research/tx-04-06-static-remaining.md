# tx-04-06：ST2–ST9 静态确认与主审

日期：2026-10-25。承接 [ST1 报告](tx-04-06-static-confirmation.md)，完成实现方案所列剩余八项静态研究。继续使用用户指定的 **GPT-6 Luna / max** 子智能体，主智能体独立通过 IDA MCP 读取函数、原字节、调用方与本地官方 REPENTOGON 1.1.3 源码审阅。这里只修改研究资料与方案，没有实现功能、修改 IDB、安装 Hook、启动游戏、构建或运行测试。tx-04-06 的唯一整体 E2E 尚未执行。

本文地址均为 **RVA**；本轮 IDB imagebase=`0x400000`，工具 VA=RVA+imagebase。`STATIC_CONFIRMED` 表示实际指令及明确控制流，`SOURCE` 表示本地官方源码，`STATIC_INFERENCE` 表示带前提的推导，`DESIGN` 表示后续实现选择；均不表示运行时观察。原始资料见[证据目录](../../../artifacts/repentogon/tx-04-06-static/README.md)。

## 1. 结论与适用边界

**ST2–ST9 的指定静态问题已逐项研究并经主审；原方案不能直接按“四处补丁加原生注册/移除”实施。** 本轮确定了 Join、角色参数、退役 ABI 和字段写入顺序，也找到了设备通知触及原版联机、UI 读取派生尾部、退出写存档等反例。静态研究完成不表示这些兼容问题已经实现解决。

| 项 | 本轮结论 |
| --- | --- |
| ST2 | 设备 vector 存裸指针；id 由构造时全局递增取得。注册有映射配置回调，移除有原版 Lobby/Netplay 回调；退出 run 不等于 InputManager 关闭 |
| ST3 | 普通 K 可采用两个合格 Join pass：第一 pass 只报告 Triggered(19)，第二 pass 只报告 Triggered(14)；查询须在整个 pass 内一致，不能按首次查询消费 |
| ST4 | 三处均为完整 5 字节 `E8 rel32`；第三处 key 在 ECX，且原函数保留它，shim 必须保留该行为 |
| ST5 | 工厂只有一个 int32 role 实参；CALL shim 入口 `[esp+4]` 是 role。拒绝清栈 8 字节是 shim 返回地址加 role，不是两个工厂参数。角色表为 int32 PlayerType |
| ST6 | `RemoveCoPlayer=0x590560`，thiscall、两个栈参数、`retn 8`；有掉落、清关联、移除实体、重编号等世界副作用，不是简单删 vector |
| ST7 | playerIndex 在 append/Init 前按现有完整 Player 数设置；Type/Variant/SubType 与初始 controller 在 Init 内写入，K 的 controller id 在工厂返回后由 Join 覆盖 |
| ST8 | 原生/官方回调链存在条件性解锁、统计与配置写入路径；Game::Exit 会保存 dirty PGD，且先清 dirty。不能承诺创建/退役的持久副作用为零 |
| ST9 | 13 槽中性不足以保证 UI 安全：物理确认能选中 K；暂停断连处理还会枚举并直接读每个设备 `+0x48C/+0x498` |

后续实现必须处理的具体问题是 **K 的生命周期通知隔离、UI 的可见性/直接读取、原生创建和退役副作用**。它们是本轮发现的功能缺口，不是新增 gate；没有扩大到死亡/ghost 偏移、实机异常 Join 或任意第三方插件的全映像证明。

## 2. ST2：注册、移除、通知与保留

### 2.1 存储与 id

`STATIC_CONFIRMED`：InputManager vtable `0x732E44` 有 **47 个函数槽（0–46）**。紧邻的下一项是另一张表的 RTTI COL `0x745F4C`，不能当成第 47 槽函数。第 45/46 槽分别为 `0x5EEB40/0x5EEBB0`，均为 ECX=manager、一个 `device*` 栈参数、`retn 4`。

manager `+4/+8/+0xC` 是 begin/end/cap，元素是裸 `device*`；没有原型的 object/control pair。基类构造 `0x5EC570` 在 `0x5EC5FC` 读取全局计数器 `0x7E65EC`，把值写设备 `+8`，在 `0x5EC605` 递增。删除前面的元素会改变 vector 位置，不改剩余设备 id；不能把 id 当 vector ordinal 或复用为网络身份。本轮该计数器指令没有 atomic 前缀；不能从主线程创建规则推导物理热插拔构造与 K 构造已经串行化。

### 2.2 注册有回调，不能视为纯 append

`0x5EEB40` 在 manager `+0x48` 锁内 append（`0x5EEB61`），解锁（`0x5EEB79`）后遍历 manager `+0x14/+0x18` 的 `{function,context}` 记录，以 `(deviceId,context)` cdecl 通知（`0x5EEB8D`）。注册 body 没有直接弹“已连接”窗口，但回调是实际副作用入口。

主审继续定位到 Manager 初始化 `0x52BA20` 将 `0x534690` 安装在该列表；`0x534690 → 0x42A520` 访问 Manager `+0x4A8D4` 的映射配置集合。后者用设备槽 3 的名称匹配 profile；已有名称时经槽 33 载入 31×2 项映射，新名称时复制名称并新增一个 `0xFC` 字节记录，经 `0x42A470`/槽 32 读取映射。名称因此必须始终有效且非空。

这一步新增的是**内存中的配置记录**，不能说注册当场写文件。后续 `0x534580 → 0x42AC90` 会序列化该集合；`0x42ACD5` 是官方 `SaveButtonMaps_InputConfigsPath` 特征的唯一匹配。`SOURCE`：`SaveSyncing/InputConfigs.cpp:28–45` 把目标从 `inputconfigs.dat` 改为 `inputconfigs_rgon.dat`。因此“设备不会引入任何持久配置变化”也不成立。

### 2.3 三种删除与原版联机隔离

主动移除 `0x5EEBB0` 先通知 manager `+0x20/+0x24` 的断连回调（`0x5EEBD6`），再调用 `0x5EECA0`、erase vector、设备槽 2 和槽 0(flags=1)。`0x5EECA0` 在设备 `+0x0C` 为真时清活动数组并通知 manager `+0x38/+0x3C`，然后清活动标记。

Poll=false 的 `0x5EDCC0` 和物理断连 `0x5EC2E0` 也调用同一断连回调集合。Manager 初始化在 `0x52C341` 直接调用原生 Lobby 构造 `0x4E3FC0`；后者从 `0x4E40CB` 起注册 `0x4EE980`，不是只有用户进入在线模式才安装。该回调访问原版 Lobby/Netplay 对象并处理其用户设备。即使没有大厅用户，也不能把进入该回调称为“不触及原版在线状态”。IDA 把构造调用错误命名成标准库时区函数，实际 E8 目标与 body 才是依据。本轮只静态阅读，未进入原版在线模式。

`DESIGN`：K 需要在**通用输入设备分派边界**按自有对象身份处理注册、主动删除和 Poll=false 通知，使 K 不进入原版联机回调，也不新增持久控制器 profile；物理设备保持原逻辑。不通过 Hook/复用 OnlineManager、NetInputDevice 或改全局 callback 列表实现隔离。具体常驻 shim 尚未编写；旧“四处新增补丁足够”的数量不再成立。

K 槽 26 恒 false 可避免其通过日常活动判定进入 manager `+0x10` 的四项活动数组，但不能消除上述无条件断连通知。NEVER 的槽 0/2 也只解决资源保留，不解决通知副作用。

### 2.4 run/menu 与线程

`STATIC_CONFIRMED`：`0x5EC040` 初始化输入子系统并创建物理轮询 worker `0x5EC450`；关闭 `0x5EC110` 设置停止位、join worker 后调用 `0x5EDAA0` 清设备。`Game::Exit 0x2F6270` 的已读路径调用 `0x58F7C0` 清 PlayerManager，没有以输入子系统关闭代替 K 的显式退休。输入对象为全局生命周期，普通退菜单不能依赖它自动删除 K；重新开 run 也不能把旧 K/旧 id 当新会话复用。

物理断连 `0x5EC2E0` 从 vector 第 1 项起遍历，先比较设备 `+0x3F8` 与平台断连 id，再做删除；自有对象在此偏移放任意私有状态存在误匹配风险。工作线程每约 100 ms 进入后端轮询；本轮没有证明 remove、构造计数器、注册与 backend callback 之间的全部并发关系。实现不能只因 K 的业务操作在主线程就宣称并发闭包完成，也不能仅选一个未经证明“不可能”的 sentinel 值消除该路径。

## 3. ST3：Join 查询、状态机与最小序列

### 3.1 已读分支

`STATIC_CONFIRMED`：Join `0x591360` 在 `0x59138F/0x591395` 比较 Manager `+0x4AFB8/+0x4AFBC`；该原版在线容器非空时跳过设备/选人处理，仍进入函数尾部的已有更新。这是静态控制流，不授权 C/S 进入原版在线机制。

普通路径先枚举设备，以设备 `+8` 的 **id** 检查它是否已由完整 Player、possessor 或待选 Join 行使用。已有占用、无空闲行、原生加入阻断标志、人数达到 64 等条件会跳过新建；Baby 模式还会检查宿主健康条件。scope 自身仍限定单一完整 A、无阻断、正确 manager 和主线程。

| 调用点 | 查询 | 作用 |
| --- | --- | --- |
| `0x591682`，随后 `0x591699/0x5916B0` | Triggered(19)，Pressed(8)、Pressed(10) | 三者同时为真走 possessor 特殊分支；K 的 8/10 必须保持 false |
| `0x59161A` | Triggered(19) | 原生加入阻断分支中的提示反馈，不是普通创建 owner |
| `0x59172C` | Triggered(19) | 未占用设备申请 Join 行；`0x59173C → 0x5911C0` 取得行 |
| `0x591794` | 设备槽 17，action=19 | 普通选择初始化时屏蔽该 action；保留有效基类映射与状态资源 |
| `0x591856` | Triggered(19) | 已有非完整 co-player 的转换/退出相关路径，K 不应靠它反复加入 |
| `0x591E7E` | Triggered(20) | 选项向前，环绕索引 |
| `0x591EFA` | Triggered(21) | 选项向后，环绕索引 |
| `0x591F7D` | Triggered(14) | 确认当前角色，进入普通创建 |
| `0x591F97` | Triggered(15) | 取消当前选择；某些原生自动确认分支会先跳过此查询 |

各 action 编号均为十进制。`0x5EE200/0x5EE140` 分别分派设备槽 9/8。原生可在一个 pass 内多次查询同一个 19；K 不得把第一次 true 后的查询改成 false，否则结果会随查询顺序改变。

### 3.2 两个合格 pass 的设计

普通新行在 `0x591797–0x5917AF` 初始化 role=0、selected=1、显示状态和自动确认位；挑战/特殊配置走 `0x591807–0x59181E`，可设置不同 role 与自动确认位。设备枚举循环结束后，同一次调用还会走选人行循环，所以“原生必需等待两帧”不是事实；报告多个动作或原生自动确认都可能让一个 pass 创建完成。

`DESIGN`：受控、普通新游戏选择最小且可追踪的 **两 pass**：P1 中 K 的槽 9 对 action19 始终 true，其余动作以及 Pressed(8/10) 为 false；观察 scope 返回后行确已分配给 K。P2 中 action14 始终 true，19/15/20/21 等其余为 false。P2 在创建点替换唯一角色参数。报告由 pass 的固定快照决定，在 pass 正常返回时推进状态，不能按查询次数推进，也不能按渲染帧/墙钟倒数自动尝试。

这两个 pass 指两次**条件合格的 Join 调用**，不保证连续两个渲染帧。若 P1 已因原生自动确认产生 B，立即按实际结果收口，不能再发 P2 造成二次创建；若受阻或超出支持条件，记录原因并退休，不能宣称任意 run 都恰好两 pass。每会话仍至多一次创建尝试。

## 4. ST4：三处 mode 调用点

| RVA | 完整原字节 | 原目标 / 恢复位置 | 进入调用点时的上下文 |
| --- | --- | --- | --- |
| `0x5915AE` | `E8 ED FA FF FF` | `0x5910A0` / `0x5915B3` | manager=EBX，key=ESI |
| `0x59174C` | `E8 4F F9 FF FF` | `0x5910A0` / `0x591751` | manager=EBX，key=ESI；EDI 此时是 5×行号，不是第三处的行地址 |
| `0x591E67` | `E8 34 F2 FF FF` | `0x5910A0` / `0x591E6C` | manager=EBX，EDI=manager+0x14+0x14×行号，key=ECX=`[edi-8]` |

`STATIC_CONFIRMED`：每处可替换的完整指令都是 **5 字节**，没有覆盖下一条 `test al,al`。原 predicate `0x5910A0` 从 Game 的 run flags `+0x25AC0` 检查位 `0x10`，以 AL 返回 bool；无栈参数、普通 `ret`，不使用 manager this、不改 ECX/ESI/EDI/EBX。若搬入 trampoline，E8 位移必须按新地址重算，不能原封复制 rel32。

第三处调用之后 `0x591E76` 会继续 push ECX 作为 device id，虽然通常 ABI 把 ECX 视为 volatile，这里也必须还原原 predicate 实际保留 ECX 的行为。shim 在任何 C++ helper 之前捕获 key；原调用恰好一次，只对 scope 中的 K 改写 AL，其他设备/未 armed 完整透传。保存 GP、EFLAGS 与原方案的浮点/SIMD 环境；不修改全局 run flags，不把三处都套用同一个错误行指针合同。

## 5. ST5：角色实参与角色表（按用户收窄范围）

本节只回答调用前 push 的来源、role 相对 ESP 的位置，以及 selected index/角色数组。既有点位、寄存器、行步长与 `+0x70/+0x74` 偏移不扩大重查。

### 5.1 一个工厂参数与 CALL shim 的两项栈内容

`STATIC_CONFIRMED`：在普通创建路径，`0x591FA9` 读 `[edi-4]` 的已选 role；`0x591FDF` **只有一条 `push eax`**，随后 `0x591FE0` 令 ECX=Game+0x1BAA8，`0x591FE6` 调 `SpawnCoPlayer2 0x590250`。工厂正常尾为 `retn 4`，返回值 EAX=`Player*`。

| 观察位置 | `[esp]` | `[esp+4]` |
| --- | --- | --- |
| 原生 E8 指令执行前 | 唯一 role | 原 Join 的既有栈内容 |
| E8 改调 CALL shim 后，shim 入口 | 返回地址 `0x591FEB` | 唯一 role |

因此原“调用前两次 push/8 字节实参”的说法被推翻。Join 序言中的保存寄存器、以及旁支日志的两参数清栈，都不是工厂第二个参数。另一个 possessor 调用分支把后续 SetControllerId 的两个参数提前留栈，也不能算成 Spawn 的参数。

但现有原型 `CreateHook` 是 **E8 调 shim，再由 shim tail-jump 工厂**。恢复 shim 私有保存区后，放行时 tail-jump 工厂，工厂 `retn 4` 会回到 `0x591FEB` 并清 role；拒绝时 `add esp,8` 清掉 **shim 返回地址 4 + role 4**，再跳 `0x5925E4`，保持既有正确栈深度。如果将入口改成 E9/jmp，则没有额外返回地址，拒绝只清 4 字节。当前方案沿用 CALL shim，不把 8 改成 4。

源码原型 pushfd+pushad 共 36 字节后，角色地址是 `[esp+40]`；该值与上表一致。主审和子智能体均曾把“一参数”过早解读为“清 8 一定错误”，已回到入口指令和原型 shim 更正，见[更正记录](../../../artifacts/repentogon/tx-04-06-static/st2-st9-review-corrections.md)。

### 5.2 selected 是索引，role 是表值

EDI=manager+0x14+0x14×行号；`[edi]` 为 selected index，`[edi-4]` 为当前 role，`[edi-8]` 为 key。20 分支在 `0x591EAD` 执行 `[begin+selected*4]`，21 分支在 `0x591F22` 同样读取，将 **int32 PlayerType** 写到 `[edi-4]`。begin/end 是 manager `+0x70/+0x74`。

确认时直接取行上保存的 role；仅 role=-1 时，`0x591FB5–0x591FD5` 按元素数量减 1 随机选择 `begin[index+1]`，跳过首项随机标记。自定义指定角色应在表中查找**值**，不能把 PlayerType 当 selected 下标，也不写 selected、原生角色表或解锁状态。特殊角色可能在 Init 内创建附属 Player，是否仍满足“恰好 A/B 两个”须按真实工厂结果确认。

## 6. ST6：移除 co-player 的 ABI 和副作用

`STATIC_CONFIRMED`：官方 `PlayerManager.zhl` 的 RemoveCoPlayer 特征唯一匹配 **`0x590560`**，ECX=PlayerManager，栈 `[esp+4]=Player*`、`[esp+8]=bool`，`retn 8`，无有意义返回值。`SOURCE`：`LuaPlayerManager.cpp:349–355` 使用 `RemoveCoPlayer(player,false)`；`.zhl` 明称第二项 `unusedBool`，本轮实际 body 未见读取它。因此传 false **不能**表示“无掉落/无副作用”。不沿用旧版 `0x5BA000`。

本轮已读正常 full Player（Variant=0）、Baby（1）、possessor（2）分支，结论分开使用：

- full Player 在当前房间条件满足时处理 pocket/饰品与主动道具掉落，调用 `0x36A8E0` 等；清理跟随物、附属武器/实体（`0x351C10`），可能递归处理相关 co-player。这些会改变世界与 RNG，不是可以凭空撤销的状态。
- Baby 分支还会向其他 Player 调整健康/资源；例如 `0x590780 → 0x352E50` 属于该分支，不能套成普通完整 B 每次必走。possessor 有自己的辅助 vector 与实体关联清理，当前 K 不接纳该类型。
- 正常公共移除路径 erase manager 的 Player vector，在 `0x590D0A` 虚调实体 Remove（本轮表目标 `0x2A8000`），在 `0x590D0D` 把被移除 Player 的 `+0x1610` 写 -1，并在 `0x590D2A` 起给剩余 Player 连续重编号；还重置 Join 选择状态、更新 HUD/关联实体与 controller 关联。
- 该函数没有代替 InputManager 删除 K，也没有在正常完整 Player 路径直接把 B 的 `+0x160C` 写 -1。实体 Remove 标记与“对象已析构/内存已释放”也要区分。

`DESIGN`：先停止输入并撤销 B 的 exact pair，再在仍有当前 manager/B 身份证据时调用一次 `RemoveCoPlayer(B,false)`；K 的移除单独处理。run teardown 已由 `0x58F7C0` 清 Player 时不能再对旧指针调用。NEVER 只针对 K，不允许用 stale `Player*` 保留网络身份。所谓“A 未受影响”只能针对明确不变量观察，不能扩张成所有字段、HUD、颜色或 RNG 均逐位不变。

## 7. ST7：返回前后的字段写入顺序（按用户收窄范围）

不再重复证明字段偏移，也不重查 death/ghost。下表为已有偏移对应的写入时机。

| 时点 | 直接指令 / 结论 |
| --- | --- |
| Player 构造 | `0x34E087/0x34E091` 将 controllerIndex/playerIndex 初始化为 -1 |
| 普通工厂、append 前 | `0x5903C4` 计算现有 Player vector 长度，`0x5903C7` 写 B `+0x1610`；只有此前恰好 A 一项才得到 1 |
| 颜色相关索引 | `0x5903E5–0x5903FD` 对既有 `+0x1614` 求最大值再加一；这不是 playerIndex 的分配规则 |
| append 后、返回前 | `0x590469/46A/475/477` 压 seed、role、Variant0、Type1；`0x590479` 虚调 Player Init=`0x34F6A0` |
| Init 内 | `0x34FD0B → 0x2A4A20`，后者 `0x2A4A35/3B/41` 写 Type/Variant/SubType；普通 Variant0 以指定 role 初始化；`0x34FCA2` 写专用 `_playerType +0x13BC` |
| Init 内初始 controller | `0x350C16–0x350C41`：Variant 非 0 或角色 20/40 用 -1，否则读取 Manager `+0x4A8D0` 的默认 controller，再调用 SetControllerId `0x39E1B0` |
| Init 尾 | `0x351A9F → 0x44A7E0` 分派 PostPlayerInit；此时尚未执行下面 K 的覆盖 |
| Spawn 返回 Join 后 | `0x591FEB` 保存 EAX，`0x591FED/0x591FEF` 压 true 与行 key，`0x591FFB → 0x39E1B0` 将 B 及关联子 Player 的 controller 改为 K |

因此在创建调用 wrapper 的 original 返回瞬间，可以检查类型、角色与 index，**不能要求 controller 已为 K**，更不能在 PostPlayerInit 回调就宣布 exact pair 生效。必须在整个 Join pass 正常返回后，重新确认完整 A/B、人数、manager、角色与 K id，再绑定。角色检查需分清实体 SubType 与官方 GetPlayerType 的专用字段；普通角色在本路径相符，特殊角色/官方 Hook 的转换必须据实际最终值判断，不能只看一处早期参数。

## 8. ST8：持久副作用与官方回调

### 8.1 原生写入入口和退出保存

主审按官方特征定位并读了实际写指令：

| RVA | 行为及条件 |
| --- | --- |
| `0x503930` TryUnlock | 检查 PGD readonly `+1`、achievementId 与已有标志；成功会写 `achievements[id]`、dirty `+0=1`，并可触发成就提示/平台路径及派生解锁检查 |
| `0x503A50` IncreaseEventCounter | readonly=false 时修改 `eventCounter +0x2BC`，在 `0x503A94` 标 dirty；后续按计数条件调用 TryUnlock |
| `0x503460` SaveLocally | 打开 PGD 本地路径，序列化、flush/close；不是单纯清 dirty |
| `0x502F00` SaveToSteamCloud | 打开 cloud stream 后序列化与关闭；并非每次运行均使用 cloud 分支 |
| `0x42AC90` SaveButtonMaps | 写上述控制器 profile 集合；官方改目标为 `inputconfigs_rgon.dat`，属于另一类持久配置 |

`Game::Exit 0x2F6270` 在 `0x2F635E` 检查 dirty，在 `0x2F6363` 检查 fileLoaded；随后 **先在 `0x2F636E` 清 dirty**，再根据 Steam/选项条件于 `0x2F639A` 保存 cloud 或 `0x2F63A3` 保存本地。稍后 `0x2F647B` 还会把 PGD readonly 复位 false。所以最终 dirty=0、某个退出快照只读位为 false、或创建函数里没有直接 Save，都不能证明整个窗口没有持久写入。

### 8.2 有条件路径，不把图的可达边当成每次必发

主审导出了一份深度 7、上限 2200 节点的直接调用图，实际 1797 节点，明确保留间接调用缺口。它用于寻找证据线索，不是无副作用证明；还包含条件分支和共享尾块，须回到 body 判定。

- Spawn 的 `0x590479` 是 **虚调用 Init**，直接调用图不会自动连边；人工已用 Player vtable 和指令连接到 `0x34F6A0`。Init 中角色 18 的尾部可经 `0x351A88 → 0x35D4B0` 进入 EvaluateItems 的条件解锁检查；`0x351A99 → 0x3899A0 → 0x358F00` 还可走道具处理。不是所有普通 Isaac 创建都走该特殊角色分支。
- 完整 Player 退役 `0x590AE1 → 0x36A8E0 → 0x3582B0` 会先 flush item queue；**只有队列非空且条目为相应道具种类**，后者才调用 `0x358F00`。该 AddCollectible 路径依据道具与状态可在 `0x359367` 调 `IncreaseEventCounter 0x503A50` 等，不能在没有队列状态证据时宣称实际发生。
- Baby 退役的 `0x590780 → 0x352E50` 在健康/模式条件满足时能于 `0x35306F` 调 TryUnlock；K 应为 Variant0，故此例证明 API 不是普遍无副作用，不证明 K 必定触发它。
- Join 的 Baby 专用创建 `0x59175A → 0x594340` 也有健康/道具副作用；K 的三个 mode override 选择普通路径，其它物理设备仍保持原生行为。

综上，静态可确认**入口和条件性风险**，无法证明真实玩家当前物品、PGD、Lua callback 状态使整个路径无写入。也没有授权为了消除风险临时翻转全局 readonly、回滚 achievements/counters 或伪造 dirty；它们不能恢复已经发生的平台通知、文件写入或回调副作用。

### 8.3 原生与官方回调

`STATIC_CONFIRMED`：Init 正常尾的 `0x44A7E0` 在 callback bit 开启时分派原生 **MC_POST_PLAYER_INIT（9）**；实体 Remove `0x2A8000 → 0x44CA50` 也会在 bit 开启时分派回调 **67**，所以退役同样会执行 Lua 消费者。`SOURCE`：官方 `CustomCallbacks.cpp:1737` 还先调用 GetHealthType，再调用原 PostPlayerInit。官方附加回调 **1127（InitPreLevelInitStats，super 前）**、**1042（InitPostLevelInitStats，super 后）** 在相应初始化 helper 被调用且 callback bit 开启时分派，不能笼统说“创建只有一个回调”。

其他已读官方 Hook 包括：`EntityManager.cpp:158/171` 的 Entity Remove/Player constructor bookkeeping；`EntitySaveStateManagement.cpp:2697/2705` 的构造和 Init 恢复状态整理；`XMLPlayerExtras.cpp:16` 的 XML 附加属性；`ItemSpoofSystem.cpp:340` 的 InitPostLevelInitStats 固有道具处理。这里的 save-state 整理主要是内存对象状态，不能仅凭文件名将其定性为立即写磁盘。

本轮没有对任意 Lua/第三方模块代码做穷尽分析，没有观察真实 Hook 顺序；官方源码也不能替代实际运行时安装证据。后续唯一 E2E 应在已有允许的夹具环境中覆盖完整操作窗口，区分 C/S 引入的写入与 D18/D20 允许的原生退出基线，不能只比较最终 dirty 或声称静态无直接 Save 就已通过 V12。

## 9. ST9：非 gameplay 中性与 UI 直接读取

### 9.1 中性策略实际覆盖的入口

ST1 的 raw 5/6/7、action 8/9/10、原始触发 11、活动 26/27 中性仍需要保留。MenuManager `0x561CB0` 用槽 26/27 选当前活动设备，`0x52E260` 用槽 11 的非 -1 结果选择输入控制器；标题页面 `0x5C4720` 用 action14/15。K 中性能避免**由 K 自己产生输入**驱动这些路径。KeyConfig `0x4B9AD0` 仍可对选中设备调用槽 30/33 重置/写映射，原始查询中性并不禁止外部 UI 写入这些基类数据。

### 9.2 反例一：物理确认选中高亮 K

ControllerSelect 更新 **`0x4AF870`** 将 InputManager 设备名/id 加入自己的列表，`0x4AF9AB` 调槽 3。随后枚举**所有设备**，`0x4AFAA1` 调槽 9、action14；任一物理设备确认即跳到当前高亮项的处理，不要求“产生确认的设备就是高亮设备”。

`0x4AFAE9` 从 `pairs[selected*8+4]` 取高亮行 id；**`0x4AFAED` 原字节 `A3 14 8A B8 00`** 写绝对地址 VA `0xB88A14`（RVA `0x788A14`，当前菜单控制器 id），并切到 KeyConfig 页面（事件 12）。因此 K 即使所有动作中性、活动为 false，也会被物理确认选中。IDA 自动名 `ArgList_0` 不是栈局部，不能据名称否认该全局写入。

随后的 KeyConfig 绘制 **`0x4B8C70`** 在 lookup 成功分支于 `0x4B8D3D/0x4B8D46` 直接读取设备 `+0x48C/+0x498`。只分配基类加名称的 `0x400` 字节会越界；返回不同设备名称和中性活动值均不能阻止它。

### 9.3 反例二：暂停断连处理不要求选中 K

主审根据官方 ControllerDisconnected 三处 ASM 特征定位到 **`0x5346B0`**，在其开头设备检查循环中：`0x534768` 直接读 `device+0x48C`；值非零时 `0x534771` 继续读 `+0x498`。这些发生在 `0x534793` 的槽 32/action14 映射查询之前，不取决于 K 是否被用户高亮选中。仅从 ControllerSelect 列表隐藏 K 仍不够。

该 owner 还通过活动/确认、SetControllerId 进行断连重分配；官方 `ASMPlayerManager.cpp` 的 Unassign/Reassign/LoopEnd 已对这一流程有常驻补丁。K 输入中性会使它无法像物理设备一样用按键确认重分配，这是符合 C/S 自有绑定 owner 的行为；恢复时应由已有 lifecycle/唯一绑定逻辑处理，不能让暂停 UI 偷换 K 的 controller 关联。

### 9.4 后续实现要求与未决项

`DESIGN`：把 K 从 ControllerSelect 的**本地 UI 列表**排除；对 KeyConfig/断连处理等已定位消费者，在 **直接派生字段读取之前**识别 K 并选择自有中性处理，物理设备保持原样。不临时 erase 全局设备 vector，不伪造 Gamepad vtable，不因 hiding 了一个列表就宣称全部 UI 安全。还需在物理断连遍历比较 `+0x3F8` 前排除 K，和 ST2 的通知隔离一起纳入统一常驻 Hook owner。

另一种工程方案是扩张自有兼容区，至少覆盖至 `+0x49B` 并初始化已读字段；但这种方案仅解决已知读取的内存边界，不证明这些字段值的物理语义，也不阻止配置页面写 K 或误匹配平台 id。本轮**没有采纳任意 sentinel 或伪造尾部字段的方案**。具体 shim 位置/安装冲突和未定位消费者仍须在实现中明确，不把它们包装成“已安全”。

控制台/ImGui/暂停等既有 D7 阻断继续对 A/B 对称中性；在这些上下文不重放 Join 动作，退出 scope 后清除合成报告。官方菜单 action 短路可能让查询根本到不了 K（例如 Mods 搜索），不能把“无 K 查询记录”当成 UI 没有输入请求的证明。

## 10. 主审结论与交接

主审逐条比对子智能体的原始 Join/调用点/Spawn/Remove 导出，独立读取并复核 ST2 通知、ST8 写入入口及退出保存、ST9 两个直接读取反例。采纳经更正的 ST3–ST7 合同；对 CALL shim 栈解释、UI 全局绝对写入、直接调用图虚边缺口、特殊分支副作用与正常 K 路径的区分保留更正记录。没有把反编译自动类型或变量名当作 ABI。

本轮静态资料审阅只确认地址/控制流/源码引用和方案修订范围；未执行游戏或程序测试。运行时并发、OOM/EH、官方 Hook 实际顺序、特殊角色附属对象、完整持久副作用窗口以及新增 K 过滤 shim 的实现效果均未验证。下一步是依本轮事实修订并实现 tx-04-06，保持一次整体 E2E；不是立即执行额外 probe 或恢复旧 gate/哈希流程。
