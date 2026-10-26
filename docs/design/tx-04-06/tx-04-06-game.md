# tx-04-06：game/ 主线程实现与接线记录

日期：2026-10-25。范围为[实现方案](tx-04-06-implementation-plan.md)第 **3.3 节 game/**（用户本次称“3.2 game/”），以及它依赖的第 3.4 节常驻 Join/K 补丁。仍属于 tx-04-06 同一次推进，不新增推进编号。已编写设备 K、Join、主线程网络桥、双参与者生命周期/查询/诊断和必要的 Hook 接线；纯编译记录见文末。

本阶段没有部署 DLL、启动游戏、运行 worker、发送网络包或执行测试，没有修改存档。Host 的主线程 API 已实现，**`cs host/stop` 仍保留 ops 原有占位命令**，待第 3.5/11 节接入；不能据本次代码或构建宣称联机已经可用。唯一整体 E2E 留待 tx-04-06 全目标完成后执行。

## 文件与职责

| 文件 | 实现 |
| --- | --- |
| `game/device_k.h/.cpp` | v1.9.7.12 的基类构造/Initialize、自有 38 槽表的 13 槽覆写、常驻 root、注册与唯一移除 |
| `game/input_lease.cpp`、`game/bridge.h` | 与物理 worker 共用原生后端 Mutex；主线程嵌套深度避免该 Mutex 的递归等待；提供 C++17 可调用的作用域 |
| `game/join.h/.cpp` | 固定 P1/P2 动作、三处 mode 判定、唯一 role 参数、原生角色表值检查、完整 pass 返回后确认 B、原生 B 退休 |
| `game/peer.h/.cpp` | Worker 状态优先处理、握手后准备 core、Manager 有界取包/统一 admission、TopExit Ack、接收窗口与序号下界、网络有界停止 |
| `game/state.h`、`game/tick.*` | 原 SoloRuntime 改为 SessionRuntime；Solo/Host 互斥，两个 ordinal 共用 TickPipeline；增加 StartHost/StopSession、双玩家回执/trace 和只读状态字段 |
| `game/topology.*`、`game/lifecycle.*` | A/B 的唯一 Player/device/id 对应，房间/暂停/加载失效与稳定重绑，B 死亡只撤销 B，teardown 前关闭会话，正常 TopExit 退休 |
| `game/query.cpp`、`game/consumers.*`、`game/input_a.*` | 按受控 Player 查 ordinal，A/B 共用 Resolve 和射击抑制规则；A 物理源的 canonical/Submit 语义不变 |
| `hooks/host_patch.cpp`、`hooks/odd_patch.cpp`、`binding/hooks.json`、`tools/gen_binding.py` | 统一常驻清单中的 11 处新补丁，和已有 Odd/设备槽安装入口协调；生成 Host 补丁规格与 manifest |
| `net/worker.h/.cpp` | 仅增加本地值元数据：从已成功发送 Ack 的记录查得 Input 回显的 generation/state；不改变 ICSN v1 字节格式，不读取游戏对象 |
| `ops/command.cpp` | 仅将 run exit 改为停止通用 Session；Host/stop 命令解析与执行仍未替换 |

新增功能、核心结构和函数均有中文注释。原 `core/` 的 codec、Participant、queue/dedupe/Resolve 没有为 B 新增分支；本次没有修改其已交付代码。

## Host 主线程顺序

1. 后续命令层在最外层 Manager 入口调用 `StartHost(WorkerConfig, character)`。它与 Solo 共用 Busy 检查，启动一次性 Worker，保留系统随机 HostIdentity 与新的本地会话 generation。主菜单可监听，此时为 Hosting，未 armed、A 保持原生输入。
2. Manager 入口先处理 Worker 粘性 terminal，再观察 Established。握手完成才 `Session::Start(identity, Host)`，初始化 A=ordinal0、B=ordinal1，进入 Warmup。主菜单 Warmup 等待首次 run；只有实际经历 run 后离开，或原生 Exit/Restore 开始，才按 teardown 退休。
3. 首个稳定 Authority 入口绑定 A、创建 K；本次返回不启动 tick。之后合格的原生 Join pass 在固定合成动作快照中执行，整个 pass 正常返回后才确认和绑定 B。
4. 后续稳定 Authority 入口首次双绑定 Activate，A/B 同步 Begin/Resolve；B 缺输入立即中性，不等待网络。A 的物理输入仍走原有 codec/流水线 admission，B 经同一 Participant admission 入队。
5. 原生模拟正常返回提交 previous，TopExit 完成 completed/回执发布，并按 ordinal 分别输出 tick 诊断；之后复制 B 的进度生成 Ack。无 Authority tick 时只发布不变的 completed，不用墙钟推进模拟。

`StartHost` 需要已安装的原设备槽、Odd 补丁和全部 Host 补丁；失败返回具体原因，不创建一个部分可用的 Host。下一会话重新构造 SessionRuntime/Worker/Participant，不复用旧 K。

## K 的对象、线程和生命周期

K 的原生区与名称兼容区是 `0x400` 字节；`+0x3FC` 在调用官方 Initialize 链之前写入常驻名称。私有状态位于外部记录，未用 `+0x3F8` 伪造物理平台 id，也未填充 Gamepad `+0x48C/+0x498` 字段。已定位的直接消费者由下表过滤，`0x400` 不是全映像消费者安全证明。

从基类 38 槽表复制，只覆写 0/2/3/5/6/7/8/9/10/11/26/27/35。槽 0/2 保留对象及有效基类资源，槽 3 返回名称，raw/活动查询中性、原始触发按钮返回 -1，8/9/10 仅在 Join 或受控 consumer 中返回结果，35 在 Registered 为真。Initialize、注册/移除调用既有原生入口，保留官方调用链；不复制物理派生 vtable，不修改游戏原表。

首次原生构造前固定 DLL，发布强 root，进入构造后按 NEVER 保留；每会话至多创建一次，失败不重试。原生 id 在注册前必须空闲、注册后唯一。退休以槽 0 删除通知和全局设备 vector 缺席共同确认；已被 Poll 删除时不重复调用 RemoveDevice。进入过注册但没有删除通知，或出现重复/未知设备关联时，不报告成功退休。

本轮静态补充确认了物理 worker `0x5EC464..0x5EC497` 持有 `InputManager+0x60`（RVA `0x7CE498`）后端 Mutex，Lock=`0x5E60E0`、Unlock=`0x5E61A0`。主线程安装物理断连补丁、构造/注册/移除 K、读取设备拓扑、Join 和 consumer 都按 **后端锁 → 原生 vector 锁** 顺序进入。Input 更新与 consumer 在调用原生 body 之前取得外层 InputLease，避免在 InputManager action 查询已持 vector 锁后反向取锁。本线程显式深度绕过原生 Mutex 的不可递归 held 标记；不改其锁状态实现。实际运行交错、异常和第三方线程仍待整体 E2E，不宣称全并发/OOM/EH 闭包已被验证。

## 常驻补丁与原生语义

首次 Manager 主线程通过既有统一安装入口执行；所有指令规格、完整长度和分支落点在 `mod/binding/hooks.json`，生成 `host_patches.inc`。先确认已知原始指令与 Mutex 依赖，持后端锁安装，进程结束前常驻；未建立 K 时物理设备按原指令分派。任一不符关闭 Join/Host 能力，不恢复已安装字节，也不猜测其他插件补丁。

| id / RVA | K 行为 |
| --- | --- |
| mode ×3：`0x5915AE / 0x59174C / 0x591E67` | 原 predicate 各调用一次，只在自有 Join scope 对 K 返回非 Baby；第三处保留原 ECX key |
| create：`0x591FE6` | 查角色表值后改唯一栈 role；拒绝清 shim 返回地址与 role 共 8 字节，跳 `0x5925E4`；普通工厂仍唯一 owner |
| register notify：`0x5EEB7C` | 完成原生 append 后跳过 K 的注册通知，不新增控制器 profile |
| remove notify：`0x5EEBC3` | K 跳过通用断连通知，保留原生 erase 与槽 2/0 |
| poll notify：`0x5EDD2D` | K 跳过 Poll=false 的通用断连通知，保留删除路径 |
| physical disconnect：`0x5EC37C` | 在读取派生 `+0x3F8` 之前排除 K |
| controller list：`0x4AF970` | 在局部列表分配/复制名称之前排除 K，不改全局设备 vector |
| keyconfig draw：`0x4B8D3D` | 若意外选中 K，在派生尾部读取前走已有绘制收尾；正常 UI 列表中没有 K |
| pause disconnect：`0x534768` | 在尾部读取前跳过 K，继续枚举物理设备；不覆盖官方 Unassign/Reassign/LoopEnd 补丁 |

这些加上原有 Odd 补丁，共 12 处字节补丁；入口 Hook 仍为原有 28 个，物理设备槽仍为 8 个。裸 shim 保存 GP/EFLAGS/x87/MXCSR/XMM；原 predicate 和普通工厂按各自真实 ABI 调用，未给物理设备改角色或改全局 run flags。

补充静态调用链：Join 所在 `Game::ProcessInput=0x2F66E0` 由 Manager `0x52E89F` 调用，Join call 为 `0x2F6D21`，早于 Game::Update；scope 因此要求 Manager/Join owner，不要求尚未出现的 Game owner。证据保存在 [Mutex/ProcessInput 导出](../../artifacts/repentogon/tx-04-06-static/game-input-lease-evidence.json)和[完整补丁窗口](../../artifacts/repentogon/tx-04-06-static/game-patch-windows.json)，没有计算或校验哈希。

## Join 与拓扑

普通 P1 只报告 Triggered(19)，P2 只报告 Triggered(14)，每个 pass 内一致。P1 返回必须有唯一 K 行；原生自动确认已创建时立即确认结果，不再发 P2。角色编号按 `manager+0x70/+0x74` 的 int32 值表查找，-1 不作为指定完整角色，不改 selected 或解锁表。

完整 pass 返回后检查：Player 数恰好 2、A 的 pair/生命状态及 Game/room 明确不变量保持、B 为 Type1/Variant0、playerIndex1、专用 PlayerType 等于配置、controller id 唯一指向 K。工厂刚返回和 PostPlayerInit 期间不提前绑定 K；Join scope 内的原生初始化也不被当成稳定拓扑刷新时机。特殊角色附属 Player 超出实际双人拓扑时失败，不凭角色编号预先宣称支持。

全局暂停、D7、加载、房间变化对 A/B 同时失效/中性，恢复后在稳定 Authority 重绑。B 的死亡/removed/ghost 只撤销 B，Active 的 A 仍可经同一 scheduler 继续；B 保持中性，不在本阶段自动接纳复活后重绑。完整 B 死亡/复活、异常 Baby=true Join 和跨房射击专项仍留 tx-04-07。

## 网络接收代次与后续 Rust 合同

每个 Manager 最多取 32 包、128 条命令，冗余按旧到新、最后当前命令；只投给 Participant[1]，不按 payload 改投 A。队列、合法性、冲突和去重仍由既有 core 处理。SequenceConflict 结束整个会话，普通重复/过期等保留原 Decision；网络 stale 不变成 core 输入错误。

失效/屏蔽期间消费并丢弃，保存最大已观察序号与丢弃下界；重绑不把旧接收窗口赋予新代次。仅靠包的接收时间不足以区分重发冗余，因此 Worker 在成功发送 Ack 后保存其 packet sequence、输入 generation/state，并在 Input 回显 `last_ack` 时将对应信息复制到本地 ReceivedInput；记录缺失时值为零，保守丢弃。它不改包格式、CRC、网络身份或 packet sequence 规则，也不接触 Session 对象。

**Ack 的 generation 是已公布可用的 B 输入代次。** 首次激活前为 0；只在 Active、未阻断且 B 已绑定时推进到 Participant 当前 generation，暂停/未绑定期间保持上次公布值。这样远端不会在暂停中提前采用新代次。主线程只接受回显当前代次 Active Ack、越过接收时间边界且 command sequence 高于已丢弃下界的命令；旧 Ack、旧积压和旧冗余不在重绑后复活。`cs status` 的 `b_generation` 仍如实给出 Participant 当前代次。

后续 Rust 客户端必须在收到更高 Ack generation 时清空旧样本/边沿/冗余历史，再采集新样本；**会话 sequence 不重置**。Warmup/暂停仍发送中性心跳并更新 last_ack，不能把原窗口的 held/edge 脚本留给新代次。首个新鲜命令可按已有启动规则选择 completed+1，连续量补偿由远端用新的 sequence 发送 completed+lead 的 edge=0 命令；主机不改写 target 或制造 B 的 sequence。该客户端尚未实现或运行。

## 退休与未验证范围

StopSession 的 PeerBye/PeerTimeout/PeerProtocol/Stop/RunTeardown 关闭新输入和新 tick；已进入的 tick 正常提交与发布。TopExit 排空主线程 owner 后：K 中性并移除 → 撤销 B exact pair、在当前 manager/B/A 仍可确认时调用一次 `RemoveCoPlayer(B,false)` → 检查回到一个 A → 尽力 Bye、最多等待网络 1000 ms → 退休 core 并回 Idle。Exit/Restore 开始前标记 teardown，原生清 Player 后跳过旧 B 移除。

任何移除、关联或线程退出确认失败均保留 `Retiring/cleanup_failed` 和实际原因，不自动重试原生移除、不卸载 DLL、不允许新会话覆盖现场。core 的 CleanupComplete 不覆盖 game 保存的原始退休原因，Worker 的粘性 terminal 也保留真实原因。

未验证项如实保留：官方实际 Hook 安装顺序/冲突、运行时并发与异常、未知直接设备消费者、特殊角色附属对象、pending Join 行的原生后续清理，以及原生创建/RemoveCoPlayer 的完整持久副作用窗口。实现没有翻转 PGD readonly、回滚 achievements/counters 或伪造 dirty；普通原生掉落/关联清理仍由原 API 完成。不能由纯构建推导这些路径已安全或 V12 已通过。

## 构建与剩余工程

Win32 / v143 / Release 的最终纯编译与 DLL 链接成功，game/ 与 host_patch 无编译 warning；net SPSC 和 SDK/生成 wrapper 的既有提示保留。见[纯构建原始记录](../../artifacts/repentogon/tx-04-06-game/README.md)，它不是 E2E 结果。后续工作是 Host/stop 命令与配置、UI、Rust 远端和 tx-04-06 harness（包含既定 V2 判定修复），然后按项目规范执行一次整体验收；本阶段未消费该次验收。
