> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-04 Native Join Active 独立测试计划 v1

## 1. 状态、职责与硬门

- 日期：2026-08-31。
- 状态：`BLOCKED_GATE_IMPLEMENTED_RUNTIME_NOT_STARTED`；本文记录独立测试设计及当前执行边界，不是 runtime-complete 或实机授权。
- 范围：`native-join-active` 的独立公开 CLI、Win32 IPC、Native ABI/事务、原生 join 因果链、恢复及 replay 验证。
- 主智能体实现生产逻辑；逆向子智能体冻结原始指令、owner/ABI/生命周期和持久化证据；测试子智能体独立设计和执行验收。测试不修改生产判据以使其通过。
- 本轮不启动、注入或控制游戏，不消费 live 授权，不覆盖历史 source/analysis/decision。完整自动门通过后，实机仍须新的独立单次授权。

继承 [研究计划第 9、14、15 节](../00-overview/isaac-cs-t0-05-research-plan.md) 的全部强制条件。当前无持久污染条件尚未证明；[静态证据](../../../../../../cs-t0/coop-native-join-static-v1.json) 已固定 ordinary Init 经 P0 更新到 achievement/dirty/Steam 的条件路径，以及尾部直接持久计数写入和 player-init Lua callback。不能把这些已确认的可达路径视为已排除。在 save、achievement、unlock、统计及 run 外持久化边界未闭合时，必须证明所有 active 入口不可达；可通过纯测试内存验证内部组件，但不得用 synthetic fixture、手填布尔值或 PASS 摘要获得 live 资格。

因此当前实际实现与验收仅为 `coop-join-preflight` 的磁盘静态证据和 fail-closed 门；后文 Native callback/55 槽事务/IPC/runtime 测试是证据闭合后的待办，本轮未实现、未执行，不计入通过数。当前独立公开黑盒为 8 组，完整继承门由 `verify-coop-native-join.ps1` 另行记录实际结果。

允许为 active prejoin 新建有证据支持的限域离线判据；不改旧 C0/passive 规则。新判据必须先于主动 UI edge 成立，不能引用尚未发生的 `46→49→51` ordinary route 作为先验。依据不足时保持 `BLOCKED_BY_EVIDENCE`。

## 2. 实施前独立只读核对

工作目录为 `D:\projects\isaac_hook_reserach`，PowerShell；使用 `Get-Content -Raw | ConvertFrom-Json` 读取声明，再逐项执行 `Get-FileHash -LiteralPath <path> -Algorithm SHA256` 比对。没有构建、写产物、创建 mapping 或附着目标。

| 核对对象 | 实际结果 |
| --- | --- |
| `coop-passive-applicability-baseline-v1.json` 的 149 条绑定 | 149/149 SHA-256 匹配 |
| baseline 文件本体 | `D56C17F366302A84A097EA40276D19B9966511CB168CCFE94B507A11CFD5EBC1` |
| plan v6 本体及 progress v8 对其声明 | `820F2F2F8410BE93EEFB98A1E2E3C66A1A9A60D090D901EF22FEDFE932991D59`，匹配 |
| plan v6 的 decision/matrix/ledger/hypothesis/verification 绑定 | 5/5 匹配 |
| verification v6 本体 | `6971C6A84867FBC15FBB63BC1F2924B7AB7D1CCF4717750DC95CB90087CC55FF` |
| 当前 `IsaacCsT0.exe` | `3DD239729F1D36F1236E16A5392266273944A837D1825628A519862ECD74F365`，匹配 verification v6 |
| 当前 `IsaacCsT0.Native.dll` | `768CDDEBE490236789F6C542AAA7C9F84B31778EE1A3655DC4F23934AD4F720A`，匹配 verification v6 |
| 当前 `IsaacCsT0.Helper.exe` | `3170D805B6F1F2D0BDE4E4F93D38D982A71AC9F65BC9E973DF00B42F63D03972`，匹配 verification v6 |

此记录仅证明冻结前输入与声明相符，不替代 05-04 自动门。后续新生产二进制必须由新门独立绑定，不得继续冒用旧 verification。

## 3. 固定测试对象和独立观测边界

### 3.1 两个原生 callback operand

采用原生 InputManager callback 参数替换方案，不使用共享 `GamepadTriggered` vtable wrapper 的返回地址推断，也不合成直接 spawn/assignment/HUD 调用。

| 槽 | 指令 RVA | operand RVA | 冻结指令 | 原 callback RVA | 用途 |
| --- | --- | --- | --- | --- | --- |
| 53 | `0x005BB057` | `0x005BB058` | `68 E0 09 A2 00` | `0x006209E0` | 限定 B 设备的 action 19 选择 |
| 54 | `0x005BB38B` | `0x005BB38C` | `68 E0 09 A2 00` | `0x006209E0` | 下一自然 pass 的 action 14 确认 |

