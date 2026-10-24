# cs-t0 tests

2026-10-17，tx-01-57 整理。执行约定见[现行项目规范](../../docs/refactor/project-conventions.md)。

此目录保留 Cargo 默认发现的 8 个 Rust 集成测试，以及它们使用的 `support/`：

- `active_cross_language.rs`
- `coop_c0_contract.rs`
- `coop_device_rebind_contract.rs`
- `coop_device_rebind_cross_language.rs`
- `coop_native_join_evidence_contract.rs`
- `coop_passive_applicability_contract.rs`
- `coop_passive_contract.rs`
- `coop_passive_cross_language.rs`

保留这些文件不表示要求执行旧测试链；每次推进的测试安排仍按现行规范决定。

旧 Join admission、static closure、Virtual Device decision/ownership 验收脚本、基线JSON及缓存曾在tx-01-57整理，见[当时记录](../../docs/refactor/cs-t0/t0-05/07-virtual-device/tx-01-57-tests-cleanup.md)。其旧`tests/trash`说明和索引在本轮开始时已不在工作树中，不补造历史文件。

2026-10-18，tx-01-72继续整理`cs-t0/`根目录的退役脚本、历史JSON及旧admission检查包，移入[统一trash入口](../trash/README.md)，逐文件位置见[本批旧→新索引](../trash/tx-01-72/archive-index.json)。这些旧工具依赖已取消的哈希、gate等流程，不是当前E2E入口；仍被源码/数据依赖的文件保留原路径。

当前真实游戏 E2E 的统一入口是 `cs-t0/accept-*-e2e.ps1`，场景输入位于 `cs-t0/testdata/`，结果分析位于 `cs-t0/report-*-e2e.py`。本次整理未移动这些入口、修改 Cargo 配置或运行任何测试。
