# tx-04-06 实现方案：网络、虚拟设备 B 与 Join

**tx-04-06 完成（用户接受，D28，2026-10-25）**：用户决定不再执行第三次 E2E，将第二次 E2E 算作验收通过（V1–V11 PASS；V12 只因每日备份的基线匹配问题失败，原报告保持 FAIL）。见[结果](tx-04-06-results.md)。

第二次 E2E（2026-10-25）：**FAIL**，S0–S11 全部执行，V1–V11 PASS；V12 只因第一、三档当日备份的 D20 基线按带日期的文件名匹配不到而失败，变化字节限于 Counter/Checksum。没有重跑。用户随后决定 D27：存档判定不看每日备份，harness 已修改并纯构建；tx-04-06 的结论仍待用户决定，见[结果](tx-04-06-results.md)。

第一次 E2E（2026-10-25）：**FAIL**，见[结果](tx-04-06-results.md)。S3 Join 已产生 B 并进入 Active；第一个 tick 后游戏在 `0x52F880` 的原生 `dynamic_cast` 处崩溃，原因是 K 的 vtable 缺少 RTTI COL（原型 `BuildTables` 有，本轮移植遗漏）。没有重跑。用户决定 D26：按原型自建 COL、修正 harness 崩溃记录、保留编号执行第二次 E2E；修复已纯构建（[build-01](../../artifacts/repentogon/tx-04-06-fix/build-01.log)），第二次 E2E 尚未执行。

3.4/3.5 进展（2026-10-25）：常驻补丁诊断、真实 Host/stop、配置/状态/trace、主机 UI、Rust 独立 ICSN codec/Endpoint/脚本远端与单启动 S0–S11 / V1–V12 harness 已编写，tx-04-05 V2 方向缺陷已修正。ST8 只读观测纳入同一 Hook 清单，当前为 33 个入口、8 个设备槽与 12 处字节补丁。Win32 DLL 与 i686 Rust harness 纯编译链接成功，见[编写记录](tx-04-06-ops-harness.md)、[构建日志](../../artifacts/repentogon/tx-04-06-ops/README.md)。没有部署、启动游戏或测试；唯一整体 E2E 尚未执行。下方为各阶段原始记录。

game 进展（2026-10-25）：第 3.3 节 game/ 与必要的第 3.4 节 Join/K 补丁已编写并纯编译、链接成功，见[实现与接线记录](tx-04-06-game.md)、[构建记录](../../artifacts/repentogon/tx-04-06-game/README.md)。包含 K/Join/peer、双 ordinal 拓扑与 consumer/lifecycle、主线程 Host API、11 处新常驻补丁及原生后端锁顺序。已补充 Ack 输入代次在可用绑定后公布、Input 回显代次的本地记录与旧窗口丢弃规则。Host/stop 命令仍为 ops 占位，配置/UI/Rust/harness 仍待接入；未部署或执行测试，唯一整体 E2E 留待 tx-04-06 全目标完成。下方保留先前 core/net/静态阶段记录。

core 进展（2026-10-25）：第 3.2 节已编写并纯编译、链接成功，见[core 实现与接线约定](tx-04-06-core.md)及[构建记录](../../artifacts/repentogon/tx-04-06-core/README.md)。增加 Host 双参与者、首次双绑定激活、三个对端有序退休原因；A/B 复用同一 admission/queue/Resolve，并统一序号冲突的整会话中止。尚未接入 game/peer/K/Join、Host 命令、状态/trace/UI 或 Rust 远端，没有部署或运行测试；仍属于 tx-04-06 同一次推进，整体 E2E 待全目标完成。下方保留 net 与静态阶段的原始进度记录。

进展（2026-10-25）：按用户指示先完成第 3.1 节 `net/`，四个模块和独立静态库已编写并纯编译、链接成功，见[实现记录与线格式](tx-04-06-net.md)。尚未接入 Host/core/game/命令/状态/trace，没有部署或运行测试；tx-04-06 全目标及唯一一次整体 E2E 尚未完成。以下首段保留静态研究结束时的记录。

网络规则同步（2026-10-25）：已按第 5.1、5.2、5.4 节后补规则修改 Authority 代码：乱序旧包丢弃计 stale，重复/stale 不算错误，主动结束使用同一 Bye 字节连发 3 份并保留原始原因，首份 Bye 后只在当前接收预算内计数原字节副本、不回包。[同步记录](tx-04-06-net.md)与 [build-04 纯构建](../../artifacts/repentogon/tx-04-06-net/build-04-network-rules.log)已保存。第 13.2 节 Rust 远端的 Hello 定时重发、握手超时与 First Ack 专用副本缓存仍待后续实现；没有执行测试。

日期：2026-10-24；2026-10-25 完成 ST1–ST9 的指定静态研究与主审，功能尚未实现。D24 已由用户确认：拆分推进；B 的角色由房主指定，默认 Isaac；对端断开后整个会话结束；静态确认沿用 tx-04-05 的做法。第 15 节的 U1–U4 已由用户按建议确认（设计 D25）。2026-10-24 暂停静态工作的指示已由用户 2026-10-25 开始 ST1、继续剩余逆向的新指示取代；GPT-6 Luna / max 子智能体使用 IDA MCP 逆向，主智能体独立取证审阅，见 [ST1](../research/tx-04-06-static-confirmation.md) 与 [ST2–ST9](../research/tx-04-06-static-remaining.md)。设备通知隔离、UI 直接读取和持久副作用已找到具体风险，尚未实现解决；没有功能代码、构建或测试。依据：

- [REPENTOGON 适配重构设计](repentogon-refactor.md)（下称“设计”），尤其是第 7.1、7.4、8、9 节；
- [v1.9.7.12 点位核对](../hooks/repentogon-binding.md)，尤其是第 5、6、7 节；
- [tx-04-05 实现方案](tx-04-05-implementation-plan.md)与[结果](tx-04-05-results.md)：本轮直接复用的 tick、A 输入链与生命周期；
- [现行项目规范](../refactor/project-conventions.md)，尤其是第 4、6、7、9、14 节；
- 原型源码：`cs-t0/native/src/minimal_protocol.cpp`（ICT8 v2 编解码与 Endpoint 状态机）、`coop_active_runtime.cpp`（Join pass、mode predicate、创建点、B 退休）、`owned_virtual_device.cpp` 与 `virtual_device_lifecycle.cpp`（v1.9.7.17 设备 ABI，只作参考）、`cs-t0/src/t09/relay.rs`（relay，留给 tx-04-07）。

按规范第 2 节，本轮只在全部实现完成后执行一次 E2E（第 14 节）；分析、逆向和实现期间不运行任何测试。

## 1. 目标、范围与边界

