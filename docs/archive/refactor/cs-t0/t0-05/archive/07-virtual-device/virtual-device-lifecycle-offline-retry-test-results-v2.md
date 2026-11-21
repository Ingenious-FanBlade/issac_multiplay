> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 Virtual Device Lifecycle Offline Group-1 Retry — TEST Results v2

Date: 2026-09-08  
Role: independent TEST  
Result: `FAIL`

## Sealed authority and precheck

- retry preseal: `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-lifecycle-offline-retry-test-plan-v4.md`
- retry preseal SHA-256: `82E66F63A75142BB959A4543AC0FB1BB235BE5325314871660B3F0D903848938`
- Section 2 frozen binding precheck: `18/18 MATCH`
- reserved create-new output before execution:
  `out/cs-t0/virtual-device-lifecycle-offline-harness-20260908-v2.json` = `ABSENT`

The command ran exactly once with the approved minimal escalated filesystem
context:

```powershell
pwsh -NoProfile -File .\cs-t0\test-virtual-device-lifecycle-offline.ps1 -Configuration Release -Output out/cs-t0/virtual-device-lifecycle-offline-harness-20260908-v2.json
```

- exitCode: `1`
- MSBuild: `EXECUTED`
- dedicated Release/Win32 build: `COMPLETED`
- offline harness: `EXECUTED`
- harness executable SHA-256:
  `AAF922B9EE4513D05457CEAB45927ACB39FB7D09C9A76A09A4A8BD9C1FBF0A6C`

## Harness result

- identity: `IsaacCsT0-VirtualDeviceLifecycleOfflineHarness-v1`
- architecture: `Win32/x86`
- status: `FAIL`
- caseCount: `16`
- assertionCount: `333`
- passing cases: `15`
- failing cases: `1`
- all 13 failure/rollback cases: `PASS`

The sole failure was `CALLBACK_MOVE_ONLY_KIND_AND_RELEASE`. Its failure text was
`callback move/release order mismatch`: the old ordered-subsequence assertion
did not match the emitted callback move/release operation order. No other case
failed.

## Output and successor gates

- canonical report output after failure: `ABSENT`
- canonical report SHA-256: `NONE`
- `TEST_RETRY_PRESEAL` v4 runtime binding: `NOT_OBSERVED`
- groups 2, 3, and 4: `NOT_RUN`
- v4 candidate creation: `PROHIBITED`

The runner stopped before create-new report publication, so the missing v4
runtime binding must not be inferred from the harness stdout.

## Preserved safety boundary

No game was started. No target-game process was enumerated, opened, read,
attached to, injected into, hooked, or mutated. No IPC was created, and no game
runtime, input, or world mutation occurred. The only executed work was the
dedicated offline Release/Win32 build and its standalone harness.

This one-shot retry is consumed and must not be rerun. Fixing the failing
assertion does not authorize execution: a further attempt requires new explicit
user authorization and a new independent TEST reseal after the change is
statically reviewed and hash-bound. Until then, groups 2–4 remain locked and no
offline, native, or formal status may be promoted.
