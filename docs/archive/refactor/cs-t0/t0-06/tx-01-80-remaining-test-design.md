# tx-01-80：全量 replay 之外的剩余测试准备

日期：2026-10-19。用户明确要求完成全量 replay 之外的剩余测试；允许仅 E2E 启用的原生观察/采集器故障开关与可移除场景 Mod。全量 replay 排除，已通过用例不重跑。当前尚未开始本轮整体验收。

后续执行记录：本轮唯一统一入口已在第一个进程识别步骤 **FAIL**，没有 controller/注入；根因因旧查询缺原始 stdout 而 UNKNOWN。见 [tx-01-80 结果](tx-01-80-e2e-results.md)。本文“未开始”等为准备时历史状态，不能当作当前运行结果。

本轮先为全部剩余项补齐触发和取证方法，再以一个统一入口执行一次。准备期间只读代码、既有文件、静态 IDA 与 API 资料，不查询或操作游戏，不运行试跑/单元测试。真实 Original Online/Isaac Sync 会话及配合玩家条件尚待用户答复，不将静态兼容检查代替真实联机证据。

用户随后明确“不考虑测试这一项”：W01.isolation 与 W08.replay 均为本轮 USER_EXCLUDED，保留 NOT_COVERED，不计为 PASS。本轮运行目标因此为其余 10 子项。

## 测试专用实现边界

- Native 仅 `/p:WorldE2E=true` 编译时启用探针；默认编译消除调用。E2E Native 要求与本次 world 名称同 nonce 的 `.E2E` 独立 mapping，否则注入初始化失败，不静默退化。现有 coop v3/v4、world v2、ISCS/Lua/loop 布局均不扩展。
- 19 槽继续指向原入口 wrapper，原 trampoline 仍由现有事务发布；E2E wrapper 经过保留全部寄存器/FLAGS/FX 的自有 bridge 后跳转到已发布 trampoline。bridge 记录实际原函数入口参数、寄存器、栈同余与 FX，返回阶段记录实际返回值与 callee-saved 状态，原函数只执行一次。FX 保留的 reserved 字节不作架构状态相等判据。
- 65536 个 128 字节 probe 记录独立消费，溢出不作为通过；正式 collector 样本不因探针删字段。原始 FX 分块、实际 GPR/FLAGS、QPC 耗时、每次复制的读取总量及首 16 条地址/位宽证据单独落盘，不计算摘要。
- 故障只作用于观察：自有预算触达、复制描述变化、引用状态歧义、事件容量失败及 root 稳定性失败。字段读失败使用 collector 在 Prepare 预分配的 PAGE_NOACCESS 页，经过真实 Read 的 SEH 异常分支，再进入 READ_FAILED；不改游戏指针、不冒称实际游戏对象内存损坏。错误线程以自有 worker 调用真实 MainThread 检查，检查在任何对象读取前拒绝；仍不在 worker 读取游戏对象，main/worker ID 和拒绝结果保存在独立 probe header。
- 终止观察的故障各使用用户新启动的游戏进程；预算/关系缺口可在同一健康采集中执行。每次故障的 scheduler 前后 tick、coop error、observer error、线程与恢复字节均保存。

## 剩余范围及必要证据

