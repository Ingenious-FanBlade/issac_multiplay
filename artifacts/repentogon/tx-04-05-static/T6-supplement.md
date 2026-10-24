# T6 补充：普通切房的 Player 定位时序

本补充只讨论普通门和测试 mod 发起的 `Game():StartRoomTransition(...)` 所在切房链。二进制为 v1.9.7.12 脱壳副本，IDA VA 基址 `0x400000`；函数、分支和指令证据见同目录的 [t6-transition-position-evidence.txt](t6-transition-position-evidence.txt)。

主审已复核并修订本补充：两处批量写入的对象归属、Lua API 参数链以本文件下述更正及 [primary-ida-evidence.txt](primary-ida-evidence.txt) 为准；子智能体原始推导保留在原证据文件，更正历史见 [candidacy-corrections.md](candidacy-corrections.md)。

普通 `Level::ChangeRoom` 路径已能确认一个**回调前**定位阶段：当 Game 的未命名字段 `Game+0x25B8C < 2` 时，Room loader 遍历 PlayerManager，调用定位 helper；该 helper 为 Player 写入 `+0x33C/+0x340`，并调整速度；随后 callback id 19（`MC_POST_NEW_ROOM`）才在 RVA `0x339719` 分派。`Game+0x25B8C` 不等同于 `_frameCount`，本结论只保留精确偏移与比较条件，不赋予语义名。若该条件不成立，已见的定位 helper 会跳过，但 callback 仍继续。

DoorNestedOwner 在 Level loader 与 callback 返回后，于 RVA `0x2FA17C` 调 Room update `sub_7F7F60`。该 Room update 自身存在 `+0x33C/+0x340` 写入（RVA `0x3FB454/0x3FB45C`、`0x3FBCB3/0x3FBCC2`），但目标寄存器无法由静态证据归为 Player。因而普通门路径里确有 callback 后位置字段写入点，**但尚不能证明它改的是 Player**；也不能把 callback 视为整条门切房链的最终位置边界。

`RoomTransition::ChangeRoom` 对 transition 对象 `+0x1C` 的分支提供了明确的路径差异；该字段不是官方 `.zhl` 中对象 `+0x0` 的 `_mode`：

| `this+0x1C` 分支 | 回调后的可确认行为 |
| --- | --- |
| `0` | DoorNestedOwner/Level loader 路径之后，ChangeRoom 本体没有对 Player `+0x33C/+0x340` 的直接写入；随后调用 `sub_803E20(Room)` 与 `sub_804500` 表现/渲染路径。前者函数范围没有这些坐标字段操作数，后者只读坐标。DoorNestedOwner 的 post-callback Room update 仍可能产生间接影响，目标归属 UNKNOWN。 |
| `11` | callback/owner 返回后，ChangeRoom 有直接坐标写入，位置来自当前分支计算；见 RVA `0x425B75/0x425B7F`。 |
| `20` | callback/owner 返回后存在按条件执行的 Player 坐标写入；维度 2 分支见 RVA `0x425E8A/0x425E92`。另一批 RVA `0x425F26/0x425F2E` 来自 type=3、variant=238 的实体查询，不能算 Player 列表定位。具体运行时分支取值仍需样本确认。 |
| `3` | 条件路径对选定 Player 在 RVA `0x42601A` 调坐标 helper `sub_7C3C40`（RVA `0x3C3C40`），该 helper 在 RVA `0x3C3E84/0x3C3E8D` 写坐标。随后 RVA `0x426144/0x42614C` 的批量写入来自 type=3、variant=238 查询，原称 PlayerManager 列表有误。 |

`RoomTransition::Update` 在 RVA `0x426AA6`、`0x426AC6`、`0x426BE9` 调用 ChangeRoom。该 Update 函数的完整范围没有 `+0x33C/+0x340` 直接字段访问；所见 `+0x360/+0x364` 写入是运动状态字段。因此 ChangeRoom 普通 `+0x1C==0` 分支本体及 Update 本体均未显示 callback 后 Player 坐标写入，但这不排除 DoorNestedOwner 内 Room update 或其他间接调用改变位置。

主审补齐了 `StartRoomTransition` 参数链：Lua 注册 RVA `0x47AD08/0x47AD0E` 将 native RVA `0x2F9710` 放入 bridge RVA `0x49EEB0` 的 upvalue；bridge 读取 Lua 第 2–6 参数为 room、direction、animation、player、dimension。native 正常分支在 RVA `0x2F9AD4` 以 ECX=`Game+0x1B83C` 调 setter RVA `0x423990`，将 animation 作为第三栈实参传入。setter 的 RVA `0x4239D5` 读取第三实参、RVA `0x423B05` 写 `this+0x1C`。该字段因而可描述为 animation/type，不是 `_mode`。setter 的 IDA std stacktrace 名称为误认，须看原始指令。

S9 使用全参数 `Game():StartRoomTransition(targetRoom, direction, 0, nil, -1)`：正常发起分支选择 `+0x1C==0`，负 dimension 在 setter 中取当前维度。原生存在拒绝发起、改写目标 room 等分支，实际切房与实际房间身份仍需在唯一 E2E 中记录；静态参数对齐不代表 API 已执行成功。

实现边界据此收窄为：callback 前的条件定位时序已证；`+0x1C` 为 11、20、3 的特定 callback 后定位路径已证；普通 `+0x1C==0` 分支没有直接坐标写入的事实已证；API 的 animation 参数到该字段的写入链已证。callback 后所有 Player 最终位置稳定点仍未知，因为门路径还调用 Room update，间接调用的对象副作用没有被全图穷举。运行时样本可以确认具体场景的实际值；静态证据不足处不能通过 Hook 到达/不到达来推断最终 Player 位置。
