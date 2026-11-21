# tx-03-03：新版实验 F0 失败 04（image profile 迭代）

日期：2026-10-21（Asia/Shanghai）。事实记录；artifact 原样保留。

## 结果

第四次 live 尝试 `tx-03-03-live-04` 再次在 host A image check 停止：

- helper `native_error=2`（Image）；
- 未生成 VM 运行提示（VM script 未运行）；
- 未进入 READY/ARMED/RUN_START；
- orchestrator 检测 A child early exit，停止 relay；
- 无 T09 进程残留；host 游戏仍有 resident Native DLL，下一次需重启。

## 新观察

文件本身 canonical PE 元数据匹配 approved；但运行中读取模块内存发现：

- loaded base = `0x790000`
- **in-memory `OptionalHeader.ImageBase` 也被改写为 `0x790000`**
- `NumberOfSections=6`、entry `0x8FE310`、SizeOfImage `0x92C000`
- 六个 section 与 canonical approved 完全一致（含 `.bind`）

因此第一次实验 profile `RelocatedCanonicalExperimental`（要求 header base 仍为 preferred）
不匹配；它把 runtime-rebased canonical 与 canonical-on-disk 混为一类。

## 修复

新增 T09-only `CanonicalRuntimeRebasedExperimental`：

- `header ImageBase == loaded module base`；
- 6 个 canonical sections、canonical SizeOfImage/entry；
- 仅通过 `IsApprovedT09` 暴露；
- 仍必须在 Prepare 阶段通过 exact hook target/RVA/bytes 检查。

同日已 pure compile Native/Helper/Rust；下一次 live 必须使用新 build、重启 host/VM 游戏，
并继续保留 experimental 结果边界。

## Artifact

`artifacts/cs-t0/tx-03-03-live-04/`
