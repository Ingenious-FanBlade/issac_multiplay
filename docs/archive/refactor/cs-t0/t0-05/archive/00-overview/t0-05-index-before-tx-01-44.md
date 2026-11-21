> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Isaac C/S T0-05 逆向记录索引

> 2026-10-12 执行约定更新：先读[现行项目规范](../../../../project-conventions.md)。永不设定 gate，不校验任何哈希；每次推进最小为 `tx-0x-0x`，整个目标完成后仅一次整体测试验收，探讨过程不得测试；逆向与测试可由同一智能体完成。下面旧 gate/解锁字段、哈希、预封、作者分离和多组验收流程只作历史资料，不作为当前要求；事实与未决功能如实保留。

本目录按研究问题整理 T0-05 的计划、逆向交接、实现记录与独立验收。这里的 `README.md` 是去冗余后的阅读入口；主题目录内的原始记录保留阶段性结论、失败证据、哈希和作者分离审计，不应被当作彼此独立的最新结论重复阅读。

## 当前总览

- T0-05 总体仍为 `IN_PROGRESS`。
- `CS-T0-05-00`、`01`、`02` 已 `PASS`；`03` 为限域适用性 `PASS_SCOPED_APPLICABILITY`。
- `CS-T0-05-04` 的规划轨已接受[原生 Join 创建/发布用户假设](../../02-active-join/native-join-active-user-planning-assumption-v1.md)；严格证据轨仍为 `BLOCKED_BY_EVIDENCE`。MT-R02 exact2 保持 `UNKNOWN`，MT-R03/R05 的既有 policy waiver 保留；manager-this、Init/持久化、reader/ABA 与恢复仍未闭合。
- 当前 manager-this v33 机器前沿为 copy/move `323/405`、memset `225/242`、remaining `99`。新增 `0x0061F2C2` 与 `0x0068E3DB` 已通过作者分离审计；MT-R04 两行按用户要求跳过 follow-up TEST 并原样留在 remaining。
- `CS-T0-05-05` 当前入口为[只读 Virtual Device 决策](../07-virtual-device/README.md)：v4 离线 lifecycle 原型已在独立 `Release|Win32` harness 中完成 16 cases / 332 assertions / 13 rollback，最终 create-new、verify-existing 与一个参数化负向批次均 PASS；三项离线轴为 `OFFLINE_IMPLEMENTATION_CONFIRMED`，但 REVERSE v2 的原生对象 owner、创建/析构和引用计数证据及三项正式状态仍为 `UNKNOWN`，`promotionCount=0`、`formalDecision=null`、`BLOCKED_BY_EVIDENCE`。
- 通用 strong/weak 机械子链虽已局部闭合，但 `{object,null-control}` 发布、callback retained ref、remove/reader 无 drain、native device worker 调用、last-release thread、失败 rollback 和 16-bit 计数边界仍构成反例或缺口；设计契约为 `DEFINED_NOT_IMPLEMENTED`，不能代替事实证明。因此 `formalDecision=null`、`implementationEligibility=false`、工作包保持 `BLOCKED_BY_EVIDENCE`。
- 该决策不改 v41 的 `0 positive / 2 UNKNOWN`、人工队列 `96` 或 strict frontier `99`。本轮没有启动游戏或动态调试；active/live、runtime mutation、05-06、T0-05 总门和 T0-06 解锁保持关闭。

## 主题导航

| 目录 | 内容 | 权威阅读入口 |
| --- | --- | --- |
| `00-overview/` | 研究计划、执行状态与阶段出口 | [总览与计划](README.md) |
| `01-passive-lifecycle/` | passive 生命周期、device rebind、原生退出适用性 | [Passive 生命周期](../01-passive-lifecycle/README.md) |
| `02-active-join/` | ordinary native join 主动路径、preflight、测试边界 | [Active Join](../02-active-join/README.md) |
| `03-admission/` | admission v1-v6 的累计准入规格与独立验收 | [准入规格](../03-admission/README.md) |
| `04-init-closure/` | Init/fresh-B/publication/manager receiver 累计闭包与 cache/P0 支线 | [Init 闭包](../04-init-closure/README.md) |
| `05-persistence/` | readonly、dirty、save/Cloud/Steam 与完整停止窗口 | [持久化安全](../05-persistence/README.md) |
| `06-manager-identity/` | manager-this 同代身份、whole-image 与 647 行 bulk 证明 | [Manager identity](../06-manager-identity/README.md) |
| `07-virtual-device/` | 物理设备自由 Endpoint B 的 virtual-device 必要性与证据阻断 | [Virtual Device 决策](../07-virtual-device/README.md) |

## 去冗余规则

1. 当前结论只从本索引、各主题 `README.md`、[研究计划](isaac-cs-t0-05-research-plan.md)和[执行状态](isaac-cs-t0-05-status.md)进入。
2. 同一系列的高版本是累计入口；低版本仅用于追溯当时的证据边界、被推翻假设和 create-new 历史，不与高版本并列汇总。
3. producer handoff 与 independent audit 分别保留，二者不能合并成同一作者的自证结论。
4. `PASS`、`STATIC_CONFIRMED`、`UNKNOWN`、`BLOCKED_BY_EVIDENCE` 等标签保持原义；负向审计覆盖不会被改写为正向闭合。
5. 目录整理本身不新增逆向事实；当前结论以各主题最高版本检查点和机器产物为准。本轮 follow-up 仅使用冻结 PE/IDA 静态证据，不调用 x32dbg/游戏，不改变权限、矩阵或解锁状态。

## 旧路径兼容

根目录中除本索引外仍可见少量同名 Markdown。它们是冻结 JSON、脚本或历史审计已经按 `docs/archive/refactor/cs-t0/t0-05/<filename>.md` 绑定的兼容副本，不是第二套当前文档：

- 阅读、维护和后续新增记录一律进入上表主题目录；
- 兼容副本保持旧相对链接与历史字节口径，不在根目录继续追加内容；
- 不为目录整理重写或重签冻结机器产物；
- 尚未被机器路径绑定的记录只保留主题目录版本，不创建重复副本。

明确被后继版本吸收的阶段稿见[过时文档迁移表](obsolete-document-map.md)。12 份旧正文已清理，旧文件名只保留短跳转；另有 41 份主题重复全文已压缩为指向根目录冻结原文的导航页。两类文件均不再重复承担当前结论。

## 推荐阅读顺序

先读[总体计划与当前状态](README.md)，再按 `passive lifecycle -> active join -> admission -> Init/persistence -> manager identity` 阅读。若只需恢复当前 manager-this 工作，从[follow-up checkpoint v36](../06-manager-identity/00-overview-and-gates/native-join-manager-this-mt-r01-r10-followup-checkpoint-v36.md)进入；v34/v35 保留前一轮事实和优先级历史，任务范围与证明门仍以[任务分解 v33](../06-manager-identity/00-overview-and-gates/native-join-manager-this-proof-completion-work-breakdown-v33.md)为准。
