> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 virtual-device lifecycle offline implementation v4 — independent TEST failure results

Date: 2026-09-08  
Role: independent TEST  
Scope: sealed group-1 offline harness invocation  
Result: `FAIL_ENVIRONMENT`; v4 candidate creation is forbidden

## 1. Sealed authorization and single execution

TEST was authorized to execute exactly one group-1 command and not retry it:

```powershell
pwsh -NoProfile -File .\cs-t0\test-virtual-device-lifecycle-offline.ps1 -Configuration Release -Output out/cs-t0/virtual-device-lifecycle-offline-harness-20260908-v1.json
```

The command was executed once from the repository root. It was not rerun.

## 2. Exact result

- exit code: `1`
- failure stage: `MSBuild`
- MSBuild error: `MSB4184`
- failing expression:
  `[Microsoft.Build.Utilities.ToolLocationHelper]::GetLatestSDKTargetPlatformVersion(Windows, 10.0)`
- environmental cause: access to
  `C:\Users\114514\AppData\Local\Microsoft SDKs` was denied
- dedicated C++ harness status: `NOT_EXECUTED`
- group-1 acceptance status: `FAIL_ENVIRONMENT`

MSBuild failed while evaluating `Microsoft.Cpp.WindowsSDK.props` before the
dedicated harness executable could be built and run. This result therefore
provides no PASS or FAIL evidence about the offline lifecycle implementation or
its assertions.

## 3. Create-new output disposition

The canonical requested output was:

`out/cs-t0/virtual-device-lifecycle-offline-harness-20260908-v1.json`

Its disposition after the failed command was independently checked read-only:

- output status: `ABSENT`
- output SHA-256: `NONE`
- partial or accepted report: none

No harness report exists for a v4 candidate to bind.

## 4. Remaining sealed groups

- group 2, production v4 decision create-new verification: `NOT_RUN`
- group 3, production v4 decision verify-existing: `NOT_RUN`
- group 4, parameterized negative batch: `NOT_RUN`

The single authorized group-1 run was consumed by the environmental failure.
The sealed no-rerun rule remains in force.

## 5. Runtime and permission boundary

TEST did not start, enumerate, open, attach to, read, or mutate a game process.
No game was started. No IPC, hook, injection, input mutation, world mutation,
virtual-device creation, passive canary, active canary, transport, scheduler, or
runtime observation occurred. No active authorization was requested or
consumed.

The attempt was limited to the dedicated offline `Release|Win32` MSBuild entry
and stopped at Windows SDK discovery. No other build, test, verifier, fixture
suite, Cargo command, Native matrix, or historical regression was run.

## 6. Acceptance disposition and required next action

`cs-t0/coop-virtual-device-ownership-lifecycle-decision-v4.json`
`MUST_NOT_BE_CREATED`. The required group-1 hash-bound PASS report is absent, so
`OFFLINE_IMPLEMENTATION_CONFIRMED` cannot be asserted and no v4 decision or
successor verification may proceed.

Before any further group-1 attempt:

1. fix the environment permission/access failure for
   `C:\Users\114514\AppData\Local\Microsoft SDKs`;
2. obtain new explicit user authorization for another run;
3. reseal the one-shot TEST run, including its run identity and create-new
   output path;
4. execute that newly authorized run at most once.

The existing failed run must not be silently retried, reclassified, overwritten,
or treated as implementation evidence.