两项都是 4 字节对齐的 image pointer operand，属于同一个 55 槽事务：旧 non-device passive 的 53 槽加这两个 operand；不引入 device-v2 的额外三槽。安装顺序为 `0..54`，恢复为 `54..0`，完整 mask 为 `0x007FFFFFFFFFFFFF`。间接 `FF D0` 不是 `REL32` patch 点，不得误当 `E8 rel32` 修改。冻结字节中的绝对地址对应既有 image base；运行时必须使用经目标等价和重定位证明的 `moduleBase + RVA`，不能把固定 VA 当成跨实例身份。

两个独立 `cdecl` callback wrapper 保留原 callback ABI：四个 dword 参数依次为 `device*`、`control*`、`action*`、`extra`；当前原生调用的 `extra=0`，原 callback 不读取第四项。wrapper 在调用前取得必要身份，再调用原 callback 恰好一次，返回后仅在全部准入条件成立时覆盖对应的一次布尔结果；不得再次解引用可能已经释放的 control。原 callback 保有设备引用和 refcount 处理；wrapper 不接管、不额外增减引用。

两个 edge 分属连续的自然 manager pass：选择 pass 返回后验证仍为单 Player，唯一 selector 的 `type=0`、`selectionIndex=1`、`scale=1`、`autoConfirm=0`；下一自然 pass 重新准入后才允许 action14。二者须绑定同一 owner/generation/B 设备，不能直接重调 manager。确认 pass 内仍要求完整 ordinary route 与原生尾部闭合。

精确 owner、manager transaction、参数/设备约束和原生尾部必须以逆向交接的原始指令及新机器契约为准；测试不得自行填补未知约束。覆盖 `true` 只允许 action19/action14 UI 语义，不授权 B gameplay、synthetic input apply、scheduler 或 transport。

### 3.2 测试 seam 的允许范围

- Native harness 自己分配测试内存、构造合法及非法对象图、提供记录原始参数和调用计数的 callee stub，并独立读回实际内存及事件。
- 必须执行生产准入、callback wrapper、one-shot 状态机、patch 事务、Event/Publish、Shutdown；不得只测复制的策略函数。
- 允许替换底层原生 callee、内存读取失败点、时钟和 OS/observer 依赖。替换接口返回原始结果，不能返回断言结论或生产自报 PASS。
- 跨语言桥接使用实际 Native 事件及 shared 终止状态。不得补造 terminal envelope、重编号 sequence 或归一化 generation。
- synthetic source 只用于 analyzer、拒绝分支及内存模型验收，必须标记为测试输入，不能进入真实 source/matrix 或用于解锁 live。

## 4. 独立验收矩阵

### J01：版本、公开 CLI 与授权隔离

- 通过公开 CLI 检查新 join schema/identity/scenario 的精确 dispatch；旧 passive、T0-04 input grant/claim、旧 verification 与旧 mapping 不能开启 join。
- 检查未知/重复字段、重复开关、权限扩张、错误 target/equivalence/catalog/contract/verification 哈希、缺项和过期报告均拒绝。
- 新 grant/claim 绑定 case、scenario、进程创建时间、generation、目标及完整当前验证链；token 必须非零、精确长度，artifact 和 claim 使用 create-new。
- 同 token 的竞争 claim 只有一个成功；改变路径、复制 grant、旧 token/claim 或 PID 复用不能重新获得执行机会。失败、超时、中断或结果丢失不恢复 arm。
- 无授权或证据不完整的拒绝发生在 mapping/注入/Hook/override 之前；独立记录这些操作次数均为零。
- 持久化仍 UNKNOWN 时，即使其他字段和 synthetic fixture 全部合法，所有生产 active 入口仍必须拒绝，并保持各项 active 能力和 live 授权为 false。

### J02：独立 prejoin 与时间顺序

- 正向内存用例为同一运行中的单 A、唯一未分配 B 手柄、稳定本地 owner/vector/HUD、冻结设备类型与目标字节，并满足已冻结的新 active 离线判据。
- 穷举未知/错位/不可读 backend、错误 vtable、roster 三指针任一异常、allocated-empty/nonempty roster、onlineFrame、NetInputDevice、错误 manager、第三 Player、null/duplicate Player、非唯一 assignment 和双读漂移。
- prejoin 正向依据不能包含 ordinary route witness；删除未来 route 数据不得改变先验的判定，伪造 future/stale route 不得把拒绝变成准入。
- 主线程、phase/depth、同 manager transaction、进程创建时间、generation、owner 或 B 设备在准入后变化时，原 callback 继续 original-once，但 adapter 不产生新 edge。
- 两个 UI edge 之间发生生命周期失效、退出、断连、selector 不符或 timeout 时永久 disarm；不能重新发 action19，也不能跳过选择 pass 返回证明及下一自然 pass 的重新准入。

