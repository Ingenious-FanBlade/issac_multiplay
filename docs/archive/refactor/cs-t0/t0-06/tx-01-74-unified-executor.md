# tx-01-74：T0-06 统一 E2E 执行器

tx-01-79 更新（2026-10-19）：`-LiveRemaining` 为本轮新场景/外部只读取证模式，固定来源为 tx-01-76 已完成 INCOMPLETE 目录，使用元数据控制器及不变 Native/Helper；不重跑已完成离线用例。覆盖采集后拾取跟班、人工正常场景、一次自有 reader 暂停及最后一个新进程的自有控制器失联。一次整体运行 21 PASS / 0 FAIL / 12 NOT_COVERED，两个先前注入前 FAIL 保留，见[设计与失败方式](tx-01-77-live-remaining.md)、[每槽 owner 修正](tx-01-79-iat-observer-fix.md)、[结果](tx-01-79-e2e-results.md)。

tx-01-76 更新（2026-10-19）：增加显式 `-ContinueFrom`、`-SkipFullReplay`、`-SupplementalCapture`；中断来源只读，沿用 PASS 单独标识，不重做默认/万次采集。一次续跑新增 9 PASS，累计 19 PASS / 0 FAIL / 14 NOT_COVERED，见[续跑使用边界](tx-01-76-remaining-e2e.md)与[实际结果](tx-01-76-e2e-results.md)。

tx-01-75 运行更新（2026-10-18）：已执行同一统一入口的实机和 primary 扫描，新增 `W07.consumer_pipeline` 后共 33 子项；用户在第一次全量 replay 期间要求结束，最终 INTERRUPTED，10 PASS / 0 FAIL / 23 NOT_COVERED。普通中断未使原执行器收尾，终止已核对身份的本轮自有进程后，仅从已保存证据整理报告；没有重新运行入口或新测试。见[实际结果与收尾方式](tx-01-75-e2e-results.md)。

后续构建提示（2026-10-18）：tx-01-75 的 world 内部 mapping 为 v2，未来验收需显式传入同一新构建的 Controller/Native，不能沿用下面示例的旧 Native 路径与新 Controller 混搭。新的三项路径及 `world/consumer.json` 证据要求见[即时消费修改](tx-01-75-immediate-consumer.md)。本次没有重新执行此入口。

后续运行更新（2026-10-18）：本执行器现已完成一次统一 E2E，整体 **FAIL**，17 子项通过、1 失败、14 未覆盖，未自动重跑；见[本轮结果](tx-01-74-e2e-results.md)。下文“未执行/NOT_RUN”说明最初实现交付时的状态，实际运行结论以本轮报告为准。

日期：2026-10-18。状态：**执行器已实现，未执行；W01–W13 仍全部 NOT_RUN**。本次只编写源码、场景计划与使用说明，进行静态源码审阅，没有启动执行器、生产 CLI、构建、注入或游戏操作，没有新增单元测试，也没有计算或校验哈希。执行要求见[现行规范](../../project-conventions.md)；验收内容仍以 [tx-01-73 清单](tx-01-73-e2e-checklist.md)为准。

用户在本会话明确选择人工检查点：由用户启动全新游戏进程、准备场景并确认，执行器再运行该段采集；无法触发的场景记 NOT_COVERED。本实现没有补造 Morph、Restore、复用等场景的自动化操作定义。

## 1. 统一入口与输入

- [PowerShell 入口](../../../../../cs-t0/accept-world-record-e2e.ps1)：仅转发参数，要求 PowerShell 7.2+；不调用任何旧构建/测试脚本。
- [Python 主入口](../../../../../cs-t0/run-world-record-e2e.py)：Python 3.10+ 标准库，无新依赖。
- [执行编排](../../../../../cs-t0/world_e2e/runner.py)、[产物分析](../../../../../cs-t0/world_e2e/artifacts.py)、[离线生产 CLI E2E](../../../../../cs-t0/world_e2e/offline.py)。
- [默认场景计划](../../../../../cs-t0/testdata/tx-01-74-world-record-plan.json)与[中性 B 输入](../../../../../cs-t0/testdata/tx-01-74-world-record-input.json)。

将来正式验收时，在交互式 PowerShell 中只启动下面这一个入口；示例是语法，本次没有执行：

