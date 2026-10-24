# cs-t0 历史归档

本目录保存截至 2026-08-27、T0-04 正式门禁完成后从 `cs-t0/` 顶层收口的过程材料。归档项不再作为当前操作入口或运行时输入，但仍是失败假设、版本演进和实机结论的审计证据，因此没有删除或改写其机器身份。

当前结论请从 [`docs/refactor/cs-t0/t0-04/isaac-cs-t0-04-reverse-handoff.md`](../../docs/refactor/cs-t0/t0-04/isaac-cs-t0-04-reverse-handoff.md) 进入；当前命令和机器契约请从 [`cs-t0/README.md`](../README.md) 进入。

## 过程文档

| 文件 | 归档原因 |
| --- | --- |
| [`t0-04-history/input-active-loopback-next-steps.md`](t0-04-history/input-active-loopback-next-steps.md) | Active loopback 与 18+2 正式门禁的完成过程；结论已进入逆向交接 |
| [`t0-04-history/input-consumer-research.md`](t0-04-history/input-consumer-research.md) | Schema-3 raw/consumer 的逐轮研究与反证 |
| [`t0-04-history/input-draft-research.md`](t0-04-history/input-draft-research.md) | 早期 input 草案，未作为当前机器契约 |
| [`t0-04-history/input-passive-validation.md`](t0-04-history/input-passive-validation.md) | 早期 witness/RPM 记录，已由后续 V5/Schema-4 证据闭合 |
| [`t0-04-history/loop-envelope-research.md`](t0-04-history/loop-envelope-research.md) | T0-03R loop envelope 的逐轮历史与失败样本 |

## 冻结的旧机器契约

下列 JSON 已不被当前源码或命令读取，仅保留为不可覆盖的历史证据。移动前后内容未修改，SHA-256 保持一致。

| 文件 | SHA-256 | 地位 |
| --- | --- | --- |
| `raw-apply-catalog-v1.json` | `C01E325A592876D0D2C0EC1354EFF1ED72C3B7E4D25E3B4FFAA0212F447A2D0C` | `FROZEN_REJECTED_HYPOTHESIS` |
| `raw-apply-capture-plan-v1.json` | `DEE4C87298045174F76689D4D5DB4D97AE6F4977303D1E89E84AC598C6847805` | 与 v1 catalog 一同冻结 |
| `raw-apply-catalog-v2.json` | `F64FA3FFF87F1B0D1AEA8C4FAE8C849E229459222CAB7CB1B9B1D232E2489266` | 历史 alias-classifier 修订，已被 v3/v4 取代 |
| `raw-apply-capture-plan-v2.json` | `6A7BC25ECDDCA0D556DE64BECAE8F2799B4AFC1E3D3618DF00E494CC6C3633CE` | 与 v2 catalog 一同冻结 |

## 为什么还有旧版本留在顶层

- `input-active-catalog-v1.json` 与 `input-active-plan-v1.json` 是拒绝地址映射的负向回归夹具，测试仍按 `CARGO_MANIFEST_DIR` 固定路径读取。
- `raw-apply-catalog-v3.json` 与 `raw-apply-capture-plan-v3.json` 是 v4 校验器的拒绝回归夹具，测试仍按顶层固定路径读取。
- `input-active-verification-v1.json`、`input-active-matrix-v2.template.json` 等文件同样由脚本或源码直接读取，不能因版本号较旧而归档。

若未来修改测试夹具路径，应先更新源码并完整运行测试，再另行移动；不能直接删除这些文件。