### J03：callback ABI、original-once 与一次性 UI 覆盖

- 独立 `cdecl` stub 记录四个原始参数、调用次序和返回值；检查调用前后 ESP、栈哨兵、EBX/ESI/EDI/EBP，以及冻结布尔返回语义。不能把未定义的高位 EAX 作为额外 ABI 要求。
- 对 selector 和 confirmer 分别测试 original false/true、正常和拒绝、wrong-thread、publication 失败、读取异常、递归/嵌套等分支；每次进入 wrapper 的 original 都恰好一次。
- 只有正确 wrapper/action、冻结 owner/transaction、同一未分配 B 设备及对应状态允许覆盖一次。A、其他手柄、键盘、错误 control/action 指针、extra 非零、其他 action、重复和乱序调用保留原值并不增加 adapter edge 计数。
- original 已返回自然 true 时不能伪称 adapter 造成了该 edge；实际自然加入或人工操作导致 provenance 不唯一时必须阻断主动证明。
- 记录并检查原 callback 对设备引用的处理次数；wrapper 不另作 AddRef/Release，也不隐藏原 callback 的失败。让 original 在返回前撤销 control 的可读性，检查 wrapper 返回阶段不再访问它。
- 完整流程仅一个 action19 override 和一个 action14 override；完成、停止或失败后反复查询均无新的覆盖。其他 UI/gameplay override、direct spawn/assignment/HUD 调用、scheduler/apply/transport 次数为零。

### J04：原生 join 因果链和映射

- 成功链必须由生产 callback/manager 流程产生：已准入 → action19 单次覆盖 → manager 返回及唯一 selector 证明 → 下一自然 pass 重新准入 → action14 单次覆盖 → 确认 pass 同事务完整 `46→49→51` → 已冻结原生尾部完成 → A/B Candidate → 下一连续完整 pass 双 BOUND。
- 独立检查所有相关 original 调用次数、返回、phase/depth/owner/generation，以及 A→0/B→1、唯一 Player/device/assignment、HUD/index/Lua 顺序与冻结 ordinal。
- 删除、复制、重排或跨代拼接任一 query、wrapper、尾部或 binding 事件必须拒绝；只有两个 Player 或自报 flags 不构成正向证据。
- join 失败、原生返回异常、partial route、possessor、退出路线或其他生成来源均不通过；不直接调用删除函数作为清理。
- 已尝试一次后，即使未出现双 BOUND，也不能自动补发 UI edge、重跑 join 或重新授权。

### J05：55 槽 operand 安全事务

- 测试独立构造 `PUSH imm32` 原指令，验证只改四字节 operand，opcode `68`、相邻指令与间接 `FF D0` 保持不变；验证旧目标是经过证明的 callback，wrapper 地址属于当前模块。
- 检查错误 opcode、错位 operand、截断/跨页读写、错误重定位/目标值、重复槽、冲突 owner 或未知 patch kind 均 fail closed；不得退化成盲写。
- 对全部 55 槽逐点注入安装失败、读回失败、rollback 失败和 restore 失败，覆盖 pointer 与 relative 两种已有 patch 类型及新增两项 image pointer。
- 检查正向安装 `0..54`、反向恢复 `54..0`、完整 mask、逐槽 proof 与 before/after snapshot；快照由独立内存读回重算，不能仅比较生产自报摘要。
- 第三方改写已安装 operand 或恢复前字节漂移必须报告冲突/Unsafe，不覆盖外来 patch。正常重试恢复仅限冻结安全契约允许的步骤，不能重新运行 join。

### J06：恢复、终止与控制器失败策略

- 执行真实生产停止 admission/publication、callback drain、身份退休/私有清理、逆序恢复、逐槽复核和 producer close 顺序。
- 验证 actual callback pending、drain timeout、restore/verify/snapshot 失败、两个 terminal Publish 分别失败、ring full 和 process identity 变化。
- 正常终止事件及 manifest/shared 的最终 generation 一致；关闭观察后的私有清理不得错误推进最后观测 generation。真实生命周期 invalidation 仍按契约推进。
- 任何不确定恢复保持 Unsafe；controller 不调用强制 unload、不重注入、不覆盖失败 artifact、不以 Stopped 掩盖缺失终止事件。
- 正向安全停止后检查 raw callback/operand 还原、private owner/device/token 清空、旧查询不能重新激活；实机阶段另查进程响应与模块/mapping 零残留。

