> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 virtual-device lifecycle offline implementation v4 — independent TEST preseal

Date: 2026-09-08  
Role: independent TEST  
Phase: candidate absent / pre-implementation / pre-REVERSE / pre-execution  
Scope: offline-only lifecycle coordinator implementation and conservative v4 decision verification

## 1. Purpose and authority boundary

This document seals the independent TEST acceptance boundary before any v4
implementation, v4 REVERSE handoff, v4 decision, v4 verifier, or v4 test run
exists. It does not implement a virtual device, perform reverse engineering, or
grant factual native evidence.

The offline module may demonstrate that a bounded coordinator design can be
implemented and tested in isolation. It cannot prove that the fixed game binary
has that coordinator, that InputManager follows the offline protocol, or that a
real virtual device is eligible. Design prose and offline tests are never
`STATIC_CONFIRMED` native evidence.

TEST must not start, enumerate, open, read, attach to, inject into, hook, or
mutate a game process. It must not create IPC, send input, alter assignment,
construct a game-side device, or consume an active authorization. OriginalOnline
`NetInputDevice` and Online Manager state are forbidden implementation
components.

## 2. Fixed target and predecessor bindings

### 2.1 Disk PE

The v4 decision verifier in groups 2 and 3 binds the same read-only disk PE:

- path: `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe`
- machine: Win32/x86, PE machine `0x014C`
- SHA-256: `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`
- runtime address rule, for static evidence only: `moduleBase + RVA`

No alternative binary, relocated byte window, runtime module, or candidate-only
anchor may satisfy this binding.

### 2.2 Exact v3 predecessor set frozen by this preseal

| Path | Required SHA-256 |
|---|---|
| `cs-t0/coop-virtual-device-ownership-lifecycle-decision-v3.json` | `6E069DD4A66D90A4122D4D1D189ED99C2CFA101357515CC6D2AEBC2A9F01E219` |
| `out/cs-t0/virtual-device-ownership-lifecycle-verification-20260908-v3.json` | `B53896E691F3B5EAABA05CEEA9740BDB7D914D6BCF8EF12D525081F5B65964BB` |
| `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-lifecycle-coordinator-contract-v1.md` | `7999DA674CC04905D857A7F542174E564FDA33082D492A0B44D31849C40A9665` |

The future v4 candidate must also bind this preseal by its final SHA-256. The
candidate, not this preseal, must create exact bindings for the later-created
source, project, harness entry, harness report, verifier, sealed v4 tests, and
independent REVERSE handoff:

`docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-ownership-lifecycle-joint-reverse-v2.md`

Missing, extra, substituted, duplicate, reordered where order is sealed, or
digest-mismatched bindings are rejected. A digest recorded by an implementation
or candidate cannot override the digest TEST independently recomputes.

## 3. Exact offline module and build isolation

The complete permitted offline implementation/harness file set is:

1. `cs-t0/native/include/isaac_cs_t0/virtual_device_lifecycle.hpp`
2. `cs-t0/native/src/virtual_device_lifecycle.cpp`
3. `cs-t0/native/tests/virtual_device_lifecycle_main.cpp`
4. `cs-t0/native/IsaacCsT0.VirtualDeviceLifecycleTests.vcxproj`
5. `cs-t0/test-virtual-device-lifecycle-offline.ps1`

The implementation types must be in namespace
`isaac_cs_t0::virtual_device`. The `.vcxproj` is a standalone
`Release|Win32` Application and may compile only the dedicated lifecycle source
and test harness plus the headers they require. Its executable is an offline
test program, not a DLL and not a game-loadable artifact.

The header/source/test files must appear only in
`IsaacCsT0.VirtualDeviceLifecycleTests.vcxproj`. They must not be added to:

- `cs-t0/native/IsaacCsT0.Native.vcxproj`;
- any other production `.vcxproj`, solution, DLL, loader, injection path, or
  post-build deployment step;
- `cs-t0/build.ps1`.

The production native project and `build.ps1` must be byte-for-byte unchanged
from their candidate-bound predecessors. The offline entry may build only the
dedicated `Release|Win32` Application. It must not invoke the complete T0 build,
Debug configurations, x64 configurations, Cargo, a Native matrix, or a game.

