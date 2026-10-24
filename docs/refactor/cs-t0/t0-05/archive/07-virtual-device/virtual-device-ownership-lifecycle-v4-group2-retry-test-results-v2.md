> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 ownership/lifecycle v4 Group 2 retry — independent TEST failure report v2

Date: 2026-09-08  
Role: independent TEST  
Group: `2 — corrected-candidate create-new retry`  
Result: `FAIL`

## Sealed inputs

- retry TEST seal:
  `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-ownership-lifecycle-v4-group2-retry-test-seal-v2.md`
- retry seal SHA-256:
  `FB20102E4C00F57D0F12A1692E81CC722F7434F16DAA6D2E94EDFEC56BAC217A`
- corrected candidate:
  `cs-t0/coop-virtual-device-ownership-lifecycle-decision-v4.json`
- corrected candidate SHA-256:
  `E787255F1ED276FC7C9D5350A9245CEB2BDED121029AD0DAA0A416EE6B6A352F`
- pre-execution frozen hash check: `8/8 MATCH`
- canonical output before execution: `ABSENT`

## Single retry execution

The retry-sealed Group 2 command was executed exactly once:

```powershell
pwsh -NoProfile -File .\cs-t0\verify-coop-virtual-device-ownership-lifecycle-decision-v4.ps1 -GameExe "F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe" -RepositoryRoot . -Output .\out\cs-t0\virtual-device-ownership-lifecycle-verification-20260908-v4.json
```

- process exit code: `1`
- verificationStatus: `REJECTED`
- verifier result exitCode: `2`
- error: `OFFLINE_RUNTIME_DEPENDENCY:openprocess`
- outputWritten: `false`
- formalDecision: `null`
- workPackageStatus: `BLOCKED_BY_EVIDENCE`
- implementationEligibility: `false`

The corrected candidate digest remained
`E787255F1ED276FC7C9D5350A9245CEB2BDED121029AD0DAA0A416EE6B6A352F`
after the failed verification.

## Static token localization

The post-failure `rg` localization found the token `openprocess` only in
`cs-t0/test-virtual-device-lifecycle-offline.ps1`, where it is literal text in
the offline runner's forbidden-symbol list. The match is not an invocation of
`OpenProcess`, a runtime dependency, or evidence of process access. This report
records the deterministic false-positive location only; it does not alter the
sealed verifier, runner, candidate, or evidence.

## Output and successor gates

- canonical output:
  `out/cs-t0/virtual-device-ownership-lifecycle-verification-20260908-v4.json`
- canonical output after failure: `ABSENT`
- canonical output SHA-256: `NONE`
- Group 3: `NOT_RUN`
- Group 4: `NOT_RUN`

The retry Group 2 authorization was consumed by this failure. No rerun was
performed. Groups 3–4 remain locked and require new explicit user authorization
and a new independent TEST seal after any narrow fix.

## Safety boundary

No game was started. No target-game process was enumerated, opened, read,
attached to, injected into, hooked, or mutated. No IPC, input mutation, world
mutation, or game-runtime side effect occurred. No verification output was
created, and no candidate, verifier, test, seal, runner, or existing evidence
file was modified by TEST.
