# I21—I25 必要原生逆向：held、身体关系与 Lazarus

日期：2026-11-04。范围仅为 I21—I25 的必要静态证据；目标为 J273。未运行测试、构建、游戏或 probe，未校验哈希，未修改 IDB。完整 Player 绘制与资源消费者仍按 I29 收束；清理与逐前缀退休由 I33 收束。本文件不扩展到 I26 或 I27 的其他角色私有状态。

## 研究顺序与来源

按项目规范先读 `docs/archive/refactor/project-conventions.md`，再读 `docs/archive/design/tx-04-20/player-native-static.md` 的 R09—R11、`tx-04-20-implementation-plan.md` 的 I21—I25，以及 `client_players_native.*`、`client_player_equipment_native.*` 和 `client_player_state_native.*`。原版语义先查 `F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md` 与 `docs/enums/PlayerType.md`；REPENTOGON 再查 `F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md`、`docs/enums/SuplexState.md`、`repentogon/LuaInterfaces/Entities/LuaEntityPlayer.cpp` 及 `libzhl/functions/{EntityPlayer,Entity}.zhl`。上述资料不能回答实际 leaf／未命名字段时，才只读检查 J273 IDA。

原版 Wiki 说明：Jacob/Esau 与 Forgotten/Soul 是各自的多身体角色关系；普通 Forgotten 与 Soul 互换形态。REP Wiki 将 `GetFlippedForm` 描述为 Lazarus 翻转形态、`GetHeldEntity` 描述为真实 Entity，并说明 `IsHologram` 用于带 Birthright 的非活动 Tainted Lazarus。用途描述不替代底层 storage 事实；以下以实际字段和 getter 为准。

## 字段、getter 与可写性

REP ZHL `libzhl/functions/EntityPlayer.zhl:747—778,830—845` 给出 Player 字段；`Entity.zhl:197—200,245—249` 给出 Entity 公共字段。`sub player` 没有出现在 ZHL 命名布局中，+0x1D84 是由 J273 getter 直接读取得出的 raw `Entity_Player*` 字段。

| 字段 | J273 偏移 | 实际用途／写入边界 |
| --- | --- | --- |
| PlayerType / ControllerIndex / PlayerIndex | Player `+0x13BC` / `+0x160C` / `+0x1610` | 原生角色、控制与本地 roster 索引。PlayerIndex 是关系判定输入，不是网络身份。ControllerIndex 作为 Source 事实保留；Client 维持 `-1` 中性值，设备控制索引的应用归 I30。 |
| held Entity | Player `+0x140C`，`Entity*` | REP inline `GetHeldEntity()` 直接读此指针。没有与之等价的纯 setter；`TryHoldEntity`/`ThrowHeldEntity` 是玩法操作，不用于复制最终量。 |
| ItemState / cooldown | Player `+0x15F4` / `+0x15F8` | `GetItemState` J273 `0x431860 / RVA 0x31860` 直接返回 +0x15F4。ItemState 写 helper `0x758830 / RVA 0x358830` 会先经过状态处理再写字段，不作为纯复制 writer。 |
| twin / backup / sub | Player `+0x1E4C` `EntityPtr` / `+0x1E50` raw `Entity_Player*` / `+0x1D84` raw 指针（由 getter 证实） | 三个独立 storage。不可从一个 getter 的结果推导另两个边，也不可因角色名补边。 |
| Suplex | Player `+0x1F08` state、`+0x1F0C` aim countdown、`+0x1F10` target `Vector`、`+0x1F18` land `Vector` | REP Lua getter/setter 对公开量直接读写；IDA 在 `0x78CE75` 看到原生写入 state=5。公开 enum 只列 0—4，因此已观察范围为 0—5；不重放 Suplex。 |
| Entity 展示／关系 | `Visible +0x171`、`Exists +0x172`、position `+0x33C`、PositionOffset `+0x34C`、Parent `EntityPtr +0x3BC`、minecart `EntityPtr +0x3D4` | Minecart 是 Entity 自身关系，不是 Player held；复用 I19 的 minecart／偏移 owner 合同。Player held Render 不要求写 `Parent=holder`。 |

### 实际 getter leaf

下列地址是 J273 VA；RVA 按本映像基址 `0x400000` 计算。均为 x86 `__thiscall`，ECX 为 `Entity_Player* this`，无显式参数；bool 结果在 EAX/AL，指针结果在 EAX。它们是读取／计算结果的 leaf，不是安装关系的 writer。

