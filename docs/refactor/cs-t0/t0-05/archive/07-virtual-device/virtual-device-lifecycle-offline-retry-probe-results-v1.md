> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 Virtual Device Lifecycle Offline Retry Probe — TEST Results v1

## Result

- role: `TEST`
- result: `PROBE_FAILED`
- readyForSingleRetryRun: `false`
- sdkReadability: `NOT_OBSERVED`
- retryPreseal:
  - path: `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-lifecycle-offline-retry-test-plan-v3.md`
  - sha256: `29DB7B7F76EFC0B49BECA3B355C6201372E5A55C86AC4C45D964E0F21DAF706E`

## Single Probe Attempt

The sealed read-only probe requested `require_escalated`; escalation was approved and the command was launched exactly once:

```powershell
pwsh -NoProfile -Command "$sdkDiscoveryPath='C:\Users\114514\AppData\Local\Microsoft SDKs'; $null=Get-Item -LiteralPath $sdkDiscoveryPath -Force -ErrorAction Stop; $null=Get-ChildItem -LiteralPath $sdkDiscoveryPath -Force -ErrorAction Stop | Select-Object -First 1; [pscustomobject]@{probeStatus='PASS'; path=$sdkDiscoveryPath; readOnly=$true} | ConvertTo-Json -Compress"
```

- exitCode: `1`
- jsonResult: `ABSENT`
- failure: `ParserError: Missing statement after '=' in hash literal.`
- cause: the outer PowerShell expanded `$sdkDiscoveryPath`, `$null`, and `$true` before the nested `pwsh -Command` parsed its script, leaving an invalid `path=` expression.

The parser failed before either SDK-path read operation ran. Therefore accessibility of `C:\Users\114514\AppData\Local\Microsoft SDKs` was not observed and must not be inferred.

## Preserved Boundaries

- MSBuild: `NOT_RUN`
- offline harness: `NOT_RUN`
- sealed retry: `NOT_RUN`
- `out/cs-t0/virtual-device-lifecycle-offline-harness-20260908-v2.json`: `ABSENT`
- groups 2–4: `LOCKED`
- game/process/IPC/runtime access: `NONE`

No automatic retry was performed. Any future probe requires new explicit user authorization and a newly sealed, argument-safe probe implemented as a script file or an equivalently argument-safe invocation. The single offline retry remains prohibited until that probe returns a valid `probeStatus=PASS` result.