**目标。** 远端玩家经网络连接到房主游戏内的 DLL。DLL 创建自有虚拟设备 K，通过原生 Join 让 B 成为完整 Player。B 的输入来自网络，与 A 走同一条 canonical / Submit / queue / Resolve 链路，在同一个 Authority tick 中生效。stop、对端断开或 run teardown 时，B 输出中性后退休，整个会话回到 Idle。

**预期结果。** 一次 E2E 通过第 14 节的 V1–V12。

**本轮包含：**
1. `net/`：新协议的编解码与 Endpoint 状态机（从 ICT8 v2 移植并换 magic）、Winsock UDP 工作线程、与主线程之间的有界队列；
2. `core/`：Session 增加 Host 模式（两个参与者）与对端相关的原因码；
3. `game/`：设备 K（按 v1.9.7.12 ABI 重做）、Join（Join pass 语义、三处 mode predicate 补丁、创建点补丁）、B 的拓扑与绑定、B 的 consumer 覆写、B 与 K 的退休；
4. 命令：`cs host <port> [character]`、`cs stop` 换成真实语义；状态字段、trace；界面主机页去掉占位说明；
5. Rust：harness 增加 Rust 版协议编解码与按 tick 脚本发送输入的远端客户端，以及新的 E2E 场景；顺带修正 tx-04-05 已知的 V2 判定方向缺陷（D23）。

**留给 tx-04-07（D24）：** relay 与故障注入（丢包、乱序、延迟、重复）；B 死亡与复活；异常 Join（实际走到原生 Baby 判断为真的路径）；B 的跨房射击；loading 期间的查询；补齐设计第 13 节的完整场景集。

**不包含：** world_record（顺延到 tx-04-08）；Replica 与状态同步；DLL 内的 Client 角色和界面“加入”页；断线重连与 late join；NAT 穿透、Steam 网络、大厅；认证。

## 2. 依据与已知事实

### 2.1 沿用的已采纳语义

| 来源 | 语义 | 本轮落点 |
| --- | --- | --- |
| 规范第 6 节 | A 固定 ordinal 0、B 固定 ordinal 1，`ClientId → ordinal` 在会话内冻结；A/B 共用 canonical、sequence、target tick、queue、dedupe、Resolve；房主没有即时旁路，B 不直接写 Player/device/world；缺输入中性；room、death、设备重分配使绑定失效；teardown 退休 session 与 generation | 第 4、9 节 |
| 规范第 9 节 | 自有设备采用自有 vtable，路线 A（虚拟设备触发原生 Join）为主；回收策略 NEVER；每个会话至多创建一次，失败不重试；退休输出中性、从 InputManager 移除、B 退出 | 第 7、8 节 |
| tx-01-51 | 重新唯一绑定后的启动例外 | 第 9.1 节，A/B 相同 |
| tx-01-54 | 无 tick 或有阻断时主动暂停射击 consumer | 第 9.2 节，A/B 相同 |
| tx-01-56 | 自有 B 为完整 Type=1/Variant=0 Player；三处原生 Baby 判断只对 K 选择普通角色路径；原 predicate 各调用一次；普通工厂仍是唯一创建 owner | 第 8 节 |
| D7 | 房主打开 REPENTOGON 控制台、菜单或 ImGui 时 A/B 同时屏蔽，按类似暂停处理 | 第 9.3 节 |
| D24 | 断开后整个会话结束；B 的角色由房主指定，默认 Isaac | 第 4、8.4 节 |

### 2.2 v1.9.7.12 上的点位（`STATIC`，[点位核对](../hooks/repentogon-binding.md)第 5、6 节）

| 项 | v1.9.7.12 | 说明 |
| --- | --- | --- |
| Join pass | `0x2F6D21 → 0x591360` | 在 `Game::ProcessInput` 中；`this` = PlayerManager（`Game+0x1BAA8`）；Netplay 非空时提前返回。tx-04-04 已注册为计数 Hook，owner 为 `Join` |
| mode predicate 1/2/3 | `0x5915AE`、`0x59174C`、`0x591E67 → 0x5910A0`（`PlayerManager::CoopBabiesOnly`） | 1、2：manager=EBX，key=ESI；3：manager=EBX，key=ECX（`[edi-8]`），行指针=EDI |
| 创建点 | `0x591FE6 → 0x590250`（`SpawnCoPlayer2`） | EDI 为行游标；工厂只有一个 role 参数。沿用 E8 CALL shim 时，入口 `[esp+4]` 为 role；拒绝 `add esp,8` 清 shim 返回地址与 role 后跳 `0x5925E4` |
| 设备基类 | vtable `0x732F04`（38 槽，纯虚槽为 3、35），构造函数 `0x5EC570`（thiscall，无参数），Initialize 第 1 槽 `0x5EC6A0`（`retn 4`），基类大小 `0x3F8` | RTTI 为 `DeviceKeyboard` / `DeviceGamepad` |
| 注册 / 移除 | InputManager vtable `0x732E44` 第 45 槽 `0x5EEB40(device*)`、第 46 槽 `0x5EEBB0` | 移除时先调设备第 2 槽，再调第 0 槽（参数 1）删除设备；Poll（第 35 槽）返回假也会被移除并删除 |
| 移除 co-player | `0x590560` | thiscall(manager)，`Player*` 与 unused bool 两个栈参数、`retn 8`；false 不抑制掉落；调用前撤销 B 绑定 |
| 设备 id | `+0x08`，来自计数器 `0x7E65EC` | Player `_controllerIndex`（`+0x160C`）与之比较；不是 vector 下标（tx-04-05 T2） |
| 查询槽 | 第 8/9/10 槽 | `thiscall(action)`、`retn 4`，第 10 槽为 x87 浮点返回 |
| Join 行 | `+0x0C` / `+0x14`，步长 `0x14` | 与原型相同 |

### 2.3 原型的 Join 流程（`SOURCE`，`coop_active_runtime.cpp`）

1. Join pass 入口：会话允许绑定、设备已注册、没有阻断、A 已唯一绑定、只有一个完整 Player 时，打开 Join scope。
2. pass 内，原生按设备查询加入与选人动作（原型记录的是动作 19、14、15、20、21）；K 在 scope 内按需报告这些动作。
3. 三处 mode predicate：原调用恰好一次；对 K 一律返回“非 Baby”。
4. 创建点：只放行 K 那一行；按配置改写栈上的角色类型，先确认该角色可选；其他行原样放行。
5. pass 返回后：确认 Player 数为 2、A 未受影响、B 是完整 Player 且角色与期望一致，然后绑定 B。
6. 退休 B：B 中性 → 移除设备 → 调用原生移除 co-player（原型 `0x5BA000`）→ 确认 Player 数回到 1。

本轮沿用流程结构；v1.9.7.12 的动作、参数与移除副作用已按第 12 节重新确认。原型的无条件设备通知与 UI 假设不能照搬。

### 2.4 原型协议（`SOURCE`，`minimal_protocol.hpp`）