| API | VA / RVA | leaf 实际行为 |
| --- | --- | --- |
| `IsSubPlayer` | `0x74D9B0 / 0x34D9B0` | `mov eax,[ecx+0x1610]; shr eax,31; ret`。只检查 PlayerIndex 符号位：负索引为 true。它不读取 +0x1D84，也不证明某个 sub 边存在。 |
| `GetSubPlayer` | `0x6D86D0 / 0x2D86D0` | 直接返回 `[this+0x1D84]`。无转换、无按类型查找。 |
| `GetOtherTwin` | `0x413160 / 0x13160` | 直接返回 `EntityPtr +0x1E4C` 的解析引用。 |
| `GetMainTwin` | `0x413140 / 0x13140` | 无 twin 时返回 self；twin 索引为 `-1` 或不低于自身索引时返回 self；仅在 twin 有有效索引且低于自身时返回 twin。它按索引选 main，不按 Jacob 名称或 participant ordinal。 |
| `IsHologram` | `0x7D1C90 / 0x3D1C90` | 先要求 Entity `Exists(+0x172)`；PlayerType(+0x13BC) 必须为 29 或 38；`twin(+0x1E4C)` 必须非空且非 self；该 twin 的 PlayerIndex(+0x1610) 必须不等于 -1 且小于 this 的索引。它不读 Birthright、backup(+0x1E50)、sub(+0x1D84)、Visible 或独立 hologram flag。 |

`GetFlippedForm` 的 REP 注册项在 `LuaEntityPlayer.cpp:3688` 指向 `Lua_PlayerGetBackupPlayer`；该 wrapper 在 `:1603—1605` 直接读 `plr->_backupPlayer` 并返回 userdata/nil，即 raw `+0x1E50`。这是公开 Lua wrapper 的实际读取合同；不把 backup getter误认为 native twin/sub getter，也不从 wrapper 名称推断 Flip 动作如何生成对象。

## 持物父代画：已有路径与边界

tx-04-19 已确认 J273 `Player::Render` VA `0x784A60 / RVA 0x384A60`，ABI 为 ECX=`Player*`、一个 `Vector*` 参数（`ret 4`）；held 调用使用目标对象 vtable `+0x14` 的 Render 槽。路径先从 Player `+0x140C` 取 held Entity，并受 ItemState、Body null-frame、目标 Visible 等门控；`0x78558D` 一段要求 held `Visible(+0x171)==false`，随后在 `0x78565E` 临时置 true、`0x7856D1` 虚调其 Render，之后在 `0x7856E2—0x785733` 还原。

临时修改／恢复的是 held 的 Sprite scale `+0xE4/+0xE8`、rotation `+0xEC` 和 alpha `+0xFC`，同时恢复 Visible；**不是** `PositionOffset +0x34C..+0x354`。Render 位置参数由 Player 与 held 的 position `+0x33C/+0x340` 计算。所有 held 动态类型共享的是 Player 父代画的条件与虚调用点，不是同一个具体 Render 实现；子类 Render、ANM、资源依赖与额外调用者仍需 I29 对实际支持类型闭合。此段没有写 Parent。

因此已知的可用量是同 T held Key/shape、ItemState、Body 完整 null-frame、Suplex 最终值及必要的短时 Render borrow/restore。`TryHoldEntity` VA `0x7C84B0 / RVA 0x3C84B0` 会按目标类别执行 Entity flag/关系/位置速度/动画/SFX/state/cache 等玩法变更；`ThrowHeldEntity` VA `0x7C87B0 / RVA 0x3C87B0` 会拆关系、清 held 并推进投掷结果。它们都不是复制 writer。当前采用直接安装最终指针／值并由 owner relation 服务维护必要 EntityPtr 边；held 是否可安全画仍取决于该目标动态类型已具名支持且资源／CPU队列借用完整。Effect136 只是 19 的已证窄类型，不代表所有 held Entity 都是 136。

## 角色关系的实际合同

### Jacob/Esau：19/20 twin

原版文档的角色关系与 J273 twin storage 相符。`InitTwin` VA `0x7C8370 / RVA 0x3C8370` 通过 `PlayerManager::SpawnCoPlayer` VA `0x990250 / RVA 0x590250` 创建新 roster Player，维护两端 `+0x1E4C` twin、传播 ControllerIndex，调整双方位置并安排 HUD。它会分配/初始化、推进 Manager RNG、登记 roster/EntityList，不能用作 Client 的关系 setter。应用最终图时直接装两端 twin 边、各自 PlayerIndex 与各自身体状态；Client ControllerIndex 保持 `-1` 中性值。先装完整集合，再统一调用公开 getter 核对。main 以 `GetMainTwin` 的实际索引规则计算，不能固定由“Jacob”或 participant ordinal推断。

### Forgotten/Soul：16/17 单向 sub

J273 `0x7C1F40 / RVA 0x3C1F40` 的 Update/创建分支在 roster Player 的类型为16或17且缺少 sub 时，分配另一 `Entity_Player` 并以相反类型初始化，然后只在当前父体 `+0x1D84` 存储 sub；该创建分支没有将补体加入 PlayerManager roster，也没有写反向 sub 边。`SwapForgottenForm` VA `0x7C2470 / RVA 0x3C2470` 的成功交换段 `0x7C26DB—0x7C26ED` 交换双方 `+0x13BC` 类型。初读时曾把创建类型误看为同型；复读三元分支后纠正为16→17、17→16，且成功 Swap 后仍是一16和一17，只是类型随实例交换。