## 4. Exact public contract

The coordinator type is `VirtualDeviceLifecycleCoordinator`. It exposes exactly
these four public lifecycle APIs, with these signatures:

```cpp
LifecycleError CreateAndRegister(EngineLifecyclePort&, ThreadToken) noexcept;
LifecycleError BeginRetire(EngineLifecyclePort&, ThreadToken) noexcept;
LifecycleError RemoveAndDestroy(EngineLifecyclePort&, ThreadToken) noexcept;
LifecycleSnapshot Snapshot() noexcept;
```

There may be no other public lifecycle transition API. Lease admission is a
private coordinator operation reachable only by the lease constructors below.
`Snapshot()` is observational: it cannot transition state, acquire or release
an engine reference, admit work, invoke a callback, or repair an invariant.

`ReaderLease` and `CallbackLease` are separate move-only RAII types. Their
constructor shapes are frozen as:

```cpp
ReaderLease(VirtualDeviceLifecycleCoordinator&, EngineLifecyclePort&, ThreadToken) noexcept;
CallbackLease(VirtualDeviceLifecycleCoordinator&, EngineLifecyclePort&, ThreadToken, CallbackKind) noexcept;
```

For both lease types:

- copy construction and copy assignment are deleted;
- move construction and move assignment transfer the one live admission and
  engine reference; the moved-from object is inert;
- destruction releases through the supplied `EngineLifecyclePort` reference
  ABI exactly once when, and only when, the lease is valid;
- `valid()` and `error()` are queryable without mutation;
- failed admission produces an invalid lease and performs no unmatched release;
- no lease may be stored by the coordinator, vector, callback, reader, scheduler,
  IPC, helper thread, or test shim beyond its bounded operation;
- no live lease may cross a completed drain, removal, final release, or
  destruction boundary.

`CallbackLease` additionally records the exact `CallbackKind`, and its admission
and release must remain in the corresponding insert/remove callback interval.
The harness port is a deterministic fake of the frozen ABI; it is not evidence
that the native ABI or caller graph follows this contract.

## 5. Exact seven-state machine

No state names other than these seven are permitted, in this order:

```text
EMPTY
OBJECT_ALLOCATED
CONTROL_READY
REGISTERED
RETIRING
UNREGISTERED
DESTROYED
```

The only forward lifecycle is:

```text
EMPTY -> OBJECT_ALLOCATED -> CONTROL_READY -> REGISTERED
REGISTERED -> RETIRING -> UNREGISTERED -> DESTROYED
```

Constructor completion, control binding, vector insertion, publication,
publication close, reader/callback drain, vector removal, final strong release,
object destruction, final weak release, and control-block free are
transition/action properties, not extra states.

The public API/state mapping is exact:

- `CreateAndRegister`: drives `EMPTY -> OBJECT_ALLOCATED -> CONTROL_READY -> REGISTERED`;
- `BeginRetire`: drives `REGISTERED -> RETIRING`, closes admission, publishes
  neutral state, and completes bounded reader/callback drain before reporting
  success;
- `RemoveAndDestroy`: requires drained `RETIRING`, drives
  `RETIRING -> UNREGISTERED -> DESTROYED`, and preserves exactly-once removal and
  release ordering;
- `Snapshot`: no transition.

Transitions are monotonic. Direct `REGISTERED -> DESTROYED`,
`RETIRING -> DESTROYED`, publication before valid control ownership, removal
before admission close and drain, or destruction before confirmed removal is a
hard failure. A retry after a completed terminal transition must be rejected
without a compensating release.

Every harness event must record its state before/after, operation, reference
class delta, thread token/class, result, and failure injection point. The
snapshot must expose sufficient counters and flags for TEST to derive state,
publication/admission closure, admitted readers/callbacks, vector membership,
reference-class cardinalities, delete/free counts, and sticky first failure.

## 6. Reference classes and bounds

The six and only six permitted strong-reference classes are:

1. `FACTORY_LOCAL_STRONG`
2. `MANAGER_VECTOR_STRONG`
3. `INSERT_CALLBACK_TRANSIENT_STRONG`
4. `REMOVE_CALLBACK_TRANSIENT_STRONG`
5. `READER_TRANSIENT_STRONG`
6. `ROLLBACK_TRANSIENT_STRONG`

