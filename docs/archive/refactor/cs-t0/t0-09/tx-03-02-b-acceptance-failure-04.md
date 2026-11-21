# tx-03-02：T0-09-B 统一验收失败 04

日期：2026-10-20（Asia/Shanghai）。事实记录；artifact 原样保留，不自动重跑。

## 实际调用

```powershell
python cs-t0/accept-t09-b.py `
  --artifact-dir artifacts/cs-t0/tx-03-02-e2e-04 `
  --authorization "user requested T0-09-B work in this chat; offline two-process foundation only"
```

## 结果

- 纯编译 exit 0。
- F0 orchestrator 在 manifest 校验阶段 exit 2，未启动 relay/controller/driver。
- 未创建 `manifest.json` 或 relay/host/vm 运行目录。
- analyzer 因 required JSON 缺失记录 0/1 FAIL。
- 未启动游戏、未注入、未操作 Original Online/Isaac Sync、未重跑。

## 原始错误

`artifacts/cs-t0/tx-03-02-e2e-04/control/orchestrator.stderr.log`：

```text
IsaacCsT0T09: endpoint os must be 1..=16 bytes
```

## 根因与修正

验收 manifest 把 B endpoint 的 `os` 写成 `host-offline-process`（21 字节），超过冻结 schema
的 1..16 字节限制。修正为 `os="host"`；离线性质仍由顶层 acceptance scope、
`realHostVmP1=NOT_RUN` 和 artifact 路径明确表达，不伪造 VM endpoint。

修正后需使用新的 create-new artifact 根执行；本次失败保留。