实现采用 Source 明示的父→sub 单向边、roster 主身体及非roster 补体；不强造 sub→parent、不按 Soul 名称猜 parent，也不调用 `SwapForgottenForm`（它还会搬移其他状态／武器 owner并触发玩法效果）。非roster 不自动意味着 IsSubPlayer=true：该公开结果完全由每身体实际安装的 PlayerIndex 符号位决定；要与来源一致就保留来源索引。

### Tainted Forgotten/Soul：35/40 twin，held 独立

J273 `ChangePlayerType` VA `0x7C4620 / RVA 0x3C4620` 的 type35 分支调用 `InitTwin(40)`；因此 35/40 走真实 roster twin，而不是普通16/17的单向 sub。`heldEntity +0x140C` 是另一个独立关系，持有时 Source 可以指向另一 Player；不因 held 关系写 `Parent=holder`，也不把 held 与 twin 合并。每身体保留独立 PlayerIndex、库存和最终状态。

### Tainted Lazarus：29/38 的 backup 与 twin 分开

J273 `0x7B2C90 / RVA 0x3B2C90` 的 type29 分支在 backup `+0x1E50` 为空、当前 Player 有 roster index 且条件允许时，单独分配并初始化 type38，写 29→38 与 38→29 的 raw backup 指针，随后把新38的 Entity `Exists(+0x172)` 置 false。该分支没有显示把新38登记进 roster，也没有写 `twin +0x1E4C` 或 `sub +0x1D84`。所以**无 Birthright 的 inactive form 属于 reciprocal backup storage**：它不是从该分支得出的 sub 或 twin。其 getter 结果按各自 leaf 导出：GetFlippedForm 读取 backup；GetOtherTwin 在没有 twin 时为 nil；GetMainTwin 为 self；IsHologram 因 Exists=false且无 twin 条件为 false。`IsSubPlayer`仍只由该对象来源的 PlayerIndex 决定。

Birthright 的公开用途是 inactive Lazarus hologram，但 `IsHologram` leaf 的必要关系是存在、type29/38、有效较低索引 twin；backup 指针不参与判定。实际 application 必须分别保存／安装 flipped-form(backup) 与 twin 边，并以真实 getter 回读。**UNKNOWN：带 Birthright 的 twin 是否与 backup 指向同一对象**；不作相等假设，也不要求二者 type 必须相反，除非 Source 的具体最终值／原生 getter能够证明。无 Birthright inactive 配对的 backup 互指已由上述分支证明。

## 直接安装与逐前缀拆边

采用最终纯值，避免原生玩法变换：先创建并注册本地对象 owner，再安装每身体 PlayerType、PlayerIndex、Exists、held 与角色关系存储；Client ControllerIndex 保持 `-1` 中性值。使用现有 EntityPtr link 服务维护 twin 边，sub/backup 按其各自真实 raw storage 安装。所有身体和所有关系边安装完毕后，再对每身体调用/核对 `GetSubPlayer`、`IsSubPlayer`、`GetOtherTwin`、`GetMainTwin`、`GetFlippedForm`、`IsHologram` 等公开结果。不要逐身体边写边查，否则另一端尚未连接时会误判；也不从字段组合合成额外关系。

对确认已完成构造/Init且归当前 owner 的 Player，正常退休时由该 owner 幂等清空其 sub、backup、held 等 storage，并拆除该 owner 的 EntityPtr twin 边；对有效 EntityPtr 调用 `SetReference(nullptr)` 即使当前为空也安全。backup、sub、twin 仍是不同 storage，不能把它们合并为同一关系或重复释放对端对象。若构造/Init 前缀或 owner 尚不确定，则不清 storage、不退休对象，保持隔离。此合同不要求新增逐边 artifact 或单独恢复流程；不调用 `RemoveCoPlayer`、`SwapForgottenForm`、Flip/Use 道具或递归玩法清理作为复制／回滚路径。具体对象构造、ANM/Costume/Weapon 清理前缀和绘制在途隔离继续由 I33 处理。

## 采用结论与剩余 UNKNOWN

- **采用：** `PlayerIndex<0` 派生 IsSubPlayer；main/other/sub/flipped 结果保留为独立查询值；关系使用 Source 指明的 storage 和方向；持物、ItemState、Suplex 只安装最终值；先全体 Link、再全体 Check。上述 leaf 只用于读取／核对，不写入状态。
- **不采用：** `InitTwin`、`SwapForgottenForm`、`TryHoldEntity`、`ThrowHeldEntity`、Flip/Use 道具等玩法路径作为复制 setter。裸指针、EntityPtr和实体 owner不可互相替代。
- **UNKNOWN / 归属：** Birthright 情况下 backup 与 twin 是否为同一对象；当前允许的每个 held 子类的虚 Render 覆写及附加字段／ANM／外层调用闭包（I29）；资源、Weapon、完整角色绘制依赖（I27/I29）；任意异常/SEH前缀的完整回滚与CPU队列最终消费闭包（I33）。静态事实不证明最终像素或所有玩法异常都已覆盖。
