> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Manager-this MT-R01–R10 后续完成优先级 v35

本文件把 `MT-R01`–`MT-R10` 已裁决但尚未正向闭合的 18 行排成后续执行顺序。优先级按“静态闭合概率、所需穷尽规模、对后续工作的解阻价值”排序，不表示安全严重度，也不改变 v33 的任务范围或 v34 的证据结论。

## 1. 权威基线与适用范围

- 任务范围、逐行证明接口和 promotion 条件仍只来自 [proof completion work breakdown v33](native-join-manager-this-proof-completion-work-breakdown-v33.md)，其 SHA-256 为 `D1004AE824677B37C006EE873EFC80A48566CBACE1B1EE44F81FD1A952FA3308`。
- 当前事实状态来自 [MT-R01–R10 completion checkpoint v34](native-join-manager-this-mt-r01-r10-completion-checkpoint-v34.md)及其机器产物：completion JSON SHA-256 `BF75125FD5B6D07FA5468EB29982B16CC8A0247EEF62B45F90E35BD6E5BBE0D1`，独立 audit SHA-256 `22372544AFF2807E8CAB38594A886E5D5BD8C1D3E2112DC7225513C918FACAB0`。
- 当前 v32 positive frontier SHA-256 为 `A336FF06F05A11CFF97CE937BF0E8CEE030E498B3E7C401A811021EFB665EFF3`，其独立 audit SHA-256 为 `CE5849CC3A37498D09AB00388E2BEFFB2AE5D967728C4F82E6938CCBEE8D04ED`。
- 冻结目标保持 x86 PE、ImageBase `0x00400000`、SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`。本优先级只适用于该目标发生漂移前。
- `MT-R01` 已完全正向闭合；`MT-R08` 已接受 `0x00689F63`，另两行仍为 `UNKNOWN`；其余八项没有正向行。当前账本为 copy/move `321/405`、memset `225/242`、总计 `546/647`、remaining `101`。
- 本文件只安排前十项的 18 个 remaining RVA。`MT-R11`–`MT-R29`、whole-image raw v2、formal v7、manager-this 最终结论和运行时权限不在本文件的执行范围。

“已裁决”与“已闭合”必须分开：前十项 20 行都已有逐行 verdict，但只有 2 行可进入 positive frontier。下文的“完成”专指为 remaining 行补齐静态正向证明并通过作者分离审计。

## 2. 优先级总表

| 优先级 | 任务 / RVA | 当前首缺口 | 本级退出条件 |
| --- | --- | --- | --- |
| `P0` | `MT-R06 {0x0061F2C2}` | double-buffer `this[48]/this[49]` 的跨设备动态类型、offset-collision/间接 writer fixed point，以及全局 source tuple 生命周期未闭 | 穷尽构造、allocator、cleanup、pointer/capacity writer 与 source lifetime；否则逐行保持 `UNKNOWN` |
| `P0` | `MT-R08 {0x0068E3DB}` | nested realloc object `+176/+180/+184`（以及被消费的 `+188`）writer fixed point 未闭 | 闭合 nested tuple 的 constructor、replacement、alias 与 lifetime universe |
| `P0` | `MT-R10 {0x002EE04F}` | 22 个 direct writer、28 个 LEA alias 未完成语义分类，且存在 `n4=10` 的 36-byte 对 32-byte target 反例 | 逐项闭合 writer/alias，并从上游证明 `1 <= n4 <= 9`；不能排除反例则停止为 `UNKNOWN` |
| `P1` | `MT-R02 {0x00008A62, 0x00008B2A}` | incoming object pair 未证明 `oldCount <= newCount`；保留 `new=1, old=2` 反例 | 为两行分别证明支配性 count/extent 不变量，不能用常见值或日志代替硬边界 |
| `P1` | `MT-R07 {0x00657E0A, 0x00657E94, 0x00658338, 0x006583C2, 0x00658487}` | RTTI/vtable 已绑定，但 type-erased dispatch、`a2` producer/output span、size no-wrap 与 replacement writer universe 未闭 | 从实际间接 callsite 闭合参数来源、输出 backing 与 begin/used/capacity replacement writers，五行分别裁决 |
| `P1` | `MT-R09 {0x0001B072, 0x0001B3E3}` | tuple identity 已在当前 PE 中建立，但 sublist 共享/Untie detach、already-offset alias、source count `<=0x8000` 与 arena/slot 不重叠未闭 | 闭合 shared/detached backing 生命周期、全部 alias writer、count 上界、`0x20000` arena extent 和双 slot disjointness |
| `P2` | `MT-R04 {0x0001AAB0, 0x0001AEB8}` | 177+5 个 destination producer 尚无完整 capacity 不变量；失败路径记录日志后仍继续复制 | 完成 producer 全集的 destination capacity、source count 与 owner lifetime 证明；不得把日志路径当保护分支 |
| `P3` | `MT-R03 {0x0001513A, 0x000155A4}` | `emittedCount=5001` 可超过固定 5000-element backing，现有限制只记录日志 | 只做有界的上游不可达性/硬 guard 搜索；找不到支配性硬边界即保持 `UNKNOWN`，不无限扩张 caller 图 |
| `P3` | `MT-R05 {0x0033570B}` | 真实 filtered vector 可为空，形成 `0xFFFFFFFC` erase length，且不存在已知 nonempty guard | 从真实 owner/backing 搜索现有非空不变量；不得补造 guard，不能排除空容器即保持 `UNKNOWN` |
| `P3` | `MT-R08 {0x00661577}` | `Size=0x10001` 可先到达 copy、后遇到日志/limit check，超过第二静态 `0x10000` ring | 只做有界 caller hard-bound/branch-feasibility 搜索；不能证明 copy 前 `Size <= 0x10000` 即保持 `UNKNOWN` |
| `CLOSED` | `MT-R01 {0x001CBEB5}`、`MT-R08 {0x00689F63}` | 无 | 不重复分析；仅在目标 PE、输入证据或绑定哈希漂移时重开 |

`P0` 先做，是因为缺口集中在有限 writer/lifetime fixed point，最可能以较小范围得到确定结果。`P3` 最后做，是因为当前已有可执行到 copy 点的局部反例；其目标是有限地证明反例不可达，而不是用更多日志、类型名或经验上限掩盖反例。

## 3. 执行波次

未来恢复静态逆向时，使用“主线集成 + 最多两个 REVERSE 角色 + 一个独立 TEST 角色”，按下列顺序执行；同一波两个 REVERSE 任务互不共享 candidate analyzer 或结论。

| 波次 | REVERSE 槽 1 | REVERSE 槽 2 | 进入下一波的条件 |
| --- | --- | --- | --- |
| `F1` | `MT-R06` | `MT-R10` | 两项均完成新的逐行 candidate/handoff；通过的行完成独立 TEST 后才可合入 successor |
| `F2` | `MT-R08` remaining 两行，先闭合 `0x0068E3DB`，对 `0x00661577` 只做有界搜索 | `MT-R02` | R08 仍须携带两行独立 verdict；不能因一行通过批量提升另一行 |
| `F3` | `MT-R07` | `MT-R09` | caller/alias/writer universe 与最小 countermodel 分别独立重算 |
| `F4` | `MT-R04` | `MT-R03` 有界可行性搜索 | R04 完成 177+5 producer；R03 到达预算停止点后接受正向或明确保留 `UNKNOWN` |
| `F5` | `MT-R05` 有界可行性搜索 | 空置 | 排除空容器反例才可提升；否则结束前十项 follow-up，保留该行 `UNKNOWN` |

每波只把独立审计通过的行合入 v32 派生的连续 positive frontier；零通过波次不创建空 successor。某行失败不阻止同波其他行进入 successor，也不要求重跑已经闭合的 `MT-R01` 或 `0x00689F63`。

## 4. 单行完成门与停止条件

每条 candidate 必须继续满足 v33 的证据接口：前一 frontier、冻结 PE、精确 RVA、指令边界/ABI、源与目标区间、backing extent、owner/lifetime、完整 writer/caller universe 和 countermodel 消解。只有同时满足以下条件才可提升：

```text
fullyAdjudicatedNoOverlap = true
disposition = STATIC_CONFIRMED_NO_OVERLAP
unresolved = []
independent TEST verdict = PASS
```

出现以下任一情况即停止该行的正向提升并保留 `UNKNOWN`：

1. 已枚举 universe 中仍存在未分类 writer、caller、indirect dispatch、already-offset alias 或 lifetime escape；
2. 现有反例路径无法由支配性硬 guard、真实 backing extent 或不可达 CFG 证明排除；
3. 结论依赖日志、常见运行值、外部类型名、REPENTOGON 布局迁移或未经当前 PE 唯一重定位的签名；
4. 目标 PE、v33、前一 frontier 或 candidate/audit 绑定发生漂移。

达到停止条件不是整项失败：写出最小首缺口并继续下一任务。不得为追求“十项全完成”而扩大为无界全程序探索。

## 5. 压缩测试策略

本 follow-up 不重复已经通过的全量 T0、Cargo、旧 self-test 或与新证据无关的回归。每次新证据只保留能改变裁决的最小测试面：

- REVERSE 侧执行一次 create-new 和一次 deterministic verify-existing；没有新输入或新结论时不重复发布同内容 candidate。
- TEST 侧不 import 或修改 candidate analyzer，从冻结 PE 与公开输入独立重算 exact RVA、关键 writer/caller universe、区间关系和逐行 verdict；只执行一次 create-new、一次 verify-existing。
- 拒绝测试覆盖不同决策分支即可：错误 PE/前沿哈希、RVA 重复或遗漏、漏 writer/caller/alias、未消解 countermodel、跨 helper 或未审计行。共享同一失败机制的重复 mutation 不扩张数量。
- 只有工具、schema 或生产代码发生变化，才补相应单元/集成回归；纯证据或 Markdown 更新不触发构建测试。
- 每波零行通过时只保留 candidate、handoff 和必要 audit，不生成空 frontier successor。

压缩只删除重复验证，不降低作者分离、逐行 verdict、wrong-hash fail-before-write 或 fail-closed 要求。

## 6. 静态与动态边界

当前 follow-up 继续以冻结 PE/IDA 只读静态分析为主，不需要先做动态分析。只有某行完成上述静态穷尽后仍停在“路径是否真实可达、动态类型实际集合或输入上界来源”时，才可在用户另行授权下建立动态补证任务。

动态观察最多产生 `RUNTIME_OBSERVED` reachability、实际类型或输入分布，不能替代 `STATIC_CONFIRMED_NO_OVERLAP`，也不能直接进入 positive frontier。若以后获授权，优先观测 `MT-R05`、`MT-R08 0x00661577`、`MT-R10`，其次是 `MT-R03/MT-R09`；`MT-R06/MT-R07` 只用于发现实际动态类型和 call stack。游戏启动、DLL 注入、调试器和 REPENTOGON scanner 均不由本文件授权。

## 7. 里程碑与对后续工作的影响

- 每闭合一行，copy/move accepted 增加 1、总 accepted 增加 1、remaining 减少 1；memset 计数不变。
- 如果 18 行全部闭合，前十项的账本上限为 copy/move `339/405`、memset `225/242`、总计 `564/647`、remaining `83`。这是数学上限，不是预设验收结果。
- 即使达到该上限，`MT-R11`–`MT-R29` 的 83 行、whole-image raw v2、formal v7、manager context 与其他 T0-05 gate 仍需完成；不得据此宣称 T0-05、manager-this 或运行时开发已解锁。
- 未闭合行不会阻止继续做相互独立的静态任务或非侵入式开发准备，但会阻止把 manager-this 同代身份作为已证事实，并继续阻止依赖该前提的 active/live、05-05、T0-05 总门和 T0-06 发布链。