ICT8 v2：64 字节包头、显式小端、包长上限 1200；kind 为 Hello、Input、Ack、StateHash；对整包计算 CRC-32/ISO-HDLC，计算时把 56–59 字节视为零。这只用于检查包是否损坏，不是认证，也不是状态哈希。Input 携带当前命令和最多 3 条冗余命令（`InputCmdDraftV2`，每条 64 字节）以及 last_ack。Endpoint 是单 owner、一次性的状态机：Dormant → Negotiating → Established → Retired / Failed。

原型的远端是第二个游戏进程，网络在 Rust 控制器里，native 经共享内存 IPC 收发。本轮网络移入 DLL，远端改为纯 Rust 客户端，两侧各有一份编解码实现，由唯一一次 E2E 共同验证。

## 3. 工程改动

### 3.1 `net/`（新增，不含游戏或 REPENTOGON 头文件）

2026-10-25：本节代码已编写并纯构建成功；准确 payload 布局、Worker 接线顺序及剩余范围见[net 实现记录](tx-04-06-net.md)。所有网络行为仍待全目标唯一 E2E 验证，不能据此声称 Host 会话已经可用。

| 文件 | 职责 |
| --- | --- |
| `net/protocol.*` | 包头、四种 kind 的编解码、CRC、严格校验；从原型 `minimal_protocol` 移植，换 magic 和版本，去掉 StateHash |
| `net/endpoint.*` | 单对端握手与序号状态机，从原型 Endpoint 移植；增加 Bye 与超时判定 |
| `net/worker.*` | Winsock UDP 工作线程：绑定、收发、解析、按来源地址锁定唯一对端、投递到有界队列；不碰游戏对象 |
| `net/queue.h` | 有界 SPSC 队列（收：网络→主线程；发：主线程→网络）；满时丢弃并计数，不阻塞 |

### 3.2 `core/`

2026-10-25：本节已编写并纯构建成功；Host 的 Start 时点、A/B 初始化与对端原因映射见[core 接线约定](tx-04-06-core.md)。没有执行测试，game 主线程接线留给第 3.3 节。

- `Session` 增加 `SessionMode::Host`，参与者数为 2；`Activate(host_bound, remote_bound)` 在 Host 模式下要求两者都已绑定。
- 原因码增加 `PeerBye`、`PeerTimeout`、`PeerProtocol`；`ControllerLost` 等历史值保留不用。
- Participant[1] 与 Participant[0] 使用同一份 admission/queue/Resolve 代码，不新增 B 专用路径。

### 3.3 `game/`

2026-10-25：本节及其必要 Hook/补丁已编写并纯构建成功，见[game 实现记录](tx-04-06-game.md)。代码尚未部署或运行；主线程 StartHost/StopSession 接口等待第 3.5/11 节命令接线。运行时并发、官方 Hook 冲突、特殊角色与持久副作用均未因纯构建而通过验收。

| 文件 | 职责 |
| --- | --- |
| `game/device_k.*`（新增） | 设备 K 的对象布局、自有 vtable、状态机、注册与移除；第 0 槽 NEVER |
| `game/join.*`（新增） | Join scope、mode predicate 判定、创建点判定、pass 结果确认、B 的首次绑定 |
| `game/peer.*`（新增） | 主线程侧的网络桥：Manager 入口取包并 Submit B 的输入，TopExit 生成 Ack 并投递；对端状态与超时 |
| `game/topology.*` | 扩展为两个 ordinal：B = `_playerList[1]` 且 `_playerIndex==1`，设备为 K（id 唯一匹配） |
| `game/query.*`、`game/consumers.*` | 覆写规则扩展到 ordinal 1：K 在 B 的 consumer 上下文返回 Resolve 结果 |
| `game/lifecycle.*`、`game/tick.*` | 失效与重绑、teardown、Retiring 序列扩展到 B 与 K |

### 3.4 Hook 与补丁

2026-10-25：本节编写已完成，补充按 site 的 Join 观测及 5 个 ST8 只读入口 Hook，均由统一清单注册，详见[3.4/3.5 记录](tx-04-06-ops-harness.md)。尚未运行安装或测试。

- Join pass 入口 Hook 已在清单中（tx-04-04），本轮加入 armed 时的语义。
- Join 有 4 处调用点补丁：三处 mode predicate、一处创建点，原指令均为完整 5 字节 E8。与 Odd shim 相同，经统一清单注册，在首次主线程 Manager 入口延后安装，安装前核对期望字节；不符时关闭 `join` 能力组，不恢复字节。汇编 thunk 按 v1.9.7.12 的寄存器分工重写，保存通用寄存器、EFLAGS 与 x87/MXCSR/XMM 状态。第三处 mode 原函数保留 ECX，shim 也必须保留，后续原生查询继续使用它作为 key。
- ST2/ST9 新增的 K 专用兼容处理也必须纳入统一常驻清单：通用输入注册/删除通知隔离，物理断连遍历排除 K，ControllerSelect 本地列表与 KeyConfig/暂停断连处理的 K 分支。当前已编写 7 处 K 通知/过滤 shim，加上 4 处 Join 共 11 处新增字节补丁，见[实现记录](tx-04-06-game.md)；尚未运行验证。不能对原版在线模块安装 K Hook 或临时改全局 vector/callback 列表。官方在断连处理中已有补丁，新增位置须遵守其调用链。
- 设备 K 是自有对象，不是补丁；它的 vtable 由我们构造，不修改游戏的 vtable。
- 原生移除 co-player 只被调用，不 Hook。

### 3.5 其他

2026-10-25：本节各项已编写，配置、状态、trace 与第 13/14 节的场景和归约一并接入，详见[3.4/3.5 记录](tx-04-06-ops-harness.md)。没有新增单元测试或执行实机验收。

- `ops/command.cpp`：去掉 `PlaceholderSession`；`cs host`、`cs stop` 接入真实会话；`cs solo` 保留，与 Host 互斥。
- `ui/isaac-cs-ui`：主机页去掉 “tx-04-04 placeholder” 说明，状态区增加对端与 B 的状态；角色不在界面中选择（第 15 节 U3）。
- `tools/harness`：新增 `remote/`（Rust 版协议与脚本客户端）和 tx-04-06 场景；修正 V2 判定方向缺陷。
- 注释：按 AGENTS.md，为每个功能模块、核心函数和数据结构写中文注释。

## 4. 会话

```text
Idle --cs host--> Hosting --握手完成--> Warmup --run 内、K 已注册、B 经 Join 加入并与 A 同时唯一绑定--> Active
Hosting/Warmup/Active --cs stop / 对端 Bye / 对端超时 / 协议错误 / run teardown / 失败--> Retiring --B 中性、K 移除、B 移除、网络线程停止--> Idle
```