| 子项 | 待准备的方法 |
| --- | --- |
| W01.isolation | 真实 Original Online/Isaac Sync 场景；明确 C/S 未进入它们的会话/IPC/API 可变状态 |
| W01.native_results | 19 入口正常调用的实际参数/栈清理/返回寄存器/FX 状态和原函数 exactly-once 证据，结合已确认指令与 ABI |
| W02.phase_order | outer/owner/Room/tail/Authority 提交的同 token 顺序跟踪和原始样本关联 |
| W02.blocked_windows | odd、暂停、loading、生命周期窗口的场景时间与实际 gap/tick 明确对应 |
| W03.retained_unknown | 保留 Player 关系、歧义与固定 profile 未知类别实际场景；未知不可伪造成八类 |
| W04.identity_history | 嵌套生成、短命对象、池内同址再用、重复发布的创建/use/成员历史 |
| W05.transitions | NPC/Pickup Morph、Player Restore/Replace，以及房间/run 退休的实际调用与身份序列 |
| W06.revocation | exact-pair/generation 失效时撤销读取依据，重新有效的正常 PlayerUpdate 后恢复，同时保留 FIRST_OBSERVED |
| W07.readonly_calls | 采样窗口线程/耗时/只读操作路径、RNG/原生调用约束的实际取证及静态调用链 |
| W11.scenario_basis | 保存真实 seed、实际统一输入及 game-frame/房间/创建事件依据，再明确配对 Authority tick；不按最近帧、地址或位置强配 |
| W12.fault_isolation | 采集器自有预算/关系/读失败/来源变化/事件丢失/错误线程/背压等已定义故障及 scheduler 继续推进证据 |

## 实现前登记的失败方式

- 探针意外默认启用、混用内部 mapping 版本/尺寸/nonce/PID、命令重放或跨 generation：拒绝配置，正式 ISCS/Lua/loop 布局不变。
- ABI 探针本身污染寄存器、EFLAGS、FX、栈对齐/清理或原函数次数：必须保留原状态并独立记录检查点；不重复执行有副作用的原函数来比较。
- 跟踪溢出、配对缺失、线程不符或序号丢失：明确失败/不可确认，不能以缺记录当通过。
- Lua API 名称存在但不对应目标原生链、场景 Mod 误触发、初始化回调重入/嵌套无限递归、对象已退休：静态核对后采用有界状态机，执行记录真实函数与对象；不按脚本完成宣称原生分支覆盖。
- 同址复用不可达或身份关联不足：保存真实取得/use 事件，不伪造旧地址、对象或出生历史。
- 故障注入越出采集器自有状态、破坏原生对象或 RNG：禁止这种触发。读失败注入是显式观测层失败，报告不得冒称实际 OS 访问异常。
- 未知类别/关系用已知 profile 冒充、仅离线副本覆盖却冒称 native 分支：分别保存来源和范围，判据必须匹配真实路径。
- collector 进入 coop::Fail、scheduler 停顿、读写器未收敛、恢复冲突或计数不完整：本项失败并停止进一步实机修改，不强制卸载、杀游戏或重注入。
- 两个进程的 seed 声明不等于实际 seed，相同数字 tick/epoch 不等于同一时刻；输入/创建映射缺证则不宣称严格对齐。
- 网络会话或特殊场景条件缺失：先向用户说明具体缺项，不能用重命名、静态结论或豁免自动产生运行 PASS。

完整可检查的实现和场景步骤准备完成后，再按用户授权进入唯一一次统一验收。历史 FAIL/INTERRUPTED 和现有 21 PASS 来源全部保留，不计算或校验哈希。

## 具体取证与运行安排

统一入口仍为 `accept-world-record-e2e.ps1`，增加 `-InstrumentedRemaining -SkipFullReplay -ContinueFrom <tx-01-79> -GameLog <实际 log.txt>`。控制器使用 `--world-e2e-probe v1` 创建独立 mapping 和 buffered trace writer；生产 Native 默认编译完全不启用探针，测试 Native 缺 probe mapping 则拒绝初始化。所有指令记录保持 70 槽，额外 bridge 是自有代码，不增加目标补丁。

一次执行包含 primary 场景、两次新进程的严格对齐场景、五个各用新进程的终止观察故障，共最多八个手动准备的游戏进程。预算和引用故障在 primary 中一次执行；背压、正常停止和失联历史原始证据沿用 tx-01-79，保留其进程/二进制边界。已通过用例不重复声称为新二进制验收。

