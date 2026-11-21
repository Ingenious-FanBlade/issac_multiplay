# tx-01-57：整理 cs-t0/tests

日期：2026-10-17。用户要求在 `cs-t0/tests` 新建 `trash`，把无用的 JSON 和脚本放入其中。本次目标是归档已停用的旧验收工具，保留现用文件和完整历史；不改游戏运行逻辑，不运行构建、测试、哈希校验或实机操作。

## 分类依据与修改

静态读取 Cargo 配置、集成测试引用、旧 Python/JSON、PowerShell 包装入口和现行规范。根目录8个 Rust 集成测试由 Cargo 默认发现，`support/` 的2个 Rust fixture 被这些测试引用，均保留原位。

15组历史工具依赖已取消的旧 Join admission/static closure/Virtual Device decision、哈希验收、预封及独立角色流程；现行 `accept-*-e2e.ps1`、`report-*-e2e.py` 和生产源码不调用这些工具。按原目录结构迁移到 `cs-t0/tests/trash/`，总计121文件，包括78 Python、8 JSON、8 README、27缓存。未删除或改写迁移文件。

15个外部旧 PowerShell 入口仍引用旧位置。原入口完整复制到 `trash/entrypoints/`，原路径保留参数定义并改为明确归档提示，停止调用 Python 验收程序。归档副本保留当时的行为，不承诺在新位置直接运行。原始报告和JSON中的历史路径/哈希不批量改写；迁移索引提供旧→新路径查询。

新增 [`tests/README.md`](../../../../../../cs-t0/tests/README.md)、[`trash/README.md`](../../../../../../cs-t0/tests/trash/README.md) 和无哈希的逐文件 [`archive-index.json`](../../../../../../cs-t0/tests/trash/archive-index.json)。项目 README 添加整理入口。Cargo、Rust测试、support、现行E2E、fixture、原始结果和游戏进程均不修改。

## 一次整体静态验收

全部整理结束后，一次静态审阅确认：15个目录的121个迁移文件均位于索引指定位置，原路径已移走、文件大小符合迁移前记录；15个入口副本存在，原入口参数前缀保留并在归档提示处停止；8个Rust集成测试、2个support文件保留。归档目录外没有代码或脚本继续引用这些旧Python路径，检查问题列表为空。

目录结构与README/迁移索引链接已核对。检查只读取文件、源码文本和路径，不导入或执行归档脚本，不运行 Rust/Python/Native 测试，不计算哈希。此结果是文件整理的静态完成记录，不改变T0-05功能验收状态。