- **Hosting。** `cs host <port> [character]` 在主菜单或 run 内都可发起（Q14 已观察到主菜单时 `g_Game` 非空）。主线程启动网络线程并绑定端口；此时会话未 armed，A 保持原生输入，Hook 透传。
- **握手。** 第一个合法 Hello 的来源地址成为唯一对端，其他地址的包丢弃并计数。握手完成后进入 Warmup。
- **Warmup。** 不在 run 内时只等待。进入 run 后，在稳定的 Authority 入口创建并注册 K，下一次 Join pass 接纳 K（第 8 节）。A 与 B 都唯一绑定后，`Activate` 转 Active。Warmup 期间不分配 sequence、不 Resolve、不推进 scheduler；与 tx-04-05 相同，受控的 A 在这段时间输出中性。
- **Active。** A 与 B 进入同一个 scheduler，Authority 不等待 B；B 缺输入时按 `max_input_age_ticks=0` 立即中性。
- **结束（D24）。** stop、对端 Bye、对端超时、协议错误、run teardown 或内部失败都结束整个会话：不再开始新 tick；已进入的 tick 走完正常返回与发布；B 输出中性；移除 K；原生移除 B；向对端发送 Bye（尽力而为）；停止网络线程；退休 session 与 generation；回到 Idle。没有断线重连，也不回到 Hosting。
- **与 Solo 的关系。** Host 与 Solo 互斥，同时请求时返回 `error=busy`。

## 5. 协议

### 5.1 包格式（`DESIGN`）

- 沿用 ICT8 v2 的 64 字节包头布局、显式小端、包长上限 1200、CRC 规则；magic 改为 `ICSN`，协议版本为 1，不承担 ICT8 或 ISCS 兼容（D4）。
- kind：Hello=1、Input=2、Ack=3、Bye=5；4 保留（原 StateHash），收到即判协议错误。
- 严格检查：包长、magic、版本、头长、kind、role、flags、reserved、CRC、session id、发送方 id；任何错误都不改变输出，按错误类型计数。握手前的错误包只丢弃；握手后对端的协议错误结束会话（原因 `PeerProtocol`）。
- 与已接受报文字节完全相同的副本不算错误：只计入重复包并刷新存活，不再次投递内容。第 5.2、5.4 节的 Hello、First Ack、Bye 重发都依赖这一条。
- **乱序旧包（2026-10-25 补充，用户确认）。** 只有 packet sequence 高于已接受高水位的包才交付。握手完成后，通过上述检查、序号不高于高水位、又不是字节相同副本的包，视为乱序或迟到：丢弃并计入 `stale`；不交付内容，不刷新存活，不改变高水位，也不结束会话。
  - **Input。** 乱序距离不超过 3 包时，较新的包已经用冗余带上了旧包的命令；更远的旧命令按缺输入中性处理，与丢包相同。
  - **Ack。** 旧的进度快照本来就会被新快照取代。
  - **仍是协议错误。** 序号更高但内容自相矛盾的包，例如 Ack 进度回退，或 Input 的 last_ack 超出对端实际发出的 Ack。
  - **Hello 与 First Ack** 仍按第 5.2 节处理。

### 5.2 握手与身份

- 远端发 Hello：build 文本、协议版本、input schema 2、role=Remote、远端 nonce。
- 主机回 Ack（purpose=First），携带完整协商信息：session id、主机 nonce、双方 client id（A=1，B=2）。远端收到后才开始发 Input。
- session id 与 nonce 由主机在 `cs host` 时用系统随机数生成，只作会话识别，不作认证。
- 没有认证：只适用于可信网络，界面和支持声明中写明（设计第 8 节）。

**丢包时的握手规则（2026-10-25 补充）。** 传输层不另加可靠机制，只有握手和 Bye 靠原字节重发补偿丢包：
- **远端重发 Hello。** 远端发出 Hello 后，每 250 ms 重发同一份字节（packet sequence 不变，不重新编码），收到合法 First Ack 后立即停止。从首次发送起 `peer_timeout_ms` 内仍未收到 First Ack，远端判握手失败并停止；这时尚未建立会话，不发 Bye。
- **主机处理重复 Hello。** 锁定对端后，与已接受 Hello 字节完全相同的包视为重传：First Ack 已发出时原样重发缓存的 First Ack 字节，尚未成功发出时补发；只计入重复包，不重新锁定，不重新生成身份，不算协议错误。锁定对端的其他 Hello（nonce、build 或序号不同）在握手完成前只丢弃，完成后按协议错误以 `PeerProtocol` 结束。
- **远端处理重复 First Ack。** Hello 重传可能换来多份字节相同的 First Ack。第一份使远端进入 Established，其余只计数、刷新存活。副本可能晚于 Progress Ack 到达，因此远端要按字节与已接受的 First Ack 比较；相同即为重复，不按序号回退判错。
- **主机侧超时起点。** 主机的对端超时从锁定对端（第一个合法 Hello）开始计时；还没有对端的纯 Hosting 无限等待，直到 `cs stop` 或 run teardown。

### 5.3 Input 与 Ack

- **Input。** 每包携带 1 条当前命令和最多 3 条冗余命令（`InputCmdDraftV2`），外加 last_ack。主线程只把 client id 为 B 的命令送入 Participant[1] 的 admission；dedupe、过期、冲突按现有规则处理并计入 Ack。
- **Ack。** 主机在每个 TopExit 发布 completed 后生成一份：completed tick、B 的 accepted / processed / processed_tick / 拒绝计数与掩码、会话状态。Ack 也是主机侧的心跳。generation 仅在 Active、未阻断且 B 已绑定时公布新代次；此前为 0，暂停/未绑定期间保留上次公布值。Input 回显的 last_ack 须对应当前可用代次的 Active Ack，本地发送记录缺失时保守丢弃，不改变包格式。详见[接收代次合同](tx-04-06-game.md)。
- **目标 tick。** 远端用最近一份 Ack 的 completed tick 加上自己的 lead 作为 target。主机按现有 future window 与 lead 规则 admission，不替远端改写 target。

### 5.4 心跳、超时与 Bye

- 双方都把对端发来的任何合法包当作存活证据。远端在 Warmup 期间也定期发送 Input（中性或冗余），使主机能判断存活。
- 主机在 `peer_timeout_ms`（默认 3000，配置项）内没有收到合法包，即以 `PeerTimeout` 结束会话。超时只用于判断断线，不用于推进 tick（规范第 6 节）。
- 主动结束时发送 Bye；收到 Bye 立即结束。
- **Bye 连发三份（2026-10-25 补充）。** 主动结束的一方先记录真实结束原因，再把同一份 Bye 字节（packet sequence 不变）连发 3 次；任何一次 sendto 失败都不覆盖原因。接收方第一份合法 Bye 即结束会话，其余副本只计数，不回 Bye。只在握手完成后发送 Bye。3 份全部丢失时，由对端超时结束会话。

## 6. 网络线程

