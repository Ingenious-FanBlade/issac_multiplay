> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 ownership/lifecycle v4 Group 2 — independent TEST failure report v1

Date: 2026-09-08  
Role: independent TEST  
Group: `2 — create-new production verification`  
Result: `FAIL`

## Sealed inputs

- candidate-pre TEST seal:
  `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-ownership-lifecycle-v4-candidate-test-seal-v1.md`
- seal SHA-256:
  `E9FE56403A1D216A2C53D392B47DF73CEE663ECBFCDAC2AFACFE872A3CDFA861`
- original candidate:
  `cs-t0/coop-virtual-device-ownership-lifecycle-decision-v4.json`
- original candidate SHA-256:
  `9C72C0ADA5440E4096DC93F90D6FC52197E3006005079D9932712043A6C3855A`
- pre-execution sealed hash check: `9/9 MATCH`
- canonical output before execution: `ABSENT`

## Single Group 2 execution

The sealed command was executed exactly once:

```powershell
pwsh -NoProfile -File .\cs-t0\verify-coop-virtual-device-ownership-lifecycle-decision-v4.ps1 -GameExe "F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe" -RepositoryRoot . -Output .\out\cs-t0\virtual-device-ownership-lifecycle-verification-20260908-v4.json
```

- process exit code: `1`
- verificationStatus: `REJECTED`
- verifier result exitCode: `2`
- error: `DECISION_CONSERVATIVE_BOUNDARY`
- outputWritten: `false`

The exact rejected field was `fixedGoal`:

- expected:
  `ENDPOINT_B_MUST_REMAIN_OPERATIONAL_WITHOUT_A_SECOND_PHYSICAL_CONTROLLER_ON_AUTHORITY`
- actual:
  `Endpoint B must keep working when Authority has no second physical controller.`

All other inspected conservative-boundary fields matched their frozen values.
The verifier rejected the candidate before target-anchor validation or output
publication.

## Output and successor gates

- canonical output:
  `out/cs-t0/virtual-device-ownership-lifecycle-verification-20260908-v4.json`
- canonical output after failure: `ABSENT`
- canonical output SHA-256: `NONE`
- Group 3: `NOT_RUN`
- Group 4: `NOT_RUN`

The one-shot Group 2 authorization was consumed. No retry was performed.
Groups 3–4 remain locked under the candidate-pre TEST seal.

## Safety boundary

No game was started. No target-game process was enumerated, opened, read,
attached to, injected into, hooked, or mutated. No IPC, input mutation, world
mutation, or game-runtime side effect occurred. No candidate, verifier, test,
or existing evidence file was modified, and no verification output was
created.