Each class has cardinality `0..1`; their simultaneous total is at most `6`.
`CONTROL_IMPLICIT_WEAK` has cardinality `0..1`.
`EXTERNAL_WEAK` has exact maximum `0`. No seventh strong class, external weak,
unbounded alias, retained callback copy, or stored reader copy is allowed.

All acquire/release operations go through `EngineLifecyclePort`. Manual
arithmetic on native 16-bit counters is forbidden. The harness must prove for
every success and injected-failure trace:

- no overflow or underflow;
- no decrement from zero, unmatched release, double release, or release by a
  moved-from/invalid lease;
- no acquire after publication close or after final strong release starts;
- every transient is balanced before its enclosing operation returns;
- exactly one object deletion and one control-block free on each completed
  lifecycle, and zero premature deletion/free on fail-closed paths;
- no callback or reader survives completed drain;
- the coordinator root is never released before confirmed vector removal.

These are offline implementation properties only. The native six-slot ABI,
copy/grow/callback behavior, actual counter width, and native last-release caller
remain governed by independent hash-bound REVERSE evidence.

## 7. Exact failure and rollback matrix

The offline harness must inject all thirteen failure IDs exactly once within its
single group-1 execution and verify the ordered safe result defined by contract
v1:

| Failure ID | Required offline result |
|---|---|
| `OBJECT_ALLOCATION_FAILURE` | `EMPTY`; no object, control, reference, publication, or vector entry. |
| `OBJECT_CONSTRUCTION_FAILURE` | reverse partial construction and free raw object exactly once; end `EMPTY`. |
| `CONTROL_BLOCK_ALLOCATION_NULL` | never form/publish `{object, null-control}`; destroy/free object; end `EMPTY`. |
| `CONTROL_BLOCK_CONSTRUCTION_FAILURE` | free partial control, then destroy/free object; end `EMPTY`. |
| `VECTOR_INSERT_FAILURE` | release insert temporaries, prove no vector membership, then release root; end `DESTROYED`. |
| `VECTOR_GROW_OR_COPY_FAILURE` | undo destination acquires, restore original vector, prove no virtual entry, release root; end `DESTROYED`. |
| `INSERT_NOTIFICATION_FAILURE` | close publication, drain callback, remove entry, release callback/local references, then root; end `DESTROYED`. |
| `CALLBACK_REENTRY_OR_ESCAPE` | reject completion and alias escape; end `CONTROL_READY` before publication or fail-closed `RETIRING` afterward. |
| `PUBLICATION_FAILURE` | close admission, drain, remove entry, release temporaries, then root; end `DESTROYED`. |
| `REMOVE_CALLBACK_FAILURE` | admission remains closed; coordinator and vector owners remain; end fail-closed `RETIRING`, no final release. |
| `READER_DRAIN_FAILURE` | retain long-lived owners; end fail-closed `RETIRING`, no removal or destruction. |
| `STOP_OR_SHUTDOWN_EARLY_RETURN` | use the same retire/drain/remove/release path; `DESTROYED` only after all confirmations, otherwise remain fail-closed. |
| `FINAL_RELEASE_FAILURE_OR_WRONG_THREAD` | reject before decrement; retain root; end `UNREGISTERED`, no duplicate release. |

Each trace must prove pre-state, injected operation, ordered rollback actions,
post-state, vector presence, publication/admission status, live reader/callback
counts, per-class reference counts, total strong/weak counts, delete/free counts,
and thread token. A missing row, merged ID, unobserved injection, guessed
unreachability, skipped action, wrong order, or unbalanced reference fails
group 1.

Fail-closed `RETIRING` and `UNREGISTERED` outcomes are not silently repaired by
the test. A group-1 assertion failure ends the one permitted run; there is no
automatic retry.

## 8. Thread and drain contract

The exact thread classes remain:

- `AUTHORITY_MAIN_LIFECYCLE`
- `MANAGER_CALLBACK`
- `READER`