- **创建与停止。** `cs host` 在主线程执行时创建网络线程；会话结束时主线程发出停止信号，并以有界时间等待它退出（不在 `DllMain` 中等待）。等待超时只记录错误，线程不访问游戏对象，退出后关闭 socket。
- **职责。** 只做 `recvfrom/sendto`、解码、校验、地址锁定和入队；不调用游戏函数，不读写游戏对象，不触碰 Lua（设计第 3.3 节）。
- **队列。** 收、发各一个固定容量的 SPSC 队列；满时丢弃并计数，输入靠冗余弥补；主线程不阻塞。
- **绑定地址。** 默认 `0.0.0.0:<port>`，配置项 `host.bind_address` 可改为 `127.0.0.1`（第 15 节 U1）。
- **计数。** 收发包数、按错误类型的丢弃数、非对端地址丢弃数、队列丢弃数、往返时延（由 Ack 回显推算），写入状态与 trace。

## 7. 设备 K

### 7.1 对象与 vtable

- 对象 = 基类区 `0x3F8` 字节 + 自有扩展。官方 Initialize Hook 会读取 `+0x3FC` 名称指针，Initialize 前须预留并初始化；槽 3 始终返回有效非空常驻名称。ST9 另确认 UI 直接读 `+0x48C/+0x498`，物理断连读 `+0x3F8`，因此 ST1 的 `0x400` 仅是名称入口的下限，不能当完整安全布局。当前方案在消费者读派生字段之前排除/处理 K；私有状态不得未经分析占用会被原生读取的尾部偏移。不凭空填 Gamepad 字段或选择未经证明安全的 sentinel。用游戏或自有分配均可，按下述槽 0/2 保留有效资源。
- 构造：主线程调用基类构造函数 `0x5EC570`，再调用 Initialize（第 1 槽，含一个栈参数，检查 bool 返回），然后把对象的 vtable 指针换成自有的 38 槽表。按 [ST1](../research/tx-04-06-static-confirmation.md)，自有表从**基类表**复制，覆写第 **0/2/3/5/6/7/8/9/10/11/26/27/35** 槽，其余 25 槽保留基类实现。这里增加第 2 槽是为了 NEVER；raw/原始触发/活动查询另行中性化，避免 UI 旁路。不复制 Keyboard/Gamepad 的物理实现。
- 回收策略 NEVER：进入“基类已构造”之后，对象在进程生命周期内不释放。只有尚未调用原生构造的原始分配可以回滚释放。每个会话至多创建一次；跨会话不复用旧对象（被 InputManager 删除过的对象不再注册）。

### 7.2 覆写的槽

| 槽 | 行为 |
| --- | --- |
| 0（删除） | ECX=this，栈上一项 flags、`retn 4`、EAX 返回 this；不调用原析构，不释放内存，只标记“已被 InputManager 删除”，记录 trace |
| 2（资源清理） | 无栈参数，无有意义的返回值；标记退休但保留映射、current/previous 和有效基类状态，不调用原清理 |
| 3（名称，纯虚） | 无栈参数，EAX 返回有效的常驻 `const char*` 自有名称；不是整数设备类型码，名称本身不证明所有消费者的分类行为 |
| 5/6/7（raw 按钮） | 原始 Pressed/Triggered 返回 false；Scalar 以 x87 返回 `0.0f`，均保留一个栈参数的 ABI |
| 8/9/10（Pressed/Triggered/Scalar） | 见 7.3 |
| 11（原始触发按钮） | 无栈参数，EAX 返回 `-1`，表示无触发按钮；不能返回 0 |
| 26/27（当前/新活动） | 无栈参数，AL 返回 false，不参与菜单控制器切换或活动设备分配 |
| 35（Poll，纯虚） | 无栈参数，AL 返回 bool；会话有效时返回真；进入退休后返回假，让 InputManager 自行移除 K |

### 7.3 查询输出

| 情况 | 返回 |
| --- | --- |
| B 的 gameplay consumer 上下文，B 已唯一绑定，当前有 SimulationTick | Participant[1] 的 Resolve 结果（12 个 gameplay action） |
| B 的 consumer 上下文，但没有 tick、绑定无效或有阻断 | 0（中性） |
| Join scope 内，pass 查询加入与选人动作 | 普通路径 P1 Triggered(19)，P2 Triggered(14)；其它 action 中性，Pressed(8/10) 必须 false。每个 pass 内查询结果一致，不按查询次数消费；pass 返回后推进状态 |
| 其他一切情况（菜单、暂停菜单、UI、Join scope 外的非 gameplay action） | 0（中性） |

K 只经第 8/9/10 槽输出，不写 Player、不写 InputManager 的状态。K 不采样任何物理输入。

ST1 补充：其他 raw/原始触发/活动入口按上表固定中性；保留的第 17 槽供原生 Join 屏蔽动作，第 30/31/32/33/34 槽保留原生映射合同，第 18 槽保留基类默认向量，第 36 槽保留 no-op。远端 canonical 不经原生 setter 写这些缓存。动作查询还须尊重原生 enable 与既有会话/阻断语义。

ST9 已确认物理设备 action14 可在 `0x4AF870` 选中列表中高亮的 K，`0x4AFAED` 写当前菜单 controller id；`0x4B8C70` 随后直接读尾部。`0x5346B0` 暂停断连处理还会对枚举到的 K 直接读尾部，不能只隐藏选择行。实现须在本地 UI 列表排除 K，并在这些消费者读取前处理 K，物理设备原样透传。13 槽中性继续保留，但不作为全部 UI 安全的证明。

### 7.4 状态

```text
Empty → RawAllocated → BaseConstructed → Initialized → Registered → Retiring → Removed
```

- 创建在 Warmup 中、run 内的稳定 Authority 入口，主线程执行；任何一步失败都以 `RuntimeFailure` 结束会话，不重试。
- 注册与退休均须经过 K 专用的通用输入兼容分派：原生注册会进入映射 profile 回调；第 46 槽/Poll=false 都会进入已安装的原版 Lobby/Netplay 断连回调，不能无条件直接调用。K 的分支须避开这些通知，物理设备保持原逻辑，不改 OnlineManager 或全局 callback 列表。静态依据见 ST2；当前实现为[game 记录](tx-04-06-game.md)中的注册、主动移除和 Poll 通知 shim，尚未运行验证。
- 退休：先标记 Retiring（查询全部中性、Poll 返回假）。在上述兼容分派生效时优先主动移除；已被 Poll=false 移除则只确认第 0 槽通知，禁止重复移除。两条路都观察到槽 0 才记为 Removed。普通退出 run 不关闭全局 InputManager，不能依赖退菜单自动删除 K；构造/移除与物理 worker 的串行化需在实现中明确。

## 8. Join

### 8.1 Join scope

