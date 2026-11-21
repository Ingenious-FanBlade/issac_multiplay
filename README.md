# issac_multiplay

从 `D:\projects\isaac_hook_reserach` 复制出的独立 C/S 研究工程，目标根目录为 `F:\projects_isaac\issac_multiplay`。迁移日期：2026-10-22；tx-03-09。保留 `cs-t0/` 层级以维持现有脚本、Rust嵌入资料、Native项目及文档相对路径。

- [文档入口](docs/README.md)：[总体架构](docs/architecture.md)、[采样](docs/capture.md)、[同步](docs/synchronization.md)、[Host 端设计](docs/host.md)、[Client 端设计](docs/client.md)
- [REPENTOGON mod 源码与构建](mod/README.md)
- [现行项目规范](docs/archive/refactor/project-conventions.md)
- [历史资料归档](docs/archive/README.md)：历史设计、逆向报告与验收记录，只作查证用，含[按功能模块整理的Hook与逆向点位](docs/archive/hooks/README.md)和[迁移范围、路径调整与历史产物](docs/archive/migration/README.md)

当前代码仍是T0研究原型；新状态同步模型已采纳为设计，尚未实现可玩Replica。迁移不把旧PASS扩大为snapshot apply完成，也不重判历史FAIL。

2026-10-24 tx-04-05：T1–T8 静态审查完成，按用户要求由 GPT-6 Luna / max 子智能体逆向、主智能体独立复核。随后完成 core 移植、game 主线程适配及必要 Hook/solo 命令接线并纯构建成功。剩余 Lua 测试夹具与 Rust harness 的 S0–S11 / V1–V10 已编写，见[记录](docs/archive/design/tx-04-05-test-harness.md)。没有运行测试，唯一整体 E2E 尚未执行。tx-04-04 第八次整体 E2E 已 PASS，下文初次失败记录保留。

2026-10-24 tx-04-04 已新增 `mod/`、`ui/`、`tools/harness/`，DLL 和 i686 harness 编译成功。唯一 E2E 在注入启动阶段 Launch 等待 60 秒超时，未进入菜单、未重跑；运行时能力尚未验证。下文的迁移记录保留其当时语境。

## 源码与构建

`cs-t0/src`为Rust控制器/分析器，`cs-t0/native`为Win32 Native/Helper与既有场景源码，`cs-t0/world_e2e`及脚本保存原研究执行器。Cargo.lock、rust-toolchain、catalog/testdata/ledger/archive均已复制。根目录保留原`Directory.Build.props`及其`VERSION`文件依赖。

构建依赖按现有工程：Rust工具链见`cs-t0/rust-toolchain.toml`与Cargo.toml；Native需要Visual Studio 2022/v143及Windows SDK，目标Win32。以下仅是未来纯构建入口，本次未执行：

```powershell
cargo build --manifest-path .\cs-t0\Cargo.toml --locked --release --bin IsaacCsT0T09
pwsh -File .\cs-t0\build-t09-b.ps1 -OutputDirectory out/cs-t0/new-build
```

旧`build.ps1`、`verify-*`和部分历史accept入口可能执行测试或旧哈希流程，使用前必须读实现并遵守现行规范；不能作为迁移后例行命令直接运行。旧编译二进制、Cargo缓存和大体积运行产物未搬入；本次没有构建或运行兼容性结论。

目标目录原有Git元数据保持原样；没有创建提交、推送或迁移原仓库`.git`。本次迁移的源文件清单见[复制manifest](docs/archive/migration/source-copy-manifest.json)。