具体 Mod 操作与撤除见 [fixture README](../../../../../test-mod/isaac-cs-world-e2e/README.md)。API 来自现行 IsaacDocs；静态分支证据保存于 `artifacts/cs-t0/tx-01-80-preparation/`：Esau Jr. 703 对应 7ACFD6→9BEE80，Glowing Hour Glass 422 对应 706349→6F8140→9BCAA0→7AE4B0；炸弹工厂 case 4 的 6F0239 `&0x3F` 为 64 槽。运行时是否走到这些分支仍必须核对实际 Hook，不以脚本成功代替。

读取路径的静态闭包为 `Capture → CopySource/Add/CopyObject/Fields/ResolveReferences → Read/U32/HasBase`，仅对明确地址 memcpy 至自有对象/字段/来源数组，关联/预算/有限值处理也只写自有数据；没有 Lua、虚 getter、Native RNG、query/诊断生产者调用。probe 保存整个复制窗口的线程、QPC 与逐次字面读总量、前 16 条地址/长度、窗口内 19 个入口是否被调用、所有已确认对象/格子 16-byte RNG 的前后原字，以及全局 MT 624 字、index、rand 分流 flag 的前后原字。实际路径与静态闭包共同解释只读约束，不仅凭首尾相同作全称推断；不外推未观察 worker、别的 RNG 或任意 Mod。

全局 MT 读取依据复用 tx-01-71 已存原始指令：array VA C7AC70/9C0；6EF356 的 `8B 1D 04 4F C3 00` 读取 index VA C34F04；6EEF8A 读取 flag VA C7AC68。只复制这些原字，不调用 6EF1F0 或任何摘要/history 生产函数，不计算摘要。rand flag 非零、RNG 改变/不可读、probe overflow、ABI/栈/阶段配对错误都保留失败并停止进一步实机修改。

严格双进程时刻使用 fixture 从 run 初始保存的实际 seed/原始中性输入序列、实际 canonical Submit/Resolve、同一绝对 Game frame 的唯一普通样本和明确创建事件。B 的实际出生 counter 单独保存：不同 Join 时机属于已知场景差异，不补造相同初始世界，也不由同 seed/input 推导完整确定性。由用户确认具体 tick/出生映射及这个范围后才比较。缺帧或输入不同不能用最近样本、地址或位置补齐；更早未采集世界和未映射对象仍显式未知，不声称全局首次差异。

## 纯编译记录（不是运行验收）

- Rust 控制器直接 `cargo build --release --locked --offline --manifest-path cs-t0/Cargo.toml --bin IsaacCsT0 --target-dir out/cs-t0/tx-01-80-build/rust` 成功；没有执行生成的 exe。
- Native 第一次编译因 Windows `min` 宏冲突报 C2589，使用 `(std::min)` 修正。第二次编译通过，链接报 LNK1101 MSPDB140.DLL 版本不匹配；保留 `out/cs-t0/tx-01-80-build/native-build-02.log`。
- 指定 VS2022/v143 14.44.35207、Hostx64/x86 与 PATH 后第三次纯编译/链接成功，记录 `native-build-03.log`；后续纯增量编译记录 `native-build-04.log`。这不构成任何 E2E PASS。
- 最终完整源码在新的 `native-final-int/` 目录重新直接编译/链接成功，输出 `out/cs-t0/tx-01-80-build/native-final/IsaacCsT0.Native.dll`，完整日志 `native-build-final.log`。统一验收使用这个最终测试专用 DLL 与上述 Rust controller，Helper 沿用 tx-01-79 冻结版本。全程未执行生成的程序。
- 准备阶段未运行 Python/Mod/测试项目、游戏查询、注入、probe 自测或 replay。唯一整体验收仍未开始。

已将已获授权的三份场景文件新增到 `F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth/mods/isaac-cs-world-e2e/`；原同名目录不存在，没有覆盖其它 Mod。后续 README 同步仅修改本次新建目录的说明。启用、全新 seeded run 和场景命令仍由用户在人工检查点操作。

当前状态：`IMPLEMENTED_AND_COMPILED / E2E_NOT_STARTED`，等待用户首个准备检查点。未把代码阅读/编译或安装 Mod 记为运行 PASS；没有启动控制器、注入或操作游戏。