Join pass 入口 Hook 在以下条件同时满足时打开 scope：会话为 Warmup、主线程、K 为 Registered、B 未加入、没有阻断、A 已唯一绑定、Player 数为 1 且是完整 Player、`this` 等于 `Game+0x1BAA8`。原函数恰好调用一次。scope 内重入判为失败。

普通路径按第 7.3 节使用两个合格 pass；这不是固定两个渲染帧。P1 后确认 K 的 Join 行，P2 才确认角色；原生特殊配置可能在 P1 自动确认，若已创建则立即检查结果，不再报告下一次创建。阻断、无空闲行等如实失败，不以墙钟或重复动作自动重试。

### 8.2 mode predicate

三处调用点的补丁先调用原 predicate 一次，再判断：调用方 key 等于 K 的 id 且处于 scope 内时，强制返回“非 Baby”，并确认 manager 与行（第 3 处）一致；其他设备保留原值。不改全局 run flags。原值为“Baby”而被我们改写的次数单独计数（tx-04-07 的异常 Join 场景会专门触发它）。

### 8.3 创建点

- 只对 K 那一行生效：读取行的 key，不等于 K 的 id 时原样放行。
- 对 K：确认 scope 与 manager 一致，读取 CALL shim 入口 `[esp+4]` 的唯一 int32 role；按 8.4 改写为指定角色，并确认该角色可选；失败恢复 shim 保存区后 `add esp,8`（返回地址+role）并跳 `0x5925E4`，结束会话。若更换为 JMP shim 必须重新按只有 role 的 4 字节栈合同实现，不能混用。
- 普通工厂 `SpawnCoPlayer2` 仍是唯一的创建 owner；不额外调用工厂，不写 selector、存档或解锁状态。

### 8.4 B 的角色（D24）

`cs host <port> [character]` 的 character 是 PlayerType 编号，省略时为 0（Isaac）。命令处理时只检查是否为整数；创建点在 manager `+0x70/+0x74` 的 int32 PlayerType 表中查找值，不能把编号当 selected 下标，不改原生表或解锁状态。角色 -1 是原生随机标记，不作为指定完整角色接纳。特殊角色可能创建附属 Player，仍须满足第 8.5 节的实际拓扑。

### 8.5 结果确认与绑定

pass 返回后：Player 数为 2；A 的指针与关键状态未变；B 是完整 Player（Type=1、Variant=0），`_playerIndex==1`，角色与指定一致，`_controllerIndex` 等于 K 的 id。全部满足才建立 B 的 exact pair，并在之后的稳定 Authority 入口执行 `Activate`。任何一项不满足都以失败结束会话，并按第 9.4 节退休。

ST7 时序：工厂在 append/Init 前按既有 Player 数写 `_playerIndex`，因此 1 依赖创建前恰好 A 一项；Type/Variant/SubType 与专用 GetPlayerType 字段在 Init 内写入。Init 内的 controller 先按默认规则设置，PostPlayerInit 回调也在工厂返回之前；Join 在 `0x591FFB` 才把 controller 覆盖成 K。因此不能在 Spawn original 刚返回或初始化回调中提前建立 K 的 exact pair。

## 9. B 的输入与生命周期

### 9.1 Submit

- 主线程在 Manager 入口从收队列取出 B 的命令，进入 Participant[1] 的统一 admission。只有 B 已唯一绑定、session 与 generation 一致时才接受；其他情况丢弃并计数，不缓存到绑定之后。
- tx-01-51 启动例外对 B 同样适用：重新唯一绑定后，首个新鲜命令可用于 completed+1。
- A 的采样与 Submit 不变（tx-04-05）。

### 9.2 Resolve 与 consumer

- Authority-pre 对 A、B 同时 Resolve；B 的 consumer 读取 K 的第 8/9/10 槽，按 7.3 返回。
- 射击分派（tx-01-54）按 Player 的 ordinal 分别判断，B 与 A 规则相同。

### 9.3 阻断与失效

- 暂停、浮层（D7）、加载、切房对 A、B 同时生效：无 tick，输入不生效，事后不补发、不重放边沿。
- 新房间、从实际暂停/加载恢复后，A 与 B 各自失效并在后续稳定 Authority 入口重绑（tx-04-05 第 6.1 节）。
- B 的死亡与 ghost 只做失效与中性，沿用 tx-04-05 的健康判定（valid、exists，且不是 dead、removed、ghost）；复活后的重绑与完整场景留给 tx-04-07。

### 9.4 退休序列

1. 停止接受新输入与新 tick；在途 tick 正常完成提交与发布。
2. B 中性；K 进入 Retiring。
3. 移除 K（第 7.4 节）。
4. 先撤销 B 的 exact pair，再在当前 manager/B 身份仍有效时调用一次 `RemoveCoPlayer 0x590560(B,false)`，确认 Player 数回到 1、A 的明确不变量仍成立。该 API 的第二参数未使用，false 不阻止掉落；其清实体、HUD、重编号、物品队列处理等副作用不能当成简单回滚。
5. 发送 Bye（尽力而为），停止网络线程。
6. 退休 session 与 generation，回到 Idle。

run teardown（`Game::Exit`）时 Player 已由原生清理，跳过第 4 步；K 的移除仍须确认。任何一步失败都记录实际原因，不重试，不强制卸载（规范第 7 节）。

## 10. Hook 与补丁行为表

| 点位 | 未 armed | armed（Host 会话，主线程） |
| --- | --- | --- |
| `Manager::Update` | tx-04-05 行为 | 入口：命令队列、取网络包并 Submit B；出口：TopExit 发布 completed、生成 Ack、快照 |
| Join pass | 计数、压栈 | 第 8.1 节的 scope；结束后按 8.5 确认 |
| mode predicate ×3（字节补丁） | 原 predicate 一次，原值返回 | 第 8.2 节 |
| 创建点（字节补丁） | 原样放行 | 第 8.3 节 |
| 设备第 8/9/10 槽（Keyboard/Gamepad 指针补丁） | 原值 | A 的规则不变（tx-04-05） |
| 设备 K 的第 8/9/10 槽（自有 vtable） | 0 | 第 7.3 节 |
| 通用设备通知、物理断连与 UI 的 K 兼容分支（已编写，未运行） | 物理设备原样透传 | 只按自有 K 身份处理第 7.4 节通知和第 7.3 节直接读取，不进入原版联机回调、不改全局列表 |
| 其余 tx-04-05 Hook | 不变 | 不变，按 ordinal 扩展到 B |

## 11. 命令、状态、trace 与界面