`ThreadToken` must make affinity testable without reading any OS or game process.
Creation, construction, control binding, registration, publication close, drain
completion, removal, rollback, final strong release, object destruction, final
weak release, and control-block free require
`AUTHORITY_MAIN_LIFECYCLE`. Insert/remove callback admissions use
`MANAGER_CALLBACK`; reader admissions use `READER`.

Admission is allowed only in `REGISTERED`. `BeginRetire` atomically closes new
reader and callback admission before waiting for already admitted leases. A
successful drain means protocol-proved zero live `ReaderLease` and
`CallbackLease`, not a sampled counter. `RemoveAndDestroy` cannot remove while a
lease remains and cannot become the final releaser on a reader, callback,
scheduler, IPC, or helper thread. Wrong-token operations fail before mutation or
counter decrement.

## 9. v4 three-axis decision contract

The future decision path and identity are reserved as:

- path: `cs-t0/coop-virtual-device-ownership-lifecycle-decision-v4.json`
- identity: `IsaacCsT0-CoopVirtualDeviceOwnershipLifecycleDecision-v4`
- `createNewOnly=true`

The exact axis vocabularies are:

- `offlineImplementationStatus` in
  `{OFFLINE_IMPLEMENTATION_CONFIRMED, OFFLINE_IMPLEMENTATION_INCOMPLETE}`;
- `nativeEvidenceStatus` in `{STATIC_CONFIRMED, UNKNOWN}`;
- `formalStatus` in `{STATIC_CONFIRMED, UNKNOWN}`.

The v4 candidate may be created only after group 1 passes once and emits its
hash-bound report. For exactly these three proof IDs, the candidate must record:

| Proof ID | `offlineImplementationStatus` | `nativeEvidenceStatus` | `formalStatus` |
|---|---|---|---|
| `VIRTUAL_DEVICE_OBJECT_OWNER` | `OFFLINE_IMPLEMENTATION_CONFIRMED` | `UNKNOWN` | `UNKNOWN` |
| `VIRTUAL_DEVICE_CREATION_DESTRUCTION` | `OFFLINE_IMPLEMENTATION_CONFIRMED` | `UNKNOWN` | `UNKNOWN` |
| `VIRTUAL_DEVICE_REFERENCE_COUNTING` | `OFFLINE_IMPLEMENTATION_CONFIRMED` | `UNKNOWN` | `UNKNOWN` |

`formalStatus=STATIC_CONFIRMED` is derivable only when that row has both
`offlineImplementationStatus=OFFLINE_IMPLEMENTATION_CONFIRMED` and
`nativeEvidenceStatus=STATIC_CONFIRMED`, and every joint owner/state/rollback/
reference/thread gate for that proof is independently closed. Neither design
contract nor offline harness evidence can set `nativeEvidenceStatus`.

This v4 is deliberately non-promoting: `promotionCount=0`, `promotedIds=[]`, the
six inherited `STATIC_CONFIRMED` dependency items remain unchanged, and all ten
v3 unknown items remain `UNKNOWN`. In particular calling thread, vector
insert/remove ownership, teardown/drain/restore, key allocation, complete
vtable/COL/slot ABI, assignment integration, and generation lifecycle are not
upgraded by the offline implementation.

## 10. Fixed conservative boundary

The candidate and both production verification modes must retain:

- `necessityAssessment=REQUIRED_FOR_PHYSICAL_DEVICE_FREE_ENDPOINT_B`;
- `formalDecision=null`;
- `workPackageStatus=BLOCKED_BY_EVIDENCE`;
- `implementationEligibility=false`;
- `csT00506Unlocked=false`, `t005Unlocked=false`, `t006Unlocked=false`;
- `runtimeObserved=false`, `processAccessed=false`, `gameStarted=false`,
  `ipcCreated=false`, `virtualDeviceImplemented=false`.

All eighteen permission fields are present and `false`:

`activeExperimentAllowed`, `activeInputAllowed`, `applyAllowed`,
`automaticLiveRetryAllowed`, `inputMutationAllowed`,
`liveAuthorizationCreated`, `modeLatchAllowed`,
`originalOnlinePositiveAllowed`, `overrideAllowed`,
`passiveHookTransactionAllowed`, `passiveObservationAllowed`,
`processEnumerationAllowed`, `processReadAllowed`, `runtimeMutationAllowed`,
`schedulerAllowed`, `transportAllowed`, `virtualDeviceCreationAllowed`, and
`worldMutationAllowed`.