```powershell
pwsh -NoProfile -File .\cs-t0\accept-world-record-e2e.ps1 `
  -GameExe 'D:\actual-game\isaac-ng.exe' `
  -Controller '.\cs-t0\target\release\IsaacCsT0.exe' `
  -NativeDll '.\out\cs-t0\tx-01-73-build\native\IsaacCsT0.Native.dll' `
  -Helper '.\out\cs-t0\tx-01-73-build\helper\IsaacCsT0.Helper.exe' `
  -ArtifactDirectory '.\artifacts\cs-t0\<new-invocation>-t0-06-e2e' `
  -Authorization '<本轮实际授权的输入、Join/B退休、换房/暂停/菜单及受控故障范围>' `
  -BCharacter native
```

`-Plan` 可指定自有计划；相对脚本路径以该计划所在目录解析。计划必须按 default、primary、secondary、controller-loss 排列四个不同角色，字段只允许 version/captures 和每项 role/script/instructions。可以在检查点跳过某段并说明原因，不删掉验收义务。修改后的计划及实际输入会保存。

使用当前已编译的三件套，或未来明确取得的同一实现版本产物。执行器将它们复制至本轮 `binaries/`，并保存来源路径、大小与修改时间；这些是来源描述，不是身份哈希或已验证的版本一致性。源码同时保存到 `sources/`。执行器本身不构建，不运行单元测试，不调用 `build.ps1` 或旧 verify/accept 链。

## 2. 一次执行的顺序与人工检查点

1. **default**：用户准备全新进程，输入 `start default`。不传 world-record，保存 v3/51 槽对照及恢复材料。
2. **primary**：用户完全重启，准备主采集场景，输入 `start primary`。覆盖可达的类别/生命周期/绑定失效场景，再取得至少 10,000 次连续完整普通采集。
3. **secondary**：用户完全重启为第二个不同进程，准备声明的同 seed/input 场景，采集并正常停止。当前本机仍必须恰好有一个用户启动的目标。
4. **controller-loss**：最后一个实机阶段，可用 `skip <原因>` 跳过。只有实际授权且准备好恢复证据采集手段时才执行。
5. 对 primary 做原始文件分析、两次 production replay、单字段/浮点变体、冲突/不完整/decoder 变体。
6. 根据双方原始记录和 `alignment-candidates.json`，人工提供明确对齐文件及依据；实际执行 production world-compare。对齐文件格式沿用 tx-01-73，不自动配 tick 或创建事件。
7. 在同一执行器进程内补充人工证据审阅，输出 W01–W13 总报告与逐文件目录。

每个实机检查点接受 `start <角色>` 或 `skip <原因>`。执行器不会启动、重启、关闭游戏，也不操作按键/控制台/存档。检查点确认后才查询目标进程信息并调用生产 controller。保存 PID、创建时间、exe 路径和模块名，拒绝复用本轮旧进程或已驻留 Native 的实例；采集后再次记录同一进程是否仍存在。进程/模块查询属于这一次正式验收，不作为本次实现阶段的试跑。

采集期间可在执行器终端输入：

| 命令 | 行为 |
| --- | --- |
| `mark <场景标记>` | 记录操作者时间，并转发给 controller，后者保留 Authority tick |
| `note <事实/证据位置>` | 写入本轮 timeline；不会把备注直接判为 PASS |
| `progress` | 从已经写完的 world JSONL 行读取当前/最长完整区间；仅供进度参考，最终判定以停机后的完整扫描为准 |
| `status` | 转发既有 controller 状态查询，实际输出保留在该段 controller.log |
| `script`、`queries`、`conflict-b`、`retire-b` | 转发现有操作；只在本轮实际授权范围内使用 |
| `stop` | 请求一次正常停止，等待 controller 收尾；不重复发送或强制卸载 |
| `lose-controller` | 仅最后的 controller-loss 阶段支持；至少已有一份普通采集后，终止执行器自己启动的 controller 子进程 |

默认 B 脚本是空 steps，因此不隐含移动、射击、物品消费或特殊生命周期操作。若要固定 B 输入，执行前在自有计划中指定经授权的真实脚本；沿用现有至多 256 个严格递增 tick entries 的格式。A 的实际输入、场景和 seed 声明需要保存证据，不能仅由“计划相同”推导实际执行相同。

现有 controller 的 1,800 秒总时限不变。执行器没有修改 Native 预算、scheduler、消费策略或 profile，也不保证该时限内必然取得 10,000 次完整样本。`progress` 不自动停止采集；结束前应看清最长完整区间，最终完整性还要核对事件、计数和源覆盖。

W07 的计数解释已由用户在 2026-10-18 明确确认：同一稳定 run/room 内，明确的 odd-only gap 保留原记录、不计次数，也不打断普通完整采集的连续区间。初稿在询问期间采用“任何 gap 都打断”的临时严格规则，现已按用户答复更新。进度与最终扫描共用 `OrdinaryWindow`，报告记录 `ORDINARY_FULL_PROFILE_ALLOW_EXPLICIT_ODD_GAPS`、区间首尾 attempt、普通完整样本数及区间内未计数的 odd-only 数量。

odd-only 例外要求 gap 原因为 `ODD_PLAYER_ONLY`、Authority kind=2 且有本次 tick，capture_seq 为空、complete=false、没有对象或非零复制预算、15 个源均为空且无错误；run/room、mutation、线程与前一条有效区间记录相同，attempt 连续，token/tick 递增。例外只维持已有普通区间，不会生成完整样本。其它 gap、字段/来源不完整、缺号（包括背压）、生命周期或范围变化都会打断区间；已记录的区间外背压不否定之后取得的有效区间，事件丢失、原生错误和持久化计数不一致仍不能通过。

## 3. 自动检查的实际范围

| 清单 | 执行器直接检查的事实 | 仍需人工运行证据 |
| --- | --- | --- |
| W01 | v3/51 与 v4/70、独立 world 目录、恢复目标 | 原版/Isaac Sync/API 隔离、ABI/返回值/原调用次数 |
| W02 | 保存的 seq/phase/kind/thread、odd 不能算完整 capture | 实际 owner/Room/tail 时序、tick 提交和暂停/loading/生命周期场景因果 |
| W03 | 固定字段集合、八类 VALUE 覆盖、源状态、Grid/door 合并来源 | 保留 Player、未知类、关系歧义等完整场景 |
| W04–W05 | Factory/Init/Publish/Spawn 与 birth 事件是否实际出现 | 逐对象创建链、短命/嵌套/复用、Morph/Restore/Replace 和退休解释 |
| W06 | FIRST_OBSERVED A 的证据 3、缺失出生及对应 witness 事件 | exact-pair/generation 失效、撤销/重取及异常路径 |
| W07 | 连续 10,000 次完整普通样本；明确 odd-only gap 只保留不计数，其它 gap/NOT_READY、重复样本或缺号打断区间 | 只读/RNG/实际原调用次数与时延证明；静态源码和首尾值不能代替 |
| W08 | 同一真实输入两次 production replay，逐条值/原位位串和重编码字节一致；自有副本 u64 >2^53 | 不以此声称 Native 连续读取已验证 |
| W09 | 自有副本一个 position.x 的精确路径、±0、NaN/±Inf | 不改游戏内存，不伪称 native mutation 场景 |
| W10 | 完整冲突组与冲突目标引用；缺映射/源/对象/tick、重复 tick、错误阶段、引用状态、READ_FAILED/NOT_READY、非法载荷、早期 gap | 离线状态不能代替 native 读失败/复用故障覆盖 |
| W11 | 两个不同实际进程，显式 alignment 经 production CLI 处理，行数和结果合法 | 同 seed/input 的实际事实、run/room/tick/出生映射依据 |
| W12 | 不提供破坏游戏指针或改写世界的故障注入器 | 实际受控 consumer/资源故障与 scheduler 继续推进证据；当前未实现专门的 world-consumer 暂停接口 |
| W13 | 正常停止 70 个唯一目标原始值/字节、producer closed、callback/event 收敛、19 入口计数配平 | 控制器丢失后的真实 publication 停止和逐槽恢复、未知 EH/worker 路线 |

自动检查都只给对应子项结论。子项失败保留 FAIL，人工文件不能覆盖自动结果。缺场景/缺映射/缺样本保持 NOT_COVERED；primary 已执行但不足 10,000 次完整采集时，W07 计数子项为 FAIL，不把它包装成达标。

default 生产 controller 还会分析历史上更大的输入/生命周期套件，因此空脚本对照可能正常恢复但返回 `NOT_OBSERVED` 和非零退出码。执行器保存该原始结果，仅按 W01 的 v3/51 槽子项作判断；不宣称旧套件通过。Native error、transport failure、旧结果明确 FAIL 或恢复异常仍不能通过该子项。

## 4. 离线副本、人工审阅和结果含义

完整 replay 使用 primary 全部原始记录的冻结文件副本并逐行处理；每次 replay 后都与原始文件作无哈希的逐字节对照，并记录逐记录的字段比较数量及首个值差异位置，不同时缓存 10,000 个世界。离线变体使用其中一份真实普通 capture：保留全部对象/字段和未知区，只在自有副本中修改，并保存 `*-changes.json`。选择唯一可映射且 position.x 有限可读的对象；没有这样的对象时相应子项未覆盖，不能合成一个“真实实体”补齐。

同一冻结副本的左右 local_id/use/birth/ordinal 映射是明确记录的“相同副本”依据，仅用于离线编辑的验收。双进程比较仍必须人工提供实际对齐，完全不复用这套自映射推断。浮点测试只断言目标字段语义；其它未知字段保留，不因该字段相等宣称整个世界 equal。

最后的人工检查点会显示 `manual-review-template.json`。操作者可在同一轮中编辑它，然后输入 `review`；也可提供另一个文件路径，或 `skip <原因>`。人工子项允许 PASS/FAIL/NOT_COVERED，每条包含：

```json
{
  "id": "W02.phase_order",
  "status": "PASS",
  "observer": "<实际审阅者>",
  "reason": "<具体时序、场景标记和记录位置如何支持结论>",
  "evidence": ["captures/primary/capture/loop-events.jsonl", "<其它实际证据文件>"]
}
```

PASS 必须有审阅者、理由和实际文件。artifact 内的引用保持原路径，外部文件复制到 `manual-evidence/` 并保存输入审阅文件来源；证据由署名操作者解释，执行器不冒称自动验证了视频/调试器材料的含义。只能填列出的人工子项，不能覆盖自动检查。控制器丢失 PASS 还必须确实发生本轮 `lose-controller`；双进程依据 PASS 必须已有本轮显式对齐步骤。未做主采集不能凭外部历史文件填当前运行 PASS。

子项汇总为 W01–W13：任一 FAIL 则该组 FAIL；全子项 PASS 才为该组 PASS；否则 NOT_COVERED。最终退出码：

- `0`：全部义务 PASS。
- `1`：断言/执行失败，或结果完整性不确定。
- `2`：执行器结束，但仍有 NOT_COVERED。
- `130`：操作者中断。

`world-compare` 的 DIFFERENT/INCOMPARABLE 可以是合法比较结果，不能只凭其退出码为零宣称世界相等；非法输入的非零退出只有在保存了预期 failure.json 时才算 decoder 用例成功。

## 5. 中断、控制器丢失与产物

恢复/进程身份不确定后，不再启动后续实机段；已有材料仍可用于离线检查。出现失败不自动重跑当前用例，也不继续注入同一不确定进程。Ctrl+C 时至多补发一次尚未发送的正常 stop，等待收尾；不杀游戏、不调用额外 helper、不 FreeLibrary、不强制卸载。超时保存 controller PID 和未确认事实，不把等待结束当作安全恢复成功。

`lose-controller` 是被测故障动作，作用于自己的 controller 子进程。它不会伪造或补写该 controller 无法产出的 final result、hook 计数或原始恢复字节。若当前监视失联机制没有完成真实恢复，或没有独立恢复记录，就保留 NOT_COVERED/人工 FAIL；不会自动调用 helper 将另一路恢复冒充被测失联处理。

本轮目录使用 create-new，保留：

- manifest、原始/解析后计划、实际授权、输入、源码与二进制来源及副本；
- timeline、操作者标记、每个子命令 argv/退出码/stdout/stderr；
- 各进程前后身份、原始 loop/co-op/world capture/gap/lifecycle、恢复记录；
- 完整 replay、逐记录原值/字节对照、自有变体与逐项修改说明；
- 显式 alignment、依据、比较/冲突/failure 报告；
- 人工模板、接受的署名审阅、外部证据副本；
- `checks.jsonl`、`cases.jsonl`、`result.json` 和无哈希的 `file-inventory.jsonl`。
- 报告及目录写完后的 `finalization.json`；它不是 PASS，只表示报告收尾完成。缺少它或存在 `finalization-failure.json` 时，报告仍可能不完整，执行器不会正常退出为 0。

本说明初次交付时仅完成实现和静态审阅；之后已按上述运行更新执行一次。实际 FAIL 与全部未覆盖项见结果报告，不将本说明或单个收尾状态作为整体验收 PASS。