- **命令。** `cs host <port> [character]`、`cs stop`；`cs solo start/stop` 保留。仍是“校验后入队，下一次 Manager 入口执行”。`cs stop` 在 Host 与 Solo 下都可用。
- **`cs status` 增加：** `peer_state`、`peer_address`、`peer_rtt_ms`、`net_rx`、`net_tx`、`net_dropped`（按原因，含 `duplicate` 与 `stale`）、`device_k_state`、`join_state`、`b_bound`、`b_generation`、`b_character`、`queue_depth_b`、`players`。`session_mode` 增加 `Host`。
- **trace 增加：** `net`（握手、Bye、超时、协议错误的摘要，不逐包）、`device_k`（状态迁移、第 0 槽调用）、`join`（scope、predicate、创建、结果）、`binding`（ordinal 1）；`tick` 汇总增加 B 的 current 与 consumer 读数；`ack`（每 30 个一条汇总）。
- **界面。** 主机页：端口输入、【开始主机】【停止】；状态区显示对端地址、往返时延、B 的绑定状态和最近错误；保留“无认证，仅限可信网络”提示。

## 12. 实现前的静态确认（ST1–ST9）

按 D24，沿用 tx-04-05 的做法：GPT-6 Luna / max 子智能体逆向，主智能体独立取证审阅；原始证据保存在 `artifacts/repentogon/tx-04-06-static/`，结论写入静态确认报告。需要 ida-mcp 可用。