### J07：持久化证据与污染快照

- 由逆向交接先冻结全部适用的 save/achievement/unlock/statistics/run 外持久化观察面、触发条件和必要测试环境；静态未知项不能靠人工确认或零计数默认通过。
- 对读取失败、缺失对象/文件、哈希或作用域不匹配、采集中途目标变化、遗漏观测面、异步写入尚未可判定等情况，生产 active 入口必须保持不可达。
- 污染报告必须与同一个 case/process creation/generation 和 source 绑定；before/after 不得借用其他 run、旧快照或替换为 fixture。
- target/Hook snapshot 仅证明 patch 恢复，不能替代持久化观察。仅磁盘文件未变不能自行升级为 Steam achievement/statistics 无副作用证明。
- 允许冻结契约中明确的 B 创建及本 Run 改变；任何未获证明的 run 外副作用保持 BLOCKED，不改 analyzer 接受范围。

### J08：真实 IPC、公开 analyzer 与 replay

- 在 Debug/Release 分别进行 C++ publisher → 实际 named mapping → Rust reader；独立核对 size/align/offset、数字 codebook、process creation、nonce、schema/identity 和固定容量。
- 覆盖未提交 slot、错误 published sequence、未知 flags/kind、gap、overwrite、QPC/generation 回退、环容量越界、重复 mapping、关闭后发布及消费者落后。
- 使用生产 wrapper/Event/Publish/Shutdown 输出建立 Native→独立序列化→公开 `coop-analyze` 桥接；断言预期由本计划和独立观察决定，不取决于 runtime 自报 PASS。
- 逐项篡改 source identity、event、manifest、原始调用次数、one-shot 因果、权限、目标哈希、恢复/污染证明和隐私字段；缺失/重复字段与 stale source 均拒绝。
- 所有 analysis/replay 输出 create-new，不得写入或覆盖 source；真实 live source 的单次公开 replay 必须与原 analysis 逐字节一致，源文件前后 SHA 不变。
- 缺少 Native harness 或环境变量时不得把跨语言测试的空执行纳入自动门 PASS；新 verifier 必须确认实际测试名称、数量、配置及成功摘要。

### J09：历史兼容、冻结输入与当前 binary

- 旧 applicability baseline 的 149 项及旧契约保持字节不变。其绑定包含 Native 接线文件；这些文件改变前，按原路径与 SHA 保存必要历史快照，历史大 source 可原位只读复用。
- 旧 fixture 从经原 SHA 验证的冻结输入构造临时历史 root；当前 CLI 继续执行旧 source/analyzer/gate 的语义和 mutation 回归。不把 baseline 的 expected SHA 更新成当前源码，也不跳过旧测试。
- 五份适用 PASS、live-07 BLOCKED、旧六源 gate BLOCKED、旧 N/A 限域及各历史失败结果不变；T0-04、C0、legacy53 与 device56 路径仍按原规则验收。
- 旧 formal verification 绑定旧 EXE；把新 EXE 代入必须拒绝。新门另行绑定当前 production 源文件、EXE/Native/Helper、独立测试二进制及实际结果。
- verification 产物必须显式区分组件测试通过、证据完整和实机资格；持久化 UNKNOWN 时不得发布 runtime-complete/live-eligible 或下一阶段解锁。

## 5. 执行顺序与交付规则

1. 主线先完成历史必要输入冻结；测试方复核快照与原 149 manifest，不在冻结前构建覆盖旧二进制。
2. 逆向交接最终 target/ABI/owner/原生尾部/持久化契约；测试方先审查是否足以构造独立期望，不替其作推断。
3. 独立实现公开 CLI contract、Native harness 和真实 IPC/生产 shutdown 桥接；优先保留可复现的拒绝/失败证据，再验证生产实现。
4. 完成 `cargo test --all-targets --locked`、fmt/check/clippy、Release 构建、Debug/Release Native 与跨语言测试、继承 T0-04/C0/passive/device/applicability 回归，并执行新的 05-04 verifier。
5. 记录实际命令、环境、配置、exit code、测试数量、产物 SHA 与失败证据。测试未执行不能标为 PASS；fixture 通过不能计为 live source。
6. 如持久化或其他强制证据仍 UNKNOWN，交付 fail-closed 自动验证与明确 BLOCKED 状态，不创建 active 授权。只有完整前置成立且取得新的单次实机授权后，另行执行一个 source → 安全停止 → 独立 analyze/replay；失败或中断不自动重试。

本计划不授权自定义 Endpoint 退出、普通 B 物理删除、virtual device、双 Endpoint gameplay、T0-06 或后续工作包；05-04 的任何通过只解释为其严格限定的创建语义证据。
