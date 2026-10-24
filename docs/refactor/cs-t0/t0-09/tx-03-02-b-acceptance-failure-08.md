# tx-03-02：T0-09-B 统一验收失败 08

日期：2026-10-20（Asia/Shanghai）。事实记录；完整 artifact 原样保留，不自动重跑。

## 实际调用

```powershell
python cs-t0/accept-t09-b.py `
  --artifact-dir artifacts/cs-t0/tx-03-02-e2e-08 `
  --authorization "user requested T0-09-B work in this chat; offline two-process foundation only"
```

后台 job 一次执行。build exit 0，offline F0 runner exit 0，analyzer **40/41 PASS**。

## 结果

唯一失败项：`orchestrator.offline-pass`。orchestrator 原始结果实际为：

```json
{
  "status": "PASS",
  "runStartSent": true,
  "stopSent": true,
  "targetReached": true,
  "allRetired": true,
  "rolesRetired": true,
  "realGame": "NOT_RUN",
  "error": null
}
```

analyzer 错把 `rolesRetired` 的布尔值 `true` 与整数 `2` 比较，因此判失败。没有任何运行/协议
失败：两端 controller exit 0、relay exit 0、allRetired=true、双端 STOP/RETIRE、Wire overwrite=0
及全部 packet/control/F0 检查均为 PASS。

artifact：`artifacts/cs-t0/tx-03-02-e2e-08/`。

## 修正

把 analyzer 条件改为 `rolesRetired is True`，并在失败 detail 中一并记录该字段。该次实际运行的
原始 artifact 保持 FAIL 状态，不修改为 PASS；修正后使用新的 create-new artifact 根执行。