| 编号 | 问题 | 结论用于 |
| --- | --- | --- |
| ST1 | **2026-10-25 已完成静态确认与主审**，见[报告](../research/tx-04-06-static-confirmation.md)：38 槽、更新/Join/菜单消费者，2/3/35 的语义与 ABI，0 的参数及三条删除路径 | 第 7.1、7.2 节已修订；第 2 槽保留资源、名称指针、raw/活动中性和 `+0x3FC` 官方 Hook 兼容 |
| ST2 | **静态研究完成**：裸指针 vector、构造分配 id、注册配置回调/原版在线断连回调、全局输入生命周期已读；见[报告第 2 节](../research/tx-04-06-static-remaining.md#2-st2注册移除通知与保留) | 第 7.4 节；通知隔离和 worker 串行化尚待实现 |
| ST3 | **静态研究完成**：19/20/21/14/15 Triggered、8/10 Pressed、行初始化与阻断；普通最小两 pass，查询不按次数消费；特殊配置可在一个 pass 自动确认 | 第 7.3、8.1 节；不是固定两渲染帧 |
| ST4 | **静态确认完成**：三处完整 5 字节 E8，原字节/恢复位置/寄存器见报告第 4 节，第三处保留 ECX | 第 8.2 节，thunk 布局 |
| ST5 | **按 2026-10-25 收窄的两项完成**：①实际只有一个工厂 role push；CALL shim 入口 `[esp+4]`，拒绝清 8 为 shim 返回地址+role，原“两次参数 push”解释不成立；②行 selected 索引 int32 PlayerType 表，再保存 role，-1 才随机。既有调用点/寄存器、拒绝落点、行和表偏移未扩大重查；原型 tx-01-58 仍只作历史索引 | 第 8.3、8.4 节 |
| ST6 | **静态研究完成**：RemoveCoPlayer=`0x590560`，thiscall、Player*+unused bool、retn8；有掉落/实体清理/重编号，不删除 K、不保证 B controller 写 -1 | 第 9.4 节 |
| ST7 | **按 2026-10-25 收窄的写入时机完成**：index 在 append/Init 前按现有数量；类型/角色/初始 controller 在 Init 内；K controller 在 Spawn 返回后由 Join 覆盖。已有偏移不扩大重查，death/ghost 保持第 9.3 节范围 | 第 8.5 节 |
| ST8 | **静态研究完成、无零副作用证明**：已定位条件性解锁/统计、官方初始化回调、控制器配置保存与 Game::Exit dirty→保存顺序；实际条件/回调/运行窗口未观察 | 第 14 节 V12；不能用最终 dirty=0 代替观察 |
| ST9 | **静态研究完成，证实仅中性不足**：物理确认可选中 K，KeyConfig 与暂停断连处理直接读派生尾部；原 0x400 下限不保证安全 | 第 7.3 节；K 消费者过滤尚待实现 |

静态阶段只阅读和反编译，不安装 Hook、不运行游戏、不执行测试。结论区分 `STATIC_CONFIRMED`、`STATIC_INFERENCE` 与 `UNKNOWN`，UNKNOWN 不改成已观察。

## 13. 测试 mod 与 harness

### 13.1 测试 mod

在 tx-04-05 夹具基础上：
- 只读观测扩展到所有 Player：每个 Player 的位置、角色、controller、类型、是否 ghost；Player 数变化时写日志。
- 在主菜单渲染中，按 harness 写入的配置执行一次 `Isaac.ExecuteCommand("cs host <port>")`，覆盖 Lua → 命令桥路径与主菜单 Hosting。
- 第二次 F8 切房沿用 tx-04-05（D21）。

### 13.2 远端客户端（Rust）

- Rust 版协议编解码与 Endpoint，与 DLL 的实现分别编写，不共享代码。
- 收到更高 Ack generation 后清空旧样本、边沿和冗余缓存，再采集新输入；会话 sequence 不重置。首次激活前 generation 为 0，不生成 gameplay 边沿；Warmup/暂停继续中性心跳并更新 last_ack。主机只在可用绑定后公布新代次，避免暂停内提前切代次后重放旧边沿。
- 按场景脚本在指定的 Ack completed tick 之后发送 B 的输入（移动、射击、炸弹），每包带冗余；记录全部收发包（JSONL）。
- 支持主动 Bye 和停止发送（制造超时）。
- 按第 5.1、5.2、5.4 节实现远端一侧的规则：Hello 每 250 ms 原字节重发、握手超时、First Ack 副本按字节判重复、乱序旧 Ack 丢弃并计为 `stale`、Bye 原字节连发 3 份。包日志区分首发、重发、收到的副本与 stale 包。
- relay 与故障注入不在本轮（tx-04-07）。

### 13.3 场景（一次执行，单次启动）

开始前按规范第 8 节告知用户：harness 会接管键盘几分钟，同时在本机运行远端客户端；不需要手动跳动画；启动后不要操作电脑。

| 场景 | 操作 | 主要观察 |
| --- | --- | --- |
| S0 | 环境检查、部署、启动、到达主菜单 | 与 tx-04-05 相同（含 D20–D22） |
| S1 主菜单 Hosting | 测试 mod 在主菜单执行 `cs host` | `Idle → Hosting`；未 armed，透传 |
| S2 握手 | 远端发 Hello | Hosting → Warmup；只锁定一个对端 |
| S3 进入 run 与 Join | harness 开新局 | K 创建并注册；Join 接纳 K；Player 数为 2，B 为 Isaac；A/B 唯一绑定后 Active |
| S4 B 移动 | 远端脚本让 B 右移、上移后松开 | B 的 Resolve、consumer 非零读数与 Lua 位置一致；松开后在 lead+1 内中性 |
| S5 A/B 同时输入 | A 物理移动，同时 B 射击 | 同一 tick 中 A、B 各自 Resolve；A 的输入不影响 B，反之亦然 |
| S6 B 炸弹 | 远端发一次 bomb edge | 恰好一次被接受、只在一个 tick Resolve；炸弹数减一 |
| S7 浮层 | A 打开 REPENTOGON 控制台 1 秒，期间远端持续发送输入 | 浮层期间无 StandardRoom tick，B 的输入不生效，事后不重放 |
| S8 切房 | 第二次 F8 切到相邻房间 | 加载期间无 tick；A、B 各自失效后重绑；B 之后仍可移动 |
| S9 stop | 控制口 `cs stop` | B 中性 → K 移除 → B 移除，Player 数回到 1；远端收到 Bye；Idle；A 原生可控 |
| S10 超时 | 再次 `cs host`，远端握手并让 B 加入后停止发送 | `PeerTimeout` 结束会话，B 与 K 按第 9.4 节退休 |
| S11 teardown | 第三次 `cs host`，B 加入后退回主菜单 | run teardown 结束会话；K 移除确认；Idle |

## 14. E2E 验收（本次推进唯一的一次测试）

一次执行，失败只记录，不自动重跑。

| 编号 | 判定 |
| --- | --- |
| V1 加载 | 能力组全部通过；入口 Hook 数与清单一致；设备槽补丁 8 个、Odd shim、4 处 Join 补丁均已安装 |
| V2 透传 | 会话未 armed（Idle、Hosting）期间查询覆写为 0、tick 计数不变，A 原生可控 |
| V3 会话 | 三轮状态迁移完整（结束原因分别为 Stop、PeerTimeout、RunTeardown），generation 递增，无 `tick_reject` |
| V4 协议 | 每轮握手只建立一次（Hello 重发、字节相同的副本与 stale 包单独计数，不算错误；回环下预期 stale 为 0，但不为 0 本身不判失败）；只锁定一个对端；双方收发包数可对齐；没有 CRC 或格式错误；远端收到 Bye |
| V5 Join | 每轮 K 恰好创建一次；Join 恰好产生一个 B；B 为完整 Player、角色为 Isaac、controller 为 K 的 id；三处 predicate 与创建点的原调用各恰好一次 |
| V6 B 输入 | S4 中 B 的 Resolve、consumer 读数与 Lua 位置一致；松开后在 lead+1 内中性；B 的输入只经 Participant[1] |
| V7 同一 tick | S5 中同一 tick 内 A、B 都有 Resolve；互不串扰；Authority 不等待 B |
| V8 炸弹 | S6 中 bomb edge 恰好一次被接受、只在一个目标 tick Resolve，炸弹数减一 |
| V9 阻断与切房 | S7、S8 期间没有 StandardRoom tick；B 的输入不生效、事后不重放；切房后 A、B 都重新绑定 |
| V10 退休 | 每轮结束时 B 中性、K 的第 0 槽被调用（不释放）、Player 数回到 1、A 未受影响、网络线程已停止 |
| V11 tick | 与 tx-04-05 V4 相同的 tick 完整性要求，扩展到 A/B 两个参与者 |
| V12 退出与副作用 | 正常退出；没有崩溃转储；存档变化按 D18/D20 规则（D27 起不判定每日备份）；`trace_dropped=0`、`context_faults=0`、非主线程调用为 0；覆盖创建至退役的完整窗口，记录 ST8 写入入口及条件，确认无未经允许的 C/S 新增持久副作用。原生退出保存按既有基线区分，不用最终 dirty=0 代替过程依据 |

**预先列出的失败情况：**

| 编号 | 情况 |
| --- | --- |
| F1 | 握手失败、锁定了错误的对端，或协议错误 |
| F2 | K 创建、注册或移除失败；第 0 槽释放了内存 |
| F3 | Join 没有产生 B、产生了 Baby、角色不对，或 A 受到影响 |
| F4 | B 的输入绕过统一队列，或 consumer 读到的不是 Resolve 结果 |
| F5 | A、B 输入串扰，或 Authority 因 B 缺输入而等待 |
| F6 | 边沿重复或丢失 |
| F7 | 阻断或切房期间出现 tick，或恢复后重放边沿 |
| F8 | 结束后 B 或 K 残留、查询仍被覆写、网络线程未停止 |
| F9 | trace 丢弃、上下文错误或非主线程访问游戏对象 |
| F10 | 崩溃、卡死或不允许的存档变化 |

**artifact：** 与 tx-04-05 相同的环境、部署、存档快照与报告；另加远端客户端的全部收发包、主机侧 net/device_k/join trace、各场景步骤与关键截图。

## 15. 用户决定与风险

**用户决定（2026-10-24 全部按下表建议确认，记为设计 D25）：**

| 编号 | 事项 | 建议（已确认） |
| --- | --- | --- |
| U1 | 默认绑定地址 | 配置项 `host.bind_address`，默认 `0.0.0.0`（真实局域网可用）；E2E 部署配置写 `127.0.0.1` |
| U2 | 对端超时 | 默认 3000 ms，配置项 `host.peer_timeout_ms` |
| U3 | 界面是否提供角色选择 | 首版不提供，只经 `cs host` 参数指定；界面固定默认 Isaac |
| U4 | 主菜单 Hosting 的发起方式 | 由测试 mod 在主菜单执行 `cs host`，同时覆盖 Lua 命令桥；控制口用于 stop 与后两轮 host |

**风险：**

| 编号 | 风险 | 处理 |
| --- | --- | --- |
| R1 | K 报告序列或特殊角色拓扑不满足最小 A/B 路径 | ST3/5/7 已静态确认；按实际 pass/创建结果判定，不自动重试 |
| R2 | InputManager 删除 K 的时机与我们的退休序列交错 | 第 0 槽只标记；两条退休路径都必须观察到第 0 槽 |
| R3 | 原生创建/移除有条件性解锁、统计、掉落及官方回调副作用 | ST6/ST8 已定位；不修改全局 readonly/dirty 回滚，V12 按完整窗口判定 |
| R4 | 网络线程与主线程的队列在高包率下丢弃 | 有界队列计数；输入有冗余；E2E 只在本机回环 |
| R5 | 远端 lead 估计不当导致大量过期 | 远端 lead 可配置；过期计数进入 Ack 与报告，不放宽 admission |
| R6 | 一次会话中多次创建 K 造成内存常驻增长 | NEVER 策略的已知代价；每个会话至多一个 K，E2E 中三轮共三个 |
| R7 | 原生注册/删除通知进入持久映射配置或原版联机回调 | ST2 已确认；通用输入边界按 K 身份处理，物理设备透传，兼容分派尚未实现 |
| R8 | 中性 K 仍可被物理 UI 选中，或被断连处理直接读派生尾部 | ST9 已给反例；本地 UI 列表和读取前过滤都需实现，不靠名称或 13 槽中性声称安全 |