`OFFLINE_IMPLEMENTATION_CONFIRMED` means only that the isolated module satisfied
the sealed fake-port harness. It does not change `virtualDeviceImplemented`,
authorize a runtime canary, unlock 05-06, or create a formal decision.

## 11. Two create-new output interfaces

### 11.1 Offline harness report

The dedicated entry and sole production group-1 command shape are:

```powershell
pwsh -NoProfile -File .\cs-t0\test-virtual-device-lifecycle-offline.ps1 `
  -Configuration Release `
  -Output <new-json>
```

The canonical one-time output is:

`out/cs-t0/virtual-device-lifecycle-offline-harness-20260908-v1.json`

The report identity is
`IsaacCsT0-VirtualDeviceLifecycleOfflineHarnessReport-v1`. It must bind the
final preseal, contract v1, all four offline module/project files, the harness
entry, exact `Release|Win32` compiler/project configuration, executable hash,
harness case manifest, thirteen rollback traces, state/ref/thread snapshots,
and the single run result. It must explicitly report no process access and no
runtime side effect. The entry neither reads nor binds the game PE.

### 11.2 v4 decision verifier

The future verifier entry, core/tests directory, and identity are:

- `cs-t0/verify-coop-virtual-device-ownership-lifecycle-decision-v4.ps1`
- `cs-t0/tests/virtual_device_ownership_lifecycle_decision_v4/`
- `IsaacCsT0-CoopVirtualDeviceOwnershipLifecycleDecisionVerification-v4`

Its interface is:

```powershell
pwsh -NoProfile -File .\cs-t0\verify-coop-virtual-device-ownership-lifecycle-decision-v4.ps1 `
  -GameExe <PE> `
  -RepositoryRoot . `
  [-Output <new-json>]
