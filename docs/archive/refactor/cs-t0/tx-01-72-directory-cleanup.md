# tx-01-72：整理 cs-t0 的旧测试脚本与JSON

日期：2026-10-18。用户要求把`cs-t0/`下无用测试脚本和JSON放入trash。本次完整目标是辨认已退役文件、保留现有依赖、原样移动到`cs-t0/trash/tx-01-72/`并提供可恢复的路径索引。遵循[现行规范](../project-conventions.md)，不运行游戏、构建、代码测试或哈希校验；不修改运行时代码、Cargo/Native配置或现行E2E逻辑。

## 1. 范围与判定

先静态阅读当前源码、构建/E2E入口、测试和数据引用，再从根目录118个JSON及32个旧verify/inspect/lifecycle脚本中选择迁移对象。保留代码、当前工具、testdata/ledger中的引用作为起点，递归保留它们引用的根目录JSON/脚本；Markdown历史链接不视为执行依赖，也不按版本号直接判废弃。读取字符串/路径不执行验证器或摘要函数。

旧Join admission、static closure、Virtual Device decision/ownership检查流程已经退役。部分verify脚本在tx-01-57已成为只抛归档提示的stub，本轮把这些stub也原样移出根目录；不伪造已缺失的旧原始入口。admission、admission_v2–v7只服务迁入的旧inspect链，没有保留的可执行/数据引用，连同配套说明/缓存一起移入，保持包结构。

一处初稿误命中是Native测试提示字符串`admission/publication`，并非Python包引用。最终分析按带`.py/.json/.ps1`的路径识别，确认该包也可移走。初稿[计划](../../../../artifacts/cs-t0/tx-01-72-cleanup/migration-plan.json)与[最终计划](../../../../artifacts/cs-t0/tx-01-72-cleanup/migration-plan-final.json)分别保留；这是资料分析修订，没有提前执行测试或试移动。

## 2. 实际移动和保留

移动129个文件：48 JSON、26 PowerShell、34 Python、6说明、15缓存。其中74个从根目录移出，另55个随7个旧admission包移动。所有源和目标绝对路径先限定在用户指定工作区/`cs-t0`/本批trash目录，排除reparse路径及目标冲突后，用原生PowerShell`Move-Item -LiteralPath`同卷移动，不覆盖原文件、不改内容。

原位置保留70个根目录JSON、6个旧verify脚本以及全部当前E2E、采集/报告、build、Rust/Native代码、场景输入和ledger。保留项逐一记录首个引用来源；这使当前编译嵌入、数据绑定及已有测试读取不会因目录整理失去文件，不代表认可旧工具的流程要求。例如：

- `coop_device_rebind.rs`与`coop_passive_commands.rs`编译嵌入catalog/plan。
- `coop_native_join.rs`编译嵌入static/catalog/predecessor/plan/applicability。
- `raw_apply_commands.rs`编译嵌入raw-apply catalog/plan。
- `coop_passive_commands.rs`及保留Rust测试还引用passive/verifier和既有JSON；其传递依赖一并保留。

本轮开始时已有源文件修改、删除与未跟踪文件；不重置、不恢复或覆盖这些工作。`cs-t0/tests/README.md`原指向tx-01-57旧trash说明/索引的链接，在当前工作树中目标已缺失；本轮说明改为指向现有历史记录和本次新索引，不补造历史证据。历史报告、JSON内部原路径/哈希/判定和artifacts不批量重写。`cs-t0/README.md`中直接指向本批文件的Markdown链接迁至真实新位置；其他历史路径按索引查找。

## 3. 可恢复记录

- [归档入口](../../../../cs-t0/trash/README.md)、[本批说明](../../../../cs-t0/trash/tx-01-72/README.md)、[逐文件索引](../../../../cs-t0/trash/tx-01-72/archive-index.json)。
- [移动日志](../../../../cs-t0/trash/tx-01-72/movement-journal.jsonl)、[移动完成记录](../../../../artifacts/cs-t0/tx-01-72-cleanup/move-completed.json)。
- [README的19处链接迁移记录](../../../../artifacts/cs-t0/tx-01-72-cleanup/readme-link-relocations.json)：只改Markdown目标位置，保留历史文字与迁入文件内容。
- [引用分析脚本](../../../../artifacts/cs-t0/tx-01-72-cleanup/plan-cleanup.ps1)、[有界移动脚本](../../../../artifacts/cs-t0/tx-01-72-cleanup/move-planned-files.ps1)。两者只执行本次文件/资料整理，不调用项目测试、旧验证器或哈希算法。

恢复所需文件时，使用索引的`destination→original`移动，并先处理已有目标冲突。相对路径脚本必须恢复原位置后才有原路径含义；归档并未使旧gate/哈希流程重新有效。

## 4. 本轮一次整体静态审阅

所有移动与入口说明完成后，只做一次覆盖本轮目标的静态目录/文档审阅：核对129条迁移位置、原路径移出、文件长度、保留依赖、编译嵌入文件、入口链接和变更范围。不调用被移动或保留的验收程序，不编译、不运行测试、不计算哈希，不把文件长度核对写成内容哈希证明。

结果：`PASS_STATIC_DIRECTORY_REVIEW`，见[唯一一次整体静态审阅记录](../../../../artifacts/cs-t0/tx-01-72-cleanup/static-review.json)。129条原路径与129条新路径均唯一，原文件均已移出、目标存在且长度与迁移前记录一致；76个保留候选均在原位置。日志完整记录7次目录移动及74次根目录文件移动，无移动错误。

18处Rust字面量编译嵌入JSON引用的目标均存在；8个Rust集成测试保留。根目录JSON从118个减为70个，PowerShell从46个减为20个。新说明/入口段落的26处文件链接及19处README迁移链接均可定位，检查问题列表为空；本段归档新增的审阅记录由该次执行直接生成，没有再次运行验收。

本轮仅执行一次目录/文档静态审阅，没有游戏、构建、Rust/Python/Native代码测试、旧验证器或哈希校验。目录整理不改变T0-05/T0-06的功能验收状态。