```

The canonical group-2 output is:

`out/cs-t0/virtual-device-ownership-lifecycle-verification-20260908-v4.json`

Omitting `-Output` is verify-existing mode and must perform no write.

For both entries, a caller may choose another output only when it is a
nonexistent `.json` whose resolved parent remains under `out/cs-t0` or
`artifacts/cs-t0`. Existing targets, symlink/reparse escapes, path traversal,
alternate data streams, wrong extensions, and paths outside the two roots must
be rejected before build, harness execution, or write. A failed command creates
no output. Neither entry may discover or access a game process.

## 12. Sealed four-group acceptance sequence

The four groups are sequential, single-use gates. No group may be automatically
rerun after PASS or FAIL.

### Group 1 — offline harness, once before candidate

After the four-file implementation, standalone project, entry script, and
independent REVERSE v2 handoff are frozen, TEST invokes the offline entry exactly
once with `-Configuration Release` and the nonexistent canonical harness-report
path. This single run may build only `Release|Win32` for the dedicated
Application and execute the offline harness. It does not read or bind the fixed
game PE; PE hash, machine, and anchor verification belong to the v4 decision
verifier only.

It verifies exact file/build isolation, four APIs, move-only lease semantics,
seven states, six strong classes and total ceiling six, external weak zero,
thread/drain rules, the success lifecycle, all thirteen injected failure/
rollback traces, exactly-once destruction/free, and conservative no-runtime
facts. Exit `0`, a PASS report, and a complete hash manifest are required.

If group 1 fails, TEST stops, preserves the failure, does not rerun, and no v4
candidate is produced. If it passes, the report is sealed by SHA-256 and handed
to mainline.

### Candidate publication boundary

Mainline then creates the v4 candidate exactly once. It binds the group-1 report
and its manifest, this preseal, the three v3 predecessors, the four offline
module/project files, harness entry, independent REVERSE v2 handoff, and the
sealed v4 verifier/tests. This ordering intentionally prevents a candidate /
harness-report hash cycle: group 1 does not read or bind the absent candidate,
while the candidate binds the already complete report.

### Group 2 — production decision create-new, once

TEST invokes the v4 decision verifier once against the fixed disk PE and the
nonexistent canonical v4 verification path. Require exit `0`,
`verificationStatus=PASS`, `workPackageStatus=BLOCKED_BY_EVIDENCE`, exact
candidate/output hashes, fixed PE hash/machine, all candidate-bound inherited
and v4 static anchors, three confirmed offline axes, three unknown native and
formal axes, and the complete conservative boundary.

### Group 3 — production verify-existing, once

Only after group 2 passes, TEST invokes the same entry once without `-Output`.
Require exit `0`, the same status pair and decision digest, deterministic
substantive output, and proof that no file was created or changed.

### Group 4 — one parameterized negative batch

Only after group 3 passes, TEST invokes one sealed parameterized batch over
in-memory or temporary isolated fixtures. Every case must fail nonzero before a
requested output is written. The batch is one group, not one process run per
mutation.

The negative batch must cover all of the following categories:

- wrong PE hash or machine; wrong/missing/extra predecessor, preseal, REVERSE,
  source, project, harness, report, verifier, or test digest; wrong static anchor
  identity/RVA/bytes/target/role; candidate-only or design-only anchor offered as
  native fact;
- missing, extra, renamed, or relocated offline module file; implementation
  added to `IsaacCsT0.Native.vcxproj`, another production project, or
  `build.ps1`; non-Application output; any configuration other than
  `Release|Win32`; build/deploy/load dependency on the game;
- missing, extra, renamed, differently typed, non-`noexcept`, or publicly
  exposed lifecycle API; `Snapshot()` mutation; public admission/transition
  escape; type outside `isaac_cs_t0::virtual_device`;
- copyable `ReaderLease` or `CallbackLease`; wrong constructor shape; missing
  `CallbackKind`; move that duplicates/drops ownership; non-inert moved-from
  lease; invalid-lease release; release outside the port ABI; lease retained
  across drain/removal/destruction or by callback/reader/coordinator state;
- missing, extra, reordered, or shortcut state; invalid API/state mapping;
  publication without object/control ownership; remove before admission close
  and protocol drain; final release before confirmed removal; mutation after
  `DESTROYED`;
- missing/extra strong-reference class; any class above one or total above six;
  `EXTERNAL_WEAK` above zero; manual 16-bit counter arithmetic; overflow,
  underflow, acquire-after-final-release, unmatched release, double release,
  callback escape, or reader escape; zero/multiple object delete or control free;
- each of the thirteen failure IDs missing, merged, renamed, not injected, or
  producing a wrong state, recovery order, vector/publication/drain result,
  reference balance, thread token, delete count, or free count;
- admission after `REGISTERED` is closed; sampled-zero substituted for drain;
  remove with a live lease; wrong-thread mutation/final release; callback or
  reader becoming final releaser;
- `OFFLINE_IMPLEMENTATION_CONFIRMED` without the one sealed group-1 PASS report;
  offline/design/test prose used to set `nativeEvidenceStatus=STATIC_CONFIRMED`;
  `formalStatus=STATIC_CONFIRMED` while either prerequisite axis or a joint gate
  is open; invalid axis enum; wrong proof ID; aggregate/promotion counts that are
  not exactly derived;
- any v3 unknown item promoted, `promotionCount` nonzero, `promotedIds` nonempty,
  inherited six/ten evidence totals changed, necessity changed,
  `formalDecision` non-null, status not blocked, or implementation eligibility
  true;
- any permission, unlock, runtime, process, game, IPC, or
  `virtualDeviceImplemented` field missing or true;
- output outside `out/cs-t0` or `artifacts/cs-t0`, non-JSON output, path/ADS/
  reparse escape, existing-output overwrite, partial output on failure, or a
  verify-existing write.

## 13. Explicit omissions and disposition

No Cargo tests, complete T0 build, production Native build, Debug/Release or
x86/x64 matrix, historical regression, v1/v2/v3 verifier or fixture suite, game
startup, process probe, debugger, injection, passive canary, active canary, IPC,
transport, input mutation, or repeated self-test is permitted by this seal.
Existing historical artifacts are not deleted or rewritten.

At preseal time there is no v4 implementation result, v4 REVERSE v2 result,
group-1 report, v4 candidate, v4 verifier result, or v4 TEST result. No outcome is
presumed. This document freezes only the future rejection boundary and the
single-run ordering needed to assess it without circular evidence.
